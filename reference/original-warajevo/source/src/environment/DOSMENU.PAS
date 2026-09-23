unit dosmenu;
{$F+,O+}

interface

type TDos=object
  procedure changedir;
  procedure directory;
  procedure osshell;
  procedure about;
end;
Pdos=^TDos;

implementation
uses stddlg,app,dialogs,drivers,objects,memory,dos,msgbox;

procedure TDos.about;
var c:integer;
begin
  c:=messagebox(
    ^C'Warajevo ZX Spectrum emulator'+#13+
    ^C'Version 2.51, December 1998'+#13+
    ^C'Integrated environment'
    ,nil,mfinformation+mfokbutton);
end;

procedure TDos.directory;
var a:pfiledialog;
  c:integer;
begin
  a:=new(PFileDIalog,Init('*.*','Directory','',0,100));
  c:=desktop^.execview(a);
  dispose(a,done);
end;

procedure TDos.ChangeDir;
var
  D: PChDirDialog;
  c:word;
begin
  D := New(PChDirDialog, Init(cdNormal, 101));
  c:=DeskTop^.ExecView(D);
  Dispose(D, Done);
end;


procedure TDos.OSShell;
begin
  DoneSysError;
  DoneEvents;
  DoneVideo;
  DoneMemory;
  
  SetMemTop(HeapPtr);
  
  {$IFNDEF GR}
  PrintStr('Type EXIT to return...');
  {$ENDIF}
  SwapVectors;
  Exec(GetEnv('COMSPEC'), '');
  SwapVectors;
  
  SetMemTop(HeapEnd);
  

  InitMemory;
  InitVideo;
  InitEvents;
  InitSysError;
  desktop^.redraw;
  menubar^.draw;
end;


end.
