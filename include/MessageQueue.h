#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

// ============================================================
// MessageQueue.h
// Each process has one of these. It's a thread-safe priority
// queue that supports both blocking and non-blocking receive.
// ============================================================

#include "Common.h"
#include "Message.h"

class MessageQueue {
public:
    // ownerID: which process owns this queue
    explicit MessageQueue(int ownerID);

    // Add a message to the queue.
    // Returns false if queue is full (overflow).
    bool enqueue(const Message& msg);

    // Blocking receive: calling thread waits until a message arrives.
    // Returns the highest-priority message.
    Message blockingReceive();

    // Non-blocking receive: returns immediately.
    // If queue is empty, returns false and doesn't touch 'out'.
    bool nonBlockingReceive(Message& out);

    // How many messages are currently waiting
    int size() const;

    bool isEmpty() const;
    bool isFull()  const;

    int getOwnerID() const { return ownerID; }

    // For the GUI: get a snapshot of current messages (copy)
    // We return a vector so GUI can iterate safely
    std::vector<Message> getSnapshot() const;

private:
    int ownerID;

    // The actual queue — ordered by priority (highest first)
    std::priority_queue<Message, std::vector<Message>, MessageComparator> pq;

    // Mutex protects the queue from concurrent access
    mutable std::mutex queueMutex;

    // Condition variable lets blockingReceive() sleep until
    // a message arrives instead of burning CPU in a loop
    std::condition_variable messageAvailable;

    // Used to signal threads to stop waiting (during shutdown)
    std::atomic<bool> shutdownFlag;
};

#endif // MESSAGEQUEUE_H
