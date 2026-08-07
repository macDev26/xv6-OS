#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHM_KEY 1234
#define END_MARKER 0xFFFF
#define DATA_SIZE 1022

void delay(int cnt) {
  for (volatile int i = 0; i < cnt; i++) asm volatile("");
}

int
main(int argc, char *argv[])
{
  if (argc < 5) {
    printf("process: bad args. usage: process <role> <start_pos> <send_key> <recv_key>\n");
    exit(1);
  }

  char role_char = argv[1][0];
  int my_pos = atoi(argv[2]);
  int send_key = atoi(argv[3]);
  int recv_key = atoi(argv[4]);

  int send_mbox = mbox_create(send_key);
  int pid_cur = getpid();
  if (send_mbox < 0) {
    printf("[proc %d] mbox_create(send_key=%d) failed\n", pid_cur, send_key);
    exit(1);
  }
  int recv_mbox = mbox_create(recv_key);
  if (recv_mbox < 0) {
    printf("[proc %d] mbox_create(recv_key=%d) failed\n", pid_cur, recv_key);
    exit(1);
  }

  volatile int *base = (volatile int*)shm_get(SHM_KEY);
  if (base == 0) {
    printf("[proc %d] shm_get failed\n", pid_cur);
    exit(1);
  }
  volatile int *shared_data = base + 2;

  int role_send_first = (role_char == 'A');
  int my_done = 0;
  int other_done = 0;

  printf("[proc %d role %c] my_start=%d send_mbox=%d recv_mbox=%d\n",
         pid_cur, role_char, my_pos, send_mbox, recv_mbox);

  while (1) {
    if (role_send_first) {
      // --- SEND: Look up MY CURRENT position to get OTHER's next position ---
      int next_for_other;
      if (my_done || my_pos < 0 || my_pos >= DATA_SIZE) {
        next_for_other = END_MARKER;
      } else {
        next_for_other = shared_data[my_pos];
      }
      
      printf("[proc %d role %c] SEND -> looking up my current pos %d -> telling peer to go to %s (%d)\n", 
             pid_cur, role_char, my_pos,
             next_for_other == END_MARKER ? "END_MARKER" : "position",
             next_for_other);
      
      if (mbox_send(send_mbox, next_for_other) < 0) {
        printf("[proc %d] mbox_send failed\n", pid_cur);
        exit(1);
      }

      if (next_for_other == END_MARKER) {
        other_done = 1;
      }

      // --- RECV: Get my next position from other process ---
      int my_next_pos;
      if (mbox_recv(recv_mbox, &my_next_pos) < 0) {
        printf("[proc %d] mbox_recv failed\n", pid_cur);
        exit(1);
      }
      
      printf("[proc %d role %c] RECV <- peer tells me to go to %s (%d)\n",
             pid_cur, role_char,
             my_next_pos == END_MARKER ? "END_MARKER" : "position",
             my_next_pos);

      // NOW update my position after sending instruction based on old position
      if (my_next_pos == END_MARKER) {
        my_done = 1;
        printf("[proc %d role %c] I have reached the end!\n", pid_cur, role_char);
      } else {
        printf("[proc %d role %c] moving from %d to %d\n", pid_cur, role_char, my_pos, my_next_pos);
        my_pos = my_next_pos;
      }
    } else {
      // --- RECV first: Get my next position from other process ---
      int my_next_pos;
      if (mbox_recv(recv_mbox, &my_next_pos) < 0) {
        printf("[proc %d] mbox_recv failed\n", pid_cur);
        exit(1);
      }
      
      printf("[proc %d role %c] RECV <- peer tells me to go to %s (%d)\n",
             pid_cur, role_char,
             my_next_pos == END_MARKER ? "END_MARKER" : "position",
             my_next_pos);

      // --- SEND: Look up MY CURRENT position (before updating) to get OTHER's next position ---
      int next_for_other;
      if (my_done || my_pos < 0 || my_pos >= DATA_SIZE) {
        next_for_other = END_MARKER;
      } else {
        next_for_other = shared_data[my_pos];
      }
      
      printf("[proc %d role %c] SEND -> looking up my current pos %d -> telling peer to go to %s (%d)\n", 
             pid_cur, role_char, my_pos,
             next_for_other == END_MARKER ? "END_MARKER" : "position",
             next_for_other);
      
      if (mbox_send(send_mbox, next_for_other) < 0) {
        printf("[proc %d] mbox_send failed\n", pid_cur);
        exit(1);
      }

      if (next_for_other == END_MARKER) {
        other_done = 1;
      }

      // NOW update my position after sending instruction based on old position
      if (my_next_pos == END_MARKER) {
        my_done = 1;
        printf("[proc %d role %c] I have reached the end!\n", pid_cur, role_char);
      } else {
        printf("[proc %d role %c] moving from %d to %d\n", pid_cur, role_char, my_pos, my_next_pos);
        my_pos = my_next_pos;
      }
    }

    if (my_done && other_done) {
      printf("[proc %d role %c] both processes done -> exiting\n", pid_cur, role_char);
      break;
    }

    delay(200000); //letting other code run
  }

  shm_close(SHM_KEY);
  printf("[proc %d role %c] cleanup completed\n", pid_cur, role_char);
  exit(0);
}