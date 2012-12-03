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
#ifndef __FAILE_DEP_H__
#define __FAILE_DEP_H__

#include <stdint.h>

/* define names for piece constants: */
#define frame   0
#define wpawn   1
#define bpawn   2
#define wknight 3
#define bknight 4
#define wking   5
#define bking   6
#define wrook   7
#define brook   8
#define wqueen  9
#define bqueen  10
#define wbishop 11
#define bbishop 12
#define npiece  13

#define rank(square) ((((square)-26)/12)+1)
#define file(square) ((((square)-26)%12)+1)
typedef struct 
{
  bool ep;
  int from;
  int target;
  int captured;
  int promoted;
  int castled;
  int cap_num;
} move;

//state information will be approximately 512bytes size
typedef struct
{
	move* m
	uint8_t board[144];
	uint8_t moved[144];
	uint8_t pieces[33];
	uint8_t	squares[144]
} state;

char divider[50] = "-------------------------------------------------";
//faile functions

extern void test_faile();
extern void init_game (state* s);

//alpha beta search interface
extern void* get_init_stateI(void);
extern void get_moves_for_game_stateI(void** moves, long int* move_orderings, void* state, uint32_t* num_moves);
extern void* get_state_for_move_and_game_stateI(void* state, void* moves, long int* move_orderings, uint32_t num_moves);
extern void free_stateI(void* state);
extern void free_movesI(void* moves);
extern void size_moveI();

#endif
