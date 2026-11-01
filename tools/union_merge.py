"""Resolve git conflict hunks by keeping both sides (ours, then theirs).

For append-only files (names.tsv, file-ranges.tsv, notes) where two
branches added lines at the same place. Review the result afterwards.

Usage: python -I tools/union_merge.py file [...]
"""
import sys
from pathlib import Path

for name in sys.argv[1:]:
	p = Path(name)
	out = []
	state = None
	for line in p.read_text(encoding='utf-8').split('\n'):
		if line.startswith('<<<<<<< '):
			state = 'ours'
		elif line.startswith('||||||| ') and state:
			state = 'base'
		elif line == '=======' and state:
			state = 'theirs'
		elif line.startswith('>>>>>>> ') and state:
			state = None
		elif state != 'base':
			out.append(line)
	p.write_text('\n'.join(out), encoding='utf-8', newline='')
	print(name)
