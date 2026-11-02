"""The original's line numbers of the error sites (issue #35).

Every error site of the client names its source line in the message ("Utils.cpp 66: assertion
failed"). The original's line is an immediate the binary pushes as the second argument of the
TMyException constructor (0x508400: assertions, errors and the "unknown exception occurred"
catch blocks; 0x5086a0: the "exception occurred" catch blocks that wrap a caught TMyException).
Our macros take that line as their first argument: TASSERT(66, s!=NULL), TEND1(874, Obj).

The script reads both sides:
- the binary: every constructor call in the game code, its line, the literal strings the code
  builds for the call (reason, names, kind, file, in the order the compiler builds them) and the
  function it belongs to (a catch block's parent from symbols.tsv);
- our sources (src/, headers included): every macro site with its arguments, the line it names,
  the strings its macro builds and the nearest address comment before it.

A site's signature is the constructor and the literal strings; a TEND is the pair of catch
blocks it makes (the wrapping one and the "unknown exception occurred" one, which must name the
same line). Sites that are the same in several places of the binary (header code compiled into
several objects, inlined functions) are one unit.

A new site: write the line the binary has for it (`python -I tools/lines.py binary` lists every
unit: file, line, strings, functions), then run the check.

Usage:
	python -I tools/lines.py check [-v]   each of our sites must name the line of a binary unit with
	                                      its signature, every unit must have one of our sites, and
	                                      the lines must grow in our source order (exceptions in
	                                      EXPLAINED); exit code 1 otherwise; -v lists the exceptions
	python -I tools/lines.py align [--write]
	                                      aligns our sites with the binary's per file by order and
	                                      signature (whatever line argument they have), prints the
	                                      result and estimates for sites without a unit; --write
	                                      puts the lines in (how the lines were first written)
	python -I tools/lines.py binary       lists the binary's units
	--src <dir>                           reads the sources from <dir> instead of src/
Needs objdump (mingw64) on PATH.
"""
import bisect
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / 'Tibia.exe'
SRC = Path(sys.argv[sys.argv.index('--src') + 1]) if '--src' in sys.argv else ROOT / 'src'
BASE = 0x400000
CTOR_STR = 0x508400   # TMyException(File, Line, Kind, Name1, Value1, Name2, Value2, std::string Reason)
CTOR_WRAP = 0x5086a0  # TMyException(File, Line, Kind, Name1, Value1, Name2, Value2, TMyException &Inner)
THROW = 0x511d2b      # _CxxThrowException
CODE_START = 0x401000

# The macros and how their arguments make the constructor's strings. Each entry:
# (number of arguments without the line, ctor, function(args) -> (kind, name1, name2, reason)).
# A value of None is not a literal (built at run time); the ctor None means either.
def _assert(e, v1=None, v2=None, bug=None, bracket=None):
	reason = stringize(e)
	if bug is not None:
		b = literal(bug)
		reason = None if b is None else reason + ' ' + b
	if bracket is not None:
		reason = reason + ' [' + stringize(bracket) + ']'
	return ('assertion failed', name(v1), name(v2), reason)


def name(v):
	return '' if v is None else stringize(v)


MACROS = {
	'TASSERT': (1, 'str', lambda a: _assert(a[0])),
	'TASSERT1': (2, 'str', lambda a: _assert(a[0], a[1])),
	'TASSERT2': (3, 'str', lambda a: _assert(a[0], a[1], a[2])),
	'TASSERTB': (2, 'str', lambda a: _assert(a[0], bug=a[1])),
	'TASSERTB1': (3, 'str', lambda a: _assert(a[0], a[1], bug=a[2])),
	'TASSERTB2': (4, 'str', lambda a: _assert(a[0], a[1], a[2], bug=a[3])),
	'TASSERT1_BUG': (3, 'str', lambda a: _assert(a[0], a[2], bracket=a[1])),
	'TASSERT2_BUG': (4, 'str', lambda a: _assert(a[0], a[2], a[3], bracket=a[1])),
	'TERROR': (1, 'str', lambda a: (literal(a[0]), '', '', '')),
	'TERROR1': (2, 'str', lambda a: (literal(a[0]), name(a[1]), '', '')),
	'TERROR2': (3, 'str', lambda a: (literal(a[0]), name(a[1]), name(a[2]), '')),
	'TERROR_REASON': (2, 'str', lambda a: (literal(a[0]), '', '', literal(a[1]))),
	'TERROR1_REASON': (3, 'str', lambda a: (literal(a[0]), name(a[1]), '', literal(a[2]))),
	'TERROR1B': (3, 'str', lambda a: (literal(a[0]), name(a[1]), '', literal(a[2]))),
	'TMYEXCEPTION': (6, None, lambda a: (literal(a[0]), literal(a[1]), literal(a[3]), literal(a[5]))),
	'TEND': (0, 'tend', lambda a: ('', '', '', None)),
	'TEND1': (1, 'tend', lambda a: ('', name(a[0]), '', None)),
	'TEND2': (2, 'tend', lambda a: ('', name(a[0]), name(a[1]), None)),
	'TEND_HANDLERS': (4, 'tend', lambda a: ('', literal(a[0]), literal(a[2]), None)),
}

