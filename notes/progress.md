# Reconstruction progress

Read this and `git log` first when resuming.

## Backlog

1. Script.cpp — **done** (commit "Reconstruct Script.cpp").
2. Preferences/Script gap (0x4ddc00-0x4df6ca) — next.
3. Player.cpp, incl. the error.txt reader (0x4da140).
4. Small gaps: ContextMenus/Control (0x445900-0x445aeb), start/Communication
   (0x401000-0x40434f), Status/SurfaceDX (0x4eaa80-0x4ed19f).
5. Objects.cpp and the Objects/Player gap — **done** (Claude Opus 5.5 delegate, reviewed, merged). The gap is all Player.obj.
6. Map.cpp and the gaps on both sides.
7. MapWindow.cpp and the MapWindow/Network gap.
8. Creatures.cpp and the Control/Creatures, Creatures/DialogElements gaps.
9. Talk.cpp and the gaps next to it — **done** (ChatGPT gpt-6.1-sol, sent back once for names/shims, then merged).
10. The remaining files in link order, smallest first.

## Current item

2: Preferences/Script gap. The Script.h inline functions in it
(0x4df550-0x4df6cf) are settled (file-ranges.tsv); left are the Tibia.cfg
loader 0x4ddc00, the saver 0x4de890 and 0x4df520.

## Delegates (running)

- Map.cpp + MainWindow/Map and Map/MapWindow gaps (item 6): ChatGPT
  gpt-6.1-sol, own worktree (the first Claude attempt failed on a rate limit).
- Creatures.cpp + Control/Creatures and Creatures/DialogElements gaps
  (item 8): Claude Opus 5.5, own worktree.
Both told: real names only, no shims, replace their external.h views and
rename the callers. Review each against decomp/clean and the binary before
merging.

## Open questions

- None yet.

## Last difftest

2,796,382 checks against the original, 0 failures; 3,201 vlong checks, 0 failures.

## Notes for later files

- The Bash tool expands `\n` inside heredocs and sed replacements: write
  C/C++ escapes with the Edit/Write tools, not through shell commands.
- tools/callcheck.py now resolves member calls named `Class_method` in
  names.tsv (own class first, else a unique method name).
