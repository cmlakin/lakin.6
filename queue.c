#include <stdbool.h>

#include "config.h"
#include "queue.h"


Queue all_queues[1];

queueItem * queueShift(Queue * queue);
void queuePush(Queue * queue, int processId, int page, int dirty);

queueItem * newItem(int processId, int page, int dirty, char * dbit) {
    queueItem * new = (queueItem *)malloc(sizeof(queueItem));

    new->next = NULL;
    new->processId = processId;
    new->page = page;
    new->dirty = dirty;
    new->dbit = dbit;
    return new;
}

void enqueue(int index, int processId, int page, int dirty, char * dbit) {
    queuePush(&all_queues[index], processId, page, dirty);
}

queueItem * dequeue(int index) {
    return queueShift(&all_queues[index]);
}

void queuePush(Queue * queue, int processId, int page, int dirty, char * dbit) {
    queueItem  * new = newItem(processId, page, dirty);


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

queueItem * queueShift(Queue * queue) {
    queueItem * item = queue->head;

    if(item == NULL) {
        queue->tail = NULL;
        return NULL;
    }
    queue->head = queue->head->next;
    if(queue->head == NULL) {
        queue->tail = NULL;
    }
    return item;
}

void queueDump(int index) {
    Queue * q = &all_queues[index];

    queueItem *h = q->head;


    while(h != NULL) {
      printf("%d\n", h->processId);
        h = h->next;
    }

}
