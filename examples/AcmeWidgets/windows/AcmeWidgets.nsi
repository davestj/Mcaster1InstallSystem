; ──────────────────────────────────────────────────────────────────────────────
; AcmeWidgets.nsi  —  NSIS installer script for ACME Widgets 2.1.0
;
; Build:
;   makensis AcmeWidgets.nsi
;   OR: .\build-windows-installer.ps1
;
; Produces:  dist\AcmeWidgets-2.1.0-win64-setup.exe
; ──────────────────────────────────────────────────────────────────────────────

Unicode True

; ── NSIS Includes ─────────────────────────────────────────────────────────────
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

; ── Application metadata ──────────────────────────────────────────────────────
!define APP_NAME         "Acme Widgets"
!define APP_VERSION      "2.1.0"
!define APP_PUBLISHER    "ACME Corporation"
!define APP_URL          "https://www.acme-corp.example.com/widgets"
!define APP_EXE          "acme-widgets.bat"
!define APP_ID           "com.acme-corp.acme-widgets"
!define INSTALL_REG_KEY  "Software\ACME Corporation\Acme Widgets"
!define UNINSTALL_KEY    "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_ID}"

; ── Installer settings ────────────────────────────────────────────────────────
Name "${APP_NAME} ${APP_VERSION}"
OutFile "..\dist\AcmeWidgets-${APP_VERSION}-win64-setup.exe"
InstallDir "$PROGRAMFILES64\ACME Corporation\Acme Widgets"
InstallDirRegKey HKLM "${INSTALL_REG_KEY}" "InstallPath"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; ── MUI2 Configuration ────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON          "..\resources\acme-widgets.icns"
!define MUI_UNICON        "..\resources\acme-widgets.icns"

; Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.txt"
!define MUI_COMPONENTSPAGE_SMALLDESC
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN           "$INSTDIR\bin\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT      "Launch Acme Widgets now"
!define MUI_FINISHPAGE_SHOWREADME    "$INSTDIR\share\acme-widgets\README.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View README"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

; ── Version information ───────────────────────────────────────────────────────
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName"     "${APP_NAME}"
VIAddVersionKey "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey "LegalCopyright"  "Copyright (c) 2025 ACME Corporation"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion"     "${APP_VERSION}"

; ── Component sections ────────────────────────────────────────────────────────

; Core Application (required)
Section "!Acme Widgets Application (required)" SEC_CORE
    SectionIn RO    ; read-only = always installed
    SetOutPath "$INSTDIR\bin"
    File "..\payload\bin\acme-widgets.bat"
    SetOutPath "$INSTDIR\etc"
    File "..\payload\etc\acme-widgets.conf"
    SetOutPath "$INSTDIR"
    File "..\LICENSE.txt"

    ; Write registry entries
    WriteRegStr HKLM "${INSTALL_REG_KEY}" "InstallPath" "$INSTDIR"
    WriteRegStr HKLM "${INSTALL_REG_KEY}" "Version"     "${APP_VERSION}"
    WriteRegStr HKLM "${INSTALL_REG_KEY}" "Publisher"   "${APP_PUBLISHER}"
    WriteRegDWORD HKCU "${INSTALL_REG_KEY}\Preferences" "FirstRun" 1

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Add/Remove Programs entry
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"     "${APP_NAME} ${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "URLInfoAbout"    "${APP_URL}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"  1

    ; Create log directory
    CreateDirectory "$LOCALAPPDATA\ACME Corporation\Acme Widgets\Logs"

SectionEnd

; Documentation (optional, default ON)
Section "Documentation" SEC_DOCS
    SetOutPath "$INSTDIR\share\acme-widgets"
    File "..\payload\share\acme-widgets\README.txt"
    File "..\payload\share\acme-widgets\CHANGELOG.txt"
SectionEnd

; Sample Projects (optional, default OFF)
Section /o "Sample Projects" SEC_SAMPLES
    SetOutPath "$INSTDIR\share\acme-widgets"
    File "..\payload\share\acme-widgets\sample.dat"
SectionEnd

; Start Menu shortcuts (optional, default ON)
Section "Start Menu Shortcuts" SEC_STARTMENU
    CreateDirectory "$SMPROGRAMS\ACME Corporation\Acme Widgets"
    CreateShortcut  "$SMPROGRAMS\ACME Corporation\Acme Widgets\Acme Widgets.lnk" \
                    "$INSTDIR\bin\${APP_EXE}"
    CreateShortcut  "$SMPROGRAMS\ACME Corporation\Acme Widgets\README.lnk" \
                    "$INSTDIR\share\acme-widgets\README.txt"
    CreateShortcut  "$SMPROGRAMS\ACME Corporation\Acme Widgets\Uninstall.lnk" \
                    "$INSTDIR\uninstall.exe"
SectionEnd

; Desktop shortcut (optional, default OFF)
Section /o "Desktop Shortcut" SEC_DESKTOP
    CreateShortcut "$DESKTOP\Acme Widgets.lnk" "$INSTDIR\bin\${APP_EXE}"
SectionEnd

; ── Component descriptions ────────────────────────────────────────────────────
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE}      "The main Acme Widgets binary, default configuration and runtime. Required."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DOCS}      "User guide, API reference and release notes."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_SAMPLES}   "Example widget configurations and demo data for learning."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} "Create shortcuts in the Start Menu."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP}   "Create a shortcut on the Desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ── Uninstaller ───────────────────────────────────────────────────────────────
Section "Uninstall"
    ; Remove files
    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\etc"
    RMDir /r "$INSTDIR\share"
    Delete "$INSTDIR\LICENSE.txt"
    Delete "$INSTDIR\uninstall.exe"
    RMDir  "$INSTDIR"

    ; Remove registry
    DeleteRegKey HKLM "${INSTALL_REG_KEY}"
    DeleteRegKey HKCU "${INSTALL_REG_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\ACME Corporation\Acme Widgets\*.*"
    RMDir  "$SMPROGRAMS\ACME Corporation\Acme Widgets"
    RMDir  "$SMPROGRAMS\ACME Corporation"
    Delete "$DESKTOP\Acme Widgets.lnk"
SectionEnd
