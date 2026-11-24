; DBHX Player installer (per-user, no admin required).
; Build with: makensis scripts\dbhx-player.nsi
; Expects dist\version.nsh to define VERSION (and optionally ARCH).

!if /FileExists "..\dist\version.nsh"
  !include "..\dist\version.nsh"
!endif
!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef ARCH
  !define ARCH "x64"
!endif

!macro RegisterVideoType extension
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\SupportedTypes" "${extension}" ""
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\Capabilities\FileAssociations" "${extension}" "Applications\dbhx-player.exe"
!macroend

Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
BrandingText "DBHX Player ${VERSION} (${ARCH})"
!define MUI_ICON "..\cmake\dbhx-installer.ico"
!define MUI_UNICON "..\cmake\dbhx-installer.ico"

!include "MUI2.nsh"
!include "FileFunc.nsh"

Name "DBHX Player"
OutFile "..\dist\DBHX-Player-${VERSION}-win${ARCH}-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\DBHX Player"
InstallDirRegKey HKCU "Software\DBHX\DBHX Player" "InstallDir"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\dbhx-player.exe"
!define MUI_FINISHPAGE_RUN_TEXT "运行 DBHX Player"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

Section "DBHX Player" SecMain
  SetOutPath "$INSTDIR"
  File /r "..\dist\windows\*.*"

  WriteRegStr HKCU "Software\DBHX\DBHX Player" "InstallDir" "$INSTDIR"

  ; Register for "Open with" without changing the user's default application.
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe" "" "DBHX Player"
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe" "FriendlyAppName" "DBHX Player"
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\shell\open" "FriendlyAppName" "DBHX Player"
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\shell\open\command" "" '"$INSTDIR\dbhx-player.exe" "%1"'
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\Capabilities" "ApplicationName" "DBHX Player"
  WriteRegStr HKCU "Software\Classes\Applications\dbhx-player.exe\Capabilities" "ApplicationDescription" "Cross-platform video player"
  WriteRegStr HKCU "Software\RegisteredApplications" "DBHX Player" "Software\Classes\Applications\dbhx-player.exe"
  !insertmacro RegisterVideoType ".mp4"
  !insertmacro RegisterVideoType ".mkv"
  !insertmacro RegisterVideoType ".avi"
  !insertmacro RegisterVideoType ".mov"
  !insertmacro RegisterVideoType ".flv"
  !insertmacro RegisterVideoType ".wmv"
  !insertmacro RegisterVideoType ".ts"
  !insertmacro RegisterVideoType ".webm"
  !insertmacro RegisterVideoType ".m4v"
  !insertmacro RegisterVideoType ".mpg"
  !insertmacro RegisterVideoType ".mpeg"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "DisplayName" "DBHX Player"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "Publisher" "DBHX"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "DisplayIcon" "$INSTDIR\dbhx-player.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "NoRepair" 1

  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer" "EstimatedSize" $0

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateShortcut "$DESKTOP\DBHX Player.lnk" "$INSTDIR\dbhx-player.exe"
  CreateDirectory "$SMPROGRAMS\DBHX Player"
  CreateShortcut "$SMPROGRAMS\DBHX Player\DBHX Player.lnk" "$INSTDIR\dbhx-player.exe"
  CreateShortcut "$SMPROGRAMS\DBHX Player\卸载 DBHX Player.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\DBHX Player.lnk"
  Delete "$SMPROGRAMS\DBHX Player\DBHX Player.lnk"
  Delete "$SMPROGRAMS\DBHX Player\卸载 DBHX Player.lnk"
  RMDir "$SMPROGRAMS\DBHX Player"

  RMDir /r "$INSTDIR"

  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\DBHXPlayer"
  DeleteRegKey HKCU "Software\DBHX\DBHX Player"
  DeleteRegValue HKCU "Software\RegisteredApplications" "DBHX Player"
  DeleteRegKey HKCU "Software\Classes\Applications\dbhx-player.exe"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
