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
#include "queue.h"
#include "oss.h"
time_t tstart;
int main(int argc, char ** argv){
        unlink(LOG_FILENAME);
        srand(getpid());
        tstart = time(NULL);
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
        int terminate;
        queueItem * item;
        struct ipcmsg recv;
        int seconds = 0;
        foo = createProcess();
        //int pInd = foo->local_pid & 0xff;
        // printf("pInd = %i\n", pInd);
        while (totalProcesses < 5) {//MAX_TOT_PROCS) {
                int rval;
                checkExitTime(seconds);
                rval = msgrcv(msg_id, (void *)&recv, sizeof(recv), 100, IPC_NOWAIT);
                recv.mtype = 1;
                if (rval != -1) {
                        printf("oss msg received rval = %i\n", rval);
                        printf("procId = %i\n", recv.procId);
                        printf("memref = %i\n", recv.memRef);
                        if(recv.procId > MAX_TOT_PROCS) {
                                printf("wtf proc id %d\n", recv.procId);
                                exit(-1);
                        } else {
                                char * dbit = 0;
                                //printf("msg dirty bit = %i\n", recv.dirtyBit);
                                if(recv.dirtyBit == 0) {
                                        dbit = "read";
                                }
                                else {
                                        dbit = "write";
                                }
                                snprintf(logbuf, sizeof(logbuf),
                                         "Master P%i is requesting %s of address %i at time %0d:%09d\n",
                                         foo->local_pid & 0xff, dbit, recv.memRef,
                                         osclock.seconds(), osclock.nanoseconds());
                                logger(logbuf);
                                checkRequest(recv.procId, recv.dirtyBit, dbit, recv.memRef);
                                checkExitTime(seconds);
                        }
                }
                queueDump(0);
                if ((item = dequeue(0)) != NULL) {
                        printf("got item from queue %i\n", item->processId);
                        // msgsnd(msg_id, (void *)&recv, sizeof(recv), 0);
                        // continue;
                        foo = &shm_data->ptab.pcb[item->processId];
                        // check to terminate
                        terminate = checkTerminate(foo);
                        if (terminate == 1) {
                                terminateProc(foo);
                                checkExitTime(seconds);
                        }
                        else {
                                // if not terminate check
                                //memoryRequest(foo);
                                char * dbit = 0;
                                //printf("msg dirty bit = %i\n", recv.dirtyBit);
                                if(item->dirtyBit == 0) {
                                        dbit = "read";
                                }
                                else {
                                        dbit = "write";
                                }
                                checkRequest(item->processId, item->dirtyBit, dbit, item->memAddr);
                                checkExitTime(seconds);
                        }
                }
                if (activeProcs < 2) {
                        launchNewProc();
                        // if ((shm_data->launchSec <= osclock.seconds())  &&
                        //     (shm_data->launchNano <= osclock.nanoseconds())) {
                        foo = createProcess();
                        checkExitTime(seconds);
                }
                else {
                }
        }
        checkExitTime(seconds);
}
void checkExitTime(time_t seconds) {
        if (tstart + 2 < time(0)) {
                printf("times up\n");
                bail();
        }
}
PCB * createProcess() {
        printf("\ncreateProcess\n");
        activeProcs++;
        printf("activeProcs = %i\n", activeProcs);
        totalProcesses++;
        PCB *pcb;
        // find available pcb and initialize first values
        int pcbIndex = findAvailablePcb();
        printf("pcbIndex = %i\n", pcbIndex);
        if(pcbIndex == -1) {
                printf("oss: createProcess: no free pcbs\n");
                return NULL;
        }
        //printf("oss: createProcess: available pcb %d\n", pcbIndex);
        setBit(pcbIndex);
        allocatedProcs++;
        pcb = &shm_data->ptab.pcb[pcbIndex];
        shm_data->local_pid++;
        pcb->local_pid =  pcbIndex;
        pid = pcb->pid = fork();
        if (pid == -1) {
                perror("Failed to create new process\n");
                return NULL;
        } else if (pid == 0) {
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
        initializeSig();
        initializeSharedMemory();
        msg_id = initializeMessageQueue();
        initializeFT();
        initStats();
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
        printf("***** adding frame for p%i\n", id);
        for (j = 0; j < 256; j++) {
                if (ft.frameTable[0][j] == -1) {
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
        osclock.add(0, 1053);
        printFrames();
        // removeFrame(frameNum, id, pNum);
        // printFrames();
        return frameNum;
}
int removeFrame(int fNum, int id, int pNum){
        printf("**** removing frame for p%i\n", id);
        ft.frameTable[0][fNum] = -1;
        ft.frameTable[1][fNum] = -1;
        ft.frameTable[2][fNum] = 0;//dirtyBit;
        // update pcb pageTable with frame number (j value)
        shm_data->ptab.pcb[id].pageTable[pNum] = -1;
        osclock.add(0, 15051);
        return fNum;
}
void printFrames() {
        printf("Current memory layout at time %i:%i is:\n", osclock.seconds(), osclock.nanoseconds());
        printf("\t   Occupied  DirtyBit\n");
        int j;
        for (j = 0; j < 6; j++) {
                printf("Frame %03d:", j);
                if (ft.frameTable[0][j] == -1) {
                        printf(" No \t     %i\n", ft.frameTable[2][j]);
                }
                else {
                        printf(" Yes \t     %i\n", ft.frameTable[2][j]);
                }
        }
}
int getPageNumber(int addr) {
        int pageNum = addr / 1024;
        return pageNum;
}
void checkRequest(int id, int dirtyBit, char * dbit, int addr){
        int pNum = getPageNumber(addr);
        struct ipcmsg send;
        printf("page table value = %i\n", shm_data->ptab.pcb[id].pageTable[pNum]);
        // check page table
        if (shm_data->ptab.pcb[id].pageTable[pNum] == -1) {
                osclock.add(0, 14000000);
                shm_data->pageFault += 1;
                printf("before enqueue\n");
                enqueue(0, id, addr, dirtyBit, dbit);
                addFrame(id, pNum, dirtyBit);
                snprintf(logbuf, sizeof(logbuf),
                         "Master: Address %i is not in a frame, pagefault\n", addr);
                logger(logbuf);
        }
        else {
                //grant request
                snprintf(logbuf, sizeof(logbuf),
                         "Master: Address in frame %i, giving %s data to P%i at time %i:%i\n",
                         shm_data->ptab.pcb[id].pageTable[pNum], dbit, id, osclock.seconds(),
                         osclock.nanoseconds());
                logger(logbuf);
                // strcpy(send.mtext, "done");
                send.mtype = id + 1;
                msgsnd(msg_id, (void *)&send, sizeof(send), 0);
                osclock.add(0, 5000000);
                if (dirtyBit == 1) {
                        snprintf(logbuf, sizeof(logbuf),
                                 "Master: Dirty bit of frame %i set, adding additional time to the clock\n",
                                 shm_data->ptab.pcb[id].pageTable[pNum]);
                        logger(logbuf);
                        osclock.add(0, 5);
                        snprintf(logbuf, sizeof(logbuf),
                                 "Master: Indicating to P%i that %s has happened to address %i\n",
                                 id, dbit, addr);
                        logger(logbuf);
                }
                else {
                        osclock.add(0, 10);
                }
        }
        printf("page faults = %i\n", shm_data->pageFault);
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
void terminateProc(PCB * pcb) {
    int j;
    for (j = 0; j < 32; j++) {
      if (pcb->pageTable[j] != -1) {
        ft.frameTable[0][j] = -1;
        ft.frameTable[1][j] = -1;
        ft.frameTable[2][j] = 0;
        pcb->pageTable[j] = -1;
      }
    }
    printf("killing process %i\n", pcb->pid);
    kill(pcb->pid, SIGKILL);
    printf("process %i killed\n", pcb->pid);
}
void launchNewProc() {
        //set new launch values;
        shm_data->launchSec = rand() % maxTimeBetweenNewProcsSecs + 1;
        shm_data->launchNano = rand() % maxTimeBetweenNewProcsNS + 1;
        // printf("launch %i.%i\n", shm_data->launchSec, shm_data->launchNano);
        // add to  to get new time to run
        shm_data->launchNano += osclock.nanoseconds();
        if (shm_data->launchNano > 1000000000) {
                shm_data->launchSec += shm_data->launchNano / 1000000000;
                shm_data->launchNano %= 1000000000;
        }
        else {
                shm_data->launchSec += osclock.seconds();
        }
        // printf("new launch %i.%i\n", shm_data->launchSec, shm_data->launchNano);
}
void ossClock() {
        // set up initial clock values operated by oss
        osclock.set(0, 0);
        printf("ossClock: clockInit %i:%i\n", osclock.seconds(), osclock.nanoseconds());
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
        printf("bail()\n");
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
        }
        else if (sig == SIGALRM) {
                printf("oss[%d]: Alarm raised\n", getpid());
        }
        bail();
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
