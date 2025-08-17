// #include <stdio.h>

// #include "process.h"
// #include "rr.h"

// // Initialize empty queue
// void init_queue(ReadyQueue* q)
// {
//     q->front = 0;
//     q->rear = 0;
//     q->size = 0;
// }

// // Add process to back of queue
// void enqueue(ReadyQueue* q, Process* p)
// {
//     if (q->size >= MAX_PROCESSES) return;  // Queue full

//     q->queue[q->rear] = p;
//     q->rear = (q->rear + 1) % MAX_PROCESSES;  //* Wrap around!
//     p->in_queue = 1;
//     q->size++;
// }
// // Remove process from front of queue
// Process* dequeue(ReadyQueue* q) {
//     if (q->size == 0) return NULL;  // Queue empty

//     Process* p = q->queue[q->front];
//     q->front = (q->front + 1) % MAX_PROCESSES;  //* Wrap around!
//     q->size--;
//     p->in_queue = 0;
//     return p;
// }

// // Check if queue is empty
// int is_empty(ReadyQueue* q) {
//     return q->size == 0;
// }

// void check_new_arrivals(Process processes[], int num_processes, int current_time, ReadyQueue* q)
// {
//     for(int i = 0; i < num_processes; i++)
//     {
//         if (processes[i].arrival_time == current_time &&
//             processes[i].completed_flag == 0 &&
//             processes[i].in_queue == 0) {  // Need this flag to avoid double-adding

//             enqueue(q, &processes[i]);
//             printf("[Time %d] %s arrived and added to queue\n", current_time, processes[i].pid);
//         }
//     }
// }

// int find_next_arrival_time(Process processes[], int num_processes, int current_time)
// {
//     int nearest_index = -1;
//     int min_diff = __INT_MAX__;

//     for(int i = 0; i < num_processes; i++)
//     {
//         int diff =  processes[i].arrival_time - current_time;

//         if(diff < min_diff && processes[i].completed_flag == 0)
//         {
//             min_diff = diff;
//             nearest_index = i;
//         }
//     }

//     // not very much needed
//     if(nearest_index == -1)
//     {
//         printf("Some error");
//         return 1;
//     }

//     return processes[nearest_index].arrival_time;
// }

// void rr_schedule(Process processes[], int num_processes, int quantum)
// {
//     int current_time = 0;
//     int completed = 0;

//     ReadyQueue ready_queue;
//     init_queue(&ready_queue);

//     //@note      RR Process:
//     //* Dequeue process from front of queue
//     //* Run it for quantum ticks (or until finished)
//     //* If not finished: Enqueue it back to rear
//     //* Repeat - dequeue next process from front

//     //@note
//     //* processes[] array comes from main.c with all processes
//     //* But we only want to put arrived processes in the ready queue
//     //* Coz not all processes are ready at time 0

//     while(completed < num_processes)
//     {
//         //* 1. Check for NEW ARRIVALS at current_time
//         // for(int i = 0; i < num_processes; i++)
//         // {
//         //     if (processes[i].arrival_time == current_time &&
//         //         processes[i].completed_flag == 0) {
//         //         enqueue(&ready_queue, &processes[i]);
//         //     }
//         // }
//         check_new_arrivals(processes, num_processes, current_time, &ready_queue);

//         //* 2. If queue is empty, handle idle time
//         if(is_empty(&ready_queue) == 1) // empty queue
//         {
//             int next_arrival = find_next_arrival_time(processes, num_processes, current_time);
//             printf("[Time %d-%d] CPU idle\n", current_time, next_arrival);
//             current_time = next_arrival;
//         }
//         //* 3. If not, dequeue and run for qunatum
//         else
//         {
//             Process* p = dequeue(&ready_queue);

//             for(int t = 0; t < quantum && p->remaining_burst_time > 0; t++)
//             {
//                 printf("[Time %d] %s is running (Remaining: %d)\n", current_time, p->pid, p->remaining_burst_time);
//                 p->remaining_burst_time--;
//                 current_time++;

//                 check_new_arrivals(processes, num_processes, current_time, &ready_queue);
//             }

//             if(p->remaining_burst_time <= 0)
//             {
//                 p->completed_flag = 1;
//                 completed++;
//             }
//             else
//             {
//                 enqueue(&ready_queue, p);
//             }

//         }
//     }

// }

//* Check I/O completions (blocked → ready) - add to rear of queue
//* Check new arrivals - add to rear of queue
//* If no process running, dequeue from front of ready queue
//* Run current process for 1 tick
//* Check if: quantum expired OR CPU burst finished OR process completed
//* Handle accordingly: preempt to rear, I/O transition, or completion

#include <stdio.h>

#include "process.h"
#include "rr.h"

void init_q(ReadyQue *rq)
{
    rq->front = 0;
    rq->rear = 0;
    rq->size = 0;
}

// Add process to back of queue
void enq(ReadyQue *rq, Process *p)
{
    // if queue is full
    if (rq->size >= MAX_PROCESSES)
        return;

    rq->queue[rq->rear] = p;
    rq->rear = (rq->rear + 1) % MAX_PROCESSES; //* Wrap around
    p->in_queue = 1;
    rq->size++;
}

// Dequeue from front of the queue
Process *deq(ReadyQue *rq)
{
    if (rq->size == 0)
        return NULL;

    Process *p = rq->queue[rq->front];
    rq->front = (rq->front + 1) % MAX_PROCESSES;
    rq->size--;
    p->in_queue = 0;
    return p;
}

int isempty(ReadyQue *rq)
{
    return rq->size == 0;
}


