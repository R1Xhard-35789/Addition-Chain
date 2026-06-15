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
// The upgraded Blueprint: 1 byte for length, 160 bytes for the exact sequence
struct Blueprint {
    uint8_t steps;
    uint32_t sequence[40]; 
};

const string LENGTHS_FILE = "master_lengths.bin";
const string SEQUENCES_FILE = "master_sequences.bin"; // Upgraded from parents.bin
const string LOG_FILE = "verification_log.txt";
const string OEIS_FILE = "b003313.txt";

// ==========================================
// 2. RAM CACHING & FILE I/O
// ==========================================
// Loads the entire OEIS text file into RAM once at startup
vector<uint8_t> load_oeis_cache() {
    vector<uint8_t> cache(100001, 0); // Pre-allocate up to N=100,000
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

int get_resume_target() {
    ifstream infile(LENGTHS_FILE, ios::binary | ios::ate);
    if (!infile.is_open()) {
        // Initialize lengths file (indices 0 and 1)
        ofstream out_lengths(LENGTHS_FILE, ios::binary);
        uint8_t dummy_len[2] = {0, 0};
        out_lengths.write(reinterpret_cast<char*>(dummy_len), 2);
        
        // Initialize sequences file (indices 0 and 1 -> 2 * 40 * 4 bytes)
        ofstream out_seqs(SEQUENCES_FILE, ios::binary);
        uint32_t dummy_seqs[80] = {0}; 
        out_seqs.write(reinterpret_cast<char*>(dummy_seqs), 320); 
        
        return 2; 
    }
    // The byte size of the 1-byte lengths file equals the total numbers processed!
    return infile.tellg(); 
}

void append_to_binary(const vector<Blueprint>& chunk) {
    vector<uint8_t> lengths(chunk.size());
    vector<uint32_t> sequences(chunk.size() * 40);

    for (size_t i = 0; i < chunk.size(); ++i) {
        lengths[i] = chunk[i].steps;
        for (int j = 0; j < 40; ++j) {
            sequences[i * 40 + j] = chunk[i].sequence[j];
        }
    }

    ofstream out_lengths(LENGTHS_FILE, ios::binary | ios::app);
    out_lengths.write(reinterpret_cast<const char*>(lengths.data()), lengths.size());

    ofstream out_seqs(SEQUENCES_FILE, ios::binary | ios::app);
    out_seqs.write(reinterpret_cast<const char*>(sequences.data()), sequences.size() * sizeof(uint32_t));
}

void verify_and_log(int start_n, int end_n, const vector<Blueprint>& chunk, double elapsed_time, const vector<uint8_t>& oeis_cache) {
    ofstream log_file(LOG_FILE, ios::app); 
    
    if (start_n > 100000) {
        log_file << "Chunk [" << start_n << " to " << end_n << "] | Time: " << elapsed_time << "s | UNVERIFIED (Exceeds OEIS limit)\n";
        return;
    }
    
    int errors = 0;
    int verified_count = 0;

    for (int i = 0; i < chunk.size(); ++i) {
        int current_n = start_n + i;
        if (current_n <= 100000 && oeis_cache[current_n] > 0) {
            if (chunk[i].steps != oeis_cache[current_n]) {
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
    
    // Geometric Pruning
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

Blueprint find_optimal_steps(int target) {
    Blueprint bp;
    bp.steps = 0;
    memset(bp.sequence, 0, sizeof(bp.sequence)); // Zero out the array
    
    if (target <= 1) return bp;
    
    int depth = 0;
    int temp = target;
    while (temp > 1) { depth++; temp /= 2; }

    while (true) {
        vector<int> chain = {1};
        if (search_general_chain(target, 0, depth, chain)) {
            bp.steps = (uint8_t)depth;
            
            // DIRECT MEMORY COPY: We save the exact path to prevent Prefix-Optimality bugs.
            for (size_t i = 0; i < chain.size() && i < 40; ++i) {
                bp.sequence[i] = chain[i];
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
    omp_set_num_threads(6); 

    cout << "=========================================" << endl;
    cout << " V2 TALLY-TABLE ENGINE STARTED           " << endl;
    cout << "=========================================" << endl;
    
    // Load OEIS to RAM before starting the OpenMP loop
    vector<uint8_t> oeis_cache = load_oeis_cache();
    
    int current_n = get_resume_target();
    cout << "Resuming at N = " << current_n << "\nPress Ctrl+C at any time to safely stop." << endl;
    
    while (true) {
        int chunk_size;
        if (current_n < 20000) chunk_size = 100;
        else if (current_n < 200000) chunk_size = 10;
        else chunk_size = 1;

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

        append_to_binary(memory_buffer);
        verify_and_log(current_n, end_n, memory_buffer, elapsed.count(), oeis_cache);

        cout << " Done in " << elapsed.count() << "s" << flush;
        current_n += chunk_size;
    }

    return 0;
}
