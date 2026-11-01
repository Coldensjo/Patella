"""Make readable per-file copies of the raw Ghidra pseudocode.

For every game and catch function in symbols.tsv, this:
	- collapses each assertion or error block (about 30 lines of std::string
	  construction and a TMyException throw) into one TASSERT/TERROR/TRETHROW line;
	- removes the SEH bookkeeping (ExceptionList, state counter, stack markers);
	- drops local declarations that are no longer used;
	- replaces FUN_xxxxxxxx with names from names.tsv.

Output: decomp/clean/<source file>.c, one file per original source file, with
gap functions in decomp/clean/gap_<A>_<B>.c. The raw files are not changed.

Usage: python -I tools/simplify.py [repo root] [file filter]
"""
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
ONLY = sys.argv[2] if len(sys.argv) > 2 else None
RAW = ROOT / 'decomp' / 'raw' / 'functions'
OUT = ROOT / 'decomp' / 'clean'
EXE = ROOT / 'Tibia.exe'
IMAGE_BASE = 0x400000

THROW_CALLS = {'FUN_00508400': 'assert', 'FUN_005086a0': 'rethrow'}
STRING_CTORS = ('FUN_00402e10', 'FUN_004039d0')
STR_ARG_RE = re.compile(r'(FUN_00402e10|FUN_004039d0)\((?:"((?:[^"\\]|\\.)*)"|&?(DAT_[0-9a-f]{8}))')
EH_LINE_RE = re.compile(r'^\s*(ExceptionList = |p[uv]Stack_\w+ = |local_\w+ = &stack0x|\w+ = ExceptionList;)')
DECL_RE = re.compile(r'^\s*[\w ]+?\**\s*\**(\w+)(\s*\[[0-9a-fx]+\])*;\s*$')
FUN_RE = re.compile(r'\bFUN_00([0-9a-f]{6})\b')


def c_escape(s):
	return s.replace('\\', '\\\\').replace('"', '\\"')


class Exe:
	"""Reads C strings from the original image; sections are mapped at file offset = VA - base."""

	def __init__(self, path):
		self.data = path.read_bytes() if path.exists() else b''

	def cstr(self, va):
		off = va - IMAGE_BASE
		if not self.data or not 0 <= off < len(self.data):
			return None
		end = self.data.find(b'\0', off, off + 512)
		return self.data[off:end].decode('latin-1') if end >= 0 else None


def split_args(s):
	"""Split a call's argument text at top-level commas."""
	args, depth, cur = [], 0, ''
	for ch in s:
		if ch in '([':
			depth += 1
		elif ch in ')]':
			depth -= 1
		if ch == ',' and depth == 0:
			args.append(cur.strip())
			cur = ''
		else:
			cur += ch
	if cur.strip():
		args.append(cur.strip())
	return args


def call_args(text, name):
	i = text.find(name + '(')
	if i < 0:
		return None
	i += len(name) + 1
	depth, j = 1, i
	while j < len(text) and depth:
		depth += {'(': 1, ')': -1}.get(text[j], 0)
		j += 1
	return split_args(text[i:j - 1])


def block_start(lines, throw_idx):
	"""Walk back from the throw to the start of its block.

	The block ends at an unmatched '{', a case label, or (at the same depth) the end of a
	preceding sibling statement: a closing '}' that is not a do-while, a return, or a label.
	Without the last rule an assertion at the end of a function would swallow the code before it.
	"""
	depth = 0
	for i in range(throw_idx - 1, -1, -1):
		s = lines[i].strip()
		if depth == 0 and (re.match(r'^(case .*|default):$', s) or s == '}' or s.startswith('return')
				or re.match(r'^\w+:$', s)):
			return i + 1
		depth += s.count('}') - s.count('{')
		if depth < 0:
			return i + 1
	return 0


