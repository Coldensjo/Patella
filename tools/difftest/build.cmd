@echo off
rem Builds and runs the base-layer tests:
rem   difftest.exe   reconstructed functions against the original machine code (including Talk.cpp and Objects.cpp)
rem   vlongtest.exe  vlong and TRSA against exact integer arithmetic (vlongcheck.py)
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VCVARS%" x86 >nul || exit /b 1
set ROOT=%~dp0..\..
set OUT=%ROOT%\build\difftest
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /std:c++17 /MT /O2 /W3 /DTIBIA_MFC_SHIM /D_WIN32_WINNT=0x0601 /I"%ROOT%\src" /Fo"%OUT%\\"

cl %FLAGS% /EHa /Fe"%OUT%\difftest.exe" "%~dp0difftest.cpp" "%~dp0statustest.cpp" "%~dp0preferencestest.cpp" "%~dp0scripttest.cpp" "%~dp0talktest.cpp" "%~dp0objectstest.cpp" "%~dp0playertest.cpp" ^
	"%ROOT%\src\Utils.cpp" "%ROOT%\src\TXTEA.cpp" "%ROOT%\src\TMyException.cpp" ^
	"%ROOT%\src\Skins.cpp" "%ROOT%\src\Status.cpp" "%ROOT%\src\Preferences.cpp" ^
	"%ROOT%\src\Script.cpp" "%ROOT%\src\Talk.cpp" "%ROOT%\src\Objects.cpp" "%ROOT%\src\Player.cpp" ^
	/link /BASE:0x20000000 /DYNAMICBASE:NO /FIXED >"%OUT%\cl.txt" || (type "%OUT%\cl.txt" & exit /b 1)
"%OUT%\difftest.exe" "%ROOT%\Tibia.exe" || exit /b 1

cl %FLAGS% /EHsc /Fe"%OUT%\vlongtest.exe" "%~dp0vlongtest.cpp" "%ROOT%\src\vlong.cpp" ^
	"%ROOT%\src\TRSA.cpp" "%ROOT%\src\TMyException.cpp" >nul || exit /b 1
"%OUT%\vlongtest.exe" > "%OUT%\vlong.txt" || exit /b 1
python -I "%~dp0vlongcheck.py" "%OUT%\vlong.txt"
