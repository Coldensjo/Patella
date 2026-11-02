@echo off
rem Build the capture helper of the rendering comparison (issue #20): build\capture\patcap.dll,
rem 32-bit, with the MSVC of Visual Studio 2026 (as build-patella.cmd). tools\compare_rendering.py
rem runs this itself when the DLL is missing or older than its source.
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
set OUT=%ROOT%\build\capture
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /O2 /W4 /WX /MT /LD "%ROOT%\tools\capture\patcap.c" /Fo"%OUT%\\" /Fe"%OUT%\patcap.dll" /link user32.lib gdi32.lib winmm.lib ddraw.lib opengl32.lib || exit /b 1
echo build-capture.cmd: built %OUT%\patcap.dll
