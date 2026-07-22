@echo off
setlocal

set UE_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
set PROJECT=%~dp0RamBedAI.uproject
set SCRIPT=%~dp0Scripts\SetupRamBedScreens.py

if not exist "%UE_EDITOR%" (
    echo Unreal Editor not found at:
    echo   %UE_EDITOR%
    exit /b 1
)

echo Running Option C: remove legacy BP_HA_AnchoredDisplay from RamBed map...
echo.

"%UE_EDITOR%" "%PROJECT%" /Game/RamBed -ExecutePythonScript="%SCRIPT%" -unattended -nop4 -nosplash -stdout

if errorlevel 1 (
    echo.
    echo Script failed. Close the Unreal Editor if it is open, then run this again.
    exit /b 1
)

echo.
echo Done. Legacy HA Blueprint removed from map; C++ auto-spawn is active.
exit /b 0