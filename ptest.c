
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "pprotos.h"
#include "pfaile_dep.h"
#define STAB	5
#define FSTAB	((1.0)/ 5.0)

struct rusage start;
struct rusage stop;
struct rusage change;
struct timeval total;
double total_sec = 0.0;
double avg_seconds, avg_num_exp, avg_num_disc, avg_num_evaluated, avg_num_seen, avg_branch_factor, avg_num_theory, avg_percent_seen;



inline void start_measure()
{
	getrusage(RUSAGE_SELF, &start);
	
}
	

inline void stop_measure()
{
	getrusage(RUSAGE_SELF, &stop);
	
	timersub(&stop.ru_utime, &start.ru_utime, &change.ru_utime);
	timersub(&stop.ru_stime, &start.ru_stime, &change.ru_stime);
	timeradd(&change.ru_utime, &change.ru_stime, &total);

//	change.ru_maxrss = stop.ru_maxrss - start.ru_maxrss;
//	change.ru_ixrss = stop.ru_ixrss - start.ru_ixrss;
//	change.ru_idrss = stop.ru_idrss - start.ru_idrss;
//	change.ru_isrss = stop.ru_isrss - start.ru_isrss;
//	change.ru_minflt = stop.ru_minflt - start.ru_minflt;
//	change.ru_majflt = stop.ru_majflt - start.ru_majflt;
//	change.ru_nswap = stop.ru_nswap - start.ru_nswap;
//	change.ru_inblock = stop.ru_inblock - start.ru_inblock;
//	change.ru_oublock = stop.ru_oublock - start.ru_oublock;
//	change.ru_msgsnd = stop.ru_msgsnd - start.ru_msgsnd;
//	change.ru_msgrcv = stop.ru_msgrcv - start.ru_msgrcv;
//	change.ru_nsignals = stop.ru_nsignals - start.ru_nsignals;
//	change.ru_nvcsw = stop.ru_nvcsw - start.ru_nvcsw;
//	change.ru_nivcsw = stop.ru_nivcsw - start.ru_nivcsw;

}

inline void print_measure(){
	printf("seconds 	\t%d\n", (int)total.tv_sec);
	printf("useconds	\t%d\n", (int)total.tv_usec);
}

void run_tests()
{
	int i = 0, d = 0, k = 0;				
	//int cpus = 0;
 	int ncpus;
	state* s;
	get_init_state = get_init_stateI;
	get_moves_for_game_state = get_moves_for_game_stateI;
	get_state_for_move_and_game_state = get_state_for_move_and_game_stateI;
	evaluate_game_state = evaluate_game_stateI;
	free_state = free_stateI;
	free_moves = free_movesI;
	size_move = size_moveI;
	ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	if(ncpus == -1)
		ncpus = 2;
	
	s = init_game();
	printf("#ncpus %d\n", ncpus);
	printf("avg_seconds avg_num_exp avg_num_disc avg_num_evaluated avg_num_seen avg_branch_factor avg_num_theory avg_percent_seen\n");
	for(d = 3; d < 7; d++)
	{
		max_depth = d;			
		printf("#depth %d\n", max_depth);
		for(i = 0; i < ncpus; i++)
		{
			num_threads = i+1;
			printf("#threads %d\n", num_threads);	
			double num_theory = 0.0;
			avg_seconds = 0.0;
			avg_num_exp = 0.0;
			avg_num_disc = 0.0;
			avg_seconds = 0.0;
			avg_num_exp = 0.0;
			avg_num_disc = 0.0;
			avg_num_evaluated = 0.0;
			avg_num_seen = 0.0;
			avg_branch_factor = 0.0;
			num_theory = 0.0;
			avg_num_theory = 0.0;
			avg_percent_seen = 0.0;

			for(k = 0; k < STAB; k++)
			{
				//display_board(stdout, 0, s);	
				//i
				
				start_measure();
				move* m = (move*)think((void*)s);
				stop_measure();
				//print_measure();
				free(m);
				avg_seconds += FSTAB*(((double)total.tv_sec) + (((double)total.tv_usec)/1000000.0));
				avg_num_exp += FSTAB*((double)num_nodes_expanded);
				avg_num_disc += FSTAB*((double)num_nodes_discarded);
				avg_num_evaluated += FSTAB*((double)num_nodes_evaluated);
				avg_num_seen += FSTAB*((double)num_nodes_seen);
				avg_branch_factor += FSTAB*((double)branching_factor);
				num_theory = pow(branching_factor, (double)d);
				avg_num_theory += FSTAB*((double)num_theory); 
				avg_percent_seen += FSTAB*(((double)num_nodes_seen)/(num_theory));
			}
		
			printf("%f %f %f %f %f %f %f %f\n", avg_seconds, avg_num_exp, avg_num_disc, avg_num_evaluated, avg_num_seen, avg_branch_factor, avg_num_theory, avg_percent_seen);
		}
	}
	free(s);
}
