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

void attachSharedMemory();

char strbuf[20];
PCB * pcb;

void uprocInitialize();
void doit();
void attachSharedMemory();

char strbuf[20];


int main (int argc, char ** argv){
	int ptype;
	int id = atoi(argv[1]);

	srand(getpid());


	uprocInitialize();
	attachSharedMemory();
	ptype = getProcessType(id);
	updateSharedCounters(ptype);
	doit(id);
	//uprocFinished(ptype);
}

void doit(int id) {
	while(1) {
		ipcmsg msg;

		if(msgrcv(msg_id, (void *)&msg, sizeof(ipcmsg), id + 1, 0) == -1) {
			printf("error receving message\n");
			exit(-1);
		}

		msg.mtype = msg.ossid;


		// strcpy(msg.mtext, strbuf);
		// snprintf(&msg.mtext[0],sizeof(msg.mtext), "from %ld",  id);
		if (msgsnd(msg_id, (void *)&msg, sizeof(msg), 0) == -1) {
			printf("oss msg not sent");
		}
		//id = foo;

		// if(operation == PT_TERMINATE) {
		// 	printf("uproc terminated\n");
		// 	kill(getpid(), SIGKILL);
		// }
	}
}

void uprocInitialize(){
	key_t sndkey = ftok(FTOK_BASE, FTOK_MSG);

	if (sndkey == -1) {

		snprintf(perror_buf, sizeof(perror_buf), "%s: ftok: ", perror_arg0);
		perror(perror_buf);
	}

	msg_id=msgget(sndkey, 0666 );
}

int getMemAddr(int id) {
	srand(time(0));
	shm_data->ptab.pcb[id].pageNum = rand() % 32 + 1;
	printf("pageNum = %i\n", shm_data->ptab.pcb[id].pageNum);
	int randOffset = rand() % 1023 + 1;
	printf("randOffset = %i\n", randOffset);
	int offset = shm_data->ptab.pcb[id].pageNum * 1024 + randOffset;
	printf("offset = %i\n", offset);

	return offset;
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
