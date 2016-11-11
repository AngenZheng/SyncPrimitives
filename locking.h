#include <stdint.h>

/* Simple atomic operations */
void atomic_sub(int * dst, int dec_value);
void atomic_add(int * dst, int add_value);



/* Spin locks */
struct spinlock {
	int value;
};

unsigned int compare_and_swap(unsigned int * ptr, unsigned int expected, unsigned int new);

void spinlock_init(struct spinlock * lock);
void spinlock_lock(struct spinlock * lock);
void spinlock_unlock(struct spinlock * lock);

/* Barriers */
int atomic_add_ret_prev(int * dst, int inc_value);
struct barrier {
	int total_runners;
	int cur_runners;
	int sense;
};


void barrier_init(struct barrier * bar, int  count);
void barrier_wait(struct barrier * bar);

/* Reader Writer Locks */
struct read_write_lock {
	//current number of readers
	int readers;
	//when a writer arrive, disallow subsequent readers
	int disallow_sub_readers;
	// 0 idle, 1 reading, 2 writing
	int rw_lock;
};
void rw_lock_init(struct read_write_lock * lock);
void rw_read_lock(struct read_write_lock * lock);
void rw_read_unlock(struct read_write_lock * lock);
void rw_write_lock(struct read_write_lock * lock);
void rw_write_unlock(struct read_write_lock * lock);




/* Lock Free Queue */

struct node {
	struct node * next;
	int value;
};

struct lf_queue {
	struct node * head;
	struct node * tail;
};

uintptr_t compare_and_swap_ptr(uintptr_t * ptr, uintptr_t expected, uintptr_t new);

void lf_queue_init(struct lf_queue * queue);
void lf_queue_deinit(struct lf_queue * queue);
void lf_enqueue(struct lf_queue * queue, int new_val);
int lf_dequeue(struct lf_queue * queue, int * val);

