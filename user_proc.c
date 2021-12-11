#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "oss.h"
#include "config.h"
#include "shm.h"
#include "ipcm.h"
#include "memory.h"
#include "user_proc.h"


char perror_buf[50];
const char * perror_arg1 = "user_proc";
//static int shm_id;
static int msg_id;

char strbuf[20];
PCB * pcb;

int main (int argc, char ** argv){
	int id = atoi(argv[1]);

	srand(getpid());


	uprocInitialize();
	attachSharedMemory();
	doit(id);
	//uprocFinished(ptype);
}

void doit(int id) {
	//while(1) {
		ipcmsg msg;
		printf("user_proc id = %i\n", id);
		printf("user_proc waiting for message\n");
		if(msgrcv(msg_id, (void *)&msg, sizeof(ipcmsg), id + 1, 0) == -1) {
			printf("error receving message\n");
			exit(-1);
		}
		printf("user_proc receving message\n");
		printf("suer_proc msg received: %s\n", msg.mtext);
		msg.mtype = msg.mtype + 100;
		int request = getMemAddr();
		//printf("after getMemAddr\n");

		msg.memRef = request;
		msg.dirtyBit = setDirtyBit(id);
		printf("memref = %i\n", msg.memRef);
		strcpy(msg.mtext, "bar");
		// strcpy(msg.mtext, strbuf);
		// snprintf(&msg.mtext[0],sizeof(msg.mtext), "from %ld",  id);
		if (msgsnd(msg_id, (void *)&msg, sizeof(msg), 0) == -1) {
			printf("user_proc msg not sent");
		}
		//id = foo;
		printf("user_proc sent msg\n");
		// if(operation == PT_TERMINATE) {
		// 	printf("uproc terminated\n");
		// 	kill(getpid(), SIGKILL);
		// }
	//}
}

void uprocInitialize(){
	msg_id = initializeMessageQueue();
}

int getMemAddr() {
	//srand(time(0));
	int pageNum = rand() % 32 + 1;
	printf("pageNum = %i\n", pageNum);
	int offset = rand() % 1023 + 1;
	printf("randOffset = %i\n", offset);
	int memReq = pageNum * 1024 + offset;
	printf("memReq = %i\n", memReq);

	return memReq;
}

int setDirtyBit(int id) {
	//srand(time(0));
	int randDB = rand() % 10;
	printf("---randDB = %i\n", randDB);

	if (randDB < PROB_DIRTY_BIT) {
		randDB = 1;
	}
	else {
		randDB = 0;
	}

	return randDB;
}

void loop(int id){
  struct timespec start;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  //srand(time(0));
  //int randNum = rand() % 250000000000 + 1;
  printf("in loop()\n");
  while(true) {
      struct timespec now;
      long diff;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
    diff = start.tv_nsec - now.tv_nsec;

    if (diff > 1000000000) {
      break;
    }
  }

  // srand(time(0));
  // int randNum = rand() % 10 + 1;
	//
  // if (randNum < PROB_RELEASE) {
  //   releaseResources(id);
  // }
  // else if (randNum < PROB_TERMINATE) {
  //   procTerminate(id);
  // }
  // else {
  //   requestResources(id);
  // }
  // int randNano = rand() % 500000000;
  // updateClock(0, randNano);

}



void attachSharedMemory() {
  printf("uproc attachSharedMemory\n");
  shm_data = shmAttach();
}
