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
#include <stdio.h>
#include <stdbool.h>

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

/* castle flags: */
#define no_castle  0
#define wck        1
#define wcq        2
#define bck        3
#define bcq        4

/* result flags: */
#define no_result      0
#define stalemate      1
#define white_is_mated 2
#define black_is_mated 3
#define draw_by_fifty  4
#define draw_by_rep    5

/* hash flags: */
#define no_info    0
#define avoid_null 1
#define exact      2
#define u_bound    3
#define l_bound    4

#define rank(square) ((((square)-26)/12)+1)
#define file(square) ((((square)-26)%12)+1)

typedef struct {
  unsigned long int x1;
  unsigned long int x2;
} d_long;

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
	uint8_t board[144];
	uint8_t moved[144];
	uint8_t pieces[33];
	uint8_t	squares[144];
	uint8_t white_to_move;
	uint8_t wking_loc;
	uint8_t bking_loc;
	uint8_t white_castled;
	uint8_t black_castled;
	uint8_t piece_count;
	uint8_t num_pieces;
	uint8_t captures;
	uint8_t ep_square;
	d_long cur_pos;
} state;

extern char divider[50]; 
extern uint8_t start_piece_count;
//faile_dep.c functions

extern void test_faile();
extern state* init_game ();
extern void comp_to_coord (move* move, char* str);
extern void display_board (FILE *stream, int color, state* s);
extern void order_moves (move* moves, long int move_ordering[], int num_moves);
extern bool remove_one (int *marker, long int* move_ordering, int num_moves);
//evaluation functions to score a board
extern long int eval (state* s);
//faile moves.c modified to be thread safe
extern void gen(move* moves, int *num_moves, state* s);
extern void make (move* moves, int i, state* s);
extern void push_king (move* moves, int *num_moves, int from, int target, int castle_type, state* s);
extern void push_knight (move* moves, int *num_moves, int from, int target, state* s);
extern void push_pawn (move* moves, int *num_moves, int from, int target, bool is_ep, state* s);
extern void push_slide (move* moves, int *num_moves, int from, int target, state* s);
void unmake(move* moves, int i, state* s);
//alpha beta search interface
extern void* get_init_stateI(void);
extern void get_moves_for_game_stateI(void** moves, long int* move_orderings, void* state, uint32_t* num_moves);
extern void* get_state_for_move_and_game_stateI(void* state, void* moves, long int* move_orderings, int* ch, uint32_t num_moves);
extern long int evaluate_game_stateI(void*);
extern void free_stateI(void* state);
extern void free_movesI(void* moves);
extern int size_moveI();

#endif
