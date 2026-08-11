{$V-}
{$O+}

unit timex;
interface

type TDockfile=object
   dockname:string;
   allowed:Boolean;
   Buf1:Pointer;
   constructor init(dcknme:string;mp1:pointer);
   function select:Boolean;
   procedure unselect;
   procedure View;
   procedure binaros;
   procedure binlros;
   procedure BinArosLRos(tip:integer);
   procedure romdck;
   procedure merge;
   procedure getheaders(offset:integer);
end;

PDockFile=^TDockFile;

implementation

uses helpctx,dialogs,iorutine,stddlg,msgbox,moucur,memory,
  menucons,views,app,dos,konvtap,objects;

type dckheader=record
  btype:byte;
  ctype:array[1..8] of byte;
end;

constructor TDockFile.init(dcknme:string;mp1:pointer);
begin
  dockname:='';
  if dcknme='' then allowed:=false
    else begin
      allowed:=true;
      dockname:=dcknme;
    end;
  buf1:=mp1;
end;

{
********************************************
    Opcija Select dock
********************************************
}

function TDockFile.select:Boolean;
var a:PFileDialog;
  c:integer;
  fname:string;
  datoteka:file;
  i:word;
 procedure we;
  var cc:boolean;
  begin
    cc:=fclose(datoteka);
    allowed:=false;
    dispose(a,done);
    select:=false;
  end;

