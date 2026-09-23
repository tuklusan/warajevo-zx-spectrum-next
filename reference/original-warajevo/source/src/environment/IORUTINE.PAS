unit iorutine;
{$F-}
{$L COMM.OBJ}
interface



procedure displayerror(poruka:string;kod:word);
procedure reseterror(kod:word;filename:string);
procedure blrderror(kod:word;filename:string);
procedure blwrterror(kod:word;filename:string);
procedure closeerror(kod:word;filename:string);
procedure seekerror(kod:word;filename:string);
procedure rewriteerror(kod:word;filename:string);
function load(warajevo:Boolean;filename:string;trenutni:longint;var buffer):Boolean;
function save(warajevo:Boolean;filename:string;duzina:word;
  trenutni:longint;fleg:byte;var buffer):Boolean;

function imedatoteke(var f:file):string;
function imedatoteket(var f:text):string;
function fresetu(var f:file;recsize:word):Boolean;
function frewriteu(var f:file;recsize:word):Boolean;
function fresett(var f:text):Boolean;
function frewritet(var f:text):Boolean;
function fblockread(var f:file;var buf;count:word):Boolean;
function fblockread2(var f:file;var buf;count:word;var rload:word):Boolean;
function fblockwrite(var f:file;var buf;count:word):Boolean;
function fclose(var f:file):Boolean;
function fcloset(var f:text):Boolean;

function fseek(var f:file;n:longint):Boolean;
function frename(var f:file;name:string):Boolean;
function ferase(var f:file):Boolean;
function fwriteln(var f:text;poruka:string):Boolean;
function fwrite(var f:text;poruka:string):Boolean;
function fchdir(direktorij:string):Boolean;
function freadt(var f:text;var poruka:string):Boolean;
function freadc(var f:text;var znak:char):Boolean;
function freadln(var f:text;var poruka:string):Boolean;
procedure ubafer(st:string);

procedure comm_setparam(param:integer);
function comm_install(port:integer):integer;
function comm_remove:integer;
procedure comm_put(c:integer);
function comm_get:integer;
function modem_status:integer;
function modem_dtr(state:integer):integer;
procedure clearvars;
function FileExists(FileName: string): Boolean;
const dfltrs232:integer=1;
      baudrates:array[1..8] of byte=($07,$27,$47,$67,$87,$A7,$C7,$E7);
      slbdrt:integer=8;
var emuldir,curemul,tempdir:String[80];

implementation
uses dos,crt,msgbox;
const
  criticalerror:array[0..12] of string[26] =
   ('Drive is write protected',
   'Unknown unit','Drive not ready','Unknown command',
   'CRC error in data','Bad request of size','Disk seek error',
   'Unrecognized media','Sector not found','Printer out of paper',
   'Device write fault','Device read fault','General failure');
var
  Done: Boolean;
  Ch: Char;
  helpfile:file;
  pozicija:word;
procedure comm_setparam(param:integer);external;
function comm_install(port:integer):integer;external;
function comm_remove:integer;external;
procedure comm_put(c:integer);external;
function comm_get:integer;external;
function modem_status:integer;external;
function modem_dtr(state:integer):integer;external;
procedure clearvars;external;


