import sys
import struct
import json
import os

def get_chain(target):
    # Ensure this path matches where your new master_ledger.bin is located!
    LEDGER_FILE = "/var/www/html/tools/master_ledger.bin" 
    
    # Fallback for local testing if the absolute path fails
    if not os.path.exists(LEDGER_FILE):
        LEDGER_FILE = "master_ledger.bin" 

    if not os.path.exists(LEDGER_FILE):
        return {"error": "New 32-bit Database file (master_ledger.bin) missing."}

    # File size divided by 4 gives the maximum indexed N
    max_indexed = os.path.getsize(LEDGER_FILE) // 4 - 1
    
    if target > max_indexed or target < 2:
        return {"error": f"Target {target} exceeds registry limit ({max_indexed})."}

    with open(LEDGER_FILE, "rb") as f:
        # Seek exactly to N's 4-byte block
        f.seek(target * 4)
        raw_data = f.read(4)
        
        if len(raw_data) < 4:
            return {"error": "Corrupted read."}
            
        encoded_32bit = struct.unpack("<I", raw_data)[0]
        
        # Unpack the 32-bit architecture
        steps = encoded_32bit & 0x7F
        flags = (encoded_32bit >> 7) & 0x0F
        
        # Decode Metadata Flags
        is_doubling = bool(flags & (1 << 0))
        is_small_step = bool(flags & (1 << 1))
        is_prime = bool(flags & (1 << 2))
        is_non_star = bool(flags & (1 << 3))

        # Reconstruct the sequence path by walking backwards
        chain = []
        current = target
        while current > 1:
            chain.append(str(current))
            f.seek(current * 4)
            prev_data = struct.unpack("<I", f.read(4))[0]
            parentA = (prev_data >> 11) & 0x1FFFFF
            
            # [!] AUTO-HEAL: Kills the "Ghosting Bug" 
            # If a parent is >= current, it's a migrated ghost. Break the loop.
            if parentA >= current:
                break
                
            current = parentA
            
        if "1" not in chain:
            chain.append("1")
            
        chain.reverse() # Flip it to read 1 -> 2 -> 4 ... -> N

        # [!] VISUAL SANITIZER: Double-check to remove any side-by-side duplicates
        deduped_chain = []
        for x in chain:
            if not deduped_chain or deduped_chain[-1] != x:
                deduped_chain.append(x)

        return {
            "target": target,
            "optimal_steps": steps,
            "synthesis": " → ".join(deduped_chain),
            "registry_limit": max_indexed,
            "metadata": {
                "is_prime": is_prime,
                "is_doubling": is_doubling,
                "is_increment": is_small_step,
                "is_non_star_anomaly": is_non_star
            }
        }

if __name__ == "__main__":
    if len(sys.argv) > 1:
        try:
            target_n = int(sys.argv[1])
            print(json.dumps(get_chain(target_n)))
        except Exception as e:
            print(json.dumps({"error": str(e)}))
    else:
        print(json.dumps({"error": "No target provided."}))
