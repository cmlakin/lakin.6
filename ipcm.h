#pragma once



int initializeMessageQueue();

// char perror_buf[50]; // buffer for perror
// static char * perror_arg0 = "oss"; // pointer to return error value
// static int shm_id = -1; // shared memory identifier

//message buffer
typedef struct ipcmsg {
	long mtype;
	char mtext[MAX_TEXT];

	int ossid;
	int pRunSec;
	int pRunNano;
	int memRef; // adress request
  int dirtyB; // read (0) or write (1)

} ipcmsg;
