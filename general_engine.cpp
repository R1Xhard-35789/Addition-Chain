#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <cstdint>
#include <cstring>

using namespace std;

// ==========================================
// 1. DATA STRUCTURES & CONSTANTS
// ==========================================
// The 32-Bit Atomic Ledger Entry
struct PackedBlueprint {
    uint32_t data;
};

const string LEDGER_FILE = "master_ledger.bin";
const string LOG_FILE = "verification_log.txt";
const string OEIS_FILE = "b003313.txt";

// Absolute maximum target based on a 22-bit Delta. 
// Max 22-bit value is 4,194,303. Since Delta = N - parentA, and parentA >= N/2, 
// the maximum safe N is 2 * 4,194,303 = 8,388,606.
const int MAX_SUPPORTED_N = 8388606; 

// Fast Primality Test for the Flag
bool is_prime_n(uint32_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (uint32_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

// ==========================================
// 2. RAM CACHING & FILE I/O
// ==========================================
vector<uint8_t> load_oeis_cache() {
    vector<uint8_t> cache(100001, 0); 
    ifstream gold_file(OEIS_FILE);
    
    if (gold_file.is_open()) {
        string line;
        while (getline(gold_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            stringstream ss(line);
            int n, steps;
            if (ss >> n >> steps && n <= 100000) {
                cache[n] = steps;
            }
        }
        cout << "[System] OEIS Database loaded into RAM." << endl;
    } else {
        cout << "[Warning] " << OEIS_FILE << " not found. Verification disabled." << endl;
    }
    return cache;
}

// UPDATED: Now accepts a dynamic filename
int get_resume_target(const string& filename, int base_start = 2) {
    ifstream infile(filename, ios::binary | ios::ate);
    if (!infile.is_open()) {
        ofstream out_ledger(filename, ios::binary);
        // If this is the master ledger (base_start == 2), write the N=0 and N=1 dummy bytes
        if (base_start == 2) {
            uint32_t dummies[2] = {0, 0};
            out_ledger.write(reinterpret_cast<char*>(dummies), 8);
        }
        return base_start;
    }
    // Calculate how many integers are already in this specific file
    int mapped_count = infile.tellg() / sizeof(uint32_t); 
    
    // If it's a chunk, it doesn't have the N=0/1 offset, so we just add the count to the base start
    if (base_start > 2) {
        return base_start + mapped_count;
    }
    return mapped_count; 
}

// UPDATED: Now accepts a dynamic filename
void append_to_binary(const vector<PackedBlueprint>& chunk, const string& filename) {
    ofstream out_ledger(filename, ios::binary | ios::app);
    out_ledger.write(reinterpret_cast<const char*>(chunk.data()), chunk.size() * sizeof(PackedBlueprint));
}

void verify_and_log(int start_n, int end_n, const vector<PackedBlueprint>& chunk, double elapsed_time, const vector<uint8_t>& oeis_cache) {
    ofstream log_file(LOG_FILE, ios::app); 
    
    if (start_n > 100000) {
        log_file << "Chunk [" << start_n << " to " << end_n << "] | Time: " << elapsed_time << "s | UNVERIFIED (Exceeds OEIS limit)\n";
        return;
    }
    
    int errors = 0;
    int verified_count = 0;

    for (size_t i = 0; i < chunk.size(); ++i) {
        int current_n = start_n + i;
        if (current_n <= 100000 && oeis_cache[current_n] > 0) {
            // Unpack just the step count (bottom 6 bits now!) to verify
            uint8_t steps = chunk[i].data & 0x3F;
            if (steps != oeis_cache[current_n]) {
                errors++;
            }
            verified_count++;
        }
    }

    log_file << "Chunk [" << start_n << " to " << end_n << "] | Time: " << elapsed_time << "s | ";
    if (errors == 0 && verified_count > 0) {
        log_file << "SUCCESS (0 Errors out of " << verified_count << " verified)\n";
    } else if (errors > 0) {
        log_file << "FAILED (" << errors << " Errors detected)\n";
    } else {
        log_file << "SKIPPED (No OEIS data in RAM)\n";
    }
}

// ==========================================
// 3. THE CORE SEARCH ENGINE
// ==========================================
bool search_general_chain(int target, int current_depth, int max_depth, vector<int>& chain) {
    if (chain.back() == target) return true;
    if (current_depth == max_depth) return false;
    
    if ((chain.back() << (max_depth - current_depth)) < target) return false;

    for (int i = chain.size() - 1; i >= 0; --i) {
        for (int j = i; j >= 0; --j) {
            int next_val = chain[i] + chain[j];
            if (next_val > chain.back() && next_val <= target) {
                chain.push_back(next_val);
                
                if (search_general_chain(target, current_depth + 1, max_depth, chain)) {
                    return true; 
                }
                chain.pop_back(); 
            }
        }
    }
    return false;
}

PackedBlueprint find_optimal_steps(int target) {
    if (target <= 1) return {0};
    
    int depth = 0;
    int temp = target;
    while (temp > 1) { depth++; temp /= 2; }

    vector<int> final_chain;
    
    // 1. Run the Grind
    while (true) {
        vector<int> chain = {1};
        if (search_general_chain(target, 0, depth, chain)) {
            final_chain = chain;
            break;
        }
        depth++;
    }

    // 2. Mathematically Deduce the Parents
    uint32_t parentA = 1;
    uint32_t parentB = 1;
    bool is_non_star = true;

    if (final_chain.size() >= 2) {
        uint32_t immediate_pred = final_chain[final_chain.size() - 2];
        uint32_t needed_b = target - immediate_pred;

        // Check if it's a standard Star Step
        for (int x : final_chain) {
            if (x == needed_b) {
                parentA = immediate_pred;
                parentB = needed_b;
                is_non_star = false;
                break;
            }
        }

        // If not a star step, deep-search the chain for the rare parent combo
        if (is_non_star) {
            bool found = false;
            for (int i = final_chain.size() - 2; i >= 0; --i) {
                for (int j = i; j >= 0; --j) {
                    if (final_chain[i] + final_chain[j] == target) {
                        parentA = final_chain[i];
                        parentB = final_chain[j];
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) { // Safety fallback
                parentA = immediate_pred;
                parentB = target - immediate_pred;
            }
        }
    }

    // 3. Set the 4 Metadata Flags
    uint8_t flags = 0;
    if (parentA == parentB) flags |= (1 << 0); // Bit 0: Doubling
    if (parentB == 1) flags |= (1 << 1);       // Bit 1: Small Step
    if (is_prime_n(target)) flags |= (1 << 2); // Bit 2: Prime
    if (is_non_star) flags |= (1 << 3);        // Bit 3: Non-Star Anomaly

    // 4. Pack into 32-bit Architecture using the 22-bit Delta method
    uint32_t delta = target - parentA;

    // Hard Safety Check to ensure the delta never overflows 22 bits
    if (delta > 0x3FFFFF) {
        #pragma omp critical
        cout << "\n[CRITICAL WARNING] 22-Bit Delta Overflow at N=" << target << "! Value corrupted." << endl;
    }

    // 6 bits (steps) | 4 bits (flags) | 22 bits (Delta)
    uint8_t steps = (uint8_t)depth;
    
    // Steps takes 6 bits (0x3F), Flags shift by 6, Delta shifts by 10 (6+4)
    uint32_t encoded = (steps & 0x3F) | ((flags & 0x0F) << 6) | ((delta & 0x3FFFFF) << 10);
    
    return { encoded };
}

// ==========================================
// 4. MAIN EXECUTION LOOP
// ==========================================
int main(int argc, char* argv[]) {
    omp_set_num_threads(6); 

    cout << "=========================================" << endl;
    cout << " 32-BIT ATOMIC ENGINE STARTED            " << endl;
    cout << " 22-BIT DELTA MODE | MAX LIMIT: 8,388,606" << endl;
    cout << "=========================================" << endl;
    
    vector<uint8_t> oeis_cache = load_oeis_cache();
    
    int current_n;
    int end_limit = -1;
    string active_file = LEDGER_FILE;

    // CLI ARGUMENT PARSING
    if (argc == 3) {
        int requested_start = stoi(argv[1]);
        end_limit = stoi(argv[2]);
        active_file = "chunk_" + to_string(requested_start) + "_" + to_string(end_limit) + ".bin";
        
        current_n = get_resume_target(active_file, requested_start);
        
        cout << "[DISTRIBUTED MODE] Target Range: " << requested_start << " to " << end_limit << endl;
        cout << "[!] Saving isolated payload to: " << active_file << endl;
        cout << "Resuming chunk at N = " << current_n << endl;
        
    } else {
        current_n = get_resume_target(active_file, 2);
        cout << "[STANDARD MODE] Active Ledger: " << active_file << endl;
        cout << "Resuming at N = " << current_n << "\nPress Ctrl+C at any time to safely stop." << endl;
    }

    // Safety checks against the architectural limit
    if (current_n > MAX_SUPPORTED_N) {
        cout << "[ABORT] System has reached max 22-Bit Delta capacity (" << MAX_SUPPORTED_N << ")." << endl;
        return 0;
    }
    if (end_limit != -1 && end_limit > MAX_SUPPORTED_N) {
        cout << "[WARNING] End limit adjusted to safe architecture maximum: " << MAX_SUPPORTED_N << endl;
        end_limit = MAX_SUPPORTED_N;
    }

    if (end_limit != -1 && current_n > end_limit) {
        cout << "\n[COMPLETED] This chunk has already reached its target limit." << endl;
        return 0;
    }
    
    while (true) {
        int chunk_size;
        // Adjusted batch sizing to hit batches of 6 threads exactly when it gets hard
        if (current_n < 20000) chunk_size = 100;
        else if (current_n < 30000) chunk_size = 10;
        else chunk_size = 6;

        // Ensure we don't overshoot the end limit in Distributed Mode
        if (end_limit != -1 && (current_n + chunk_size - 1) > end_limit) {
            chunk_size = (end_limit - current_n) + 1;
        }

        // Ensure we don't overshoot the absolute architecture limit
        if (current_n + chunk_size - 1 > MAX_SUPPORTED_N) {
            chunk_size = (MAX_SUPPORTED_N - current_n) + 1;
        }

        int end_n = current_n + chunk_size - 1;
        vector<PackedBlueprint> memory_buffer(chunk_size);

        cout << "\nProcessing N=" << current_n << " to " << end_n << "..." << flush;
        auto start = chrono::high_resolution_clock::now();

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < chunk_size; ++i) {
            memory_buffer[i] = find_optimal_steps(current_n + i);
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;

        // Atomic write to whichever file is active (master or chunk)
        append_to_binary(memory_buffer, active_file);
        
        // Log and Verify against OEIS
        verify_and_log(current_n, end_n, memory_buffer, elapsed.count(), oeis_cache);

        cout << " Done in " << elapsed.count() << "s" << flush;
        current_n += chunk_size;

        // Stop the engine if we hit the chunk limit
        if (end_limit != -1 && current_n > end_limit) {
            cout << "\n\n[CHUNK COMPLETE] Node has successfully mapped " << argv[1] << " to " << end_limit << "." << endl;
            break;
        }
        
        // Final safety exit if ceiling is reached
        if (current_n > MAX_SUPPORTED_N) {
            cout << "\n\n[SYSTEM LIMIT REACHED] Reached N=" << MAX_SUPPORTED_N << ". Architecture limit reached." << endl;
            break;
        }
    }

    return 0;
}
