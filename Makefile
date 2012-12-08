
CC = gcc
OPT = #-O3
PFLAGS = -Wall -Werror
FLAGS = -g -Wall #-DNDEBUG
PROF = -pg

# Release Build:

objects = faile.o utils.o moves.o search.o eval.o hash.o rand.o book.o
parallel_objects = pfaile.o psearch.o putils.o ppqueue.o chess_plug.o pfaile_dep.o pmoves.o peval.o ptest.o

headers = extvars.h faile.h protos.h ptree.c
parallel_headers = pprotos.h pfaile_dep.h

# Parallel Faile Build

pfaile: $(parallel_objects)
	$(CC) $(OPT) $(PFLAGS) $(FALGS) -o pfaile $(parallel_objects) -lm -lpthread

pprotos.h: pfaile_dep.h

%.o: %.c $(parallel_headers)
	$(CC) $(OPT) $(PFLAGS) $(FLAGS) -c -o $@ $<

# Profiling Parallel Faile Build:

pr_parallel_objects = pfaile_pr.o psearch_pr.o putils_pr.o ppqueue_pr.o chess_plug_pr.o pfaile_dep_pr.o pmoves_pr.o peval_pr.o ptest_pr.o


pfaile_pr: $(pr_parallel_objects)
	$(CC) $(PROF) $(OPT) $(FLAGS) -o pr_pfaile $(pr_parallel_objects) -lm -lpthread

%_pr.o: %.c
	$(CC) $(PROF) $(OPT) $(PFLAGS) $(FLAGS) -c -o $@ $<



all:	pfaile

clean:
	rm -f *.o
	rm -f *.out
	rm -f *~

clean_win:
	del *.o
	del *~

touch:
	touch *.c
	touch *.h
new:
	rm -f *.o
	rm -f *.out
	touch *.c
	touch *.h

profile:	faile_pr

emacs:
	rm -f *~

#EOF
