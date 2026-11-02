"""Compare a compiled resource file (.res, or a linked PE32 image) with Tibia.exe's resources.

Usage: python -I tools/compare_resources.py <Tibia.exe> <Patella.res | Patella.exe>

Both sides must have the same set of (type, name, language) entries with byte-identical
data. Prints one line per entry and a summary; exits with 1 on any difference.

Not compared, because a .res file does not have them or the linker makes them: the
directory's time stamp and version, the data entries' code page (0 in Tibia.exe), the
placement of the data, and the .res memory flags (MOVEABLE, PURE, DISCARDABLE), which
the linker drops.
"""

import os
import struct
import sys

sys.dont_write_bytecode = True  # no tools/__pycache__
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import extract_resources as er  # noqa: E402


def read_res(path):
	"""Entries of a 32-bit .res file (the format rc.exe writes)."""
	with open(path, "rb") as f:
		data = f.read()
	entries = []
	position = 0

	def read_id(p):
		value = struct.unpack_from("<H", data, p)[0]
		if value == 0xFFFF:
			return struct.unpack_from("<H", data, p + 2)[0], p + 4
		end = p
		while struct.unpack_from("<H", data, end)[0] != 0:
			end += 2
		return data[p:end].decode("utf-16le").upper(), end + 2

	while position + 8 <= len(data):
		data_size, header_size = struct.unpack_from("<II", data, position)
		type_, p = read_id(position + 8)
		name, p = read_id(p)
		p = (p + 3) & ~3
		_, memory_flags, language, _, _ = struct.unpack_from("<IHHII", data, p)
		start = position + header_size
		if not (data_size == 0 and type_ == 0 and name == 0):  # the empty first entry
			entries.append(er.Entry(type_, name, language, 0, data[start:start + data_size], None))
		position = (start + data_size + 3) & ~3
	return entries


def read_any(path):
	with open(path, "rb") as f:
		head = f.read(2)
	if head == b"MZ":
		return er.read_pe_resources(path)[0]
	return read_res(path)


def describe(key):
	type_, name, language = key
	return "%s %s lang %s" % (er.type_name(type_), name, language)


def main(argv):
	if len(argv) != 3:
		print(__doc__.strip().splitlines()[2], file=sys.stderr)
		return 2
	original = {e.key(): e for e in er.read_pe_resources(argv[1])[0]}
	rebuilt = {}
	for e in read_any(argv[2]):
		key = e.key()
		if key in rebuilt:
			print("DUPLICATE %s in %s" % (describe(key), argv[2]))
		rebuilt[key] = e
	differences = 0
	for key in sorted(set(original) | set(rebuilt), key=lambda k: tuple((isinstance(x, str), x) for x in k)):
		a = original.get(key)
		b = rebuilt.get(key)
		if a is None:
			print("EXTRA     %s (%d bytes)" % (describe(key), len(b.data)))
			differences += 1
		elif b is None:
			print("MISSING   %s (%d bytes)" % (describe(key), len(a.data)))
			differences += 1
		elif a.data != b.data:
			first = next((i for i in range(min(len(a.data), len(b.data))) if a.data[i] != b.data[i]), min(len(a.data), len(b.data)))
			print("DIFFERENT %s: %d vs %d bytes, first difference at 0x%x" % (describe(key), len(a.data), len(b.data), first))
			differences += 1
		else:
			print("same      %s (%d bytes)" % (describe(key), len(a.data)))
	print("%d entries in the original, %d in %s, %d differences" % (len(original), len(rebuilt), os.path.basename(argv[2]), differences))
	return 1 if differences else 0


if __name__ == "__main__":
	sys.exit(main(sys.argv))
