#ifndef PROCESS_H
#define PROCESS_H

// ============================================================
// Process.h
// Simulates an OS process using a std::thread.
// Each process has its own message queue, priority, and state.
// Processes randomly send messages to each other.
// ============================================================

#include "Common.h"
#include "Message.h"
#include "MessageQueue.h"

// Forward declaration — Process needs to call IPCManager::sendMessage
// but IPCManager also holds Process objects, so we avoid circular include
class IPCManager;

class Process {
public:
    Process(int id, Priority priority, IPCManager* ipc);
    ~Process();

    // Start the process thread
    void start();

    // Tell the process to stop cleanly
    void stop();

    // Wait for the thread to finish
    void join();

    // Getters used by GUI and IPCManager
    int           getID()       const { return processID; }
    Priority      getPriority() const { return processPriority; }
    ProcessState  getState()    const { return state; }
    MessageQueue& getQueue()          { return messageQueue; }
    int           getSentCount()     const { return sentCount; }
    int           getReceivedCount() const { return receivedCount; }
    std::string   getName()     const { return "P" + std::to_string(processID); }

    // Called by IPCManager to deliver a message to this process
    bool deliverMessage(const Message& msg);

private:
    int          processID;
    Priority     processPriority;
    ProcessState state;
    IPCManager*  ipcManager;    // pointer to the IPC system (to send messages)
    MessageQueue messageQueue;  // this process's inbox

    std::thread processThread;
    std::atomic<bool> running;

    // Counters for statistics
    std::atomic<int> sentCount;
    std::atomic<int> receivedCount;

    // The main function that runs in the thread
    // Alternates between sending and receiving messages
    void run();

    // Helper: pick a random target process to send to
    int pickRandomTarget(int totalProcesses) const;

    // Helper: pick a random message priority
    Priority pickRandomPriority() const;

    // Used to change state safely
    void setState(ProcessState newState);
    mutable std::mutex stateMutex;
};

#endif // PROCESS_H