function smisleno(filename:string):string;
var i:integer;
begin
  smisleno:=filename;
  for i:=1 to length(filename) do
    if not(filename[i] in [#32..#255]) then smisleno:='file';
end;

procedure displayerror(poruka:string;kod:word);
const linwid=40;
var
  i:integer;
begin
   poruka:=chr(3)+poruka;
   while pos('#',poruka)>0 do begin
     i:=pos('#',poruka);
     insert(#13,poruka,i);
     poruka[i+1]:=chr(3)
   end;
   i:=messagebox(poruka,nil,mfError+mfOkButton);
end;

procedure recover;
var dummy:integer;
  regs:registers;
begin
  {$I-}
  regs.ah:=$0D;
  msdos(regs);
  {$I+}
  dummy:=ioresult
end;
procedure writeerror(kod:integer;filename:string);
var errorstring:string;
begin
  errorstring:='Error writing to#'+smisleno(filename);
  case kod of
    005:errorstring:=errorstring+'#(acess denied)';
    101:errorstring:=errorstring+'#(disk full)';
    103:errorstring:=errorstring+'#(file not open)';
    105:errorstring:=errorstring+'#(file not open for output)';
    150..162: errorstring:=errorstring+'#DOS reports:#'+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4003);
  recover;
  exit;
end;

procedure reseterror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:=smisleno(filename)+'#can not be opened for read&write';
  case kod of
    002: errorstring:=errorstring+'#(file not found)';
    003: errorstring:=errorstring+'#(path not found)';
    004: errorstring:=errorstring+'#(too many files)';
    005: errorstring:=errorstring+'#(directory or read only)';
    012: errorstring:=errorstring+'#(filemode is invalid)';
    102: errorstring:=errorstring+'#(not assigned)';
    150..162: errorstring:=errorstring+'#DOS reports:# '+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4000);
  recover;
  exit;
end;

procedure blrderror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:='Error while reading# '+smisleno(filename);
  case kod of
    005:errorstring:=errorstring+'#(file not opened for reading)';
    100:errorstring:=errorstring+'#(end of file passed)';
    103:errorstring:=errorstring+'#(file not open)';
    104:errorstring:=errorstring+'#(file not open for input)';
    150..162: errorstring:=errorstring+'#DOS reports:# '+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4001);
  recover;
  exit;
end;

procedure blwrterror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:='Error while writing# '+smisleno(filename);
  case kod of
    005:errorstring:=errorstring+'#(file not opened for writing)';
    101:errorstring:=errorstring+'#(disk full)';
    103:errorstring:=errorstring+'#(file not open)';
    105:errorstring:=errorstring+'#(file not open for output)';
    150..162: errorstring:=errorstring+'.#DOS reports:# '+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4002);
  recover;
  exit;
end;

procedure closeerror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:='Error while closing# '+smisleno(filename);
  case kod of
    101:errorstring:=errorstring+'#(disk full)';
    103:errorstring:=errorstring+'#(file not open)';
    150..162: errorstring:=errorstring+'#DOS reports:# '+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4003);
  recover;
  exit;
end;

procedure seekerror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:='Error while seeking on'+smisleno(filename);
  case kod of
    103:errorstring:=errorstring+'#(file not open)';
    150..162: errorstring:=errorstring+'.#DOS reports:#'+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4004);
  recover;
  exit;
end;

procedure rewriteerror(kod:word;filename:string);
var
  izbor:integer;
  errorstring:string;
begin
  errorstring:=smisleno(filename)+'# can not be created or rewritten.';
  case kod of
    002: errorstring:=errorstring+'#(file not found)';
    003: errorstring:=errorstring+'#(path not found)';
    004: errorstring:=errorstring+'#(too many files)';
    005: errorstring:=errorstring+'#(directory or read only)';
    012: errorstring:=errorstring+'#(filemode is invalid)';
    102: errorstring:=errorstring+'#(not assigned)';
    150..162: errorstring:=errorstring+'#DOS reports:#'+
      criticalerror[kod-150]
  end;
  displayerror(errorstring,4005);
  recover;
  exit;
end;
procedure renameerror(kod:word;filename:string);
var errorstring:string;
a:integer;
begin
  errorstring:='Error renaming file# '+smisleno(filename);
  case kod of
    002:errorstring:=errorstring+'#(file not found)';
    003:errorstring:=errorstring+'#(path not found)';
    005:errorstring:=errorstring+'#(file acess denied)';
    006:errorstring:=errorstring+'#(invalid file handle)';
    017:errorstring:=errorstring+'#(can not rename accross drives)';
    150..162: errorstring:=errorstring+'#DOS reports:# '+
      criticalerror[kod-150]
   end;
   displayerror(errorstring,4002);
   recover;