def collapse(lines, exe):
	"""Replace each TMyException block with one line. Returns the new line list."""
	out = list(lines)
	i = len(out) - 1
	while i >= 0:
		if '__CxxThrowException_8(' not in out[i]:
			i -= 1
			continue
		start = block_start(out, i)
		text = ' '.join(l.strip() for l in out[start:i])
		kind = next((k for f, k in THROW_CALLS.items() if f + '(' in text), None)
		if kind is None:
			i -= 1
			continue
		strings = []
		for m in STR_ARG_RE.finditer(text):
			if m.group(2) is not None:
				strings.append(m.group(2).encode('latin-1', 'replace').decode('unicode_escape', 'replace'))
			else:
				s = exe.cstr(int(m.group(3)[4:], 16))
				strings.append(s if s is not None else m.group(3))
		args = call_args(text, next(f for f in THROW_CALLS if f + '(' in text)) or []
		line = args[1] if len(args) > 1 else '?'
		line = str(int(line, 0)) if re.fullmatch(r'0x[0-9a-f]+|\d+', line) else line
		v1 = args[4] if len(args) > 4 else '?'
		v2 = args[6] if len(args) > 6 else '?'
		indent = re.match(r'^\s*', out[i]).group(0)
		# Strings are built right to left: [expr,] name2, name1, kind, file.
		if kind == 'assert' and len(strings) >= 5:
			expr, name2, name1, msg = strings[-5], strings[-4], strings[-3], strings[-2]
		elif len(strings) >= 4:
			expr, name2, name1, msg = '', strings[-4], strings[-3], strings[-2]
		else:
			expr, name2, name1, msg = '', '', '', ' / '.join(strings)
		vals = ''.join(f', "{c_escape(n)}", {v}' for n, v in ((name1, v1), (name2, v2)) if n)
		if kind == 'rethrow':
			stmt = f'TRETHROW({line}{vals});'
		elif msg == 'assertion failed':
			stmt = f'TASSERT_FAILED({line}, "{c_escape(expr)}"{vals});'
		else:
			stmt = f'TERROR({line}, "{c_escape(msg)}"{vals});'
		out[start:i + 1] = [indent + stmt]
		i = start - 1
	return out


def eh_state_var(lines):
	"""The SEH state counter is the local set to 0xffffffff just before the handler label."""
	for a, b in zip(lines, lines[1:]):
		m = re.match(r'^\s*(\w+) = 0xffffffff;', a)
		if m and re.match(r'^\s*p[uv]Stack_\w+ = &LAB_', b):
			return m.group(1)
	return None


def clean_body(body, exe, names):
	lines = [l.rstrip() for l in body.splitlines() if l.strip()]
	state = eh_state_var(lines)
	lines = collapse(lines, exe)
	keep = []
	for l in lines:
		if EH_LINE_RE.match(l):
			continue
		if state and re.match(rf'^\s*{state}(\._\d_\d_)? = [^;]*;\s*$', l):
			continue
		keep.append(l)
	text = '\n'.join(keep)
	for _ in range(3):  # declarations can become unused in cascades
		out = []
		for l in text.splitlines():
			m = DECL_RE.match(l)
			if m and not l.lstrip().startswith(('return', 'goto')):
				var = m.group(1)
				if len(re.findall(rf'\b{re.escape(var)}\b', text)) == 1:
					continue
			out.append(l)
		text = '\n'.join(out)
	text = FUN_RE.sub(lambda m: names.get('0x' + m.group(1), m.group(0)), text)
	return text


def main():
	names = {}
	if (ROOT / 'names.tsv').exists():
		for r in csv.DictReader((ROOT / 'names.tsv').open(encoding='utf-8'), delimiter='\t'):
			names[r['address']] = r['name']
	exe = Exe(EXE)
	groups = defaultdict(list)
	for r in csv.DictReader((ROOT / 'symbols.tsv').open(encoding='utf-8'), delimiter='\t'):
		if r['kind'] not in ('game', 'catch'):
			continue
		f = r['file']
		key = Path(f).stem if '|' not in f else 'gap_' + '_'.join(x.replace('.cpp', '').replace('?', 'start' if i == 0
			else 'end') for i, x in enumerate(f.split('|')))
		if ONLY and ONLY.lower() not in key.lower():
			continue
		groups[key].append(r)
	OUT.mkdir(parents=True, exist_ok=True)
	total_in = total_out = 0
	for key, rows in groups.items():
		parts = [f'// {key}: readable copy of Ghidra pseudocode, generated by tools/simplify.py.',
			'// TASSERT_FAILED(line, "expr", "name", value...) is a failed assertion at that line;',
			'// TERROR(line, "message", ...) is a thrown error; TRETHROW(line, "arg", value...) ends a catch block',
			'// and names the enclosing function\'s parameters. Not compilable; see decomp/raw for the original.', '']
		for r in rows:
			raw = (RAW / f"{r['address']}.c").read_text(encoding='utf-8', errors='replace')
			body = raw.split('// pseudocode: provider-specific; not original source', 1)[-1]
			total_in += len(body)
			clean = clean_body(body, exe, names)
			total_out += len(clean)
			meta = [f"{r['address']}", f"size {r['size']}", r['kind']]
			if r['name'] in names.values() or r['address'] in names:
				meta.append(f"name {names.get(r['address'], r['name'])}")
			if r['assert_lines']:
				meta.append(f"lines {r['assert_lines']}")
			if r['params']:
				meta.append(f"params {r['params']}")
			if r['parent']:
				meta.append(f"parent {r['parent']}")
			parts += ['//// ' + '  '.join(meta), clean, '']
		(OUT / f'{key}.c').write_text('\n'.join(parts), encoding='utf-8', newline='\n')
	print(f'{len(groups)} files, {total_in} -> {total_out} bytes ({100 * total_out // max(total_in, 1)}%)')


if __name__ == '__main__':
	main()
