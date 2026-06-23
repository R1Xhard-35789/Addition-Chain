#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <csignal>
#include <omp.h>
#include <iomanip>

using namespace std;

// ==========================================
// 1. CONFIGURATION
// ==========================================
const string FILE_INDEX = "chain_index.bin";
const string FILE_BLOB  = "chain_blob.bin";
const int RUN_LIMIT     = 100000;
const int FLUSH_CHUNK   = 120; // Multiple of 6 for clean batching

volatile sig_atomic_t keep_running = 1;
void sig_handler(int signum) { keep_running = 0; }

// ==========================================
// 2. ANALYZER & MATH HELPERS
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

uint16_t calculate_enrichment_mask(uint32_t target_n, const vector<uint32_t>& chain) {
    uint16_t mask = 0;
    
    // Bit 0: Is Prime
    if (is_prime_n(target_n)) mask |= (1 << 0);
    
    // Bit 1: Is Safe Prime (Prime Double)
    if (is_prime_n(target_n) && is_prime_n((target_n - 1) / 2)) mask |= (1 << 1);
    
    // Bit 2: High Hamming Weight (More 1s than 0s)
    int bit_length = (int)std::log2((double)target_n) + 1;
    if (__builtin_popcount(target_n) > bit_length / 2) mask |= (1 << 2);

    return mask;
}

// ==========================================
// 3. FAST-BREAK IDDFS SEARCH
// ==========================================
bool search_chain(int target, int current_depth, int max_depth, vector<uint32_t>& chain, vector<uint32_t>& best_chain) {
    if (chain.back() == target) {
        best_chain = chain;
        return true; 
    }
    if (current_depth == max_depth) return false;
    if ((chain.back() << (max_depth - current_depth)) < target) return false;

    for (int i = chain.size() - 1; i >= 0; --i) {
        for (int j = i; j >= 0; --j) {
            uint32_t next_val = chain[i] + chain[j];
            if (next_val > chain.back() && next_val <= target) {
                chain.push_back(next_val);
                if (search_chain(target, current_depth + 1, max_depth, chain, best_chain)) return true;
                chain.pop_back(); 
            }
        }
    }
    return false;
}

vector<uint32_t> solve_n(uint32_t target_n) {
    vector<uint32_t> best_chain;
    int target_depth = (int)std::ceil(std::log2((double)target_n));
    
    while (best_chain.empty() && keep_running) {
        vector<uint32_t> initial_chain = {1};
        search_chain(target_n, 0, target_depth, initial_chain, best_chain);
        if (best_chain.empty()) target_depth++;
    }
    return best_chain;
}

// ==========================================
// 4. MAIN SLIDING WINDOW ENGINE
// ==========================================
int main() {
    signal(SIGINT, sig_handler);
    omp_set_num_threads(6); 
    
    // Both buffers are raw bytes to ensure perfect Little-Endian packing
    vector<uint8_t> buf_index;
    vector<uint8_t> buf_blob;

    uint32_t start_n = 2;
    uint64_t current_blob_offset = 0;

    // RESUME LOGIC (Fat Index is 12 bytes per N)
    ifstream check_index(FILE_INDEX, ios::binary | ios::ate);
    if (check_index.is_open()) {
        start_n = check_index.tellg() / 12; 
        check_index.close();
        
        ifstream check_blob(FILE_BLOB, ios::binary | ios::ate);
        if (check_blob.is_open()) {
            current_blob_offset = check_blob.tellg();
            check_blob.close();
        }
        cout << "[RESUME] Starting at N=" << start_n << " | Blob Offset: " << current_blob_offset << endl;
    } else {
        cout << "[NEW RUN] Formatting Fat-Index Database..." << endl;
        // Pad N=0 and N=1 with 24 blank bytes total (12 bytes each)
        for(int i=0; i<24; i++) buf_index.push_back(0); 
    }

    uint32_t last_processed_n = start_n - 1; 
    auto batch_start = chrono::high_resolution_clock::now();

    cout << "\n[SLIDING WINDOW MODE] 6 Cores Hunting Asynchronously..." << endl;

    // THE SLIDING WINDOW: Out-of-order math, In-order loop resolution
    #pragma omp parallel for ordered schedule(dynamic, 1)
    for (uint32_t n = start_n; n <= RUN_LIMIT; ++n) {
        
        if (!keep_running) continue;

        // ---------------------------------------------------------
        // ASYNC PHASE: 6 Cores calculate at maximum speed
        // ---------------------------------------------------------
        vector<uint32_t> best_chain = solve_n(n);
        uint16_t length = (uint16_t)(best_chain.size() - 1);
        uint16_t flags = calculate_enrichment_mask(n, best_chain);

        // ---------------------------------------------------------
        // ORDERED PHASE: Strictly Sequential File Packing
        // ---------------------------------------------------------
        #pragma omp ordered
        {
            if (keep_running) {
                last_processed_n = n; 

                // 1. PACK FAT INDEX (12 Bytes)
                // A. Offset (8 Bytes)
                for (int b = 0; b < 8; ++b) buf_index.push_back((current_blob_offset >> (b * 8)) & 0xFF);
                // B. Length (2 Bytes)
                buf_index.push_back(length & 0xFF);
                buf_index.push_back((length >> 8) & 0xFF);
                // C. Flags (2 Bytes)
                buf_index.push_back(flags & 0xFF);
                buf_index.push_back((flags >> 8) & 0xFF);

                // 2. PACK BLOB (Length Prefix + 32-bit Integers)
                buf_blob.push_back(best_chain.size() & 0xFF);
                buf_blob.push_back((best_chain.size() >> 8) & 0xFF);
                current_blob_offset += 2;

                for (uint32_t num : best_chain) {
                    buf_blob.push_back(num & 0xFF);
                    buf_blob.push_back((num >> 8) & 0xFF);
                    buf_blob.push_back((num >> 16) & 0xFF);
                    buf_blob.push_back((num >> 24) & 0xFF);
                    current_blob_offset += 4;
                }

                // 3. CHUNK FLUSH SAFETY
                if (n % FLUSH_CHUNK == 0 || n == RUN_LIMIT) {
                    ofstream out_idx(FILE_INDEX, ios::binary | ios::app);
                    out_idx.write(reinterpret_cast<const char*>(buf_index.data()), buf_index.size());
                    out_idx.close();

                    ofstream out_blob(FILE_BLOB, ios::binary | ios::app);
                    out_blob.write(reinterpret_cast<const char*>(buf_blob.data()), buf_blob.size());
                    out_blob.close();

                    buf_index.clear(); 
                    buf_blob.clear();
                    
                    auto batch_end = chrono::high_resolution_clock::now();
                    chrono::duration<double> diff = batch_end - batch_start;
                    
                    cout << ">>> FLUSHED up to N=" << n 
                         << " | Blob: " << current_blob_offset / 1024 << " KB"
                         << " | Time: " << fixed << setprecision(2) << diff.count() << "s <<<" << endl;
                         
                    batch_start = chrono::high_resolution_clock::now(); 
                }
            }
        }
    }

    // SAFE EXIT DUMP
    if (!buf_index.empty()) {
        ofstream out_idx(FILE_INDEX, ios::binary | ios::app);
        out_idx.write(reinterpret_cast<const char*>(buf_index.data()), buf_index.size());
        ofstream out_blob(FILE_BLOB, ios::binary | ios::app);
        out_blob.write(reinterpret_cast<const char*>(buf_blob.data()), buf_blob.size());
    }
    
    cout << "\n[SAFE EXIT] Engine halted cleanly at N=" << last_processed_n << endl;
    return 0;
}