end;
procedure eraseerror(kod:word;filename:string);
var errorstring:string;
begin
  errorstring:='Error erasing '+smisleno(filename);
  case kod of
    002:errorstring:=errorstring+'#(file not found)';
    003:errorstring:=errorstring+'#(path not found)';
    005:errorstring:=errorstring+'#(file acess denied)';
    006:errorstring:=errorstring+'#(invalid file handle)';
    102:errorstring:=errorstring+'#(file not assigned)';
    150..162: errorstring:=errorstring+'.#DOS reports:#'+
      criticalerror[kod-150]
   end;
   displayerror(errorstring,4032);
   recover;
end;

procedure chdirerror(kod:word;direktorij:string);
var errorstring:string;
begin
  errorstring:='Error changing to directory#'+direktorij;
  case kod of
    003:errorstring:=errorstring+'#(path not found)';
    005:errorstring:=errorstring+'#(file acess denied)';
    015:errorstring:=errorstring+'#(invalid drive number)';
    150..162: errorstring:=errorstring+'#DOS reports:# '+
      criticalerror[kod-150]
   end;
   displayerror(errorstring,4033);
   recover;
end;

function imedatoteke(var f:file):string;
var izlazni:string;
    i:integer;
begin
  izlazni:='';
  i:=0;
  while filerec(f).name[i]<>#0 do begin
     izlazni:=izlazni+filerec(f).name[i];
     inc(i)
  end;
  imedatoteke:=izlazni;
end;

function imedatoteket(var f:text):string;
var izlazni:string;
    i:integer;
begin
  izlazni:='';
  i:=0;
  while textrec(f).name[i]<>#0 do begin
     izlazni:=izlazni+textrec(f).name[i];
     inc(i)
  end;
  imedatoteket:=izlazni;
end;
function fchdir(direktorij:string):Boolean;
var a:integer;
begin
  {$I-}
  chdir(direktorij);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    chdirerror(a,direktorij);
    fchdir:=false;
  end
   else fchdir:=true
end;

function frename(var f:file;name:string):Boolean;
var a:integer;
begin
  {$I-}
  rename(f,name);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    renameerror(a,imedatoteke(f));
    frename:=false;
  end
   else frename:=true
end;

function fwrite(var f:text;poruka:string):Boolean;
var a:integer;
begin
  {$I-}
  write(f,poruka);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    writeerror(a,imedatoteket(f));
    fwrite:=false;
  end
   else fwrite:=true
end;

function fwriteln(var f:text;poruka:string):Boolean;
var a:integer;
begin
  {$I-}
  writeln(f,poruka);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    writeerror(a,imedatoteket(f));
    fwriteln:=false;
  end
   else fwriteln:=true
end;

function freadt(var f:text;var poruka:string):Boolean;
var a:integer;
begin
  {$I-}
  read(f,poruka);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blrderror(a,imedatoteket(f));
    freadt:=false;
  end
   else freadt:=true
end;

function freadc(var f:text;var znak:char):Boolean;
var a:integer;
begin
  {$I-}
  read(f,znak);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blrderror(a,imedatoteket(f));
    freadc:=false;
  end
   else freadc:=true
end;

function freadln(var f:text;var poruka:string):Boolean;
var a:integer;
begin
  {$I-}
  readln(f,poruka);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blrderror(a,imedatoteket(f));
    freadln:=false;
  end
   else freadln:=true
end;

function ferase(var f:file):Boolean;
var a:integer;
begin
  {$I-}
  erase(f);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    eraseerror(a,imedatoteke(f));
    ferase:=false;
  end
   else ferase:=true
end;

function fresetu(var f:file;recsize:word):Boolean;
var a:integer;
attr:word;
begin
  {$I-}
  GetFAttr(f,Attr);
  if (attr and readonly)<>0 then filemode:=0 else filemode:=2;
  reset(f,recsize);
  filemode:=2;
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    reseterror(a,imedatoteke(f));
    fresetu:=false;
  end
   else fresetu:=true
end;

function frewriteu(var f:file;recsize:word):Boolean;
var a:integer;
begin
  {$I-}
  FileMode:=2;
  rewrite(f,recsize);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    rewriteerror(a,imedatoteke(f));
    frewriteu:=false;
  end
   else frewriteu:=true
end;

