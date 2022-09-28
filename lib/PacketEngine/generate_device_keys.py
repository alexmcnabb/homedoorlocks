import sys
import os
import secrets

# Pass one argument, the device id to remake keys for. If it already exists the keys are replaced, otherwise they are added

output_file_path = "lib/PacketEngine/device_keys.h"
device_id = int(sys.argv[1])
print(f"Remaking keys for ID {device_id}")
new_base_hash = ", ".join("0x{:02X}".format(c) for c in secrets.token_bytes(64))
base_hash_line = f"#define BASE_HASH_{device_id:02} {{{new_base_hash}}}\n"
new_hash_token = "".join("{:08X}".format(secrets.randbits(32)))
hash_token_line = f"#define HASH_TOKEN_{device_id:02} 0x{new_hash_token}\n"

output_file = ""
inserted = False
for in_line in open(output_file_path).readlines():
    if f"#define BASE_HASH_{device_id:02X}"  in in_line:
        output_file += base_hash_line
        inserted = True
    elif f"#define HASH_TOKEN_{device_id:02X}" in in_line:
        output_file += hash_token_line
    elif in_line == "// --EndOfDeviceKeyDefs--\n" and not inserted:
        output_file += base_hash_line + hash_token_line + "\n" + in_line
    else:
        output_file += in_line

with open(output_file_path, 'w') as f:
    f.write(output_file)