# Reconstruction progress

Read this and `git log` first when resuming.

## Backlog

1. Script.cpp — **done** (commit "Reconstruct Script.cpp").
2. Preferences/Script gap (0x4ddc00-0x4df6ca) — **done**: loader/saver/ExitPreferences in Preferences.cpp, inline code in Script.h.
3. Player.cpp, incl. the error.txt reader (0x4da140) — **done**.
4. Small gaps — **done**: ContextMenus/Control (Control.cpp), start/Communication
   (InitCommunication + STL), Status/SurfaceDX (Surface.cpp + SurfaceDX.cpp range).
5. Objects.cpp and the Objects/Player gap — **done** (Claude Opus 5.5 delegate, reviewed, merged). The gap is all Player.obj.
6. Map.cpp and the gaps on both sides.
7. MapWindow.cpp and the MapWindow/Network gap.
8. Creatures.cpp and the Control/Creatures, Creatures/DialogElements gaps.
9. Talk.cpp and the gaps next to it — **done** (ChatGPT gpt-6.1-sol, sent back once for names/shims, then merged).
10. The remaining files in link order, smallest first.

## Current item

6 (Map.cpp) and 8 (Creatures.cpp) are with delegates; 7 (MapWindow.cpp)
waits for Map.cpp. Meanwhile: item 10, the smallest remaining files.

## Delegates (running)

- Map.cpp (item 6): third attempt, Claude Opus 5.5 delegate (the first hit
  a Claude rate limit, the gpt-6.1-sol retry the ChatGPT usage limit).
- Creatures.cpp + Control/Creatures and Creatures/DialogElements gaps
  (item 8): Claude Opus 5.5, own worktree.
Both told: real names only, no shims, replace their external.h views and
rename the callers. Review each against decomp/clean and the binary before
merging.

## Open questions

- None yet.

## Last difftest

2,870,603 checks against the original, 0 failures; 3,201 vlong checks, 0 failures.

## Notes for later files

- The Bash tool expands `\n` inside heredocs and sed replacements: write
  C/C++ escapes with the Edit/Write tools, not through shell commands.
- tools/callcheck.py now resolves member calls named `Class_method` in
  names.tsv (own class first, else a unique method name).