function fresett(var f:text):Boolean;
var a:integer;
attr:word;
begin
  {$I-}
  GetFAttr(f,Attr);
  if (attr and readonly)<>0 then filemode:=0 else filemode:=2;
  reset(f);
  filemode:=2;
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    reseterror(a,imedatoteket(f));
    fresett:=false;
  end
   else fresett:=true
end;

function frewritet(var f:text):Boolean;
var a:integer;
begin
  {$I-}
  filemode:=2;
  rewrite(f);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    rewriteerror(a,imedatoteket(f));
    frewritet:=false;
  end
   else frewritet:=true
end;

function fblockread(var f:file;var buf;count:word):Boolean;
var a:integer;
begin
  {$I-}
  blockread(f,buf,count);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blrderror(a,imedatoteke(f));
    fblockread:=false;
  end
   else fblockread:=true
end;

function fblockread2(var f:file;var buf;count:word; var rload:word):Boolean;
var a:integer;
begin
  {$I-}
  blockread(f,buf,count,rload);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blrderror(a,imedatoteke(f));
    fblockread2:=false;
  end
   else fblockread2:=true
end;

function fblockwrite(var f:file;var buf;count:word):Boolean;
var a:integer;
begin
  {$I-}
  blockwrite(f,buf,count);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    blwrterror(a,imedatoteke(f));
    fblockwrite:=false;
  end
   else fblockwrite:=true
end;

function fclose(var f:file):Boolean;
var a:integer;
begin
  {$I-}
  close(f);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    closeerror(a,imedatoteke(f));
    fclose:=false;
  end
   else fclose:=true
end;
function fcloset(var f:text):Boolean;
var a:integer;
begin
  {$I-}
  close(f);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    closeerror(a,imedatoteket(f));
    fcloset:=false;
  end
   else fcloset:=true

end;

function fseek(var f:file;n:longint):Boolean;
var a:integer;
begin
  {$I-}
  seek(f,n);
  {$I+}
  a:=ioresult;
  if a<>0 then begin
    seekerror(a,imedatoteke(f));
    fseek:=false;
  end
   else fseek:=true
end;

function load(warajevo:Boolean;filename:string;trenutni:longint;var buffer):Boolean;
var
  duzina,duzdec1,duzcomp1,duzsign1:word;
  datoteka:file;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka);
    load:=false;
  end;
begin
  load:=false;
  assign(datoteka,filename);
  if not(fresetu(datoteka,1)) then exit;
  if not(warajevo) then begin
    if not(fseek(datoteka,trenutni)) then begin whenexit;exit end;
    if not(fblockread(datoteka,duzina,2)) then begin whenexit;exit end;
    if not(fblockread(datoteka,buffer,1)) then begin whenexit;exit end;
    if not(fblockread(datoteka,buffer,duzina-2)) then begin whenexit;exit end;
    if not(fclose(datoteka)) then begin load:=false;exit; end;
    load:=true;
    exit
  end;
  if not(fseek(datoteka,trenutni+8)) then begin whenexit;exit end;
  if not(fblockread(datoteka,duzina,2)) then begin whenexit;exit end;
  (* Specijalno ako je duzina 65535 *)
  if duzina>=65534 then begin
    if not(fblockread(datoteka,duzdec1,1)) then begin whenexit;exit end;
      {fleg}
    if not(fblockread(datoteka,duzdec1,2)) then begin whenexit;exit end;
    if not(fblockread(datoteka,duzcomp1,2)) then begin whenexit;exit end;
    if not(fblockread(datoteka,duzsign1,2)) then begin whenexit;exit end;
    duzina:=duzcomp1+6;
    if not(fseek(datoteka,trenutni+10)) then begin whenexit;exit end;
  end;
  if not(fblockread(datoteka,buffer,1)) then begin whenexit;exit end;
  if not(fblockread(datoteka,buffer,duzina)) then begin whenexit;exit end;
  if not(fclose(datoteka)) then begin load:=false;exit; end;
  load:=true;
end;

function save(warajevo:Boolean;filename:string;duzina:word;
  trenutni:longint;fleg:byte;var buffer):Boolean;
type konvtip=array[1..4] of word;
  konvtip2=array[0..65520] of byte;

