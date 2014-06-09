; Setup script for RC700 simulator

[Setup]
AppId={{EF090301-CEEC-474B-943B-E022DE015E01}
AppName=RC700 Simulator
AppVersion=1.0
AppPublisher=Michael Ringgaard
AppPublisherURL=http://www.jbox.dk/rc702/simulator.shtm
AppSupportURL=http://www.jbox.dk/rc702/simulator.shtm
AppUpdatesURL=http://www.jbox.dk/rc702/simulator.shtm
DefaultDirName={pf}\RC700 Simulator
DefaultGroupName=RC700 Simulator
AllowNoIcons=yes
OutputBaseFilename=rc700-win-setup
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "rc700.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "SDL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "disks\RCCPM22.IMD"; DestDir: "{app}"; Flags: ignoreversion
Source: "disks\RCCOMAL0112.IMD"; DestDir: "{app}"; Flags: ignoreversion
Source: "disks\COMPAS220.IMD"; DestDir: "{app}"; Flags: ignoreversion
Source: "disks\GAMES1.IMD"; DestDir: "{app}"; Flags: ignoreversion
Source: "disks.url"; DestDir: "{app}"

[Icons]
Name: "{group}\RC700 CPM v2.2"; Filename: "{app}\rc700.exe"; Parameters: "RCCPM22.IMD"; IconFilename: "{app}\rc700.exe"
Name: "{group}\RC700 COMAL"; Filename: "{app}\rc700.exe"; Parameters: "RCCOMAL0112.IMD"; IconFilename: "{app}\rc700.exe"
Name: "{group}\RC700 COMPAS Pascal"; Filename: "{app}\rc700.exe"; Parameters: "COMPAS220.IMD"; IconFilename: "{app}\rc700.exe"
Name: "{group}\RC700 Games"; Filename: "{app}\rc700.exe"; Parameters: "GAMES1.IMD"; IconFilename: "{app}\rc700.exe"
Name: "{group}\More disks..."; Filename: "{app}\disks.url"
Name: "{group}\{cm:UninstallProgram,RC700 Simulator}"; Filename: "{uninstallexe}"

[Registry]
Root: HKCR; Subkey: ".imd"; ValueType: string; ValueName: ""; ValueData: "DiskImage"; Flags: uninsdeletevalue
Root: HKCR; Subkey: ".imd\Shell\Run unsing RC700 simulator\Command"; ValueType: string; ValueName: ""; ValueData: """{app}\rc700.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCR; Subkey: ".imd\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\rc700.exe"

[Run]
Filename: "{app}\rc700.exe"; Parameters: "RCCPM22.IMD"; Description: "{cm:LaunchProgram,RC700 Simulator}"; Flags: nowait postinstall skipifsilent
