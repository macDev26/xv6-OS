#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


#define MBOX_KEY 5678
#define NUM_MESSAGES 6   // total messages (half parent->child, half child->parent)

// Simple busy-wait delay loop
void
delay(int count)
{
  for (volatile int i = 0; i < count; i++) {
    asm volatile("");   // prevent compiler optimization
  }
}

int
main()
{
    int pid;
    int mbox_id;
    int i, msg;

    printf("Mailbox Test (Bidirectional)\n");

    // Create mailbox
    mbox_id = mbox_create(MBOX_KEY);
    if(mbox_id < 0) {
        printf("Failed to create mailbox\n");
        exit(0);
    }
    printf("Created mailbox with ID: %d\n", mbox_id);

    pid = fork();
    if(pid < 0) {
        printf("Fork failed\n");
        exit(0);
    }

    if(pid == 0) {
        // Child process
        delay(5000000); // give parent time to start

        for(i = 0; i < NUM_MESSAGES; i++) {
            if (i < NUM_MESSAGES/2) {
                // First half: child RECEIVES
                if(mbox_recv(mbox_id, &msg) < 0) {
                    printf("Child: Failed to receive message %d\n", i);
                    exit(0);
                }
                printf("Child: Received message %d: %d\n", i, msg);
            } else {
                // Second half: child SENDS
                msg = (i + 1) * 100; // arbitrary message value
                printf("Child: Sending message %d: %d\n", i, msg);
                if(mbox_send(mbox_id, msg) < 0) {
                    printf("Child: Failed to send message %d\n", i);
                    exit(0);
                }
            }
            delay(5000000);
        }
        printf("Child: Finished messages\n");
        exit(0);
    } else {
        // Parent process
        printf("Parent: Starting messages\n");

        for(i = 0; i < NUM_MESSAGES; i++) {
            if (i < NUM_MESSAGES/2) {
                // First half: parent SENDS
                msg = (i + 1) * 10;
                printf("Parent: Sending message %d: %d\n", i, msg);
                if(mbox_send(mbox_id, msg) < 0) {
                    printf("Parent: Failed to send message %d\n", i);
                    exit(0);
                }
            } else {
                // Second half: parent RECEIVES
                if(mbox_recv(mbox_id, &msg) < 0) {
                    printf("Parent: Failed to receive message %d\n", i);
                    exit(0);
                }
                printf("Parent: Received message %d: %d\n", i, msg);
            }
            delay(5000000);
        }

        printf("Parent: Finished messages\n");
        wait(0);
    }

    printf("Mailbox test completed\n");
    exit(0);
}