var
  actdriv,back,new_dat,dummy,snima_bajtova:longint;
  paritet,bajt:byte;
  duzina1,j:word;
  datoteka:file;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka);
    save:=false;
  end;
begin
  filename:=fexpand(filename);
  actdriv:=ord(upcase(filename[1]))-64;
  save:=false;
  if duzina<65534 then
    if diskfree(actdriv)<duzina*1.1 then begin
      window(1,1,80,25);
      displayerror('Disk almost full. Block will not be saved',4006);
      exit
    end;
  if duzina>=65534 then begin
     snima_bajtova:=konvtip(buffer)[2]+6;
     if diskfree(actdriv)<snima_bajtova*1.1 then begin
       window(1,1,80,25);
       displayerror('Disk almost full. Block will not be saved',4006);
       exit
     end;
  end;
  assign(datoteka,filename);
  if not(fresetu(datoteka,1)) then exit;
  if not(warajevo) then begin
    if not(fseek(datoteka,filesize(datoteka))) then begin whenexit;exit end;
    duzina1:=duzina+2;
    if not(fblockwrite(datoteka,duzina1,2)) then begin whenexit;exit end;
    if not(fblockwrite(datoteka,fleg,1)) then begin whenexit;exit end;
    if duzina<>0 then
      if not(fblockwrite(datoteka,buffer,duzina)) then begin
         whenexit;exit
      end;
     paritet:=fleg;
     for j:=0 to duzina-1 do begin
       bajt:=konvtip2(buffer)[j];
       paritet:=paritet xor bajt;
     end;

    if not(fblockwrite(datoteka,paritet,1)) then begin whenexit; exit end;
    if not(fclose(datoteka)) then exit ;
    save:=true;
    exit;
  end;
  if trenutni=0 then begin
      if not(fblockread(datoteka,trenutni,4)) then begin whenexit;exit end;
  end;
  if not(fseek(datoteka,trenutni)) then begin whenexit;exit end;
  if not(fblockread(datoteka,back,4)) then begin whenexit;exit end;
  if not(fseek(datoteka,filesize(datoteka))) then begin whenexit;exit end;
  new_dat:=filepos(datoteka);
  if not(fseek(datoteka,trenutni)) then begin whenexit;exit end;
  if not(fblockwrite(datoteka,new_dat,4)) then begin whenexit;exit end;
  if not(fseek(datoteka,back)) then begin whenexit;exit end;
  if back<>0 then if not(fblockread(datoteka,dummy,4)) then begin
    whenexit;exit
  end;
  if not(fblockwrite(datoteka,new_dat,4)) then begin whenexit;exit end;
  if not(fseek(datoteka,new_dat)) then begin whenexit;exit end;
  if not(fblockwrite(datoteka,back,4)) then begin whenexit;exit end;
  if not(fblockwrite(datoteka,trenutni,4)) then begin whenexit;exit end;
  if not(fblockwrite(datoteka,duzina,2)) then begin whenexit;exit end;
  if not(fblockwrite(datoteka,fleg,1)) then begin whenexit;exit end;
  snima_bajtova:=konvtip(buffer)[2]+6;

  (* Specijalno ako je duzina 65535 *)

  if duzina>=65534 then
    if not(fblockwrite(datoteka,buffer,snima_bajtova)) then begin
       whenexit;exit
    end;
  if (duzina<>0) and (duzina<65534) then
    if not(fblockwrite(datoteka,buffer,duzina)) then begin
       whenexit;exit
    end;
  if not(fclose(datoteka)) then exit ;
  save:=true;
end;
function FileExists(FileName: string): Boolean;
var
 f: file;
begin
 {$I-}
 Assign(f, FileName);
 FileMode:=0;
 Reset(f);
 Close(f);
 FileMode:=2;
 {$I+}
 FileExists := (IOResult = 0) and (FileName <> '');
end;  { FileExists }


procedure ubafer(st:string);
var regs:registers;
  i:integer;

begin
  for i:=1 to length(st) do begin
    regs.ah:=5;
    regs.cl:=ord(st[i]);
    intr($16,regs);
  end;
end;

begin
  clearvars;
  asm
    mov sp,bp
  end

end.
