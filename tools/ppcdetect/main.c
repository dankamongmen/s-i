
#include "main.h"

char *machine = NULL;
char *generation = NULL;

static void detect_newworld() {
	if(check_value(generation, "NewWorld", "NewWorld PowerMac"))
		exit(0);
}

static void detect_oldworld() {
	if(check_value(generation, "OldWorld", "OldWorld PowerMac"))
		exit(0);
}

static void detect_prep() {
	if(check_value(machine, "PReP", NULL))
		exit(0);
}

static void detect_chrp() {
	if(check_value(machine, "CHRP Pegasos", NULL))
		exit(0);
	if(check_value(machine, "CHRP", NULL))
		exit(0);
}

int check_value(const char *key, const char* value, const char *msg) {
	/* make NULL-pointer sanity */
	if((key == NULL) || (value == NULL))
		return(FALSE);

	if(strstr(key, value) > 0) {
		if(msg != NULL)
			printf("%s\n", msg);
		else
			printf("%s\n", value);
		return(TRUE);
	}

	return(FALSE);
}

int main(int argc, char *argv[]) {
	FILE *cpuinfo = NULL;
	char line[1024];
	int probe = 0;
	void (*probes[])() = {
		detect_prep,
		detect_chrp,
		detect_newworld,
		detect_oldworld,
		NULL
	};

	/* read some usefull informations */
	cpuinfo = fopen("/proc/cpuinfo", "r");
	if(cpuinfo == NULL) {
		fprintf(stderr, "Unable to open /proc/cpuinfo file!\n");
		return(1);
	}

	while(fgets(line, 1024, cpuinfo) != NULL) {
		char *pos = NULL;
		int buflen = 0;

		if(line == NULL)
			continue;
		pos = strchr(line, ':');
		if(pos == NULL)
			continue;
		pos++;
		while(pos != NULL && pos[0] == 32)
			pos++;
		buflen = strlen(pos)-1;

		if(strstr(line, "machine") == line) {
			machine = (char*)malloc(sizeof(char)*buflen);
			strncpy(machine, pos, buflen);
		}

		if(strstr(line, "pmac-generation") == line) {
			generation = (char*)malloc(sizeof(char)*buflen);
			strncpy(generation, pos, buflen);
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

