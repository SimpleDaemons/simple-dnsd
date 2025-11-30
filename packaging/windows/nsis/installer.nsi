; NSIS Installer Script for simple-dnsd
; Modern UI with license acceptance

!include "MUI2.nsh"
!include "FileFunc.nsh"

;--------------------------------
; General

Name "simple-dnsd"
OutFile "simple-dnsd-Setup-${VERSION}.exe"
Unicode True
InstallDir "$PROGRAMFILES\SimpleDaemons\simple-dnsd"
RequestExecutionLevel admin

; Version Information
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "simple-dnsd"
VIAddVersionKey "FileDescription" "simple-dnsd Installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "CompanyName" "SimpleDaemons"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2024 SimpleDaemons"

;--------------------------------
; Interface Settings

!define MUI_ICON "..\assets\icons\simple-dnsd.ico"
!define MUI_UNICON "..\assets\icons\simple-dnsd.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "..\assets\icons\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "..\assets\icons\wizard.bmp"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\licenses\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Sections

Section "Core Files" SecCore
    SectionIn RO
    
    SetOutPath "$INSTDIR"
    File /r "simple-dnsd\*"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    
    ; Registry entries
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "DisplayName" "simple-dnsd"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "Publisher" "SimpleDaemons"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd" \
        "NoRepair" 1
SectionEnd

Section "Start Menu Shortcuts" SecShortcuts
    CreateDirectory "$SMPROGRAMS\SimpleDaemons"
    CreateShortcut "$SMPROGRAMS\SimpleDaemons\simple-dnsd.lnk" "$INSTDIR\simple-dnsd.exe"
    CreateShortcut "$SMPROGRAMS\SimpleDaemons\Uninstall simple-dnsd.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Windows Service" SecService
    ; Install as Windows service
    ExecWait '"$INSTDIR\simple-dnsd.exe" install'
SectionEnd

;--------------------------------
; Descriptions

LangString DESC_SecCore ${LANG_ENGLISH} "Core application files (required)"
LangString DESC_SecShortcuts ${LANG_ENGLISH} "Create Start Menu shortcuts"
LangString DESC_SecService ${LANG_ENGLISH} "Install as Windows Service"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} $(DESC_SecShortcuts)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecService} $(DESC_SecService)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller

Section "Uninstall"
    ; Remove service
    ExecWait '"$INSTDIR\simple-dnsd.exe" uninstall'
    
    ; Remove files
    RMDir /r "$INSTDIR"
    
    ; Remove shortcuts
    Delete "$SMPROGRAMS\SimpleDaemons\simple-dnsd.lnk"
    Delete "$SMPROGRAMS\SimpleDaemons\Uninstall simple-dnsd.lnk"
    RMDir "$SMPROGRAMS\SimpleDaemons"
    
    ; Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\simple-dnsd"
SectionEnd

