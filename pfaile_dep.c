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
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "pfaile_dep.h"
//REMEBER to set start pieces count before running a search

void test_faile()
{
	move m[200];
	int i, num_moves = 0;
	char* str = (char*)malloc(sizeof(char)*8);
	state* s = init_game();
	if(s == NULL)
	{
		exit(-1);
		printf("error\n");
	}
	gen(&m[0], &num_moves, s);
	printf("FIRST BOARD\n");
	display_board (stdout, 1, s);
	start_piece_count = s->num_pieces;
	for(i = 0; i < num_moves; i++)
	{
		make(&m[0], i, s);
		printf("score  = %ld\n", eval(s));
		display_board (stdout, 1, s);
		unmake(&m[0], i, s);
	}
	//comp_to_coord(m, str);
	printf("%s\n", str);
}

state* init_game () 
{

	// set up a new game: 
	int i;
	state* s = (state*)malloc(sizeof(state));
	move* m = (move*)malloc(sizeof(move));
	uint8_t* board = (uint8_t*)(s->board);
	uint8_t* squares = (uint8_t*)(s->squares);
	uint8_t* moved = (uint8_t*)(s->moved);
	uint8_t* pieces = (uint8_t*)(s->pieces);
	uint8_t* num_pieces = &(s->num_pieces);
	uint8_t init_board[144] = {
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,7,3,11,9,5,11,3,7,0,0,
	0,0,1,1,1,1,1,1,1,1,0,0,
	0,0,13,13,13,13,13,13,13,13,0,0,
	0,0,13,13,13,13,13,13,13,13,0,0,
	0,0,13,13,13,13,13,13,13,13,0,0,
	0,0,13,13,13,13,13,13,13,13,0,0,
	0,0,2,2,2,2,2,2,2,2,0,0,
	0,0,8,4,12,10,6,12,4,8,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0
	};
	s->m = m;
	memcpy (s->board, init_board, sizeof(init_board));

	for (i = 0; i <= 143; i++)
		moved[i] = 0;

	s->white_to_move = 1;
	s->ep_square = 0;
	s->wking_loc = 30;
	s->bking_loc = 114;
	s->white_castled = no_castle;
	s->black_castled = no_castle;
	//fifty = 0;
	//game_ply = 0;

	//result = no_result;
	s->captures = false;

	s->piece_count = 14;

	//post = TRUE;

	//moves_to_tc = 30;
	//min_per_game = 10;
	//time_cushion = 0;

	//reset_piece_square ();

	/* we use piece number 0 to show a piece taken off the board, so don't
     use that piece number for other things: */
	pieces[0] = 0;
	*num_pieces = 0;
	//pieces  - tells where each piece is positioned
	//squares - tells for each piece found how many pieces there are on 
	//before it.
	for (i = 26; i < 118; i++) 
	{
		if (board[i] != frame && board[i] != npiece) 
		{
			pieces[++(*num_pieces)] = i;
			squares[i] = *num_pieces;
		}
		else 
		{
			squares[i] = 0;
			
		}
	}
	//STILL will not be used
	/*
	// compute the initial hash:
	srand (173);
	cur_pos = compute_hash ();

	// set the first 3 rep entry:
	rep_history[0] = cur_pos;
	*/
	return s;
}

void comp_to_coord (move* move, char* str) {

	// convert a move_s internal format move to coordinate notation: 

	uint8_t prom, from, target, f_rank, t_rank, converter;
	uint8_t f_file, t_file;

	prom = move->promoted;
	from = move->from;
	target = move->target;

	// check to see if we have valid coordinates before continuing: 
	if (rank (from) < 1 || rank (from) > 8 ||
		file (from) < 1 || file (from) > 8 ||
		rank (target) < 1 || rank (target) > 8 ||
		file (target) < 1 || file (target) > 8) {
		sprintf (str, "xxxx");
		return;
	}

	f_rank = rank (from);
	t_rank = rank (target);
	converter = (uint8_t) 'a';
	f_file = file (from)+converter-1;
	t_file = file (target)+converter-1;

	// "normal" move: 
	if (!prom) 
	{
		sprintf (str, "%c%d%c%d", f_file, f_rank, t_file, t_rank);
	}
	// promotion move: 
	else 
	{
		if (prom == wknight || prom == bknight) 
		{
			sprintf (str, "%c%d%c%dn", f_file, f_rank, t_file, t_rank);
		}
		else if (prom == wrook || prom == brook) 
		{
			sprintf (str, "%c%d%c%dr", f_file, f_rank, t_file, t_rank);
		}
		else if (prom == wbishop || prom == bbishop) 
		{
			sprintf (str, "%c%d%c%db", f_file, f_rank, t_file, t_rank);
		}
		else 
		{
			sprintf (str, "%c%d%c%dq", f_file, f_rank, t_file, t_rank);
		}
	}

}

//char divider[50] = "-------------------------------------------------";

