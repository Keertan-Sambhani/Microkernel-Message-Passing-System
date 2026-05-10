// ============================================================
// main.cpp
// Entry point for the IPC simulation.
//
// What happens here:
//   1. Create the IPCManager (the post office)
//   2. Create N processes and register them
//   3. Start the dispatcher thread
//   4. Start all process threads
//   5a. If GUI build: open the SFML window
//   5b. If no-GUI build or --stress-test flag: run in terminal
//   6. After simulation time, stop everything and print stats
// ============================================================

#include "Common.h"
#include "IPCManager.h"
#include "Process.h"
#include "Logger.h"
#include "StressTester.h"

#ifndef NO_GUI
#include "GUIManager.h"
#endif

#include <csignal>
#include <atomic>

// Global flag so Ctrl+C cleanly stops the simulation
std::atomic<bool> globalStop(false);
void handleSignal(int) { globalStop = true; }

int main(int argc, char* argv[]) {
    // Register Ctrl+C handler
    std::signal(SIGINT, handleSignal);

    // Check for --stress-test flag
    bool stressTestMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--stress-test") {
            stressTestMode = true;
        }
    }

    // -------------------------------------------------------
    // Setup Logger
    // -------------------------------------------------------
    Logger::getInstance().openLogFile("logs/ipc_simulation.log");
    LOGINFO("=== IPC Simulation Starting ===");
    LOGINFO("Processes: " + std::to_string(Config::NUM_PROCESSES));
    LOGINFO("Queue size: " + std::to_string(Config::MAX_QUEUE_SIZE));
    LOGINFO("Msg loss:   " + std::to_string((int)(Config::MSG_LOSS_CHANCE * 100)) + "%");

    // -------------------------------------------------------
    // Create IPCManager
    // -------------------------------------------------------
    IPCManager ipcManager;

    // -------------------------------------------------------
    // Create processes with different priorities
    // We assign priorities round-robin: HIGH, MEDIUM, LOW, ...
    // -------------------------------------------------------
    std::vector<Process*> processes;

    Priority priorities[] = { Priority::HIGH, Priority::MEDIUM, Priority::LOW,
                               Priority::MEDIUM, Priority::HIGH };

    for (int i = 0; i < Config::NUM_PROCESSES; i++) {
        Priority p = priorities[i % 3];
        Process* proc = new Process(i + 1, p, &ipcManager);
        processes.push_back(proc);
        ipcManager.registerProcess(proc);
    }

    // -------------------------------------------------------
    // Start dispatcher and all process threads
    // -------------------------------------------------------
    ipcManager.startDispatcher();

    for (auto* proc : processes) {
        proc->start();
    }

    LOGINFO("All processes started. Simulation running...");
    std::cout << "\n  Simulation running for "
              << Config::SIMULATION_SECONDS << " seconds...\n";
    std::cout << "  Press Ctrl+C to stop early.\n\n";

    // -------------------------------------------------------
    // Run stress tests if requested
    // -------------------------------------------------------
    if (stressTestMode) {
        // Wait a moment for processes to warm up
        std::this_thread::sleep_for(std::chrono::seconds(2));

        StressTester tester(&ipcManager);
        tester.runAll();

        // Let simulation continue briefly after tests
        std::this_thread::sleep_for(std::chrono::seconds(3));
        globalStop = true;
    }

#ifndef NO_GUI
    // -------------------------------------------------------
    // GUI Mode: open SFML window (blocks until window closed)
    // -------------------------------------------------------
    else {
        GUIManager gui(processes, &ipcManager);

        // Run GUI in main thread (SFML requires this on some platforms)
        // Meanwhile, a background timer stops after SIMULATION_SECONDS
        std::thread timer([&]() {
            for (int i = 0; i < Config::SIMULATION_SECONDS * 10 && !globalStop; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            globalStop = true;
        });

        gui.run(); // blocks here until window is closed
        globalStop = true;
        timer.join();
    }
#else
    // -------------------------------------------------------
    // No-GUI Mode: just run for SIMULATION_SECONDS in terminal
    // -------------------------------------------------------
    if (!stressTestMode) {
        for (int i = 0; i < Config::SIMULATION_SECONDS && !globalStop; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "  [" << (i + 1) << "s] Pending in scheduler: "
                      << "Sent=" << ipcManager.getTotalSent()
                      << " Dropped=" << ipcManager.getTotalDropped()
                      << " Delivered=" << ipcManager.getTotalDelivered()
                      << "\n";
        }
    }
#endif

    // -------------------------------------------------------
    // Shutdown: stop all threads cleanly
    // -------------------------------------------------------
    LOGINFO("=== Simulation Ending ===");

    for (auto* proc : processes) {
        proc->stop();
    }

    ipcManager.stopDispatcher();

    // Give threads a moment to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    for (auto* proc : processes) {
        proc->join();
    }

    // -------------------------------------------------------
    // Print final statistics
    // -------------------------------------------------------
    Logger::getInstance().printStats();

    std::cout << "  Per-process stats:\n";
    for (auto* proc : processes) {
        std::cout << "    " << proc->getName()
                  << " | Sent: "     << proc->getSentCount()
                  << " | Received: " << proc->getReceivedCount()
                  << " | Queue: "    << proc->getQueue().size()
                  << "\n";
    }

    std::cout << "\n  Log saved to: logs/ipc_simulation.log\n\n";

    // Clean up
    for (auto* proc : processes) {
        delete proc;
    }

    Logger::getInstance().closeLogFile();
    return 0;
}
