{%MainUnit pas2jsfileutils.pas}
{
    This file is part of the Free Component Library (FCL)
    Copyright (c) 2018  Mattias Gaertner  mattias@freepascal.org

    Unix backend of pas2jsfileutils

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************
}

function FilenameIsAbsolute(const aFilename: string): boolean;
begin
  Result:=FilenameIsUnixAbsolute(aFilename);
end;

function ExpandFileNamePJ(const FileName: string; BaseDir: string): string;
var
  IsAbs: Boolean;
  HomeDir, Fn: String;
begin
  Fn := FileName;
  ForcePathDelims(Fn);
  IsAbs := FileNameIsUnixAbsolute(Fn);
  if (not IsAbs) then
  begin
    if ((Length(Fn) > 1) and (Fn[1] = '~') and (Fn[2] = '/')) or (Fn = '~') then
    begin
      HomeDir := GetEnvironmentVariablePJ('HOME');
      if not FileNameIsUnixAbsolute(HomeDir) then
        HomeDir := ExpandFileNamePJ(HomeDir,'');
      Fn := HomeDir + Copy(Fn,2,length(Fn));
      IsAbs := True;
    end;
  end;
  if IsAbs then
  begin
    Result := ResolveDots(Fn);
  end
  else
  begin
    if (BaseDir = '') then
      Fn := IncludeTrailingPathDelimiter(GetCurrentDirPJ) + Fn
    else
      Fn := IncludeTrailingPathDelimiter(BaseDir) + Fn;
    Fn := ResolveDots(Fn);
    //if BaseDir is not absolute then this needs to be expanded as well
    if not FileNameIsUnixAbsolute(Fn) then
      Fn := ExpandFileNamePJ(Fn, '');
    Result := Fn;
  end;
end;

function GetCurrentDirPJ: String;
begin
  Result:=GetCurrentDir;
end;

function GetPhysicalFilename(const Filename: string; ExceptionOnError: boolean
  ): string;
var
  OldPath: String;
  NewPath: String;
  p: PChar;
begin
  Result:=Filename;
  p:=PChar(Result);
  repeat
    while p^='/' do
      inc(p);
    if p^=#0 then exit;
    if p^<>'/' then
    begin
      repeat
        inc(p);
      until p^ in [#0,'/'];
      OldPath:=LeftStr(Result,p-PChar(Result));
      NewPath:=ResolveSymLinks(OldPath,ExceptionOnError);
      if NewPath='' then exit('');
      if OldPath<>NewPath then
      begin
        Result:=NewPath+copy(Result,length(OldPath)+1,length(Result));
        p:=PChar(Result)+length(NewPath);
      end;
    end;
  until false;
end;

function ResolveSymLinks(const Filename: string; ExceptionOnError: boolean
  ): string;
var
  LinkFilename: string;
  AText: string;
  Depth: Integer;
begin
  Result:=Filename;
  Depth:=0;
  while Depth<12 do begin
    inc(Depth);
    LinkFilename:=fpReadLink(Result);
    if LinkFilename='' then
    begin
      AText:='"'+Filename+'"';
      case fpGetErrno() of
      ESysEAcces:
        AText:='read access denied for '+AText;
      ESysENoEnt:
        AText:='a directory component in '+AText
                            +' does not exist or is a dangling symlink';
      ESysENotDir:
        AText:='a directory component in '+AText+' is not a directory';
      ESysENoMem:
        AText:='insufficient memory';
      ESysELoop:
        AText:=AText+' has a circular symbolic link';
      else
        // not a symbolic link, just a regular file
        exit;
      end;
      if (not ExceptionOnError) then
      begin
        Result:='';
        exit;
      end;
      raise EFOpenError.Create(AText);
    end else begin
      if not FilenameIsAbsolute(LinkFilename) then
        Result:=ExtractFilePath(Result)+LinkFilename
      else
        Result:=LinkFilename;
    end;
  end;
  // probably an endless loop
  if ExceptionOnError then
    raise EFOpenError.Create('too many links, maybe an endless loop.')
  else
    Result:='';
end;

function IsUNCPath(const Path: String): Boolean;
begin
  Result := false;
  if Path='' then ;
end;

function ExtractUNCVolume(const Path: String): String;
begin
  Result := '';
  if Path='' then ;
end;

function FileIsWritable(const AFilename: string): boolean;
begin
  Result := BaseUnix.FpAccess(AFilename, BaseUnix.W_OK) = 0;
end;

function FileIsExecutable(const AFilename: string): boolean;
var
  Info : Stat;
begin
  // first check AFilename is not a directory and then check if executable
  Result:= (FpStat(AFilename,info{%H-})<>-1) and FPS_ISREG(info.st_mode) and
           (BaseUnix.FpAccess(AFilename,BaseUnix.X_OK)=0);
end;

function GetEnvironmentVariableCountPJ: Integer;
begin
  Result:=GetEnvironmentVariableCount;
end;

function GetEnvironmentStringPJ(Index: Integer): string;
begin
  Result:=ConsoleToUTF8(GetEnvironmentString(Index));
end;

function GetEnvironmentVariablePJ(const EnvVar: string): String;
begin
  Result:=ConsoleToUTF8(GetEnvironmentVariable(EnvVar));
end;

{$IFNDEF Darwin}
function GetUnixEncoding: string;
var
  i: integer;
begin
  Result:=EncodingSystem;
  i:=pos('.',Lang);
  if (i>0) and (i<=length(Lang)) then
    Result:=copy(Lang,i+1,length(Lang)-i);
end;
{$ENDIF}

function GetConsoleTextEncoding: string;
begin
  Result:=GetDefaultTextEncoding;
end;

function UTF8ToSystemCP(const s: string): string;
begin
  if NonUTF8System and not IsASCII(s) then
  begin
    Result:=UTF8ToAnsi(s);
    // prevent UTF8 codepage appear in the strings - we don't need codepage
    // conversion magic
    SetCodePage(RawByteString(Result), StringCodePage(s), False);
  end
  else
    Result:=s;
end;

function SystemCPToUTF8(const s: string): string;
begin
  if NonUTF8System and not IsASCII(s) then
  begin
    Result:=AnsiToUTF8(s);
    // prevent UTF8 codepage appear in the strings - we don't need codepage
    // conversion magic
    SetCodePage(RawByteString(Result), StringCodePage(s), False);
  end
  else
    Result:=s;
end;

function ConsoleToUTF8(const s: string): string;
begin
  Result:=SystemCPToUTF8(s);
end;

function UTF8ToConsole(const s: string): string;
begin
  Result:=UTF8ToSystemCP(s);
end;

procedure InitPlatform;
begin

end;

procedure FinalizePlatform;
begin

end;

