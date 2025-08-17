# CPU_Scheduler-v3
#OS

# CPU Scheduling Simulator with I/O Bursts

A complete **CPU scheduling simulator** written in C that supports realistic process execution with **CPU bursts, I/O bursts, and multiple scheduling algorithms**.

This project started as a simple **CPU-only FIFO** scheduler but evolved into a **tick-based system** that models real operating system behavior:

* Processes alternate between **CPU** and **I/O bursts**
* Full support for **process states** (`READY`, `RUNNING`, `BLOCKED`, `FINISHED`)
* Handles **arrival times**, **I/O blocking/unblocking**, and **ready queue management**
* Multiple algorithms implemented: **FIFO, SJF, STCF, Round Robin**

<hr>

## Features

* **Realistic Process Model**

  * Alternating CPU and I/O bursts
  * Arrival times & blocked state handling
  * Processes re-enter ready queue after I/O completion

* **Scheduling Algorithms Implemented**

  * **FIFO (First-Come-First-Served)** – baseline implementation
  * **SJF (Shortest Job First)** – non-preemptive
  * **STCF (Shortest Time to Completion First)** – preemptive SJF
  * **Round Robin** – quantum-based preemption

* **Core Simulation Framework**

  * Tick-by-tick execution
  * Event-driven state transitions
  * Ready queue implemented (circular or array-based depending on algorithm)
  * Detailed event logging for debugging and analysis

<hr>

## Process Structure

```c
typedef enum {
    READY, RUNNING, BLOCKED, FINISHED
} process_state_t;

typedef struct {
    char *pid;
    int arrival_time;
    int *cpu_bursts;      // e.g., [3, 4, 2]
    int *io_times;        // e.g., [2, 1]
    int num_bursts;       // number of CPU bursts
    int current_burst;    // index of current burst
    int remaining_time;   // time left in current burst
    process_state_t state;
    int io_end_time;      // when I/O completes
} Process;
```

<hr>

## Usage

1. Clone the repo:

   ```bash
   git clone https://github.com/niscollect/CPU_Scheduler-v3.git
   cd scheduler_simulator
   ```

2. Compile:

   ```bash
   gcc main.c fifo.c sjf.c stcf.c rr.c -o scheduler
   ```

3. Run with algorithm flag:

   ```bash
   ./scheduler --algorithm FIFO
   ./scheduler --algorithm SJF
   ./scheduler --algorithm STCF
   ./scheduler --algorithm RR
   ```
4. Specify quantum size of your choice (RR):

   ```bash
   ./scheduler --algorithm RR --quantum 3
   ```

<hr>

## Output Example

Each tick shows:

* Time step
* Running process (if any)
* Ready queue contents
* Blocked processes & I/O timers
* Key events (arrival, I/O completion, preemption, completion)

<hr>

## Current Status

* [x] FIFO (with I/O)
* [x] SJF (non-preemptive, with I/O)
* [x] STCF (preemptive SJF with I/O)
* [x] Round Robin (with I/O, quantum preemption)

<hr>

## What I Learned

* How **OS scheduling** really requires a tick-based model when I/O is involved
* Preemption is just "putting current process back in line"
* Queue structure choice depends on algorithm (FIFO → circular queue, SJF/STCF → array for searching)
* Edge cases (burst finishes vs quantum expiry) matter more a lot
