#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "locking.h"




/* Exercise 1:
 *     Basic memory barrier
 * 
 * 	Memory barrier is needed on architectures with weakly ordered memory. Generally, there are two types of memory barrier.
 * 	1. Complier Memory Barrier: It only prevents the compiler from reordering instructions, but does not prevent reordering by CPU.
 *	2. Hardware Memory Barrier: 
 *	Intel:
 *		ASM Instruction		Intel(R) C++ Compiler Intrinsic Equivalant
 *		lfence (asm)		void_mm_lfence(void)
 *		sfence (asm)		void_mm_sfence(void)
 *		mfence (asm)		void_mm_mfence(void)
 *	Take mfence for example. It performs a serializing operation on all load-from-memory and store-to-memory instructions that were issued prior the MFENCE instruction. This serializing operation guarantees that every load and store instruction that precedes the MFENCE instruction in program order becomes globally visible before any load or store instruction that follows the MFENCE instruction. 
 *
 * References:
 * [1]. ISA Manual Vol. 2
 * [2]. Memory Ordering. http://en.wikipedia.org/wiki/Memory_ordering 
 * [3]. Difference in fence and ask volatile memory. 
 *	http://stackoverflow.com/questions/12183311/difference-in-mfence-and-asm-volatile-memory
 * [4]. Intel® 64 Architecture Memory Ordering
 * 	http://www.multicoreinfo.com/research/papers/2008/damp08-intel64.pdf
 */

void mem_barrier() {
	asm("mfence");
}


/* Exercise 2: 
 *     Simple atomic operations 
 *	
 *	LOCK operation causes the processor’s LOCK# signal to be asserted during execution of the accompanying instruction (turns the instruction into an atomic instruction). 
 *	The x86 exposes a complex ISA that is implemented as a RISC ISA under the covers. This means that a single x86 instruction is actually implemented as a set of micro-ops (such as load/store). This means that by default, x86 operations are not atomic. 
 *	Intel 64 memory ordering guarantees that for each of the following memory-access instructions, the constituent memory operation appears to execute as a single memory access regardless of memory type:
 *	1. Instructions that read or write a single byte.
 *	2. Instructions that read or write a word (2 bytes) whose address is aligned on a 2 byte boundary.
 *	3. Instructions that read or write a doubleword (4 bytes) whose address is aligned on a 4 byte boundary.
 *	4. Instructions that read or write a quadword (8 bytes) whose address is aligned on an 8 byte boundary.
 *	All locked instructions (the implicitly locked xchg instruction and other read-modify-write instructions with a lock prefix) are indivisible and uninterruptible sequence of load(s) followed by store(s) regardless of memory type and alignment.
 *	Other instructions may be implemented with multiple memory accesses.
 *
 * References:
 * [1]. ISA Manual Vol. 2
 * [2]. Intel® 64 Architecture Memory Ordering
 * 	http://www.multicoreinfo.com/research/papers/2008/damp08-intel64.pdf
 * [3]. GCC-Inline-Assembly-HOWTO
 *	http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s2
 */

void atomic_sub( int * value, int dec_val) {
	//atomic_add(value, -dec_val);
	asm( 
		"lock sub %1, %0;"
		: "+m" (* value)	//Output
		: "r" (dec_val) 	//Input
		: "memory" );		//Changing the content of the memory.
}
 /*
 *References:
 *	[1]. Fetch-and-add. 
 *	http://en.wikipedia.org/wiki/Fetch-and-add#x86_implementation
 *	[2]. ISA Manual Vol. 2
 */
void atomic_add( int * value, int inc_val) {
	asm( 
		"lock add %1, %0;"
		: "+m" (* value)	//Output
		: "r" (inc_val)  	//Input
		: "memory" );		//Changing the content of the memory.
}


/* Exercise 3:
 *     Spin lock implementation
 */


/* Compare and Swap
 * Returns the value that was stored at *ptr when this function was called 
 * ###
 * why shold we return the old value, not a success or fail indicator?
 * ###
 * Sets '*ptr' to 'new' if '*ptr' == 'expected'
 */

