# DSA Queue Simulator - Traffic Light System


A C++/SDL3 implementation of a traffic light queue management system for COMP202 Data Structures and Algorithms assignment.

## 📋 Description
Simulation of a traffic junction management system using queue data structures. Features:
- Priority-based vehicle queue management
- SDL3 graphical visualization
- Traffic light state management
- Vehicle generation system
- Lane prioritization logic

## 🚀 Features
- Real-time traffic simulation
- Priority lane handling (AL2)
- Normal/High-priority mode switching
- Vehicle queue visualization
- Traffic light state synchronization
- File-based inter-process communication

## 🛠️ Build Instructions

### Prerequisites
- C++17 compatible compiler
- CMake 3.12+
- SDL3 library
- Git

### Build Steps
```bash
# Clone repository with submodules
git clone --recursive https://github.com/sarbeshkc/dsa-queue-simulator.git
cd dsa-queue-simulator

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. && cmake --build .

# Run the simulation
cd Debug && ./comp_assignment