@echo off
rem Compile-check reconstructed sources with modern MSVC (x86, static MFC).
rem Compiles only; tools\build-patella.cmd links Patella.exe (issue #16).
rem It uses the real MFC of the Visual Studio MFC component; without it, or with the environment
rem variable TIBIA_MFC_SHIM set (to anything), it checks against src\compat\mfc_shim.h instead.
rem Usage: tools\compile-check.cmd [file.cpp ...]   (default: every src\*.cpp and the C files of src\Extern\regexp)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
if not exist "%ROOT%\build\check" mkdir "%ROOT%\build\check"
set SHIM=
if defined TIBIA_MFC_SHIM (
	echo TIBIA_MFC_SHIM is set; checking against src\compat\mfc_shim.h
	set SHIM=/DTIBIA_MFC_SHIM
) else if not exist "%VCToolsInstallDir%atlmfc\include\afxwin.h" (
	echo MFC is not installed; checking against src\compat\mfc_shim.h
	set SHIM=/DTIBIA_MFC_SHIM
)
set FILES=%*
if "%FILES%"=="" set FILES=%ROOT%\src\*.cpp %ROOT%\src\Extern\regexp\*.c
cl /nologo /c /std:c++17 /EHsc /MT /O2 /W3 /permissive- /Zc:strictStrings- /D_WIN32_WINNT=0x0601 %SHIM% /I"%ROOT%\src" /Fo"%ROOT%\build\check\\" %FILES%
