; Debian-Installer Loader
; Copyright (C) 2007,2008  Robert Millan <rmh@aybabtu.com>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

; SetCompressor must _always_ be the first command
SetCompressor /SOLID lzma

!include l10n/templates/all.nsh

Name $(program_name)
Icon "swirl.ico"
UninstallIcon "swirl.ico"
XPStyle on
OutFile "win32-loader.exe"

!include LogicLib.nsh
!include FileFunc.nsh
!include WinMessages.nsh
!insertmacro GetRoot
!insertmacro un.GetRoot

!addplugindir plugins
!addplugindir plugins/cpuid
!addplugindir plugins/systeminfo
!include getwindowsversion.nsh

;--------------------------------

LicenseData $(license)

;--------------------------------

; Pages

Page license
Page custom ShowExpert
Page custom ShowRescue
Page custom ShowGraphics
!ifdef NETWORK_BASE_URL
Page custom ShowBranch
!endif
Page custom ShowDesktop
Page custom ShowCustom
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

Var /GLOBAL c
Var /GLOBAL d
Var /GLOBAL preseed_cmdline
Var /GLOBAL preseed_cfg
Var /GLOBAL proxy
Var /GLOBAL arch

Function .onInit
  InitPluginsDir

  ; needed by: timezones, keymaps, languages
  File /oname=$PLUGINSDIR\maps.ini maps.ini

  ; default to English
  StrCpy $LANGUAGE ${LANG_ENGLISH}

  ; Language selection dialog
  Push ""
!include l10n/templates/dialog.nsh
  Push unsupported
  Push "-- Not in this list --"

  Push A ; A means auto count languages
         ; for the auto count to work the first empty push (Push "") must remain
  LangDLL::LangDialog "Choose language" "Please choose the language used for \
the installation process. This language will be the default language for the \
final system."

  Pop $LANGUAGE
  ${If} $LANGUAGE == "cancel"
    Abort
  ${Endif}

  ; Note: Possible API abuse here.  Nsis *seems* to fallback sanely to English
  ; when $LANGUAGE == "unsupported", so we'll use that to decide wether to
  ; preseed later.

  Var /GLOBAL unsupported_language
  ${If} $LANGUAGE == "unsupported"
    StrCpy $unsupported_language true
    ; Translators: your language is supported by d-i, but not yet by nsis.
    ; Please get your translation in nsis before translating win32-loader.
    MessageBox MB_OK "Because your language is not supported by this \
stage of the installer, English will be used for now.  On the second \
(and last) stage of the install process, you will be offered a much \
wider choice, where your language is more likely to be present."
  ${Else}
    StrCpy $unsupported_language false
  ${Endif}
FunctionEnd

Function ShowExpert
; Do initialisations as early as possible, but not before license has been
; accepted unless absolutely necessary.

; ********************************************** Initialise $preseed_cmdline
  StrCpy $preseed_cmdline " "
; ********************************************** Initialise $c
  ; FIXME: this line is duplicated in the uninstaller.  keep in sync!
  ${GetRoot} $WINDIR $c

!ifndef NETWORK_BASE_URL
; ********************************************** Initialise $d
  ${GetRoot} $EXEDIR $d

; ********************************************** Check win32-loader.ini presence
; before initialising $arch or $gtk
  IfFileExists $d\win32-loader.ini +3 0
    MessageBox MB_OK|MB_ICONSTOP "$(error_missing_ini)"
    Quit

; ********************************************** Display README.html if found
  ReadINIStr $0 $PLUGINSDIR\maps.ini "languages" "$LANGUAGE"
  IfFileExists $d\README.$0.html 0 +3
    StrCpy $0 README.$0.html
    Goto readme_file_found
  IfFileExists $d\README.html 0 readme_file_not_found
    StrCpy $0 README.html
    Goto readme_file_found

readme_file_found:
  ShowWindow $HWNDPARENT ${SW_MINIMIZE}
  ExecShell "open" "file://$d/$0"

readme_file_not_found:
!endif

; ********************************************** Initialise $arch
  test64::get_arch
  StrCpy $arch $0
