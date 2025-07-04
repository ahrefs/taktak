[Setup]
AppName=Taktak Installer
AppVersion=1.0
WizardStyle=modern
DefaultDirName={autopf}\Taktak
DefaultGroupName=Taktak
OutputBaseFilename=Taktak Installer
SetupIconFile=taktak_installer_icon.ico
UninstallDisplayIcon={app}\Taktak.exe

[Files]
; These files will be downloaded
Source: "{tmp}\Setup.exe"; DestDir: "{app}"; Flags: external deleteafterinstall
Source: "{tmp}\Chrome.7z"; DestDir: "{app}"; Flags: external deleteafterinstall

[Icons]
Name: "{group}\Taktak"; Filename: "{app}\Taktak.exe"

[Code]
var
  DownloadPage: TDownloadWizardPage;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  if Progress = ProgressMax then
    Log(Format('Successfully downloaded file to {tmp}: %s', [FileName]));
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
    // Using bunny CDN for testing. todo: to replace the links with production one
    DownloadPage.Add('https://taktak.b-cdn.net/Setup.exe', 'Setup.exe', '');
    DownloadPage.Add('https://taktak.b-cdn.net/Chrome.7z', 'Chrome.7z', '');
    DownloadPage.Show;
    try
      try
        DownloadPage.Download; // This downloads the files to {tmp}
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

[Run]
; Run Setup.exe after installation
Filename: "{app}\Setup.exe"; Description: "Run Setup"; Flags: nowait postinstall skipifsilent