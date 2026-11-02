@echo off
rem Builds and runs the base-layer tests:
rem   difftest.exe   reconstructed functions against the original machine code (including Talk.cpp, Objects.cpp, Creatures.cpp, Map.cpp, graphics.cpp, GUI.cpp, DialogWindow.cpp, ContextMenus.cpp, ConsoleWindow.cpp, ContentWindows.cpp, DialogElements.cpp, Dialogs.cpp, surfaceDX9.cpp, SurfaceGL.cpp, MapWindow.cpp, MainWindow.cpp and the regexp of src\Extern\regexp)
rem   sysinctest.exe sysinc.cpp against the original (separate: difftest.exe stubs some of its functions)
rem   vlongtest.exe  vlong and TRSA against exact integer arithmetic (vlongcheck.py)
setlocal
rem A step that fails stops the script with exit code 1 and a message: the
rem compiler's output, or difftest.exe's report of the child's exit code and
rem the test it was in (a crash too). A passing run ends with
rem "build.cmd: all tests passed". From Git Bash run it as
rem cmd //c 'tools\difftest\build.cmd' (Git Bash turns cmd /c into cmd C:/).
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "OUT=%ROOT%\build\difftest"
if not exist "%OUT%" mkdir "%OUT%"
call "%VCVARS%" x86 >"%OUT%\vcvars.txt" 2>&1 || (call :failed "vcvarsall.bat x86" "%OUT%\vcvars.txt" & exit /b 1)
where /q cl.exe || (echo build.cmd: cl.exe is not on PATH after vcvarsall.bat & exit /b 1)
where /q python.exe || (echo build.cmd: python.exe is not on PATH, vlongcheck.py needs it & exit /b 1)
rem The tests write to fixed directories under %TEMP%; a private TEMP per checkout keeps
rem concurrent runs in other worktrees from overwriting each other's files.
if not exist "%OUT%\tmp" mkdir "%OUT%\tmp"
set "TEMP=%OUT%\tmp"
set "TMP=%OUT%\tmp"
set BASEFLAGS=/nologo /std:c++17 /MT /O2 /W3 /DTIBIA_MFC_SHIM /D_WIN32_WINNT=0x0601 /I"%ROOT%\src"
set FLAGS=%BASEFLAGS% /Fo"%OUT%\\"
call :remove difftest.exe || exit /b 1

cl %FLAGS% /EHa /Fe"%OUT%\difftest.exe" "%~dp0difftest.cpp" "%~dp0statustest.cpp" "%~dp0preferencestest.cpp" "%~dp0scripttest.cpp" "%~dp0talktest.cpp" "%~dp0objectstest.cpp" "%~dp0playertest.cpp" "%~dp0helptest.cpp" "%~dp0inventorytest.cpp" "%~dp0creaturestest.cpp" "%~dp0maptest.cpp" "%~dp0graphicstest.cpp" "%~dp0guitest.cpp" "%~dp0dialogwindowtest.cpp" "%~dp0contextmenustest.cpp" "%~dp0consolewindowtest.cpp" "%~dp0contentwindowstest.cpp" "%~dp0dialogelementstest.cpp" "%~dp0dialogstest.cpp" "%~dp0surfacedx9test.cpp" "%~dp0surfacegltest.cpp" "%~dp0containertest.cpp" "%~dp0surfacedxtest.cpp" "%~dp0mapwindowtest.cpp" "%~dp0mainwindowtest.cpp" "%~dp0elementfakes.cpp" "%~dp0regexptest.cpp" ^
	"%ROOT%\src\Utils.cpp" "%ROOT%\src\TXTEA.cpp" "%ROOT%\src\TMyException.cpp" ^
	"%ROOT%\src\Skins.cpp" "%ROOT%\src\Status.cpp" "%ROOT%\src\Preferences.cpp" ^
	"%ROOT%\src\Script.cpp" "%ROOT%\src\Talk.cpp" "%ROOT%\src\Objects.cpp" "%ROOT%\src\Player.cpp" "%ROOT%\src\Surface.cpp" "%ROOT%\src\SurfaceDX.cpp" "%ROOT%\src\surfaceDX9.cpp" "%ROOT%\src\SurfaceGL.cpp" "%ROOT%\src\Help.cpp" "%ROOT%\src\Inventory.cpp" "%ROOT%\src\MainFrm.cpp" "%ROOT%\src\Creatures.cpp" "%ROOT%\src\Map.cpp" "%ROOT%\src\graphics.cpp" "%ROOT%\src\GUI.cpp" "%ROOT%\src\DialogWindow.cpp" "%ROOT%\src\ContextMenus.cpp" "%ROOT%\src\ConsoleWindow.cpp" "%ROOT%\src\ContentWindows.cpp" "%ROOT%\src\DialogElements.cpp" "%ROOT%\src\Dialogs.cpp" "%ROOT%\src\MapWindow.cpp" "%ROOT%\src\MainWindow.cpp" "%ROOT%\src\Extern\regexp\regexp.c" ^
	/link ddraw.lib dxguid.lib winmm.lib user32.lib /BASE:0x20000000 /DYNAMICBASE:NO /FIXED >"%OUT%\cl.txt" || (type "%OUT%\cl.txt" & exit /b 1)
