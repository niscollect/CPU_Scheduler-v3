#ifndef FIFO_H
#define FIFO_H

#include "process.h"

#define MAX_PROCESSES 100

typedef struct readyQueue
{
    Process* queue[MAX_PROCESSES];
    int front;
    int rear;
    int size;
}readyQueue;


void fifo_schedule(Process processes[], int num_processes);



#endif
