# Lists the __xc initialiser table of Tibia.exe (0x5c0000-0x5c01ff): every entry, the data it writes, the
# calls and the atexit thunk it registers (with what the thunk calls on which datum). A file's static
# objects must match it: our compiled initialisers are the ??__E functions of a listing (tools/listing.cmd),
# our atexit destructors the ??__F ones (issue #24).
# Usage: python -I tools/xctable.py [Tibia.exe]
import os, struct, subprocess, re, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'Tibia.exe')
D = open(EXE, 'rb').read()
names = {}
for l in open(os.path.join(ROOT, 'names.tsv'), encoding='utf-8'):
	p = l.rstrip('\n').split('\t')
	if len(p) > 1 and p[0].startswith('0x'):
		try:
			names[int(p[0], 16)] = p[1]
		except ValueError:
			pass

def word(va):
	return struct.unpack_from('<I', D, va - 0x5c0000 + 0x1c0000)[0]

def dis(a, n=0x40):
	out = subprocess.run(['objdump', '-d', '-M', 'intel', '--no-show-raw-insn', '--start-address=%#x' % a,
		'--stop-address=%#x' % (a + n), EXE], capture_output=True, text=True).stdout
	res = []
	for l in out.splitlines():
		m = re.match(r'\s+([0-9a-f]+):\s+(.*)', l)
		if m:
			res.append((int(m.group(1), 16), m.group(2).strip()))
			if m.group(2).startswith(('ret', 'jmp')):
				break
	return res

def label(v):
	return '%#x%s' % (v, '=' + names[v] if v in names else '')

def summary(a):
	parts = []
	for ad, t in dis(a):
		for m in re.finditer(r'0x([0-9a-f]{6})\b', t):
			v = int(m.group(1), 16)
			if 0x5c0000 <= v < 0x723000 or t.startswith(('call', 'jmp')) or 0x401000 <= v < 0x55b000:
				parts.append('%s %s' % (t.split()[0], label(v)))
	return '; '.join(parts)

for va in range(0x5c0000, 0x5c0150, 4):
	v = word(va)
	if v == 0:
		print('%#x: 0' % va)
		continue
	if not (0x401000 <= v < 0x55b000):
		print('%#x: %#x (not code)' % (va, v))
		continue
	print('%#x: %s | %s' % (va, label(v), summary(v)))
	for ad, t in dis(v):
		m = re.match(r'push\s+0x([0-9a-f]+)', t)
		if m and 0x559000 <= int(m.group(1), 16) < 0x55b000:
			t = int(m.group(1), 16)
			print('        atexit %s | %s' % (label(t), summary(t)))
