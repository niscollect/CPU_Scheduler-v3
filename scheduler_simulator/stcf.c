// #include <stdio.h>

// #include "process.h"
// #include "stcf.h"


// int stcf_find_shortest_arrived_process(Process processes[], int num_processes, int current_time)
// {
//     int shortest_process_index = -1;
//     int min_remaining_burst_time = __INT_MAX__;

//     for(int i = 0; i < num_processes; i++)
//     {

//     //     printf("Time %d: Checking P%s, arrived=%d, remaining=%d\n", 
//     //    current_time, processes[i].pid, 
//     //    processes[i].arrival_time <= current_time, 
//     //    processes[i].remaining_burst_time);

//         if(processes[i].remaining_burst_time < min_remaining_burst_time && processes[i].completed_flag == 0 && processes[i].arrival_time <= current_time)
//         {
//             shortest_process_index = i;
//             min_remaining_burst_time = processes[i].remaining_burst_time;
//         }
//     }

//     return shortest_process_index;
// }

// int stcf_find_nearest_arriving_process(Process processes[], int num_processes, int current_time)
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

//     return nearest_index;

// }


// void stcf_schedule(Process processes[], int num_processes)
// {
//     int current_time = 0;
//     int completed = 0;

//     int current_running = -1;  //* Index of currently running process
    

//     while (completed < num_processes)
//     {
//         // find shortest job among arrived processes
//         int shortest_idx = stcf_find_shortest_arrived_process(processes, num_processes, current_time);
        
        
//         if(shortest_idx == -1)
//         {
//             int nearest_idx = stcf_find_nearest_arriving_process(processes, num_processes, current_time);
//             // skip current time to the very next arriving process' arrival time
//             printf("CPU idle from %d to %d\n", current_time, processes[nearest_idx].arrival_time);
//             current_time = processes[nearest_idx].arrival_time; 
//             continue;
//         }
        
//         //* preempt if needed
//         if(shortest_idx != current_running)
//         {
//             current_running = shortest_idx;
//             printf("Context switch to %s\n", processes[shortest_idx].pid);
//         }

//         // run one tick
//         printf("[Time %d] %s is running (Remaining: %d)\n", current_time, processes[current_running].pid, processes[current_running].remaining_burst_time);
//         processes[shortest_idx].remaining_burst_time--;




//         // mark it completed if it has completed
//         if(processes[shortest_idx].remaining_burst_time == 0) {
//             // Mark completed
//             processes[shortest_idx].completed_flag = 1;
//             completed++;
//         }

//         current_time++;
//     }
    
// }


#include <stdio.h>
#include "stcf.h"

// Initialize empty queue
void Init_Queue(ReadyQ *q)
{
    q->size = 0;
}

// Enqueue logic
void Enqueue_(ReadyQ *q, Process *p)
{
    if (q->size >= MAX_PROCESSES)
        return; // Queue full

    q->queue[q->size] = p; // Add at the end
    p->in_queue = 1;
    q->size++;
}

// dequeue laogic
Process *dequeue_Shortest(ReadyQ *q)
{
    if (q->size == 0)
        return NULL; // Queue empty

    int shortest_time = __INT_MAX__;
    int shortest_index = 0;

    for (int i = 0; i < q->size; i++)
    {
        if (q->queue[i]->remaining_time < shortest_time)
        {
            shortest_time = q->queue[i]->remaining_time;
            shortest_index = i;
        }
    }

    Process *shortest_process = q->queue[shortest_index];

    // Shift all elements left
    for (int i = shortest_index; i < q->size - 1; i++)
    {
        q->queue[i] = q->queue[i + 1];
    }

    q->queue[q->size - 1] = NULL; // Optional safety step
    q->size--;

    shortest_process->in_queue = 0;

    return shortest_process;
}

// Check if queue is empty
int Is_Empty(ReadyQ *q)
{
    return q->size == 0;
}

void Check_New_Arrivals(Process processes[], int num_processes, int current_time, ReadyQ *q)
{
    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].arrival_time == current_time && processes[i].completed_flag == 0)
        {
            Enqueue_(q, &processes[i]);
        }
    }
}

int find_shortest_in_queue(ReadyQ *q) {
    
    if (q->size == 0)
        return __INT_MAX__;  // No processes in queue
        
    int shortest_time = __INT_MAX__;
    
    for (int i = 0; i < q->size; i++) 
    {
        if (q->queue[i]->remaining_time < shortest_time) 
        {
            shortest_time = q->queue[i]->remaining_time;
        }
    }
    
    return shortest_time;
}

void Printing_(Process processes[], int num_processes, int current_time, Process *running_process, ReadyQ rq, int completed)
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
    if (running_process == NULL && !Is_Empty(&rq))
    {
        int shortest_idx = 0;
        for (int i = 1; i < rq.size; i++)
        {
            if (rq.queue[i]->remaining_time < rq.queue[shortest_idx]->remaining_time)
            {
                shortest_idx = i;
            }
        }
        Process *next = rq.queue[shortest_idx];

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
        printf("%s", rq.queue[i]->pid);
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

void stcf_schedule(Process processes[], int num_processes)
{
    ReadyQ rq;
    Init_Queue(&rq);

    Process *running_process = NULL;

    int current_time = 0;
    int completed = 0;

    // @note
    //* Check I/O completions (blocked → ready)
    //* Check new arrivals

    //* Check if currently running process should be preempted

    //* Assign CPU if needed  --------- Get next process from ready queue if CPU is free
    //* Execute current running process

    while (completed < num_processes)
    {
        //* 1) Check I/O completions (blocked -> ready)
        for (int i = 0; i < num_processes; i++)
        {
            if (processes[i].state == BLOCKED && processes[i].io_end_time == current_time)
            { // i/o burst completed

                processes[i].state = READY;
                processes[i].current_burst++; // Move to next CPU burst

                // Check if CPU bursts are remaining or not
                if (processes[i].current_burst < processes[i].num_bursts)
                { // More CPU bursts remaining

                    processes[i].remaining_time = processes[i].cpu_bursts[processes[i].current_burst];
                    
                    if (!processes[i].in_queue)
                    {
                        Enqueue_(&rq, &processes[i]);
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

        //* 2) Check new arrivals
        Check_New_Arrivals(processes, num_processes, current_time, &rq);


        //* 3) Check if currently running process needs to be preempted
        if (running_process != NULL && !Is_Empty(&rq)) {
            // Find shortest in ready queue
            int shortest_remaining = find_shortest_in_queue(&rq);
            if (shortest_remaining < running_process->remaining_time) 
            {
                // Preempt: move current process back to ready queue
                running_process->state = READY;
                Enqueue_(&rq, running_process);
                running_process = NULL;  // Will be reassigned in next step
            }
        }


        //* 4) Assign CPU if needed
        if (running_process == NULL && !Is_Empty(&rq))
        {
            running_process = dequeue_Shortest(&rq);
            running_process->state = RUNNING;
        }

        //* 5) Execution
        if (running_process != NULL)
        {
            // printf("Running: %s (remaining: %d) \n", running_process->pid, running_process->remaining_time);
            Printing_(processes, num_processes, current_time, running_process, rq, completed);
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