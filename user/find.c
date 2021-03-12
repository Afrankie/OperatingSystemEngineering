#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int flag = 1;

void strcat(char *dst, char *src) {
  char *tp = dst;
  while (*tp++);
  --tp;
  while ((*tp++ = *src++) != 0);
}

void rec(char *path, char *pattern) {
    char buf[512];
    int fd;
    struct dirent de;
    struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }


  switch(st.type){
    case T_FILE:
        // read(fd, &de, sizeof(de));
        // if (!strcmp(de.name, pattern)) {
        //   strcpy(buf, path);
        //   strcat(buf, "/");
        //   strcat(buf, de.name);
        //   printf("%s\n", buf);
        // }
        break;

    case T_DIR:
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            strcpy(buf, path);
            strcat(buf, "/");
            strcat(buf, de.name);
            if (!strcmp(de.name, pattern)) {
              printf("%s\n", buf);
            }
            if (strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0) {
              int tfd = open(path, 0);
              struct stat tst;
              fstat(tfd, &tst);
              if (tst.type == T_DIR) {
                // printf("enter de.name is %s\n", de.name);
                rec(buf, pattern);
              }
              close(tfd);
            }
        }
        break;
  }
  close(fd);

}

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "find argc != 3");
        exit(-1);
    }
    // printf("begin dir is %s, pattern is %s\n", argv[1], argv[2]);
    rec(argv[1], argv[2]);
    exit(0);
}