!ifndef NETWORK_BASE_URL
  ReadINIStr $0 $d\win32-loader.ini "installer" "arch"
  ${If} "$0:$arch" == "amd64:i386"
    MessageBox MB_OK|MB_ICONSTOP $(amd64_on_i386)
    Quit
  ${Endif}
  ${If} "$0:$arch" == "i386:amd64"
    MessageBox MB_YESNO|MB_ICONQUESTION $(i386_on_amd64) IDNO +2
    Quit
    StrCpy $arch "i386"
  ${Endif}
!endif

  StrCpy $INSTDIR "$c\debian"
  SetOutPath $INSTDIR

  ; Windows version is another abort condition
  Var /GLOBAL windows_version
  Var /GLOBAL windows_boot_method
  Call GetWindowsVersion
  Pop $windows_version
  StrCpy $1 $windows_version 3
  ${If} $1 == "NT "
    StrCpy $windows_boot_method ntldr
    Goto windows_version_ok
  ${Endif}
  ${If} $windows_version == "95"
  ${OrIf} $windows_version == "98"
    StrCpy $windows_boot_method direct
  ${Endif}
  ${If} $windows_version == "2000"
  ${OrIf} $windows_version == "XP"
  ${OrIf} $windows_version == "2003"
    StrCpy $windows_boot_method ntldr
    Goto windows_version_ok
  ${Endif}
  ${If} $windows_version == "Vista"
    StrCpy $windows_boot_method bootmgr
    Goto windows_version_ok
  ${Endif}
  MessageBox MB_OK|MB_ICONSTOP $(unsupported_version_of_windows)
  Quit
windows_version_ok:

  File /oname=$PLUGINSDIR\expert.ini	templates/binary_choice.ini
  WriteINIStr $PLUGINSDIR\expert.ini "Field 1" "Text" $(expert1)
  WriteINIStr $PLUGINSDIR\expert.ini "Field 2" "Text" $(expert2)
  WriteINIStr $PLUGINSDIR\expert.ini "Field 3" "Text" $(expert3)
  InstallOptions::dialog $PLUGINSDIR\expert.ini

  Var /GLOBAL expert
  ReadINIStr $0 $PLUGINSDIR\expert.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $expert true
  ${Else}
    StrCpy $expert false
  ${Endif}

  Var /GLOBAL debian_release
  StrCpy $debian_release lenny
FunctionEnd

Function ShowRescue
  File /oname=$PLUGINSDIR\rescue.ini	templates/binary_choice.ini
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 1" "Text" $(rescue1)
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 2" "Text" $(rescue2)
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 3" "Text" $(rescue3)
  InstallOptions::dialog $PLUGINSDIR\rescue.ini

  ReadINIStr $0 $PLUGINSDIR\rescue.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i rescue/enable boolean true"
  ${Endif}
FunctionEnd

Function ShowGraphics
  Var /GLOBAL user_interface
  Var /GLOBAL gtk

!ifndef NETWORK_BASE_URL
  Var /GLOBAL predefined_user_interface
  ReadINIStr $predefined_user_interface $d\win32-loader.ini "installer" "user_interface"

  ${If} $predefined_user_interface == ""
!endif
    ${If} $expert == true
      File /oname=$PLUGINSDIR\graphics.ini	templates/graphics.ini
      File /oname=$PLUGINSDIR\gtk.bmp	templates/gtk.bmp
      File /oname=$PLUGINSDIR\text.bmp	templates/text.bmp
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 1" "Text" $(graphics1)
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 2" "Text" "$PLUGINSDIR\gtk.bmp"
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 3" "Text" "$PLUGINSDIR\text.bmp"
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 4" "Text" $(graphics2)
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 5" "Text" $(graphics3)
      InstallOptions::dialog $PLUGINSDIR\graphics.ini
      ReadINIStr $0 $PLUGINSDIR\graphics.ini "Field 4" "State"
      ${If} $0 == "1"
        StrCpy $user_interface graphical
      ${EndIf}
    ${Else}
        StrCpy $user_interface graphical
    ${Endif}
!ifndef NETWORK_BASE_URL
  ${Else}
    StrCpy $user_interface $predefined_user_interface
  ${Endif}
