#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "pprotos.h"
#include "ptree.c"

typedef struct thread_info_t
{
	pthread_t 	t;
	int		t_id;	
	//other arguments
} thread_info;


//global thread variables
int num_threads = 4;
bool solved = false;
pthread_rwlock_t solved_lock;
//OPEN and CLOSE queues, used to prioritized 
//the nodes to be searched
#define OPEN_CAP_SIZE	10000000
pp_queue* open;
pp_queue* close;


//ab_search is performed by all workers threads to work on nodes 
//added to the priority queue.
void* ab_search(void *args)
{
	
	//work while there is work to be done else finish
	//until every branch of the root node is searched, then 
	//the search will be done and all thread will return
	thread_info* t_info = (thread_info*)args;	
	//printf("thread %d : reporting for duty\n", t_info->t_id);
	//while the problem is not solved
	pthread_rwlock_rdlock(&solved_lock);	
	while(!solved)	
	{
		pthread_rwlock_unlock(&solved_lock);	
		//no locks are hold -- HERE --
		//get a node from the OPEN parallel queue

		//move up the tree (locking each ancestor)
		//and check if any ancestor is prunned, if any is, then 
		//prun all ancestors visited up until this point
		//and discard this node.
		if(t_info->t_id == 3)
		{
			pthread_rwlock_wrlock(&solved_lock);
			solved = true;
			pthread_rwlock_unlock(&solved_lock);
		}
		
		//else if the node is a leaf (and no ancestor has been prunned)
		//then process the leaf node
		
		//else expand the node and add it's nodes to the OPEN parallel queue
		//add itself to CLOSE parallel queue
		
		//check whether the problem has been solved
		//no locks are hold -- HERE --
		pthread_rwlock_rdlock(&solved_lock);	
	}	
	pthread_rwlock_unlock(&solved_lock);	
	return NULL;	
}

thread_info* workers;
	
void init_workers(void)
{
	int i, tc_ret;
	//create solved lock
	if((tc_ret = pthread_rwlock_init(&solved_lock, NULL)))
		error_shutdown("workers solved rwlock init", tc_ret);
	//create the OPEN parallel priority queue
	tnode_test();	
	open = ppq_create(OPEN_CAP_SIZE, pri_compare);
	workers = (thread_info*)malloc(sizeof(thread_info)*num_threads);
	if(workers == NULL)
		error_shutdown("workers malloc", -1);		
	//run num_threads 
	for(i = 0; i < num_threads; i++)
	{
		workers[i].t_id = i;
		tc_ret = pthread_create(&(workers[i].t), NULL, &ab_search, &(workers[i])); 
		if(tc_ret)
			error_shutdown("pthread_create", tc_ret);
	}
}

void rest_workers()
{
	int i, tj_ret;
	void* val_ret;
	//join all threads
	for(i = 0; i < num_threads; i++)
	{
		tj_ret = pthread_join(workers[i].t, &val_ret);
		if(tj_ret)
			error_shutdown("pthread_join", tj_ret);
	}	
}

void* think(void)
{
	//parent thread sets up priority queue and root node
	
	//parent thread initializes worker threads to begin reading from priority queue 
	init_workers();	
	//parent thread checks to find if workers threads are done or if time is up
	//parent threads join and free all workers threads
	rest_workers();	
	//best move is returned to main
	return NULL;
}

