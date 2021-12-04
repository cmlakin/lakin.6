#pragma once
//#include "shm.h"


//
// priority queues items
//
typedef struct queueItem {
    struct queueItem * next;
    int processId;
} queueItem;
//
// priority queue
//
typedef struct Queue {
    void (*enqueue)(int pid);
    int (*dequeue)(void);
    struct queueItem * head;
    struct queueItem * tail;
} Queue;

extern Queue all_queues[RESOURCES];
//void queueDump(int index, char * indent);


void enqueue(int, int);
int dequeue(int);
void queueDump(int);
