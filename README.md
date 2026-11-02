# Patella

A source reconstruction of the Tibia 7.72 client (`Tibia.exe`, built 2006-06-08,
MSVC 7.1), one original source file at a time, checked against the binary with
differential tests.

- `src/`: the reconstructed sources and headers.
- `notes/reconstruction.md`: the process, the tools and the status of each file.
- `notes/progress.md`: where the work stands and what comes next.
- `notes/decisions.md`: the judgement calls made along the way.
- `tools/`: triage, pseudocode cleanup, exception-table and call checks, and the
  differential tests (`tools/difftest`).
- `Patella.sln` / `Patella.vcxproj`: build `Patella.exe`, the reconstructed client, with
  Visual Studio 2026 and its MFC component (`tools\build-patella.cmd`; see "Building" in
  `notes/reconstruction.md`).

The client's own files (`Tibia.exe`, `Tibia.dat`, `Tibia.spr`, `Tibia.pic`) and
the raw decompiler output are not part of this repository.