void display_board (FILE *stream, int color, state* s) 
{

	  
	char *line_sep = "+----+----+----+----+----+----+----+----+";
	char *piece_rep[14] = {"!!", " P", "*P", " N", "*N", " K", "*K", " R",
			  "*R", " Q", "*Q", " B", "*B", "  "};
		
	int a,b,c;
	uint8_t* board = s->board;	
	if (color % 2) //when color 1 or white 
		{
		fprintf (stream, "  %s\n", line_sep);
		for (a = 1; a <= 8; a++) 
		{
			fprintf (stream, "%d |", 9 - a);
			for (b = 0; b <= 11; b++) 
			{
				c = 120 - a*12 + b;
				if (board[c] != 0)
				 	fprintf (stream, " %s |", piece_rep[board[c]]);
			}
			fprintf (stream, "\n  %s\n", line_sep);
		}
		fprintf (stream, "\n     a    b    c    d    e    f    g    h\n\n");
	}
	else //when color 0 is black
	{
		fprintf (stream, "  %s\n", line_sep);
		for (a = 1; a <= 8; a++) {
			fprintf (stream, "%d |", a);
			for (b = 0; b <= 11; b++) {
				c = 24 + a*12 -b;
				if (board[c] != 0)
					fprintf (stream, " %s |", piece_rep[board[c]]);
			}
			fprintf (stream, "\n  %s\n", line_sep);
		}
		fprintf (stream, "\n     h    g    f    e    d    c    b    a\n\n");
	}

}
void order_moves (move* moves, long int* move_ordering, int num_moves) {
	int i;
	for(i = 0; i < num_moves; i++)
	{
		move_ordering[i] = (long int)rand();
	}	
}

/* DO RANDOM ORDERING INSTEAD
void order_moves (move* moves, long int move_ordering[], int num_moves, move *h_move, state* s) {

	uint8_t* board = s->board;
	uint8_t* pieces = s->pieces;
	uint8_t* moved = s->moved;
	uint8_t white_to_move = (s->white_to_move);	
	uint8_t wking_loc= (s->wking_loc);
	uint8_t bking_loc= (s->bking_loc);
	uint8_t white_castled= (s->white_castled);
	uint8_t black_castled= (s->black_castled);
	uint8_t piece_count= (s->piece_count);
	uint8_t num_pieces= (s->num_pieces);
  // sort out move ordering scores in move_ordering, using implemented
//     heuristics: 

  int cap_values[14] = {
    0,100,100,310,310,0,0,500,500,955,955,325,325,0};
  int i, from, target, promoted, captured;

  // fill the move ordering array: 

  // if searching the pv, give it the highest move ordering, and if not, rely
//     on the other heuristics: 
  if (searching_pv) {
    searching_pv = FALSE;
    for (i = 0; i < num_moves; i++) {
      from = moves[i].from;
      target = moves[i].target;
      promoted = moves[i].promoted;
      captured = moves[i].captured;
      
      // give captures precedence in move ordering, and order captures by
//	 material gain 
      if (captured != npiece)
	move_ordering[i] = cap_values[captured]-cap_values[board[from]]+1000;
      else
	move_ordering[i] = 0;

      // make sure the suggested hash move gets ordered high: 
      if (from == h_move->from && target == h_move->target
	  && promoted == h_move->promoted) {
	move_ordering[i] += INF-10000;
      }

      // make the pv have highest move ordering: 
      if (from == pv[1][ply].from && target == pv[1][ply].target
	  && promoted == pv[1][ply].promoted) {
	searching_pv = TRUE;
	move_ordering[i] += INF;
      }

      // heuristics other than pv (no need to use them on the pv move - it is
//	 already ordered highest) 
      else {
	// add the history heuristic bonus: 
	move_ordering[i] += (history_h[from][target]>>i_depth);

	// add the killer move heuristic bonuses: 
	if (from == killer1[ply].from && target == killer1[ply].target
	    && promoted == killer1[ply].promoted)
	  move_ordering[i] += 1000;
	else if (from == killer2[ply].from && target == killer2[ply].target
	    && promoted == killer2[ply].promoted)
	  move_ordering[i] += 500;
	else if (from == killer3[ply].from && target == killer3[ply].target
	    && promoted == killer3[ply].promoted)
	  move_ordering[i] += 250;
      }
    }
  }

  // if not searching the pv: 
  else {
    for (i = 0; i < num_moves; i++) {
      from = moves[i].from;
      target = moves[i].target;
      promoted = moves[i].promoted;
      captured = moves[i].captured;
      
      // give captures precedence in move ordering, and order captures by
//	 material gain 
      if (captured != npiece)
	move_ordering[i] = cap_values[captured]-cap_values[board[from]]+1000;
      else
	move_ordering[i] = 0;

      // make sure the suggested hash move gets ordered first: 
      if (from == h_move->from && target == h_move->target
	  && promoted == h_move->promoted) {
	move_ordering[i] += INF+1000;
      }

      // heuristics other than pv 
      
      // add the history heuristic bonus: 
      move_ordering[i] += (history_h[from][target]>>i_depth);

      // add the killer move heuristic bonuses: 
      if (from == killer1[ply].from && target == killer1[ply].target
	  && promoted == killer1[ply].promoted)
	move_ordering[i] += 1000;
      else if (from == killer2[ply].from && target == killer2[ply].target
	  && promoted == killer2[ply].promoted)
	move_ordering[i] += 500;
      else if (from == killer3[ply].from && target == killer3[ply].target
	  && promoted == killer3[ply].promoted)
	move_ordering[i] += 250;
    }
  }

}
*/
bool remove_one (int *marker, long int* move_ordering, int num_moves) {

//     a function to give pick the top move order, one at a time on each call.
//     Will return TRUE while there are still moves left, FALSE after all moves
//     have been used

  int i, best = -1;

  *marker = -1;

  for (i = 0; i < num_moves; i++) {
    if (move_ordering[i] > best) {
      *marker = i;
      best = move_ordering[i];
    }
  }

  if (*marker > -1) {
    move_ordering[*marker] = -1;
    return true;
  }
  else {
    return false;
  }

}

