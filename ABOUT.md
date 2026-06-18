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
The 32-Bit Master Stamp

Every integer in our database is mapped to a single, contiguous 4-byte (32-bit) block in master_ledger.bin. This architecture is "Atomic"—it cannot be fractured by thread crashes or interrupted file writes.
Bit Range	Size	Purpose	Description
0–6	7 bits	Steps	Stores the absolute minimum step count (max 127).
7–10	4 bits	Flags	Metadata markers for Primality, Doubling, and Non-Star anomalies.
11–31	21 bits	Payload	Stores the immediate parent index, allowing for N up to 2,097,1
