; Melodee Windows installer (Inno Setup 6.3 or newer).
;
; Build:   ISCC.exe /DPayloadDir=<folder with VST3\, Standalone\, CLI\, README.md, INSTALL.md>
;                   /DAppVersion=1.0.0 /O<output folder> installer\windows\Melodee.iss
;
; Installs the VST3 plugin where every Windows DAW looks for it
; (C:\Program Files\Common Files\VST3), plus the standalone app and the command-line tool.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

#ifndef PayloadDir
  #define PayloadDir "..\..\dist\Melodee-Windows"
#endif

#define AppName "Melodee"
#define AppPublisher "Melodee"
#define AppURL "https://github.com/beerjaspaldhaliwal-ctrl/Melodee23"

[Setup]
; Keep this AppId forever: it lets new versions upgrade the old one in place.
AppId={{439E36EA-A0DF-42F0-B96F-07D608796D25}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}
VersionInfoDescription={#AppName} installer
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; Writing to C:\Program Files\Common Files\VST3 needs administrator rights.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputBaseFilename=Melodee-Setup-Windows
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\Melodee.exe
InfoAfterFile=AfterInstall.txt
; Ask to close a DAW that has the old plugin loaded, so it can be replaced.
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Everything (recommended)"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig...)"; Types: full custom
Name: "standalone"; Description: "Melodee app (works without a DAW)"; Types: full custom
Name: "cli"; Description: "Command-line tool (writes .mid files)"; Types: full

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the Melodee app"; GroupDescription: "Shortcuts:"; Components: standalone; Flags: unchecked

[Files]
; VST3 bundle: the whole Melodee.vst3 folder goes into the shared VST3 folder.
Source: "{#PayloadDir}\VST3\Melodee.vst3\*"; DestDir: "{commoncf64}\VST3\Melodee.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app
Source: "{#PayloadDir}\Standalone\Melodee.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
; Command-line tool (own folder: Windows file names are not case-sensitive, so melodee.exe would clash with Melodee.exe)
Source: "{#PayloadDir}\CLI\melodee.exe"; DestDir: "{app}\CLI"; Components: cli; Flags: ignoreversion
; Guides
Source: "{#PayloadDir}\README.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "{#PayloadDir}\INSTALL.md"; DestDir: "{app}\docs"; Flags: ignoreversion skipifsourcedoesntexist
Source: "AfterInstall.txt"; DestDir: "{app}\docs"; DestName: "Getting started.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Melodee"; Filename: "{app}\Melodee.exe"; Components: standalone
Name: "{group}\Getting started"; Filename: "{app}\docs\Getting started.txt"
Name: "{group}\Uninstall Melodee"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Melodee"; Filename: "{app}\Melodee.exe"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\Melodee.exe"; Description: "Open the Melodee app now"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
; Remove the plugin folder completely (Windows may add desktop.ini or icon files inside it).
Type: filesandordirs; Name: "{commoncf64}\VST3\Melodee.vst3"
