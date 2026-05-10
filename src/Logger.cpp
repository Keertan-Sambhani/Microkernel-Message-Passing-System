// ============================================================
// Logger.cpp
// Thread-safe logging to console + file.
// Uses a mutex so multiple threads don't interleave output.
// ============================================================

#include "Logger.h"

// Return the one global Logger instance
Logger& Logger::getInstance() {
    static Logger instance; // created once, lives until program ends
    return instance;
}

void Logger::openLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    logFile.open(filename, std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "[Logger] Warning: could not open log file: " << filename << "\n";
    }
}

void Logger::closeLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// ------------------------------------------------------------------
// log: the main logging function.
// Writes a formatted line to console and file.
// Thread-safe — multiple processes can call this simultaneously.
// ------------------------------------------------------------------
void Logger::log(LogEvent event, const std::string& details) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::string line = "[" + getCurrentTimestamp() + "] "
                     + "[" + eventToString(event) + "] "
                     + details;

    // Print to console
    std::cout << line << "\n";

    // Write to file
    if (logFile.is_open()) {
        logFile << line << "\n";
        logFile.flush(); // make sure it's written immediately
    }

    // Keep in memory for GUI display
    recentLogs.push_back(line);
    if ((int)recentLogs.size() > MAX_RECENT) {
        recentLogs.erase(recentLogs.begin()); // drop oldest
    }

    // Update counters
    if (event == LogEvent::MSG_SENT)     totalSent++;
    if (event == LogEvent::MSG_RECEIVED) totalReceived++;
    if (event == LogEvent::MSG_DROPPED)  totalDropped++;
}

void Logger::info(const std::string& msg) {
    log(LogEvent::GENERAL, msg);
}

// ------------------------------------------------------------------
// printStats: summary printed at end of simulation
// ------------------------------------------------------------------
void Logger::printStats() const {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << "\n==============================\n";
    std::cout << "  SIMULATION STATISTICS\n";
    std::cout << "==============================\n";
    std::cout << "  Messages Sent:     " << totalSent.load()     << "\n";
    std::cout << "  Messages Received: " << totalReceived.load() << "\n";
    std::cout << "  Messages Dropped:  " << totalDropped.load()  << "\n";
    std::cout << "==============================\n\n";
}

std::vector<std::string> Logger::getRecentLogs(int count) const {
    std::lock_guard<std::mutex> lock(logMutex);
    int start = std::max(0, (int)recentLogs.size() - count);
    return std::vector<std::string>(recentLogs.begin() + start, recentLogs.end());
}

std::string Logger::eventToString(LogEvent e) const {
    switch (e) {
        case LogEvent::MSG_SENT:          return "SENT    ";
        case LogEvent::MSG_RECEIVED:      return "RECV    ";
        case LogEvent::MSG_DROPPED:       return "DROPPED ";
        case LogEvent::MSG_QUEUED:        return "QUEUED  ";
        case LogEvent::QUEUE_FULL:        return "Q-FULL  ";
        case LogEvent::PROCESS_BLOCKED:   return "BLOCKED ";
        case LogEvent::PROCESS_UNBLOCKED: return "UNBLOCKD";
        case LogEvent::SCHEDULER_EVENT:   return "SCHED   ";
        case LogEvent::STRESS_TEST:       return "STRESS  ";
        default:                          return "INFO    ";
    }
}
