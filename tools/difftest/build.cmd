@echo off
rem Builds and runs the base-layer tests:
rem   difftest.exe   reconstructed functions against the original machine code (including Talk.cpp, Objects.cpp, Creatures.cpp and GUI.cpp)
rem   sysinctest.exe sysinc.cpp against the original (separate: difftest.exe stubs some of its functions)
rem   vlongtest.exe  vlong and TRSA against exact integer arithmetic (vlongcheck.py)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..\..
set OUT=%ROOT%\build\difftest
if not exist "%OUT%" mkdir "%OUT%"
set BASEFLAGS=/nologo /std:c++17 /MT /O2 /W3 /DTIBIA_MFC_SHIM /D_WIN32_WINNT=0x0601 /I"%ROOT%\src"
set FLAGS=%BASEFLAGS% /Fo"%OUT%\\"

cl %FLAGS% /EHa /Fe"%OUT%\difftest.exe" "%~dp0difftest.cpp" "%~dp0statustest.cpp" "%~dp0preferencestest.cpp" "%~dp0scripttest.cpp" "%~dp0talktest.cpp" "%~dp0objectstest.cpp" "%~dp0playertest.cpp" "%~dp0helptest.cpp" "%~dp0inventorytest.cpp" "%~dp0creaturestest.cpp" "%~dp0guitest.cpp" ^
	"%ROOT%\src\Utils.cpp" "%ROOT%\src\TXTEA.cpp" "%ROOT%\src\TMyException.cpp" ^
	"%ROOT%\src\Skins.cpp" "%ROOT%\src\Status.cpp" "%ROOT%\src\Preferences.cpp" ^
	"%ROOT%\src\Script.cpp" "%ROOT%\src\Talk.cpp" "%ROOT%\src\Objects.cpp" "%ROOT%\src\Player.cpp" "%ROOT%\src\Surface.cpp" "%ROOT%\src\Help.cpp" "%ROOT%\src\Inventory.cpp" "%ROOT%\src\MainFrm.cpp" "%ROOT%\src\Creatures.cpp" "%ROOT%\src\GUI.cpp" ^
	/link /BASE:0x20000000 /DYNAMICBASE:NO /FIXED >"%OUT%\cl.txt" || (type "%OUT%\cl.txt" & exit /b 1)
"%OUT%\difftest.exe" "%ROOT%\Tibia.exe" || exit /b 1

rem sysinc.cpp in its own executable: difftest.exe replaces some of its functions with recorders.
if not exist "%OUT%\sysinc" mkdir "%OUT%\sysinc"
cl %BASEFLAGS% /Fo"%OUT%\sysinc\\" /EHa /DDIFFTEST_SYSINC /Fe"%OUT%\sysinctest.exe" "%~dp0difftest.cpp" "%~dp0sysinctest.cpp" ^
	"%ROOT%\src\Utils.cpp" "%ROOT%\src\TXTEA.cpp" "%ROOT%\src\TMyException.cpp" "%ROOT%\src\sysinc.cpp" ^
	/link user32.lib /BASE:0x20000000 /DYNAMICBASE:NO /FIXED >"%OUT%\cl.txt" || (type "%OUT%\cl.txt" & exit /b 1)
"%OUT%\sysinctest.exe" "%ROOT%\Tibia.exe" || exit /b 1

cl %FLAGS% /EHsc /Fe"%OUT%\vlongtest.exe" "%~dp0vlongtest.cpp" "%ROOT%\src\vlong.cpp" ^
	"%ROOT%\src\TRSA.cpp" "%ROOT%\src\TMyException.cpp" >nul || exit /b 1
"%OUT%\vlongtest.exe" > "%OUT%\vlong.txt" || exit /b 1
python -I "%~dp0vlongcheck.py" "%OUT%\vlong.txt"