# Our sites without a unit of the binary at their line, keyed (file, line, macro): (reason, function).
# 'nocatch': a TEND whose function has no catch block in the binary (its FuncInfo has no try
# block: nothing in the try block can throw, so the compiler dropped the handlers); the check
# verifies that symbols.tsv lists no catch block of the function. 'dropped': an assertion the
# compiler dropped (the function returns before it, or it folded it into another one); the check
# verifies that the function has no unit at that line. The lines of both are estimates, between
# the neighbouring sites' lines; no message can carry them.
# 'order': a site whose unit is out of our source order (the check finds the unit as usual).
EXPLAINED = {
	('ContentWindows.cpp', 1357, 'TEND'): ('nocatch', 0x42b450),
	('ContentWindows.cpp', 1990, 'TEND'): ('nocatch', 0x42e6b0),
	('Creatures.cpp', 470, 'TEND'): ('nocatch', 0x44c530),
	('Creatures.cpp', 504, 'TEND'): ('nocatch', 0x44c860),
	('DialogElements.cpp', 62, 'TEND'): ('nocatch', 0x453ad0),
	('DialogElements.cpp', 106, 'TEND'): ('nocatch', 0x453f60),
	('DialogElements.cpp', 321, 'TEND'): ('nocatch', 0x4556b0),
	('DialogElements.cpp', 1138, 'TEND'): ('nocatch', 0x4584e0),
	('DialogElements.cpp', 1213, 'TEND'): ('nocatch', 0x458ab0),
	('DialogElements.cpp', 1471, 'TEND'): ('nocatch', 0x459b00),
	('DialogElements.cpp', 1531, 'TEND'): ('nocatch', 0x459d30),
	('DialogElements.cpp', 1740, 'TEND'): ('nocatch', 0x45a360),
	('DialogElements.cpp', 1992, 'TEND'): ('nocatch', 0x45ade0),
	('DialogElements.cpp', 2223, 'TEND'): ('nocatch', 0x45bc80),
	('DialogElements.cpp', 3022, 'TEND'): ('nocatch', 0x45f1a0),
	('DialogElements.cpp', 3351, 'TEND'): ('nocatch', 0x460a90),
	('DialogElements.cpp', 4255, 'TEND'): ('nocatch', 0x464d00),
	('Dialogs.cpp', 1787, 'TEND'): ('nocatch', 0x471b60),
	('GUI.cpp', 844, 'TEND'): ('nocatch', 0x498160),
	('MainFrm.cpp', 135, 'TEND'): ('nocatch', 0x4a88f0),
	('MainFrm.cpp', 388, 'TEND'): ('nocatch', 0x4a94b0),
	('MainFrm.cpp', 795, 'TEND'): ('nocatch', 0x4ab730),
	('MainWindow.cpp', 484, 'TEND'): ('nocatch', 0x4ac7c0),
	('MainWindow.cpp', 762, 'TEND'): ('nocatch', 0x4ae430),
	('graphics.cpp', 498, 'TEND'): ('nocatch', 0x48ce00),
	('graphics.cpp', 516, 'TEND'): ('nocatch', 0x48ce70),
	('graphics.cpp', 681, 'TEND'): ('nocatch', 0x48df40),
	('graphics.cpp', 706, 'TEND'): ('nocatch', 0x48dfc0),
	('graphics.cpp', 728, 'TEND'): ('nocatch', 0x48e0f0),
	('graphics.cpp', 750, 'TEND'): ('nocatch', 0x48e190),
	('graphics.cpp', 1368, 'TEND'): ('nocatch', 0x491880),
	('graphics.cpp', 1404, 'TEND'): ('nocatch', 0x491b80),
	('graphics.cpp', 1437, 'TEND'): ('nocatch', 0x491f50),
	# DrawPicturePart: Width>0 and Height>0 after "if (Width <= 0 || Height <= 0) return;" (the
	# exception tables keep their states between the first assertion, 1164, and the try block).
	('graphics.cpp', 1166, 'TASSERT1'): ('dropped', 0x490c60),
	('graphics.cpp', 1167, 'TASSERT1'): ('dropped', 0x490c60),
	# THelpBubble::Place: the second assertion of the corner case is folded into the first (262)
	# by the compiler; 263 from the pairs of the other cases (257-258, 267-268).
	('ContextMenus.cpp', 263, 'TASSERT1'): ('dropped', 0x442110),
	# ProcessMissileEffects: the original has the branch of an arrived missile first (the code at
	# 0x4bbe2a falls through to it, line 841 before the moving branch's 850-858); ours has it in
	# the else branch.
	('Map.cpp', 841, 'TASSERT'): ('order', 0x4bbd40),
}


