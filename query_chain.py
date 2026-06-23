import sys
import struct
import json
import os
import math

def get_chain(target):
    INDEX_FILE = "chain_index.bin"
    BLOB_FILE  = "chain_blob.bin"
    
    # Fallback for local testing if needed
    if not os.path.exists(INDEX_FILE):
        INDEX_FILE = "/var/www/html/tools/chain_index.bin"
        BLOB_FILE  = "/var/www/html/tools/chain_blob.bin"

    if not os.path.exists(INDEX_FILE) or not os.path.exists(BLOB_FILE):
        return {"error": "Database files missing. Please run the C++ engine first."}

    # Fat Index is 12 bytes per record. 
    max_indexed = os.path.getsize(INDEX_FILE) // 12 - 1
    
    if target > max_indexed or target < 2:
        return {"error": f"Target {target} exceeds registry limit (Max: {max_indexed})."}

    try:
        # 1. READ THE FAT INDEX (12 bytes)
        with open(INDEX_FILE, "rb") as f_idx:
            f_idx.seek(target * 12)
            index_data = f_idx.read(12)
            
            # Unpack: Q (uint64 offset), H (uint16 length), H (uint16 flags)
            blob_offset, chain_length, flags = struct.unpack("<QHH", index_data)

        # 2. READ THE PURE BLOB
        with open(BLOB_FILE, "rb") as f_blob:
            f_blob.seek(blob_offset)
            nodes_count = struct.unpack("<H", f_blob.read(2))[0]
            chain_data = struct.unpack(f"<{nodes_count}I", f_blob.read(nodes_count * 4))
            chain_str = " → ".join(map(str, chain_data))

        # 3. CALCULATE DEFECT
        # Defect = Steps (chain_length) - log2(Target)
        defect = chain_length - math.floor(math.log2(target))

        # 4. DECODE 16-BIT ENRICHMENT FLAGS (Fixed assignment here)
        metadata = {
            "is_prime": bool(flags & (1 << 0)),
            "is_safe_prime": bool(flags & (1 << 1)),
            "high_hamming": bool(flags & (1 << 2)),
            "is_peak": bool(flags & (1 << 3)),
            "is_fib_step": bool(flags & (1 << 4)),
            "uses_factor": bool(flags & (1 << 5)),
            "is_square": bool(flags & (1 << 6)),
            "is_square_free": bool(flags & (1 << 7)),
            "is_triangular": bool(flags & (1 << 8)),
            "is_power_of_2": bool(flags & (1 << 9)),
            "is_mersenne": bool(flags & (1 << 10)),
            "is_palindrome": bool(flags & (1 << 11)),
            "heavy_divisors": bool(flags & (1 << 12)),
            "is_small_base": bool(flags & (1 << 13))
        }

    except Exception as e:
        return {"error": f"Database Read Error: {str(e)}"}

    return {
        "target": target,
        "optimal_steps": chain_length,
        "defect": defect,
        "synthesis": chain_str,
        "metadata": metadata,
        "registry_limit": max_indexed
    }

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "No target specified."}))
        sys.exit(1)
    target_val = int(sys.argv[1])
    print(json.dumps(get_chain(target_val), indent=4))
