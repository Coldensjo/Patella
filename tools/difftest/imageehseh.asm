; Registers imageeh.cpp's ImageHandlerTrampoline as a safe exception handler
; of difftest.exe (/SAFESEH): Windows calls only the handlers listed in an
; image's SafeSEH table, and the C++ compiler cannot list a function there.
; See imageeh.cpp.
	.386
	.model flat, c
ImageHandlerTrampoline PROTO
	.safeseh ImageHandlerTrampoline
	end
