# Microkernel-Based Message Passing IPC System
**Course:** CS2006 - Operating Systems (OS-Sp26)  
**Group:** Keertan (24k-0716) · Rohan Kumar (24k-0778) · Muhammad Ammar Adil (24k-0510)

---

## What This Project Does

Simulates a microkernel-style IPC system where processes communicate
exclusively through message passing. No shared memory is used between
processes — all data flows through message queues, just like in real
microkernel OSes (QNX, MINIX).

---

## OS Concepts Demonstrated

| Concept | Where Used |
|---|---|
| IPC | IPCManager routes messages between processes |
| Threads | Each Process runs as a std::thread |
| Mutexes | Protect MessageQueue and Scheduler from race conditions |
| Condition Variables | blockingReceive() sleeps until a message arrives |
| Scheduling | Scheduler picks highest-priority message next |
| Starvation Prevention | Aging boosts LOW priority messages over time |
| Blocking I/O | Process blocks when its queue is empty |
| Non-blocking I/O | tryReceive() returns immediately if queue empty |
| Synchronization | All shared data protected by locks |

---

## Build Instructions (Linux)

### Prerequisites
```bash
sudo apt update
sudo apt install g++ make libsfml-dev
```

### Build with GUI (SFML window)
```bash
cd ipc_project
make
./ipc_system
```

### Build without GUI (terminal only)
```bash
make no_gui
./ipc_system_nogui
```

### Run stress tests
```bash
make test
# or
./ipc_system_nogui --stress-test
```

### Clean build files
```bash
make clean
```

---

## Project Structure

```
ipc_project/
├── Makefile
├── README.md
├── include/
│   ├── Common.h        — shared enums, constants, config
│   ├── Message.h       — Message struct + comparator
│   ├── MessageQueue.h  — thread-safe priority queue
│   ├── Process.h       — simulated process (runs as thread)
│   ├── IPCManager.h    — central message router
│   ├── Scheduler.h     — priority scheduling + aging
│   ├── Logger.h        — thread-safe logging
│   ├── GUIManager.h    — SFML visualization
│   └── StressTester.h  — automated test cases
├── src/
│   ├── main.cpp
│   ├── Message.cpp
│   ├── MessageQueue.cpp
│   ├── Process.cpp
│   ├── IPCManager.cpp
│   ├── Scheduler.cpp
│   ├── Logger.cpp
│   ├── GUIManager.cpp
│   └── StressTester.cpp
├── tests/
│   └── test_cases.cpp
└── logs/
    └── ipc_simulation.log  (created at runtime)
```

---

## Configuration (include/Common.h)

```cpp
namespace Config {
    MAX_QUEUE_SIZE     = 20;    // max msgs per process inbox
    MSG_LOSS_CHANCE    = 0.05f; // 5% random message drop
    NUM_PROCESSES      = 5;     // number of simulated processes
    SIMULATION_SECONDS = 15;    // how long simulation runs
    AGING_THRESHOLD    = 5;     // skips before priority boost
}
```

---

## Viva Questions & Answers

**Q1: What is IPC and why is it needed?**  
IPC (Inter-Process Communication) allows separate processes to exchange
data without sharing memory. It's needed because processes run in isolated
address spaces — they can't directly access each other's variables.

**Q2: Why message passing instead of shared memory?**  
Message passing avoids race conditions inherently — only one process owns
a message at a time. Shared memory requires careful locking everywhere.
Microkernel OSes prefer message passing for isolation and security.

**Q3: How does your blocking receive work?**  
We use a `std::condition_variable`. The receiving thread calls `wait()`,
which atomically releases the mutex and sleeps. When a message is enqueued,
`notify_one()` wakes the sleeping thread. This avoids busy-waiting.

**Q4: What is priority inversion / starvation?**  
If HIGH priority messages always arrive, LOW priority messages never get
delivered — starvation. We prevent this with aging: after a LOW message
is skipped 5 times, its priority is boosted to MEDIUM, then to HIGH.

**Q5: How do you prevent race conditions on the message queue?**  
Every access to the queue (enqueue, dequeue, size check) is wrapped in
a `std::lock_guard<std::mutex>`. Only one thread can hold the lock at a
time, so no two threads can modify the queue simultaneously.

**Q6: What is the role of the Dispatcher thread?**  
The dispatcher runs in IPCManager. It continuously pulls messages from
the Scheduler (highest priority first) and delivers them to target process
queues. It's like a postal worker routing mail between mailboxes.

**Q7: How do you simulate message loss?**  
In `IPCManager::sendMessage()`, we generate a random float 0.0–1.0. If
it's less than 0.05 (5%), the message is dropped and logged. This simulates
unreliable network channels.

**Q8: What's the difference between blocking and non-blocking receive?**  
Blocking: the thread sleeps until a message arrives (uses condition_variable).
Non-blocking: the thread checks the queue and returns immediately — if empty,
returns false. Blocking is simpler but wastes a thread; non-blocking needs
a polling loop but allows the thread to do other work.

**Q9: How does your scheduler work?**  
Messages are stored in a vector. `getNextMessage()` scans for the highest
priority+aging score and removes it. All others have their agingCounter
incremented (they waited one more round). `applyAging()` runs every ~1s
and boosts any message that has been waiting too long.

**Q10: What OS concept do threads simulate here?**  
Threads simulate OS processes. Real OS processes have isolated address spaces
managed by the kernel, but simulating true process isolation in user space
requires threads. The IPC logic (message queues, blocking, scheduling)
mirrors what a real microkernel kernel would do.

---

## Future Improvements

- Named pipes or sockets for real cross-process (not cross-thread) IPC
- Message acknowledgement (reliable delivery confirmation)
- Broadcast messages (one sender, all processes receive)
- Configurable scheduling algorithms (Round Robin, EDF)
- Persistent log database instead of text file
- Network simulation (processes on different machines)
