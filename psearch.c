/*
MIT License

Copyright (c) 2000 Adrien M. Regimbald

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <strings.h>
#include <string.h>
#include "pprotos.h"
#include "ptree.c"
#include "pfaile_dep.h"

typedef struct thread_info_t
{
	pthread_t 	t;
	int		t_id;	
	int 		num_nodes_expanded;
	int		num_nodes_discarded;
	int		num_nodes_evaluated;
	//other arguments
} thread_info;


//global thread variables
int num_threads = 1;
tnode* root = NULL;
//search characterisitcs
int max_depth = 3;
//variable denoting if the problem has been solved
bool solved = false;
//nodes_added is used to broadcast to waiting workers
//if new nodes have been added to the priority queue
bool get_more_nodes = true;
pthread_cond_t nodes_added;
pthread_mutex_t get_nodes;
//generic functions for a specific search
//get_moves_for_game_state is recommended to return the moves in order to be visited
void*(*get_init_state)(void);
void (*get_moves_for_game_state)(void**, long int*, void*, uint32_t*);
void*(*get_state_for_move_and_game_state)(void*, void*, long int*, uint32_t);
//evaluate_game_state returns the integer value of this game state
long int (*evaluate_game_state)(void*);
//deallocation functions
void (*free_state)(void*);
void (*free_moves)(void*);
int (*size_move)(void);
//OPEN and CLOSE queues, used to prioritized 
//the nodes to be searched
#define OPEN_CAP_SIZE	10000000
pp_queue* open;
pp_queue* close;
//if any ancestor is prunned then all successors of that ancestor should
//be prunned (incomplete)
bool has_prunned_ancestor(tnode* node)
{
	tnode* p;
	while(node != NULL)
	{
		pthread_rwlock_rdlock(&(node->lock));
		if(node->status == PRUNNED)
		{
			pthread_rwlock_unlock(&(node->lock));
			return true;
		}
		p = node->parent;
		pthread_rwlock_unlock(&(node->lock));
		node = p;
	}
	return false;
}
//caller must make sure there are no race conditions
void free_node(tnode* node)
{
	free_state(node->state);
	free_moves(node->moves);		
	pthread_rwlock_destroy(&(node->lock));
	free(node->ch);
}
//expand_node adds all the children of the given node to the priority queue,
//afterwards threads waiting for nodes for processing are notified of the
//new nodes added to the priority queue
void expand_node(tnode* node)
{
	int i;
	tnode* tn_childs;
	void* moves;
	long int move_ordering[MAX_TNODES];
	int bits, type;
	//get successor states of this node
	get_moves_for_game_state((void**)&moves, (long int*)move_ordering, node->state, &(node->num_ch));
	//allocate holder tnodes for successor states
	tn_childs = (tnode*)malloc(sizeof(tnode)*(node->num_ch)); //FREE remember
	bits = ((int)(ceilf(log2f((float)node->num_ch))));
	type = ((node->type) + 1) % 2;
	//assign pointer to array of tnodes
	node->ch = tn_childs;
	node->num_ch_rem = node->num_ch;
	node->moves = moves;	
	//initialize each successor tnode
	for(i = 0; i < (node->num_ch); i++)
	{
		tnode* child = (tn_childs+i);
		pthread_rwlock_init(&(child->lock), NULL);
		child->depth = node->depth + 1;
		child->type = type;
		child->alpha = node->alpha;
		child->beta = node->beta;	
		child->num_ch = 0;
		child->num_ch_rem = 0;
		child->status = NOT_PRUNNED;
		child->parent = node;	
		child->ch = NULL;
		child->best_ch = -1;
		child->ch_ref = i;
		child->moves = NULL;
		//can be expensive
		child->state = get_state_for_move_and_game_state(node->state, moves, move_ordering, node->num_ch);
		tnode_set_pri(node, child, i, bits);	//need to work for general case
		//one can tell if a node is min or max by depth
	}
	
	pthread_mutex_lock(&get_nodes);
		//add each child to the priority queue
		for(i = 0; i < (node->num_ch); i++)
		{
			ppq_enqueue(open, tn_childs+i);	
		}
		//awake threads to work on new added childs if 
		//these nodes are the only ones on the priority queue
		pthread_cond_broadcast(&nodes_added);
	pthread_mutex_unlock(&get_nodes);
}
//discard_node 
//ASSUMPTIONS
//only called on nodes that have no children, because they have not been expanded
void discard_node(tnode* node)
{
	//set all ancestor up to first prunned ancestor to prunned
	//including this ancestor
	tnode *ancestor = node, *tmp;
	pthread_rwlock_wrlock(&(ancestor->lock));
	//should stop when the first prunned ancestor is found
	while(ancestor->status != PRUNNED) 
	{
		//executed when ancestor is not prunned
		ancestor->status = PRUNNED;
		pthread_rwlock_unlock(&(ancestor->lock));
		ancestor = ancestor->parent;
		pthread_rwlock_wrlock(&(ancestor->lock));
	}
	//executed when ancestor is prunned
	pthread_rwlock_unlock(&(ancestor->lock));

	ancestor = node->parent;
	//now free this node NO need for LOCKS
	free_node(node);

	//**then free ancestor nodes that have no children	
	pthread_rwlock_wrlock(&(ancestor->lock));
	//decrease num children remainig since we freed a children (node)
	ancestor->num_ch_rem--;
	//should stop when the first prunned ancestor is found
	while(ancestor->num_ch_rem == 0 && ancestor->status == PRUNNED) 
	{
		//executed when ancestor is not prunned
		pthread_rwlock_unlock(&(ancestor->lock));
		tmp = ancestor->parent;
		//now free this node NO need for LOCKS
		free_node(ancestor);
		//check next ancestor 
		ancestor = tmp;
		pthread_rwlock_wrlock(&(ancestor->lock));
		//decrease num children remaining since we freed a children (ancestor)
		ancestor->num_ch_rem--;
	}
	//executed when ancestor is prunned
	pthread_rwlock_unlock(&(ancestor->lock));
		
}

void propagate_down_to_leaf_values(tnode* node, long int* alpha, long int* beta, int* prunned)
{
	if(node == root)
	{
		pthread_rwlock_rdlock(&(node->lock));
		*alpha = node->alpha;	
		*beta = node->beta;
		pthread_rwlock_unlock(&(node->lock));
	}
	else
	{
		pthread_rwlock_rdlock(&(node->lock));
		if(node->status == PRUNNED) //stop updating if node is prunned
		{
			pthread_rwlock_unlock(&(node->lock));
			*prunned = 1;
			return;
		}
		pthread_rwlock_unlock(&(node->lock));
		propagate_down_to_leaf_values(node->parent, alpha, beta, prunned);	
		//if any ancestor was prunned then don't update any values
		if(*prunned == 1)
			return;
		//update values with ancestor values
		pthread_rwlock_wrlock(&(node->lock));
			//if use the smallest possible window size
			*alpha = (*alpha > node->alpha) ? *alpha : node->alpha;
			*beta = (*beta < node->beta) ? *beta : node->beta;
			node->alpha = *alpha;
			node->beta = *beta;
		pthread_rwlock_unlock(&(node->lock));
	}
}


//update min and max values and 
void process_leaf(tnode* leaf)
{
	tnode* ancestor, *p;
	long int val = evaluate_game_state(leaf->state);
	long int alpha = 0, beta = 0;
	int prunned = 0;
	//propagate up variables
	int rem_ch = 1, free = 0, stop = 0;
	int ch_ref = -1;
	propagate_down_to_leaf_values(leaf->parent, &alpha, &beta, &prunned);
	//if any node up the tree is found to be prunned then prun this one
	//and every node like discard node actually discard node may be used
	ancestor = leaf->parent;
	ch_ref = leaf->ch_ref;
	//display_board(stdout, 0, leaf->state);
	//go up the tree updating each ancestor with this node's values if possible
	//TODO: check this is done correctly
	//clean up after leaf
	if(prunned == 1)
		discard_node(leaf);
	else
		free_node(leaf);

	while(ancestor != root)
	{
		pthread_rwlock_wrlock(&(ancestor->lock));
		//if the last ancestor was a leaf node or it had zero children
		//then update the last ancestors parent
		if(rem_ch == 1)
		{
			assert(ancestor->num_ch_rem > 0);
			ancestor->num_ch_rem--;
		}
		else
		{
			assert(ancestor->num_ch_rem > 0); 	
		}
		assert(ancestor->num_ch_rem >= 0);
		//if after update the node has zero children
		if(ancestor->num_ch_rem == 0)
		{
			rem_ch = 1;
			free = 1;
		}
		else
		{
			rem_ch = 0;
			free = 0;
		}
		//should also SAVE THE BEST MOVE to be done later
		//update alpha and beta
		if(ancestor->type == max)
		{
			if(val > ancestor->alpha)
			{
				ancestor->alpha = val;
				ancestor->best_ch = ch_ref;
			}
			if(ancestor->beta < ancestor->alpha)
				ancestor->status = PRUNNED;
			val = ancestor->alpha;
		}
		else //ancestor is min
		{
			if(val < ancestor->beta)
			{
				ancestor->beta = val;
				ancestor->best_ch = ch_ref;
			}
			if(ancestor->beta < ancestor->alpha)
				ancestor->status = PRUNNED;
			val = ancestor->beta;
		}
		//the position of this ancestor with respect to it's father
		ch_ref = ancestor->ch_ref;
		//free ancestor node
		if(free)
		{
			p = ancestor->parent;
			pthread_rwlock_unlock(&(ancestor->lock));
			free_node(ancestor);
			ancestor = p;
		}
		else
		{
			p = ancestor->parent;
			pthread_rwlock_unlock(&(ancestor->lock));
			ancestor = p;
		}	
	}

	//ancestor is ROOT different processing at root
	assert(ancestor != NULL);
	pthread_rwlock_wrlock(&(ancestor->lock));
	//check if rem_ch
	if(rem_ch == 1)
	{
		assert(ancestor->num_ch_rem > 0);
		ancestor->num_ch_rem--;
	}
	else
	{
		assert(ancestor->num_ch_rem > 0); 	
	}
	//if the root does not have anymore childs remaining then stop all threads
	//the problem has been searched
	if(ancestor->num_ch_rem == 0)
	{
		stop = 1;	
	}
	
	//update values at the root		
	if(val > ancestor->alpha)
	{
		ancestor->alpha = val;	
		ancestor->best_ch = ch_ref;
	}
	pthread_rwlock_unlock(&(ancestor->lock));
	
	//search tree has been fully explored
	if(stop == 1)
	{
		pthread_mutex_lock(&(get_nodes));
			solved = true;
		pthread_cond_broadcast(&nodes_added);
		pthread_mutex_unlock(&(get_nodes));
	}
}

//ab_search is performed by all workers threads to work on nodes 
//added to the priority queue.
void* ab_search(void *args)
{
	int stop = 0;	
	//work while there is work to be done else finish
	//until every branch of the root node is searched, then 
	//the search will be done and all thread will return
	thread_info* t_info = (thread_info*)args;
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
				printf("hello\n");
				//printf("num_exp = %d, num_disc = %d, num_val %d\n", 
				pthread_cond_wait(&nodes_added, &get_nodes);
				//should elminiate the spinlock on ppq_dequeue
				node = ppq_dequeue(open);
			}
			//if(node != NULL)
			//tnode_print_pri(node->pri, node->pri_bits);
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
			t_info->num_nodes_discarded++;
			discard_node(node);	
		}
		//else if the node is a leaf (and no ancestor has been prunned)
		//then process the leaf node (MOST WORK HERE)
		else if(node->depth == max_depth)
		{
			t_info->num_nodes_evaluated++;
			process_leaf(node);
		}
		//else expand the node and add it's nodes to the OPEN parallel queue
		//add itself to CLOSE parallel queue (EASY)
		else
		{
			t_info->num_nodes_expanded++;
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
	//if((tc_ret = pthread_rwlock_init(&solved_lock, NULL)))
	//	error_shutdown("workers solved rwlock init", tc_ret);
	//create nodes_added condition variable
	if((tc_ret = pthread_cond_init(&nodes_added, NULL)))
		error_shutdown("workers nodes added cond init", tc_ret);
	//create get_nodes mutex
	if((tc_ret = pthread_mutex_init(&get_nodes, NULL)))
		error_shutdown("workers get_nodes mutex init", tc_ret);
	//create the OPEN parallel priority queue
	open = ppq_create(OPEN_CAP_SIZE, pri_compare);
	workers = (thread_info*)malloc(sizeof(thread_info)*num_threads);
	workers->num_nodes_evaluated = 0;
	workers->num_nodes_expanded = 0;
	if(workers == NULL)
		error_shutdown("workers malloc", -1);		
	//add the ROOT tnode
	root = tnode_init();
	root->type = max;
	root->depth = 0;
	root->alpha = LONG_MIN;
	root->beta = LONG_MAX;
	root->num_ch_rem = 0;
	root->num_ch = 0;
	root->status = NOT_PRUNNED;
	root->pri_bits = 0;
	bzero((void*)(root->pri),sizeof(uint64_t)*MAX_PRI_SIZE);
	root->state = get_init_state();
	root->moves = NULL;
	root->parent = NULL;
	root->ch = NULL;
	root->best_ch = -1;
	root->ch_ref = -1;
	ppq_enqueue(open, root);
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

void* finally()
{
	//get size of move in bytes
	int size = size_move(); 		
	assert(size > 0);
	assert((root->best_ch) > 0);
	void* best_move = (void*)malloc(size);
	memcpy(best_move, ((root->moves)+(size*(root->best_ch))), size);
	//free the root
	free_node(root);		
	return best_move;			
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
	return finally();
}

