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
}

void doit(int id) {
	while(1) {
		ipcmsg msg;
		int terminate;

		terminate = checkTerminate();
		if (terminate == 1) {
			strcpy(msg.mtext, "terminate");
			//printf("proc wants to terminate\n");
		}
		else {
			strcpy(msg.mtext, "run");
			//printf("proc wants to run\n");
		}

		msg.mtype = 100;
		int request = getMemAddr();
		//printf("after getMemAddr\n");
		msg.procId = id;
		msg.memRef = request;
		msg.dirtyBit = setDirtyBit(id);
		//printf("memref = %i\n", msg.memRef);
		strcpy(msg.mtext, "bar");
		// strcpy(msg.mtext, strbuf);
		// snprintf(&msg.mtext[0],sizeof(msg.mtext), "from %ld",  id);
		if (msgsnd(msg_id, (void *)&msg, sizeof(msg) - sizeof(long), 0) == -1) {
			//printf("user_proc msg not sent");
			exit(0);
		}
		shm_data->memPerSec++;

		//printf("user_proc sent msg\n");

		// printf("user_proc id = %i\n", id);
		//printf("user_proc waiting for message\n");
		if(msgrcv(msg_id, (void *)&msg, sizeof(ipcmsg) - sizeof(long), id + 1, 0) == -1) {
			//printf("error receving message\n");
			exit(-1);
		}
		//printf("user_proc receving message\n");
		// printf("suer_proc msg received: %s\n", msg.mtext);
	}
}

void uprocInitialize(){
	msg_id = initializeMessageQueue();
}

int getMemAddr() {
	//srand(time(0));
	int pageNum = rand() % 32;
	//printf("pageNum = %i\n", pageNum);
	int offset = rand() % 1024;
	//printf("randOffset = %i\n", offset);
	int memReq = pageNum * 1024 + offset;
	//printf("memReq = %i\n", memReq);

	return memReq;
}

int setDirtyBit(int id) {
	//srand(time(0));
	int randDB = rand() % 10;
	//printf("---randDB = %i\n", randDB);

	if (randDB < PROB_DIRTY_BIT) {
		randDB = 1;
	}
	else {
		randDB = 0;
	}

	return randDB;
}

int checkTerminate() {
  int termTime = rand() % 200 - 100;
  //printf("_______termTime = %i\n", termTime);
  termTime = termTime + 1000 + osclock.nanoseconds();
  //printf("_______termTime2 = %i\n", termTime);
  if (termTime <= osclock.nanoseconds()) {
    return 1;
  }
  else {
    return 0;
  }
}

void attachSharedMemory() {
  //printf("uproc attachSharedMemory\n");
  shm_data = shmAttach();
}
