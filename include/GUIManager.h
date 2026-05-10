#ifndef GUIMANAGER_H
#define GUIMANAGER_H

// ============================================================
// GUIManager.h
// SFML-based real-time visualization of the IPC system.
// Shows processes, their queues, message flow, and stats.
// Compiled away with -DNO_GUI flag for headless/test builds.
// ============================================================

#ifndef NO_GUI

#include "Common.h"
#include "Process.h"
#include "IPCManager.h"
#include <SFML/Graphics.hpp>
#include <vector>

class GUIManager {
public:
    GUIManager(std::vector<Process*>& procs, IPCManager* ipc);

    // Opens the window and runs the render loop
    // This blocks until the window is closed
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;

    std::vector<Process*>& processes;
    IPCManager* ipcManager;

    // Window dimensions
    static const int WIN_W = 1920;  // will be overridden at runtime
    static const int WIN_H = 1080;

    // Colors
    sf::Color bgColor        = sf::Color(20, 20, 35);
    sf::Color panelColor     = sf::Color(35, 35, 55);
    sf::Color highColor      = sf::Color(220, 60,  60);   // red for HIGH
    sf::Color medColor       = sf::Color(230, 160, 40);   // orange for MEDIUM
    sf::Color lowColor       = sf::Color(60,  180, 100);  // green for LOW
    sf::Color runningColor   = sf::Color(60,  200, 100);
    sf::Color blockedColor   = sf::Color(200, 80,  80);
    sf::Color idleColor      = sf::Color(150, 150, 150);
    sf::Color textColor      = sf::Color(220, 220, 220);
    sf::Color dimTextColor   = sf::Color(130, 130, 130);

    // Drawing helpers
    void drawProcessPanel(float x, float y, Process* proc);
    void drawQueueBar(float x, float y, float w, float h,
                      int queueSize, int maxSize);
    void drawStatsPanel(float x, float y);
    void drawLogPanel(float x, float y, float w, float h);
    void drawTitle();

    // Draw a filled rectangle with optional border
    void drawRect(float x, float y, float w, float h,
                  sf::Color fill, sf::Color border = sf::Color::Transparent,
                  float borderThickness = 0);

    // Draw text at position
    void drawText(const std::string& str, float x, float y,
                  int size, sf::Color color);

    sf::Color priorityColor(Priority p) const;
    sf::Color stateColor(ProcessState s) const;

    // Timestamp for FPS / refresh tracking
    sf::Clock clock;
    float refreshInterval = 0.1f; // redraw every 100ms
};

#endif // NO_GUI
#endif // GUIMANAGER_H
