// ============================================================
// MessageQueue.cpp
// Thread-safe priority queue for inter-process messages.
// Uses mutex + condition_variable for blocking receive.
// ============================================================

#include "MessageQueue.h"
#include "Logger.h"

MessageQueue::MessageQueue(int ownerID)
    : ownerID(ownerID), shutdownFlag(false) {}

// ------------------------------------------------------------------
// enqueue: add a message to this process's queue.
// Called by IPCManager when delivering a message.
// Returns false if queue is full.
// ------------------------------------------------------------------
bool MessageQueue::enqueue(const Message& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);

    // Check for overflow — don't let queue grow unbounded
    if ((int)pq.size() >= Config::MAX_QUEUE_SIZE) {
        LOG(LogEvent::QUEUE_FULL,
            "Queue full for P" + std::to_string(ownerID) +
            " — dropped MSG#" + std::to_string(msg.messageID));
        return false;
    }

    pq.push(msg);

    // Wake up any thread that's blocked in blockingReceive()
    // now that we have a message available
    messageAvailable.notify_one();
    return true;
}

// ------------------------------------------------------------------
// blockingReceive: the calling thread sleeps here until a message
// arrives. This simulates a process blocking on a receive() syscall.
// ------------------------------------------------------------------
Message MessageQueue::blockingReceive() {
    std::unique_lock<std::mutex> lock(queueMutex);

    // Wait until either a message arrives OR we're shutting down
    messageAvailable.wait(lock, [this] {
        return !pq.empty() || shutdownFlag.load();
    });

    // If woken up due to shutdown and queue is empty, return empty msg
    if (pq.empty()) {
        return Message(); // empty sentinel
    }

    // Get highest-priority message (priority_queue puts it at top)
    Message msg = pq.top();
    pq.pop();
    return msg;
}

// ------------------------------------------------------------------
// nonBlockingReceive: try to get a message, return immediately.
// Returns true and fills 'out' if a message was available,
// returns false if queue was empty.
// ------------------------------------------------------------------
bool MessageQueue::nonBlockingReceive(Message& out) {
    std::lock_guard<std::mutex> lock(queueMutex);

    if (pq.empty()) {
        return false; // nothing to receive right now
    }

    out = pq.top();
    pq.pop();
    return true;
}

int MessageQueue::size() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return (int)pq.size();
}

bool MessageQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return pq.empty();
}

bool MessageQueue::isFull() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return (int)pq.size() >= Config::MAX_QUEUE_SIZE;
}

// ------------------------------------------------------------------
// getSnapshot: returns a copy of all messages for GUI display.
// We can't iterate a priority_queue directly, so we dump it
// into a vector. This is a read-only copy — won't affect queue.
// ------------------------------------------------------------------
std::vector<Message> MessageQueue::getSnapshot() const {
    std::lock_guard<std::mutex> lock(queueMutex);

    // Copy the internal container by making a temp copy of the queue
    auto tempPQ = pq; // copy
    std::vector<Message> result;
    while (!tempPQ.empty()) {
        result.push_back(tempPQ.top());
        tempPQ.pop();
    }
    return result;
}