unsigned int compare_and_swap(unsigned int * ptr, unsigned int expected,  unsigned int new) {
	int ret;
	asm(
        	"mov %2, %%eax;\n\t"
		"lock cmpxchg %3, %0;\n\t"   //if EAX == '*ptr' then set '*ptr' to 'new' and ZF=1
		"setz %1;\n\t"
		: "+m" (* ptr), "=m" (ret)	//Output
		: "m" (expected), "r" (new)	//Input
		: "eax", "memory" );
	return ret;
	
}

void spinlock_init(struct spinlock * lock) {
	lock->value = 1;
}
 /*
 *References
 *	[1]. Spinlock. http://en.wikipedia.org/wiki/Spinlock
 *	[2]. Spinlock and Read-Write Lokcs.
 *	http://www.lockless.com/articles/locks
 *	[3]. ISA Manual Vol. 2
 */
void spinlock_lock(struct spinlock * lock) {
	while(compare_and_swap(&lock->value, 1, 0) == 0){
		asm("pause\n\t" : : : "memory");
	}
}


void spinlock_unlock(struct spinlock * lock) {
	compare_and_swap(&lock->value, 0, 1);
}


/* Exercise 4:
 *     Barrier operations
 */


/* return previous value */
int atomic_add_ret_prev(int * value, int inc_val) {
	asm(
		"lock xadd %0, %1\n\t"
		: "+r" (inc_val)
		: "m" (*value)
		: "memory"
	);
	return inc_val;
}

void barrier_init(struct barrier * bar, int count) {
	bar->total_runners = count;
	bar->cur_runners = 0;
	bar->sense = 1;
}

 /*
 *	When a new thread arrives,
 *		set toggle off the thread local sense
 * 	if it is the last one: 
 * 		1. reset the barrier
 * 		2. toggle off the bar->sense
 *	else
 *		1. spinloop
 *
 *Other possible implementation:
 *	1. sigaction and sigsuspend
 *	2. system call SYS_futex
 *References:
 *	[1]. Linux Tutorial: POSIX Thread Library. 
 *	http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
 *	[2]. Barriers.
 *	http://www.lockless.com/articles/barriers
 *	[3]. Futex Cheat Sheet
 *	http://www.lockless.com/articles/futex_cheat_sheet
 *	[4]. Barrier Synchronization
 *	http://home.dei.polimi.it/speziale/tech/barrier_synchronization/bs.html
 *	[5]. Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors
 *	http://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html#centralized
 */
void barrier_wait(struct barrier * bar) {
	int sense = 1 - bar->sense;
	int cur_runners = atomic_add_ret_prev(&bar->cur_runners, 1) + 1;
	if(cur_runners == bar->total_runners){
		bar->cur_runners = 0;
		bar->sense = sense;
	}else{
		while(sense != bar->sense){
			asm("pause\t\n" : : : "memory");		
		}
	}
}


/* Exercise 5:
 *     Reader Writer Locks
 */

void rw_lock_init(struct read_write_lock * lock) {
	lock->readers = 0;
	lock->disallow_sub_readers = 0;
	lock->rw_lock = 0;	
}
/**
 * When a reader arrives,
 * 	if some writer is waiting for readers
 * 		spin-wait
 * 	else if the target is in idle state
 * 		set the target to reading state
 * 	else if the target is in reading state
 * 		proceed to reading
 * 	else the target must be in writing state
 * 		spin-wait
 *
 */
void rw_read_lock(struct read_write_lock * lock) {
	while(lock->disallow_sub_readers == 1
		|| compare_and_swap(&lock->rw_lock, 0, 1) == 0 
		|| compare_and_swap(&lock->rw_lock, 1, 1) == 0){
		asm("pause\n\t" : : : "memory");
	}
	atomic_add(&lock->readers, 1);
}

void rw_read_unlock(struct read_write_lock * lock) {
	int cur_readers = atomic_add_ret_prev(&lock->readers, -1) - 1;
	if(cur_readers == 0){//last reader unlock the file
		compare_and_swap(&lock->rw_lock, 1, 0);
	}
}
/*
 * When a writer arrives, 
 * 	if the target is in idle state
 * 		set the target to writing state
 * 	else if the target is in writing state
 * 		spin-wait
 * 	else the target is in reading state
 * 		disallow subsequent readers and spin-wait
 */
