@echo off
setlocal

set UE_ROOT=C:\Program Files\Epic Games\UE_5.6
set PROJECT=D:\RamAI\UnrealBuild\RamBedAI\RamBedAI.uproject

echo Building RamBedAI Editor (Development)...
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" RamBedAIEditor Win64 Development -Project="%PROJECT%" -WaitMutex -Progress

if errorlevel 1 (
    echo.
    echo Build failed. Fix compile errors before launching the editor.
    pause
    exit /b 1
)

echo.
echo Build succeeded. You can now press F5 in Visual Studio or run LaunchEditor.bat.
pause