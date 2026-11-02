@echo off
rem Build Patella.exe from Patella.vcxproj with the MSBuild of Visual Studio 2026 (issue #16) and
rem check its resources against Tibia.exe.
rem 1. MSBuild builds every src\*.cpp, src\Extern\regexp\*.c and src\res\Patella.rc (x86, static
rem    MFC, /MT); a warning fails the build (-warnAsError). The project extracts the resource
rem    pictures from the local Tibia.exe into build\res when they are missing.
rem 2. tools\compare_resources.py compares Patella.exe's resources with Tibia.exe's (0 differences).
rem Usage: tools\build-patella.cmd [Release|Debug] [more MSBuild options, e.g. -t:Rebuild]
rem Output: build\<configuration>\Patella.exe; the MSBuild log is build\<configuration>\build.log.
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Release
if not "%~1"=="" shift
if not exist "%VCToolsInstallDir%atlmfc\include\afxwin.h" (
	echo build-patella.cmd: the Visual Studio MFC component is not installed
	exit /b 1
)
if not exist "%ROOT%\build\%CONFIG%" mkdir "%ROOT%\build\%CONFIG%"
msbuild "%ROOT%\Patella.vcxproj" -nologo -m -v:minimal -warnAsError -p:Configuration=%CONFIG% -p:Platform=Win32 -flp:logfile="%ROOT%\build\%CONFIG%\build.log";verbosity=normal %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 (
	echo build-patella.cmd: MSBuild failed
	exit /b 1
)
python -I "%ROOT%\tools\compare_resources.py" "%ROOT%\Tibia.exe" "%ROOT%\build\%CONFIG%\Patella.exe" > "%ROOT%\build\%CONFIG%\resources.txt"
if errorlevel 1 (
	type "%ROOT%\build\%CONFIG%\resources.txt"
	echo build-patella.cmd: the resources differ from Tibia.exe's
	exit /b 1
)
findstr /c:" differences" "%ROOT%\build\%CONFIG%\resources.txt"
echo build-patella.cmd: built %ROOT%\build\%CONFIG%\Patella.exe
