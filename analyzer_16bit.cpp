#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <unordered_set>

using namespace std;

// ==========================================
// 1. FAST MATH HELPERS
// ==========================================

// Fast Primality Test
bool is_prime_n(uint32_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (uint32_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

// Precompute Fibonacci numbers up to the 8.38M limit
unordered_set<uint32_t> generate_fib_set(uint32_t limit) {
    unordered_set<uint32_t> fibs;
    uint32_t a = 1, b = 2;
    fibs.insert(a);
    fibs.insert(b);
    while (true) {
        uint32_t next = a + b;
        if (next > limit) break;
        fibs.insert(next);
        a = b;
        b = next;
    }
    return fibs;
}

// Check if N is a perfect power of 2, 3, 5, or 7 (Bases 2-9)
bool is_small_base_power(uint32_t n) {
    // Power of 2 is a fast bitwise check
    if (n > 0 && (n & (n - 1)) == 0) return true;
    
    // Check powers of 3, 5, 7
    uint32_t bases[] = {3, 5, 7};
    for (uint32_t b : bases) {
        uint32_t temp = n;
        while (temp % b == 0) temp /= b;
        if (temp == 1) return true;
    }
    return false;
}

// ==========================================
// 2. MAIN ENRICHMENT ENGINE
// ==========================================

int main() {
    string input_file = "master_ledger.bin";
    string output_file = "enrichment.bin";

    cout << "[*] Loading Master Ledger into RAM..." << flush;
    ifstream infile(input_file, ios::binary | ios::ate);
    if (!infile.is_open()) {
        cerr << "\n[ERROR] Could not open " << input_file << endl;
        return 1;
    }

    streamsize size = infile.tellg();
    infile.seekg(0, ios::beg);
    
    uint32_t max_n = size / sizeof(uint32_t);
    vector<uint32_t> master_ledger(max_n);
    infile.read(reinterpret_cast<char*>(master_ledger.data()), size);
    cout << " Done. (" << max_n << " entries found)" << endl;

    cout << "[*] Precomputing mathematical constants..." << flush;
    unordered_set<uint32_t> fib_set = generate_fib_set(max_n);
    vector<uint16_t> enrichment_ledger(max_n, 0); // The new 16-bit array
    cout << " Done." << endl;

    cout << "[*] Running 16-Bit Post-Analysis..." << flush;

    // Loop starts at 2, stops at max_n - 1 (so we can safely check N+1 for peaks)
    for (uint32_t n = 2; n < max_n - 1; ++n) {
        
        // Unpack Current N
        uint32_t raw_data = master_ledger[n];
        uint8_t steps = raw_data & 0x3F;
        uint32_t delta = (raw_data >> 10) & 0x3FFFFF;
        uint32_t parentA = n - delta;
        uint32_t parentB = n - parentA;

        // Unpack Neighbors for Topological Anomalies
        uint8_t prev_steps = master_ledger[n - 1] & 0x3F;
        uint8_t next_steps = master_ledger[n + 1] & 0x3F;

        uint16_t mask = 0;

        // --- 4-BIT DEFECT (Bits 12-15) ---
        uint16_t defect = steps - (uint16_t)log2(n);
        if (defect > 15) defect = 15; // Hard cap
        mask |= (defect << 12);

        // --- 12-BIT BOOLEAN FLAGS ---
        
        // Bit 0: Is Prime
        if (is_prime_n(n)) mask |= (1 << 0);

        // Bit 1: Is Fibonacci Step (Both parents are Fibonacci numbers)
        if (fib_set.count(parentA) && fib_set.count(parentB)) mask |= (1 << 1);

        // Bit 2: Uses Factor Method
        if ((parentA > 1 && n % parentA == 0) || (parentB > 1 && n % parentB == 0)) mask |= (1 << 2);

        // Bit 3: Is Peak Anomaly (L(N) > L(N+1))
        if (steps > next_steps) mask |= (1 << 3);

        // Bit 4: Is Valley Anomaly (L(N) < L(N-1))
        if (steps < prev_steps) mask |= (1 << 4);

        // Bit 5: Safe Prime (Prime Double)
        if (is_prime_n(n) && is_prime_n((n - 1) / 2)) mask |= (1 << 5);

        // Bit 6: Mersenne Prime (2^k - 1)
        if ((n & (n + 1)) == 0 && is_prime_n(n)) mask |= (1 << 6);

        // Bit 7: High Hamming Weight (More 1s than 0s in binary)
        int bit_length = log2(n) + 1;
        if (__builtin_popcount(n) > bit_length / 2) mask |= (1 << 7);

        // Bit 8: Near Power of 2 (3-Offset: 2^k +/- 3)
        if (((n + 3) & (n + 2)) == 0 || ((n - 3) & (n - 4)) == 0) mask |= (1 << 8);

        // Bit 9: Strict Star Chain (Could be approximated if parentA == N-1, but requires full path traversal. Left disabled/placeholder for now).
        // if (parentA == n - 1) mask |= (1 << 9); 

        // Bit 10: Is Perfect Power of Small Base (2, 3, 5, 7)
        if (is_small_base_power(n)) mask |= (1 << 10);

        // Save to enrichment array
        enrichment_ledger[n] = mask;
    }

    cout << " Done." << endl;

    // Write the output payload
    cout << "[*] Writing " << output_file << "..." << flush;
    ofstream outfile(output_file, ios::binary);
    // Write the whole vector in one atomic dump
    outfile.write(reinterpret_cast<const char*>(enrichment_ledger.data()), max_n * sizeof(uint16_t));
    cout << " Saved " << (max_n * sizeof(uint16_t)) / (1024 * 1024) << " MB successfully." << endl;

    return 0;
}
