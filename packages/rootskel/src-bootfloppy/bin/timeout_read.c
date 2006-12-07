#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

int main(int argc, char *argv[])
{  
	int i;
	char str[255] = "";
	fd_set set;
	struct timeval ts;
	int ret = -1;

	if (argc != 2) {
	  fprintf(stderr, "Usage: %s nb_seconds\n",
		  argv[0]);
	  exit(1);
	}

	ts.tv_sec = strtol(argv[1], NULL, 10);
	ts.tv_usec = 0;

	FD_ZERO (&set);
	FD_SET (0, &set);
	
	i = select (FD_SETSIZE, &set, NULL, NULL, &ts);

	if (i == -1) {
	  fprintf(stderr, "select error\n");
	}
	else if (i) {
		i = read(0, str,255);
		if ( i > 0 ) {
		  str[i] = '\0';
		  ret = 0;
		}
	}
	return ret;
}
