
#include "main.h"

char *machine = NULL;
char *generation = NULL;

static void detect_newworld() {
	if(strstr(generation, "NewWorld") > 0) {
		printf("NewWorld\n");
		exit(0);
	}
}

static void detect_prep() {
	if(strstr(machine, "PReP") > 0) {
		printf("PReP\n");
		exit(0);
	}
}

static void detect_chrp() {
	if(strstr(machine, "CHRP Pegasos") > 0) {
		printf("CHRP Pegasos\n");
		exit(0);
	}
	if(strstr(machine, "CHRP") > 0) {		/* fallback to CHRP */
		printf("CHRP\n");
		exit(0);
	}
}

int main(int argc, char *argv[]) {
	FILE *cpuinfo = NULL;
	char line[1024];
	int probe = 0;
	void (*probes[])() = {
		detect_prep,
		detect_chrp,
		detect_newworld,
		NULL
	};

	/* read some usefull informations */
	cpuinfo = fopen("/proc/cpuinfo", "r");
	if(cpuinfo == NULL) {
		fprintf(stderr, "Unable to open /proc/cpuinfo file!\n");
		return(1);
	}

	while(fgets(line, 1024, cpuinfo) != NULL) {
		char key[1024], value[1024];

		sscanf(line, "%s : %s", key, value);
		if((key == NULL) || (strlen(key) < 0)) {
			printf("break me\n");
			break;
		}

		if(strstr(key, "machine") > 0) {
			machine = (char*)malloc(sizeof(char)*1024);
			strncpy(machine, value, 1024);
		}

		if(strstr(key, "pmac-generation") > 0) {
			generation = (char*)malloc(sizeof(char)*1024);
			strncpy(generation, value, 1024);
		}
	}

	fclose(cpuinfo);

	/* walk through all available checks */
	while(probe >= 0) {
		if(probes[probe] == NULL)
			break;

		probes[probe]();
		probe++;
	}

	/* if all other fails, exit with UNKNOWN */
	printf("Unknown\n");
	return(1);
}

