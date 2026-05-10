#ifndef IPCMANAGER_H
#define IPCMANAGER_H

// ============================================================
// IPCManager.h
// The central hub of the system — like a post office.
// Processes register here and use it to send messages.
// It routes messages through the Scheduler and handles
// message loss simulation.
// ============================================================

#include "Common.h"
#include "Message.h"
#include "Scheduler.h"
#include "Logger.h"
#include <unordered_map>

// Forward declaration
class Process;

class IPCManager {
public:
    IPCManager();
    ~IPCManager();

    // Register a process so it can send/receive messages
    void registerProcess(Process* proc);

    // Called by a Process to send a message to another process.
    // This goes through the Scheduler.
    // Returns false if the message was dropped (simulated loss)
    bool sendMessage(const Message& msg);

    // The scheduler dispatch loop — runs in its own thread.
    // Continuously pulls messages from Scheduler and delivers them.
    void startDispatcher();
    void stopDispatcher();

    // How many processes are registered
    int getProcessCount() const;

    // Get a process by ID (used by Process to find targets)
    Process* getProcess(int id);

    // Total messages handled so far
    int getTotalSent()     const { return totalSent;     }
    int getTotalDropped()  const { return totalDropped;  }
    int getTotalDelivered()const { return totalDelivered;}

    // Unique ID counter for messages
    int generateMessageID();

private:
    // Registry of all processes: processID -> Process*
    std::unordered_map<int, Process*> processRegistry;
    mutable std::mutex registryMutex;

    // The scheduler that orders messages by priority
    Scheduler scheduler;

    // Dispatcher thread
    std::thread dispatcherThread;
    std::atomic<bool> dispatcherRunning;

    // Message loss: returns true if this message should be dropped
    bool shouldDropMessage() const;

    // Random number generator for message loss simulation
    mutable std::mt19937 rng;

    // Stats
    std::atomic<int> totalSent;
    std::atomic<int> totalDropped;
    std::atomic<int> totalDelivered;
    std::atomic<int> nextMessageID;

    // The actual dispatch loop function
    void dispatchLoop();
};

#endif // IPCMANAGER_H
