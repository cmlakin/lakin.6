#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "config.h"
#include "shm.h"
#include "user_proc.h"
#include "oss.h"
#include "ipcm.h"




int initializeMessageQueue() {
	// messages
	key_t msgkey = ftok(FTOK_BASE, FTOK_MSG);

	if (msgkey == -1) {
		snprintf(perror_buf, sizeof(perror_buf), "%s: ftok: ", perror_arg0);
		perror(perror_buf);
		//return -1;
	}

	msg_id = msgget(msgkey, 0666 | IPC_CREAT);
	if (msg_id == -1) {
		printf("Error creating queue\n");
	}
  return msg_id;
}

void mqDetach() {
  if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Message queue could not be deleted.\n");
		exit(EXIT_FAILURE);
	}
}
