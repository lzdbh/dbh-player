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

Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
BrandingText "DBHX Player ${VERSION} (${ARCH})"

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
SectionEnd