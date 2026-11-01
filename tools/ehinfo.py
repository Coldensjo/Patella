"""Print the MSVC C++ exception tables of functions in Tibia.exe.

For each function: its FuncInfo, and every try block with the catch handlers in
order, each with the caught type (TypeDescriptor name, "..." for catch-all),
whether it is caught by reference (adjectives 0x8) or const (0x1), where the
catch object is stored, and the handler (the Catch@ funclet).

Type names: .H int, .PAD const char*, .?AVTMyException@@ class TMyException,
.?AW4PROGRAM_ERROR@@ enum PROGRAM_ERROR.

Usage: python -I tools/ehinfo.py 0x5089f0 [0x508aa0 ...]
"""
import struct
import sys
from pathlib import Path

EXE = Path(__file__).resolve().parent.parent / 'Tibia.exe'
BASE = 0x400000
DATA = EXE.read_bytes()


def u32(va):
	return struct.unpack_from('<I', DATA, va - BASE)[0]


def cstr(va):
	o = va - BASE
	return DATA[o:DATA.index(b'\0', o)].decode('latin-1')


def funcinfo(function):
	"""The prologue is PUSH -1; PUSH stub; the stub is MOV EAX, FuncInfo; JMP __CxxFrameHandler."""
	code = DATA[function - BASE:function - BASE + 32]
	i = code.find(b'\x6a\xff\x68')
	if i >= 0:
		stub = struct.unpack_from('<I', code, i + 3)[0]
	else:
		# PUSH -1; MOV EAX, FS:[0]; PUSH stub (DialogElements.cpp 0x45dfc0)
		i = code.find(b'\x6a\xff\x64\xa1\x00\x00\x00\x00\x68')
		if i < 0:
			return None, None
		stub = struct.unpack_from('<I', code, i + 9)[0]
	stub_code = DATA[stub - BASE:stub - BASE + 16]
	j = stub_code.find(b'\xb8')
	return stub, struct.unpack_from('<I', stub_code, j + 1)[0] if j >= 0 else None


def main():
	for arg in sys.argv[1:]:
		function = int(arg, 16)
		stub, info = funcinfo(function)
		if info is None:
			print(f'{function:#x}: no C++ exception frame')
			continue
		magic, states, unwind, tries, trymap = struct.unpack_from('<IiIII', DATA, info - BASE)
		print(f'{function:#x}: FuncInfo {info:#x}, {states} states, {tries} try blocks')
		for t in range(tries):
			low, high, catch_high, catches, handlers = struct.unpack_from('<iiiiI', DATA, trymap - BASE + t * 20)
			print(f'  try states {low}-{high}')
			for c in range(catches):
				adjectives, type_desc, disp, handler = struct.unpack_from('<IIiI', DATA, handlers - BASE + c * 16)
				name = cstr(type_desc + 8) if type_desc else '...'
				flags = ' &' if adjectives & 8 else ''
				flags += ' const' if adjectives & 1 else ''
				print(f'    catch {name}{flags}  object at [ebp{disp:+#x}]  handler {handler:#x}')


if __name__ == '__main__':
	main()
