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
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "pfaile_dep.h"
#define MAX_TNODES 200

void* get_init_stateI(void)
{
	return ((void*)init_game());
}

void get_moves_for_game_stateI(void** moves, long int* move_orderings, void* state, uint32_t* num_moves)
{
	//uint32_t i, num_moves_rem = 0;
	move* moves_i = (move*)malloc(sizeof(move)*MAX_TNODES);
	int num_moves_int = 0;
	//generate alll possible moves
	gen(moves_i, &num_moves_int, state);	
	//order randomly the possible moves
	order_moves(moves_i, move_orderings, num_moves_int);
	//for each move check if the move is legal
	//for(i = 0; i < num_	
	//set the pointer to point to these moves
	*num_moves = (uint32_t)num_moves_int;
	*moves = moves_i;
}

void* get_state_for_move_and_game_stateI(void* s_from, void* moves, long int* move_orderings, int *ch, uint32_t num_moves)
{
	int i = 0;
	state* s_to = (state*)malloc(sizeof(state));	
	move* m = (move*)moves;
	//copy the old state to the new state
	memcpy(s_to, s_from, sizeof(state));	
	//remove get the number of the best ordering
	assert(remove_one(&i, move_orderings, num_moves));
	*ch = i;
	assert(i < num_moves);
	//make the move 
	make(m, i, s_to);
	return s_to;
}

long int evaluate_game_stateI(void* s)
{
	return eval((state*) s);
}

void free_stateI(void* state)
{
	free(state);
}
//TODO: change the way memory is set for moves
void free_movesI(void* moves)
{
	free(moves);
}

int size_moveI()
{
	return (int)sizeof(move);
}