!endif

${If} $user_interface == "graphical"
  StrCpy $gtk "gtk/"
${Endif}

; ********************************************** Preseed vga mode
  ${If} "$user_interface" == "graphical"
    StrCpy $preseed_cmdline "$preseed_cmdline video=vesa:ywrap,mtrr vga=788"
  ${Else}
    StrCpy $preseed_cmdline "$preseed_cmdline vga=normal"
  ${EndIf}
FunctionEnd

!ifdef NETWORK_BASE_URL

Function Download
    ; Base URL
    Pop $0
    ; Target dir
    Pop $1
    ; Filename
    Pop $2
    ; Allow it to fail?
    Pop $3
    Var /GLOBAL nsisdl_proxy
    ${If} $proxy == ""
      StrCpy $nsisdl_proxy /NOIEPROXY
    ${Else}
      StrCpy $nsisdl_proxy "/PROXY $proxy"
    ${Endif}

    NSISdl::download /TRANSLATE $(nsisdl1) $(nsisdl2) $(nsisdl3) $(nsisdl4) $(nsisdl5) $(nsisdl6) $(nsisdl7) $(nsisdl8) $nsisdl_proxy $0/$2 $1\$2
    Pop $R0
    ${If} $R0 != "success"
      ${If} $R0 != "cancel"
         MessageBox MB_OK|MB_ICONSTOP "$(error): $R0"
      ${EndIf}
      ${If} $3 == "false"
        Quit
      ${Endif}
    ${EndIf}
    StrCpy $0 "$R0"
FunctionEnd

Function ShowBranch
  Var /GLOBAL di_branch
  StrCpy $di_branch stable
  StrCpy $debian_release etch
  File /oname=$PLUGINSDIR\di_branch.ini	templates/binary_choice.ini
  ${If} $expert == true
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 1" "Text" $(di_branch1)
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 2" "Text" $(di_branch2)
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 3" "Text" $(di_branch3)
    InstallOptions::dialog $PLUGINSDIR\di_branch.ini
    ReadINIStr $0 $PLUGINSDIR\di_branch.ini "Field 3" "State"
    ${If} $0 == "1"
      StrCpy $di_branch daily
      StrCpy $debian_release lenny
    ${Endif}
  ${Endif}

; ********************************************** Initialise base_url
  Var /GLOBAL base_url
  ReadINIStr $0 $PLUGINSDIR\di_branch.ini "Field 3" "State"
  ${If} $di_branch == "daily"
    MessageBox MB_YESNO|MB_ICONQUESTION $(di_branch4) IDNO +2
    ExecShell "open" "http://wiki.debian.org/DebianInstaller/Today"
    Var /GLOBAL hacker
    ${If} $arch == "amd64"
      StrCpy $hacker aba
    ${Else}
      StrCpy $hacker joeyh
    ${Endif}
    StrCpy $base_url "http://people.debian.org/~$hacker/d-i/images/daily/netboot/$gtkdebian-installer/$arch"
  ${Else}
    StrCpy $base_url "http://ftp.se.debian.org/debian/dists/stable/main/installer-$arch/current/images/netboot/$gtkdebian-installer/$arch"
  ${Endif}
FunctionEnd
!endif

Function ShowDesktop
  Var /GLOBAL _desktop
  ; It appears that "desktop" is a reserved keyword, hence the _ prefix
  StrCpy $_desktop "gnome"
  ${If} $expert == false
    Return
  ${Endif}
  File /oname=$PLUGINSDIR\desktop.ini	templates/4_choices.ini
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 1" "Text" $(desktop1)
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 2" "Text" "GNOME"
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 3" "Text" "KDE"
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 4" "Text" "XFCE"
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 5" "Text" $(desktop2)
  InstallOptions::dialog $PLUGINSDIR\desktop.ini
  ReadINIStr $0 $PLUGINSDIR\desktop.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $_desktop "kde"
  ${Endif}
  ReadINIStr $0 $PLUGINSDIR\desktop.ini "Field 4" "State"
  ${If} $0 == "1"
    StrCpy $_desktop "xfce"
  ${Endif}
  ${If} $_desktop == "gnome"
    Return ; GNOME is already default, do nothing
  ${Endif}
  ${If} $debian_release == "etch"
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
tasksel tasksel/first multiselect $_desktop-desktop, standard$\n\
tasksel tasksel/first seen true"
  ${Else}
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
tasksel tasksel/desktop multiselect $_desktop-desktop$\n\
tasksel tasksel/desktop seen true"
  ${Endif}