# --- The binary ---------------------------------------------------------------------------

DATA = EXE.read_bytes()


def cstring(va):
	"""The C string at va, or None if va is not one (printable text up to a NUL)."""
	o = va - BASE
	if not 0 <= o < len(DATA):
		return None
	end = DATA.find(b'\0', o, o + 2000)
	if end < 0:
		return None
	raw = DATA[o:end]
	if any(c < 0x20 and c not in (9, 10, 13) or c == 0x7f for c in raw):
		return None
	return raw.decode('latin-1')


def load_symbols():
	"""Function starts (game, catch, eh-funclet, library) and each catch block's parent."""
	rows = []
	for line in (ROOT / 'symbols.tsv').read_text(encoding='utf-8').splitlines()[1:]:
		c = line.split('\t')
		rows.append((int(c[0], 16), c[2], c[3], int(c[7], 16) if c[7] else None))
	rows.sort()
	return rows


def disassemble(start, stop):
	out = subprocess.run(['objdump', '-d', '-M', 'intel', '--no-show-raw-insn', f'--start-address={start:#x}',
		f'--stop-address={stop:#x}', str(EXE)], capture_output=True, text=True, check=True).stdout
	code = []
	for line in out.split('\n'):
		m = re.match(r'\s*([0-9a-f]+):\s+(\S+)\s*(.*)$', line)
		if m:
			code.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
	return code


def call_target(ins):
	if ins[1] != 'call':
		return None
	m = re.fullmatch(r'0x([0-9a-f]+)', ins[2])
	return int(m.group(1), 16) if m else None


def binary_sites():
	"""Every constructor call: dict with addr, ctor, line, strings, func, catch, file."""
	rows = load_symbols()
	starts = [r[0] for r in rows]
	stop = next(r[0] for r in rows if r[2] == 'library')
	code = disassemble(CODE_START, stop)
	sites = []
	for i, ins in enumerate(code):
		target = call_target(ins)
		if target not in (CTOR_STR, CTOR_WRAP):
			continue
		row = rows[bisect.bisect_right(starts, ins[0]) - 1]
		fstart = row[0]
		# The arguments: the pushes before the call; the last one is the file, the one before the line.
		pushes, j = [], i - 1
		while j >= 0 and len(pushes) < 2 and code[j][0] >= fstart:
			if code[j][1] == 'push':
				pushes.append(code[j])
			elif code[j][1] == 'call':
				break
			j -= 1
		line, problem = None, None
		if len(pushes) < 2:
			problem = f'{len(pushes)} pushes before the call'
		else:
			m = re.fullmatch(r'0x([0-9a-f]+)', pushes[1][2])
			if m:
				line = int(m.group(1), 16)
			else:
				problem = f'line pushed as {pushes[1][2]}'
		# The literal strings built since the previous error site or throw, in code order.
		strings, j = [], i - 1
		while j >= 0 and code[j][0] >= fstart:
			t = call_target(code[j])
			if t in (CTOR_STR, CTOR_WRAP, THROW):
				break
			if code[j][1] == 'push':
				m = re.fullmatch(r'0x([0-9a-f]{6})', code[j][2])
				if m:
					s = cstring(int(m.group(1), 16))
					if s is not None and int(m.group(1), 16) >= 0x550000:
						strings.append(s)
			j -= 1
		strings.reverse()
		files = [s for s in strings if re.search(r'\.(cpp|h)$', s)]
		file = re.split(r'[\\/]', files[-1])[-1] if files else None
		kind = row[2]
		func = row[3] if kind == 'catch' and row[3] is not None else row[0]
		sites.append({'addr': ins[0], 'ctor': 'str' if target == CTOR_STR else 'wrap', 'line': line,
			'strings': strings, 'func': func, 'catch': row[0] if kind == 'catch' else None, 'file': file,
			'problem': problem, 'fname': rows[bisect.bisect_right(starts, func) - 1][1]})
	return sites


