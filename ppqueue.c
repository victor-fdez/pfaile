#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "pprotos.h"
#define root 0
//parent can't be used by root
#define parent(i) (((i)-1)>>1)
#define left(i)	  (((i)*2)+ 1)
#define right(i)  (((i)*2)+ 2)
#define heap_ele(ppq, i)	*(((ppq)->heap) + i)

//for testing purposes...
int comparefun(void* int_a, void* int_b)
{
	int a = *(int*)int_a, b = *(int*)int_b; 	
	if(a > b)
		return 1;
	else if(a < b)
		return -1;
	else
		return 0;
}

void ppq_test(void)
{
	int i;
	pp_queue* ppq = ppq_create(100, &comparefun);
	printf("\nenqueuing...\n");
	for(i = 0; i < 20; i++)
	{
		int *e = (int*)malloc(sizeof(int));
		*e = rand() % 30;
		printf("%d ", *e);
		ppq_enqueue(ppq, e);
	}
	printf("\ndequeuing...\n");
	for(i = 0; i < 30; i++)
	{
		int *e = ppq_dequeue(ppq);
		if(e)
		{
			printf("%d ", *e);
			free(e);
		}
		else
		{
			printf("X ");
		}
	}
	printf("\nenqueuing...\n");
	for(i = 0; i < 10; i++)
	{
		int *e = (int*)malloc(sizeof(int));
		*e = rand() % 30;
		printf("%d ", *e);
		ppq_enqueue(ppq, e);
	}
	printf("\ndequeuing...\n");
	for(i = 0; i < 15; i++)
	{
		int *e = ppq_dequeue(ppq);
		if(e)
		{
			printf("%d ", *e);
			free(e);
		}
		else
		{
			printf("X ");
		}
	}
	printf("\n");
	ppq_free(ppq);
}

//it will create a queue of
pp_queue* ppq_create(int cap, compare compare_func)
{
	pp_queue* ppq;
	int 	  ret;
	if((ppq = (pp_queue*)malloc(sizeof(pp_queue))) == NULL)
		error_shutdown("pp_queue malloc", -1);	
	if((ppq->heap = (void**)malloc(sizeof(void*) * cap)) == NULL)
		error_shutdown("pp_queue heap malloc", -1);
	ppq->size = 0;
	ppq->cap = cap;
	ppq->compare_func = compare_func;
	if((ret = pthread_spin_init(&(ppq->s_lock), PTHREAD_PROCESS_PRIVATE)))
		error_shutdown("pp_queue spin lock init", ret);
	return ppq;
}

//enqueues an element to the parallel priority queue
void ppq_enqueue(pp_queue* ppq, void* e)
{
	int i, ret; 
	void** p;
	if((ret = pthread_spin_lock(&(ppq->s_lock))))	
		error_shutdown("pp_queue enqueue spin lock lock", ret);
	//enqueue element in min queue
	ppq->size++;
	//initial position of node in head
	i = ppq->size - 1;
	while(i != root)
	{
		p = ppq->heap + parent(i);
		if(ppq->compare_func(*p, e) > 0)
		{
			heap_ele(ppq, i) = *p;
		}
		else
		{
			break;	
		}
		i = parent(i);
	}	
	
	heap_ele(ppq, i) = e;
	if((ret = pthread_spin_unlock(&(ppq->s_lock))))
		error_shutdown("pp_queue enqueue spin lock unlock", ret);
}

//dequeues and element from the paralle priority queue
void* ppq_dequeue(pp_queue* ppq)
{
	int i, l, r, smallest, last;
	int ret;
	void *tmp, *min = NULL;
	compare func;

	if((ret = pthread_spin_lock(&(ppq->s_lock))))	
		error_shutdown("pp_queue enqueue spin lock lock", ret);

	if(ppq->size)
	{
		last = (--ppq->size);
		min = heap_ele(ppq, 0);
		//change the last element to be the min element
		heap_ele(ppq, 0) = heap_ele(ppq, last);
		//setup some variables before beginning to heapify
		i = 0;
		func = ppq->compare_func;
		while(1)
		{
			l = left(i);
			r = right(i);
			if(l < last && (func(heap_ele(ppq, l), heap_ele(ppq, i)) < 0))
				smallest = l;	
			else
				smallest = i;
			
			if(r < last && (func(heap_ele(ppq, r), heap_ele(ppq, smallest)) < 0))
				smallest = r;	
			
			if(smallest == i)
				break;

			tmp = heap_ele(ppq, i);
			heap_ele(ppq, i) = heap_ele(ppq, smallest);
			heap_ele(ppq, smallest) = tmp;
			i = smallest;
		}	
	}

	if((ret = pthread_spin_unlock(&(ppq->s_lock))))
		error_shutdown("pp_queue enqueue spin lock unlock", ret);

	return min;
}

void ppq_free(pp_queue* ppq)
{
	int ret;
	//free up memory can only be called by one thread for a specific
	//parallel priority queue else behaviour is undefined
	if((ret = pthread_spin_destroy(&(ppq->s_lock))))
		error_shutdown("pp_queue spin lock destroy", ret);
	free(ppq->heap);
	free(ppq);
}
