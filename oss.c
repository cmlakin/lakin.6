#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/fcntl.h>
#include <stdbool.h>

#include "config.h"
#include "osclock.h"
#include "shm.h"
#include "ipcm.h"
#include "logger.h"
#include "memory.h"
#include "oss.h"

int main(int argc, char ** argv){

    unlink(LOG_FILENAME);
    srand(getpid());
    //FT ft;

    initialize();

    if (totalProcesses == 0) {
        launchNewProc();
        osclock.add(shm_data->launchSec, shm_data->launchNano);
    }

    //while (totalProcesses < 5) {
      scheduler();
    //}

    sleep(1);

    //printStats();

    printf("oss done\n");
    bail();
    return 0;
}

/** maybe reuse **/
void scheduler() {
    PCB * foo;

    foo = createProcess();
    //int pInd = foo->local_pid & 0xff;
    // printf("pInd = %i\n", pInd);

    // while (totalProcesses < MAX_TOT_PROCS) {
    //
    //
    //   if (shm_data->activeProcs < PROCESSES) {
    //     int create = osclock.seconds() > shm_data->launchSec;
    //
    //     if(!create && osclock.seconds()) {
    //       create = osclock.seconds() > shm_data->launchSec && osclock.nanoseconds() >= shm_data->launchNano;
    //     }
    //     create = 1;
    //     if(create) {
    //       printf("current %0d:%09d\n", osclock.seconds(), osclock.nanoseconds());
    //       printf("lanuch  %0d:%09d\n", shm_data->launchSec, shm_data->launchNano);
    //       foo = createProcess();
    //       launchNewProc();
    //     }
    //   }
    // }
    memoryRequest(foo);

}

void memoryRequest(PCB *pcb) {
	printf("oss: dispatch %d\n", pcb->local_pid & 0xff);

	// create msg to send to uproc
	struct ipcmsg send;
	struct ipcmsg recv;
  int id = pcb->local_pid & 0xff;

	memset((void *)&send, 0, sizeof(send));
	send.mtype = (pcb->local_pid & 0xff) + 1;
  send.ossid = send.mtype + 100;
	strcpy(send.mtext, "foo");
  printf("ossid %d\n", (int)send.ossid);
	while (msgsnd(msg_id, (void *)&send, sizeof(send), 0) == -1) {
		printf("oss: msg not sent to %d error %d\n", (int)send.mtype, errno);
		sleep(1);
	}

	printf("oss: msg sent to %d\n", (int)send.mtype);
	printf("msg_id %i\n", msg_id);

	printf("oss: waiting for msg\n");
  //sleep(1);
	while(msgrcv(msg_id, (void *)&recv, sizeof(recv), send.ossid, 0) == -1) {
		printf("oss: waiting for msg error %d\n", errno);
	}

	printf("oss msg received: %s\n", recv.mtext);

  char * dbit = NULL;
  printf("msg dirty bit = %i\n", recv.dirtyBit);
  if(recv.dirtyBit == 0) {
    strcpy(dbit, "read");
  }
  else {
    strcpy(dbit, "write");
  }
  snprintf(logbuf, sizeof(logbuf),
          "Master P%i is requesting %s of address %i at time %0d:%09d\n",
            pcb->local_pid & 0xff, dbit, recv.memRef,
            osclock.seconds(), osclock.nanoseconds());

  logger(logbuf);

  // calculate page number
  int addr = recv.memRef;
  printf("addr = %i\n", addr);
  int pNum = addr / 1024;
  printf("pNum = %i\n", pNum);

  int dirtyBit = recv.dirtyBit;


  checkRequest(id, pNum, dirtyBit, dbit, addr);

  printf("after checkRequest()\n");
  //printFrames();

}


