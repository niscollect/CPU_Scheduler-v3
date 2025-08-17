#ifndef PROCESS_H
#define PROCESS_H

typedef enum {
    READY, RUNNING, BLOCKED, FINISHED
} process_state_t;

typedef struct Process
{
    char *pid;
    int arrival_time;
    
    //* I/O support
    int *cpu_bursts;        // array of CPU burst lengths [3, 4, 2]
    int *io_times;          // array of I/O times [2, 1] 
    int num_bursts;         // total number of CPU bursts
    int current_burst;      // which CPU burst we're currently on (0, 1, 2...)
    int remaining_time;     // time left in current CPU burst
    process_state_t state;  // current process state
    int io_end_time;        // when current I/O will complete
    
    // Existing flags
    int completed_flag;
    int in_queue;
} Process;

#endif