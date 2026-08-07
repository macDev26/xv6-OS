#ifndef __USER__

struct stat;
struct rtcdate;
typedef struct __lock_t{
  uint flag;
 }lock_t;
typedef struct __mutex{
  uint flag;
 }thread_mutex;
 
// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int draw(void*, uint);
int clone(void(*start_routine)(void*), void *, void *);
int join(void**);
int texit(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int thread_create(void (*start_routine)(void *), void * arg1,void *s1);
int thread_join(); 
void thread_exit();
int spinlock_init(lock_t *lk);
void spinlock_acquire(lock_t *lk);
void spinlock_release(lock_t *lk);
void mutex_init(thread_mutex *m);
void mutex_acquire(thread_mutex *m);
void mutex_release(thread_mutex *m);

#endif
