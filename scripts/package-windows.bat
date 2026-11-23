@echo off
REM Build a Windows portable package + installer (run in MSYS2 MINGW64 shell
REM context or with Qt's bin on PATH). Unsigned; signing is a release step.
REM Usage: scripts\package-windows.bat [qt-prefix]
setlocal
cd /d "%~dp0\.."

set QT_PREFIX=%1
if "%QT_PREFIX%"=="" set QT_PREFIX=C:\msys64\mingw64

cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=%QT_PREFIX% -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build-release || exit /b 1

if not exist dist\windows mkdir dist\windows
copy /y build-release\dbhx-player.exe dist\windows\ || exit /b 1
windeployqt --release --compiler-runtime --qmldir qml dist\windows\dbhx-player.exe || exit /b 1

REM Version is defined by the project() line in CMakeLists.txt.
for /f "tokens=3" %%v in ('findstr /r /c:"^project" CMakeLists.txt') do set APP_VERSION=%%v
echo Version: %APP_VERSION%

> dist\version.nsh echo !define VERSION "%APP_VERSION%"
>> dist\version.nsh echo !define ARCH "x64"

set MAKENSIS=%QT_PREFIX%\bin\makensis.exe
if not exist "%MAKENSIS%" set MAKENSIS=makensis
"%MAKENSIS%" scripts\dbhx-player.nsi || exit /b 1

echo Done: dist\windows\dbhx-player.exe
echo Installer: dist\DBHX-Player-%APP_VERSION%-winx64-Setup.exe
endlocal