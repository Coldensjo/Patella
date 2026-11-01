# Reconstruction progress

Read this and `git log` first when resuming.

## Backlog

1. Script.cpp — **done** (commit "Reconstruct Script.cpp").
2. Preferences/Script gap (0x4ddc00-0x4df6ca) — **done**: loader/saver/ExitPreferences in Preferences.cpp, inline code in Script.h.
3. Player.cpp, incl. the error.txt reader (0x4da140) — **done**.
4. Small gaps — **done**: ContextMenus/Control (Control.cpp), start/Communication
   (InitCommunication + STL), Status/SurfaceDX (Surface.cpp + SurfaceDX.cpp range).
5. Objects.cpp and the Objects/Player gap — **done** (Claude Opus 5.5 delegate, reviewed, merged). The gap is all Player.obj.
6. Map.cpp and the gaps on both sides — **done** (Claude Opus 5.5 delegate, third attempt, reviewed, merged).
7. MapWindow.cpp and the MapWindow/Network gap.
8. Creatures.cpp and the Control/Creatures, Creatures/DialogElements gaps — **done** (Claude Opus 5.5 delegate, reviewed, merged; the second gap is DialogElements.cpp + Container.h).
9. Talk.cpp and the gaps next to it — **done** (ChatGPT gpt-6.1-sol, sent back once for names/shims, then merged).
10. The remaining files in link order, smallest first. Done: sysinc.cpp, Help.cpp, Inventory.cpp, MainFrm.cpp, GUI.cpp (delegates, merged).

## Current item

Item 10 (remaining files). Done so far in item 10: sysinc.cpp, Help.cpp,
Inventory.cpp, MainFrm.cpp. Settled but not reconstructed: SurfaceDX.cpp
range 0x4eae70-0x4ed19f, MainWindow.cpp gap 0x4ac010-0x4ac20d.

Plan (order changed on purpose, see decisions.md "Order of the GUI files"):
GUI.cpp next (it defines TWindow and the base classes that MainWindow,
DialogWindow, DialogElements, Dialogs, ContentWindows, ConsoleWindow,
ContextMenus and MapWindow derive from), as a delegate when a slot is free;
then MainWindow.cpp; then the derived files smallest first. In parallel,
the non-GUI files: SurfaceGL.cpp, surfaceDX9.cpp, SurfaceDX.cpp, graphics.cpp.

## Delegates (running)

- graphics.cpp + DialogWindow/graphics gap: same story; WIP f867cba on
  t3/reconstruct-graphics-cpp, continued on t3/graphics-cpp-finish.
Both told: real names only, no shims, replace their external.h views and
rename the callers. Review each against decomp/clean and the binary before
merging.

## Open questions

- None yet.

## Last difftest

11,245,343 checks against the original + 67 in sysinctest.exe, 0 failures; 3,201 vlong checks, 0 failures.

## Notes for later files

- The Bash tool expands `\n` inside heredocs and sed replacements: write
  C/C++ escapes with the Edit/Write tools, not through shell commands.
- tools/callcheck.py now resolves member calls named `Class_method` in
  names.tsv (own class first, else a unique method name).
- Map.cpp (delegate branch): TObject now clears itself (default member
  initializers), TRGBColor has constructors (palette index 0x425950, R/G/B,
  default), vector<T> has its destructor, Container.h has matrix<T> and
  matrix3d<T>. Communication.cpp's map code uses TObject; TCommunicationObject
  is left for the Inventory.cpp calls. The Map/MapWindow gap has names.tsv
  rows (TMapWindow_*) for MapWindow.cpp.
- The difftest image's static initialisers never run: call the original
  constructors of a file's static objects explicitly (maptest.cpp does it
  for Map.cpp's), and keep inputs away from paths that raise exceptions in
  the original (e.g. opening a missing file).
- tools\difftestuild.cmd can fail once without a message (a difftest.exe
  still locked from the previous run); kill difftest.exe and run it again.
