; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=DarkRadiant
AppVerName=DarkRadiant 2.0.5pre2 x64
AppPublisher=The Dark Mod
AppPublisherURL=http://www.thedarkmod.com
AppSupportURL=http://www.thedarkmod.com
AppUpdatesURL=http://www.thedarkmod.com
DefaultDirName={pf}\DarkRadiant
DefaultGroupName=DarkRadiant 2.0.5pre2 x64
OutputDir=.
OutputBaseFilename=darkradiant-2.0.5pre2-x64
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\..\install\darkradiant.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\install\*"; Excludes: "*.pdb,*.exp,*.lib,*.in,*.fbp,*.iobj,*.ipdb"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x64\Microsoft.VC140.CRT\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x64\Microsoft.VC140.CRT\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\DarkRadiant"; Filename: "{app}\darkradiant.exe";
Name: "{group}\{cm:UninstallProgram,DarkRadiant}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\DarkRadiant"; Filename: "{app}\darkradiant.exe"; Tasks: desktopicon

[InstallDelete]
; Remove the legacy WaveFront plugin before installation
Type: files; Name: {app}\plugins\wavefront.dll;
