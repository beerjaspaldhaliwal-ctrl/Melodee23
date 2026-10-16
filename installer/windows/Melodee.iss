; Melodee Windows installer (Inno Setup 6.3 or newer).
;
; Build:   ISCC.exe /DPayloadDir=<folder with VST3\, Standalone\, CLI\, README.md, INSTALL.md>
;                   /DAppVersion=1.0.0 /O<output folder> installer\windows\Melodee.iss
;
; Installs the VST3 plugin where every Windows DAW looks for it
; (C:\Program Files\Common Files\VST3), plus the standalone app and the command-line tool.

#if Ver < EncodeVer(6, 3, 0)
  #error Inno Setup 6.3 or newer is required (for ArchitecturesAllowed=x64compatible).
#endif

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

#ifndef PayloadDir
  #define PayloadDir "..\..\dist\Melodee-Windows"
#endif

#define AppName "Melodee"
#define AppPublisher "Melodee"
#define AppURL "https://github.com/beerjaspaldhaliwal-ctrl/Melodee23"
#define VST3Dir "{commoncf64}\VST3\Melodee.vst3"

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
; Fixed locations, so the paths in the guide are always right.
DefaultDirName={autopf}\{#AppName}
DisableDirPage=yes
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; Writing to C:\Program Files\Common Files\VST3 needs administrator rights.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputBaseFilename=Melodee-Setup-Windows
SetupIconFile=Melodee.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\Melodee.ico
InfoAfterFile=AfterInstall.txt
; The plugin binary is a DLL named *.vst3, which the default filter (*.exe,*.dll,*.chm) would skip.
; With it added, Setup lists a DAW that has the old plugin loaded and offers to close it before upgrading.
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.chm,*.vst3
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Everything (recommended)"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (FL Studio, Ableton Live, Cubase, Studio One/Studio Pro, Reaper, Bitwig...)"; Types: full custom
Name: "standalone"; Description: "Melodee app (works without a DAW)"; Types: full custom
Name: "cli"; Description: "Command-line tool (writes .mid files)"; Types: full

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the Melodee app"; GroupDescription: "Shortcuts:"; Components: standalone; Flags: unchecked

[InstallDelete]
; Start every install/upgrade from a clean plugin bundle (no stale files from an older version).
Type: filesandordirs; Name: "{#VST3Dir}"; Components: vst3

[Files]
; VST3 bundle: the whole Melodee.vst3 folder goes into the shared VST3 folder.
; restartreplace/uninsrestartdelete: if a DAW still has the plugin loaded, swap/remove it at the next restart instead of failing.
Source: "{#PayloadDir}\VST3\Melodee.vst3\*"; DestDir: "{#VST3Dir}"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs restartreplace uninsrestartdelete
; Standalone app
Source: "{#PayloadDir}\Standalone\Melodee.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion restartreplace uninsrestartdelete
; Command-line tool (own folder: Windows file names are not case-sensitive, so melodee.exe would clash with Melodee.exe)
Source: "{#PayloadDir}\CLI\melodee.exe"; DestDir: "{app}\CLI"; Components: cli; Flags: ignoreversion
; Icon (used by the uninstall entry in Settings > Apps)
Source: "Melodee.ico"; DestDir: "{app}"; Flags: ignoreversion
; Guides
Source: "{#PayloadDir}\README.md"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "{#PayloadDir}\INSTALL.md"; DestDir: "{app}\docs"; Flags: ignoreversion skipifsourcedoesntexist
Source: "AfterInstall.txt"; DestDir: "{app}\docs"; DestName: "Getting started.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Melodee"; Filename: "{app}\Melodee.exe"; Components: standalone
Name: "{group}\Getting started"; Filename: "{app}\docs\Getting started.txt"
Name: "{group}\Uninstall Melodee"; Filename: "{uninstallexe}"; IconFilename: "{app}\Melodee.ico"
Name: "{autodesktop}\Melodee"; Filename: "{app}\Melodee.exe"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\Melodee.exe"; Description: "Open the Melodee app now"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
; Remove the plugin folder completely (Windows may add desktop.ini or icon files inside it),
; but only if this installer put the plugin there.
Type: filesandordirs; Name: "{#VST3Dir}"; Components: vst3

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
  // The plugin is built for x64. Windows on ARM runs x64 DAWs fine, but native ARM64 DAWs cannot load it.
  if IsArm64 and not WizardSilent then
    MsgBox('Melodee is a 64-bit Intel/AMD (x64) plugin.' #13#10#13#10 +
           'On Windows on ARM it works in the x64 version of your DAW, ' +
           'but a native ARM64 DAW cannot load it. The Melodee app works either way.',
           mbInformation, MB_OK);
end;
