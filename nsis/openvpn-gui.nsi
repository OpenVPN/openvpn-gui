; ****************************************************************************
; * Copyright (C) 2013-2015 OpenVPN Technologies, Inc.                       *
; *  This program is free software; you can redistribute it and/or modify    *
; *  it under the terms of the GNU General Public License version 2          *
; *  as published by the Free Software Foundation.                           *
; ****************************************************************************

SetCompressor lzma

; Includes
!include "LogicLib.nsh"
!include "MultiUser.nsh"
!include "MUI2.nsh"

; WinMessages.nsh is needed to send WM_CLOSE to the GUI if it is still running
!include "WinMessages.nsh"

; EnvVarUpdate.nsh is needed to update the PATH environment variable
!include "EnvVarUpdate.nsh"

; Defines
!define MULTIUSER_EXECUTIONLEVEL Admin
!define PACKAGE_NAME "OpenVPN-GUI"
!define REG_KEY "HKLM Software\OpenVPN-GUI"

; Basic configuration
Name "OpenVPN-GUI ${VERSION}"
OutFile "..\openvpn-gui-installer.exe"
RequestExecutionLevel admin
ShowInstDetails show

; Default installation directory. Needed for silent installations. Will get
; overwritten in the function .onInit
InstallDir "$PROGRAMFILES\${PACKAGE_NAME}"

; Installer pages
!insertmacro MUI_PAGE_LICENSE "..\COPYRIGHT.GPL"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Macros

!macro SelectByParameter SECT PARAMETER DEFAULT
    ${GetOptions} $R0 "/${PARAMETER}=" $0
    ${If} ${DEFAULT} == 0
        ${If} $0 == 1
            !insertmacro SelectSection ${SECT}
        ${EndIf}
    ${Else}
        ${If} $0 != 0
            !insertmacro SelectSection ${SECT}
        ${EndIf}
    ${EndIf}
!macroend

;--------------------
; Functions

Function .onInit
    ${GetParameters} $R0
    ClearErrors

    !insertmacro SelectByParameter ${SecOverwriteConfiguration} SELECT_OVERWRITE_CONFIGURATION 1
 
    SetShellVarContext all

    ; Check if we're running on 64-bit Windows
    ${If} "${ARCH}" == "x86_64"
        SetRegView 64

        ; Change the installation directory to C:\Program Files, but only if the
        ; user has not provided a custom install location.
        ${If} "$INSTDIR" == "$PROGRAMFILES\${PACKAGE_NAME}"
            StrCpy $INSTDIR "$PROGRAMFILES64\${PACKAGE_NAME}"
        ${EndIf}
    ${EndIf}

FunctionEnd

Function un.onInit
    ClearErrors
    !insertmacro MULTIUSER_UNINIT
    SetShellVarContext all
    ${If} "${ARCH}" == "x86_64"
        SetRegView 64
    ${EndIf}
FunctionEnd

;--------------------
; Sections

Section -pre
    Push $0 ; for FindWindow
    FindWindow $0 "OpenVPN-GUI"
    StrCmp $0 0 guiNotRunning

    ; In silent mode always kill the GUI
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "To perform the specified operation, OpenVPN-GUI needs to be closed. Shall I close it?" /SD IDYES IDNO guiEndNo
    DetailPrint "Closing OpenVPN-GUI..."
    Goto guiEndYes

    guiEndNo:
        Quit

    guiEndYes:
        ; user wants to close GUI as part of install/upgrade
        FindWindow $0 "OpenVPN-GUI"
        IntCmp $0 0 guiNotRunning
        SendMessage $0 ${WM_CLOSE} 0 0
        Sleep 100
        Goto guiEndYes

    guiNotRunning:
        ; openvpn-gui not running/closed successfully, carry on with install/upgrade

SectionEnd

; This empty section allows selecting whether to reset OpenVPN-GUI registry key
; values to the default. OpenVPN-specific part of the configuration gets
; auto-detected and overwritten regardless of this setting.
Section /o "$(NAME_SecOverwriteConfiguration)" SecOverwriteConfiguration
SectionEnd

