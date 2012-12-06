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
#include "pprotos.h"
#include "pfaile_dep.h"

int main(int argc, char** argv)
{
					
	get_init_state = get_init_stateI;
	get_moves_for_game_state = get_moves_for_game_stateI;
	get_state_for_move_and_game_state = get_state_for_move_and_game_stateI;
	evaluate_game_state = evaluate_game_stateI;
	free_state = free_stateI;
	free_moves = free_movesI;
	size_move = size_moveI;
	//test_faile();
	state* s = init_game();
	move* m = (move*)think();
	display_board(stdout, 0, s);	
	make(m, 0, s);
	display_board(stdout, 0, s);	
	free(s);
	free(m);
	//ppq_test();
	return 0;
}
