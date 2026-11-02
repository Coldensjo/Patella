@echo off
rem Compile-check reconstructed sources with modern MSVC (x86, static MFC).
rem Compiles only; linking needs the files that are not reconstructed yet.
rem Without the MFC component installed it falls back to src\compat\mfc_shim.h.
rem Usage: tools\compile-check.cmd [file.cpp ...]   (default: every src\*.cpp and the C files of src\Extern\regexp)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
if not exist "%ROOT%\build\check" mkdir "%ROOT%\build\check"
set SHIM=
if not exist "%VCToolsInstallDir%atlmfc\include\afxwin.h" (
	echo MFC is not installed; checking against src\compat\mfc_shim.h
	set SHIM=/DTIBIA_MFC_SHIM
)
set FILES=%*
if "%FILES%"=="" set FILES=%ROOT%\src\*.cpp %ROOT%\src\Extern\regexp\*.c
cl /nologo /c /std:c++17 /EHsc /MT /O2 /W3 /permissive- /Zc:strictStrings- /D_WIN32_WINNT=0x0601 %SHIM% /I"%ROOT%\src" /Fo"%ROOT%\build\check\\" %FILES%
