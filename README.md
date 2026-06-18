Addition Chain Engine (v2.0 - Delta Folded Architecture)

STATUS: MAPPING [N=0 → 8.38M] | ACTIVE_GRIND_MODE

A high-performance C++ engine engineered to mine the mathematical ground-truth of Shortest Addition Chains (OEIS A003313).

Designed for bare-metal execution, this engine leverages OpenMP dynamic scheduling, iterative depth-first search (IDDFS) with mathematical branch-and-bound pruning, and a sophisticated bit-packed binary persistence layer.

🖥️ Live Frontend Tracking: caring4change.au/tools/ACT.html
⚡ Core Architecture

    Multi-Threaded Grind: Hard-locked to physical cores via OpenMP dynamic scheduling. This eliminates OS-level context switching and mitigates "Prime Wall" thread starvation by dynamically re-assigning sub-tasks as complexity varies.

    Mathematical Pruning (IDDFS): Implements a look-ahead branch-and-bound search. Branches that cannot reach the target within the optimal depth—based on the current head-value's maximum doubling rate—are culled in the nanosecond range.

    22-Bit Folded Delta Storage: To maintain a coupled 32-bit atomic structure, the engine employs a "Folded Ruler" encoding. Instead of storing the absolute parentA (which overflows at 2M), it stores the Delta (N−parentA). Given that parentA≥N/2, this Delta never exceeds the 22-bit address space, effectively doubling the engine's architectural ceiling to 8,388,606.

    Distributed Horizontal Scaling: Designed for cluster computation. Compute any specific segment of the number line independently via CLI arguments (e.g., ./engine 400000 500000). Resulting binary chunks are binary-compatible and can be concatenated to grow the master ledger continuously.

    Failsafe Verification: Automated parity-check against official OEIS sequence data (up to N=100,000) performed on every chunk completion.

💻 Reference Hardware Benchmark (Device: r1x)
Component	Specification
Processor	AMD Ryzen 5 8500G (6 Cores / 12 Threads)
Memory	16.0 GB RAM
Architecture	32-Bit Atomic (Packed 22-Bit Delta)
📖 Documentation & Setup

    USAGE.md: Compilation instructions (g++ -O3 -fopenmp), thread-tuning, and CLI parameter range definitions.

    MIGRATION.md: Instructions for using the repacker utility to convert older 21-bit absolute ledgers into the new 22-bit Delta format.

    ABOUT.md: Algorithmic philosophy, the geometric proof of the "Prime Cliff," and the logic behind the Folded Delta codec.

⚖️ License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0). You are free to use, modify, and distribute this software, provided that any derivative works are also made open-source under the same GPLv3 terms.
Engineering Roadmap: The Path to 8.38 Million

The engine is currently configured for a 32-bit word size. By shaving the bit-depth of the step-counter from 7 bits down to 6, we have reclaimed 1 bit for the Delta, effectively shifting your mapping horizon from ~4M to ~8.38M.
