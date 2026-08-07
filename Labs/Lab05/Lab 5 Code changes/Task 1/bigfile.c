#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define TESTSIZE (4 * 1024 * 1024)  // 4 MB test file

int
main()
{
  int fd, i, bytes;
  char buf[512];
  
  printf("bigfile: testing large file support (doubly-indirect blocks)\n");
  printf("Creating a file with size %d bytes...\n", TESTSIZE);
  
  // Create a test file
  fd = open("/bigfile", O_CREATE | O_WRONLY);
  if(fd < 0){
    printf("bigfile: cannot create /bigfile\n");
    exit(1);
  }
  
  // Fill buffer with a pattern
  for(i = 0; i < 512; i++){
    buf[i] = (char)i;  // Pattern: 0, 1, 2, ..., 255, 0, 1, ...
  }
  
  // Write data in 512-byte chunks
  bytes = 0;
  while(bytes < TESTSIZE){
    if(write(fd, buf, 512) != 512){
      printf("bigfile: write error at byte %d\n", bytes);
      close(fd);
      exit(1);
    }
    bytes += 512;
    
    // Print progress every 256KB
    if(bytes % (256 * 1024) == 0){
      printf("bigfile: wrote %d bytes...\n", bytes);
    }
  }
  
  printf("bigfile: write complete! File size: %d bytes\n", bytes);
  close(fd);
  
  // Now read it back and verify
  printf("bigfile: verifying file contents...\n");
  fd = open("/bigfile", O_RDONLY);
  if(fd < 0){
    printf("bigfile: cannot open /bigfile for reading\n");
    exit(1);
  }
  
  bytes = 0;
  int errors = 0;
  while(bytes < TESTSIZE){
    int n = read(fd, buf, 512);
    if(n <= 0){
      printf("bigfile: read error at byte %d\n", bytes);
      break;
    }
    
    // Verify pattern
    for(i = 0; i < n; i++){
      char expected = (char)((bytes + i) % 256);
      if(buf[i] != expected){
        if(errors == 0){
          printf("bigfile: verification error at byte %d: expected %d, got %d\n", 
                 bytes + i, expected, buf[i]);
        }
        errors++;
        if(errors > 10) break;  // Stop after too many errors
      }
    }
    
    if(errors > 10) break;
    
    bytes += n;
    
    // Print progress every 256KB
    if(bytes % (256 * 1024) == 0){
      printf("bigfile: verified %d bytes...\n", bytes);
    }
  }
  
  close(fd);
  
  if(errors == 0){
    printf("bigfile: SUCCESS! All %d bytes verified correctly.\n", bytes);
    printf("bigfile: File system now supports large files!\n");
  } else {
    printf("bigfile: FAILURE! Found %d verification errors.\n", errors);
    exit(1);
  }
  
  exit(0);
}