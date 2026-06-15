import sys
import struct
import json
import os

def get_chain(target):
    LEN_FILE = "master_lengths.bin"
    SEQ_FILE = "master_sequences.bin"

    if not os.path.exists(LEN_FILE) or not os.path.exists(SEQ_FILE):
        return {"error": "Database files missing."}

    max_indexed = os.path.getsize(LEN_FILE) - 1
    if target > max_indexed or target < 2:
        return {"error": f"Target {target} exceeds indexed registry limit ({max_indexed})."}

    with open(LEN_FILE, "rb") as f_len, open(SEQ_FILE, "rb") as f_seq:
        # 1. Get length
        f_len.seek(target)
        steps = struct.unpack("B", f_len.read(1))[0]

        # 2. Get the exact sequence array
        # Offset: 2 dummy numbers * 160 bytes = 320 byte header. Then target * 160.
        offset = target * 160
        f_seq.seek(offset)
        
        # Read 40 uint32_t integers
        raw_data = f_seq.read(160)
        sequence_array = struct.unpack("<40I", raw_data)
        
        # Filter out the zero-padding
        clean_chain = [str(x) for x in sequence_array if x != 0]

        return {
            "target": target,
            "optimal_steps": steps,
            "synthesis": " → ".join(clean_chain),
            "registry_limit": max_indexed
        }

if __name__ == "__main__":
    try:
        print(json.dumps(get_chain(int(sys.argv[1]))))
    except Exception as e:
        print(json.dumps({"error": str(e)}))
