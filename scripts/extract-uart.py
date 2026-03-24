#!/usr/bin/env python3
"""Extract clean UART output from cor24-run log, stripping REPL prompts.

Usage:
  cor24-run ... | python3 extract-uart.py          # all results
  cor24-run ... | python3 extract-uart.py --last    # only last result
"""
import sys, re

last_only = '--last' in sys.argv

output = []
for line in sys.stdin:
    m = re.match(r"^\[UART TX @ \d+\] '(.*?)'", line)
    if m:
        ch = m.group(1)
        if ch == '\\n':
            output.append('\n')
        else:
            output.append(ch)

text = ''.join(output)
results = [line.strip() for line in text.split('> ') if line.strip()]

if last_only:
    if results:
        print(results[-1])
else:
    for r in results:
        print(r)
