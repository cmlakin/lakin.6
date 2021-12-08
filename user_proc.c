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
static int shm_id;
static int msg_id;
static int foo;

void attachSharedMemory();

char strbuf[20];
PCB * pcb;

void uprocInitialize();
void doit();
void attachSharedMemory();

char strbuf[20];


int main (int argc, char ** argv){

	int id = atoi(argv[1]);

	foo = id;

	srand(getpid());

	//int addr = getMemAddr();
	uprocInitialize();
	attachSharedMemory();
	doit(id);
	//uprocFinished(ptype);
}

void doit(int id) {
	while(1) {
		ipcmsg msg;
		printf("user_proc waiting for msg from oss\n");
		if(msgrcv(msg_id, (void *)&msg, sizeof(ipcmsg), id + 1, 0) == -1) {
			printf("u_proc error receving message\n");
			exit(-1);
		}
		printf("user_proc recv msg from oss\n");

		msg.mtype = msg.ossid;

		foo = id;
		// strcpy(msg.mtext, strbuf);
		// snprintf(&msg.mtext[0],sizeof(msg.mtext), "from %ld",  id);
		if (msgsnd(msg_id, (void *)&msg, sizeof(msg), 0) == -1) {
			printf("oss msg not sent");
		}
		printf("user_proc sent msg to oss\n");
	}
}

void uprocInitialize(){
	msg_id = initializeMessageQueue() ;
}

int getMemAddr(int id) {
	srand(time(0));
	int pageNum = rand() % 32;
	printf("pageNum = %i\n", pageNum);
	int offset = rand() % 1024;
	printf("offset = %i\n", offset);
	int memAddr = pageNum * 1024 + offset;
	printf("memAddr = %i\n", memAddr);

	return memAddr;
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
