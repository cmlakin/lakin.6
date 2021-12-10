#include "config.h"


void initializeFT(){
  FT * frameTable;
  int i;

  for (i = 0; i < 256; i++) {
    frameTable[0][i] = -1;
  }
}

int addToFrame(int id, int pNum) {
  FT * frameTable;
  int i, j, frameNum;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < 3; i++) {
      if (frameTable[i][j] == -1){
        frameNum = j;
        // add process, page num, and dirty bit to frame table
        // update pcb pageTable with frame number (j value)
        // add time to clock appropriately
        // exit
      }
      else {
        frameNum = -1;

      }
    }
  }

  return frameNum;
}

void remFromFrame(){

}
