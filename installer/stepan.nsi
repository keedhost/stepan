Unicode True
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "LogicLib.nsh"

; ── Defines (passed via /D from CI; fallbacks for local builds) ───────────────
!ifndef APP_VERSION
  !define APP_VERSION "0.1beta"
!endif
!ifndef DEPLOY_DIR
  !define DEPLOY_DIR "..\deploy"
!endif

!define APP_NAME       "Stepan"
!define APP_PUBLISHER  "Andrii Kondratiev"
!define APP_URL        "https://github.com/keedhost/stepan"
!define APP_EXE        "Stepan.exe"
!define REG_KEY        "Software\${APP_PUBLISHER}\${APP_NAME}"
!define UNINSTALL_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define VCREDIST_URL   "https://aka.ms/vs/17/release/vc_redist.x64.exe"

; ── Installer metadata ────────────────────────────────────────────────────────
Name             "${APP_NAME} ${APP_VERSION}"
OutFile          "..\Stepan-${APP_VERSION}-windows-x64-Setup.exe"
InstallDir       "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "${REG_KEY}" "InstallDir"
RequestExecutionLevel admin
ShowInstDetails  show

; ── MUI2 configuration ────────────────────────────────────────────────────────
!define MUI_ICON                   "..\resources\app.ico"
!define MUI_UNICON                 "..\resources\app.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN         "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT    "Запустити ${APP_NAME}"
!define MUI_FINISHPAGE_NOAUTOCLOSE

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE      "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "English"

; ── VC++ Runtime check & download ─────────────────────────────────────────────
!macro EnsureVCRedist
    ; MSVC 2015-2022 x64 runtime registry key
    ReadRegDWORD $R0 HKLM \
        "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ${If} $R0 != 1
        DetailPrint "Завантаження Visual C++ Redistributable 2022..."
        nsExec::ExecToLog \
            'powershell.exe -NonInteractive -NoProfile -Command \
            "Invoke-WebRequest -Uri ${VCREDIST_URL} \
            -OutFile $env:TEMP\vc_redist.x64.exe"'
        Pop $R0
        ${If} $R0 == 0
            DetailPrint "Встановлення Visual C++ Redistributable..."
            ExecWait '"$TEMP\vc_redist.x64.exe" /install /quiet /norestart' $R0
            ${If} $R0 != 0
                MessageBox MB_ICONEXCLAMATION \
                    "Не вдалося встановити Visual C++ Redistributable (код: $R0).$\n\
                    Встановіть вручну з microsoft.com та повторіть."
            ${EndIf}
        ${Else}
            MessageBox MB_ICONEXCLAMATION \
                "Не вдалося завантажити Visual C++ Redistributable.$\n\
                Перевірте інтернет-з'єднання або встановіть вручну з microsoft.com"
        ${EndIf}
    ${EndIf}
!macroend

; ── Main install section ──────────────────────────────────────────────────────
Section "Основні файли" SecMain
    SectionIn RO

    !insertmacro EnsureVCRedist

    ; Install all application files (preserves subdirectory structure)
    SetOutPath "$INSTDIR"
    File /r "${DEPLOY_DIR}\*"

    ; Ensure config directory exists
    CreateDirectory "$INSTDIR\config"

    ; Registry — app info
    WriteRegStr  HKLM "${REG_KEY}" "InstallDir" "$INSTDIR"
    WriteRegStr  HKLM "${REG_KEY}" "Version"    "${APP_VERSION}"

    ; Registry — Add/Remove Programs
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "${APP_PUBLISHER}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "URLInfoAbout"         "${APP_URL}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"          "$INSTDIR\${APP_EXE}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      '"$INSTDIR\Uninstall.exe"'
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1

    ; Start menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Видалити ${APP_NAME}.lnk" \
        "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; ── Uninstaller ───────────────────────────────────────────────────────────────
Section "Uninstall"
    RMDir /r "$INSTDIR"

    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Видалити ${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
SectionEnd
