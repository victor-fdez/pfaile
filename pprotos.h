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
#ifndef __P_PROTOS_H__
#define __P_PROTOS_H__
#include <pthread.h>
#include <stdint.h>
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
extern void*(*get_init_state)(void);
extern void (*get_moves_for_game_state)(void**, long int*, void*, uint32_t*);
extern void*(*get_state_for_move_and_game_state)(void*, void*, long int*, int*, uint32_t);
//evaluate_game_state returns the integer value of this game state
extern long int  (*evaluate_game_state)(void*);
//deallocation functions
extern void (*free_state)(void*);
extern void (*free_moves)(void*);
extern int (*size_move)(void);
#endif
