#ifndef MESSAGE_H
#define MESSAGE_H

// ============================================================
// Message.h
// Defines the Message struct — the basic unit of communication
// between processes in our IPC system.
// ============================================================

#include "Common.h"

// A message is what one process sends to another.
// Think of it like an envelope with a sender, receiver, and content.
struct Message {
    int         senderID;       // which process sent this
    int         receiverID;     // which process should receive it
    std::string text;           // actual message content
    Priority    priority;       // LOW, MEDIUM, or HIGH
    std::string timestamp;      // when it was created
    int         messageID;      // unique ID for tracking
    int         agingCounter;   // how many times this was skipped (for fairness)

    // Constructor — fills in all fields when creating a message
    Message(int from, int to, const std::string& msg,
            Priority prio = Priority::MEDIUM, int id = 0)
        : senderID(from),
          receiverID(to),
          text(msg),
          priority(prio),
          timestamp(getCurrentTimestamp()),
          messageID(id),
          agingCounter(0)
    {}

    // Default constructor needed for some STL operations
    Message() : senderID(0), receiverID(0), priority(Priority::LOW),
                messageID(0), agingCounter(0) {}

    // Print a short summary of this message (used in logs)
    std::string toString() const {
        return "[MSG#" + std::to_string(messageID) +
               " | P" + std::to_string(senderID) +
               "->P" + std::to_string(receiverID) +
               " | " + priorityToString(priority) +
               " | " + timestamp +
               " | \"" + text + "\"]";
    }
};

// Custom comparator so we can use Message in a priority_queue.
// Higher priority value = comes first. If same priority, lower
// agingCounter comes first (newer messages if not aged yet).
struct MessageComparator {
    bool operator()(const Message& a, const Message& b) const {
        // If same priority level, prefer the one that has been waiting longer
        if (static_cast<int>(a.priority) == static_cast<int>(b.priority)) {
            return a.agingCounter < b.agingCounter; // higher aging = more urgent
        }
        // Otherwise, higher priority number wins
        return static_cast<int>(a.priority) < static_cast<int>(b.priority);
    }
};

#endif // MESSAGE_H