def binary_units(sites):
	"""Sites to units, one per (file, line, form): a TEND's two catch blocks are one 'tend' unit
	with its two names; any other site is a 'str' or 'wrap' unit with the string lists of its
	copies (header code compiled into several objects, inlined functions).

	Returns a dict key -> unit, key = (file, line, 'tend', name1, name2) or (file, line, ctor)."""
	# Pair the catch blocks per parent function: each "exception occurred" block with the next
	# "unknown exception occurred" block with the same names (they follow each other).
	used, pairs = set(), {}
	bycatch = defaultdict(list)
	for s in sites:
		s['body'] = s['strings'][:-1] if s['file'] else s['strings']
		if s['catch'] is not None:
			bycatch[s['func']].append(s)
	for func, lst in bycatch.items():
		lst.sort(key=lambda s: s['addr'])
		for k, s in enumerate(lst):
			b = s['body']
			if s['ctor'] != 'wrap' or len(b) < 3 or b[-1] != 'exception occurred':
				continue
			for t in lst[k + 1:]:
				if (t['ctor'] == 'str' and id(t) not in used and t['line'] == s['line']
						and t['body'][-4:] == ['', b[-3], b[-2], 'unknown exception occurred']):
					pairs[id(s)] = t
					used.add(id(t))
					break
	units = {}

	def add(key, s, line):
		u = units.setdefault(key, {'file': s['file'], 'line': line, 'form': key[2], 'names': key[3:],
			'bodies': [], 'sites': []})
		u['sites'].append(s)
		if s['body'] not in u['bodies']:
			u['bodies'].append(s['body'])
	for s in sites:
		if id(s) in used:
			continue
		if id(s) in pairs:
			t = pairs[id(s)]
			line = s['line'] if s['line'] == t['line'] else (s['line'], t['line'])
			key = (s['file'], line, 'tend', s['body'][-2], s['body'][-3])
			add(key, s, line)
			add(key, t, line)
			continue
		add((s['file'], s['line'], s['ctor']), s, s['line'])
	return units


# --- Our sources --------------------------------------------------------------------------

def strip_comments(text):
	"""Comments replaced by a space, strings kept."""
	out, i, n = [], 0, len(text)
	while i < n:
		c = text[i]
		if text.startswith('//', i):
			j = text.find('\n', i)
			j = n if j < 0 else j
			out.append(' ')
			i = j
		elif text.startswith('/*', i):
			j = text.find('*/', i + 2)
			out.append(' ')
			i = n if j < 0 else j + 2
		elif c in '"\'':
			j = skip_literal(text, i)
			out.append(text[i:j])
			i = j
		else:
			out.append(c)
			i += 1
	return ''.join(out)


def skip_literal(text, i):
	q = text[i]
	j = i + 1
	while j < len(text) and text[j] != q:
		j += 2 if text[j] == '\\' else 1
	return j + 1


def stringize(arg):
	return re.sub(r'\s+', ' ', strip_comments(arg)).strip()


ESCAPES = {'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', '"': '"', "'": "'", '0': '\0', 'a': '\a'}


