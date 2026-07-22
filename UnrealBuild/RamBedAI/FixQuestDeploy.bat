@echo off
setlocal

set ADB=C:\Users\tmans\AppData\Local\Android\Sdk\platform-tools\adb.exe
set PACKAGE=com.YourCompany.RamBedAI

if not exist "%ADB%" (
    echo adb.exe not found at %ADB%
    exit /b 1
)

echo Restarting adb and clearing stale Quest deploy cache...
echo.

"%ADB%" kill-server
ping -n 3 127.0.0.1 >nul
"%ADB%" start-server
ping -n 3 127.0.0.1 >nul

"%ADB%" devices
echo.

echo Stopping %PACKAGE%...
"%ADB%" shell am force-stop %PACKAGE%

echo Removing partial Android File Server deploy files...
"%ADB%" shell rm -rf /storage/emulated/0/Android/data/%PACKAGE%/files/UnrealGame

echo Done.
echo.
echo Next: Launch to Quest again from the editor (single click, wait for BUILD SUCCESSFUL).
echo APK content is now packaged inside the app (AFS disabled), so deploy should be more reliable.
exit /b 0