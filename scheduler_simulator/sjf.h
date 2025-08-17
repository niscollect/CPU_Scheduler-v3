#ifndef SJF_H
#define SJF_H

#include "process.h"

#define MAX_PROCESSES 100

typedef struct readyQ
{
    Process* queue[MAX_PROCESSES];
    int size;
}readyQ;

void sjf_schedule(Process processes[], int num_processes);

#endif