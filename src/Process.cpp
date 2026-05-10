// ============================================================
// Process.cpp
// Simulates an OS process as a C++ thread.
// Each process runs a loop: send a message, then receive one,
// sleep for a random interval, repeat.
// ============================================================

#include "Process.h"
#include "IPCManager.h"

Process::Process(int id, Priority priority, IPCManager* ipc)
    : processID(id),
      processPriority(priority),
      state(ProcessState::IDLE),
      ipcManager(ipc),
      messageQueue(id),
      running(false),
      sentCount(0),
      receivedCount(0)
{}

Process::~Process() {
    stop();
    join();
}

void Process::start() {
    running = true;
    processThread = std::thread(&Process::run, this);
}

void Process::stop() {
    running = false;
}

void Process::join() {
    if (processThread.joinable()) {
        processThread.join();
    }
}

// ------------------------------------------------------------------
// run: the main thread function.
// Each process alternates between sending and receiving,
// with random sleeps to simulate realistic timing.
// ------------------------------------------------------------------
void Process::run() {
    // Random number setup — each thread gets its own generator
    // seeded differently so they don't all do the same thing
    std::mt19937 localRng(std::chrono::steady_clock::now()
                          .time_since_epoch().count() + processID * 1000);
    std::uniform_int_distribution<int> sleepDist(200, 800); // ms between actions

    int totalProcesses = ipcManager->getProcessCount();

    while (running) {
        setState(ProcessState::RUNNING);

        // --- SEND PHASE ---
        // Pick a random target (not ourselves)
        int targetID = pickRandomTarget(totalProcesses);
        Priority msgPrio = pickRandomPriority();

        // Build and send the message
        Message msg(
            processID,
            targetID,
            "Hello from P" + std::to_string(processID),
            msgPrio,
            ipcManager->generateMessageID()
        );

        ipcManager->sendMessage(msg);
        sentCount++;

        // Short sleep between send and receive
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepDist(localRng)));

        // --- RECEIVE PHASE ---
        // Try non-blocking first; if nothing there, do a short blocking wait
        setState(ProcessState::RUNNING);
        Message received;

        if (!messageQueue.nonBlockingReceive(received)) {
            // Nothing in queue — block briefly
            setState(ProcessState::BLOCKED);
            LOG(LogEvent::PROCESS_BLOCKED,
                getName() + " is blocking on receive...");

            // We use a timed approach: don't block forever,
            // just wait up to 500ms then continue
            // (real blocking receive is in MessageQueue::blockingReceive)
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            messageQueue.nonBlockingReceive(received); // try again after wait

            setState(ProcessState::RUNNING);
            LOG(LogEvent::PROCESS_UNBLOCKED,
                getName() + " unblocked.");
        }

        if (received.messageID != 0) { // valid message received
            receivedCount++;
            // In a real OS, the process would process the message here.
            // We just log it.
            LOGINFO(getName() + " processed: " + received.toString());
        }

        // Sleep before next iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepDist(localRng)));
    }

    setState(ProcessState::IDLE);
}

// ------------------------------------------------------------------
// deliverMessage: called by IPCManager to put a message into
// this process's inbox queue. Returns false if queue is full.
// ------------------------------------------------------------------
bool Process::deliverMessage(const Message& msg) {
    return messageQueue.enqueue(msg);
}

// ------------------------------------------------------------------
// pickRandomTarget: choose a random process ID that isn't us.
// Process IDs are 1..totalProcesses.
// ------------------------------------------------------------------
int Process::pickRandomTarget(int totalProcesses) const {
    static thread_local std::mt19937 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(1, totalProcesses);

    int target;
    do {
        target = dist(rng);
    } while (target == processID); // don't send to ourselves

    return target;
}

// Pick a random priority — weighted towards MEDIUM for realism
Priority Process::pickRandomPriority() const {
    static thread_local std::mt19937 rng(
        std::chrono::steady_clock::now().time_since_epoch().count() + 99);
    std::uniform_int_distribution<int> dist(1, 10);
    int roll = dist(rng);

    if (roll <= 2) return Priority::HIGH;   // 20% chance
    if (roll <= 6) return Priority::MEDIUM; // 40% chance
    return Priority::LOW;                   // 40% chance
}

void Process::setState(ProcessState newState) {
    std::lock_guard<std::mutex> lock(stateMutex);
    state = newState;
}