FunctionEnd

Function ShowCustom
; Gather all the missing information before ShowCustom is displayed

!ifndef NETWORK_BASE_URL

; ********************************************** Media-based install
  Var /GLOBAL linux
  Var /GLOBAL initrd
  Var /GLOBAL g2ldr
  Var /GLOBAL g2ldr_mbr
  ReadINIStr $linux	$d\win32-loader.ini "installer" "$arch/$gtklinux"
  ReadINIStr $initrd	$d\win32-loader.ini "installer" "$arch/$gtkinitrd"
  ReadINIStr $g2ldr	$d\win32-loader.ini "grub" "g2ldr"
  ReadINIStr $g2ldr_mbr	$d\win32-loader.ini "grub" "g2ldr.mbr"
  StrCmp $linux		"" incomplete_ini
  StrCmp $initrd	"" incomplete_ini
  StrCmp $g2ldr		"" incomplete_ini
  StrCmp $g2ldr_mbr	"" incomplete_ini
  Goto ini_is_ok
incomplete_ini:
  MessageBox MB_OK|MB_ICONSTOP "$(error_incomplete_ini)"
  Quit
ini_is_ok:

!endif

; ********************************************** Initialise proxy (even when in network-less mode, for the sake of preseeding)
  StrCpy $proxy ""
  ReadRegDWORD $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings" ProxyEnable
  IntCmp $0 1 0 proxyless
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings" ProxyServer
  StrCmp $0 "" proxyless
  StrCpy $proxy "$0"
proxyless:

; ********************************************** preseed locale
  ${If} $unsupported_language == false
    ReadINIStr $0 $PLUGINSDIR\maps.ini "languages" "$LANGUAGE"
    ReadRegStr $1 HKCU "Control Panel\International" iCountry
    ReadINIStr $1 $PLUGINSDIR\maps.ini "countries" "$1"
    ${If} $0 != ""
      ${If} $1 != ""
        StrCpy $0 "$0_$1"
      ${Endif}
      StrCpy $preseed_cmdline "$preseed_cmdline locale=$0"
    ${Endif}
  ${Endif}

; ********************************************** preseed domain
  systeminfo::domain
  Pop $2
  Pop $0
  ${If} $2 != 0
    ${If} $0 == ""
      StrCpy $0 "localdomain"
    ${Endif}
    StrCpy $preseed_cmdline "$preseed_cmdline domain?=$0"
  ${EndIf}

; ********************************************** preseed timezone
  ReadRegStr $0 HKLM SYSTEM\CurrentControlSet\Control\TimeZoneInformation TimeZoneKeyName
  ${If} $0 == ""
    ReadRegStr $0 HKLM SYSTEM\CurrentControlSet\Control\TimeZoneInformation StandardName
  ${Endif}
  ReadINIStr $0 $PLUGINSDIR\maps.ini "timezones" "$0"
  ${If} $0 != ""
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i time/zone string $0"
  ${Endif}

; ********************************************** preseed keymap
  systeminfo::keyboard_layout
  Pop $0
  ; lower word is the locale identifier (higher word is a handler to the actual layout)
  IntOp $0 $0 & 0x0000FFFF
  IntFmt $0 "0x%04X" $0
  ReadINIStr $0 $PLUGINSDIR\maps.ini "keymaps" "$0"
  ; FIXME: do we need to support non-AT keyboards here?
  ${If} $0 != ""
    ${If} $expert == true
      MessageBox MB_YESNO|MB_ICONQUESTION $(detected_keyboard_is) IDNO keyboard_bad_guess
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i console-keymaps-at/keymap select $0$\n\
d-i console-keymaps-at/keymap seen true"
      Goto keyboard_end
