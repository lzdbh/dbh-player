@echo off
REM Build a Windows portable package (run in MSYS2 MINGW64 shell context or
REM with Qt's bin on PATH). Unsigned dev package; signing is a release step.
REM Usage: scripts\package-windows.bat [qt-prefix]
setlocal
cd /d "%~dp0\.."

set QT_PREFIX=%1
if "%QT_PREFIX%"=="" set QT_PREFIX=C:\msys64\mingw64

cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=%QT_PREFIX% -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build-release || exit /b 1

if not exist dist\windows mkdir dist\windows
copy build-release\dbhx-player.exe dist\windows\ || exit /b 1
windeployqt --release --qmldir qml dist\windows\dbhx-player.exe || exit /b 1

echo Done: dist\windows\dbhx-player.exe
endlocal