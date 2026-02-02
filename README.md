# Thread Synchronization - Traffic Intersection Simulation

A multi-threaded traffic intersection simulation demonstrating advanced concurrency concepts using POSIX pthreads. Each vehicle operates as an independent thread, coordinating through mutexes and condition variables to safely navigate intersections.

![C](https://img.shields.io/badge/C-POSIX-blue)
![Threads](https://img.shields.io/badge/POSIX-pthreads-green)
![Course](https://img.shields.io/badge/Course-CSC369-orange)

## Overview

This project implements two types of intersection control:
- **Stop Sign Intersections**: Cars yield and check quadrant availability before proceeding
- **Traffic Light Intersections**: Cars follow light state (N-S green, E-W green, or red)

Each car runs in its own pthread, and the simulation ensures safe coordination without collisions or deadlocks.

For detailed assignment instructions, see the [instructions PDF](a2-instructions.pdf).

## Features

- **Lane Ordering**: FIFO queues ensure cars exit in arrival order per lane
- **Quadrant Collision Prevention**: Cars acquire required quadrants atomically
- **Traffic Light State Machine**: Three-state light with proper transition handling
- **Left-Turn Safety**: Left-turning cars wait for opposite straight traffic to clear
- **Runtime Collision Detection**: MutexAccessValidator verifies correct sequencing

## Tech Stack

| Component | Technology |
|-----------|------------|
| **Language** | C (C99/POSIX) |
| **Threading** | POSIX pthreads |
| **Synchronization** | Mutexes, Condition Variables |
| **Build System** | Make/GCC |

## Architecture

```
a2/
├── carsim.c              # Main simulation entry point
├── car.h/car.c           # Car struct with position/action enums
├── intersection.h/c      # Common lane ordering logic
├── stopSign.h/c          # Base stop sign implementation
├── safeStopSign.h/c      # Thread-safe stop sign wrapper
├── trafficLight.h/c      # Base traffic light implementation
├── safeTrafficLight.h/c  # Thread-safe traffic light wrapper
├── mutexAccessValidator.h/c  # Collision detection
└── Makefile
```

## Synchronization Strategy

### Stop Sign
```c
pthread_mutex_t lane_mutex[4];      // Per-direction queue protection
pthread_cond_t lane_turn[4];        // Wake cars at queue front
pthread_mutex_t quadrants_mutex;     // Global quadrant state
pthread_cond_t quadrants_turn;       // Wake cars waiting for quadrants
```

### Traffic Light
```c
pthread_mutex_t lane_mutex[12];     // Per-lane (direction × action) protection
pthread_cond_t lane_turn[12];       // Wake cars at queue front
pthread_mutex_t light_mutex;         // Global light state
pthread_cond_t light_turn;           // Wake cars on light change
```

## Building & Running

```bash
cd a2

# Compile
make

# Run simulation
./carsim

# Clean
make clean
```

## Why This Project is Interesting

### Demonstrates Advanced Concurrency Skills

1. **Real-World Problem Modeling**
   - Models autonomous vehicle coordination - a practical AI/robotics challenge
   - Safety-critical requirements (no collisions, no deadlocks)

2. **Complex Synchronization Patterns**
   - Hierarchical locking (lane-level + global locks)
   - Condition variables with proper spurious wakeup handling
   - Broadcast signaling for multiple waiters
   - Queue-based ordering constraints across threads

3. **Deadlock Avoidance**
   - Lock ordering discipline prevents circular waits
   - Careful critical section management
   - Condition variable broadcasts prevent lost wakeups

4. **Data Structure Innovation**
   - Custom LaneQueue maintains exit ordering despite concurrent access
   - Tracks original front while advancing current front
   - Elegant solution to "FIFO guarantee across threads"

### Technical Skills Showcased

- **Systems Programming**: Low-level C, POSIX APIs
- **Concurrent Programming**: Thread synchronization primitives
- **Algorithm Design**: State machines, queue management
- **Debugging**: Race condition detection, deadlock analysis
- **Performance Optimization**: Balancing safety with throughput

### Applicable Domains

- **Distributed Systems**: Similar patterns in database locks, message queues
- **Real-Time Systems**: Traffic control, robotics coordination
- **Cloud Infrastructure**: Resource allocation, connection pooling
- **Game Development**: Entity coordination, physics simulation

## Course Context

Developed for CSC369 (Operating Systems) at University of Toronto, demonstrating practical application of OS concepts in thread synchronization.

## Author

[Kevin Mok](https://github.com/Kevin-Mok)
