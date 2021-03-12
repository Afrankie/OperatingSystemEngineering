#include "kernel/types.h"
#include "user/user.h"

int
main(void) {
    int p[36][2];
    int max = 36;
    
    memset(p, 0, sizeof(p));
    pipe(p[1]);
    for (int i = 2; i < max; i ++) {
        write(p[1][1], &i, 4);
    }
    close(0);
    dup(p[1][0]);
    close(p[1][1]);
    close(p[1][0]);

    // printf("p[1][1] is %d, p[1][0] is %d\n", p[1][1], p[1][0]);
    int p_size = 0;
    for (int i = 2; i < max; i ++) {
        while (p_size >= 10 || p[i - 1][0] == 0) {
            sleep(10);
        }
        p_size++;
        pipe(p[i]);
        if (fork() == 0) {
            // printf("loop i is %d\n", i);
            int num;
            // p[i - 1][0]
            while (read(0, &num, 4) != 0) {
                // printf("read out is %d, i is %d\n", num, i);
                if (num == i) {
                    // printf("prime %d, i is %d\n", num, i);
                    printf("prime %d\n", num);
                } else if (num % i != 0) {
                    // if (p[i][0] == 0 && p[i][1] == 0) pipe(p[i]);
                    if (i != max - 1) write(p[i][1], &num, 4);
                }
            }
            exit(0);
        } else {
            wait(0);
            p_size--;
            close(0);
            dup(p[i][0]);
            close(p[i][1]);
            close(p[i][0]);
        }
    }
    exit(0);
}
