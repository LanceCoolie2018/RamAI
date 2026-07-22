@echo off
setlocal

set UE_ROOT=C:\Program Files\Epic Games\UE_5.6
set PROJECT=D:\RamAI\UnrealBuild\RamBedAI\RamBedAI.uproject

echo Cleaning stale generated C++ stubs and corrupted build rules...
if exist "%~dp0Intermediate\Source" rmdir /s /q "%~dp0Intermediate\Source"
if exist "%~dp0Intermediate\Build\BuildRules\RamBedAIModuleRules.dll" del /f /q "%~dp0Intermediate\Build\BuildRules\RamBedAIModuleRules.dll"
if exist "%~dp0Intermediate\Build\BuildRules\RamBedAIModuleRules.pdb" del /f /q "%~dp0Intermediate\Build\BuildRules\RamBedAIModuleRules.pdb"

echo.
echo Generating Visual Studio project files...
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="%PROJECT%" -game -engine -progress

if errorlevel 1 (
    echo.
    echo Project file generation failed. See the log above.
    pause
    exit /b 1
)

echo.
echo Applying Visual Studio debugger settings...
copy /Y "%~dp0Build\RamBedAI.vcxproj.user" "%~dp0Intermediate\ProjectFiles\RamBedAI.vcxproj.user" >nul

echo Patching generated vcxproj project path...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\PatchVcxprojPath.ps1" -VcxprojPath "%~dp0Intermediate\ProjectFiles\RamBedAI.vcxproj"

echo.
echo Done. Open RamBedAI.sln or double-click RamBedAI.uproject.
echo.
echo Visual Studio run/debug checklist:
echo   1. Set startup project to RamBedAI (under Games), not UE5.
echo   2. Set configuration to Development Editor and platform to Win64.
echo   3. Run BuildEditor.bat once, then press F5 (uses -skipcompile).
echo   4. Or use the "Launch RamBedAI Editor" debug profile.
echo   5. If the debugger stops on Oculus/Meta exceptions, click Continue
echo      or disable Win32/C++ exceptions in Debug - Exception Settings.
echo.
echo First editor launch will compile C++ — allow several minutes.
pause