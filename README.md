# Patella

A source reconstruction of the Tibia 7.72 client (`Tibia.exe`, built 2006-06-08,
MSVC 7.1), one original source file at a time, checked against the binary with
differential tests.

- `src/`: the reconstructed sources and headers.
- `notes/reconstruction.md`: the process, the tools and the status of each file.
- `notes/progress.md`: where the work stands and what comes next.
- `notes/decisions.md`: the judgement calls made along the way.
- `tools/`: the build, the differential tests (`tools/difftest`) and the comparisons of
  the running clients (traffic, written files, rendering, network).
- `Patella.sln` / `Patella.vcxproj`: build `Patella.exe`, the reconstructed client, with
  Visual Studio 2026 and its MFC component (`tools\build-patella.cmd`; see "Building" in
  `notes/reconstruction.md`).

The client's own files (`Tibia.exe`, `Tibia.dat`, `Tibia.spr`, `Tibia.pic`) and
the decompilation workspace (the decompiler output, the name and symbol databases,
the analysis notes and tools) are not part of this repository; `tools\decompilation.ps1`
moves the local copy between `decompilation/` and its working paths (see CLAUDE.md).

## License

MIT, see [LICENSE](LICENSE). It covers this repository's own work; `src/Extern` keeps its own licenses, and
Tibia and its files belong to CipSoft GmbH and are not licensed here.