def literal(arg):
	"""The value of an argument made of string literals only, else None."""
	s = strip_comments(arg).strip()
	if not s.startswith('"'):
		return None
	out, i = [], 0
	while i < len(s):
		if s[i].isspace():
			i += 1
			continue
		if s[i] != '"':
			return None
		j = skip_literal(s, i)
		body = s[i + 1:j - 1]
		k = 0
		while k < len(body):
			if body[k] == '\\':
				out.append(ESCAPES.get(body[k + 1], body[k + 1]))
				k += 2
			else:
				out.append(body[k])
				k += 1
		i = j
	return ''.join(out)


def source_sites(path):
	"""Every macro invocation of path: dict with macro, line (ours), start/end offsets of the
	invocation, args (raw texts), given (the line argument if the macro already has one), hint."""
	text = path.read_text(encoding='utf-8')
	sites = []
	i, n, lineno = 0, len(text), 1
	line_starts = [0] + [m.end() for m in re.finditer('\n', text)]
	hints = [(m.start(), int(m.group(1), 16)) for m in re.finditer(r'^// (?:[A-Za-z_:~ ]*\b)?0x([0-9a-f]{6})\b', text, re.M)]
	hint_pos = [h[0] for h in hints]
	at_line_start = True
	while i < n:
		c = text[i]
		if c == '\n':
			at_line_start = True
			i += 1
			continue
		if text.startswith('//', i):
			j = text.find('\n', i)
			i = n if j < 0 else j
			continue
		if text.startswith('/*', i):
			j = text.find('*/', i + 2)
			i = n if j < 0 else j + 2
			continue
		if c in '"\'':
			i = skip_literal(text, i)
			at_line_start = False
			continue
		if c == '#' and at_line_start:
			# A preprocessor line, with its continuations.
			while True:
				j = text.find('\n', i)
				if j < 0:
					i = n
					break
				if text[j - 1] == '\\':
					i = j + 1
					continue
				i = j
				break
			continue
		if c.isspace():
			i += 1
			continue
		at_line_start = False
		m = re.compile(r'[A-Za-z_]\w*').match(text, i)
		if not m:
			i += 1
			continue
		word = m.group(0)
		i = m.end()
		if word not in MACROS:
			continue
		start = m.start()
		j = i
		while j < n and text[j] in ' \t':
			j += 1
		args, end = [], i
		if j < n and text[j] == '(':
			depth, k, a0 = 0, j, j + 1
			while True:
				ch = text[k]
				if ch in '"\'':
					k = skip_literal(text, k)
					continue
				if text.startswith('//', k):
					k = text.find('\n', k)
					continue
				if text.startswith('/*', k):
					k = text.find('*/', k) + 2
					continue
				if ch in '([{':
					depth += 1
				elif ch in ')]}':
					depth -= 1
					if depth == 0:
						args.append(text[a0:k])
						end = k + 1
						break
				elif ch == ',' and depth == 1:
					args.append(text[a0:k])
					a0 = k + 1
				k += 1
			i = end
		if args == ['']:
			args = []
		count = MACROS[word][0]
		given = None
		if len(args) == count + 1 and re.fullmatch(r'\s*\d+\s*', args[0]):
			given = int(args[0])
			args = args[1:]
		elif len(args) != count:
			raise SystemExit(f'{path.name}:{bisect.bisect_right(line_starts, start)}: {word} with {len(args)} arguments')
		h = bisect.bisect_right(hint_pos, start) - 1
		sites.append({'macro': word, 'line': bisect.bisect_right(line_starts, start), 'start': start, 'end': end,
			'args': args, 'given': given, 'hint': hints[h][1] if h >= 0 else None, 'path': path,
			'file': path.name})
	return sites


def our_sig(site):
	"""('tend', name1, name2) or (ctor, literal strings in the order the code builds them):
	str: reason, name2, name1, kind; wrap: name2, name1, kind. Strings built at run time are left
	out; ctor None (TMYEXCEPTION) may be either."""
	count, ctor, f = MACROS[site['macro']]
	kind, n1, n2, reason = f(site['args'])
	if ctor == 'tend':
		return ('tend', n1, n2)
	order = [reason, n2, n1, kind] if ctor == 'str' else [n2, n1, kind]
	if ctor is None and reason is not None:
		ctor = 'str'
	return (ctor, [x for x in order if x is not None])