keyboard_bad_guess:
      MessageBox MB_OK $(keyboard_bug_report)
keyboard_end:
    ${Else}
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i console-keymaps-at/keymap select $0"
    ${Endif}
  ${Endif}

; ********************************************** preseed hostname
  systeminfo::hostname
  Pop $2
  Pop $0
  ${If} $2 != 0
    ${If} $0 == ""
      StrCpy $0 "debian"
    ${Endif}
    StrCpy $preseed_cmdline "$preseed_cmdline hostname?=$0"
  ${EndIf}

; ********************************************** preseed priority
  ${If} $expert == true
    StrCpy $preseed_cmdline "$preseed_cmdline priority=low"
  ${Endif}

; ********************************************** preseed user-fullname
  systeminfo::username
  Pop $0
  ${If} $0 != ""
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i passwd/user-fullname string $0"
  ${Endif}

!ifdef NETWORK_BASE_URL
; ********************************************** if ${NETWORK_BASE_URL} provides a preseed.cfg, use it
  Push true
  Push "preseed.cfg"
  Push "$PLUGINSDIR"
  Push "${NETWORK_BASE_URL}"
  Call Download
  ${If} $0 == "success"
    StrCpy $preseed_cfg "$preseed_cfg$\n$\n"
    ClearErrors
    FileOpen $0 $PLUGINSDIR\preseed.cfg r
network_preseed_loop:
    FileRead $0 $1
    IfErrors +3
    StrCpy $preseed_cfg "\
$preseed_cfg\
$1"
    Goto network_preseed_loop
    FileClose $0
    StrCpy $preseed_cfg "$preseed_cfg$\n"
  ${Endif}
!endif

; ********************************************** Display customisation dialog now
  Var /GLOBAL boot_ini
  StrCpy $boot_ini "$c\boot.ini"
  ${If} $expert == true
    File /oname=$PLUGINSDIR\custom.ini templates/custom.ini
    WriteINIStr $PLUGINSDIR\custom.ini "Field 1" "Text" $(custom1)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 2" "Text" $(custom2)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 3" "Text" $(custom3)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 4" "Text" $(custom4)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 5" "Text" $(custom5)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 6" "State" "$proxy"
    WriteINIStr $PLUGINSDIR\custom.ini "Field 7" "State" "$boot_ini"
!ifdef NETWORK_BASE_URL
    WriteINIStr $PLUGINSDIR\custom.ini "Field 8" "State" "$base_url"
!endif
    WriteINIStr $PLUGINSDIR\custom.ini "Field 9" "State" "$preseed_cmdline"
    InstallOptions::dialog $PLUGINSDIR\custom.ini
    ReadINIStr $proxy		$PLUGINSDIR\custom.ini "Field 6" "State"
    ReadINIStr $boot_ini	$PLUGINSDIR\custom.ini "Field 7" "State"
!ifdef NETWORK_BASE_URL
    ReadINIStr $base_url	$PLUGINSDIR\custom.ini "Field 8" "State"
!endif
    ReadINIStr $preseed_cmdline	$PLUGINSDIR\custom.ini "Field 9" "State"
  ${Endif}

; do this inmediately after custom.ini, because proxy settings can be
; overriden there
; ********************************************** preseed proxy
  ${If} $proxy == ""
    StrCpy $preseed_cmdline "$preseed_cmdline mirror/http/proxy="
  ${Else}
    StrCpy $preseed_cmdline "$preseed_cmdline mirror/http/proxy=http://$proxy/"
  ${Endif}
FunctionEnd

Section "Debian-Installer Loader"

; ******************************************************************************
; ***************************************** THIS IS WHERE THE REAL ACTION STARTS
; ******************************************************************************

  ; Up to this point, we haven't modified host system.  The first modification
  ; we want to do is preparing the Uninstaller (so that in case something went
  ; wrong, our half-install can be undone).
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader" "DisplayName" $(program_name)
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader" "UninstallString" "$INSTDIR\uninstall.exe"