PCB * createProcess() {
    printf("\ncreateProcess\n");
    shm_data->activeProcs++;
    printf("activeProcs = %i\n", shm_data->activeProcs);
    totalProcesses++;

    PCB *pcb;

    // find available pcb and initialize first values
    int pcbIndex = findAvailablePcb();
    if(pcbIndex == -1) {
        printf("oss: createProcess: no free pcbs\n");
        return NULL;
    }
    //printf("oss: createProcess: available pcb %d\n", pcbIndex);
    setBit(pcbIndex);
    allocatedProcs++;

    pcb = &shm_data->ptab.pcb[pcbIndex];

    pid = pcb->pid = fork();

    if (pid == -1) {
        perror("Failed to create new process\n");
        return NULL;
    } else if (pid == 0) {
        pcb->local_pid = shm_data->local_pid << 8 | pcbIndex;
        shm_data->local_pid++;

        // initialize pcb pageTable to all -1's
        int i;
        for (i = 0; i < 32; i++) {
          shm_data->ptab.pcb[pcbIndex].pageTable[i] = -1;
        }

        snprintf(indBuf, sizeof(indBuf), "%d", pcbIndex);
        if (execl(CHILD_PROGRAM, CHILD_PROGRAM, indBuf, NULL) < 0) {
          perror("execl(2) failed\n");
          exit(EXIT_FAILURE);
        }
    }
    srand(time(0));
    int randNano = rand() % 500000000;
  	updateClock(0, randNano);

    osclock.add(0,1);
    return pcb;
}

void initialize() {
	//createQueues();
	initializeSharedMemory();
	msg_id = initializeMessageQueue();
  initializeFT();
  //initStats();
  ossClock();
}

void initStats() {
  shm_data->memPerSec = 0;
  shm_data->pageFault = 0;
  shm_data->avgMemAccessSpd = 0;
}

// void printStats() {
//   printf("Number of memory requests per second: %02d\n", shm_data->memPerSec);
//   printf("Number of page faults: %02d\n", shm_data->pageFault);
//   float avg = shm_data->avgMemAccessSpd / (float)totalProcesses;
//   printf("Average of processes terminated by deadlock algorithm: %.2f%%\n", avg);
// }

void initializeSharedMemory() {
    shm_data = shmAttach();
    shm_data->local_pid = 1;
}

int findAvailablePcb(void) {
    int i;
    //
    // shortcut since bit vector will often be all 1s
    //
    if((g_bitVector & 0x3ffff) == 0x3ffff) {
        return -1;
    }
    for(i = 0; i < PROCESSES; i++) {
        if(!bitIsSet(i)) {
            return i;
        }
    }
    return -1;
}

void initializeFT(){
  int i;

  for (i = 0; i < 256; i++) {
    ft.frameTable[0][i] = -1;
  }
}

int addFrame(int id, int pNum, int dirtyBit) {
  //FT * frameTable;
  int j, frameNum;

  for (j = 0; j < 256; j++) {
      if (ft.frameTable[0][j] == -1){
        frameNum = j;
        // add process, page num, and dirty bit to frame table
        ft.frameTable[0][j] = id;
        ft.frameTable[1][j] = pNum;
        ft.frameTable[2][j] = dirtyBit;//dirtyBit;
        // update pcb pageTable with frame number (j value)
        shm_data->ptab.pcb[id].pageTable[pNum] = frameNum;
        // add time to clock appropriately
        // exit
        break;
      }
      else {
        frameNum = -1;
      }
  }

  printFrames();

  removeFrame(frameNum, id, pNum);
  printFrames();
  return frameNum;
}

int removeFrame(int fNum, int id, int pNum){
  ft.frameTable[0][fNum] = -1;
  ft.frameTable[1][fNum] = -1;
  ft.frameTable[2][fNum] = 0;//dirtyBit;
  // update pcb pageTable with frame number (j value)
  shm_data->ptab.pcb[id].pageTable[pNum] = -1;
  return fNum;
}

void printFrames() {
  printf("Current memory layout at time %i:%i is:\n", osclock.seconds(), osclock.nanoseconds());
  printf("\t   Occupied  DirtyBit\n");
  int j;

  for (j = 0; j < 6; j++) {
    printf("Frame %03d:", j);

    if (ft.frameTable[0][j] == -1){
      printf(" No \t     %i\n", ft.frameTable[2][j]);
    }
    else {
      printf(" Yes \t     %i\n", ft.frameTable[2][j]);
    }
  }
}