def sig_match(ours, unit):
	"""Our signature against a binary unit: the same TEND names, or our strings at the end of
	every copy's strings."""
	if ours[0] == 'tend' or unit['form'] == 'tend':
		return ours[0] == unit['form'] and tuple(ours[1:]) == tuple(unit['names'])
	if ours[0] is not None and ours[0] != unit['form']:
		return False
	# A copy whose list is shorter (a string built before the previous call of the window, as in
	# Player.cpp's catch blocks) is compared on the strings it has, at least the kind and names.
	want = ours[1]
	for b in unit['bodies']:
		m = min(len(b), len(want))
		if m < min(len(want), 3) or b[len(b) - m:] != want[len(want) - m:]:
			return False
	return True


def all_source_sites():
	sites = []
	for path in sorted(SRC.rglob('*')):
		if path.suffix.lower() in ('.cpp', '.h') and path.name != 'common.h':
			sites += source_sites(path)
	return sites


# --- Alignment ----------------------------------------------------------------------------

def align(ours, theirs, funcs_of):
	"""Our sites (source order) against the units (line order): a maximum-score alignment
	where a pair needs matching signatures; a pair scores 10, plus 1 if the site's address
	comment names the unit's function."""
	n, m = len(ours), len(theirs)
	score = [[0] * (m + 1) for _ in range(n + 1)]
	move = [[0] * (m + 1) for _ in range(n + 1)]
	for i in range(1, n + 1):
		for j in range(1, m + 1):
			best, mv = score[i - 1][j], 1
			if score[i][j - 1] > best:
				best, mv = score[i][j - 1], 2
			if sig_match(ours[i - 1]['sig'], theirs[j - 1]):
				s = score[i - 1][j - 1] + 10 + (1 if ours[i - 1]['hint'] in funcs_of(theirs[j - 1]) else 0)
				if s > best:
					best, mv = s, 3
			score[i][j], move[i][j] = best, mv
	pairs, i, j = [], n, m
	while i > 0 and j > 0:
		if move[i][j] == 3:
			pairs.append((i - 1, j - 1))
			i, j = i - 1, j - 1
		elif move[i][j] == 1:
			i -= 1
		else:
			j -= 1
	pairs.reverse()
	return pairs


def unit_list(units):
	out = list(units)
	for u in out:
		u['funcs'] = sorted({s['func'] for s in u['sites']})
	out.sort(key=lambda u: (str(u['file']), u['line'] if isinstance(u['line'], int) else -1, u['sites'][0]['addr']))
	return out


def fmt_sig(sig):
	if sig[0] == 'tend':
		return f'TEND{sig[1:]!r}'
	return f'{sig[0]}{sig[1]!r}'


def fmt_unit(u):
	if u['form'] == 'tend':
		return f'TEND{tuple(u["names"])!r}'
	return f'{u["form"]}' + '|'.join(repr(b[-5:]) for b in u['bodies'])


def main():
	mode = sys.argv[1] if len(sys.argv) > 1 else 'check'
	sites = binary_sites()
	problems = [s for s in sites if s['problem'] or s['file'] is None]
	units = unit_list(binary_units(sites).values())
	if mode == 'binary':
		for s in problems:
			print(f'PROBLEM {s["addr"]:#x} {s["problem"]} file={s["file"]} strings={s["strings"]}')
		for u in units:
			print(f'{u["file"]}\t{u["line"]}\t{fmt_unit(u)}\t{len(u["sites"])}\t'
				+ ','.join(f'{f:#x}' for f in u['funcs']))
		return 0
	ours = all_source_sites()
	for s in ours:
		s['sig'] = our_sig(s)
	byfile_ours = defaultdict(list)
	for s in ours:
		byfile_ours[s['file']].append(s)
	byfile_units = defaultdict(list)
	for u in units:
		byfile_units[u['file']].append(u)
	if mode == 'align':
		return do_align(byfile_ours, byfile_units, '--write' in sys.argv)
	return do_check(byfile_ours, byfile_units, problems)