Section "-Add registry keys" SecAddRegistryKeys

    AddSize 0

    Var /GLOBAL OPENVPN_INSTALL_DIR

    ; This code checks if OpenVPN is installed and bails out if it's not.
    goto read_32bit_registry

    read_32bit_registry:
        ClearErrors
        ReadRegStr $OPENVPN_INSTALL_DIR HKLM "Software\OpenVPN" ""
        IfErrors read_64bit_registry openvpn_found

    read_64bit_registry:
        ClearErrors
        SetRegView 64
        ReadRegStr $OPENVPN_INSTALL_DIR HKLM "Software\OpenVPN" ""
        IfErrors openvpn_not_found openvpn_found

    openvpn_not_found:
        Abort "OpenVPN not installed, bailing out..."

    openvpn_found:
        DetailPrint "OpenVPN installed to $OPENVPN_INSTALL_DIR"

        ; If we're told to overwrite existing configuration, we do it.
        ${If} ${SectionIsSelected} ${SecOverwriteConfiguration}
            goto overwrite_gui_configuration
        ${EndIf}

        ; If registry values are missing, we need to add them regardless of what
        ; we've been told.
        ClearErrors
        ReadRegStr $0 HKLM "Software\OpenVPN-GUI" "allow_edit"
        IfErrors overwrite_gui_configuration update_openvpn_settings

        overwrite_gui_configuration:

            ; OpenVPN-GUI-specific registry keys. We may or may not update
            ; these.
            WriteRegStr ${REG_KEY} "allow_edit" "1"
            WriteRegStr ${REG_KEY} "allow_password" "1"
            WriteRegStr ${REG_KEY} "allow_proxy" "1"
            WriteRegStr ${REG_KEY} "allow_service" "0"
            WriteRegStr ${REG_KEY} "connectscript_timeout" "15"
            WriteRegStr ${REG_KEY} "disconnect_on_suspend" "1"
            WriteRegStr ${REG_KEY} "disconnectscript_timeout" "10"
            WriteRegStr ${REG_KEY} "preconnectscript_timeout" "10"
            WriteRegStr ${REG_KEY} "editor" "C:\Windows\notepad.exe"
            WriteRegStr ${REG_KEY} "log_append" "0"
            WriteRegStr ${REG_KEY} "log_viewer" "C:\Windows\notepad.exe"
            WriteRegStr ${REG_KEY} "passphrase_attempts" "3"
            WriteRegStr ${REG_KEY} "priority" "NORMAL_PRIORITY_CLASS"
            WriteRegStr ${REG_KEY} "show_balloon" "1"
            WriteRegStr ${REG_KEY} "show_script_window" "1"
            WriteRegStr ${REG_KEY} "silent_connection" "0"

        update_openvpn_settings:

            ; Registry keys related to OpenVPN. We always update these during
            ; install.

            DeleteRegValue ${REG_KEY} "config_dir"
            DeleteRegValue ${REG_KEY} "config_ext"
            DeleteRegValue ${REG_KEY} "exe_path"
            DeleteRegValue ${REG_KEY} "log_dir"
            WriteRegStr ${REG_KEY} "config_dir" "$OPENVPN_INSTALL_DIR\config"
            WriteRegStr ${REG_KEY} "config_ext" "ovpn"
            WriteRegStr ${REG_KEY} "exe_path" "$OPENVPN_INSTALL_DIR\bin\openvpn.exe"
            WriteRegStr ${REG_KEY} "log_dir" "$OPENVPN_INSTALL_DIR\log"

SectionEnd

Section "Add ${PACKAGE_NAME} to PATH" SecAddPath

    ; append our bin directory to end of current user path
    ${EnvVarUpdate} $R0 "PATH" "A" "HKLM" "$INSTDIR\bin"

SectionEnd

Section "$(NAME_SecInstallExecutable)" SecInstallExecutable

    SetOutPath "$INSTDIR\bin"
    AddSize 400
    File "..\openvpn-gui.exe"

