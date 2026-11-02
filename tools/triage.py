"""Map decompiled Tibia.exe functions to original source files.

Reads decomp/raw (REA decompile-all output) and writes:
	symbols.tsv          one row per function: address, size, name, kind, file, how, assert lines
	notes/source-map.md  per-file summary, address ranges, unresolved gaps and the header code
	                     not yet in src/ (src/ naming the address counts as written)

kind:  game        game code
       catch       body of a catch block in game code; it belongs to the function containing the try
       library     runtime, STL and MFC code after the last game file
       eh-funclet  compiler-generated unwind cleanup (Unwind@)
how:   assert      the function names exactly one source file in an assertion
       header      it names a .h file only (inline or template code)
       between     it lies between two assert functions of the same file
       gap         it lies between two different files; file lists both candidates
       named       Ghidra identified it as library code by name
       region      it lies in the library region after the last game file
       range       file-ranges.tsv assigns its address range to a file (settled gaps)

The linker placed the game objects in case-insensitive alphabetical order of
their file names, so a gap between files A and B can also hold a file whose
name sorts between A and B and that has no assertions.

Usage: python -I tools/triage.py [repo root]
"""
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
RAW = ROOT / 'decomp' / 'raw'

FILE_RE = re.compile(r'"([^"\n]{0,80}\.(?:cpp|h))"')
# Error builders, called as FUN(buf, LINE, ...) after the file-name string is built:
# 0x508400 for assertions, 0x5086a0 for the rethrow at the end of each catch block.
LINE_RE = re.compile(r'FUN_00(?:508400|5086a0)\(\s*[^,()]+,\s*(0x[0-9a-f]+|\d+)\s*,')
# A catch block builds one string per parameter of the enclosing function, then this marker.
STR_RE = re.compile(r'FUN_004039d0\("([^"\n]*)"\)')
MARKER = 'exception occurred'
UNWIND_RE = re.compile(r'^Unwind@')
CATCH_RE = re.compile(r'^Catch@')
GAME_KINDS = ('game', 'catch')


def base_name(path):
	return re.split(r'[\\/]', path)[-1]


def load():
	index = json.loads((RAW / 'decompile-index.json').read_text(encoding='utf-8'))
	funcs = []
	for f in index['functions']:
		if not f['address'].startswith('0x'):
			continue  # imported DLL function in Ghidra's EXTERNAL space
		size = 0
		for r in (f.get('body') or {}).get('ranges') or []:
			size += int(r['end'], 16) - int(r['start'], 16) + 1
		files, lines, params = [], [], None
		if f['status'] == 'written':
			text = (RAW / f['relative_path']).read_text(encoding='utf-8', errors='replace')
			files = [base_name(m) for m in FILE_RE.findall(text)]
			lines = sorted({int(x, 0) for x in LINE_RE.findall(text)})
			strs = STR_RE.findall(text)
			if CATCH_RE.match(f['name']) and MARKER in strs:
				# Arguments are built right to left, so reverse to get declaration order.
				params = strs[:strs.index(MARKER)][::-1]
		funcs.append({'addr': int(f['address'], 16), 'size': size, 'name': f['name'], 'files': files,
			'lines': lines, 'catch_params': params, 'params': None, 'parent': None,
			'kind': '', 'file': '', 'how': ''})
	funcs.sort(key=lambda x: x['addr'])
	return funcs


