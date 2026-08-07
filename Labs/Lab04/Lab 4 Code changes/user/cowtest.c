#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main() {
  char *p = sbrk(4096);
  if(p == (char*)-1) {
    printf("sbrk failed\n");
    exit(1);
  }
  p[0] = 'A';
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }
  if(pid == 0){
    // child
    p[0] = 'C'; // should cause COW
    if(p[0] != 'C'){
      printf("child write failed (%c)\n", p[0]);
      exit(1);
    }
    exit(0);
  } else {
    wait(0);
    if(p[0] != 'A'){
      printf("parent sees wrong value %c (COW broken)\n", p[0]);
      exit(1);
    }
    printf("Copy-on-Write Fork OK\n");
  }
  exit(0);
}