begin
  a:=new(PFileDIalog,Init('*.DCK','Select dock name','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcdockselect;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(fname);
    fname:=fexpand(fname);
    if not(fileexists(fname)) then begin
      displayerror('Dock file does not exist',0);
      we; exit;
    end;
    dockname:=fname;
    allowed:=true;
    select:=true;
  end
   else select:=false;
  dispose(a,done);
end;

procedure TDockFile.unselect;
var d,n,e:string;
  i:word;
begin
  hourglascur;
  dockname:='';
  allowed:=false;
end;
type TDckHeaderView=object(TListBox)
  inf:Pointer;
  function GetText(item:integer; maxlen:integer):string;virtual;
  constructor init(var Bounds:TRect; ANumCols:
   Integer; AScrollBar: PScrollBar; pp:Pointer);
end;
PDckHeaderView=^TDckHeaderView;

function TDckHeaderView.GetText(item:integer; maxlen:integer):string;
type konvtip=array[0..7280] of dckHeader;
var n,k,i:integer;
dh:dckheader;
bn,st,gt:string;
begin
  n:=item div 10;
  str(n,bn);
  dh:=konvtip(inf^)[n+1000];
  case item mod 10 of
    0: GT:='=============================================';
    1: if dh.btype=n then
       case n of
         0: GT:=bn+' Dock bank present';
         254: GT:=bn+' ExROM bank present';
         255: GT:=bn+' Home bank present';
         else GT:=bn+' bank present'
       end
       else GT:='';
    2..9:
     if dh.btype=n then begin
       k:=(item mod 10)-1;
       str(k,st);
       st:='  Chunk '+st+' ';
       if (dh.ctype[k] and 1)=0 then st:=st+'Read only ' else St:=St+'Read/Write ';
       if (dh.ctype[k] and 2)=0 then st:=st+'Absent ' else St:=St+'Present ';
       GT:=st
     end;
  end;
  for i:=length(GT) to maxlen do gt:=gt+' ';
  GetText:=gt;
end;
constructor TDckHeaderView.init(var Bounds:TRect; ANumCols:
   Integer; AScrollBar: PScrollBar; pp:pointer);
begin
TListBox.init(Bounds, ANumCols, AScrollBar);
inf:=pp;
end;

Procedure TDockFile.View;
var a: PDialog;
  R:TRect;
  skrolnica:PScrollBar;
  ll:PDckheaderView;
  Bruce:Pointer;
  c:word;
begin
   if not(allowed) then exit;
   getheaders(1000);
   R.assign(5,2,75,22);
   a:=new(PDialog,Init(R,'View'));
   a^.helpctx:=hcdockview;
   R.assign(68,1,69,15);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(55,16,65,18);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.assign(1,1,67,15);
   ll:=new(PDckHeaderView,Init(R,1,skrolnica,buf1));
   ll^.setrange(2560);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   dispose(a,done);
end;

Procedure TDockFile.BinArosLRos(tip:integer);
var
  datoteka,izlaz:file;
  cc:boolean;
  binfilename,search,tmp,d,n,e,outdockname:string;
  dirinfo: searchrec;
  dh:DCKHeader;
  a:PFileDialog;
  fs,lim1:longint;
  i:integer;
  start:word;
  ct:byte;
  procedure we;
  begin
    cc:=fclose(datoteka);
    cc:=fclose(izlaz);
  end;

begin
  search:='.BIN';
  if tip=3 then search:='.ROM';
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  case tip of
    1: begin a^.helpctx:=hcbinaros;lim1:=32768; start:=5;ct:=0 end;
    2: begin a^.helpctx:=hcbinlros;lim1:=65536; start:=1; ct:=0 end;
    3: begin a^.helpctx:=hcromtodck; start:=1; ct:=1 end;
  end;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(binfilename);
    dispose(a,done);
    binfilename:=fexpand(binfilename);
    if not(fileexists(binfilename)) then
     displayerror('File#'+binfilename+'#not found',0)
    else begin
      assign(datoteka,binfilename);
      fsplit(binfilename,d,n,e);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      fs:=filesize(datoteka);
      if (tip=3) and (fs<>16384) then begin
        displayerror('Only ROM files of 16K supported',0);
        cc:=fclose(datoteka); exit
      end
      else
      if fs>lim1 then begin
        displayerror('File too big',0);
        cc:=fclose(datoteka); exit
      end;
      outdockname:=outputfile(n+'.DCK',binfilename);
      hourglascur;
      assign(izlaz,outdockname);
      if not(frewriteu(izlaz,1)) then begin we; exit end;

      dh.btype:=0;
      if tip=3 then dh.btype:=255;
      for i:=1 to 8 do dh.ctype[i]:=ct;
      if not(fblockwrite(izlaz,dh,9)) then begin; we; exit; end;
      i:=start;
      while fs>0 do begin
        if fs>8192 then begin
          if not(fblockread(datoteka,buf1^,8192)) then begin we;exit end;
          fs:=fs-8192;
        end
         else begin
           fillchar(buf1^,8192,0);
           if not(fblockread(datoteka,buf1^,fs)) then begin we;exit end;
           fs:=0;
         end;
         if not(fblockwrite(izlaz,buf1^,8192)) then begin we; exit end;
         dh.ctype[i]:=2;
         inc(i);
      end;
    end;
  if not(fseek(izlaz,0)) then begin we; exit end;

  if not(fblockwrite(izlaz,dh,9)) then begin we; exit end;
  we;
  end
   else dispose(a,done);
end;


Procedure TDockFile.BinLRos;
begin
  BinArosLros(2);
end;
Procedure TDockFile.BinARos;
begin
  BinArosLros(1);
end;

Procedure TDockFile.RomDck;
begin
  BinArosLros(3);
end;

Procedure TDockFile.Merge;
type konvtip=array[0..7280] of dckHeader;
var a:PFileDialog;
dh1,dh2:dckheader;
dock2,sta,fname:String;
conflict,cc:Boolean;
i,j:integer;
datoteka,izlaz:file;
fs:longint;
  procedure we;
  begin
    cc:=fclose(datoteka);
    cc:=fclose(izlaz);
  end;

begin
  a:=new(PFileDIalog,Init('*.DCK','Select dock name to be merged','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcdockmerge;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(fname);
    fname:=fexpand(fname);
    if not(fileexists(fname)) then begin
      displayerror('Dock file does not exist',0);
      dispose(a,done); exit;
    end;
    dock2:=fname;
  end
   else begin
    dispose(a,done);
    exit
   end;
  dispose(a,done);
  getheaders(1000);
  sta:=dockname;
  dockname:=dock2;
  getheaders(2000);
  dockname:=sta;
  conflict:=false;
  for i:=0 to 255 do begin
    dh1:=konvtip(buf1^)[1000+i];
    dh2:=konvtip(buf1^)[2000+i];
    if (dh1.btype=dh2.btype) and (dh1.btype=i) then begin
      for j:=1 to 8 do begin
        if (dh1.ctype[j]) * (dh2.ctype[j])<>0 then conflict:=true;
      end
    end
  end;
  if conflict then DisplayError('Conflict in docks. Same memory used.',0)
  else begin
      assign(datoteka,dock2);
      assign(izlaz,dockname);
      if not(fresetu(datoteka,1)) then exit;
      if not(fresetu(izlaz,1)) then begin cc:=fclose(datoteka); exit end;
      if not(fseek(izlaz,filesize(izlaz))) then begin we; exit end;
      fs:=filesize(datoteka);
      while fs>0 do begin
        if fs>8192 then begin
          if not(fblockread(datoteka,buf1^,8192)) then begin we;exit end;
          if not(fblockwrite(izlaz,buf1^,8192)) then begin we; exit end;
          fs:=fs-8192;
        end
         else begin
           fillchar(buf1^,8192,0);
           if not(fblockread(datoteka,buf1^,fs)) then begin we;exit end;
           if not(fblockwrite(izlaz,buf1^,fs)) then begin we; exit end;
           fs:=0;
         end;
       end;
       we
  end;
end;

procedure TDockFile.GetHeaders(offset:integer);
{offset mora biti veci od 1000 jer se prvi dio bafera koristi za GET}
type konvtip=array[0..7280] of dckHeader;
var dh:DckHeader;
datoteka:File;
cc:Boolean;
i:integer;
procedure we;
begin
  cc:=fclose(datoteka);
end;

begin
  fillchar(konvtip(buf1^)[offset],2500,0);
  assign(datoteka,dockname);
  if not(fresetu(datoteka,1)) then begin we; exit end;
  dh.btype:=255;
  konvtip(buf1^)[offset]:=dh;
  while not eof(datoteka) do begin
    if not(fblockread(datoteka,dh,9)) then begin we; exit end;
    konvtip(buf1^)[dh.btype+offset]:=dh;
    for i:=1 to 8 do if dh.ctype[i] and 2 <>0 then
      if not(fblockread(datoteka,buf1^,8192)) then begin we; exit end;
  end;
  we;
end;

end.


