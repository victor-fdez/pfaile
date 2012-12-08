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

#define rwlock_r(node) 	(pthread_rwlock_rdlock(&((node)->lock)))
#define rwlock_w(node) 	(pthread_rwlock_wrlock(&((node)->lock)))
#define rwlock_u(node) 	(pthread_rwlock_unlock(&((node)->lock)))
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
//before max_depth was 7
int max_depth = 8;
int num_nodes_expanded;
int num_nodes_discarded;
int num_nodes_evaluated;
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
void*(*get_state_for_move_and_game_state)(void*, void*, long int*, int* ch, uint32_t);
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

//has_prunned_ancestor
//goes up the tree and find a prunned node, if one is found
//then every child visited will be set to prunned, else if 
//no prunned child is found then just return false
void has_prunned_ancestor_recur(tnode* node, int* prunned){
	tnode* parent = NULL;
	if(node != NULL){
		rwlock_r(node);
			if(node->status == PRUNNED)	
				*prunned = PRUNNED;
			else
				parent = node->parent;
		rwlock_u(node);
		//don't recurse up if this node is prunned
		if(*prunned == PRUNNED)
			return;	
		else
			has_prunned_ancestor_recur(parent, prunned);
		//if an ancestor was prunned then prun this node	
		rwlock_w(node);
			if(*prunned == PRUNNED)
				node->status = PRUNNED;
		rwlock_u(node);
	}
}
inline bool has_prunned_ancestor(tnode* node)
{
	int prunned = 0;
	tnode* parent = NULL;
	if(node != NULL)
	{
		//may not need a lock since, the node will not change parent
		//until it is erase, but that can't happen.
		parent = node->parent;
		has_prunned_ancestor_recur(parent, &prunned);
		if(prunned == PRUNNED)
			return true;	
	}
	return false;
}
//caller must make sure there are no race conditions
void free_node(tnode* node)
{
	if(node->state != NULL)
		free_state(node->state);
	if(node->moves != NULL)
		free_moves(node->moves);		
	pthread_rwlock_destroy(&(node->lock));
	if(node->ch != NULL)
		free(node->ch);
}
//expand_node adds all the children of the given node to the priority queue,
//afterwards threads waiting for nodes for processing are notified of the
//new nodes added to the priority queue
void expand_node(tnode* node)
{
	int i;
	int ch;
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
		child->moves = NULL;
		//gets the best first
		child->state = get_state_for_move_and_game_state(node->state, moves, move_ordering, &ch, node->num_ch);
		child->ch_ref = ch;
		tnode_set_pri(node, child, i, bits);	//need to work for general case
		//one can tell if a node is min or max by depth
		assert(child->type != node->type);
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

typedef struct
{
	long int alpha;
	long int beta;
	int prunned;
} propagate;

///propagate_values
void propagate_values(tnode* node, propagate* p){
	//before node is null it must be root therefore
	if(node != root)
	{
		//TODO: check if the node is prunned (try both approaches w and r)
		//and update both alpha and beta
		rwlock_r(node);
			if(node->status == PRUNNED)
				p->prunned = PRUNNED;	
		rwlock_u(node);
		//if the node is prunned then return
		if(p->prunned == PRUNNED)
			return;
		
		//RECURSIVE CALL UP THE TREE
		propagate_values(node->parent, p);

		//if any ancestor was prunned then return
		if(p->prunned == PRUNNED)	
		{
			rwlock_w(node);
				node->status = PRUNNED;
			rwlock_u(node);	
		}
		else
		{
			rwlock_w(node);
				(p->alpha) = ((p->alpha) > (node->alpha)) ? (p->alpha) : (node->alpha);
				(p->beta) = ((p->beta) < (node->beta)) ? (p->beta) : (node->beta);
				node->alpha = (p->alpha);
				node->beta = (p->beta);
				if((node->beta) <= (node->alpha))
				{
					node->status = PRUNNED;
					p->prunned = PRUNNED;
				}
			rwlock_u(node);
		}
			
	}	
	//return whenever root is found
	else
	{
		assert(node == root);
		//root is never prunned
		rwlock_r(node);
			p->alpha = node->alpha;
			p->beta = node->beta;
			assert(node->beta == LONG_MAX);
		rwlock_u(node);
	}
}

inline void remove_node(tnode* node, int val)
{
	tnode* ancestor, *p;
	//if this node is min then return the smallest possible value, since
	//this value will not be chosen by max parent, and vise-versa.
	//propagate up variables
	int free = 0, stop = 0;
	int ch_ref = node->ch_ref;
	ancestor = node->parent;
	//free node, is either a node to be discarded or a leaf node just evaluated
	free_node(node);

	while(ancestor != root) {
		//TODO: goes up until the 
		rwlock_w(ancestor);

		assert(ancestor->num_ch_rem > 0);
		ancestor->num_ch_rem--;
		
		//if after update the node has zero children
		if(ancestor->num_ch_rem == 0){
			free = 1;
		}
		else{
			free = 0;
		}
			
		if(ancestor->type == max){
			if(val > ancestor->alpha){
				ancestor->alpha = val;
				ancestor->best_ch = ch_ref;
			}
			val = ancestor->alpha;
		}
		else{
			if(val < ancestor->beta){
				ancestor->beta = val;
				ancestor->best_ch = ch_ref;
			}
			val = ancestor->beta;
		}	
		//even if this ancestor is prunned it's alpha or beta
		//should be propagated
		if(ancestor->beta <= ancestor->alpha){
			ancestor->status = PRUNNED;
		}

		ch_ref = ancestor->ch_ref;	
		//free ancestor node
		if(free){
			p = ancestor->parent;
			rwlock_u(ancestor);
			free_node(ancestor);
			ancestor = p;
		}
		else{
			rwlock_u(ancestor);
			return;
		}	

	}

	assert(ancestor != NULL);
	rwlock_w(ancestor);
		assert(ancestor->num_ch_rem > 0);
		ancestor->num_ch_rem--;

		if(val > ancestor->alpha){
			ancestor->alpha = val;	
			ancestor->best_ch = ch_ref;
		}	
		//if the root does not have anymore childs remaining then stop all threads
		//the problem has been searched
		if(ancestor->num_ch_rem == 0){
			stop = 1;	
		}
	rwlock_u(ancestor);

	if(stop == 1){
		pthread_mutex_lock(&(get_nodes));
			solved = true;
		pthread_cond_broadcast(&nodes_added);
		pthread_mutex_unlock(&(get_nodes));
	}

}
inline void discard_node(tnode* leaf){
	int val;	
	val = leaf->type == min ? LONG_MIN : LONG_MAX;
	remove_node(leaf, val);
}
///process_leaf
///propagate game evaluation up the tree, and discard of useless nodes
inline void process_leaf(tnode* leaf){
	//tnode* ancestor, *p;
	int val;
	propagate p;
	p.alpha = 0;
	p.beta = 0;
	val = evaluate_game_state(leaf->state);
	p.prunned = 0;
	propagate_values(leaf->parent, &p);
	remove_node(leaf, val);	
}

//ab_search is performed by all workers threads to work on nodes 
//added to the priority queue.
void* ab_search(void *args){
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
	
void init_workers(void* s)
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
	root->state = s;
	root->moves = NULL;
	root->parent = NULL;
	root->ch = NULL;
	root->best_ch = -1;
	root->ch_ref = -1;
	ppq_enqueue(open, root);
	solved = false;
	num_nodes_expanded = 0;
	num_nodes_discarded = 0;
	num_nodes_evaluated = 0;
	//run num_threads 
	for(i = 0; i < num_threads; i++)
	{
		workers[i].t_id = i;
		workers[i].num_nodes_expanded = 0;
		workers[i].num_nodes_discarded = 0;
		workers[i].num_nodes_evaluated = 0;
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
		num_nodes_expanded += workers[i].num_nodes_expanded;
		num_nodes_discarded += workers[i].num_nodes_discarded;
		num_nodes_evaluated += workers[i].num_nodes_evaluated;
		if(tj_ret)
			error_shutdown("pthread_join", tj_ret);
	}	
	ppq_free(open);	
	free(workers);
}

void print_stats()
{
	printf("number of nodes expanded \t%d\n", num_nodes_expanded);
	printf("number of nodes discarded\t%d\n", num_nodes_discarded);
	printf("number of nodes evaluated\t%d\n", num_nodes_evaluated); 
}

void* finally()
{
	//get size of move in bytes
	int size = size_move(); 		
	assert((root->best_ch) >= 0);
	if(root->best_ch < 0)
		exit(-1);
	void* best_move = (void*)malloc(size);
	memcpy(best_move, ((root->moves)+(size*(root->best_ch))), size);
	//free the root
	free_moves(root->moves);
	free(root->ch);
	free(root);
	return best_move;			
}

void* think(void* s)
{
	//parent thread sets up priority queue and root node
	
	//parent thread initializes worker threads to begin reading from priority queue 
	init_workers(s);	
	//parent thread checks to find if workers threads are done or if time is up
	//parent threads join and free all workers threads
	rest_workers();	
	//best move is returned to main
	print_stats();
	return finally();
}

