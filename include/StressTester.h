#ifndef STRESSTESTER_H
#define STRESSTESTER_H

// ============================================================
// StressTester.h
// Runs automated test cases to verify the IPC system works
// correctly under heavy load and edge cases.
// ============================================================

#include "Common.h"
#include "IPCManager.h"
#include "Process.h"

class StressTester {
public:
    explicit StressTester(IPCManager* ipc);

    // Run all test cases and print results
    void runAll();

private:
    IPCManager* ipcManager;

    // Individual test cases
    void testHeavyTraffic();      // flood the system with messages
    void testQueueOverflow();     // try to exceed MAX_QUEUE_SIZE
    void testConcurrentSenders(); // many threads send at once
    void testPriorityOrdering();  // verify HIGH arrives before LOW
    void testMessageLoss();       // verify ~5% drop rate
    void testBlockingReceive();   // verify blocking works

    // Helper: print pass/fail
    void report(const std::string& testName, bool passed);
    int passCount;
    int failCount;
};

#endif // STRESSTESTER_H
