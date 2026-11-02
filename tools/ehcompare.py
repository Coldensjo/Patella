# For each function name given (names.tsv spelling, Class_member for members), prints the binary's
# exception info (tools/ehinfo.py) next to the unwind states, try blocks and catch handlers of our
# compiled function, read from the listings in build\listing (tools/listing.cmd). A try block that the
# original compiler removed must be missing from ours too, so that a guessed catch body is never
# emitted (issue #24). Usage: python -I tools/ehcompare.py Name ...
import os, sys, glob, subprocess
W = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
names = {}
for l in open(os.path.join(W, 'names.tsv'), encoding='utf-8'):
	p = l.rstrip('\n').split('\t')
	if len(p) > 1 and p[0].startswith('0x'):
		names.setdefault(p[1], p[0])
lines = []
for f in glob.glob(os.path.join(W, 'build', 'listing', '*.asm')):
	lines += open(f, encoding='latin-1').read().splitlines()
procs = [l.split()[0] for l in lines if ' PROC' in l]

def block(label):
	# The DD lines of the table that starts with label.
	res = []
	inside = False
	for l in lines:
		if l.startswith(label + ' '):
			inside = True
			res.append(l)
		elif inside and l.startswith('\t'):
			res.append(l)
		elif inside:
			break
	return res

for n in sys.argv[1:]:
	addr = names.get(n)
	if '_' in n:
		cls, short = n.split('_', 1)
		pat = '?%s@%s@@' % (short, cls)
	else:
		pat = '?%s@@' % n
	eh = 'no address in names.tsv'
	if addr:
		eh = subprocess.run(['python', '-I', os.path.join(W, 'tools', 'ehinfo.py'), addr], capture_output=True,
			text=True).stdout.strip()
	print('== %s %s\n  binary: %s' % (n, addr, eh.replace('\n', '\n          ')))
	found = False
	for m in sorted(set(p for p in procs if p.startswith(pat))):
		found = True
		states = sum(1 for l in block('__unwindtable$' + m) if 'DD' in l) // 2
		tries = sum(1 for l in block('__tryblocktable$' + m) if 'FLAT:__catchsym$' in l)
		handlers = sum(1 for l in lines if l.startswith('__catch$' + m + '$') and l.endswith(':'))
		print('  ours %s: %d unwind states, %d try blocks, %d handlers' % (m, states, tries, handlers))
	if not found:
		print('  ours: not in build\\listing (run tools\\listing.cmd on its file)')
