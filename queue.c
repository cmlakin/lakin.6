#include <stdbool.h>

#include "config.h"
#include "queue.h"


Queue all_queues[576];

int queueShift(Queue * queue);
void queuePush(Queue * queue, int processId);

queueItem * newItem(int processId) {
    queueItem * new = (queueItem *)malloc(sizeof(queueItem));

    new->next = NULL;
    new->processId = processId;
    return new;
}

void enqueue(int index, int processId) {
    queuePush(&all_queues[index], processId);
}

int dequeue(int index) {
    return queueShift(&all_queues[index]);
}

void queuePush(Queue * queue, int processId) {
    queueItem  * new = newItem(processId);


    // printf("push head %x tail %lx\n", (int)q->head, (long)q->tail);

    if(queue->tail == NULL) {
        queue->tail = new;
        queue->head = queue->tail;
    } else {
        queue->tail->next = new;
        queue->tail = new;
    }
    return;
}

int queueShift(Queue * queue) {
    queueItem * item = queue->head;

    if(item == NULL) {
        queue->tail = NULL;
        return -1;
    }
    queue->head = queue->head->next;
    if(queue->head == NULL) {
        queue->tail = NULL;
    }
    return item->processId;
}

void queueDump(int index) {
    Queue * q = &all_queues[index];

    queueItem *h = q->head;


    while(h != NULL) {
      printf("%d\n", h->processId);
        h = h->next;
    }

}
