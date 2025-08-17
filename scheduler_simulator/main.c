#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fifo.h"
#include "sjf.h"
#include "stcf.h"
#include "rr.h" // future addition

#define NUM_PROCESSES 3

int main(int argc, char *argv[])
{

    //* default algorithm
    char *algorithm = "FIFO";
    //* default quantum = 1
    int quantum = 1;

    // Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--algorithm") == 0)
        {
            algorithm = argv[++i];
        }
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--quantum") == 0)
        {
            quantum = atoi(argv[++i]);
        }
    }

    Process processes[NUM_PROCESSES] = {
        {
            .pid = "P1",
            .arrival_time = 0,
            .cpu_bursts = (int[]){3, 2, 1},
            .io_times = (int[]){2, 2},
            .num_bursts = 3,
            .current_burst = 0,
            .remaining_time = 4, // Will be set properly in initialization
            .state = READY,
            .io_end_time = -1,
            .completed_flag = 0,
            .in_queue = 0
        },
        {
            .pid = "P2", 
            .arrival_time = 2,
            .cpu_bursts = (int[]){3, 1},        // CPU: 3 → I/O →   CPU: 1
            .io_times = (int[]){1},             // I/O duration: 1
            .num_bursts = 2,
            .current_burst = 0,
            .remaining_time = 3,
            .state = READY,
            .io_end_time = -1,
            .completed_flag = 0,
            .in_queue = 0
        },
        {
           .pid = "P3",
            .arrival_time = 5,
            .cpu_bursts = (int[]){2},           // Just CPU: 2 (no I/O)
            .io_times = NULL,
            .num_bursts = 1,
            .current_burst = 0,
            .remaining_time = 2,
            .state = READY,
            .io_end_time = -1,
            .completed_flag = 0,
            .in_queue = 0
        }
    };

    if (strcmp(algorithm, "FIFO") == 0)
        fifo_schedule(processes, NUM_PROCESSES);
    else if (strcmp(algorithm, "SJF") == 0)
        sjf_schedule(processes, NUM_PROCESSES);
    else if (strcmp(algorithm, "STCF") == 0)
        stcf_schedule(processes, NUM_PROCESSES);
    else if (strcmp(algorithm, "RR") == 0)
        rr_schedule(processes, NUM_PROCESSES, quantum);
    else
        printf("Unknown scheduling algorithm.\n");

    return 0;
}