def classify(funcs):
	for fn in funcs:
		if UNWIND_RE.match(fn['name']):
			fn['kind'] = 'eh-funclet'
			continue
		cpp = sorted({x for x in fn['files'] if x.lower().endswith('.cpp')})
		hdr = sorted({x for x in fn['files'] if x.lower().endswith('.h')})
		if cpp:
			fn['file'], fn['how'] = '|'.join(cpp), 'assert'
		elif hdr:
			fn['file'], fn['how'] = hdr[0], 'header'

	anchors = [i for i, fn in enumerate(funcs) if fn['how'] == 'assert' and '|' not in fn['file']]
	last = anchors[-1]
	# The library region starts at the first Ghidra-named function after the last game file.
	lib_start = next(i for i in range(last + 1, len(funcs))
		if not funcs[i]['kind'] and not funcs[i]['name'].startswith(('FUN_', 'Catch@')))

	def fill(lo, hi, file, how):
		for fn in funcs[lo:hi]:
			if not fn['kind'] and not fn['how']:
				fn['file'], fn['how'] = file, how

	fill(0, anchors[0], '?|' + funcs[anchors[0]]['file'], 'gap')
	for a, b in zip(anchors, anchors[1:]):
		fa, fb = funcs[a]['file'], funcs[b]['file']
		if fa == fb:
			fill(a + 1, b, fa, 'between')
		else:
			fill(a + 1, b, f'{fa}|{fb}', 'gap')
	fill(last + 1, lib_start, funcs[last]['file'] + '|?', 'gap')

	for i, fn in enumerate(funcs):
		if fn['kind']:
			continue
		if i >= lib_start:
			fn['kind'], fn['file'] = 'library', ''
			fn['how'] = 'region' if fn['name'].startswith(('FUN_', 'Catch@')) else 'named'
		else:
			fn['kind'] = 'catch' if CATCH_RE.match(fn['name']) else 'game'

	# MSVC emits catch blocks after the code of the function containing the try, so the
	# parent is the nearest preceding game function. This is a heuristic, not a proof.
	parent = None
	for fn in funcs:
		if fn['kind'] == 'game':
			parent = fn
		elif fn['kind'] == 'catch' and parent is not None:
			fn['parent'] = parent['addr']
			if fn['catch_params'] is not None and parent['params'] is None:
				parent['params'] = fn['catch_params']
	return funcs[lib_start]['addr']


def apply_ranges(funcs):
	"""Assign settled gaps from file-ranges.tsv; assertion and header evidence still win."""
	path = ROOT / 'file-ranges.tsv'
	if not path.exists():
		return
	ranges = []
	for line in path.read_text(encoding='utf-8').splitlines()[1:]:
		cols = line.split('\t')
		if len(cols) >= 3:
			ranges.append((int(cols[0], 16), int(cols[1], 16), cols[2]))
	for fn in funcs:
		if fn['kind'] not in GAME_KINDS or fn['how'] in ('assert', 'header'):
			continue
		for lo, hi, file in ranges:
			if lo <= fn['addr'] <= hi:
				fn['file'], fn['how'] = file, 'range'


def write_symbols(funcs):
	with (ROOT / 'symbols.tsv').open('w', encoding='utf-8', newline='\n') as w:
		w.write('address\tsize\tname\tkind\tfile\thow\tassert_lines\tparent\tparams\n')
		for fn in funcs:
			parent = f"0x{fn['parent']:06x}" if fn['parent'] is not None else ''
			params = fn['params'] if fn['kind'] == 'game' else fn['catch_params']
			w.write(f"0x{fn['addr']:06x}\t{fn['size']}\t{fn['name']}\t{fn['kind']}\t{fn['file']}\t"
				f"{fn['how']}\t{','.join(map(str, fn['lines']))}\t{parent}\t"
				f"{','.join(params) if params else ''}\n")


def source_addresses():
	"""Addresses named in src/ (written there) and names.tsv rows of kind stl (not source)."""
	in_src = set()
	for path in (ROOT / 'src').rglob('*'):
		if path.suffix.lower() in ('.h', '.cpp'):
			text = path.read_text(encoding='utf-8', errors='replace')
			in_src.update(int(x, 16) for x in re.findall(r'\b0x([0-9a-fA-F]{6})\b', text))
	not_source = set()
	path = ROOT / 'names.tsv'
	if path.exists():
		for line in path.read_text(encoding='utf-8').splitlines()[1:]:
			cols = line.split('\t')
			if len(cols) >= 3 and cols[2] == 'stl':
				not_source.add(int(cols[0], 16))
	return in_src, not_source


def table(rows, header):
	out = ['| ' + ' | '.join(header) + ' |', '|' + '|'.join('---:' if h in ('Functions', 'Catch', 'Bytes') else '---'
		for h in header) + '|']
	out += ['| ' + ' | '.join(str(c) for c in r) + ' |' for r in rows]
	return out + ['']


