#ifndef RR_H
#define RR_H

#include "process.h"

#define MAX_PROCESSES 100

typedef struct {
    Process* queue[MAX_PROCESSES];
    int front;   // where to take from
    int rear;    // where to add
    int size;    // how many processes in queue
} ReadyQue;

void rr_schedule(Process processes[], int num_processes, int quantum);

#endif