void print(Process processes[], int num_processes, int current_time, Process *running_process, ReadyQue rq, int completed)
{
    printf("\n=== Time %d ===\n", current_time);

    // Print what happened during I/O completions
    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].state == BLOCKED && processes[i].io_end_time == current_time)
        {
            printf("  ✓ %s completed I/O, ", processes[i].pid);
            if (processes[i].current_burst + 1 < processes[i].num_bursts)
            {
                printf("moving to CPU burst %d (duration: %d)\n",
                       processes[i].current_burst + 1,
                       processes[i].cpu_bursts[processes[i].current_burst + 1]);
            }
            else
            {
                printf("all bursts completed - FINISHED\n");
            }
        }
    }

    // Print new arrivals
    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].arrival_time == current_time)
        {
            printf("  → %s arrived and added to ready queue\n", processes[i].pid);
        }
    }

    // Print CPU assignment
    if (running_process == NULL && !isempty(&rq))
    {
        Process *next = rq.queue[rq.front]; // Front of circular queue

        printf("  🖥️  CPU assigned to %s (burst %d, duration: %d)\n",
               next->pid, next->current_burst, next->remaining_time);
    }

    // Print execution
    if (running_process != NULL)
    {
        printf("  ⚡ %s executing (remaining: %d → %d)\n",
               running_process->pid,
               running_process->remaining_time,
               running_process->remaining_time - 1);

        // Check if this will complete the burst
        if (running_process->remaining_time == 1)
        {
            if (running_process->current_burst == running_process->num_bursts - 1)
            {
                printf("     └─ Will FINISH after this tick\n");
            }
            else
            {
                int io_duration = running_process->io_times[running_process->current_burst];
                printf("     └─ Will go to I/O (duration: %d) after this tick\n", io_duration);
            }
        }
    }
    else
    {
        printf("  💤 CPU: IDLE\n");
    }

    // Print current system state
    printf("  📊 System State:\n");
    printf("     Running: ");
    if (running_process != NULL)
    {
        printf("%s (burst %d, remaining: %d after execution)\n",
               running_process->pid, running_process->current_burst,
               running_process->remaining_time - 1);
    }
    else
    {
        printf("None\n");
    }

    printf("     Ready Queue: [");
    for (int i = 0; i < rq.size; i++)
    {
        int actual_index = (rq.front + i) % MAX_PROCESSES; // Circular queue logic
        printf("%s", rq.queue[actual_index]->pid);
        if (i < rq.size - 1)
            printf(", ");
    }
    printf("]\n");

    printf("     Blocked (I/O): [");
    int first_blocked = 1;
    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].state == BLOCKED)
        {
            if (!first_blocked)
                printf(", ");
            printf("%s(ends@%d)", processes[i].pid, processes[i].io_end_time);
            first_blocked = 0;
        }
    }
    printf("]\n");

    printf("     Completed: %d/%d\n", completed, num_processes);
}

void rr_schedule(Process processes[], int num_processes, int quantum)
{
    ReadyQue rq;
    init_q(&rq);

    Process *running_process = NULL;

    int completed = 0;
    int current_time = 0;

    int quantum_counter = 0;
    const int QUANTUM = quantum; // or whatever quantum size

    while (completed < num_processes)
    {
        //* 1) Check for I/O Completions
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].state == BLOCKED && processes[i].io_end_time == current_time)
            { // i/o burst completed

                processes[i].state = READY;
                
                processes[i].current_burst++;

                // check if CPU Bursts are remaining or not
                if (processes[i].current_burst < processes[i].num_bursts)
                {

                    processes[i].remaining_time = processes[i].cpu_bursts[processes[i].current_burst];

                    if (!processes[i].in_queue)
                    {
                        enq(&rq, &processes[i]);
                    }
                }
                else
                { // No more CPU bursts - process is finished

                    processes[i].state = FINISHED;
                    processes[i].completed_flag = 1;
                    completed++;
                }
            }
        }

        //* 2) Check for new arrivals
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].arrival_time == current_time && processes[i].completed_flag == 0 && !processes[i].in_queue &&
        processes[i].state != RUNNING)
            {
                processes[i].state = READY;
                // Initialize first CPU burst
                processes[i].current_burst = 0;
                processes[i].remaining_time = processes[i].cpu_bursts[0]; //******* !!!!! *******//
                enq(&rq, &processes[i]);
            }
        }

        //* 3) If no process is running, dequeue from front of Ready Queue
        if (running_process == NULL && !isempty(&rq))
        {
            running_process = deq(&rq);
            running_process->state = RUNNING;

            quantum_counter = 0; // Fresh quantum for new process
        }

        //* 4) Run current process for 1 tick
        if (running_process != NULL)
        {
            print(processes, num_processes, current_time, running_process, rq, completed);

            running_process->remaining_time--;

            quantum_counter++; // Track quantum usage

            
            // Check if current CPU burst is complete
            if (running_process->remaining_time == 0)
            {
                //* Check if this was the last CPU burst
                if (running_process->current_burst == running_process->num_bursts - 1)
                {
                    // Process completed all bursts
                    running_process->state = FINISHED;
                    running_process->completed_flag = 1;
                    completed++;
                    running_process = NULL;
                }
                else
                {
                    // Move to I/O
                    running_process->state = BLOCKED;
                    int io_duration = running_process->io_times[running_process->current_burst];
                    running_process->io_end_time = current_time + io_duration + 1;
                    running_process = NULL;
                }
            }
            
            //* 5) Check quantum expiration first
            else if (quantum_counter >= QUANTUM)
            {
                // Quantum expired - preempt (back of line)
                running_process->state = READY;
                enq(&rq, running_process);
                running_process = NULL;
                quantum_counter = 0;
            }
        }
        else
        {
            printf("[Time %d] ", current_time);
            printf("CPU: IDLE \n");
        }

        current_time++;

        printf("\n=== FINAL: All %d processes completed ===\n", completed);
    }
}