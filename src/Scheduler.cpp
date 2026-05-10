// ============================================================
// Scheduler.cpp
// Priority-based message scheduler with aging.
//
// How it works:
//   - Messages are stored in a vector (not priority_queue)
//     so we can iterate and apply aging to each one.
//   - getNextMessage() scans for highest priority, picks it.
//   - applyAging() runs periodically; if a LOW/MEDIUM message
//     has been skipped too many times, we boost its priority.
//     This prevents starvation.
// ============================================================

#include "Scheduler.h"
#include "Logger.h"

Scheduler::Scheduler() : agingEventCount(0) {}

// Add a message into the pending list
void Scheduler::addMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    pendingMessages.push_back(msg);
}

// ------------------------------------------------------------------
// getNextMessage: pick the highest-priority pending message.
// Removes it from the list and returns it.
// ------------------------------------------------------------------
bool Scheduler::getNextMessage(Message& out) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (pendingMessages.empty()) return false;

    // Find the message with the highest priority
    // (and if tie, the one with highest agingCounter — waited longest)
    int bestIdx = findHighestPriorityIndex();

    out = pendingMessages[bestIdx];

    // Remove it from the pending list
    pendingMessages.erase(pendingMessages.begin() + bestIdx);

    // Increment agingCounter for all remaining messages
    // (they just got skipped one more time)
    for (auto& m : pendingMessages) {
        m.agingCounter++;
    }

    return true;
}

// ------------------------------------------------------------------
// findHighestPriorityIndex: scan the list and find best candidate.
// Ties broken by agingCounter (higher = has waited longer = wins).
// ------------------------------------------------------------------
int Scheduler::findHighestPriorityIndex() const {
    int bestIdx = 0;
    for (int i = 1; i < (int)pendingMessages.size(); i++) {
        const Message& best = pendingMessages[bestIdx];
        const Message& curr = pendingMessages[i];

        int bestScore = static_cast<int>(best.priority) * 10 + best.agingCounter;
        int currScore = static_cast<int>(curr.priority) * 10 + curr.agingCounter;

        if (currScore > bestScore) {
            bestIdx = i;
        }
    }
    return bestIdx;
}

// ------------------------------------------------------------------
// applyAging: called periodically (e.g. every 1 second).
// Any message that has been skipped >= AGING_THRESHOLD times
// gets its priority bumped up by one level.
// This ensures no message waits forever (starvation prevention).
// ------------------------------------------------------------------
void Scheduler::applyAging() {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    for (auto& msg : pendingMessages) {
        if (msg.agingCounter >= Config::AGING_THRESHOLD) {
            // Only boost if not already at HIGH
            if (msg.priority == Priority::LOW) {
                msg.priority = Priority::MEDIUM;
                agingEventCount++;
                LOG(LogEvent::SCHEDULER_EVENT,
                    "Aging: MSG#" + std::to_string(msg.messageID) +
                    " boosted LOW -> MEDIUM (waited " +
                    std::to_string(msg.agingCounter) + " turns)");
                msg.agingCounter = 0; // reset counter after boost
            } else if (msg.priority == Priority::MEDIUM) {
                msg.priority = Priority::HIGH;
                agingEventCount++;
                LOG(LogEvent::SCHEDULER_EVENT,
                    "Aging: MSG#" + std::to_string(msg.messageID) +
                    " boosted MEDIUM -> HIGH (waited " +
                    std::to_string(msg.agingCounter) + " turns)");
                msg.agingCounter = 0;
            }
        }
    }
}

int Scheduler::pendingCount() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return (int)pendingMessages.size();
}