void checkRequest(int id, int pNum, int dirtyBit, char * dbit, int addr){
  printf("page table value = %i\n", shm_data->ptab.pcb[id].pageTable[pNum]);
  // check page table
  if (shm_data->ptab.pcb[id].pageTable[pNum] == -1) {
    shm_data->pageFault += 1;
    addFrame(id, pNum, dirtyBit);
    osclock.add(0, 14000000);
    snprintf(logbuf, sizeof(logbuf),
            "Master: Address %i is not in a frame, pagefault\n", addr);

    logger(logbuf);
  }
  else {
    //grant request

    snprintf(logbuf, sizeof(logbuf),
            "Master: Address in frame %i, giving data to P%i at time %i:%i\n",
            shm_data->ptab.pcb[id].pageTable[pNum], id, osclock.seconds(),
            osclock.nanoseconds());

    logger(logbuf);

    if (dirtyBit == 1) {
      snprintf(logbuf, sizeof(logbuf),
              "Master: Dirty bit of frame %i set, adding additional time to the clock\n",
                shm_data->ptab.pcb[id].pageTable[pNum]);

      logger(logbuf);
      osclock.add(0, 5000000);
      snprintf(logbuf, sizeof(logbuf),
              "Master: Indicating to P%i that %s has happened to address %i\n",
              id, dbit, addr);

      logger(logbuf);
      osclock.add(0, 5000000);
    }
    else {
      osclock.add(0, 10);
    }
  }

  printf("page faults = %i\n", shm_data->pageFault);
}

void updatePageTable(){

}

void launchNewProc() {
    //set new launch values;
    shm_data->launchSec = rand() % maxTimeBetweenNewProcsSecs + 1;
    shm_data->launchNano = rand() % maxTimeBetweenNewProcsNS + 1;
    // printf("launch %i.%i\n", shm_data->launchSec, shm_data->launchNano);

    // add to  to get new time to run
    shm_data->launchSec += osclock.seconds();
    shm_data->launchNano += osclock.nanoseconds();
    // printf("new launch %i.%i\n", shm_data->launchSec, shm_data->launchNano);
}

void ossClock() {
    // set up initial clock values operated by oss
    osclock.set(0, 0);
    printf("ossClock: clockInit %i:%i\n", osclock.seconds(), osclock.nanoseconds());
}

// void deinitSharedMemory() {
//     if (shmdt(shm_data) == -1) {
//         snprintf(perror_buf, sizeof(perror_buf), "%s: shmdt: ", perror_arg0);
//         perror(perror_buf);
//     }
//     shmctl(shm_id, IPC_RMID, NULL);
// }

void setBit(int b) {
    g_bitVector |= (1 << b);
}

// test if int i is set in bv
bool bitIsSet(int b) {
    return (g_bitVector & (1 << b)) != 0;
}

void clearBit(int b) {
    g_bitVector &= ~(1 << b);
}

void doSigHandler(int sig) {
    if (sig == SIGTERM) {
        // kill child process - reconfig to work with current code
        kill(getpid(), SIGKILL); // resend to child
    }
}

void bail() {
    shmDetach();
    mqDetach();
    kill(0, SIGTERM);
    exit(0);
}

void sigHandler(const int sig) {
    sigset_t mask, oldmask;
    sigfillset(&mask);

    // block all signals
    sigprocmask(SIG_SETMASK, &mask, &oldmask);

    if (sig == SIGINT) {
        printf("oss[%d]: Ctrl-C received\n", getpid());
        bail();
    }
    else if (sig == SIGALRM) {
        printf("oss[%d]: Alarm raised\n", getpid());
        bail();
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
}

int initializeSig() {
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask); // ignore next signals

    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    // alarm and Ctrl-C(SIGINT) have to be handled
    sa.sa_handler = sigHandler;
    if ((sigaction(SIGALRM, &sa, NULL) == -1) ||    (sigaction(SIGINT, &sa, NULL) == -1)) {
        perror("sigaction");
        return -1;
    }
    alarm(ALARM_TIME);
    return 0;
}
