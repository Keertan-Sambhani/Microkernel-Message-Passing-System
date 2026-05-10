#ifndef COMMON_H
#define COMMON_H

// ============================================================
// Common.h
// Shared constants, enums, and includes used by all modules.
// Putting these here avoids repeating them in every file.
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <functional>
#include <sstream>
#include <iomanip>
#include <map>
#include <random>

// -----------------------------------------------
// Message Priority Levels
// Higher number = higher priority
// -----------------------------------------------
enum class Priority {
    LOW    = 1,
    MEDIUM = 2,
    HIGH   = 3
};

// Helper: convert Priority to readable string
inline std::string priorityToString(Priority p) {
    if (p == Priority::HIGH)   return "HIGH";
    if (p == Priority::MEDIUM) return "MEDIUM";
    return "LOW";
}

// -----------------------------------------------
// Process States
// -----------------------------------------------
enum class ProcessState {
    RUNNING,
    BLOCKED,   // waiting for a message
    IDLE
};

inline std::string stateToString(ProcessState s) {
    if (s == ProcessState::RUNNING) return "RUNNING";
    if (s == ProcessState::BLOCKED) return "BLOCKED";
    return "IDLE";
}

// -----------------------------------------------
// Simulation Settings
// Tweak these to change simulation behavior
// -----------------------------------------------
namespace Config {
    const int   MAX_QUEUE_SIZE     = 20;    // max messages per process queue
    const float MSG_LOSS_CHANCE    = 0.05f; // 5% chance a message gets dropped
    const int   NUM_PROCESSES      = 5;     // default number of simulated processes
    const int   SIMULATION_SECONDS = 15;    // how long the simulation runs
    const int   AGING_THRESHOLD    = 5;     // after this many skips, boost LOW priority
}

// -----------------------------------------------
// Utility: get current timestamp as string
// Used in logs and message timestamps
// -----------------------------------------------
inline std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return ss.str();
}

#endif // COMMON_H
