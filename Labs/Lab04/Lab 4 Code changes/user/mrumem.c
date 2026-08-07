#include "kernel/types.h"
#include "user/user.h"

#define PAGES_TO_ALLOC 15  // Allocate enough pages to trigger swapping
#define TEST_ITERATIONS 50 // More iterations to see MRU behavior

int
main(int argc, char *argv[])
{
  char *memory[PAGES_TO_ALLOC];
  struct pagestat stats;
  int i, j;
  
  printf("MRU Memory Test Program\n");
  printf("=======================\n");
  
  // Allocate pages using sbrk
  printf("Allocating %d pages...\n", PAGES_TO_ALLOC);
  for(i = 0; i < PAGES_TO_ALLOC; i++) {
    memory[i] = sbrk(4096); // Allocate one page
    if(memory[i] == (char *)-1) {
      printf("Failed to allocate page %d\n", i);
      exit(1);
    }
    printf("Allocated page %d at address 0x%lx\n", i, (uint64)memory[i]);
  }
  
  printf("\nStarting random memory access test...\n");
  
  // Track the last iteration that wrote to each page
  int last_write[PAGES_TO_ALLOC];
  for(i = 0; i < PAGES_TO_ALLOC; i++) {
    last_write[i] = -1;
  }
  
  // Perform random memory accesses to trigger page faults and swapping
  for(i = 0; i < TEST_ITERATIONS; i++) {
    // Access pages in a pattern that will trigger MRU behavior
    // First half: sequential access, second half: random access
    int page_idx;
    if(i < TEST_ITERATIONS/2) {
      page_idx = i % PAGES_TO_ALLOC;  // Sequential access
    } else {
      // Random-ish access pattern to trigger more swapping
      page_idx = (i * 7 + 3) % PAGES_TO_ALLOC;
    }
    
    // Track this write
    last_write[page_idx] = i;
    
    // Write some data to the page
    for(j = 0; j < 4096; j += 64) {
      memory[page_idx][j] = (char)(i + j);
    }
    
    // Read some data from the page
    char sum = 0;
    for(j = 0; j < 4096; j += 64) {
      sum += memory[page_idx][j];
    }
    
    printf("Iteration %d: Accessed page %d, checksum = %d\n", i, page_idx, sum);
    
    // Every 5 iterations, show page statistics
    if((i + 1) % 5 == 0) {
      if(getpagestat(getpid(), &stats) == 0) {
        printf("  Page Statistics: Faults=%d, Swap-ins=%d, Swap-outs=%d\n", 
               stats.page_faults, stats.swap_ins, stats.swap_outs);
      }
      
      printf("  Current MRU List:\n");
      dumpmru();
      printf("\n");
    }
  }
  
  printf("\nFinal verification - checking data integrity...\n");
  
  // Verify that all data is still correct using tracked last writes
  int errors = 0;
  for(i = 0; i < PAGES_TO_ALLOC; i++) {
    if(last_write[i] >= 0) {
      // Check a few offsets
      int check_count = 0;
      for(j = 0; j < 256; j += 64) {
        char expected = (char)(last_write[i] + j);
        if(memory[i][j] != expected) {
          printf("ERROR: Data corruption in page %d offset %d! Expected %d, got %d (last write: iter %d)\n", 
                 i, j, expected, memory[i][j], last_write[i]);
          errors++;
          if(check_count++ > 3) break; // Don't spam too many errors
        }
      }
      if(check_count == 0) {
        printf("Page %d: Data integrity verified (last write: iter %d)\n", i, last_write[i]);
      }
    }
  }
  
  // Final statistics
  if(getpagestat(getpid(), &stats) == 0) {
    printf("\nFinal Page Statistics:\n");
    printf("  Page Faults: %d\n", stats.page_faults);
    printf("  Swap-ins: %d\n", stats.swap_ins);
    printf("  Swap-outs: %d\n", stats.swap_outs);
  }
  
  printf("\nFinal MRU List:\n");
  dumpmru();
  
  if(errors == 0 && stats.page_faults > 0) {
    printf("\n✓ MRU Memory Test PASSED!\n");
  } else if(stats.page_faults == 0) {
    printf("\n✗ MRU Memory Test FAILED - No page faults occurred (lazy allocation not working)\n");
  } else {
    printf("\n✗ MRU Memory Test FAILED - %d data corruption errors\n", errors);
  }
  
  printf("MRU Memory Test Completed!\n");
  
  exit(0);
}