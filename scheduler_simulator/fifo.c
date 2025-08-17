#include <stdio.h>
#include "fifo.h"

// Initialize empty queue
void init_queue(readyQueue *q)
{
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

// enqueue logic
void enqueue(readyQueue *q, Process *p)
{
    if (q->size >= MAX_PROCESSES)
        return; // Queue full

    q->queue[q->rear] = p;
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    p->in_queue = 1;
    q->size++;
}

// dequeue logic
Process *dequeue(readyQueue *q)
{
    if (q->size == 0)
        return NULL; // Queue empty

    Process *p = q->queue[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->size--;
    p->in_queue = 0;
    return p;
}

// Check if queue is empty
int is_empty(readyQueue *q)
{
    return q->size == 0;
}

void check_new_arrivals(Process processes[], int num_processes, int current_time, readyQueue *q)
{
    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].arrival_time == current_time && processes[i].completed_flag == 0)
        {
            enqueue(q, &processes[i]);
        }
    }
}

void printing(Process processes[], int num_processes, int current_time, Process *running_process, readyQueue rq, int completed)
{
    // Replace your printing section with this enhanced version

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
    if (running_process == NULL && !is_empty(&rq))
    {
        Process *next = rq.queue[rq.front]; // Peek at next process
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
        int idx = (rq.front + i) % MAX_PROCESSES;
        printf("%s", rq.queue[idx]->pid);
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

void fifo_schedule(Process processes[], int num_processes)
{
    int current_time = 0;
    int completed = 0;

    Process *running_process = NULL;

    readyQueue rq;
    init_queue(&rq);

    // @note
    //* Check I/O completions (blocked → ready)
    //* Check new arrivals
    //* Assign CPU if needed
    //* Execute current running process

    while (completed < num_processes)
    {

        //* 1. Check I/O completions (blocked → ready)
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].state == BLOCKED && processes[i].io_end_time == current_time)
            {

                // I/O completed, move back to READY
                processes[i].state = READY;
                processes[i].current_burst++; // Move to next CPU burst

                // Check if remaining CPU bursts
                if (processes[i].current_burst < processes[i].num_bursts)
                {
                    // More CPU bursts remaining
                    processes[i].remaining_time = processes[i].cpu_bursts[processes[i].current_burst];
                    if (!processes[i].in_queue)
                    {
                        enqueue(&rq, &processes[i]);
                    }
                }
                else
                {
                    // No more CPU bursts - process is finished
                    processes[i].state = FINISHED;
                    processes[i].completed_flag = 1;
                    completed++;
                }
            }
        }

        //* 2. Check new arrivals
        check_new_arrivals(processes, num_processes, current_time, &rq);

        //* 3. Get next process from ready queue if CPU is free
        if (running_process == NULL && !is_empty(&rq))
        {
            running_process = dequeue(&rq);
            running_process->state = RUNNING;
        }

        //* 4. Execute current running process
        if (running_process != NULL)
        {
            // printf("Running: %s (remaining: %d) \n", running_process->pid, running_process->remaining_time);
            printing(processes, num_processes, current_time, running_process, rq, completed);
            // ! run and try to understand which of the two will be up and down
            running_process->remaining_time--;

            // Check if current CPU burst is complete
            if (running_process->remaining_time == 0)
            {

                // Check if this was the last CPU burst
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
        }
        else
        {
            printf("[Time %d] ", current_time);
            printf("CPU: IDLE \n");
        }

        current_time++;
    }
}
