#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

inline void error_shutdown(char* msg, int error_val)
{
	errno = error_val;
	perror(msg);
	exit(EXIT_FAILURE);
}
