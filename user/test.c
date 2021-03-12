#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    printf("p value is %s\n", p);

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      printf("memmove de.name = %s p = %s\n", de.name, p);
      p[DIRSIZ] = 0;
    //   if(stat(buf, &st) < 0){
    //     printf("ls: cannot stat %s\n", buf);
    //     continue;
    //   }

      printf("buf = %s path = %s\n", buf, path);
    }
    break;
  }
  close(fd);
}

int
main(void) {
    ls(".");
    exit(0);
}