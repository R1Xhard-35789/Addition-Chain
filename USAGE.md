# Build & Execution Guide

This document outlines how to compile the Addition Chain Engine from source, tune it for your specific hardware, and extract optimal chains using the included Python utility.

---

## 1. Prerequisites

To compile and run the engine, your system must have:
* **C++ Compiler:** GCC/G++ (v9.0 or higher recommended) or MinGW for Windows.
* **OpenMP:** Usually bundled with GCC.
* **Python 3.x:** Required to run the binary query script.

---

## 2. Compiling the Engine (`general_engine.cpp`)

The engine relies heavily on physical core mapping and cache optimization. It is critical to compile with the correct flags to unlock hardware-level acceleration.

Run the following command in your terminal:

```bash
g++ -O3 -fopenmp -march=native general_engine.cpp -o chain_engine

Understanding the Compiler Flags:

    -O3: Enables aggressive compiler optimizations, including loop unrolling, vectorization, and optimized register allocation.

    -fopenmp: Unlocks the #pragma omp directives required for high-performance multi-threaded dynamic scheduling.

    -march=native: Instructs the compiler to utilize every modern instruction set extension supported by your specific CPU (e.g., AVX2, AVX-512), generating a binary custom-tailored to your exact silicon.

3. Running the Engine & Thread Tuning

Execute the compiled binary to begin the brute-force mapping process:
Bash

# On Linux / Mac
./chain_engine

# On Windows
chain_engine.exe

🧵 Critical Note on Thread Allocation

The engine is explicitly hardcoded to use 6 threads via omp_set_num_threads(6); within general_engine.cpp.

Attempting to run at full logical capacity (e.g., allocating all 12 threads on a 6-core processor) introduces SMT (Simultaneous Multithreading) resource contention over the execution units and L1/L2 caches. This bottlenecks the arithmetic pipeline and actually degrades overall brute-force performance.
Porting to Your Setup:

Before compiling, open general_engine.cpp and modify the integer inside omp_set_num_threads(6); to match the exact number of physical cores available on your specific CPU.
💾 Auto-Resume Failsafe

You can safely stop the engine at any time using Ctrl+C. The engine features a zero-overhead persistence layer. Upon restart, it reads the exact byte size of the local database files and instantly resumes calculations from the exact integer it left off on.
4. Querying the Chains (query_chain.py)

    Note: The binary databases (master_lengths.bin and master_parents.bin) are generated locally on your machine and are not included in this repository due to storage constraints.

Because the engine outputs raw, contiguous binary arrays for maximum write-speed efficiency, the databases cannot be parsed with a standard text editor. Use the included query_chain.py script to unpack the binary structures, recursively trace the parent pointers, and synthesize the readable addition chain.
Usage

Provide the target integer as a command-line argument:
Bash

python query_chain.py 13337

Output

The script returns a JSON object containing the target, the optimal number of steps, the completely reconstructed sequence, and the current upper boundary of your localized index database:
JSON

{
  "target": 13337,
  "optimal_steps": 17,
  "synthesis": "1 → 2 → 4 → 8 → 16 → 32 → 64 → 128 → 256 → 512 → 1024 → 2048 → 4096 → 4104 → 8192 → 8208 → 12312 → 13336 → 13337",
  "registry_limit": 19005
}

If you query a target value that the C++ engine has not yet reached or mapped, the script will return an error indicating your current local registry limit.
