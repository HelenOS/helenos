[Setup]
AppName={#AppName}
AppID={#AppNameLower}
AppVerName={#AppName} {#AppVersion}
AppPublisher=PCC Project
AppPublisherURL=http://pcc.ludd.ltu.se/
DefaultDirName={pf}\{#AppNameLower}
DefaultGroupName={#AppName}
Compression=lzma/ultra
InternalCompressLevel=ultra
SolidCompression=yes
OutputDir=.
OutputBaseFilename={#AppNameLower}-{#AppVersion}-win32
ChangesEnvironment=yes

[Files]
Source: "c:\Program Files\{#AppNameLower}\*.*"; DestDir: {app}; Flags: recursesubdirs

[Icons]
Name: {group}\{cm:UninstallProgram, {#AppName}}; FileName: {uninstallexe}

[Registry]
Root: HKLM; Subkey: System\CurrentControlSet\Control\Session Manager\Environment; ValueName: {#AppName}DIR; ValueType: string; ValueData: {app}; Flags: deletevalue

[Tasks]
Name: modifypath; Description: Add application directory to your system path; Flags: unchecked

[Code]

// Jared Breland <jbreland@legroom.net>
// http://www.legroom.net/software/modpath

procedure ModPath(BinDir : String);
var
	oldpath : String;
	newpath : String;
	pathArr : TArrayOfString;
	i : Integer;
begin
	RegQueryStringValue(HKEY_LOCAL_MACHINE, 'System\CurrentControlSet\Control\Session Manager\Environment', 'Path', oldpath);
	oldpath := oldpath + ';';
	i := 0;
	while (Pos(';', oldpath) > 0) do begin
		SetArrayLength(pathArr, i+1);
		pathArr[i] := Copy(oldpath, 0, Pos(';', oldpath) - 1);
		oldpath := Copy(oldpath, Pos(';', oldpath) + 1, Length(oldpath));
		i := i + 1;

		if BinDir = pathArr[i-1] then
			if IsUninstaller() = true then begin
				continue;
			end else begin
				abort;
			end;

		if i = 1 then begin
			newpath := pathArr[i-1];
		end else begin
			newpath := newpath + ';' + pathArr[i-1];
		end;
	end;

	if IsUninstaller() = false then
		newpath := newpath + ';' + BinDir;

	RegWriteStringValue(HKEY_LOCAL_MACHINE, 'System\CurrentControlSet\Control\Session Manager\Environment', 'Path', newpath);
end;
		
procedure CurStepChanged(CurStep : TSetupStep);
var
	appdir : String;
begin
	if CurStep = ssPostInstall then
		if IsTaskSelected('modifypath') then begin
			appdir := ExpandConstant('{app}');
			ModPath(appdir + '\bin');
			SaveStringToFile(appdir + '\uninsTasks.txt', WizardSelectedTasks(False), False);
		end;
end;

procedure CurUninstallStepChanged(CurUninstallStep : TUninstallStep);
var
	appdir : String;
	selectedTasks : String;
begin
	if CurUninstallStep = usUninstall then begin
		appdir := ExpandConstant('{app}');
		if LoadStringFromFile(appdir + '\uninsTasks.txt', selectedTasks) then
			if (Pos('modifypath', selectedTasks) > 0) then
				ModPath(appdir + '\bin');
			DeleteFile(appdir + '\uninsTasks.txt')
	end;
end;
