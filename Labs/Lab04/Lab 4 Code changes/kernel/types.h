typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

// Page statistics structure for getpagestat syscall
struct pagestat {
  int page_faults;
  int swap_ins;
  int swap_outs;
};
