#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHM_KEY 1234
#define MBOX_A_TO_B_KEY 5001
#define MBOX_B_TO_A_KEY 5002
#define END_MARKER 0xFFFF
#define PAGE_SIZE 4096

// Structure to represent the maze in shared memory
typedef struct {
    int process_a_start;
    int process_b_start;
    int data[1022];
} maze_t; // size = 4096 B

// convert int to string
void itoa(int n, char* s, int base) {
    int i = 0;
    int sign = n;
    if (sign < 0) n = -n;
    do {
        s[i++] = n % base + '0';
    } while ((n /= base) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';

    // reversing the string
    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = s[j];
        s[j] = s[len - 1 - j];
        s[len - 1 - j] = temp;
    } 
}


void initialize_maze(maze_t* maze) {
    for (int i = 0; i < 1022; i++) {
        maze->data[i] = END_MARKER;
    }

    maze->process_a_start = 0;
    maze->process_b_start = 5;

    // maze->data is initialised as: 
    // maze->data[current position of 1 player] = next position of other player
    maze->data[0] = 1;
    maze->data[2] = 3;
    maze->data[4] = 7;
    maze->data[6] = END_MARKER;

    maze->data[5] = 2;
    maze->data[1] = 4;
    maze->data[3] = 6;
    maze->data[7] = END_MARKER;

    printf("Master: Initialized intertwined maze paths:\n");
    printf("  Process A will traverse: 0 -> 2 -> 4 -> 6 -> END (guided by B)\n");
    printf("  Process B will traverse: 5 -> 1 -> 3 -> 7 -> END (guided by A)\n");

    printf("Master: Lookup table:\n");
    for (int i = 0; i <= 7; i++) {
        if (maze->data[i] == END_MARKER)
            printf("  data[%d] = END_MARKER\n", i);
        else
            printf("  data[%d] = %d\n", i, maze->data[i]);
    }
}

int main(int argc, char* argv[]) {
    printf("Master: Starting maze traversal game setup...\n");
    
    // Create shared memory for the maze structure
    int shm_id = shm_create(SHM_KEY);
    if (shm_id < 0) {
        printf("Master: Failed to create shared memory\n");
        exit(1);
    }
    printf("Master: Created shared memory (ID: %d)\n", shm_id);
    
    // Attach to shared memory and initialize maze
    maze_t* maze = (maze_t*)shm_get(SHM_KEY);
    if (maze == 0) {
        printf("Master: Failed to attach to shared memory\n");
        shm_close(SHM_KEY);
        exit(1);
    }
    
    initialize_maze(maze);
    
    // Create mailboxes for inter-process communication
    int mbox_a_to_b = mbox_create(MBOX_A_TO_B_KEY);
    int mbox_b_to_a = mbox_create(MBOX_B_TO_A_KEY);
    
    if (mbox_a_to_b < 0 || mbox_b_to_a < 0) {
        printf("Master: Failed to create mailboxes\n");
        shm_close(SHM_KEY);
        exit(1);
    }
    
    printf("Master: Created mailboxes - A->B (ID: %d), B->A (ID: %d)\n", 
           mbox_a_to_b, mbox_b_to_a);
    
    // Fork process A
    int pid_a = fork();
    if (pid_a == 0) {
        // Child process - this will be Process A
        char start_pos[10];
        char send_key[10];
        char recv_key[10];
        itoa(maze->process_a_start, start_pos, 10);
        itoa(MBOX_A_TO_B_KEY, send_key, 10);
        itoa(MBOX_B_TO_A_KEY, recv_key, 10);
        char* args[6];
        args[0] = "process";
        args[1] = "A";
        args[2] = start_pos;
        args[3] = send_key;
        args[4] = recv_key;
        args[5] = 0;
        exec("process", args); // execute process.c(for A) with arguments given in args
        printf("Master: Failed to exec process A\n");
        exit(1);
    }

    // Fork process B
    int pid_b = fork();
    if (pid_b == 0) {
        // Child process - this will be Process B
        char start_pos[10];
        char send_key[10];
        char recv_key[10];
        itoa(maze->process_b_start, start_pos, 10);
        itoa(MBOX_B_TO_A_KEY, send_key, 10);
        itoa(MBOX_A_TO_B_KEY, recv_key, 10);
        char* args[6];
        args[0] = "process";
        args[1] = "B";
        args[2] = start_pos;
        args[3] = send_key;
        args[4] = recv_key;
        args[5] = 0;
        exec("process", args); // execute process.c(for B) with arguments given in args
        printf("Master: Failed to exec process B\n");
        exit(1);
    }

    if (pid_a < 0 || pid_b < 0) {
        printf("Master: Failed to fork processes\n");
        shm_close(SHM_KEY);
        exit(1);
    }

    printf("Master: Launched Process A (PID: %d) and Process B (PID: %d)\n", pid_a, pid_b);
    printf("Master: Waiting for both processes to complete maze traversal...\n");

    // Wait for both processes to complete
    int status_a, status_b;
    wait(&status_a);
    wait(&status_b);

    printf("Master: Both processes completed!\n");
    printf("Master: Process A exit status: %d\n", status_a);
    printf("Master: Process B exit status: %d\n", status_b);

    // Cleanup resources
    printf("Master: Cleaning up resources...\n");
    shm_close(SHM_KEY);
    
    printf("Master: Cleanup completed. Game over!\n");
    return 0;
}