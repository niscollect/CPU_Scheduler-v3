# IO Bursts

**What we're missing:**

- Process states: READY, RUNNING, **BLOCKED** (for I/O)
- CPU bursts + I/O bursts alternating pattern
- I/O completion handling (blocked → ready transitions)
- Workload files with I/O specifications

**Current implementation is simplified** - all processes are purely CPU-bound with no I/O blocking.

**What we need to add:**

**1. Process Structure Changes:**

```c
typedef enum {
    READY, RUNNING, BLOCKED, FINISHED
} process_state_t;
typedef struct {
    char *pid;
    int arrival_time;
    int *cpu_bursts;      // array: [3, 4, 2] 
    int *io_times;        // array: [2, 1]
    int num_bursts;       // 3 CPU bursts
    int current_burst;    // which burst we're on
    int remaining_time;   // time left in current burst
    process_state_t state;
    int io_end_time;      // when I/O completes
    // existing flags...
} Process;
```

**2. Main Loop Changes:** Every algorithm needs to handle I/O completions at each tick.

#### **Initial Setup (v1 & early v2)**

The scheduler originally handled **CPU-only** processes. The approach was textbook FIFO:

- Iterate over the array of processes (assumed sorted by arrival time)
- Run each to completion
- No dynamic state changes or interruptions

**Limitation identified**: The model was too simplified. Real processes **alternate between CPU and I/O**, and this wasn't accounted for.

<hr>

#### **Problem Realization**

We realized that we completely skipped I/O handling in implementation:

- No `BLOCKED` state
- No I/O bursts
- No re-entry into the ready queue after I/O

This made our simulation **unrealistic** and also less impressive as a learning tool or a demonstrable project.

<hr>

#### **Design Decisions for v2 with I/O**

1. **Process Alternates Between CPU and I/O**  
    Structure decided:
    - `cpu_bursts[]` → array of CPU bursts
    - `io_times[]` → array of I/O times (between the CPU bursts)  
        Example:  
        `P1: CPU(3) → IO(2) → CPU(4) → IO(1) → CPU(2)`
2. **Updated Process Structure**
    - Added `cpu_bursts`, `io_times`, `current_burst`, `remaining_time`, `state`, `io_end_time`
    - Replaced old single `remaining_burst_time`
3. **Rejected Alternate Idea**  
    Considered an implicit I/O model (e.g., "burst at 3rd tick, then 6th") but dropped it in favor of the **standard array-based model** due to:
    - Poor clarity
    - Difficulty in testing

<hr>

#### **New Scheduling Loop (Tick-by-Tick)**

We agreed that **for I/O**, a tick-based simulation loop is essential. Why?

- Process may block anytime
- Other processes may arrive or unblock during that time
- Static for-loop over sorted process array becomes insufficient

**Key additions in loop:**

- I/O Completion Handler (`BLOCKED → READY`)
- Arrival Handler (newly arrived → `READY`)
- CPU Selection (FIFO among `READY`)
- Tick-by-tick execution of `RUNNING` process
- State transitions (e.g., to `BLOCKED` after CPU burst)

<hr>

#### **Ready Queue**

Ready queue is implemented  
FIFO rule now applies to the **earliest-arrived READY process**, not the next index.

<hr>

## **Actually Building This Thing**

Started implementing the ready queue first. Went with a circular queue - `front`, `rear`, `size` tracking. Pretty standard stuff.

Wrote the basic functions:

- `init_queue()` - sets everything to 0
- `enqueue()` - adds to rear, wraps around with modulo
- `dequeue()` - gets from front, moves front pointer
- `is_empty()` - just checks if size is 0

Had a stupid bug in `dequeue()` - was passing an unused `Process* p` parameter but then declaring another `Process* p` inside. Obviously conflicted. Removed the parameter.

Now for the main loop. This is where it gets interesting.

#### **The Four-Step Dance**

Realized I need to do things in a specific order each tick:

**Step 1: Check who finished I/O** This was tricky. Loop through all processes, find ones that are BLOCKED and their `io_end_time` matches current time. When found:

