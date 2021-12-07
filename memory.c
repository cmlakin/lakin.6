#include "config.h"

typedef struct frame {
  int processID;
  int pageNum;
  int dirtyBit;
} frame;

typedef struct frame_table {

  frame frameTable[256];
  // [3] processID, page number, dirty bit
} FT;
