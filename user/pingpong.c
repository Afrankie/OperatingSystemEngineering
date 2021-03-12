#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
	int p[2];

	pipe(p);

	if (fork() == 0) {
		printf("%d: received ping\n", getpid());
		exit(0);
	} else {
		// write(p[1], 't', 1);
		wait(0);
		printf("%d: received pong\n", getpid());
	}

	exit(0);
}