- Set state back to READY
- Increment `current_burst` (move to next CPU burst)
- Either add back to ready queue OR mark as finished if no more bursts

**Step 2: Check new arrivals**  
Made a separate function `check_new_arrivals()`. Just checks if any process has `arrival_time == current_time` and adds them to ready queue. Simple.

Had another bug here - was passing `&rq` to `enqueue()` but `rq` is already a pointer. Fixed.

**Step 3: Assign CPU** Only if no process is running AND ready queue isn't empty:

- Dequeue next process (FIFO order)
- Set its state to RUNNING
- Store in `running_process` pointer

**Step 4: Execute current process** If someone's running:

- Decrement their `remaining_time`
- If it hits 0, handle burst completion:
    - Last burst? → Mark FINISHED, increment completed counter
    - More bursts? → Move to BLOCKED, calculate `io_end_time`

#### **Timing Questions That Confused Me**

Got confused about when exactly a process "executes". Like if it needs 3 ticks, does it run during the tick when remaining_time goes 3→2, 2→1, 1→0?

Answer: YES. The decrement represents work being done. So:

- Time 0: "remaining: 3" → executes → 2 left
- Time 1: "remaining: 2" → executes → 1 left
- Time 2: "remaining: 1" → executes → 0 left, burst complete

The process IS running during that final tick.

#### **I/O Timing Calculation**

When a process goes to I/O:

```c
io_end_time = current_time + io_duration + 1
```

The +1 is because I/O starts AFTER the current tick ends. Took me a bit to get this right.

#### **Event Ordering Decision**

Decided to check I/O completions BEFORE new arrivals. This means if P1 finishes I/O at the same time P2 arrives, P1 gets priority in the ready queue. Seems more fair - P1 was already in the system.

#### **No Separate Blocked Queue**

Could have made another queue for blocked processes, but decided against it. Just iterate through the process array each tick to check I/O completions. Simpler, and for the number of processes we're dealing with, performance is fine.

#### **Adding Better Printing**

Started with basic printing but realized I couldn't see what was happening. Added much more detailed output:

- Shows what events happened (I/O completions, arrivals)
- Current execution state
- Ready queue contents
- Blocked processes with their I/O end times

Makes debugging SO much easier.

#### **Test Case Setup**

Made a meaningful test:

- P1: arrives at 0, has multiple CPU/I/O cycles
- P2: arrives at 2 while P1 is running, also has I/O
- P3: arrives at 5, CPU-only (no I/O)

This tests everything - queuing, I/O handling, mixed workloads.

#### **Current State**

It's working! The scheduler properly:

- Maintains FIFO order in ready queue
- Handles I/O blocking/unblocking
- Transitions states correctly
- Completes processes when done

Still need to test with more complex cases, but the core logic is solid. Ready to move on to other scheduling algorithms using this same framework.

<hr>

# SJF with I/O Implementation

## Starting SJF Implementation

Now that I got FIFO working with I/O bursts, time to tackle SJF. The good news is most of the framework should be reusable.

#### **The Big Question: What Changes?**

Spent some time thinking about this. The core I/O logic (4-step loop) should stay exactly the same:

1. Check I/O completions
2. Check new arrivals
3. Assign CPU
4. Execute

Only step 3 changes - instead of FIFO ordering, need to pick the shortest job.

#### **Queue Structure Dilemma**

Hit a design choice early. My FIFO uses circular queue with front/rear pointers. Problem: SJF needs to pick from the MIDDLE of the queue (shortest job), not just the front.

Options:

1. Keep circular queue, do complex middle-removal with shifting
2. Switch to simple array

Researched this - turns out simple array is the standard for SJF implementations. Makes sense:

