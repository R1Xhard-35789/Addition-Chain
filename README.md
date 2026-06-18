Addition Chain Engine

    STATUS: MAPPING | ACTIVE_GRIND_MODE

A raw, high-performance C++ engine designed to mine the mathematical ground-truth of Shortest Addition Chains, strictly matching the OEIS A003313 sequence.

Built for bare-metal execution, this engine utilizes OpenMP dynamic scheduling, mathematical branch-and-bound pruning, and a zero-overhead binary persistence system to brute-force optimal additive paths through the integer number line.

🖥️ Live Frontend Tracking: caring4change.au/tools/ACT.html
⚡ Core Architecture

    Multi-Threaded Grind: Hard-locked to physical cores via OpenMP dynamic scheduling. This eliminates OS scheduling overhead and handles the highly variable computational depth of "Prime Walls" without thread starvation.

    Mathematical Pruning: Employs a look-ahead Iterative Deepening Depth-First Search (IDDFS). If the maximum possible geometric progression (doubling the current path's head value) cannot reach the target within the remaining depth allocation, the branch is culled instantly.

    Zero-Overhead Atomic Storage: State is continuously serialized into a single, packed 32-bit binary ledger (master_ledger.bin). This creates an indestructible atomic record that allows instant, automated resuming without complex database drivers or file-slide corruption.

    Distributed Range Compute (Horizontal Scaling): Compute any specific chunk of the number line independently via CLI arguments (e.g., ./chain_engine 40000 50000). Because the engine builds chains entirely from scratch rather than relying on dynamic programming, isolated nodes can map network segments concurrently. The resulting 32-bit binary chunks can then be instantly concatenated to form a continuous master ledger.

    Failsafe Verification: Automated against the official OEIS sequence up to N=100,000.

💻 Reference Hardware Benchmark

The development and reference testing environment for this engine uses the following bare-metal configuration:
Component	Specification	Device Name
Processor	AMD Ryzen 5 8500G (3.55 GHz, 6 Physical Cores / 12 Threads)	r1x
Memory	16.0 GB RAM (15.1 GB usable)	
Graphics	AMD Radeon 740M Graphics	
OS	Windows / Architecture: x64	
📖 Documentation Directory

    USAGE.md: Full instructions for compiling the C++ engine, passing CLI range parameters, tuning threads, and using the Python query script to unpack the binary data and 32-bit metadata flags.

    ABOUT.md: A deeper dive into the algorithmic philosophy, the "Prime Cliff" problem, and why this engine relies on geometric limits rather than algebraic guessing.

⚖️ License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).
You are free to use, modify, and distribute this software, provided that any derivative works are also made open-source under the same GPLv3 terms. Kudos and credit to the original repository are appreciated!
