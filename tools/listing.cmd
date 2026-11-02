@echo off
rem Compiles the given src files like compile-check.cmd (/EHsc, MFC shim) and writes assembly listings
rem (/FAs) and objects to build\listing, for tools/ehcompare.py and comparisons with tools/xctable.py.
rem Usage: tools\listing.cmd [/EHa] File.cpp ...   (names relative to src)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
if not exist "%ROOT%\build\listing" mkdir "%ROOT%\build\listing"
set EH=/EHsc
if "%1"=="/EHa" (
	set EH=/EHa
	shift
)
set FILES=
:loop
if "%1"=="" goto done
set FILES=%FILES% "%ROOT%\src\%1"
shift
goto loop
:done
cl /nologo /c /std:c++17 %EH% /MT /O2 /W3 /permissive- /Zc:strictStrings- /D_WIN32_WINNT=0x0601 /DTIBIA_MFC_SHIM /I"%ROOT%\src" /FAs /Fa"%ROOT%\build\listing\\" /Fo"%ROOT%\build\listing\\" %FILES%