!ifndef NETWORK_BASE_URL
  ClearErrors
  StrCpy $0 "$EXEDIR\$linux"
  StrCpy $1 "$INSTDIR\linux"
  CopyFiles /FILESONLY "$0" "$1"
  IfErrors 0 +3
    MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
    Quit
  StrCpy $0 "$EXEDIR\$initrd"
  StrCpy $1 "$INSTDIR\initrd.gz"
  CopyFiles /FILESONLY "$0" "$1"
  IfErrors 0 +3
    MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
    Quit
!else
  Push "false"
  Push "linux"
  Push "$INSTDIR"
  Push "$base_url"
  Call Download
  Push "false"
  Push "initrd.gz"
  Push "$INSTDIR"
  Push "$base_url"
  Call Download
!endif

; We're about to write down our preseed line.  This would be a nice place
; to add post-install parameters.
  StrCpy $preseed_cmdline "$preseed_cmdline --"

; ********************************************** preseed quietness
  ${If} $expert == false
    StrCpy $preseed_cmdline "$preseed_cmdline quiet"
  ${Endif}

; ********************************************** grub.cfg
  FileOpen $0 $c\grub.cfg w
  FileWrite $0 "\
search	--set /debian/initrd.gz$\n\
linux	/debian/linux $preseed_cmdline$\n\
initrd	/debian/initrd.gz$\n\
boot"
  FileClose $0

; ********************************************** cpio hack
  File /oname=$PLUGINSDIR\cpio.exe /usr/share/win32/cpio.exe
  File /oname=$PLUGINSDIR\gzip.exe /usr/share/win32/gzip.exe

  FileOpen $0 $PLUGINSDIR\preseed.cfg w
  FileWrite $0 "$preseed_cfg$\n"
  FileClose $0

  ; cpio awkward CLI, meet Winf**k awkward CLI
  FileOpen $0 $PLUGINSDIR\cpio_list w
  FileWrite $0 "preseed.cfg"
  FileClose $0

  nsExec::Exec '"cmd.exe" /c cd $PLUGINSDIR && cpio.exe -o -H newc < cpio_list > newc_chunk'
  nsExec::Exec '"cmd.exe" /c cd $PLUGINSDIR && gzip.exe -1 < newc_chunk >> $INSTDIR\initrd.gz'

; ********************************************** Needed for systems with compressed NTFS
  nsExec::Exec '"compact" /u $c\g2ldr $c\grub.cfg $INSTDIR\linux $INSTDIR\initrd.gz'
  ; in my tests, uncompressing $c\grub.cfg wasn't necessary, but better be safe than sorry

; ********************************************** Do bootloader last, because it's the most dangerous
  ${If} $windows_boot_method == ntldr
    ${Unless} ${FileExists} "$boot_ini"
      MessageBox MB_OK|MB_ICONSTOP $(boot_ini_not_found)
      Quit
    ${Endif}
!ifdef NETWORK_BASE_URL
    Push "false"
    Push "g2ldr"
    Push "$c"
    Push "${NETWORK_BASE_URL}"
    Call Download
    Push "false"
    Push "g2ldr.mbr"
    Push "$c"
    Push "${NETWORK_BASE_URL}"
    Call Download
!else
    ClearErrors
    StrCpy $0 "$EXEDIR\$g2ldr"
    StrCpy $1 "$c\g2ldr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$g2ldr_mbr"
    StrCpy $1 "$c\g2ldr.mbr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
!endif
    SetFileAttributes "$boot_ini" NORMAL
    SetFileAttributes "$boot_ini" SYSTEM|HIDDEN
    ; Sometimes timeout isn't set.  This may result in ntldr booting straight to
    ; Windows (bad) or straight to Debian-Installer (also bad)!  Force it to 30
    ; just in case.
    WriteIniStr "$boot_ini" "boot loader" "timeout" "30"
    WriteIniStr "$boot_ini" "operating systems" "$c\g2ldr.mbr" '"$(d-i_ntldr)"'
  ${Endif}

  ${If} $windows_boot_method == direct
!ifdef NETWORK_BASE_URL
    Push "false"
    Push "grub.exe"
    Push "$INSTDIR"
    Push "${NETWORK_BASE_URL}"
    Call Download
    Push "false"
    Push "grub.pif"
    Push "$INSTDIR"
    Push "${NETWORK_BASE_URL}"
    Call Download
