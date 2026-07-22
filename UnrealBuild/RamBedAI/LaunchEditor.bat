@echo off
setlocal

set UE_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0RamBedAI.uproject

if not exist "%UE_EDITOR%" (
    echo Unreal Editor not found at:
    echo   %UE_EDITOR%
    echo Update LaunchEditor.bat if UE is installed elsewhere.
    pause
    exit /b 1
)

start "" "%UE_EDITOR%" "%PROJECT%"