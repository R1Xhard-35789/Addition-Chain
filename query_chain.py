import sys
import struct
import json
import os

def get_chain(target):
    LEN_FILE = "master_lengths.bin"
    PAR_FILE = "master_parents.bin"

    if not os.path.exists(LEN_FILE) or not os.path.exists(PAR_FILE):
        return {"error": "Database files missing."}

    max_indexed = os.path.getsize(LEN_FILE) - 1
    if target > max_indexed or target < 2:
        return {"error": f"Target {target} exceeds indexed registry limit ({max_indexed})."}

    with open(LEN_FILE, "rb") as f_len, open(PAR_FILE, "rb") as f_par:
        f_len.seek(target)
        steps = struct.unpack("B", f_len.read(1))[0]

        chain_set = {1}
        
        def trace_parents(n):
            if n <= 1: return
            offset = 16 + (n - 2) * 8
            f_par.seek(offset)
            pA, pB = struct.unpack("<II", f_par.read(8))
            chain_set.add(n)
            if pA > 1 and pA not in chain_set: trace_parents(pA)
            if pB > 1 and pB not in chain_set: trace_parents(pB)

        trace_parents(target)
        
        full_chain = sorted(list(chain_set))
        ancestors = [str(x) for x in full_chain if x != target]
        
        return {
            "target": target,
            "optimal_steps": steps,
            "synthesis": " → ".join(ancestors),
            "registry_limit": max_indexed
        }

if __name__ == "__main__":
    try:
        print(json.dumps(get_chain(int(sys.argv[1]))))
    except Exception as e:
        print(json.dumps({"error": str(e)}))
