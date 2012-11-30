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
//search characterisitcs
int max_depth = 5;
//variable denoting if the problem has been solved
bool solved = false;
pthread_rwlock_t solved_lock;
//nodes_added is used to broadcast to waiting workers
//if new nodes have been added to the priority queue
bool get_more_nodes = true;
pthread_cond_t nodes_added;
pthread_mutex_t get_nodes;
//generic functions for a specific search
//get_moves_for_game_state is recommended to return the moves in order to be visited
void (*get_successor_states_for_game_state)(void***, void*, int*);
//evaluate_game_state returns the integer value of this game state
int   (*evaluate_game_state)(void*);

//OPEN and CLOSE queues, used to prioritized 
//the nodes to be searched
#define OPEN_CAP_SIZE	10000000
pp_queue* open;
pp_queue* close;
//if any ancestor is prunned then all successors of that ancestor should
//be prunned (incomplete)
bool has_prunned_ancestor(tnode* node)
{
	while(node != NULL)
	{
		if(node->parent == NULL)
			return false;	
		
		pthread_rwlock_rdlock(&(node->lock));
		if(node->status == PRUNNED)
		{
			pthread_rwlock_unlock(&(node->lock));
			return true;
		}
		node = node->parent;
		pthread_rwlock_unlock(&(node->lock));
	}
	return false;
}

void expand_node(tnode* node)
{
	int i;
	tnode* tn_childs;
	void* childs[MAX_TNODES];
	int bits, type;
	//get successor states of this node
	get_successor_for_game_state(&(childs), node->state, &(node->num_ch));
	//allocate holder tnodes for successor states
	tn_childs = (tnode*)malloc(sizeof(tnode)*(node->num_ch));
	bits = ((int)(ceilf(logf((float)node->num_ch))));
	type = ((node->type) + 1) % 2;
	//assign pointer to array of tnodes
	node->ch = tn_childs;
	//initialize each successor tnode
	for(i = 0; i < (node->num_ch); i++)
	{
		tnode* child = (tn_child+i);
		pthread_rwlock_init(&(child->lock), NULL);
		child->parent = node;	
		child->ch = NULL;
		child->num_ch = 0;
		child->num_ch_rem = 0;
		child->status = NOT_PRUNNED;
		tnode_set_pri(node, child, i, bits);	//need to work for general case
		//one can tell if a node is min or max by depth
		child->depth = node->depth + 1;
		child->type = type;
	}
	
	pthread_mutex_lock(&get_nodes);
		//add each child to the 
		for(i = 0; i < (node->num_ch); i++)
		{
			ppq_enqueue(open, childs+i);	
		}
		pthread_cond_broadcast(&nodes_added);
	pthread_mutex_unlock(&get_nodes);
}
//discard_node free the state of the give node
void discard_node(tnode* node)
{


}

void process_leaf(tnode* leaf)
{


}

//ab_search is performed by all workers threads to work on nodes 
//added to the priority queue.
void* ab_search(void *args)
{
	int stop = 0;	
	//work while there is work to be done else finish
	//until every branch of the root node is searched, then 
	//the search will be done and all thread will return
	//thread_info* t_info = (thread_info*)args; (BEWARE)
	//printf("thread %d : reporting for duty\n", t_info->t_id);
	//while the problem is not solved
	while(true)	
	{
		tnode* node;
		//get a node from the OPEN parallel queue
		node = NULL;
		pthread_mutex_lock(&get_nodes);
			node = ppq_dequeue(open);
			while(node == NULL && !solved)	
			{
				//wakes from either a broadcast from either
				//the main thread that either found a solution
				//or a thread that added more nodes to the 
				//priority queue
				pthread_cond_wait(&nodes_added, &get_nodes);
				//should elminiate the spinlock on ppq_dequeue
				node = ppq_dequeue(open);
			}
			if(solved)
				stop = 1;
		pthread_mutex_unlock(&get_nodes);
		//if the problem has been solved (still might have a children)
		if(stop)
			break;	
		//move up the tree (locking each ancestor)
		//and check if any ancestor is prunned, if any is, then 
		//prun all ancestors visited up until this point
		//and discard this node. (EASY)
		if(has_prunned_ancestor(node->parent))
		{
			discard_node(node);	
		}
		//else if the node is a leaf (and no ancestor has been prunned)
		//then process the leaf node (MOST WORK HERE)
		else if(node->depth == max_depth)
		{
			process_leaf(node);
		}
		//else expand the node and add it's nodes to the OPEN parallel queue
		//add itself to CLOSE parallel queue (EASY)
		else
		{
			expand_node(node);	
		}	
	}	
	return NULL;	
}

thread_info* workers;
	
void init_workers(void)
{
	int i, tc_ret;
	//create solved lock
	if((tc_ret = pthread_rwlock_init(&solved_lock, NULL)))
		error_shutdown("workers solved rwlock init", tc_ret);
	//create nodes_added condition variable
	if((tc_ret = pthread_cond_init(&nodes_added, NULL)))
		error_shutdown("workers nodes added cond init", tc_ret);
	//create get_nodes mutex
	if((tc_ret = pthread_mutex_init(&get_nodes, NULL)))
		error_shutdown("workers get_nodes mutex init", tc_ret);
	//create the OPEN parallel priority queue
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
	ppq_free(open);	
	free(workers);
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

