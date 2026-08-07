#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
delay(int count) {
  for (volatile int i = 0; i < count; i++) asm volatile("");
  return 0;
}

int
main(void) {
    int key = 2025;
    if (shm_create(key) < 0) {
        printf("shm_create failed\n");
        exit(1);
    }

    volatile int *p = (volatile int*)shm_get(key);
    if (p == 0) {
        printf("shm_get failed\n");
        exit(1);
    }

    // initialize shared region
    p[0] = 0;   // shared value
    p[1] = 0;   // parent_ready flag
    p[2] = 0;   // child_read flag
    p[3] = 0;   // child_written flag

    if (fork() == 0) {
        // Child process
        volatile int *c = (volatile int*)shm_get(key);
        if (c == 0) {
            printf("Child: shm_get failed\n");
            exit(1);
        }

        printf("Child: starting\n");
        printf("Child: waiting for parent signal\n");
        // Wait for parent to write and signal ready
        while (c[1] == 0) {
            delay(1000);   // small backoff so we don't busy spin hot
        }

        printf("Child: parent signaled, reading value\n");
        printf("Child: read %d from shared page\n", c[0]);
        c[2] = 1;  // Signal child has read and printed

        // Child writes
        c[0] = 5678;
        printf("Child: wrote %d into shared page\n", c[0]);
        c[3] = 1;  // Signal child has written and printed

        printf("Child: exiting\n");
        shm_close(key);
        exit(0);
    } else {
        // Parent writes first
        p[0] = 1234;
        printf("Parent: wrote %d into shared page\n", p[0]);

        p[1] = 1;  // Signal parent is ready
        printf("Parent: signaled child, waiting for child to finish\n");

        // Wait for child to finish both reading and writing
        while (p[3] == 0) {
            delay(1000);
        }

        // Now parent can safely print final message
        printf("Parent: final value = %d in shared page\n", p[0]);

        wait(0);
        shm_close(key);
    }
    exit(0);
}