- Ready queues are small anyway (performance doesn't matter)
- Middle removal is much cleaner with simple array
- Textbooks all use simple arrays for SJF

**Decision: Switch to simple array approach**

#### **Updating Queue Structure**

Changed `readyQueue` struct:

```c
// Old FIFO version:
typedef struct readyQueue {
    Process* queue[MAX_PROCESSES];
    int front, rear, size;  // Circular queue
} readyQueue;

// New SJF version:  
typedef struct readyQueue {
    Process* queue[MAX_PROCESSES];
    int size;  // Just size, no front/rear
} readyQueue;
```

Updated functions:

- `init_queue()` - now just sets size to 0
- `enqueue()` - add at index `size`, increment size. Much simpler!
- `dequeue()` - completely replaced with `dequeue_shortest()`

#### **The Tricky Part: dequeue_shortest()**

This is where the real work happens. Need to:

1. Find shortest job in queue
2. Remove it from array
3. Shift everything left to fill gap

First attempt had bugs:

```c
// My buggy version:
int shortest = __INT_MAX__;  // Confusing variable name
if(p->remaining_time < shortest) // Logic was right
// But then I forgot to actually REMOVE the process from array!
```

**Ah, the "shifting" confusion:** Got confused about why I need to shift array elements. Realized: if I don't shift, there's a GAP in the middle of my array. Next time I loop through `q->size`, I'll hit garbage data in that gap.

**Example that clarified it for me:**

```
Before: [P1, P2, P3, P4] (remove P2)
Without shifting: [P1, ?, P3, P4] ← GAP!
With shifting: [P1, P3, P4, ?] ← Clean
```

Fixed `dequeue_shortest()`:

1. Loop through queue to find shortest
2. Save pointer to shortest process
3. Shift all elements left from that position
4. Decrement size
5. Return the shortest process

#### **Small But Important Fixes**

Found a stupid bug in I/O completion handling:

```c
processes[i].state == READY;  // WRONG - this is comparison!
processes[i].state = READY;   // Fixed - assignment
```

One character bug that would have broken everything.

#### **Testing the Logic**

Created test case:

- P1: Multi-burst with I/O (tests full functionality)
- P2: Arrives while P1 running (tests queuing + SJF selection)
- P3: CPU-only process (tests mixed workloads)

**Key test scenarios:**

- Time 6: Both P1(remaining=2) and P3(remaining=2) in queue
    - Should pick P1 (arrived earlier - tie breaker)
- Time 4: P2(remaining=3) vs later arrivals
    - Should pick shortest available

#### **Printing Modifications**

Had to fix the printing function because it was still using FIFO queue logic:

```c
// Broken (from FIFO days):
int idx = (rq.front + i) % MAX_PROCESSES;  // No more front/rear!

// Fixed:
printf("%s", rq.queue[i]->pid);  // Simple array access
```

Also the "CPU assignment prediction" was wrong - was just showing first process in queue, not the shortest one.

#### **Current Status**

SJF is working! The output shows:

- Correct shortest job selection
- Proper I/O handling (all timing calculations right)
- Multi-burst processes work perfectly
- Queue management clean

**What I learned:**

- Framework reusability paid off - 90% of FIFO logic transferred over
- The queue structure change was the right call
- Simple array > circular queue for algorithms that need middle-access
- One-character bugs can break everything (== vs =)

**Next up:** STCF should be even easier - just add preemption to this SJF base. Round Robin will need timer interrupts but same I/O framework.

<hr>

# STCF with I/O Implementation

## Starting Point: Working SJF + I/O

Just finished SJF and it's solid. All the I/O burst handling works perfectly, queue management is clean with the simple array approach. Time to add preemption for STCF.

#### **The Realization: STCF = SJF + "Hey, can you move?"**

Spent a few minutes thinking about what STCF actually does differently. It's literally just SJF that can tap a running process on the shoulder and say "excuse me, someone shorter just showed up."

The I/O framework stays identical. The queue operations stay identical. Just need to add one more check in the main loop.

#### **Where to Add the Preemption Check?**
Looking at my existing loop structure:
```c
//* Check I/O completions (blocked → ready)
//* Check new arrivals
//* Assign CPU if needed
//* Execute current running process
```

The preemption check has to go after steps 1 and 2 (when new processes might become available) but before step 3 (CPU assignment). Makes perfect sense.

#### **New Function Needed: find_shortest_in_queue()**
My `dequeue_shortest()` removes the process from queue. For preemption check, I just need to PEEK at the shortest time without removing anything.

```c
int find_shortest_in_queue(readyQ *q) {
    if (q->size == 0)
        return __INT_MAX__;
        
    int shortest_time = __INT_MAX__;
    for (int i = 0; i < q->size; i++) {
        if (q->queue[i]->remaining_time < shortest_time) {
            shortest_time = q->queue[i]->remaining_time;
        }
    }
    return shortest_time;
}
```

Simple. Just the finding logic from `dequeue_shortest()` without the removal/shifting part.

#### **The Preemption Logic**
Added as step 3 in main loop:
```c
if (running_process != NULL && !is_Empty(&rq)) {
    int shortest_remaining = find_shortest_in_queue(&rq);
    if (shortest_remaining < running_process->remaining_time) {
        // Preempt: move current process back to ready queue
        running_process->state = READY;
        Enqueue(&rq, running_process);
        running_process = NULL;  // Will be reassigned in next step
    }
}
```

**The beauty:** Setting `running_process = NULL` means the existing CPU assignment logic (step 4) automatically picks up the shortest job - whether that's the preempted process or the new arrival that caused the preemption.

#### **Testing Scenarios**
Key test case: Process A running, Process B arrives with shorter burst.

- B gets added to ready queue
- Preemption check: B's time < A's remaining time
- A goes back to ready queue, running_process = NULL
- CPU assignment picks B (shortest)
- Later when B finishes, A resumes

**Another scenario:** Process returns from I/O with shorter time than currently running process.

- I/O completion adds process to ready queue
- Same preemption logic triggers
- Works perfectly

#### **One Small Gotcha**
Initially forgot that preempted processes need their state set back to READY. Without that line, they'd still show as RUNNING in the system state printout even though they're back in the queue.

#### **Current Status**
STCF working perfectly! The preemption happens exactly when it should - when shorter jobs become available through arrivals or I/O completions.

**What I learned:**
- Sometimes the "complex" algorithm is just the simple one + one extra check
- The framework approach paid off again - 95% code reuse from SJF
- Preemption is just "put current process back in line"

**Next up:** Round Robin. That'll be more interesting - time-based preemption instead of length-based.

<hr>

# Round Robin with I/O Implementation

## Starting Point: Working STCF + I/O

Just finished STCF with I/O bursts and it's solid. Time to tackle Round Robin. Looking at my old basic RR implementation - it had the right quantum logic but was designed for simple CPU-only processes. Now need to merge quantum preemption with the I/O burst framework.

#### **The Key Realization: Different Preemption Triggers**

Spent time understanding what changes from STCF to RR:

- STCF: "Preempt if shorter job becomes available"
- RR: "Preempt when time quantum expires"

The I/O framework should stay identical - same 5-step loop, same state transitions. Just need to swap the preemption condition and change queue ordering from shortest-first to FIFO.

#### **Queue Structure Decision: Circular vs Simple Array**

My STCF uses simple array with `dequeue_shortest()`. RR needs FIFO ordering.

**Options considered:**

1. Modify simple array to use `dequeue_front()`
2. Switch back to circular queue with front/rear pointers

Decided on circular queue because:

- True FIFO semantics are cleaner with front/rear pointers
- My old basic RR already used circular queue successfully
- Performance doesn't matter for small ready queues anyway

**Updated queue structure:**

```c
typedef struct ReadyQ {
    Process* queue[MAX_PROCESSES];
    int front, rear, size;  // Circular queue
} ReadyQ;
```

#### **Core Functions: Back to Circular Queue Logic**

Implemented the circular queue trio:

```c
void enq(ReadyQ *rq, Process *p) {
    rq->queue[rq->rear] = p;
    rq->rear = (rq->rear + 1) % MAX_PROCESSES;  // Wrap around
    rq->size++;
}

Process *deq(ReadyQ *rq) {
    Process *p = rq->queue[rq->front];
    rq->front = (rq->front + 1) % MAX_PROCESSES;  // Wrap around  
    rq->size--;
    return p;
}
```

The modulo arithmetic handles wraparound automatically - much cleaner than shifting arrays.

#### **The Critical Condition Ordering Discovery**

Hit a major debugging challenge early. My initial quantum check was:

```c
if (quantum_counter >= QUANTUM) {
    // Preempt
} else if (running_process->remaining_time == 0) {
    // Handle burst completion
}
```

**Problem:** This was preempting processes that were about to finish their CPU burst naturally! Wasteful context switches.

**The insight:** Burst completion should be checked FIRST:

```c
if (running_process->remaining_time == 0) {
    // Natural burst completion - no preemption needed
} else if (quantum_counter >= QUANTUM) {
    // Only preempt if burst isn't finishing anyway
}
```

**Why this matters:** If a process has 1 tick left and quantum is expiring, let it finish naturally instead of preempting it just to put it back immediately.

#### **Quantum Counter Management - The Hidden Complexity**

Initially thought quantum tracking was simple: increment each tick, reset on preemption.

**Wrong.** Quantum counter needs reset in multiple scenarios:

1. Quantum expiration (obvious)
2. Process completion (new process gets fresh quantum)
3. Process goes to I/O (new process gets fresh quantum)
4. New process assigned to CPU (obvious)

**The pattern:** Any time `running_process` becomes NULL, the next assigned process needs a fresh quantum.

**Final quantum logic:**

```c
quantum_counter++;  // Track current process usage

// Reset whenever assigning new process:
if (running_process == NULL && !isempty(&rq)) {
    running_process = deq(&rq);
    quantum_counter = 0;  // Fresh start
}
```

#### **Printing Function Adaptation**

Had to fix multiple issues in the print function:

**Issue 1:** Circular queue display

```c
// Wrong (simple array logic):
printf("%s", rq.queue[i]->pid);

// Right (circular queue logic):
int actual_index = (rq.front + i) % MAX_PROCESSES;
printf("%s", rq.queue[actual_index]->pid);
```

**Issue 2:** CPU assignment prediction

```c
// Wrong (STCF logic - find shortest):
for (int i = 1; i < rq.size; i++) {
    if (rq.queue[i]->remaining_time < rq.queue[shortest_idx]->remaining_time)

// Right (RR logic - take front):
Process *next = rq.queue[rq.front];
```

#### **The Missing Piece: Time Increment**

Spent 20 minutes debugging an infinite loop before realizing: I forgot `current_time++` at the end of the main loop!

Without time advancement:

- I/O completions never trigger
- New arrivals never happen
- Stuck checking time 0 forever

**Simple fix, critical requirement.**

#### **State Transition Timing - The Gotcha**

The order of operations in the main loop is crucial:

```c
//* 1) Check I/O completions (blocked → ready)
//* 2) Check new arrivals  
//* 3) Assign CPU if needed (FIFO)
//* 4) Execute current process (quantum tracking)
//* 5) Advance time
```

**Why this order matters:**

- I/O completions must be handled before CPU assignment (returning processes might get CPU)
- New arrivals must be checked before assignment (new processes join queue)
- Execution happens last (after all system state updates)

#### **Current Status**

RR with I/O bursts is working! The output shows:

- Proper FIFO queue ordering (not shortest-job)
- Quantum preemption working correctly
- Natural burst completion takes precedence over quantum
- All I/O timing calculations correct
- Circular queue display showing processes in correct order

#### **What I Learned**

- **Framework reusability:** 80% of STCF+I/O code transferred directly
- **Condition precedence matters:** Check natural completion before forced preemption
- **Quantum management is stateful:** Reset on every process transition, not just preemption
- **Circular queue benefits:** Cleaner FIFO semantics than array shifting
- **Time advancement is not optional:** Easy to forget, breaks everything

**The debugging insight:** When scheduler behavior looks "mostly right" but has edge cases, it's usually condition ordering or missing state resets, not fundamental algorithmic problems.
