#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <omp.h>

using namespace std;

#pragma pack(push, 1)
struct FatIndexEntry {
    uint64_t blob_offset;
    uint16_t length;
    uint16_t flags;
};
#pragma pack(pop)

// ==========================================
// FAST MATH HELPERS
// ==========================================
bool is_prime_n(uint32_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (uint32_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

bool is_perfect_square(uint32_t n) {
    uint32_t sq = std::round(std::sqrt(n));
    return (sq * sq == n);
}

bool is_square_free(uint32_t n) {
    if (n % 4 == 0) return false;
    for (uint32_t i = 3; i * i <= n; i += 2) {
        if (n % (i * i) == 0) return false;
    }
    return true;
}

bool is_binary_palindrome(uint32_t n) {
    uint32_t reversed = 0, temp = n;
    while (temp > 0) {
        reversed = (reversed << 1) | (temp & 1);
        temp >>= 1;
    }
    return reversed == n;
}

bool is_small_base_power(uint32_t n) {
    if (n < 9) return false; // Exclude 2^k since we have a dedicated bit for that
    for (uint32_t b = 3; b <= 7; ++b) {
        uint32_t temp = b;
        while (temp < n) temp *= b;
        if (temp == n) return true;
    }
    return false;
}

int main() {
    cout << "=========================================" << endl;
    cout << " 16-BIT TOPOLOGICAL ENRICHER / REPACKER  " << endl;
    cout << "=========================================" << endl;

    // 1. Read Index
    ifstream index_in("chain_index.bin", ios::binary | ios::ate);
    if (!index_in.is_open()) {
        cout << "[ERROR] Could not find chain_index.bin!" << endl;
        return 1;
    }
    
    streampos file_size = index_in.tellg();
    int total_records = file_size / sizeof(FatIndexEntry);
    
    index_in.seekg(0, ios::beg);
    vector<FatIndexEntry> ledger(total_records);
    index_in.read(reinterpret_cast<char*>(ledger.data()), file_size);
    index_in.close();

    cout << "Loaded " << total_records << " records. 6-Core Enrichment engaged..." << endl;

    // 2. Parallel Processing
    #pragma omp parallel for schedule(dynamic)
    for (uint32_t n = 2; n < total_records; ++n) {
        uint16_t mask = 0;
        uint16_t steps = ledger[n].length;
        
        // Bit 0: Is Prime
        bool is_p = is_prime_n(n);
        if (is_p) mask |= (1 << 0);
        
        // Bit 1: Safe Prime (Prime Double)
        if (is_p && is_prime_n((n - 1) / 2)) mask |= (1 << 1);
        
        // Bit 2: High Hamming Weight
        int bit_length = (int)std::log2((double)n) + 1;
        if (__builtin_popcount(n) > bit_length / 2) mask |= (1 << 2);

        // Bit 3: Peak Anomaly (Requires looking at neighbors)
        if (n > 2 && n < total_records - 1) {
            if (steps > ledger[n-1].length && steps > ledger[n+1].length) mask |= (1 << 3);
        }

        // Bit 4: Fibonacci Step
        uint64_t n2 = (uint64_t)n * n;
        uint64_t f1 = 5 * n2 + 4, f2 = 5 * n2 - 4;
        if (is_perfect_square(f1) || is_perfect_square(f2)) mask |= (1 << 4);

        // Bit 5: Factor Method Match
        // Instantly checks if L(N) == L(A) + L(B) for its smallest prime factor
        if (!is_p && n > 3) {
            for (uint32_t i = 2; i * i <= n; ++i) {
                if (n % i == 0) {
                    if (steps == ledger[i].length + ledger[n/i].length) {
                        mask |= (1 << 5);
                    }
                    break; // Only test smallest factor
                }
            }
        }

        // Bit 6: Perfect Square
        if (is_perfect_square(n)) mask |= (1 << 6);

        // Bit 7: Square-Free
        if (is_square_free(n)) mask |= (1 << 7);

        // Bit 8: Triangular Number (8n + 1 is square)
        if (is_perfect_square(8 * n + 1)) mask |= (1 << 8);

        // Bit 9: Exact Power of 2
        if ((n & (n - 1)) == 0) mask |= (1 << 9);

        // Bit 10: Mersenne Number (All 1s)
        if (((n + 1) & n) == 0) mask |= (1 << 10);

        // Bit 11: Binary Palindrome
        if (is_binary_palindrome(n)) mask |= (1 << 11);

        // Bit 12: Heavy Divisors (Composite focus)
        int divs = 0;
        for (uint32_t i = 1; i * i <= n; i++) {
            if (n % i == 0) { divs += (i * i == n) ? 1 : 2; }
        }
        if (divs >= 12) mask |= (1 << 12);

        // Bit 13: Base 3-7 Power
        if (is_small_base_power(n)) mask |= (1 << 13);

        ledger[n].flags = mask;
    }

    // 3. Save the new Enriched Index
    ofstream index_out("chain_index_enriched.bin", ios::binary);
    index_out.write(reinterpret_cast<const char*>(ledger.data()), file_size);
    index_out.close();

    cout << "[SUCCESS] Saved chain_index_enriched.bin!" << endl;
    cout << "Overwrite 'chain_index.bin' with this file." << endl;
    cout << "=========================================" << endl;

    return 0;
}
