#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <cstdint>

using namespace std;

// ==========================================
// 1. DATA STRUCTURES & CONSTANTS
// ==========================================
struct Blueprint {
    uint8_t steps;
    uint32_t parentA;
    uint32_t parentB;
};

const string LENGTHS_FILE = "master_lengths.bin";
const string PARENTS_FILE = "master_parents.bin";
const string LOG_FILE = "verification_log.txt";
const string OEIS_FILE = "b003313.txt";

// ==========================================
// 2. FILE I/O & RESUME LOGIC
// ==========================================
int get_resume_target() {
    ifstream infile(LENGTHS_FILE, ios::binary | ios::ate);
    if (!infile.is_open()) {
        ofstream out_lengths(LENGTHS_FILE, ios::binary);
        uint8_t dummy_len[2] = {0, 0};
        out_lengths.write(reinterpret_cast<char*>(dummy_len), 2);
        
        ofstream out_parents(PARENTS_FILE, ios::binary);
        uint32_t dummy_parents[4] = {0, 0, 0, 0}; 
        out_parents.write(reinterpret_cast<char*>(dummy_parents), 16); 
        
        return 2; 
    }
    // The byte size of the 1-byte lengths file equals the total numbers processed!
    return infile.tellg(); 
}

void append_to_binary(const vector<Blueprint>& chunk) {
    vector<uint8_t> lengths(chunk.size());
    vector<uint32_t> parents(chunk.size() * 2);

    for (size_t i = 0; i < chunk.size(); ++i) {
        lengths[i] = chunk[i].steps;
        parents[i*2] = chunk[i].parentA;
        parents[i*2 + 1] = chunk[i].parentB;
    }

    ofstream out_lengths(LENGTHS_FILE, ios::binary | ios::app);
    out_lengths.write(reinterpret_cast<const char*>(lengths.data()), lengths.size());

    ofstream out_parents(PARENTS_FILE, ios::binary | ios::app);
    out_parents.write(reinterpret_cast<const char*>(parents.data()), parents.size() * sizeof(uint32_t));
}

// Change the function definition to accept the elapsed time
void verify_and_log(int start_n, int end_n, const vector<Blueprint>& chunk, double elapsed_time) {
    ofstream log_file(LOG_FILE, ios::app); 
    
    // THE 100K FAILSAFE
    if (start_n > 100000) {
        log_file << "Chunk [" << start_n << " to " << end_n << "] | UNVERIFIED (Exceeds OEIS limit)\n";
        return;
    }
    
    ifstream gold_file(OEIS_FILE);
    int errors = 0;
    int verified_count = 0;

    if (gold_file.is_open()) {
        string line;
        while (getline(gold_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            stringstream ss(line);
            int n, gold_steps;
            if (!(ss >> n >> gold_steps)) continue;

            if (n < start_n) continue; 
            if (n > end_n) break;      

            int chunk_index = n - start_n;
            if (chunk[chunk_index].steps != gold_steps) {
                errors++;
            }
            verified_count++;
        }
    } else {
        log_file << "WARNING: " << OEIS_FILE << " not found. Skipping verification.\n";
        return;
    }

    // Add the elapsed_time to the text file output
    log_file << "Chunk [" << start_n << " to " << end_n << "] | Time: " << elapsed_time << "s | ";
    if (errors == 0) {
        log_file << "SUCCESS (0 Errors out of " << verified_count << " verified)\n";
    } else {
        log_file << "FAILED (" << errors << " Errors detected)\n";
    }
}

// ==========================================
// 3. THE CORE SEARCH ENGINE (KISS - BRUTE FORCE)
// ==========================================
bool search_general_chain(int target, int current_depth, int max_depth, vector<int>& chain) {
    if (chain.back() == target) return true;
    if (current_depth == max_depth) return false;
    
    // Mathematical pruning: If we double the current number for the rest of the depth 
    // and still can't hit the target, it's a mathematically dead branch.
    if ((chain.back() << (max_depth - current_depth)) < target) return false;

    // Pure, raw C++ iteration. No sorting, no modulo overhead.
    for (int i = chain.size() - 1; i >= 0; --i) {
        for (int j = i; j >= 0; --j) {
            int next_val = chain[i] + chain[j];
            if (next_val > chain.back() && next_val <= target) {
                chain.push_back(next_val);
                
                if (search_general_chain(target, current_depth + 1, max_depth, chain)) {
                    // We DO NOT pop_back if we find the true chain. 
                    // This leaves the winning sequence perfectly intact in memory.
                    return true; 
                }
                chain.pop_back(); // Only pop if it was a dead end
            }
        }
    }
    return false;
}

Blueprint find_optimal_steps(int target) {
    Blueprint bp = {0, 0, 0};
    if (target <= 1) return bp;
    
    int depth = 0;
    int temp = target;
    while (temp > 1) { depth++; temp /= 2; }

    while (true) {
        vector<int> chain = {1};
        if (search_general_chain(target, 0, depth, chain)) {
            bp.steps = (uint8_t)depth;
            
            // The chain array now holds the winning path!
            // We just quickly find which two numbers added up to our target.
            bool found = false;
            for (size_t i = 0; i < chain.size() - 1 && !found; ++i) {
                for (size_t j = i; j < chain.size() - 1 && !found; ++j) {
                    if (chain[i] + chain[j] == target) {
                        bp.parentA = chain[i];
                        bp.parentB = chain[j];
                        found = true;
                    }
                }
            }
            return bp;
        }
        depth++;
    }
}

// ==========================================
// 4. MAIN EXECUTION LOOP
// ==========================================
int main() {
    // Lock to exact physical cores to eliminate OS scheduling overhead
    omp_set_num_threads(6); 

    cout << "=========================================" << endl;
    cout << " RELATIONAL GENERAL CHAIN ENGINE STARTED " << endl;
    cout << "=========================================" << endl;
    cout << "Press Ctrl+C at any time to safely stop." << endl;

    int current_n = get_resume_target();
    
    while (true) {
        // Updated Dynamic Chunk Sizing
        int chunk_size;
        if (current_n < 20000) {
            chunk_size = 100;
        } else if (current_n < 200000) {
            chunk_size = 10;
        } else {
            chunk_size = 1;
        }

        int end_n = current_n + chunk_size - 1;
        vector<Blueprint> memory_buffer(chunk_size);

        cout << "\nProcessing N=" << current_n << " to " << end_n << "..." << flush;
        auto start = chrono::high_resolution_clock::now();

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < chunk_size; ++i) {
            memory_buffer[i] = find_optimal_steps(current_n + i);
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;

        // Append results to the dual-file system
        append_to_binary(memory_buffer);
        
        // Pass the elapsed time into the verification logger
        verify_and_log(current_n, end_n, memory_buffer, elapsed.count());

        cout << " Done in " << elapsed.count() << "s | Logged to txt." << flush;

        current_n += chunk_size;
    }

    return 0;
}
