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

## Future Improvements

- Named pipes or sockets for real cross-process (not cross-thread) IPC
- Message acknowledgement (reliable delivery confirmation)
- Broadcast messages (one sender, all processes receive)
- Configurable scheduling algorithms (Round Robin, EDF)
- Persistent log database instead of text file
- Network simulation (processes on different machines)
