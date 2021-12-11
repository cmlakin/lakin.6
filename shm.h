#pragma once
#include "osclock.h"
#include "config.h"

typedef struct proc_ctrl_blck {
	int pid;
	int local_pid;
  int pageNum;
  int pageTable[32];
} PCB;

typedef struct proc_table {
	struct proc_ctrl_blck pcb[18];
} proc_table;

// shared memory
typedef struct shared_data {
	int local_pid; 	// assigned PID

	int launchSec;
	int launchNano;
	int activeProcs;
	// simulated clock
  osclock_t osclock;

	// process table
	struct proc_table ptab;

/****** may not need to be in shared memory ****/
	// report stats
	int memPerSec;
	int pageFault;
	int avgMemAccessSpd;

} shared_data;

shared_data * shm_data;
int shmGet();
shared_data * shmAttach();
void shmDetach();
