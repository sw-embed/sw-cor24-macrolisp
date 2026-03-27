#!/usr/bin/env python3
"""Extract TML snapshot from UART hex output, convert to binary."""
import sys

data = open(sys.argv[1], 'r').read()
idx = data.find('TML')
if idx < 0:
    print('ERROR: TML magic not found in output', file=sys.stderr)
    sys.exit(1)

# Everything after 'TML' is hex-encoded
hex_str = data[idx + 3:].strip()
# Remove any non-hex characters (prompts, newlines, etc.)
hex_clean = ''.join(c for c in hex_str if c in '0123456789abcdef')

binary = bytes.fromhex(hex_clean)
out = sys.argv[2] if len(sys.argv) > 2 else sys.argv[1].replace('.raw', '.snap')

# Write with TML magic prefix (binary)
with open(out, 'wb') as f:
    f.write(b'TML')
    f.write(binary)

print(f'Snapshot: {len(binary) + 3} bytes ({len(binary)} data + 3 magic)')
