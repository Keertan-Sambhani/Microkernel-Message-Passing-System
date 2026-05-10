// ============================================================
// GUIManager.cpp
// SFML-based real-time visualization.
// Draws each process as a panel showing:
//   - Process ID, priority, state
//   - Queue fill bar (color-coded)
//   - Message count stats
// Also shows live log feed and system-wide stats.
// Compiled away with -DNO_GUI.
// ============================================================

#ifndef NO_GUI

#include "GUIManager.h"
#include <cmath>

GUIManager::GUIManager(std::vector<Process*>& procs, IPCManager* ipc)
    : processes(procs), ipcManager(ipc)
{
    // Create window
sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
window.create(desktop, "Microkernel Message Passing System", sf::Style::Fullscreen);
    window.setFramerateLimit(30);

    // Try to load a font — fall back to default if not found
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")) {
        // Try alternative paths
        if (!font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf")) {
            if (!font.loadFromFile("/usr/share/fonts/opentype/urw-base35/NimbusMono-Regular.otf")) {
                // If no font found, GUI will still work, text just won't show
                std::cerr << "[GUI] Warning: no monospace font found. Text may not display.\n";
            }
        }
    }
}

// ------------------------------------------------------------------
// run: main render loop. Handles events and redraws every frame.
// ------------------------------------------------------------------
void GUIManager::run() {
    while (window.isOpen()) {

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }
            // Press Escape to exit fullscreen
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                window.close();
                return;
            }
        }

        // Get screen size ONCE here (not inside event loop)
        float screenW = window.getSize().x;
        float screenH = window.getSize().y;

        // Clear and redraw
        window.clear(bgColor);

        drawTitle();

        // Scale panel size based on screen width and number of processes
        int numProcs = (int)processes.size();
        float gapX   = screenW * 0.01f;
        float startX = 15.0f;
        float startY = screenH * 0.08f;
        float panelW = (screenW - startX * 2 - gapX * (numProcs - 1)) / numProcs;
        float panelH = screenH * 0.35f;

        for (int i = 0; i < numProcs; i++) {
            float x = startX + i * (panelW + gapX);
            float y = startY;
            drawProcessPanel(x, y, processes[i]);
        }

        float statsPanelY = startY + panelH + screenH * 0.02f;
        float statsPanelH = screenH * 0.12f;
        drawStatsPanel(15.0f, statsPanelY);

        float logY = statsPanelY + statsPanelH + screenH * 0.01f;
        drawLogPanel(15.0f, logY, screenW - 30.0f, screenH - logY - 15.0f);

        window.display();
    }
}
// ------------------------------------------------------------------
// drawProcessPanel: draws one process's info box
// ------------------------------------------------------------------
void GUIManager::drawProcessPanel(float x, float y, Process* proc) {
    // Get actual panel size from window proportions
    float screenW = window.getSize().x;
    float screenH = window.getSize().y;
    int numProcs  = (int)processes.size();
    float gapX    = screenW * 0.01f;
    float w       = (screenW - 30.0f - gapX * (numProcs - 1)) / numProcs;
    float h       = screenH * 0.35f;

    // Panel background
    drawRect(x, y, w, h, panelColor, sf::Color(70, 70, 100), 1.5f);

    // Header bar
    drawRect(x, y, w, h * 0.13f, stateColor(proc->getState()));

    float headerH = h * 0.13f;
    drawText(proc->getName() + "  [" + stateToString(proc->getState()) + "]",
             x + 7, y + headerH * 0.2f, 14, sf::Color::White);

    drawText("Priority: " + priorityToString(proc->getPriority()),
             x + 7, y + headerH + h * 0.04f, 13,
             priorityColor(proc->getPriority()));

    int qs = proc->getQueue().size();
    drawText("Queue: " + std::to_string(qs) + "/" +
             std::to_string(Config::MAX_QUEUE_SIZE),
             x + 7, y + headerH + h * 0.13f, 13, textColor);

    // Queue fill bar
    drawQueueBar(x + 7, y + headerH + h * 0.22f,
                 w - 14, h * 0.07f, qs, Config::MAX_QUEUE_SIZE);

    drawText("Sent:  " + std::to_string(proc->getSentCount()),
             x + 7, y + headerH + h * 0.33f, 12, dimTextColor);
    drawText("Rcvd:  " + std::to_string(proc->getReceivedCount()),
             x + 7, y + headerH + h * 0.42f, 12, dimTextColor);

    // Queued messages preview
    auto msgs = proc->getQueue().getSnapshot();
    drawText("-- Queued Messages --",
             x + 7, y + headerH + h * 0.54f, 11, dimTextColor);
    for (int i = 0; i < (int)std::min((int)msgs.size(), 4); i++) {
        std::string line = "  P" + std::to_string(msgs[i].senderID)
                         + " [" + priorityToString(msgs[i].priority).substr(0,1) + "] "
                         + msgs[i].text.substr(0, 12);
        sf::Color c = priorityColor(msgs[i].priority);
        drawText(line, x + 7, y + headerH + h * (0.63f + i * 0.09f), 11, c);
    }
}
// ------------------------------------------------------------------
// drawQueueBar: fill bar showing how full the queue is
// Color: green when low, yellow when mid, red when near full
// ------------------------------------------------------------------
void GUIManager::drawQueueBar(float x, float y, float w, float h,
                               int queueSize, int maxSize) {
    // Background (empty portion)
    drawRect(x, y, w, h, sf::Color(50, 50, 70));

    if (maxSize == 0) return;
    float ratio = (float)queueSize / (float)maxSize;

    // Color based on fill level
    sf::Color barColor;
    if (ratio < 0.5f)       barColor = sf::Color(60, 190, 90);   // green
    else if (ratio < 0.8f)  barColor = sf::Color(220, 160, 40);  // yellow
    else                    barColor = sf::Color(210, 60, 60);    // red

    drawRect(x, y, w * ratio, h, barColor);
}