def write_report(funcs, lib_start):
	kinds, ksize = Counter(), Counter()
	for fn in funcs:
		kinds[fn['kind']] += 1
		ksize[fn['kind']] += fn['size']
	game = [fn for fn in funcs if fn['kind'] in GAME_KINDS]

	per, gaps = defaultdict(list), defaultdict(list)
	for fn in game:
		if fn['how'] in ('assert', 'between', 'range') and '|' not in fn['file']:
			per[fn['file']].append(fn)
		elif fn['how'] == 'gap':
			gaps[fn['file']].append(fn)
	header_fns = [fn for fn in game if fn['how'] == 'header']
	in_src, not_source = source_addresses()
	headers = Counter(fn['file'] for fn in header_fns)
	left = [fn for fn in header_fns if fn['addr'] not in in_src and fn['addr'] not in not_source]

	out = ['# Tibia.exe source map', '',
		'Generated by `tools/triage.py` from `decomp/raw`. Do not edit by hand; rerun the script.', '',
		'## Totals', '']
	out += table([(k, kinds[k], ksize[k]) for k in ('game', 'catch', 'library', 'eh-funclet')],
		('Kind', 'Functions', 'Bytes'))
	out += [f'The library region starts at `0x{lib_start:06x}`. `catch` functions are the bodies of the '
		"client's own `catch` blocks and belong to the function containing the `try`. `eh-funclet` "
		'functions (`Unwind@`) are exception cleanup code that the compiler regenerates from the source.', '',
		'Game and catch functions by assignment method:', '']
	out += table([(h, sum(1 for fn in game if fn['how'] == h), sum(fn['size'] for fn in game if fn['how'] == h))
		for h in ('assert', 'between', 'range', 'header', 'gap')], ('How', 'Functions', 'Bytes'))

	out += ['## Source files', '', 'In link order. First and last are the outermost assigned functions. '
		'Assert lines show which part of the original file the assertions cover.', '']
	rows = []
	for name, fs in sorted(per.items(), key=lambda kv: kv[1][0]['addr']):
		lines = [ln for fn in fs for ln in fn['lines']]
		rows.append((name, sum(1 for fn in fs if fn['kind'] == 'game'), sum(1 for fn in fs if fn['kind'] == 'catch'),
			sum(fn['size'] for fn in fs), f'`0x{fs[0]["addr"]:06x}`', f'`0x{fs[-1]["addr"]:06x}`',
			f'{min(lines)}-{max(lines)}' if lines else ''))
	out += table(rows, ('File', 'Functions', 'Catch', 'Bytes', 'First', 'Last', 'Assert lines'))

	out += ['## Unresolved gaps', '', 'Functions between two different files. Each belongs to one of the '
		'neighbours, or to a file without assertions whose name sorts between them.', '']
	rows = []
	for name, fs in sorted(gaps.items(), key=lambda kv: kv[1][0]['addr']):
		end = fs[-1]['addr'] + fs[-1]['size']
		rows.append((name.replace('|', ' / '), len(fs), sum(fn['size'] for fn in fs),
			f'`0x{fs[0]["addr"]:06x}`-`0x{end:06x}`'))
	out += table(rows, ('Between', 'Functions', 'Bytes', 'Range'))
	if headers:
		# A header function counts as written when src/ names its address (the "// 0x..." comments
		# next to the definitions), or as not source when names.tsv gives it kind stl.
		out += ['## Header code', '', f'{len(header_fns)} functions name only a header in their assertions; '
			f'{sum(1 for fn in header_fns if fn["addr"] in in_src)} of them are defined in `src/` (their address '
			f'is named there), {sum(1 for fn in header_fns if fn["addr"] in not_source and fn["addr"] not in in_src)} '
			'are not source (names.tsv kind `stl`). Left:', '']
		out += table([(fn['file'], f'`0x{fn["addr"]:06x}`', fn['size']) for fn in left], ('Header', 'Address', 'Bytes'))
	(ROOT / 'notes').mkdir(exist_ok=True)
	(ROOT / 'notes' / 'source-map.md').write_text('\n'.join(out), encoding='utf-8', newline='\n')
	hows = Counter(fn['how'] for fn in game)
	print(f'game {kinds["game"]} ({ksize["game"]} B), catch {kinds["catch"]} ({ksize["catch"]} B), '
		f'library {kinds["library"]} ({ksize["library"]} B), eh-funclet {kinds["eh-funclet"]}; by how: {dict(hows)}')


def main():
	funcs = load()
	lib_start = classify(funcs)
	apply_ranges(funcs)
	write_symbols(funcs)
	write_report(funcs, lib_start)


if __name__ == '__main__':
	main()
