#ifndef __P_PROTOS_H__
#define __P_PROTOS_H__
#include <pthread.h>
//parallel priority queue
typedef int (*compare)(void*, void*);
typedef struct
{
	void** heap;
	int size;
	int cap;
	//operations should be short
	pthread_spinlock_t s_lock;
	compare compare_func;
}pp_queue;

//ppqueue.c parallel queue implementation
extern void ppq_test(void);
extern pp_queue* ppq_create(int cap, compare);
extern void ppq_enqueue(pp_queue* ppq, void* e);
extern void* ppq_dequeue(pp_queue* ppq);
extern void ppq_free(pp_queue* ppq);

//putils.c other utility functions
extern inline void error_shutdown(char* msg, int error_val);

//psearch.c alpha beta parallel search function
extern void* think(void);

#endif
