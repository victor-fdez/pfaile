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
void test_faile()
{

}

void init_game (state* s) 
{

	/* set up a new game: */
	int i;
		
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

	memcpy (s->board, init_board, sizeof(init_board));

	for (i = 0; i <= 143; i++)
		(s->moved)[i] = 0;
	/*
	white_to_move = 1;
	ep_square = 0;
	wking_loc = 30;
	bking_loc = 114;
	white_castled = no_castle;
	black_castled = no_castle;
	fifty = 0;
	game_ply = 0;
	*/

	/*
	result = no_result;
	captures = FALSE;

	piece_count = 14;

	post = TRUE;

	moves_to_tc = 30;
	min_per_game = 10;
	time_cushion = 0;

	reset_piece_square ();
	*/

	/* we use piece number 0 to show a piece taken off the board, so don't
     use that piece number for other things: */
	pieces[0] = 0;
	num_pieces = 0;
	//pieces  - tells where each piece is positioned
	//squares - tells for each piece found how many pieces there are on 
	//before it.
	for (i = 26; i < 118; i++) 
	{
		if (board[i] != frame && board[i] != npiece) 
		{
			pieces[++num_pieces] = i;
			squares[i] = num_pieces;
		}
		else 
		{
			squares[i] = 0;
			
		}
	}

	/*
	// compute the initial hash:
	srand (173);
	cur_pos = compute_hash ();

	// set the first 3 rep entry:
	rep_history[0] = cur_pos;
	*/

}

void comp_to_coord (move* move, char* str) {

  /* convert a move_s internal format move to coordinate notation: */

  int prom, from, target, f_rank, t_rank, converter;
  char f_file, t_file;

  prom = move.promoted;
  from = move.from;
  target = move.target;

  /* check to see if we have valid coordinates before continuing: */
  if (rank (from) < 1 || rank (from) > 8 ||
      file (from) < 1 || file (from) > 8 ||
      rank (target) < 1 || rank (target) > 8 ||
      file (target) < 1 || file (target) > 8) {
    sprintf (str, "xxxx");
    return;
  }
  
  f_rank = rank (from);
  t_rank = rank (target);
  converter = (int) 'a';
  f_file = file (from)+converter-1;
  t_file = file (target)+converter-1;

  /* "normal" move: */
  if (!prom) {
    sprintf (str, "%c%d%c%d", f_file, f_rank, t_file, t_rank);
  }

  /* promotion move: */
  else {
    if (prom == wknight || prom == bknight) {
      sprintf (str, "%c%d%c%dn", f_file, f_rank, t_file, t_rank);
    }
    else if (prom == wrook || prom == brook) {
      sprintf (str, "%c%d%c%dr", f_file, f_rank, t_file, t_rank);
    }
    else if (prom == wbishop || prom == bbishop) {
      sprintf (str, "%c%d%c%db", f_file, f_rank, t_file, t_rank);
    }
    else {
      sprintf (str, "%c%d%c%dq", f_file, f_rank, t_file, t_rank);
    }
  }

}


void display_board (FILE *stream, int color) {

  /* prints a text-based representation of the board: */
  
  char *line_sep = "+----+----+----+----+----+----+----+----+";
  char *piece_rep[14] = {"!!", " P", "*P", " N", "*N", " K", "*K", " R",
			  "*R", " Q", "*Q", " B", "*B", "  "};
  int a,b,c;

  if (color % 2) {
    fprintf (stream, "  %s\n", line_sep);
    for (a = 1; a <= 8; a++) {
      fprintf (stream, "%d |", 9 - a);
      for (b = 0; b <= 11; b++) {
	c = 120 - a*12 + b;
	if (board[c] != 0)
	  fprintf (stream, " %s |", piece_rep[board[c]]);
      }
      fprintf (stream, "\n  %s\n", line_sep);
    }
    fprintf (stream, "\n     a    b    c    d    e    f    g    h\n\n");
  }

  else {
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