// ------------------------------------------------------------------
// drawStatsPanel: global system statistics
// ------------------------------------------------------------------
void GUIManager::drawStatsPanel(float x, float y) {
    drawRect(x, y, WIN_W - 30.0f, 80.0f, panelColor, sf::Color(70, 70, 100), 1.0f);
    drawText("== System Stats ==", x + 10, y + 8, 13, textColor);

    std::string sent     = "Total Sent: "      + std::to_string(ipcManager->getTotalSent());
    std::string dropped  = "Dropped: "         + std::to_string(ipcManager->getTotalDropped());
    std::string deliverd = "Delivered: "       + std::to_string(ipcManager->getTotalDelivered());

    drawText(sent,     x + 10,  y + 30, 12, sf::Color(100, 200, 120));
    drawText(dropped,  x + 200, y + 30, 12, sf::Color(210, 80, 80));
    drawText(deliverd, x + 350, y + 30, 12, sf::Color(100, 160, 230));

    float lossRate = 0.0f;
    if (ipcManager->getTotalSent() > 0) {
        lossRate = 100.0f * ipcManager->getTotalDropped()
                           / ipcManager->getTotalSent();
    }
    std::string loss = "Loss Rate: " + std::to_string((int)lossRate) + "%";
    drawText(loss, x + 550, y + 30, 12, sf::Color(210, 150, 80));

    drawText("Press [X] to stop simulation", x + 10, y + 56, 11, dimTextColor);
}

// ------------------------------------------------------------------
// drawLogPanel: scrolling list of recent log entries
// ------------------------------------------------------------------
void GUIManager::drawLogPanel(float x, float y, float w, float h) {
    drawRect(x, y, w, h, sf::Color(25, 25, 40), sf::Color(60, 60, 90), 1.0f);
    drawText("== Live Log ==", x + 8, y + 6, 12, dimTextColor);

    auto logs = Logger::getInstance().getRecentLogs(18);
    float lineY = y + 24;
    for (const auto& line : logs) {
        // Trim long lines to fit panel
        std::string display = line.substr(0, 110);
        drawText(display, x + 8, lineY, 10, sf::Color(180, 180, 200));
        lineY += 13;
        if (lineY > y + h - 10) break;
    }
}

void GUIManager::drawTitle() {
    drawText("Microkernel IPC System  |  CS2006 OS  |  Real-Time Simulation",
             15, 10, 14, sf::Color(160, 180, 230));
}

// ------------------------------------------------------------------
// Drawing helpers
// ------------------------------------------------------------------
void GUIManager::drawRect(float x, float y, float w, float h,
                           sf::Color fill, sf::Color border, float thick) {
    sf::RectangleShape rect(sf::Vector2f(w, h));
    rect.setPosition(x, y);
    rect.setFillColor(fill);
    if (thick > 0) {
        rect.setOutlineThickness(thick);
        rect.setOutlineColor(border);
    }
    window.draw(rect);
}

void GUIManager::drawText(const std::string& str, float x, float y,
                           int size, sf::Color color) {
    sf::Text text;
    text.setFont(font);
    text.setString(str);
    text.setCharacterSize(size);
    text.setFillColor(color);
    text.setPosition(x, y);
    window.draw(text);
}

sf::Color GUIManager::priorityColor(Priority p) const {
    if (p == Priority::HIGH)   return highColor;
    if (p == Priority::MEDIUM) return medColor;
    return lowColor;
}

sf::Color GUIManager::stateColor(ProcessState s) const {
    if (s == ProcessState::RUNNING) return sf::Color(50, 140, 80);
    if (s == ProcessState::BLOCKED) return sf::Color(160, 60, 60);
    return sf::Color(80, 80, 100);
}

#endif // NO_GUI
