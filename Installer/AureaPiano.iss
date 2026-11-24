; Build with build-installer.ps1 after the application and plug-in binaries exist.
; INCLUDE_AAX=1 is reserved for a separately signed AAX package.
#define AppVersion "0.1.1"

[Setup]
AppId=AureaPiano
AppName=Aurea Piano
AppVersion={#AppVersion}
AppPublisher=Aurea Audio
#ifdef INCLUDE_AAX
DefaultDirName={commonpf64}\Aurea Piano
PrivilegesRequired=admin
OutputBaseFilename=AureaPiano-{#AppVersion}-win64-protools-setup
#else
DefaultDirName={userpf}\Aurea Piano
PrivilegesRequired=lowest
OutputBaseFilename=AureaPiano-{#AppVersion}-win64-setup
#endif
DefaultGroupName=Aurea Piano
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\Aurea Piano.exe

[Languages]
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "vst3"; Description: "Instalar instrumento VST3"; GroupDescription: "Componentes adicionales:"; Flags: checkedonce
#ifdef INCLUDE_AAX
Name: "aax"; Description: "Instalar instrumento AAX firmado para Pro Tools"; GroupDescription: "Componentes adicionales:"; Flags: checkedonce
#endif

[Files]
Source: "..\build\AureaPiano_artefacts\Release\Standalone\Aurea Piano.exe"; DestDir: "{app}"; Flags: ignoreversion
#ifdef INCLUDE_AAX
Source: "..\build\AureaPiano_artefacts\Release\VST3\Aurea Piano.vst3\*"; DestDir: "{commoncf64}\VST3\Aurea Piano.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3
Source: "..\release\signed\Aurea Piano.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\Aurea Piano.aaxplugin"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: aax
#else
Source: "..\build\AureaPiano_artefacts\Release\VST3\Aurea Piano.vst3\*"; DestDir: "{usercf}\VST3\Aurea Piano.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: vst3
#endif

[Icons]
Name: "{group}\Aurea Piano"; Filename: "{app}\Aurea Piano.exe"

[Run]
Filename: "{app}\Aurea Piano.exe"; Description: "Abrir Aurea Piano"; Flags: nowait postinstall skipifsilent
