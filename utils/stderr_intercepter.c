/* 
 * Runs the program given as an argument, saptures its stderr, and
 * reformats it as debconf protocol messages, for clean display.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void display_error(char *buf) {
	char *p;
	while ((p = strchr(buf, '\n'))) {
		p[0] = ' ';
	}
	printf("SUBST debian-installer/generic_error error %s\n", buf);
	printf("INPUT high debian-installer/generic_error\n");
	/* The reason a GO is not set to debconf is because if another
	 * question is pending, it would be displayed too. The user might
	 * ask to back up. Then, when the program that asked the other
	 * question tells debconf to GO, there will be nothing displayed.
	 * The information that the user backed up will be lost.
	 * Since there's always another GO coming along eventually, this
	 * program will not bother to force immediate display of the
	 * question. */
}

int main (int argc, char **argv) {
	pid_t pid;
	int filedes[2];
	
	if (argc < 1) {
		fprintf(stderr, "need an argument\n");
		exit(1);
	}
	
	/* set up pipe used to hook child stderr to parent */
	if (pipe(filedes) != 0) {
		perror("pipe");
		exit(1);
	}
	
	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (! pid) {
		/* child */
		close(filedes[0]);
		
		if (dup2(filedes[1], 2) == -1) {
			perror("dup2");
			exit(1);
		}
		execlp(argv[1], argv[1], NULL);
		perror("execlp");
		exit(1);
	}
	else {
		/* parent */
		char buf[1024];

		close(filedes[1]);
		
		while (read(filedes[0], &buf, 1023) > 0) {
			display_error(buf);
		}
		
		exit(0);
	}
}
