[Setup]
AppName=Taktak
AppVerName=Taktak
WizardStyle=modern
DefaultDirName={commonpf}\Taktak
DefaultGroupName=Taktak
OutputBaseFilename=Taktak Installer
SetupIconFile=taktak_installer_icon.ico
UninstallDisplayIcon={app}\Taktak.exe
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
CreateUninstallRegKey=no
Uninstallable=no
ExtraDiskSpaceRequired=378535936
WizardImageStretch=no

[Files]
Source: "{tmp}\Setup.exe"; DestDir: "{app}"; Flags: external
Source: "{tmp}\Taktak.7z"; DestDir: "{app}"; DestName: "Chrome.7z"; Flags: external

[Icons]
Name: "{commonprograms}\Taktak"; Filename: "{app}\Taktak.exe"
Name: "{commondesktop}\Taktak"; Filename: "{app}\Taktak.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Registry]
Root: HKLM; Subkey: "Software\Taktak"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Taktak"; ValueType: string; ValueName: "Version"; ValueData: "{#SetupSetting("AppVersion")}"
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Taktak.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\Taktak.exe"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Taktak.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app}"

[Code]
var
  DownloadPage: TDownloadWizardPage;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  if Progress = ProgressMax then
      Log(Format('Successfully downloaded file to: %s', [ExpandConstant('{tmp}\'+FileName)]));
  Result := True;
end;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), SetupMessage(msgPreparingDesc), @OnDownloadProgress);
  DownloadPage.ShowBaseNameInsteadOfUrl := True;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpReady then begin
    DownloadPage.Clear;
    DownloadPage.Add('https://cdn.static.taktak.com/win/setup.exe', 'Setup.exe', '');
    DownloadPage.Add('https://cdn.static.taktak.com/win/taktak.7z', 'Taktak.7z', '');
    DownloadPage.Show;
    try
      try
        DownloadPage.Download;
        Result := True;
      except
        if DownloadPage.AbortedByUser then
          Log('Aborted by user.')
        else
          SuppressibleMsgBox(AddPeriod(GetExceptionMessage), mbCriticalError, MB_OK, IDOK);
        Result := False;
      end;
    finally
      DownloadPage.Hide;
    end;
  end else
    Result := True;
end;

[Code]
procedure KillAppIfRunning;
var
  ResultCode: Integer;
begin
  Exec('taskkill', '/IM Taktak.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
  begin
    KillAppIfRunning;
  end;

  if CurStep = ssPostInstall then
  begin
    // Delete downloaded files after installation is complete
    DeleteFile(ExpandConstant('{app}\Setup.exe'));
    DeleteFile(ExpandConstant('{app}\Chrome.7z'));
  end;
end;

[Run]
Filename: "{app}\Setup.exe"; Parameters: "--system-level"; Description: "Install Taktak"; Flags: waituntilterminated
Filename: "{commonpf}\Taktak\Application\Taktak.exe"; Description: "Launch Taktak"; Flags: nowait postinstall skipifsilent