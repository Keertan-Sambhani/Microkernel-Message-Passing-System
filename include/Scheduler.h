#ifndef SCHEDULER_H
#define SCHEDULER_H

// ============================================================
// Scheduler.h
// Decides which message gets delivered next.
// Uses priority-based scheduling with an aging mechanism
// to prevent low-priority messages from starving forever.
// ============================================================

#include "Common.h"
#include "Message.h"
#include <vector>

class Scheduler {
public:
    Scheduler();

    // Add a message that needs to be scheduled for delivery
    void addMessage(const Message& msg);

    // Get the next message to deliver (highest priority, or aged-up message)
    // Returns false if nothing is waiting
    bool getNextMessage(Message& out);

    // How many messages are waiting to be scheduled
    int pendingCount() const;

    // Apply aging: messages that have been waiting too long
    // get their priority boosted to prevent starvation
    void applyAging();

    // How many times aging has happened (for logging/display)
    int getAgingCount() const { return agingEventCount; }

private:
    // All pending messages waiting for delivery
    // We use a vector so we can iterate and apply aging
    std::vector<Message> pendingMessages;

    mutable std::mutex schedulerMutex;

    int agingEventCount;    // total aging boosts applied so far

    // Find the index of the highest-priority message in pendingMessages
    int findHighestPriorityIndex() const;
};

#endif // SCHEDULER_H