"%OUT%\difftest.exe" "%ROOT%\Tibia.exe" || exit /b 1

rem sysinc.cpp in its own executable: difftest.exe replaces some of its functions with recorders.
if not exist "%OUT%\sysinc" mkdir "%OUT%\sysinc"
call :remove sysinctest.exe || exit /b 1
cl %BASEFLAGS% /Fo"%OUT%\sysinc\\" /EHa /DDIFFTEST_SYSINC /Fe"%OUT%\sysinctest.exe" "%~dp0difftest.cpp" "%~dp0sysinctest.cpp" ^
	"%ROOT%\src\Utils.cpp" "%ROOT%\src\TXTEA.cpp" "%ROOT%\src\TMyException.cpp" "%ROOT%\src\sysinc.cpp" ^
	/link user32.lib /BASE:0x20000000 /DYNAMICBASE:NO /FIXED >"%OUT%\cl.txt" 2>&1 || (call :failed "compiling sysinctest.exe" "%OUT%\cl.txt" & exit /b 1)
"%OUT%\sysinctest.exe" "%ROOT%\Tibia.exe" || (call :failed "sysinctest.exe" & exit /b 1)

call :remove vlongtest.exe || exit /b 1
cl %FLAGS% /EHsc /Fe"%OUT%\vlongtest.exe" "%~dp0vlongtest.cpp" "%ROOT%\src\vlong.cpp" ^
	"%ROOT%\src\TRSA.cpp" "%ROOT%\src\TMyException.cpp" >"%OUT%\cl.txt" 2>&1 || (call :failed "compiling vlongtest.exe" "%OUT%\cl.txt" & exit /b 1)
"%OUT%\vlongtest.exe" >"%OUT%\vlong.txt" || (call :failed "vlongtest.exe" & exit /b 1)
python -I "%~dp0vlongcheck.py" "%OUT%\vlong.txt" || (call :failed "vlongcheck.py" & exit /b 1)
echo build.cmd: all tests passed
exit /b 0

rem %1 the step that failed, %2 a file with its output (optional); errorlevel
rem still holds the step's exit code.
:failed
set "CODE=%errorlevel%"
if not "%~2"=="" type "%~2"
echo build.cmd: %~1 failed (exit code %CODE%)
exit /b 1

rem Deletes a test program before it is linked again. A copy still running
rem from an interrupted run locks the file: the copies of this checkout are
rem stopped, found by their full path (other worktrees run their own copies at
rem the same time), and the file is deleted again.
:remove
if not exist "%OUT%\%~1" exit /b 0
del "%OUT%\%~1" >nul 2>&1
if not exist "%OUT%\%~1" exit /b 0
echo build.cmd: %OUT%\%~1 is in use; stopping the copies of this checkout
set "STALE=%OUT%\%~1"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -Command ^
	"Get-Process | Where-Object { $_.Path -eq $env:STALE } | ForEach-Object { $_.Kill(); [void]$_.WaitForExit(10000) };" ^
	"for ($i = 0; $i -lt 20 -and (Test-Path -LiteralPath $env:STALE); $i++) { Remove-Item -LiteralPath $env:STALE -Force -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 500 }"
if exist "%OUT%\%~1" (echo build.cmd: cannot delete %OUT%\%~1 & exit /b 1)
exit /b 0
