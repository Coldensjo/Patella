"""Compare the calls of each reconstructed function with its clean pseudocode.

For every "// 0x<address>" function in a src file, counts the calls to named
functions (names.tsv names and FUN_00xxxxxx) in the function body and in
decomp/clean/<File>.c, and prints the functions whose counts differ. Calls in
catch blocks, the error macros and inlined helpers make some differences
expected; the list is a review aid, not a verdict.

Usage: python -I tools/callcheck.py src/Communication.cpp
"""
import csv
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
names = {}
for r in csv.DictReader(open(ROOT / 'names.tsv', encoding='utf-8'), delimiter='\t'):
	names[r['name']] = int(r['address'], 16)
CALL = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
IGNORE = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'TASSERT', 'TASSERT1', 'TASSERT2', 'TERROR',
	'TERROR1', 'TERROR2', 'TEND1', 'TEND2', 'TASSERT_FAILED', 'TRETHROW', 'TMYEXCEPTION', 'In', 'catch'}


def calls(body):
	c = Counter()
	for m in CALL.finditer(body):
		n = m.group(1)
		if n in IGNORE:
			continue
		if n.startswith('FUN_00'):
			c[int(n[6:], 16)] += 1
		elif n in names:
			c[names[n]] += 1
	return c


src = Path(sys.argv[1])
text = src.read_text(encoding='utf-8')
own = {}
parts = re.split(r'^// (0x[0-9a-f]+)\b.*$', text, flags=re.M)
for i in range(1, len(parts), 2):
	body = parts[i + 1]
	own[int(parts[i], 16)] = body[body.index('{'):] if '{' in body else body
clean = (ROOT / 'decomp' / 'clean' / (src.stem + '.c')).read_text(encoding='utf-8')
dec = {}
parts = re.split(r'^//// (0x[0-9a-f]+)  size \d+  (\w+).*$', clean, flags=re.M)
for i in range(1, len(parts), 3):
	if parts[i + 1] == 'game':
		body = parts[i + 2]
		dec[int(parts[i], 16)] = body[body.index('{'):] if '{' in body else body
bad = 0
for a, body in sorted(own.items()):
	if a not in dec:
		continue
	mine, theirs = calls(body), calls(dec[a])
	theirs.pop(a, None)
	mine.pop(a, None)
	if mine != theirs:
		bad += 1
		diff = {hex(k): (mine.get(k, 0), theirs.get(k, 0)) for k in set(mine) | set(theirs) if mine.get(k, 0) != theirs.get(k, 0)}
		print(hex(a), 'ours/decomp:', diff)
print(bad, 'of', len(own), 'functions differ')
