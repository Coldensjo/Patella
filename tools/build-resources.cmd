@echo off
rem Build build\res\Patella.res from src\res\Patella.rc and check it against Tibia.exe.
rem 1. tools\extract_resources.py writes the cursors, the icon and the bitmaps of the
rem    local Tibia.exe to build\res (they are not in the repository).
rem 2. rc.exe of the Windows SDK compiles the script with build\res on the include path.
rem 3. tools\compare_resources.py compares every entry with Tibia.exe's .rsrc section.
rem Usage: tools\build-resources.cmd [path\to\Tibia.exe]   (default: Tibia.exe in the root)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
set IMAGE=%~1
if "%IMAGE%"=="" set IMAGE=%ROOT%\Tibia.exe
python -I "%ROOT%\tools\extract_resources.py" "%IMAGE%" "%ROOT%\build\res" || exit /b 1
rc /nologo /I "%ROOT%\build\res" /fo "%ROOT%\build\res\Patella.res" "%ROOT%\src\res\Patella.rc" || exit /b 1
python -I "%ROOT%\tools\compare_resources.py" "%IMAGE%" "%ROOT%\build\res\Patella.res"
