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

    initialize();

    if (totalProcesses == 0) {
        launchNewProc();
        osclock.add(shm_data->launchSec, shm_data->launchNano);
    }

    scheduler();

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

    while (totalProcesses < MAX_TOT_PROCS) {


      if (shm_data->activeProcs < PROCESSES) {
        int create = osclock.seconds() > shm_data->launchSec;

        if(!create && osclock.seconds()) {
          create = osclock.seconds() > shm_data->launchSec && osclock.nanoseconds() >= shm_data->launchNano;
        }
        create = 1;
        if(create) {
          printf("current %0d:%09d\n", osclock.seconds(), osclock.nanoseconds());
          printf("lanuch  %0d:%09d\n", shm_data->launchSec, shm_data->launchNano);
          foo = createProcess();
          launchNewProc();
        }
      }
    }

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
        srand(time(0));
        int i;
        int max = 0;
        printf("\n");

        shm_data->local_pid++;

        snprintf(indBuf, sizeof(indBuf), "%d", pcbIndex);
        if (execl(CHILD_PROGRAM, CHILD_PROGRAM, indBuf, NULL) < 0) {
          perror("execl(2) failed\n");
          exit(EXIT_FAILURE);
        }
    }
    int randNano = rand() % 500000000;
  	updateClock(0, randNano);


    osclock.add(0,1);
    return pcb;
}

void initialize() {
	//createQueues();
	initializeSharedMemory();
	//initializeMessageQueue();
  //initStats();
  ossClock();
}

void initStats() {
  int memPerSec = 0;
  int pageFault = 0;
  int avgMemAccessSpd = 0;
}
/*** rewrite statements ***/
// void printStats() {
//   printf("Number of times granted resource immediately: %02d\n", shm_data->grantNow);
//   printf("Number of times granted resource after waiting: %02d\n", shm_data->grantWait);
//   printf("Number of processes terminated by deadlock algorithm: %02d\n", shm_data->procTbyDlck);
//   printf("Number of processes who chose to terminate: %02d\n", shm_data->procChoseT);
//   printf("Number of times deadlock algorithm ran: %02d\n", shm_data->numDlckRun);
//   float avg = shm_data->avgTbyDlck / (float)totalProcesses;
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

void deinitSharedMemory() {
    if (shmdt(shm_data) == -1) {
        snprintf(perror_buf, sizeof(perror_buf), "%s: shmdt: ", perror_arg0);
        perror(perror_buf);
    }
    shmctl(shm_id, IPC_RMID, NULL);
}

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
    deinitSharedMemory();
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
