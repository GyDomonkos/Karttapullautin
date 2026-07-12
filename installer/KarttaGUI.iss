; KarttaGUI installer script
; Built locally with: ISCC installer\KarttaGUI.iss
; Built in CI with:    ISCC installer\KarttaGUI.iss /DMyAppVersion=1.2.3
;
; #MyAppVersion defaults to 1.0.0 for local/manual runs; the GitHub Actions
; workflow overrides it with the pushed tag via the /D command-line switch.
#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

[Setup]
; Basic App Information
AppName=KarttaGUI
AppVersion={#MyAppVersion}
AppPublisher=Domonkos Gyorffy
DefaultDirName={autopf}\KarttaGUI
DefaultGroupName=KarttaGUI
; Output Settings (Where the final .exe goes)
OutputDir=.\InstallerOutput
OutputBaseFilename=KarttaGUI_Setup_v{#MyAppVersion}_Karttapullautin_v2.12.1
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
; This creates the "Do you accept this license?" screen during install
LicenseFile=..\LGPL_License.txt
; This adds the text to the uninstaller/info menu
InfoBeforeFile=..\LICENSE.txt
; This sets the icon for the setup wizard .exe itself
SetupIconFile=..\AppDist\bin\resources\icon.ico
; This ensures the uninstaller icon matches in the Windows Control Panel
UninstallDisplayIcon={app}\KarttaGUI.exe

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; This tells Inno Setup to grab EVERYTHING inside your clean AppDist/bin folder
Source: "..\AppDist\bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Lines so the license files are physically copied to the install directory
Source: "..\LGPL_License.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu shortcuts
Name: "{group}\KarttaGUI"; Filename: "{app}\KarttaGUI.exe"
Name: "{group}\Uninstall KarttaGUI"; Filename: "{uninstallexe}"
; Desktop shortcut
Name: "{autodesktop}\KarttaGUI"; Filename: "{app}\KarttaGUI.exe"; Tasks: desktopicon

[Run]
; Launch the app immediately after the user finishes installation
Filename: "{app}\KarttaGUI.exe"; Description: "Launch KarttaGUI"; Flags: nowait postinstall skipifsilent
