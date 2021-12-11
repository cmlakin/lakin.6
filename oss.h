#pragma once
#include "config.h"

#ifndef CREATED_STATICS
//
// if statics not created yet do so
//
#define CREATED_STATICS
static unsigned long maxTimeBetweenNewProcsNS = 100;
static int maxTimeBetweenNewProcsSecs = 1;
static int g_bitVector = 0;
static char * perror_arg0 = "oss"; // pointer to return error value
static int msg_id = -1;
#else
//
// otherwise just refer to them
//
extern unsigned long maxTimeBetweenNewProcsNS = 100;
extern int maxTimeBetweenNewProcsSecs = 1;
extern int g_bitVector = 0;
extern char * perror_arg0 = "oss"; // pointer to return error value
extern int msg_id = -1;
#endif

char perror_buf[50]; // buffer for perror

//static int shm_id = -1; // shared memory identifier

//int g_bitVector = 0;
// char logbuf[200];
char indBuf[2];
pid_t pid;

typedef struct frame_table {
  int frameTable[3][256];
} FT;

FT ft;
void initialize();
void initializeSharedMemory();
//void initializeMessageQueue();
struct proc_ctrl_blck  * createProcess();

void createMessageQueue();
void launchNewProc();
void ossClock();
void deinitSharedMemory();
void setBit(int);
bool bitIsSet(int);
void clearBit(int);
void bail();
void sigHandler(const int);
int initializeSig();
void scheduler();
int findAvailablePcb(void);
void printStats();
void initStats();
void memoryRequest();
void checkRequest(int, int, int, char *, int);
void updatePageTable();
int addFrame(int, int, int);
int removeFrame(int, int, int);
void initializeFT();
void printFrames();
void swapFrames();
