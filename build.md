# Build & Execution Guide

This document outlines how to compile the Addition Chain Engine from source, generate the local binary databases, and extract optimal chains using the included Python utility.

---

## 1. Prerequisites

To compile and run the engine, your system must have:
* **C++ Compiler:** GCC/G++ (v9.0 or higher recommended) or MinGW for Windows.
* **OpenMP:** Usually bundled with GCC.
* **Python 3.x:** Required to run the query script.

---

## 2. Compiling the Engine (`general_engine.cpp`)

The engine relies heavily on physical core mapping and cache optimization. It is critical to compile with the correct flags to unlock hardware-level acceleration.

Run the following command in your terminal:

```bash
g++ -O3 -fopenmp -march=native general_engine.cpp -o chain_engine