void rw_write_lock(struct read_write_lock * lock) {
	while(compare_and_swap(&lock->rw_lock, 0, 2) == 0){
		if(lock->readers > 0){
			lock->disallow_sub_readers = 1;
		}
		asm("pause\n\t" : : : "memory");
	}
}

void rw_write_unlock(struct read_write_lock * lock) {
	compare_and_swap(&lock->rw_lock, 2, 0);
	lock->disallow_sub_readers = 0;
}



/* Exercise 6:
 *      Lock-free queue
 *
 * see: Implementing Lock-Free Queues. John D. Valois.
 *
 * The test function uses multiple enqueue threads and a single dequeue thread.
 *  Would this algorithm work with multiple enqueue and multiple dequeue threads? Why or why not?
 *####
 *The algorithm works with multiple enqueue and multiple dequeue threads.
 *because the dequeue return the head node only if no other threads have dequeue *any node. That is the dequeue algorithm is also thread-safe.
 *####
 *
 *
 * /


/* Compare and Swap 
 * Same as earlier compare and swap, but with pointers 
 * Explain the difference between this and the earlier Compare and Swap function
 * ##
 * Differences: 
 * 1. compare_and_swap_ptr use 64 bit instructions(CMPXCHGQ) and registers(RAX).
 * 2. compare_and_swap_ptr use 32 bit instructions(CMPXCHG) and registers(EAX).
 * ##
 */
uintptr_t compare_and_swap_ptr(uintptr_t * ptr, uintptr_t expected, uintptr_t new) {
	uintptr_t ret;
	asm(
		"mov %2, %%rax;"		//Move 'expected' to EAX
		"lock cmpxchgq %3, %0;"		//if RAX == '*ptr' then set '*ptr' to 'new' and ZF=1
		"setz %1;\n\t"
		: "+m" (* ptr), "=m" (ret)	//Output
		: "m" (expected), "r" (new)	//Input
		: "rax", "memory" 
	);	
	return ret;
}



void lf_queue_init(struct lf_queue * queue) {
	struct node * p = (struct node *) malloc(sizeof(struct node));
	p->next = NULL;
	queue->head = queue->tail = p;	
}

void lf_queue_deinit(struct lf_queue * lf) {
	free(lf);
}
/*
 * References:
 * [1]. John D. Valois. Implementing Lock-free Queues. 
 */ 
void lf_enqueue(struct lf_queue * queue, int val) {
	struct node * q = (struct node *) malloc(sizeof(struct node));
	q->value = val;
	q->next = NULL;
	
	//printf("Enqueuing: %d\n", q->value);
	struct node * p = NULL;
	int succ = 0;
	do{
		p = queue->tail;
		succ = compare_and_swap_ptr((uintptr_t *) &p->next, (uintptr_t) NULL, (uintptr_t) q);//if no other threads have enqueued any node, CMS will succed.
		if(!succ){
		//if other threads have enqueued some nodes.
		//update current tail	
			succ = compare_and_swap_ptr((uintptr_t *) &queue->tail, (uintptr_t) p, (uintptr_t) p->next);
		}
	}while(!succ);
	compare_and_swap_ptr((uintptr_t *) &queue->tail, (uintptr_t) p, (uintptr_t) q);	//update current tail to the newly added node
}
/*
 *	The loop is done to ensure that when an element is dequeued, there is no *	other threads who have dequeue any elements.
 */
int lf_dequeue(struct lf_queue * queue, int * val) {
	struct node * p = NULL;
	do{
		p = queue->head;
		if(p->next == NULL){//empty queue
			return 0;
		}
	}while(compare_and_swap_ptr((uintptr_t *) &queue->head, (uintptr_t) p, (uintptr_t) p->next));
	* val = p->next->value;
	//printf("dequeued: %d\n", * val);
	return 1;
}



