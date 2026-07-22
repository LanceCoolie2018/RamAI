@echo off
setlocal

set UE_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=D:\RamAI\UnrealBuild\RamBedAI\RamBedAI.uproject

echo.
echo Step 1: Launching editor WITHOUT the debugger.
echo        OpenXR/Oculus throws many harmless exceptions that freeze F5 startup.
echo.
start "" "%UE_EDITOR%" "%PROJECT%" -skipcompile

echo.
echo Step 2: Wait until the editor window is fully open, then in Visual Studio:
echo        Debug - Attach to Process - UnrealEditor.exe
echo.
echo Tip: In Exception Settings, uncheck Win32 Exceptions and C++ Exceptions
echo      so Oculus/OpenXR does not pause startup.
pause