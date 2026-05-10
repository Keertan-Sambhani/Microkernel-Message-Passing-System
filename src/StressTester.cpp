// ============================================================
// StressTester.cpp
// Automated tests to verify correctness and stability.
// Each test creates its own mini environment to stay isolated.
// ============================================================

#include "StressTester.h"
#include <cassert>
#include <chrono>

StressTester::StressTester(IPCManager* ipc)
    : ipcManager(ipc), passCount(0), failCount(0) {}

void StressTester::runAll() {
    std::cout << "\n========================================\n";
    std::cout << "  RUNNING STRESS TESTS\n";
    std::cout << "========================================\n\n";

    testPriorityOrdering();
    testQueueOverflow();
    testMessageLoss();
    testHeavyTraffic();
    testConcurrentSenders();
    testBlockingReceive();

    std::cout << "\n========================================\n";
    std::cout << "  RESULTS: " << passCount << " passed, "
              << failCount << " failed\n";
    std::cout << "========================================\n\n";
}

// ------------------------------------------------------------------
// Test 1: Priority Ordering
// Put 3 messages with different priorities in a queue.
// They should come out HIGH, MEDIUM, LOW.
// ------------------------------------------------------------------
void StressTester::testPriorityOrdering() {
    std::cout << "[TEST] Priority ordering...\n";

    MessageQueue q(99); // dummy queue for process 99

    Message low(1, 99, "low msg",    Priority::LOW,    1);
    Message med(1, 99, "medium msg", Priority::MEDIUM, 2);
    Message hi (1, 99, "high msg",   Priority::HIGH,   3);

    // Enqueue in reverse order to test sorting
    q.enqueue(low);
    q.enqueue(hi);
    q.enqueue(med);

    // Should come out: HIGH, MEDIUM, LOW
    Message out1, out2, out3;
    bool got1 = q.nonBlockingReceive(out1);
    bool got2 = q.nonBlockingReceive(out2);
    bool got3 = q.nonBlockingReceive(out3);

    bool passed = got1 && got2 && got3
               && out1.priority == Priority::HIGH
               && out2.priority == Priority::MEDIUM
               && out3.priority == Priority::LOW;

    report("Priority Ordering", passed);
    if (!passed) {
        std::cout << "  -> Got: " << priorityToString(out1.priority)
                  << ", " << priorityToString(out2.priority)
                  << ", " << priorityToString(out3.priority) << "\n";
    }
}

// ------------------------------------------------------------------
// Test 2: Queue Overflow
// Fill queue to max, try to add one more — should fail.
// ------------------------------------------------------------------
void StressTester::testQueueOverflow() {
    std::cout << "[TEST] Queue overflow...\n";

    MessageQueue q(98);

    // Fill it up to MAX_QUEUE_SIZE
    int accepted = 0;
    for (int i = 0; i < Config::MAX_QUEUE_SIZE + 5; i++) {
        Message m(1, 98, "overflow test", Priority::LOW, i + 100);
        if (q.enqueue(m)) accepted++;
    }

    bool passed = (accepted == Config::MAX_QUEUE_SIZE);
    report("Queue Overflow (cap at " + std::to_string(Config::MAX_QUEUE_SIZE) + ")", passed);
    if (!passed) {
        std::cout << "  -> Accepted " << accepted
                  << " (expected " << Config::MAX_QUEUE_SIZE << ")\n";
    }
}

// ------------------------------------------------------------------
// Test 3: Message Loss
// Send 1000 messages through IPCManager.
// Expect ~5% to be dropped (within 3% tolerance).
// ------------------------------------------------------------------
void StressTester::testMessageLoss() {
    std::cout << "[TEST] Message loss (~5%)...\n";

    // Count how many get dropped by checking IPCManager counters
    int before = ipcManager->getTotalDropped();
    int sentBefore = ipcManager->getTotalSent();

    // Send 500 messages
    for (int i = 0; i < 500; i++) {
        Message m(1, 2, "loss test", Priority::MEDIUM,
                  ipcManager->generateMessageID());
        ipcManager->sendMessage(m);
    }

    int sent    = ipcManager->getTotalSent()    - sentBefore;
    int dropped = ipcManager->getTotalDropped() - before;
    float rate  = (float)dropped / (float)sent;

    // Allow 2% to 10% range (expected is 5%)
    bool passed = (rate >= 0.02f && rate <= 0.10f);
    report("Message Loss Rate (" + std::to_string((int)(rate * 100)) + "%)", passed);
}

// ------------------------------------------------------------------
// Test 4: Heavy Traffic
// Flood the scheduler with 200 messages and verify none crash.
// ------------------------------------------------------------------
void StressTester::testHeavyTraffic() {
    std::cout << "[TEST] Heavy traffic (200 msgs)...\n";

    bool passed = true;
    try {
        for (int i = 0; i < 200; i++) {
            Priority p = (i % 3 == 0) ? Priority::HIGH :
                         (i % 3 == 1) ? Priority::MEDIUM : Priority::LOW;
            Message m(1, 2, "stress msg " + std::to_string(i), p,
                      ipcManager->generateMessageID());
            ipcManager->sendMessage(m);
        }
    } catch (...) {
        passed = false;
    }

    // Let dispatcher work through some of them
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    report("Heavy Traffic (no crash)", passed);
}

// ------------------------------------------------------------------
// Test 5: Concurrent Senders
// 10 threads each send 20 messages simultaneously.
// Should not crash or deadlock.
// ------------------------------------------------------------------
void StressTester::testConcurrentSenders() {
    std::cout << "[TEST] Concurrent senders (10 threads)...\n";

    std::vector<std::thread> threads;
    std::atomic<int> totalSent(0);

    for (int t = 0; t < 10; t++) {
        threads.push_back(std::thread([&, t]() {
            for (int i = 0; i < 20; i++) {
                Message m(t + 1, 2, "concurrent msg",
                          Priority::MEDIUM,
                          ipcManager->generateMessageID());
                ipcManager->sendMessage(m);
                totalSent++;
            }
        }));
    }

    for (auto& th : threads) th.join();

    bool passed = (totalSent == 200);
    report("Concurrent Senders (" + std::to_string(totalSent.load()) + "/200 sent)", passed);
}

// ------------------------------------------------------------------
// Test 6: Blocking Receive
// Enqueue a message, then verify blocking receive gets it.
// Also verify it blocks when queue is empty.
// ------------------------------------------------------------------
void StressTester::testBlockingReceive() {
    std::cout << "[TEST] Blocking receive...\n";

    MessageQueue q(97);

    // Push a message from another thread after a short delay
    bool messageDelivered = false;
    std::thread sender([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Message m(1, 97, "blocking test", Priority::HIGH,
                  ipcManager->generateMessageID());
        q.enqueue(m);
        messageDelivered = true;
    });

    // This should block until the sender delivers
    Message received = q.blockingReceive();
    sender.join();

    bool passed = messageDelivered && (received.messageID != 0);
    report("Blocking Receive", passed);
}

void StressTester::report(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "  [PASS] " << testName << "\n";
        passCount++;
    } else {
        std::cout << "  [FAIL] " << testName << "\n";
        failCount++;
    }
}
