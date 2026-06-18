ABOUT: The Addition Chain Research Engine
The Philosophy of Empirical Truth

Traditional number theory often relies on algebraic "shortcuts" or heuristics to approximate properties of integers. While efficient, these methods are essentially educated guesses that ignore the underlying physical structure of the number line.

This project is built on the philosophy of Empirical Truth. Instead of using shortcuts, this engine performs an exhaustive search of every possible addition path to determine the absolute, shortest chain for any integer N. We treat the number line not as an abstract set of symbols, but as a computational landscape that must be physically mapped.
The "Prime Cliff" Problem

As we iterate through the integers, we observe recurring "Prime Cliffs"—computational bottlenecks where the search space for an optimal chain expands exponentially.

Algebraic guessing (like the Power Method or Binary Method) fails to account for the "resistance" of these numbers. Our engine treats these Cliffs as empirical data points. By identifying exactly where the computation time spikes, we are mapping the computational friction of the integers themselves, providing data for future studies in cryptographic vulnerability and prime density.
Why Geometric Limits Over Algebraic Guessing

Algebraic shortcuts often assume that the shortest path to N is always a function of its binary representation. This is frequently false.

Our engine uses Iterative Deepening Depth-First Search (IDDFS) with strict geometric pruning. We define a "geometric horizon"—if a branch cannot possibly reach the target N within the current depth limit (even by doubling at every step), we prune the branch instantly. By discarding algebraic assumptions and forcing the engine to "see" the entire geometric landscape, we discover "Non-Star" chains—rare, complex paths that algebraic methods are blind to.
The Atomic Binary Ledger

To handle the massive scale of this data, we moved away from fragile, text-based arrays toward a 32-bit Atomic Packed Architecture.
🛠️ The 32-Bit Atomic Master Stamp (Updated: 22-Bit Delta Mode)

Every integer is mapped to a contiguous 4-byte (32-bit) atomic block in master_ledger.bin. The engine employs a "Folded Ruler" delta codec to map addition chains up to N = 8,388,606 without sacrificing storage efficiency.
Bit Range	Size	Purpose	Description
0–5	6 bits	Steps	Absolute minimum chain length (optimized for N up to 8.38M).
6–9	4 bits	Flags	Metadata markers: Primality, Doubling, Star/Non-Star status.
10–31	22 bits	Payload	Stores the Delta (N−parentA), allowing for deep mapping.
Why this architecture changes everything:

    Geometric Compression: By storing the distance (Delta) rather than the absolute parentA, we bypass the 21-bit hard ceiling, doubling our addressable number line while remaining fully coupled within a 32-bit word.

    Instant Seeking: With a fixed 4-byte record size, finding the geodesic path for any target N is a constant-time O(1) memory operation, seek(N * 4).

    Zero-Copy Logic: The JavaScript visualizer can perform bit-shifts directly on the raw buffer, rendering ancestry paths without needing to "parse" complex JSON or string objects.

The "Non-Star" Anomaly: Standard addition chain heuristics (like the Binary Method) rely on "Star Chains," where each step must use the immediately preceding value. Our engine actively searches for "Non-Star" chains, which deviate from this rule. By mapping these anomalies, we are uncovering the computational "dark matter" of addition chains—paths that are mathematically shorter but hidden from traditional algebraic approaches.
