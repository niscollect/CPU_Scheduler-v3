#ifndef STCF_H
#define STCF_H

#include "process.h"

#define MAX_PROCESSES 100

typedef struct ReadyQ
{
    Process* queue[MAX_PROCESSES];
    int size;
}ReadyQ;

void stcf_schedule(Process processes[], int num_processes);

#endif