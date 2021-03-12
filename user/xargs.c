#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char *argv[]) {
    char buf[512];

    read(0, buf, sizeof(buf));
    int i = 0;
    int bound = 32;
    while (i < bound) {
        char path[512];
        int j = 0;
        // char *path = buf;
        while (buf[i] != '\n' && i < bound) {
            // printf("char is %c\n", buf[i++]);
            path[j++] = buf[i++];
        }
        char *tc[3];
        tc[0] = argv[1];
        tc[1] = argv[2];
        tc[2] = 0;
        if (buf[i] == '\n') {
            path[j] = '\0';
            // printf("path is %s\n", path);
            if (fork() == 0) {
                close(0);
                open(path, 0);
                exec(argv[1], tc);
            }
        }
        // printf("i is %d\n", i);
        i++;
    }

    exit(0);
}