#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_draw(void)
{
  void* buf;
  uint size;
  
  argint(1, (void*)&size);
  argptr(0, (void*)&buf, size);
  
  char text[] = "      Hi Abana \n\
  \n\
  Print Dog \n\
  \n\
       /^-^\\\n\
      / o o \\\n\
     /   Y   \\\n\
     V \\ v / V\n\
       / - \\   \n\
      /    |   \n\
(    /     |   \n\
====/___) ||    \n\n";
 
  
 /* char text[] = "Hi Abana\n\
        bb\n\
        bb\n\
  aa aa bb        aa aa nn nnn    aa aa\n\
 aa aaa bbbbbb   aa aaa nnn  nn  aa aaa\n\
aa  aaa bb   bb aa  aaa nn   nn aa  aaa\n\
 aaa aa bbbbbb   aaa aa nn   nn  aaa aa\n\n";       */
 
  if(sizeof(text)>size)
    return -1;
    
  strncpy((char *)buf, text, size);
  return sizeof(text);
}

int
sys_clone(void)
{
  int fcn, arg1, stack;
  if(argint(0, &fcn)<0 || argint(1, &arg1)<0 || argint(2, &stack)<0)
    return -1;
  return clone((void *)fcn, (void *)arg1,(void *)stack);
}

int
sys_join(void)
{
  void **stack;
  int stackArg;
  stackArg = argint(0, &stackArg);
  stack = (void**) stackArg;
  return join(stack);
}

void
sys_texit(void)
{
 return texit();
}
 
                                                                     
 