!else
    ClearErrors
    StrCpy $0 "$EXEDIR\$grub_exe"
    StrCpy $1 "$INSTDIR\grub.exe"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$grub_pif"
    StrCpy $1 "$INSTDIR\grub.pif"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
!endif
  ${Endif}

  ${If} $windows_boot_method == bootmgr
!ifdef NETWORK_BASE_URL
    Push "false"
    Push "g2ldr"
    Push "$c"
    Push "${NETWORK_BASE_URL}"
    Call Download
    Push "false"
    Push "g2ldr.mbr"
    Push "$c"
    Push "${NETWORK_BASE_URL}"
    Call Download
!else
    ClearErrors
    StrCpy $0 "$EXEDIR\$g2ldr"
    StrCpy $1 "$c\g2ldr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$g2ldr_mbr"
    StrCpy $1 "$c\g2ldr.mbr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
!endif
    ReadRegStr $0 HKLM "Software\Debian\Debian-Installer Loader" "bootmgr"
    ${If} $0 == ""
      nsExec::ExecToStack '"bcdedit" /create /d "$(d-i)" /application bootsector'
      Pop $0
      ${If} $0 != 0
        StrCpy $0 bcdedit.exe
        MessageBox MB_OK|MB_ICONSTOP "$(error_exec)"
        Quit
      ${Endif}
      ; "The entry {id} was successfully created" is in top of stack now
      string::bcdedit_extract_id
      Pop $0
      ${If} $0 == "error"
        MessageBox MB_OK|MB_ICONSTOP "$(error_bcdedit_extract_id)"
        Quit
      ${Endif}
      ; $0 holds the boot id.  Write it down, both for installer idempotency
      ; and for uninstaller.
      WriteRegStr HKLM "Software\Debian\Debian-Installer Loader" "bootmgr" "$0"
    ${Endif}
    nsExec::Exec '"bcdedit" /set $0 device boot'
    nsExec::Exec '"bcdedit" /set $0 path \g2ldr.mbr'
    nsExec::Exec '"bcdedit" /displayorder $0 /addlast'
  ${Endif}
SectionEnd

Function .onInstSuccess
  Var /GLOBAL warning2
  ${If} $windows_boot_method == direct
    StrCpy $warning2 $(warning2_direct)
  ${Else}
    StrCpy $warning2 $(warning2_reboot)
  ${Endif}
  MessageBox MB_OK '$(warning1)$warning2$(warning3)'
  ${If} $windows_boot_method == direct
    Exec '"$INSTDIR\grub.pif"'
  ${Else}
    MessageBox MB_YESNO|MB_ICONQUESTION $(reboot_now) IDNO +2
    Reboot
  ${Endif}
FunctionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  ; Initialise $c
  ; FIXME: this line is duplicated in the installer.  keep in sync!
  ${un.GetRoot} $WINDIR $c

  SetFileAttributes "$c\boot.ini" NORMAL
  SetFileAttributes "$c\boot.ini" SYSTEM|HIDDEN
  DeleteINIStr "$c\boot.ini" "operating systems" "$c\g2ldr.mbr"

  ReadRegStr $0 HKLM "Software\Debian\Debian-Installer Loader" "bootmgr"
  ${If} $0 != ""
    nsExec::Exec '"bcdedit" /delete $0'
    Pop $0
    ${If} $0 != 0
      StrCpy $0 bcdedit.exe
      MessageBox MB_OK|MB_ICONSTOP "$(error_exec)"
    ${Endif}
  ${Endif}

  DeleteRegKey HKLM "Software\Debian\Debian-Installer Loader"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader"
  Delete $c\g2ldr
  Delete $c\g2ldr.mbr
  Delete $c\grub.cfg
  Delete $INSTDIR\grub.exe
  Delete $INSTDIR\grub.pif
  Delete $INSTDIR\linux
  Delete $INSTDIR\initrd.gz
  Delete /REBOOTOK $INSTDIR\Uninstall.exe
  RMDir /REBOOTOK $INSTDIR
SectionEnd
