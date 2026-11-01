"""Checks vlongtest.exe output against Python's exact integer arithmetic.

Usage: python -I tools/difftest/vlongcheck.py <vlongtest output file>
"""
import sys

MODULUS = int(
	'124710459426827943004376449897985582167801707960697037164044904862948569380850421396904597686953877022394604239428'
	'185498284169068581802277612081027966724336319448537811441719076484340922854929273517308661370727105382899118999403'
	'808045846444647284499123164879035103627004668521005328367415259939915284902061793')


def main():
	checks = failures = 0
	for number, line in enumerate(open(sys.argv[1], encoding='ascii'), 1):
		parts = line.split()
		if not parts:
			continue
		op = parts[0]
		if op == 'rsa':
			plain, cipher = int(parts[1], 16), int(parts[2], 16)
			expected = pow(plain, 65537, MODULUS)
		elif op == 'dec':
			expected, cipher = int(parts[1]), int(parts[2], 16)
		else:
			a, b, cipher = int(parts[1], 16), int(parts[2], 16), int(parts[3], 16)
			expected = {'+': a + b, '-': a - b, '*': a * b, '/': a // b, '%': a % b}[op]
		checks += 1
		if expected != cipher:
			failures += 1
			if failures <= 10:
				print(f'line {number}: {op} gives {cipher:x}, expected {expected:x}')
	print(f'{checks} checks, {failures} failures')
	return 1 if failures else 0


if __name__ == '__main__':
	sys.exit(main())
