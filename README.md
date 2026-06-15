# Addition-Chain
C++ engine designed to mine the mathematical ground-truth of Shortest Addition Chains.

Project Philosophy: Empirical Truth

This project prioritizes realistic computation over theoretical shortcuts. By brute-forcing the search space, we identify "Prime Walls" and "Arithmetic Gridlock"—data points where standard mathematical rules deviate from physical reality. These anomalies are archived for further study in number theory and cryptographic vulnerability mapping.
Technical Architecture
1. The Engine (general_engine.cpp)

    Logic: Employs parallelized IDDFS to search the addition tree.

    Resilience: Features a zero-overhead persistence system. The engine calculates the current registry_limit on startup and seamlessly resumes work, making it suitable for distributed, multi-machine grinds.

    Performance: Utilizes #pragma omp for multi-threaded dynamic scheduling across physical CPU cores.

2. The Data Ledger

    master_lengths.bin: Stores the optimal step count for every integer (1 byte per N).

    master_parents.bin: Stores the parent indices A and B that produced N (8 bytes per N).

    verification_log.txt: An audit trail of compute time and status for every processed batch.