def do_align(byfile_ours, byfile_units, write):
	edits = defaultdict(list)
	unmatched_ours, unmatched_units, hint_diff = [], [], []
	for file in sorted(set(byfile_ours) | set(byfile_units), key=str):
		o = byfile_ours.get(file, [])
		t = byfile_units.get(file, [])
		pairs = align(o, t, lambda u: u['funcs'])
		po = {a for a, b in pairs}
		pt = {b for a, b in pairs}
		for a, b in pairs:
			s, u = o[a], t[b]
			if not isinstance(u['line'], int):
				print(f'TEND LINES DIFFER {file}:{s["line"]} {u["line"]}')
				continue
			if s['hint'] is not None and s['hint'] not in u['funcs']:
				hint_diff.append((file, s['line'], s['hint'], u['funcs']))
			edits[s['path']].append((s, u['line']))
		# The original's lines must grow with ours.
		last = None
		for a, b in pairs:
			if last is not None and t[b]['line'] < last[1]:
				print(f'ORDER {file}: ours {o[a]["line"]} -> {t[b]["line"]} after ours {last[0]} -> {last[1]}')
			last = (o[a]['line'], t[b]['line'])
		# A site and a unit left over with the only matching signature on both sides: paired
		# out of order (code the original has in another order, e.g. an inverted if).
		lo = [a for a in range(len(o)) if a not in po]
		lt = [b for b in range(len(t)) if b not in pt]
		for a in lo:
			cand = [b for b in lt if sig_match(o[a]['sig'], t[b])]
			if len(cand) == 1 and sum(1 for x in lo if sig_match(o[x]['sig'], t[cand[0]])) == 1:
				b = cand[0]
				print(f'OUT OF ORDER {file}:{o[a]["line"]} {o[a]["macro"]} -> {t[b]["line"]}')
				edits[o[a]['path']].append((o[a], t[b]['line']))
				po.add(a)
				pt.add(b)
		# The rest of ours: an estimate between the neighbouring pairs' lines.
		for a in range(len(o)):
			if a in po:
				continue
			before = [(x, y) for x, y in pairs if x < a]
			after = [(x, y) for x, y in pairs if x > a]
			ol = o[a]['line']
			if before and after:
				(x0, y0), (x1, y1) = before[-1], after[0]
				l0, l1, m0, m1 = o[x0]['line'], o[x1]['line'], t[y0]['line'], t[y1]['line']
				est = m0 + round((ol - l0) * (m1 - m0) / (l1 - l0)) if l1 != l0 else m0
				est = max(m0 + 1, min(est, m1 - 1)) if m1 - m0 >= 2 else est
			elif before:
				est = t[before[-1][1]]['line'] + ol - o[before[-1][0]]['line']
			elif after:
				est = max(1, t[after[0][1]]['line'] - (o[after[0][0]]['line'] - ol))
			else:
				est = ol
			o[a]['estimate'] = est
			edits[o[a]['path']].append((o[a], est))
		unmatched_ours += [o[a] for a in range(len(o)) if a not in po]
		unmatched_units += [t[b] for b in range(len(t)) if b not in pt]
	for s in unmatched_ours:
		print(f'OURS UNMATCHED {s["file"]}:{s["line"]} {s["macro"]} {fmt_sig(s["sig"])} hint={s["hint"] and hex(s["hint"])}'
			f' estimate {s.get("estimate")}')
	for u in unmatched_units:
		print(f'BINARY UNMATCHED {u["file"]}:{u["line"]} {fmt_unit(u)} funcs='
			+ ','.join(f'{f:#x}' for f in u['funcs']) + ' at ' + ','.join(f'{x["addr"]:#x}' for x in u['sites']))
	for file, line, hint, funcs in hint_diff:
		print(f'HINT {file}:{line} comment {hint:#x}, unit in ' + ','.join(f'{f:#x}' for f in funcs))
	total = sum(len(v) for v in edits.values())
	print(f'{total} pairs, {len(unmatched_ours)} of our sites and {len(unmatched_units)} units unmatched, '
		f'{len(hint_diff)} address comments that name another function')
	if write:
		for path, lst in edits.items():
			text = path.read_text(encoding='utf-8')
			for s, line in sorted(lst, key=lambda x: -x[0]['start']):
				text = text[:s['start']] + with_line(text[s['start']:s['end']], s, line) + text[s['end']:]
			with path.open('w', encoding='utf-8', newline='\n') as w:
				w.write(text)
		print('written')
	return 0