SectionEnd

Section "$(NAME_SecInstallDesktopIcon)" SecInstallDesktopIcon
    AddSize 1
    SetOverwrite on
    CreateShortcut "$DESKTOP\OpenVPN-GUI.lnk" "$INSTDIR\bin\openvpn-gui.exe"
SectionEnd

Section "$(NAME_SecAddStartMenuEntries)" SecAddStartMenuEntries
    AddSize 1
    SetOverwrite on
    CreateDirectory "$SMPROGRAMS\${PACKAGE_NAME}"
    WriteINIStr "$SMPROGRAMS\${PACKAGE_NAME}\${PACKAGE_NAME} Web Site.url" "InternetShortcut" "URL" "http://sourceforge.net/projects/openvpn-gui"

    ${If} ${SectionIsSelected} ${SecInstallExecutable}
        CreateShortcut "$SMPROGRAMS\${PACKAGE_NAME}\${PACKAGE_NAME}.lnk" "$INSTDIR\bin\openvpn-gui.exe"
    ${EndIf}
SectionEnd

Section "$(NAME_SecInstallDocumentation)" SecInstallDocumentation
    AddSize 1
    SetOverwrite on
    SetOutPath "$INSTDIR\doc"
    File /oname=README.txt "..\README"
SectionEnd

Section "-post"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayName" "${PACKAGE_NAME} ${VERSION}"
    WriteRegExpandStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
    ;WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayIcon" "$INSTDIR\icon.ico"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayVersion" "${VERSION}"

SectionEnd

; This section is here to allow deletion of user-defined OpenVPN-GUI
; configuration. Registry keys related to current OpenVPN install are always
; deleted during uninstall.
Section /o "un.$(NAME_DeleteAllRegistryKeys)" SecDeleteAllRegistryKeys
SectionEnd

Section "-un.Uninstall" SecUninstall

    ; Stop OpenVPN-GUI if currently running
    DetailPrint "Stopping OpenVPN-GUI..."

    StopGUI:

        FindWindow $0 "OpenVPN-GUI"
        IntCmp $0 0 guiClosed
        SendMessage $0 ${WM_CLOSE} 0 0
        Sleep 100
        Goto StopGUI

    guiClosed:

        ; We'll always delete registry keys related to OpenVPN. These will be
        ; recreated automatically when OpenVPN-GUI is installed again.
        DeleteRegValue ${REG_KEY} "config_dir"
        DeleteRegValue ${REG_KEY} "config_ext"
        DeleteRegValue ${REG_KEY} "exe_path"
        DeleteRegValue ${REG_KEY} "log_dir"

        ; We wipe OpenVPN-GUI-specific configuration if asked to
        ${If} ${SectionIsSelected} ${SecDeleteAllRegistryKeys}
            DeleteRegKey ${REG_KEY}
        ${EndIf}

        ; Remove desktop icon, start menu entries and installed files
        Delete "$DESKTOP\${PACKAGE_NAME}.lnk"
        RMDir /r "$SMPROGRAMS\${PACKAGE_NAME}"
        RMDir /r "$INSTDIR"

        ;DeleteRegKey HKCR "${PACKAGE_NAME}File"

        ; Remove OpenVPN-GUI uninstall/change option from Add/Remove programs list
        DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}"

SectionEnd


;--------------------
; Language settings

!include "english.nsh"

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecOverwriteConfiguration} $(DESC_SecOverwriteConfiguration)
!insertmacro MUI_DESCRIPTION_TEXT ${SecInstallExecutable} $(DESC_SecInstallExecutable)
!insertmacro MUI_DESCRIPTION_TEXT ${SecInstallDocumentation} $(DESC_SecInstallDocumentation)
!insertmacro MUI_DESCRIPTION_TEXT ${SecInstallDesktopIcon} $(DESC_SecInstallDesktopIcon)
!insertmacro MUI_DESCRIPTION_TEXT ${SecAddStartMenuEntries} $(DESC_SecAddStartMenuEntries)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

