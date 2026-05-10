# ============================================================
# Makefile - Microkernel Message Passing IPC System
# Course: CS2006 - Operating Systems
# Group: Keertan (24k-0716), Rohan Kumar (24k-0778),
#        Muhammad Ammar Adil (24k-0510)
# ============================================================

# Compiler and flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I./include

# SFML libraries for GUI (install with: sudo apt install libsfml-dev)
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# All source files
SRCS = src/main.cpp \
       src/Message.cpp \
       src/Process.cpp \
       src/MessageQueue.cpp \
       src/IPCManager.cpp \
       src/Scheduler.cpp \
       src/Logger.cpp \
       src/GUIManager.cpp \
       src/StressTester.cpp

# Object files go in obj/ folder
OBJS = $(SRCS:.cpp=.o)

# Final binary name
TARGET = ipc_system

# -----------------------------------------------
# Default build target
# -----------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(SFML_LIBS)
	@echo ""
	@echo "  Build successful! Run with: ./$(TARGET)"
	@echo ""

# Compile each .cpp into a .o
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# -----------------------------------------------
# Build without GUI (no SFML needed)
# Useful for testing backend logic only
# -----------------------------------------------
no_gui: CXXFLAGS += -DNO_GUI
no_gui: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET)_nogui $(SRCS) -lpthread
	@echo "  No-GUI build done. Run: ./$(TARGET)_nogui"

# -----------------------------------------------
# Run stress tests only
# -----------------------------------------------
test: no_gui
	./$(TARGET)_nogui --stress-test

# -----------------------------------------------
# Clean all compiled files
# -----------------------------------------------
clean:
	rm -f src/*.o $(TARGET) $(TARGET)_nogui
	@echo "  Cleaned."

# -----------------------------------------------
# Show help
# -----------------------------------------------
help:
	@echo ""
	@echo "  Usage:"
	@echo "    make          - Build with GUI (needs SFML)"
	@echo "    make no_gui   - Build without GUI"
	@echo "    make test     - Run stress tests"
	@echo "    make clean    - Remove compiled files"
	@echo ""

.PHONY: all no_gui test clean help
