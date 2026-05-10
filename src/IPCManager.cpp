// ============================================================
// IPCManager.cpp
// The central message router. Processes register here,
// send messages through here, and the dispatcher thread
// pulls messages from the Scheduler and delivers them.
// ============================================================

#include "IPCManager.h"
#include "Process.h"

IPCManager::IPCManager()
    : dispatcherRunning(false),
      totalSent(0), totalDropped(0), totalDelivered(0), nextMessageID(1)
{
    // Seed the random number generator with current time
    rng.seed(static_cast<unsigned>(std::chrono::steady_clock::now()
                                    .time_since_epoch().count()));
}

IPCManager::~IPCManager() {
    stopDispatcher();
}

// ------------------------------------------------------------------
// registerProcess: add a process to the registry so we can
// look it up by ID when routing messages.
// ------------------------------------------------------------------
void IPCManager::registerProcess(Process* proc) {
    std::lock_guard<std::mutex> lock(registryMutex);
    processRegistry[proc->getID()] = proc;
    LOGINFO("Registered " + proc->getName() +
            " (priority: " + priorityToString(proc->getPriority()) + ")");
}

// ------------------------------------------------------------------
// sendMessage: called by a Process to send a message.
// Steps:
//   1. Simulate random message loss (~5%)
//   2. If not dropped, hand it to the Scheduler
//   3. Scheduler will deliver it later via dispatchLoop()
// ------------------------------------------------------------------
bool IPCManager::sendMessage(const Message& msg) {
    totalSent++;

    // Simulate network/channel unreliability
    if (shouldDropMessage()) {
        totalDropped++;
        LOG(LogEvent::MSG_DROPPED,
            "DROPPED " + msg.toString());
        return false;
    }

    // Pass to scheduler for priority-ordered delivery
    scheduler.addMessage(msg);

    LOG(LogEvent::MSG_SENT,
        "QUEUED  " + msg.toString());

    return true;
}

// ------------------------------------------------------------------
// startDispatcher: launches the background thread that continuously
// pulls messages from the Scheduler and delivers them to processes.
// ------------------------------------------------------------------
void IPCManager::startDispatcher() {
    dispatcherRunning = true;
    dispatcherThread = std::thread(&IPCManager::dispatchLoop, this);
    LOGINFO("Dispatcher thread started.");
}

void IPCManager::stopDispatcher() {
    dispatcherRunning = false;
    if (dispatcherThread.joinable()) {
        dispatcherThread.join();
    }
}

// ------------------------------------------------------------------
// dispatchLoop: runs forever (until stopped).
// Every 50ms it:
//   - Applies aging to prevent starvation
//   - Picks the next message from the Scheduler
//   - Finds the target process
//   - Delivers the message to that process's queue
// ------------------------------------------------------------------
void IPCManager::dispatchLoop() {
    int agingTimer = 0; // count iterations for aging

    while (dispatcherRunning) {
        // Apply aging roughly every 20 iterations (~1 second)
        agingTimer++;
        if (agingTimer >= 20) {
            scheduler.applyAging();
            agingTimer = 0;
        }

        Message msg;
        if (scheduler.getNextMessage(msg)) {
            // Find the destination process
            Process* target = getProcess(msg.receiverID);

            if (target != nullptr) {
                // Deliver to the process's message queue
                bool delivered = target->deliverMessage(msg);

                if (delivered) {
                    totalDelivered++;
                    LOG(LogEvent::MSG_RECEIVED,
                        "DELIVRD " + msg.toString() +
                        " -> " + target->getName());
                } else {
                    // Target's queue was full
                    LOG(LogEvent::QUEUE_FULL,
                        "FULL    Could not deliver MSG#" +
                        std::to_string(msg.messageID) +
                        " to " + target->getName());
                }
            } else {
                LOGINFO("Unknown target P" + std::to_string(msg.receiverID));
            }
        }

        // Small sleep so we don't burn 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ------------------------------------------------------------------
// shouldDropMessage: returns true ~MSG_LOSS_CHANCE % of the time.
// Simulates unreliable communication channels.
// ------------------------------------------------------------------
bool IPCManager::shouldDropMessage() const {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < Config::MSG_LOSS_CHANCE;
}

Process* IPCManager::getProcess(int id) {
    std::lock_guard<std::mutex> lock(registryMutex);
    auto it = processRegistry.find(id);
    if (it != processRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

int IPCManager::getProcessCount() const {
    std::lock_guard<std::mutex> lock(registryMutex);
    return (int)processRegistry.size();
}

int IPCManager::generateMessageID() {
    return nextMessageID++;
}