def with_line(inv, site, line):
	"""The invocation text with the line as its first argument."""
	m = re.match(r'([A-Za-z_]\w*)(\s*)(\()?', inv)
	if not m.group(3):
		return f'{m.group(1)}({line})' + inv[m.end():]
	rest = inv[m.end():]
	if site['given'] is not None:
		rest = re.sub(r'^\s*\d+\s*,\s*', '', rest, count=1)
		if site['macro'] in ('TEND',) or rest.strip() == ')':
			rest = re.sub(r'^\s*\d+\s*', '', rest, count=1)
	if rest.strip() == ')':
		return f'{m.group(1)}{m.group(2)}({line})'
	return f'{m.group(1)}{m.group(2)}({line}, ' + rest


def do_check(byfile_ours, byfile_units, problems):
	rows = load_symbols()
	catch_parents = {r[3] for r in rows if r[2] == 'catch'}
	errors = []
	for s in problems:
		errors.append(f'binary site {s["addr"]:#x} not read: {s["problem"] or "no file name"}')
	claimed = set()
	explained = defaultdict(list)
	matched = defaultdict(int)
	used_explained = set()
	for file in sorted(byfile_ours, key=str):
		units = byfile_units.get(file, [])
		last = None
		for s in byfile_ours[file]:
			where = f'{file}:{s["line"]} {s["macro"]}'
			if s['given'] is None:
				errors.append(f'{where}: no line argument')
				continue
			line = s['given']
			key = (file, line, s['macro'])
			found = [u for u in units if u['line'] == line and sig_match(s['sig'], u)]
			reason = EXPLAINED.get(key)
			if found:
				for u in found:
					claimed.add(id(u))
				matched[s['macro']] += 1
				if reason is not None:
					used_explained.add(key)
					if reason[0] != 'order':
						errors.append(f'{where} ({line}): explained as {reason[0]}, but the binary has the site')
					else:
						explained['order'].append(f'{where} -> {line}')
					continue
				if last is not None and line < last[1]:
					errors.append(f'{where} ({line}): before the line of {file}:{last[0]} ({last[1]}) in our order')
				last = (s['line'], line)
				continue
			if reason is None or reason[0] == 'order':
				near = [u['line'] for u in units if sig_match(s['sig'], u)]
				errors.append(f'{where}: line {line} has no binary site of {fmt_sig(s["sig"])}; '
					f'the binary has it at {near or "no line"}')
				continue
			used_explained.add(key)
			kind, func = reason
			if kind == 'nocatch' and func in catch_parents:
				errors.append(f'{where} ({line}): explained as nocatch, but {func:#x} has catch blocks')
				continue
			if any(u['line'] == line for u in units):
				errors.append(f'{where} ({line}): explained as {kind}, but the binary has a site at that line')
				continue
			explained[kind].append(f'{where} -> {line} (estimated; {func:#x})')
	for key in EXPLAINED:
		if key not in used_explained:
			errors.append(f'EXPLAINED entry {key} names no site of ours')
	for file, units in byfile_units.items():
		for u in units:
			if not isinstance(u['line'], int):
				errors.append(f'binary {file}: a TEND whose two catch blocks name {u["line"]}')
			elif id(u) not in claimed:
				errors.append(f'binary {file}:{u["line"]} {fmt_unit(u)} at '
					+ ','.join(f'{x["addr"]:#x}' for x in u['sites']) + ': no site of ours names it')
	nunits = sum(len(v) for v in byfile_units.values())
	nsites = sum(len(v) for v in byfile_ours.values())
	print(f'lines.py: {nsites} sites of ours, {nunits} units of the binary '
		f'({sum(len(u["sites"]) for v in byfile_units.values() for u in v)} constructor calls)')
	print('matched: ' + ', '.join(f'{k} {v}' for k, v in sorted(matched.items())) + f' ({sum(matched.values())})')
	for kind in ('nocatch', 'dropped', 'order'):
		print(f'explained, {kind}: {len(explained[kind])}')
		if '-v' in sys.argv:
			for x in explained[kind]:
				print('  ' + x)
	for e in errors:
		print('MISMATCH ' + e)
	print(f'{len(errors)} mismatches')
	return 1 if errors else 0


if __name__ == '__main__':
	sys.exit(main())
