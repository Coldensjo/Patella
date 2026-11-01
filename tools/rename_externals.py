"""Replace FUN_00xxxxxx names of reconstructed functions in a source file.

For every FUN_00xxxxxx in the given files whose address lies in a range of
file-ranges.tsv or belongs to a reconstructed src/<File>.cpp, and that has a
name in names.tsv, the call is renamed and its declaration line is removed
from src/external.h (the real header declares it).

Usage: python -I tools/rename_externals.py src/Communication.cpp [...]
"""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
names = {}
for r in csv.DictReader(open(ROOT / 'names.tsv', encoding='utf-8'), delimiter='\t'):
	if not r['name'].startswith('FUN_'):
		names[int(r['address'], 16)] = r['name']

# Files that have a reconstruction in src/ and their address ranges.
done = {p.stem + '.cpp' for p in (ROOT / 'src').glob('*.cpp')}
ranges = []
for r in csv.DictReader(open(ROOT / 'symbols.tsv', encoding='utf-8'), delimiter='\t'):
	if r['file'] in done and r['kind'] == 'game':
		ranges.append(int(r['address'], 16))
owned = set(ranges)

FUN = re.compile(r'\bFUN_00([0-9a-f]{6})\b')
ext_path = ROOT / 'src' / 'external.h'
ext = ext_path.read_text(encoding='utf-8').splitlines()
renamed = set()
for f in sys.argv[1:]:
	p = Path(f)
	text = p.read_text(encoding='utf-8')

	def sub(m):
		a = int(m.group(1), 16)
		if a in owned and a in names:
			renamed.add(a)
			return names[a]
		return m.group(0)

	p.write_text(FUN.sub(sub, text), encoding='utf-8', newline='\n')
keep = []
for line in ext:
	m = FUN.search(line)
	if m and int(m.group(1), 16) in renamed and '(' in line:
		continue
	keep.append(line)
ext_path.write_text('\n'.join(keep) + '\n', encoding='utf-8', newline='\n')
for a in sorted(renamed):
	print(hex(a), names[a])
