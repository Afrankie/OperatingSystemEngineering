#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char *argv[]) {
	if (argc == 1) {
		printf("error");
		exit(1);
	}
	int interval = atoi(argv[1]);
	sleep(interval);
	exit(0);
}