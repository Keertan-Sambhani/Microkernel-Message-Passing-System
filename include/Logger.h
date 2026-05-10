#ifndef LOGGER_H
#define LOGGER_H

// ============================================================
// Logger.h
// A simple thread-safe logger. Writes events to console
// and to a log file. Used by all other modules.
// ============================================================

#include "Common.h"
#include <fstream>

// Event types we track
enum class LogEvent {
    MSG_SENT,
    MSG_RECEIVED,
    MSG_DROPPED,
    MSG_QUEUED,
    QUEUE_FULL,
    PROCESS_BLOCKED,
    PROCESS_UNBLOCKED,
    SCHEDULER_EVENT,
    STRESS_TEST,
    GENERAL
};

class Logger {
public:
    // Singleton — only one logger exists in the whole program
    static Logger& getInstance();

    // Log an event with a description
    void log(LogEvent event, const std::string& details);

    // Log a plain message (for general info)
    void info(const std::string& msg);

    // Print stats summary at the end
    void printStats() const;

    // Returns recent log lines (for GUI display)
    std::vector<std::string> getRecentLogs(int count = 15) const;

    // Open the log file
    void openLogFile(const std::string& filename);
    void closeLogFile();

private:
    Logger() : totalSent(0), totalReceived(0), totalDropped(0) {}
    ~Logger() { closeLogFile(); }

    // Prevent copying (singleton)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    mutable std::mutex logMutex;
    std::ofstream logFile;

    // Keep last N log lines in memory for GUI
    std::vector<std::string> recentLogs;
    static const int MAX_RECENT = 100;

    // Counters
    std::atomic<int> totalSent;
    std::atomic<int> totalReceived;
    std::atomic<int> totalDropped;

    std::string eventToString(LogEvent e) const;
};

// Convenience macro so we don't type Logger::getInstance() everywhere
#define LOG(event, msg) Logger::getInstance().log(event, msg)
#define LOGINFO(msg)    Logger::getInstance().info(msg)

#endif // LOGGER_H
