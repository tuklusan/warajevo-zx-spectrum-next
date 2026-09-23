
unit dbase;
{$O+,V-,R+}
Interface

Type
  Fileitem= Record
              Tip: Char;
              Filename: String [10];
              pozicija:integer;
            End;
  Pfileitem= ^Fileitem;
  exrecdata=record
    fi:Fileitem;   {tip,filename}
    direct:String[36];
    vollab:String[11];
    kompjuter:string[4];
    pozicija:string[3];
    simulopc:string[50]
  end;

Const
  sirindex=40;
  allocsirindex:integer=sirindex;
  Printpars:Record
    Stauklj:Word; {Checkboxes}
    Device : Word; {RadioButtons}
    Replen : LongInt; {Inputlong}
    Printable : LongInt; {Inputlong}
    Repwid : LongInt; {Inputlong}
    Which : Word; {RadioButtons}
    Prntype : Word; {RadioButtons}
  End=(Stauklj:1;device:0;replen:72;printable:66;repwid:80;
    which:0;prntype:0);

  procedure selectdbdir;
  procedure allocdb;
  procedure deallocdb;
  procedure dbsort;
  procedure dbreport;
  procedure dbdisplay;
  function dbRun(var progelem:exrecdata):integer;
  procedure markselected;
  procedure swappriority;
  procedure unmarkall;
  procedure sgdwara;
  Function Rtrim (A: String): String;
  function dbstart(koji:string;var progelem:exrecdata):Boolean;

  function bazafield(field:word):string;
  procedure warajevospecpic;
  procedure warajevosgd;
  procedure warajevorainbow;
  procedure warajevospecbase;
  procedure specbasewarajevo;
  procedure specpicrainWara(rainbow:Boolean);

Implementation

Uses Objects,editors, Drivers, Dialogs, Menus, App, Views, Memory, Histlist,
   Hexedito,empty,stddlg,dos,msgbox,
  iorutine,konvtap,setup,helpctx,moucur;

Const

  dbtname:string[80]='';

type dbf=record
  dbfname:string;
  recnum:longint;
  reclen:word;
  headlen:word;
  reccount:longint;
  opened:Boolean;
  datoteka:file;
end;
type tsoftbuf=record
  deleted:char;
  skraceno:array[1..10] of char;
  punoime:array[1..30] of char;
  proizvodj:array[1..4] of char;
  vrsta:array[1..4] of char;
  godiste:array[1..4] of char;
  kompjuter:array[1..4] of char;
  materjezik:array[1..4] of char;
  traka:array[1..10] of char;
  pozicija:array[1..3] of char;
  dodatni:array[1..10] of char;
  problemi:array[1..5] of char;
  simulopc:array[1..50] of char;
  opis:array[1..10] of char;
  prioritet: char;
end;
var softbuf:tsoftbuf;
addbuf:record
  deleted:char;
  redpodat:array[1..60] of char
end;
spbase:record
  deleted:char;
  filename:array[1..12] of char;
  full_name:array[1..30] of char;
  file_type:array[1..14] of char;
  size:array[1..7] of char;
  machine:array[1..3] of char;
  category:array[1..15] of char;
  developer:array[1..30] of char;
  language:array[1..15] of char;
  year:array[1..4] of char;
  remarks:array[1..10] of char;
  path:array[1..60] of char;
  emulator:array[1..10] of char;
end;

starikraj:longint;
prazna:char;
brojac:longint;
dummybytes:word;

procedure replace(var sta; cime:string; koliko:word);
var i:integer;
begin
  for i:=1 to 255 do cime:=cime+' ';
  fillchar(sta,koliko,' ');
  if cime<>'' then
    move(cime[1],sta,koliko);
end;

procedure addreplace(field:word; cime:string);
begin
  case field of
    1: replace(addbuf.redpodat,cime,sizeof(addbuf.redpodat));
  end
end;

procedure bazareplace(field:word; cime:string);
begin
  with softbuf do begin
    case field of
       1: replace(skraceno,cime,10);
       2: replace(punoime,cime,30);
       3: replace(proizvodj,cime,4);
       4: replace(vrsta,cime,4);
       5: replace(godiste,cime,4);
       6: replace(kompjuter,cime,4);
       7: replace(materjezik,cime,4);
       8: replace(traka,cime,10);
       9: replace(pozicija,cime,3);
      10: replace(dodatni,cime,10);
      11: replace(problemi,cime,5);
      12: replace(simulopc,cime,50);
      13: replace(opis,cime,10);
      14: replace(prioritet,cime,1);
    end;
  end
end;
procedure prebaci(var source; var dest:string; count:word);
begin
  move(source,dest[1],count);
  dest[0]:=chr(count);
end;

function addfield(field:word):string;
var buf:string;
begin
  buf:='';
  case field of
    1: prebaci(addbuf.redpodat,buf,sizeof(addbuf.redpodat));
  end;
  addfield:=buf;
end;

function bazafield(field:word):string;
var buf:string;
begin
  buf:='';
  with softbuf do begin
    case field of
       1: prebaci(skraceno,buf,10);
       2: prebaci(punoime,buf,30);
       3: prebaci(proizvodj,buf,4);
       4: prebaci(vrsta,buf,4);
       5: prebaci(godiste,buf,4);
       6: prebaci(kompjuter,buf,4);
       7: prebaci(materjezik,buf,4);
       8: prebaci(traka,buf,10);
       9: prebaci(pozicija,buf,3);
      10: prebaci(dodatni,buf,10);
      11: prebaci(problemi,buf,5);
      12: prebaci(simulopc,buf,50);
      13: prebaci(opis,buf,10);
      14: prebaci(prioritet,buf,1);
    end;
  end;
  bazafield:=buf;
end;

procedure getdbrec(var baza:dbf;slog:longint;var buffer);
var cc:boolean;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.recnum:=slog;
end;
begin
  with baza do begin
    if not(opened) then begin
      assign(datoteka,baza.dbfname);
      if not(fresetu(datoteka,1)) then exit;
    end;
    if not(fseek(datoteka,baza.headlen+(slog-1)*baza.reclen)) then begin
      we;exit;
    end;
    if not(fblockread(datoteka,buffer,baza.reclen)) then exit;
    if not(opened) then cc:=fclose(datoteka);
    recnum:=slog
  end;
end;

procedure putdbrec(var baza:dbf;slog:longint;var buffer);
var cc:Boolean;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.recnum:=slog;
end;

begin
  with baza do begin
    if not(opened) then begin
      assign(datoteka,baza.dbfname);
      if not(fresetu(datoteka,1)) then exit;
    end;
    if not(fseek(datoteka,baza.headlen+(slog-1)*baza.reclen)) then begin
      we;exit
    end;
    if not(fblockwrite(datoteka,buffer,baza.reclen)) then begin we; exit end;
    if not(opened) then cc:=fclose(datoteka);
    recnum:=slog;
  end;
end;

procedure Opendfile(var baza:dbf);
begin
  with baza do begin
    assign(datoteka,baza.dbfname);
    if not(fresetu(datoteka,1)) then begin opened:=false; exit end;
    opened:=true;
  end
end;

procedure Closedfile(var baza:dbf);
var cc:Boolean;
begin
  with baza do begin
    cc:=fclose(datoteka);
    opened:=false;
  end
end;

procedure zap(var baza:dbf;var buffer;count:word);
var cc:Boolean;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.recnum:=0;
  baza.opened:=false;
end;
begin
  with baza do begin
    assign(datoteka,baza.dbfname);
    if not(frewriteu(datoteka,1)) then exit;
    if not(fblockwrite(datoteka,buffer,count)) then begin we; exit end;
    cc:=fclose(datoteka);
    baza.recnum:=0;
    opened:=false;
  end;
end;

procedure zapdbt(dbtname:string;var buffer;count:word);
var datoteka:file;
var cc:Boolean;
procedure we;
begin
  cc:=fclose(datoteka);
end;
begin
  assign(datoteka,dbtname);
  if not(frewriteu(datoteka,1)) then exit;
  if not(fblockwrite(datoteka,buffer,count)) then begin we; exit end;
  cc:=fclose(datoteka);
end;

procedure deletedbrec(var baza:dbf;slog:longint);
var buffer:array[1..1024] of byte;
  rc:longint;
  cc:boolean;
  const eofsign:char=#$1A;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.opened:=false;
end;

begin
  with baza do begin
    if slog>reccount then exit;

    if not(opened) then begin
      assign(datoteka,baza.dbfname);
      if not(fresetu(datoteka,1)) then exit;
    end;
    if not(fseek(datoteka,4)) then begin we; exit end;
    if not(fblockread(datoteka,rc,4)) then begin we; exit end;
    dec (rc);
    if not(fseek(datoteka,4)) then begin we; exit end;
    if not(fblockwrite(datoteka,rc,4)) then begin we; exit end;
    if not(fseek(datoteka,baza.headlen+(reccount-1)*baza.reclen)) then begin
      we; exit
    end;
    if not(fblockread(datoteka,buffer,baza.reclen)) then begin we; exit end;
    if not(fseek(datoteka,baza.headlen+(slog-1)*baza.reclen)) then begin
      we;exit
    end;
    if not(fblockwrite(datoteka,buffer,baza.reclen)) then begin we; exit end;
    if not(fseek(datoteka,baza.headlen+(reccount-1)*baza.reclen)) then begin
      we; exit
    end;
    {$I-}
    truncate(datoteka);
    {$I+}
    if IORESULT<>0 then exit;
    if not(fblockwrite(datoteka,eofsign,1)) then begin we; exit end;
    if not(opened) then cc:=fclose(datoteka);
    reccount:=rc;
    recnum:=slog;
    if recnum>reccount then recnum:=reccount;
  end
end;

procedure Adddbrec(var baza:dbf;var buffer);
var
  slog:longint;
  cc:Boolean;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.opened:=false;
end;

const eofsign:char=#$1A;
begin
  with baza do begin
    if not(opened) then begin
      assign(datoteka,baza.dbfname);
      if not(fresetu(datoteka,1)) then begin we; exit end;
    end;
    if not(fseek(datoteka,4)) then begin we; exit end;
    if not(fblockread(datoteka,slog,4)) then begin we; exit end;
    inc (slog);
    if not(fseek(datoteka,4)) then begin we; exit end;
    if not(fblockwrite(datoteka,slog,4)) then begin we; exit end;
    if not(fseek(datoteka,filesize(datoteka)-1)) then begin we; exit end;
    if not(fblockwrite(datoteka,buffer,baza.reclen)) then begin we; exit end;
    if not(fblockwrite(datoteka,eofsign,1)) then begin we; exit end;
    if not(opened) then cc:=fclose(datoteka);
    reccount:=slog;
  end
end;

function Initdbf(var baza:dbf):Boolean;
var
  cc:Boolean;
procedure we;
begin
  cc:=fclose(baza.datoteka);
  baza.opened:=false;
end;

begin
  Initdbf:=false;
  With Baza do begin
    opened:=false;
    assign(datoteka,dbfname);
    if not(fresetu(datoteka,1)) then exit;
    recnum:=1;
    if not(fseek(datoteka,4)) then begin we; exit end;
    if not(fblockread(datoteka,reccount,4)) then begin we; exit end;
    if not(fblockread(datoteka,headlen,2)) then begin we; exit end;
    if not(fblockread(datoteka,reclen,2)) then begin we; exit end;
    cc:=fclose(datoteka);
  end;
  Initdbf:=true;
end;

Var
  Baza, Addition,specbase:Dbf;
Type Tlistboxrec= Record
                    Col: Pstringcollection;
                    Focused: Word;
                  End;

Procedure Commit;
Var St: String;
Begin
  St:= Baza.Dbfname;
End;

Function Upper(A:string):string;
Var I:Integer;
Begin
  for I:=1 to length(A) do A[i]:=upcase(A[i]);
  Upper:=A
End;

Function Rtrim (A: String): String;
Var I: Integer;
Begin
  If A= '' Then begin rtrim:=''; Exit End;
  I:= Length (A);
  While (A [I] = ' ') Do Dec (I);
  if i<1 then Rtrim:='' else Rtrim:= Copy (A, 1, I)
End;

Type Indexitem= Record
                  Position: Word;
                  Indvalue: String [sirindex];
                End;
  Pindexitem= ^Indexitem;

  Tindex= Object (TSortedCollection)
            Procedure Freeitem (Item: Pointer); Virtual;
            Function Compare (Key1, Key2: Pointer): Integer; Virtual;
          End;
  Pindex= ^Tindex;

Procedure Tindex.Freeitem (Item: Pointer);
Var A: Pindexitem;
Begin
  A:= Item;
  If Item <> Nil Then Freemem (A,AllocSirIndex);
End;

Function Tindex.Compare (Key1, Key2: Pointer): Integer;
Var  Ky1, Ky2:^Indexitem;
Begin
  Ky1:= Key1;
  Ky2:= Key2;
  If Upper(Ky1^.Indvalue)= Upper(Ky2^.Indvalue) Then Compare:= 0
  Else Begin
    If Upper(Ky1^.Indvalue) < Upper(Ky2^.Indvalue) Then Compare:= - 1
    Else Compare:= 1
  End;
End;
Var Ind: Pindex;

Function Indexstring: String;Forward;

Const
  Indsmin: Array[1..9] of Integer = (1, 03, 04, 04, 04, 04, 04, 03, 13);
  Indsmax: Array[1..9] of Integer = (1, 10, 30, 30, 30, 04, 04, 30, 13);
Var
  Indexsize: Array[1..9] of Integer;

Procedure Createindex;
Var Item:^Indexitem;
  I, Sirina: Integer;
  daljinar:pprocenter2;
  Datoteka:File;
  cc:Boolean;
  p:word;
Begin
  hourglascur;
  allocsirindex:=sirindex+3;

  for i:=1 to 9 do Indexsize[i]:=0;
  Indexsize[1]:=Indsmin[1];
  Sirina:=0;
  For I:=1 to Length(Indexedfields) do Case Indexedfields[i] of
    'S': if sirina+Indsmin[2]<sirindex then  begin indexsize[2]:=Indsmax[2]; sirina:=sirina+Indsmax[2]; End;
    'F': if sirina+Indsmin[3]<sirindex then begin indexsize[3]:=Indsmax[3]; sirina:=sirina+Indsmax[3]; End;
    'P': if sirina+Indsmin[4]<sirindex then begin indexsize[4]:=Indsmax[4]; sirina:=sirina+Indsmax[4]; End;
    'C': if sirina+Indsmin[5]<sirindex then begin indexsize[5]:=Indsmax[5]; sirina:=sirina+Indsmax[5]; End;
    'Y': if sirina+Indsmin[6]<sirindex then begin indexsize[6]:=Indsmax[6]; sirina:=sirina+Indsmax[6]; End;
    'M': if sirina+Indsmin[7]<sirindex then begin indexsize[7]:=Indsmax[7]; sirina:=sirina+Indsmax[7]; End;
    'L': if sirina+Indsmin[8]<sirindex then begin indexsize[8]:=Indsmax[8]; sirina:=sirina+Indsmax[8]; End;
    'T': if sirina+Indsmin[9]<sirindex then begin indexsize[9]:=Indsmax[9]; sirina:=sirina+Indsmax[9]; End;
  End;
  i:=1;
  while sirina>sirindex do begin
    if (Indexsize[i]>Indsmin[i]) and (IndexSize[i]<>0) then begin
      dec(Indexsize[i]);
      dec(sirina);
    end;
    inc(i);
    if i=10 then i:=1;
  end;

  Ind:= New (Pindex, Init (1000, 20) );
  Ind^.Duplicates:= True;
  new(daljinar,init(16,10,52,1,Baza.reccount));
  desktop^.insert(daljinar);
  opendfile(baza);
  allocsirindex:=sirina+3; {Integer+duzina}

  For I:= 1 To Baza.Reccount Do Begin
    if i mod 100 = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,SoftBuf);
    GetMem(Item,Allocsirindex);
    Item^.Indvalue:= Indexstring;
    Item^.Position:= Baza.recnum;
    Ind^.Insert (Item);
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  closedfile(baza);
End;

Procedure Makeemptyind;
Begin
  Dispose (Ind,Done);
  Ind:=New(PIndex,Init(1000,20));
  Ind^.Duplicates:=True;
End;
Procedure Reindex;
Begin
  hourglascur;
  Dispose (Ind, Done);
  Createindex
End;

Type
  Dictitem= Record
              Key: String [4];
              Replkey: String [4];
              pozicija:Integer;
              Dictvalue: String [35]
            End;
  Pdictitem= ^Dictitem;

Type Tdictionary= Object (TSortedCollection)
                    Bazafield: Integer;
                    Slovo: Char;
                    Function Compare (Key1, Key2: Pointer): Integer; Virtual;
                    Procedure Freeitem (Item: Pointer); Virtual;
                    Function Finditem (T: Integer; Item: String): Integer;
                    Function Keyval (Item: String; Size: Integer): String;
                  End;
  Pdictionary= ^Tdictionary;
Procedure Tdictionary.Freeitem (Item: Pointer);
Var A: Pdictitem;
Begin
  A:= Item;
  If Item <> Nil Then Dispose (A);
End;


Function Tdictionary.Keyval (Item: String; Size: Integer): String;
Var Bb: Pdictitem;
  I: Integer;
Function Resize (A: String): String;
Var N: Integer;
Begin
  If Length (A) >= Size Then A:= Copy (A, 1, Size)
  Else Begin
    While Length (A) < Size Do Begin
      A:= A+ ' ';
    End
  End;
  Resize:= A;
End;
Begin
  For I:= 0 To Count- 1 Do Begin
    Bb:= At (I);
    If Bb^.Key= Item Then Begin
      Keyval:= Resize (Bb^.Dictvalue); ;
      Exit
    End;
  End;
  Keyval:= Resize (chr(22));
End;

Function Tdictionary.Finditem (T: Integer; Item: String): Integer;
Var I: Word;
  Bb: Pdictitem;
Begin
  Case T Of
    1 : Begin
          For I:= 0 To Count- 1 Do Begin
            Bb:= At (I);
            If Bb^.Key= Item Then Begin
              Finditem:= I;
              Exit
            End;
          End;
          Finditem:= 0;
          Exit;
        End;
    2 : Begin
          For I:= 0 To Count- 1 Do Begin
            Bb:= At (I);
            If Bb^.Dictvalue= Item Then Begin
              Finditem:= I;
              Exit
            End;
          End;
          Finditem:= 0;
          Exit;
        End;

  End
End;

Function Tdictionary.Compare (Key1, Key2: Pointer): Integer;
Var  Ky1, Ky2:^Dictitem;
Begin
  Ky1:= Key1;
  Ky2:= Key2;
  If Upper(Ky1^.Dictvalue)= Upper(Ky2^.Dictvalue) Then Compare:= 0
  Else Begin
    If Upper(Ky1^.Dictvalue) < Upper(Ky2^.Dictvalue) Then Compare:= - 1
    Else Compare:= 1
  End;
End;

Type TFileList= Object (TSortedCollection)
                  Bazafield: Word;
                  Function Compare (Key1, Key2: Pointer): Integer; Virtual;
                  Function Finditem (Item: String): Integer; Virtual;
                  Procedure Freeitem (Item: Pointer);Virtual;

                End;
  Pfilelist= ^TFileList;

Procedure TFilelist.Freeitem (Item: Pointer);
Var A: PFileitem;
Begin
  A:= Item;
  If Item <> Nil Then Dispose (A);
End;


Function TFileList.Finditem (Item: String): Integer;
Var I: Word;
  Bb: Pfileitem;
Begin
  For I:= 0 To Count- 1 Do Begin
    Bb:= At (I);
    If Rtrim(Bb^.Filename)= Rtrim(Item) Then Begin
      Finditem:= I;
      Exit
    End;
  End;
  Finditem:= 0;
  Exit;
End;

Function TFileList.Compare (Key1, Key2: Pointer): Integer;
Var  Ky1, Ky2:^Fileitem;
Begin
  Ky1:= Key1;
  Ky2:= Key2;
  If Ky1^.Filename= Ky2^.Filename Then Compare:= 0
  Else Begin
    If Ky1^.Filename < Ky2^.Filename Then Compare:= - 1
    Else Compare:= 1
  End;
End;

Function Directory(BB:PFileItem):String;
var tmp:String;
begin
  fillchar(tmp,250,' ');
  tmp[0]:=#250;
  GetDBRec(Addition,bb^.Pozicija,tmp[1]);
  Directory:= Copy (tmp, 13, 36);
end;

Function Vollabel(BB:PFileItem):String;
var tmp:String;
begin
  fillchar(tmp,250,' ');
  tmp[0]:=#250;
  GetDBRec(Addition,bb^.Pozicija,tmp[1]);
  Vollabel:= Copy (Tmp, 50, 11);
end;

Type Tfilelistbox= Object (TListBox)
                     seekbuf:String;
                     Filelist: Pfilelist;
                     Constructor Init (Var Bounds: TRect; Anumcols: Word;
                       Ascrollbar: Pscrollbar; Var Infilelist: Pfilelist);
                     Function Gettext (Item: Integer; Maxlen: Integer): String; Virtual;
                     Procedure Setdata (Var Rec); Virtual;
                     Procedure Handleevent (Var Event: TEvent); Virtual;
                     Procedure Vezani(What:Integer);

                   End;
  Pfilelistbox= ^Tfilelistbox;

Constructor Tfilelistbox.Init (Var Bounds: TRect; Anumcols: Word;
  Ascrollbar: Pscrollbar; Var Infilelist: Pfilelist);
Begin
  TListBox.Init (Bounds, Anumcols, Ascrollbar);
  Filelist:= Infilelist;
  seekbuf:='';

End;

Procedure Tfilelistbox.Setdata (Var Rec);
Begin
End;

Function Tfilelistbox.Gettext (Item: Integer; Maxlen: Integer): String;
Var Bb: Pfileitem;
tmp:String;
Begin
  Bb:= Filelist^.At (Item);
  tmp:= copy(Bb^.Filename,1,8);
  if rtrim(copy(Bb^.Filename,9,2))<>'' then tmp:=tmp+'#'+copy(Bb^.Filename,9,2);
  gettext:=tmp;
End;
const cvinsert=1; cvreplace=2; cvdelete=3;

Procedure Tfilelistbox.Handleevent (Var Event: TEvent);
var n:Integer;
BB:PFileItem;
Nadjen:Boolean;
Begin
  TListBox.Handleevent (Event);
  If Event.What= evKeyDown Then begin
    If (Event.charcode>=#32) and (Event.charcode<=#127) then begin
      seekbuf:=seekbuf+upper(Event.charcode);
      n:=-1;
      nadjen:=false;
      repeat
        n:=n+1;
        BB:=FileLIst^.At(n);
        if upper(copy(bb^.filename,1,length(seekbuf)))=seekbuf
          then nadjen:=true
      until nadjen or (n=FileList^.Count-1);

      if nadjen then FocusItem(n) else seekbuf:='';
      ClearEvent(Event);
      exit;
    end
    else
    case Event.Keycode of
      kbRight:Vezani(cvreplace);
      kbIns:Vezani(cvInsert);
      kbDel: Vezani(cvDelete);
    end
  end
End;

Var Jezici, Proizvodjaci, Tipovi: Pdictionary;
  Tapes: Pfilelist;

Type TBrowseButton=Object(TButton)
  vollab,dir,name:PinputLine;
  categ:PRadioButtons;
  constructor Init(var Bounds: TRect; ATitle: TTitleStr;
     ACommand: Word; AFlags: Word;pvollab,pdir,pname:Pinputline;
     pCateg:PRadioButtons);

  procedure press; virtual;
end;
PBrowseButton=^TBrowseButton;
constructor TBrowseButton.Init(var Bounds: TRect; ATitle: TTitleStr;
     ACommand: Word; AFlags: Word;pvollab,pdir,pname:Pinputline;
     pCateg:PRadioButtons);
begin
  TButton.Init(Bounds,Atitle,ACommand, AFlags);
  vollab:=pvollab;
  dir:=pdir;
  name:=pname;
  categ:=pCateg;
end;
Procedure TBrowseButton.press;
var c:integer;
  dlg:PFiledialog;
  fname,d,n,e,vl:string;
  b:word;
begin
  dlg:=new(PFileDIalog,Init('*.*','Select file name','~N~ame',
     fdOKButton,100));
  c:=desktop^.execview(dlg);
  if c<>cmCancel then begin
    dlg^.getfilename(fname);
    fname:=fexpand(fname);
    fsplit(fname,d,n,e);
    dir^.setdata(d);
    name^.setdata(n);
    b:=3;
    if e='.TAP' then b:=0;
    if e='.Z80' then b:=1;
    if e='.MDR' then b:=2;
    if b=3 then dir^.setdata(fname);
    categ^.setdata(b);
    if d[1] in ['a','A','b','B'] then vl:='Diskette' else vl:='Disc';
    vollab^.setdata(vl);
  end;
  dispose(dlg,done);
end;

Procedure Tfilelistbox.Vezani(what:integer);
Const
  Trislova: Array [0..3] Of Char= ('T', 'S', 'M','O');
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  vollab,dir,name,ordid:PInputLine;
  categ:PRadioButtons;
  Vol1,Dir1,Name1,ordid1,NameOrd:String;
  Item:Pfileitem;
  radio,c,i,zapamti:Word;
  Daljinar:PProcenter2;
  Izabrani:Integer;
var
  DataRec : record
    Name : String[8]; {Inputline}
    ordid : String[2]; {Inputline}
    Categ : Word; {RadioButtons}
    dir : String[35]; {Inputline}
    vollab : String[11]; {Inputline}
    end;
  Bb, Zamijeni: Pfileitem;
  OldName:String;
  IndItem:^IndexItem;
procedure dodelete;
var i:integer;
begin
  Izabrani:=focused;
  If (Izabrani= 0) Then Exit;
  Zamijeni:= Filelist^.At (Izabrani);
  Zapamti:= Baza.recnum;
  Oldname:= Zamijeni^.Filename;
  Filelist^.Atdelete (Izabrani);
  opendfile(baza);
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  makeemptyind;
  Oldname:=Rtrim(Oldname);
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,softbuf);
    If Rtrim(BazaField (8)) = Oldname Then Begin
      BazaReplace (8, ' ');
      Putdbrec (Baza,I,softbuf);
    End;
    GetMem(IndItem,Allocsirindex);
    IndItem^.Indvalue:= Indexstring;
    IndItem^.Position:= Baza.recnum;
    Ind^.Insert (IndItem);
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  closedfile(baza);
  addreplace(1,' ');
  addbuf.deleted:=#$2a;
  Putdbrec(addition,Zamijeni^.pozicija,addbuf);
  Dispose (Zamijeni);
  Getdbrec (Baza,Zapamti,softbuf);
  Setrange (Range- 1);
  Draw;
End;

Begin
if what=cvdelete then begin
  dodelete; exit;
end;
R.Assign(10,2,70,20);
New(Dlg, Init(R, 'File name'));

R.Assign(14,2,24,3);
name := New(PInputLine, Init(R, 8));
Dlg^.Insert(name);

  R.Assign(2,2,12,3);
  Dlg^.Insert(New(PLabel, Init(R, '~F~ile name', name)));

R.Assign(26,2,30,3);
ordid := New(PInputLine, Init(R, 2));
Dlg^.Insert(ordid);

R.Assign(14,4,54,8);
Categ := New(PRadiobuttons, Init(R,
  NewSItem('~T~ape file',
  NewSItem('~S~napshot file',
  NewSItem('~M~icrodrive file',
  NewSItem('Full name in ~P~ath field',Nil))))));
Dlg^.Insert(Categ);
  R.Assign(2,4,12,5);
  Dlg^.Insert(New(PLabel, Init(R, 'File ~t~ype', Categ)));

R.Assign(8,9,43,10);
Dir := New(PInputLine, Init(R, 35));
Dlg^.Insert(Dir);
  R.Assign(2,9,7,10);
  Dlg^.Insert(New(PLabel, Init(R, '~P~ath', Dir)));

R.Assign(20,11,32,12);
Vollab := New(PInputLine, Init(R, 11));
Dlg^.Insert(Vollab);
  R.Assign(2,11,15,12);
  Dlg^.Insert(New(PLabel, Init(R, '~V~olume label', Vollab)));

R.Assign(13,13,28,15);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(31,13,43,15);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

R.Assign(46,2,58,4);
Control := New(PBrowseButton, Init(R, '~B~rowse', cmCancel, bfNormal,vollab,
dir,name,categ));
Dlg^.Insert(Control);

R.Assign(25,2,26,3);
Control := New(PStaticText, Init(R, '('));
Dlg^.Insert(Control);

R.Assign(30,2,31,3);
Control := New(PStaticText, Init(R, ')'));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
  Item:=Filelist^.At(Focused);
  with Item^ do begin
    Vol1:=Rtrim(Vollabel(Item));
    Dir1:=Rtrim(Directory(Item));
    Name1:=Rtrim(Copy(Filename,1,8));
    Ordid1:=Rtrim(Copy(Filename,9,2));
    if what=cvreplace then begin
      Name^.Setdata (Name1);
      OrdId^.SetData(Ordid1);
      Dir^.Setdata (Dir1);
      Vollab^.Setdata (Vol1);
      Radio:= 0;
      Case Tip Of
        'T': Radio:= 0;
        'S': Radio:= 1;
        'M': Radio:= 2;
        'O': Radio:= 3;
      End;
      Categ^.Setdata (Radio);
    end
     else Name^.SetData(seekbuf)
  End;


  C:= Desktop^.Execview (Dlg);
  hourglascur;
  Dlg^.Getdata (Datarec);
  Dispose(dlg,done);
  for i:=1 to length(Rtrim(Datarec.Name)) do
    if Datarec.Name[i] in
     [' ','.','*','?','%','\','|',':'] then begin
      Displayerror(^C'File name contains illegal characters.'+#13+
        ^C'It must be entered without path and extension.',0);
      exit
    end;
   NameOrd:=Rtrim(Datarec.Name);
   if rtrim(DataRec.Ordid)<>'' then begin
     while length(NameOrd)<8 do NameOrd:=NameOrd+' ';
       NameOrd:=NameOrd+Datarec.Ordid;
   end;
   NameOrd:=RTrim(NameOrd);
  if c=cmcancel then exit;
  Izabrani:=Focused;
  hourglascur;

   if what=cvInsert then Begin
     If (Rtrim (Datarec.Name) = '') Then Begin
       Focusitem (Izabrani); Exit;
     End;

     For I:= 0 To Filelist^.Count- 1 Do Begin
       Bb:= Filelist^.At (I);
       If Bb^.Filename = NameOrd Then Begin
         Focusitem (I);
         With Bb^ Do Begin
           Tip:= Trislova [Datarec.Categ];
           Filename:=NameOrd;
           Dir1:= Rtrim(Datarec.dir);
           Vol1:= Rtrim(Datarec.vollab);
         End;
         Fillchar(addbuf,sizeof(addbuf),' ');
         With bb^ Do Begin
           While Length (NameOrd) < 10 Do  NameOrd:= NameOrd+ ' ';
           While Length (Dir1) < 36 Do Dir1:= Dir1+ ' ';
           While Length (Vol1) < 11 Do Vol1:= Vol1+ ' ';
           AddReplace (1, Tip+ NameOrd+ Dir1+ '('+ Vol1+ ')');
         End;
         Putdbrec(addition,bb^.pozicija,addbuf);
         Draw;
         Exit;
       End;
     End;
     New (Bb);
     With Bb^ do Begin
       Tip:= Trislova [Datarec.Categ];
       Filename:= NameOrd;
       Dir1:=Rtrim( Datarec.dir);
       Vol1:= Rtrim(Datarec.vollab);
     End;
     Filelist^.Insert (BB);
     Fillchar(addbuf,sizeof(addbuf),' ');
     With bb^ Do Begin
       While Length (FileName) < 10 Do
         FileName:= FileName+ ' ';
       While Length (Dir1) < 36 Do
         Dir1:= Dir1+ ' ';
       While Length (Vol1) < 11 Do
         Vol1:= Vol1+ ' ';
       AddReplace (1, Tip+ FileName+ Dir1+ '('+ Vol1+ ')');
     End;
     Adddbrec(Addition,addbuf);
     bb^.pozicija:=addition.reccount;
     SetRange(Range+1);
     Focusitem (Filelist^.Indexof (Bb) );

     Draw;
   End;

   if what= cvReplace then  Begin
     If (Izabrani= 0) Or (Rtrim (Datarec.Name) = '') Then Exit;
     Zapamti:= Baza.recnum;
     Zamijeni:= Filelist^.At (Izabrani);
     Oldname:= Zamijeni^.Filename;
     Filelist^.Atdelete (Izabrani);
     New(bb);
     With Bb^ Do Begin
       Tip:= Trislova [Datarec.Categ];
       FileName:=NameOrd;
       Dir1:=Rtrim( DataRec.Dir);
       Vol1:= Rtrim(Datarec.vollab);
       Pozicija:=Zamijeni^.Pozicija;
     End;
     Filelist^.Insert(bb);
     new(daljinar,init(16,10,52,1,baza.reccount));
     desktop^.insert(daljinar);
     opendfile(baza);
     While Length (Oldname) < 10 Do
       Oldname:= Oldname+ ' ';
     makeemptyind;

     For I:= 1 To Baza.Reccount Do Begin
       if (i mod 100)=0 then  daljinar^.update(i,'');
       Getdbrec (Baza,I,softbuf);

       If softbuf.traka = Oldname Then Begin
         BazaReplace (8, NameOrd);
         Putdbrec (Baza,I,softbuf);
       End;
       GetMem(IndItem,Allocsirindex);
       IndItem^.Indvalue:= Indexstring;
       IndItem^.Position:= Baza.recnum;
       Ind^.Insert (IndItem);
     End;
     desktop^.delete(daljinar);
     dispose(daljinar,done);
     closedfile(baza);
     Focusitem (Filelist^.Indexof (Bb) );
     Getdbrec (Baza,Zapamti,softbuf);
     Fillchar(addbuf,sizeof(addbuf),' ');
     With bb^ Do Begin
       While Length (Filename) < 10 Do  Filename:= Filename+ ' ';
       While Length (Dir1) < 36 Do Dir1:= Dir1+ ' ';
       While Length (Vol1) < 11 Do Vol1:= Vol1+ ' ';
       AddReplace (1, Tip+ Filename+ Dir1+ '('+ Vol1+ ')');
     End;
     Putdbrec(addition,bb^.pozicija,addbuf);
     Draw;
   End;
  End;

Type Tdbflist= Object (TListViewer)
                  Saveditem: Word;
                  deleting:boolean;
                  Procedure Handleevent (Var Event: TEvent); Virtual;
                  Function Gettext (Item: Integer; Maxlen: Integer): String; Virtual;
                  Constructor Init (Var Bounds: TRect; Anumcols: Integer; Ahscrollbar, Avscrollbar: Pscrollbar);
                  Procedure Seekfocused;
                End;
  Pdbflist= ^Tdbflist;
Procedure Tdbflist.Handleevent (Var Event: TEvent);
Begin
  TListViewer.Handleevent (Event);

  If Event.What= evKeyDown Then Begin
    With Hscrollbar^ Do
      Case Event.Keycode Of
        kbLeft: If Value> Min Then Begin Setvalue (Value- 1); End;
        kbRight: If Value< Max Then Begin Setvalue (Value+ 1); End;
        kbctrlRight: if value<Max-20 then Setvalue(Value+20)
                      else Setvalue(Max);
        kbctrlLeft: if value>Min+20 then Setvalue(Value-20)
                      else Setvalue(Min);

      End
  End;

End;

Constructor Tdbflist.Init (Var Bounds: TRect; Anumcols: Integer;
  Ahscrollbar, Avscrollbar: Pscrollbar);
Begin
  Saveditem:= 1;
  deleting:=false;
  TListViewer.Init (Bounds, Anumcols,
    Ahscrollbar, Avscrollbar);
End;

Procedure Tdbflist.Seekfocused;
Var Dummy: String;
Begin
  Dummy:= Gettext (Focused, 2);
End;

Function Tdbflist.Gettext (Item: Integer; Maxlen: Integer): String;
Var Poz, I: Integer;
  Bb:^Indexitem;
  Mainst: String;
Begin
  Bb:= Ind^.At (Item);
  Getdbrec (Baza,Bb^.Position,softbuf);
    Mainst:= '';
    For I:= 1 To 2 Do Mainst:= Mainst+ BazaField (I) + '³';

    Mainst:= Mainst+ Proizvodjaci^.Keyval (BazaField (3), 30) + '³';
    Mainst:= Mainst+ Tipovi^.Keyval (BazaField (4), 30) + '³';
    For I:= 5 To 6 Do Mainst:= Mainst+ BazaField (I) + '³';
    Mainst:= Mainst+ Jezici^.Keyval (BazaField (7), 29) + '³';

    For I:= 8 To 12 Do Mainst:= Mainst+ BazaField (I) + '³';
    Mainst:= Mainst+ BazaField (13) + '³';
    Mainst:= Mainst+ BazaField (14) + '³';

  If Softbuf.Deleted=#$2A Then Mainst:= '  --- Record is marked for deletion  ---';
  Gettext:= Mainst;
End;



Type Tdictbox= Object (TListBox)
                 seekbuf:String;
                 Dictionary: Pdictionary;
                 Constructor Init (Var Bounds: TRect; Anumcols: Word;
                   Ascrollbar: Pscrollbar; Var Indictionary: Pdictionary);
                 Function Gettext (Item: Integer; Maxlen: Integer): String; Virtual;
                 Procedure Setdata (Var Rec); Virtual;
                 Procedure Getdata (Var Rec); Virtual;
                 Procedure Handleevent (Var Event: TEvent); Virtual;
                 Procedure InsertItem (Strvalue:String);
                 Procedure ReplaceItem(StrValue:String);
                 Procedure DeleteItem;
               End;
  Pdictbox= ^Tdictbox;

Constructor Tdictbox.Init (Var Bounds: TRect; Anumcols: Word;
  Ascrollbar: Pscrollbar; Var Indictionary: Pdictionary);
Begin
  TListBox.Init (Bounds, Anumcols, Ascrollbar);
  Seekbuf:='';
  Dictionary:= Indictionary
End;

Procedure TDictBox.InsertItem(StrValue:String);
var izabrani,i:Word;
Inicod:String[4];
bb:PDictItem;
Gotovo:Boolean;
begin
  Izabrani:=Focused;
  If Range= 0 Then Exit;
  If (Rtrim (Strvalue) = '') Then Begin
    Focusitem (Izabrani); Exit;
  End;
  Inicod:= Strvalue [1] + '000';
  For I:= 0 To Dictionary^.Count- 1 Do Begin
    Bb:= Dictionary^.At (I);
    If Rtrim (Bb^.Dictvalue) = Rtrim (Strvalue) Then Begin
      Focusitem (I);
      Draw;
    Exit;
  End;
  If (Strvalue [1] = Bb^.Key [1] ) And (Bb^.Key> Inicod) Then
    Inicod:= Bb^.Key
  End;
  Gotovo:= False;
  I:= 4;
  Repeat
    Inc (Inicod [I] );
    If Inicod [I] > '9' Then Inicod [I] := '0' Else Gotovo:= True;
    I:= I- 1;
  Until Gotovo;
  New (Bb);
  Bb^.Dictvalue:= Strvalue;
  Bb^.Key:= Inicod;
  Dictionary^.Insert (Bb);
  Setrange (Range+ 1);
  Focusitem (Dictionary^.Indexof (Bb) );

  if not(Initdbf (Addition)) then exit;
  Fillchar(Addbuf,sizeof(Addbuf),' ');
  AddReplace (1, Dictionary^.Slovo+ Inicod+ Strvalue);
  Adddbrec(Addition,addbuf);
  BB^.Pozicija:=addition.reccount;
End;

Procedure TDictBox.ReplaceItem(StrValue:String);
var izabrani,i,zapamti:Word;
Inicod,OldKey:String[4];
bb,zamijeni:PDictItem;
Gotovo:Boolean;
daljinar:PProcenter2;
IndItem:^IndexItem;
Begin
  Izabrani:=Focused;
  If Range= 0 Then Exit;
  hourglascur;
  If (Izabrani= 0) Or (Rtrim (Strvalue) = '') Then Exit;
  Zapamti:= Baza.recnum;
  Zamijeni:= Dictionary^.At (Izabrani);
  Oldkey:= Zamijeni^.Key;
  if StrValue[1]<>Zamijeni^.Dictvalue[1] then begin
  Dictionary^.Atdelete (Izabrani);
    Inicod:= Strvalue [1] + '000';
    For I:= 0 To Dictionary^.Count- 1 Do Begin
      Bb:= Dictionary^.At (I);
       If (Strvalue [1] = Bb^.Key [1] ) And (Bb^.Key> Inicod) Then
       Inicod:= Bb^.Key
    End;
    Gotovo:= False;
    I:= 4;
    Repeat
      Inc (Inicod [I] );
      If Inicod [I] > '9' Then Inicod [I] := '0' Else Gotovo:= True;
      I:= I- 1;
    Until Gotovo;
    Zamijeni^.Key:= Inicod;
    Zamijeni^.Dictvalue:= Strvalue;
    Dictionary^.Insert (Zamijeni);
    new(daljinar,init(16,10,52,1,baza.reccount));
    desktop^.insert(daljinar);
    opendfile(baza);
    makeemptyind;
    For I:= 1 To Baza.Reccount Do Begin
      if (i mod 100) = 0 then daljinar^.update(i,'');
      Getdbrec (Baza,I,softbuf);
      If BazaField (Dictionary^.Bazafield)=Oldkey Then Begin
        BazaReplace (Dictionary^.Bazafield, Inicod);
        Putdbrec (Baza,I,softbuf);
      End;
      GetMem(IndItem,Allocsirindex);
      IndItem^.Indvalue:= Indexstring;
      IndItem^.Position:= Baza.recnum;
      Ind^.Insert (IndItem);

    End;
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    closedfile(baza);
    Focusitem (Dictionary^.Indexof (Zamijeni) );
  End
  else Zamijeni^.DictValue:=StrValue;
  Getdbrec (Baza,Zapamti,softbuf);
  addreplace(1,Dictionary^.Slovo+ Zamijeni^.Key+ Zamijeni^.Dictvalue);
  Putdbrec(addition,Zamijeni^.pozicija,addbuf);
  Draw;
End;

Procedure TDictBox.DeleteItem;
var izabrani,i,zapamti:Word;
Inicod,OldKey:String[4];
bb,zamijeni:PDictItem;
Gotovo:Boolean;
daljinar:PProcenter2;
IndItem:^IndexItem;
Begin
  Izabrani:=Focused;
  If Range= 0 Then Exit;
  hourglascur;
  If (Izabrani= 0) Then Exit;
  Zamijeni:= Dictionary^.At (Izabrani);
  Zapamti:= Baza.recnum;
  Oldkey:= Zamijeni^.Key;
  Dictionary^.Atdelete (Izabrani);
  Inicod:= '    ';
  opendfile(baza);
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  Makeemptyind;
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,softbuf);
    If BazaField (Dictionary^.Bazafield) = Oldkey Then Begin
      BazaReplace (Dictionary^.Bazafield, Inicod);
      Putdbrec (Baza,I,softbuf);
    End;
    GetMem(IndItem,Allocsirindex);
    IndItem^.Indvalue:= Indexstring;
    IndItem^.Position:= Baza.recnum;
    Ind^.Insert (IndItem);

  End;
  closedfile(baza);
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  {    focusitem(0);}
  addreplace(1,' ');
  addbuf.deleted:=#$2a;
  Putdbrec(addition,Zamijeni^.pozicija,addbuf);

  Dispose (Zamijeni);
  Getdbrec (Baza,Zapamti,softbuf);
  Setrange (Range- 1);
  Draw;
 End;


Procedure Tdictbox.Getdata (Var Rec);
Var
  Value: Record
           Lb: Pointer;
           Position: Word;
         End;
Begin
  Value.Lb:= Dictionary;
  Value.Position:= Focused;
  Move (Value, Rec, SizeOf (Value) );
End;

Procedure Tdictbox.Setdata (Var Rec);
Begin
End;

Function Tdictbox.Gettext (Item: Integer; Maxlen: Integer): String;
Var Bb: Pdictitem;
Begin
  Bb:= Dictionary^.At (Item);
  Gettext:= Bb^.Dictvalue;
End;

Procedure Tdictbox.Handleevent (Var Event: TEvent);
var n:integer;
nadjen:Boolean;
BB:PDictItem;
Edline:PInputLine;
R:TRect;
EventEd:Tevent;
unesen:String;
Begin
  If Event.What= evKeyDown Then begin
    If (Event.charcode>=#32) and (Event.charcode<=#127) then begin
      seekbuf:=seekbuf+upper(Event.charcode);
      n:=-1;
      nadjen:=false;
      repeat
        n:=n+1;
        BB:=Dictionary^.At(n);
        if upper(copy(bb^.Dictvalue,1,length(seekbuf)))=seekbuf
          then nadjen:=true
      until nadjen or (n=Dictionary^.Count-1);

      if nadjen then FocusItem(n) else seekbuf:='';
      ClearEvent(Event);
      exit;
    end
    else begin
      case Event.Keycode of
      kbIns: begin
        GetBounds(R);
        R.B.Y:=R.A.Y+1;
        Edline:=new(PInputLine,Init(R,30));
        Owner^.Insert(Edline);
        Edline^.Setdata(SeekBuf);
        Edline^.Select;
        repeat
          Edline^.GetEvent(EventEd);
          Edline^.HandleEvent(EventEd);
        until ((EventEd.What=evKeyDown) and (
        (EventEd.charcode=#27) or (EventEd.Charcode=#13)
        or (EventEd.KeyCode=kbTab) or (EventEd.KeyCode=kbShiftTab))
        );
        if EventEd.charcode<>#27 then begin
          Edline^.getdata(unesen);
          InsertItem(Unesen);
        end;
        dispose(Edline,done);
        Select;
      end;
      kbDel: DeleteItem;
      kbRight: begin
        GetBounds(R);
        R.A.Y:=R.A.Y+Focused-TopItem;
        R.B.Y:=R.A.Y+1;
        Edline:=new(PInputLine,Init(R,30));
        Owner^.Insert(Edline);
        bb:=Dictionary^.At(Focused);
        unesen:=RTrim(bb^.DictValue);
        Edline^.Setdata(unesen);
        Edline^.Select;
        repeat
          Edline^.GetEvent(EventEd);
          Edline^.HandleEvent(EventEd);
        until ((EventEd.What=evKeyDown) and (
        (EventEd.charcode=#27) or (EventEd.Charcode=#13)
        or (EventEd.KeyCode=kbTab) or (EventEd.KeyCode=kbShiftTab))
        );
        Edline^.GetData(unesen);
        dispose(Edline,done);
        if EventEd.charcode<>#27 then begin
          ReplaceItem(Unesen);
        end;
        Select;
      end;
      end;

      seekbuf:='';
    end;
  end;
  TListBox.Handleevent (Event);

End;

Type Tproblems= Object (TCheckBoxes)
                  Fvalue: Word;
                  Procedure Setdata (Var Rec); Virtual;
                  Procedure Getdata (Var Rec); Virtual;
                  Procedure Press (Item: Integer); Virtual;
                End;

  Pproblems= ^Tproblems;
Procedure Tproblems.Setdata (Var Rec);
Begin
  Fvalue:= Word (Rec);
  If Fvalue<> 0 Then Value:= 1;
  Drawview
End;

Procedure Tproblems.Getdata (Var Rec);
Begin
  Move (Fvalue, Rec, 2);
End;

Procedure Tproblems.Press (Item: Integer);
Var
  Dlg : Pdialog;
  R : TRect;
  Control, Hscroll : Pview;
  Butt: Pcheckboxes;
  C: Word;
Begin
  TCheckBoxes.Press (Item);
  If 1= 1 Then Begin
    R.Assign (3, 2, 77, 16);
    New (Dlg, Init (R, 'Problem explain') );
    Dlg^.helpctx:=hcexpproblem;
    R.Assign (1, 3, 73, 10);
    Butt := New (Pcheckboxes, Init (R,
      NewSItem ('Cannot work theoreticaly',
      NewSItem ('Problems not listed there',
      NewSItem ('Exits with message',
      NewSItem ('Interface emul. problems',
      NewSItem ('Tape emulation problems',
      NewSItem ('Border flickers too much',
      NewSItem ('If hangs, can not help F9, F10',
      NewSItem ('Hangs up or resets even with /I',
      NewSItem ('Hangs up or resets without /I',
      NewSItem ('Sound unrecognizable',
      NewSItem ('Bad Herc/CGA/EGAmono shades',
      NewSItem ('Too slow on EGA/VGA',
      NewSItem ('Too slow on Herc/CGA/EGAmono',

      NewSItem ('Flickering',   Nil) ) ) ) ) ) ) ) ) ) ) ) ) ) ) );
    Dlg^.Insert (Butt);
    Butt^.Setdata (Fvalue);

    R.Assign (24, 2, 33, 3);
    Dlg^.Insert (New (Plabel, Init (R, '~P~roblems', Control) ) );

    R.Assign (16, 11, 28, 13);
    Control := New (Pbutton, Init (R, '~O~K', Cmok, bfNormal) );
    Dlg^.Insert (Control);

    R.Assign (42, 11, 54, 13);
    Control := New (Pbutton, Init (R, '~C~ancel', Cmcancel, bfNormal) );
    Dlg^.Insert (Control);

    Dlg^.Selectnext (False);
    C:= Desktop^.Execview (Dlg);
    hourglascur;
    Fvalue:= Butt^.Value;
    Dispose (Dlg, Done);
    If Fvalue= 0 Then Value:= 0 Else Value:= 1;
    Drawview;
  End
End;


Procedure Getedit (Novi: Boolean; var Inditem: integer);
Var
  C, Chk: Word;
  Dlg : Pdialog;
  R : TRect;
  Ll: Pdictbox;
  Lt: Pfilelistbox;
  Item:^Indexitem;
  Dbt: File;
  Pozicija: LongInt;
  Searchpos, I: Integer;
  Datoteka: File;
  St: String;
  Result1: String [3];
  Skrolnica: Pscrollbar;
  Indicator: Pindicator;
  Control, Hscroll : Pview;
  Stariindex: String;
  Pozicijamemo: LongInt;
Var
  Datarec : Record
              dShort : String [10]; {Inputline}
              dFull : String [30]; {Inputline}
              dCategory : Tlistboxrec;  {Listbox}
              dProducer : Tlistboxrec;  {Listbox}
              dComputer : Word; {RadioButtons}
              dLanguage : Tlistboxrec;  {Listbox}
              dTapes : Tlistboxrec;  {Listbox}
              dPosition : LongInt; {Inputlong}
              dYear : String [4]; {Inputline}
              dOpisduz : Word;       {TMemo text length}
              dOpis : Array [1..512] Of Char;  {TMemo text}

              dSortpri : Word; {Checkboxes}
              dProblems : Word; {Checkboxes}
              dParams : String [50]; {Inputline}
            End;
  Listboxrec: Tlistboxrec;
  cc:Boolean;
procedure we;
begin
  cc:=fclose(dbt);
end;

Begin
  If Novi Then Begin
    Fillchar(softbuf,sizeof(softbuf),' ');
    softbuf.pozicija[3]:='0';
    softbuf.problemi[5]:='0';

  end;
  With Datarec Do Begin
    Val (Rtrim (BazaField (13) ), Pozicija, C);
    For C:= 1 To 512 Do dOpis [C] := ' ';
    If Pozicija<> 0 Then Begin
      Assign (Dbt, dbtname);
      if not(fResetu (Dbt, 1)) then exit;
      if not(fSeek (Dbt, Pozicija* 512)) then begin we; exit end;
      if not(fBlockRead2 (Dbt, dOpis, 512,DummyBytes)) then begin we; exit end;
      cc:=fClose (Dbt);
      dOpisduz:= 1;
      While (dOpisduz< 512) And (dOpis [dOpisduz] <> #$1A) Do Inc (dOpisduz);
      Dec (dOpisduz);
    End
    Else dOpisduz:= 0;

    dShort := Rtrim (BazaField (1) ); {Inputline}
    dFull := Rtrim (BazaField (2) ); {Inputline}
    dComputer:= 0;
    If  BazaField (6) =  '16  ' Then dComputer:= 0;
    If  BazaField (6) =  '48  ' Then dComputer:= 1;
    If  BazaField (6) =  '2068' Then dComputer:= 2;
    If  BazaField (6) =  '128 ' Then dComputer:= 3;
    If  BazaField (6) =  '+2  ' Then dComputer:= 4;
    If  BazaField (6) =  '+3  ' Then dComputer:= 5;
    If  BazaField (6) =  'SAM ' Then dComputer:= 6;
    Val (Rtrim (BazaField (9) ), dPosition, C);
    dYear:= Rtrim (BazaField (5) ); {Inputline}
    Val (Rtrim (BazaField (11) ), Chk, C);
    dProblems:= Chk;
    If BazaField (14) = '*' Then dSortpri := 1 Else dSortpri:= 0;
    dParams := Rtrim (BazaField (12) ); {Inputline}
  End;
  if Novi then Datarec.Dparams:=comopts;
  R.Assign (1, 1, 77, 22);
  New (Dlg, Init (R, 'Database entry') );
  Dlg^.helpctx:=hceditrecord;
  R.Assign (13, 2, 25, 3);
  Control := New (PKeyInputline, Init (R, 10) );
  Dlg^.Insert (Control);

  R.Assign (2, 2, 13, 3);
  Dlg^.Insert (New (Plabel, Init (R, 'S~h~ort name', Control) ) );

  R.Assign (35, 2, 65, 3);
  Control := New (PKeyInputline, Init (R, 30) );
  Dlg^.Insert (Control);

  R.Assign (25, 2, 35, 3);
  Dlg^.Insert (New (Plabel, Init (R, '~F~ull name', Control) ) );

  R.Assign (32, 5, 33, 9);
  Skrolnica := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Skrolnica);

  R.Assign (2, 5, 32, 9);
  Ll := New (Pdictbox, Init (R, 1, Skrolnica, Tipovi) );
  Dlg^.Insert (Ll);
  Ll^.Setrange (Tipovi^.Count);
  Ll^.Focusitem (Tipovi^.Finditem (1, BazaField (4) ) );

  R.Assign (8, 4, 17, 5);
  Dlg^.Insert (New (Plabel, Init (R, 'Cate~g~ory', Ll) ) );

  R.Assign (64, 5, 65, 9);
  Control := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Control);


  R.Assign (34, 5, 64, 9);
  Ll := New (Pdictbox, Init (R, 1, Pscrollbar (Control), Proizvodjaci) );
  Dlg^.Insert (Ll);
  Ll^.Setrange (Proizvodjaci^.Count);
  Ll^.Focusitem (Proizvodjaci^.Finditem (1, BazaField (3) ) );

  R.Assign (34, 4, 65, 5);
  Dlg^.Insert (New (Plabel, Init (R, 'Software producer or ~d~eveloper', Ll) ) );

  R.Assign (66, 2, 75, 9);
  Control := New (Pradiobuttons, Init (R,
    NewSItem ('16',
    NewSItem ('48',
    NewSItem ('2068',
    NewSItem ('128',
    NewSItem ('+2',
    NewSItem ('+3',
    NewSItem ('SAM', Nil) ) ) ) ) ) ) ) );
  Dlg^.Insert (Control);

  R.Assign (66, 1, 74, 2);
  Dlg^.Insert (New (Plabel, Init (R, '~M~achine', Control) ) );

  R.Assign (32, 11, 33, 15);
  Control := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Control);

  R.Assign (2, 11, 32, 15);
  Ll := New (Pdictbox, Init (R, 1, Pscrollbar (Control), Jezici) );
  Dlg^.Insert (Ll);
  Ll^.Setrange (Jezici^.Count);
  Ll^.Focusitem (Jezici^.Finditem (1, BazaField (7) ) );

  R.Assign (8, 10, 25, 11);
  Dlg^.Insert (New (Plabel, Init (R, 'Message ~l~anguage', Ll) ) );

  R.Assign (44, 11, 45, 15);
  Control := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Control);

  R.Assign (34, 11, 46, 15);
  Lt := New (Pfilelistbox, Init (R, 1, Pscrollbar (Control), Tapes) );
  Dlg^.Insert (Lt);
  Lt^.Setrange (Tapes^.Count);
  Lt^.Focusitem (Tapes^.Finditem (Rtrim(BazaField (8)) ) );

  R.Assign (34, 10, 44, 11);
  Dlg^.Insert (New (Plabel, Init (R, '~F~ile name', Lt) ) );

  R.Assign (37, 16, 42, 17);
  Control := New (Pnuminputline, Init (R, 3, 0, 999) );
  Dlg^.Insert (Control);



  R.Assign (35, 15, 44, 16);
  Dlg^.Insert (New (Plabel, Init (R, 'Position', Control) ) );

  R.Assign (64, 10, 70, 11);
  Control := New (Pinputline, Init (R, 4) );
  Dlg^.Insert (Control);

  R.Assign (58, 10, 64, 11);
  Dlg^.Insert (New (Plabel, Init (R, '~Y~ear', Control) ) );

  R.Assign (73, 12, 74, 17);
  Control := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Control);

  R.Assign (46, 16, 73, 17);
  Hscroll := New (Pscrollbar, Init (R) );
  Dlg^.Insert (Hscroll);

  R.Assign (49, 12, 73, 16);
  Indicator:= New (Pindicator, Init (R) );
  Control := New (Pmemo, Init (R, Pscrollbar (Hscroll), Pscrollbar (Control),
    Indicator, 512) );
  Dlg^.Insert (Control);

  R.Assign (46, 11, 54, 12);
  Dlg^.Insert (New (Plabel, Init (R, '~R~emark', Control) ) );


  R.Assign (2, 16, 20, 17);
  Control := New (Pcheckboxes, Init (R,
    NewSItem ('~S~ort marker', Nil) ) );
  Dlg^.Insert (Control);

  R.Assign (21, 16, 34, 17);
  Control := New (Pproblems, Init (R,
    NewSItem ('~P~roblems', Nil) ) );
  Dlg^.Insert (Control);

  R.Assign (15, 18, 45, 19);
  Control := New (Pinputline, Init (R, 50) );
  Dlg^.Insert (Control);

  R.Assign (2, 18, 15, 19);
  Dlg^.Insert (New (Plabel, Init (R, '~C~ommand line', Control) ) );

  R.Assign (50, 18, 60, 20);
  Control := New (Pbutton, Init (R, '~O~K', Cmok, bfNormal) );
  Dlg^.Insert (Control);

  R.Assign (64, 18, 74, 20);
  Control := New (Pbutton, Init (R, 'Cancel', Cmcancel, bfNormal) );
  Dlg^.Insert (Control);

  Dlg^.Selectnext (False);
  Dlg^.Setdata (Datarec);

  C:= Desktop^.Execview (Dlg);
  hourglascur;

  If C= Cmok Then Begin
    Dlg^.Getdata (Datarec);
    If Not (Novi) Then Begin
      Item:= Ind^.At (Inditem);
      Ind^.Atdelete (Inditem);
    End
    Else Begin
        GetMem(Item,Allocsirindex);
    End;
    With Datarec Do Begin
      dShort:=Upper(dShort);
      For i:=1 to Length(dShort) do
        if (not(dShort[i] in ['A'..'Z','0'..'9'])) then dShort[i]:='-';
      BazaReplace (1, dShort);
      BazaReplace (2, dFull);
      BazaReplace (3, Pdictitem (Proizvodjaci^.At (dProducer.Focused) )^.Key);
      BazaReplace (4, Pdictitem (Tipovi^.At (dCategory.Focused) )^.Key);
      BazaReplace (5, dYear);
      Case dComputer Of
        0 : BazaReplace (6, '16  ');
        1 : BazaReplace (6, '48  ');
        2 : BazaReplace (6, '2068');
        3 : BazaReplace (6, '128 ');
        4 : BazaReplace (6, '+2  ');
        5 : BazaReplace (6, '+3  ');
        6 : BazaReplace (6, 'SAM ');
      End;
      BazaReplace (7, Pdictitem (Jezici^.At (dLanguage.Focused) )^.Key);
      BazaReplace (8, Pfileitem (Tapes^.At (dTapes.Focused) )^.Filename);
      Str(dPosition:3,st);
      BazaReplace (9, st );
      Str(dProblems:5,st);
      BazaReplace (11, st );
      BazaReplace (12, dParams);
      if dOpisduz>=512 then dOpisduz:=511;
      if dOpisduz>0 then begin

        dOpis [dOpisduz+1] := #$1A;
        Assign (Dbt, dbtname);

        if not(fResetu(Dbt, 1)) then exit;
        If Pozicija= 0 Then Begin
          if not(fBlockRead (Dbt, Pozicija, 4)) then begin we;exit end;
          Inc (Pozicija);
          if not(fSeek (Dbt, 0)) then begin we; exit end;
          if not(fBlockWrite (Dbt, Pozicija, 4)) then begin we;exit end;
          str(pozicija:10,st);
          BazaReplace (13, st);
        End;
        if Pozicija*512<=Filesize(dbt) then begin
          if not(fSeek (Dbt, Pozicija* 512)) then begin we;exit end;
          if not(fBlockWrite (Dbt, dOpis, 512)) then begin we; exit end;
        end
        else begin
          starikraj:=Filesize(dbt);
          prazna:=' ';
          for brojac:=starikraj to Pozicija*512 do begin
            if not(fBlockWrite(Dbt,prazna,1)) then begin we; exit end
          end;
          if not(fSeek(Dbt,Pozicija*512)) then begin we; exit end;
          if not(fBlockWrite(Dbt,dOpis,512)) then begin we; exit end;
        end;
        cc:=FClose (Dbt)
      end;
      If dSortpri<> 0 Then BazaReplace (14, '*') Else BazaReplace (14, ' ')
    End;
    Item^.Indvalue:= Indexstring;
    Item^.Position:= Baza.recnum;
    if novi then Item^.Position:=Baza.reccount+1;
    Ind^.Insert (Item);
    Inditem:=Ind^.Indexof(Item);
    If Not (Novi) Then Putdbrec (Baza,Baza.recnum,softbuf)
      Else Adddbrec(baza,softbuf);
  End;
  Dispose (Dlg, Done);
  Dispose(Indicator,done);
  Commit;

End;

Type Texecedit= Object (TButton)
                   Ll: Pdbflist;
                   Constructor Init (Var Bounds: TRect; Atitle: TTitleStr;
                     Acommand: Word; Aflags: Word; Lister: Pdbflist );

                   Procedure Press; Virtual;
                 End;
  Pexecedit= ^Texecedit;

Constructor Texecedit.Init (Var Bounds: TRect; Atitle: TTitleStr;
  Acommand: Word; Aflags: Word; Lister: Pdbflist );
Begin
  TButton.Init (Bounds, Atitle, Acommand, Aflags);
  Ll:= Lister
End;
Procedure Texecedit.Press;
Begin
  If Ll^.Range= 0 Then Exit;
  Ll^.Seekfocused;
  Getedit (False, Ll^.Focused);
  ll^.Focusitem(Ll^.Focused);
  Ll^.Draw;
End;

Type Texecappend        = Object (Texecedit)
                    Procedure Press; Virtual;
                  End;
  Pexecappend= ^Texecappend;

Procedure Texecappend.Press;
Begin

  Getedit (True, Ll^.Focused);
  ll^.Focusitem(Ll^.Focused);
  Ll^.Setrange (Baza.reccount);
  Ll^.Draw;
End;

function findmark(mode:integer):word;forward;

Type Texecseek        = Object (Texecedit)
                    Procedure Press; Virtual;
                  End;
  Pexecseek= ^Texecseek;

Procedure Texecseek.Press;
var c:word;
Begin
  c:=findmark(2);
  ll^.Focusitem(c);
  Ll^.Draw;
End;

Type Texecdelete= Object (Texecedit)
                    Procedure Press; Virtual;
                  End;
  Pexecdelete= ^Texecdelete;

Procedure Texecdelete.Press;
Var Bb:^Indexitem;
Begin
  If Ll^.Range= 0 Then Exit;
  ll^.deleting:=true;
  Bb:= Ind^.At (Ll^.Focused);
{
  Getdbrec (Baza,Bb^.Position,softbuf);
  If Not (Softbuf.Deleted=#$2A) Then Softbuf.Deleted:=#$2A
    Else Softbuf.Deleted:=#$20;
  Putdbrec (Baza,Bb^.Position,softbuf);
}
  deletedbrec(Baza,BB^.position);
  reindex;
  ll^.setrange(ll^.range-1);
  Ll^.Draw;
End;

Procedure DBDisplay;
Const Cmedit= 2101;
  Cmappend= 2102;
  Cmdelete= 2103;
  Cmfinish= 2104;
  CmSeek=2105;
Var A: Pdialog;
  R: TRect;
  Ll: Pdbflist;
  Hscr, Vscr: Pscrollbar;
  C: Word;
  Bruce: Pointer;
  deleting:Boolean;

Begin

  R.Assign (5, 2, 70, 21);
  A:= New (Pdialog, Init (R, 'Database file viewer') );
  A^.helpctx:=hceditbase;
  R.Assign (62, 1, 63, 13);
  Vscr:= New (Pscrollbar, Init (R) );
  A^.Insert (Vscr);
  R.Assign (1, 13, 62, 14);
  Hscr:= New (Pscrollbar, Init (R) );
  A^.Insert (Hscr);
  Hscr^.Setrange (1, 100);
  R.Assign (1, 1, 62, 13);
  Ll:= New (Pdbflist, Init (R, 1, Hscr, Vscr) );
  A^.Insert (Ll);
  Ll^.Setrange (Baza.reccount);
  R.Assign (2, 15, 12, 17);
  Bruce:= New (Pexecedit, Init (R, '~E~dit', Cmedit, 0, Ll) );
  A^.Insert (Bruce);

  R.Assign (12, 15, 22, 17);
  Bruce:= New (Pexecappend, Init (R, '~A~ppend', Cmappend, 0, Ll) );
  A^.Insert (Bruce);
  R.Assign (22, 15, 32, 17);
  Bruce:= New (Pexecdelete, Init (R, '~D~elete', Cmdelete, 0, Ll) );
  A^.Insert (Bruce);
  R.Assign (32, 15, 42, 17);
  Bruce:= New (Pexecseek, Init (R, 'See~k~', CmSeek, 0,ll) );
  A^.Insert (Bruce);
  R.Assign (42, 15, 52, 17);
  Bruce:= New (Pbutton, Init (R, '~C~lose', CmCancel, 0) );
  A^.Insert (Bruce);

  A^.Selectnext (False);

  C:= Desktop^.Execview (A);
  hourglascur;
  deleting:=ll^.deleting;
  If Ll^.Range<> 0 Then Ll^.Seekfocused;
  Dispose (A, Done);
  if deleting then begin

  end;
End;

function dbRun(var progelem:exrecdata):integer;
Const CmRun= cmYes;
  CmSelect= cmOk;
Var A: Pdialog;
  R: TRect;
  Ll: Pdbflist;
  Hscr, Vscr: Pscrollbar;
  C: Word;
  Bruce: Pointer;
  deleting:Boolean;
  BB:^Indexitem;
  fip:pfileitem;
Begin
  if Baza.reccount=0 then Begin
    Displayerror('No data records',0);
    dbrun:=0;
    exit
  end;

  R.Assign (5, 2, 70, 21);
  A:= New (Pdialog, Init (R, 'Run from database') );
  A^.helpctx:=hcdbrun;
  R.Assign (62, 1, 63, 13);
  Vscr:= New (Pscrollbar, Init (R) );
  A^.Insert (Vscr);
  R.Assign (1, 13, 62, 14);
  Hscr:= New (Pscrollbar, Init (R) );
  A^.Insert (Hscr);
  Hscr^.Setrange (1, 100);
  R.Assign (1, 1, 62, 13);
  Ll:= New (Pdbflist, Init (R, 1, Hscr, Vscr) );
  A^.Insert (Ll);
  Ll^.Setrange (Baza.reccount);
  R.Assign (2, 15, 12, 17);
  Bruce:= New (PButton, Init (R, '~R~un', CmRun,0 ));
  A^.Insert (Bruce);

  R.Assign (12, 15, 22, 17);
  Bruce:= New (PButton, Init (R, '~S~elect', CmSelect,0) );
  A^.Insert (Bruce);
  R.Assign (22, 15, 32, 17);
  Bruce:= New (PButton, Init (R, '~C~ancel', CmCancel,0) );
  A^.Insert (Bruce);

  A^.Selectnext (False);

  C:= Desktop^.Execview (A);
  hourglascur;
  if (c=cmrun) or (c=cmselect) then begin
    Bb:= Ind^.At (Ll^.Focused);
    Getdbrec (Baza,Bb^.Position,softbuf);
    fip:=tapes^.At(tapes^.finditem(Rtrim(bazafield(8))));
       {tip,filename,directory,vollabel}
    progelem.kompjuter:=bazafield(6);
    progelem.fi:=fip^;
    progelem.pozicija:=bazafield(9);
    progelem.simulopc:=bazafield(12);
    progelem.direct:=Directory(fip);
    progelem.vollab:=Vollabel(fip);

  end;
  If Ll^.Range<> 0 Then Ll^.Seekfocused;
  Dispose (A, Done);
  dbrun:=0;
  if c=cmrun then dbrun:=1;
  if c=cmselect then dbrun:=2;
  if progelem.fi.tip='O' then dbrun:=3;
End;

type ASarr = Array [0..9] of Integer;

Function Reportline (Item: Integer; Actualsize:ASArr):String;
Var Poz, I: Integer;
  Bb:^Indexitem;
  Mainst: String;
  Problems, Linija: Word;
  izlaz:Boolean;
  const marksimbol:string[2]='* ';
Begin
    Bb:= Ind^.At (Item);
    Getdbrec (Baza,Bb^.Position,softbuf);
    Mainst:= '';
    if actualsize[0]<>0 then mainst:=mainst+copy(Bazafield(1),1,actualsize[0])+' ';
    if actualsize[1]<>0 then mainst:=mainst+copy(Bazafield(2),1,actualsize[1])+' ';
    if actualsize[2]<>0 then mainst:=mainst+
      proizvodjaci^.keyval(Bazafield(3),actualsize[2])+' ';
    if actualsize[3]<>0 then mainst:=mainst+
      tipovi^.keyval(Bazafield(4),actualsize[3])+' ';
    if actualsize[4]<>0 then mainst:=mainst+copy(Bazafield(5),1,actualsize[4])+' ';
    if actualsize[5]<>0 then mainst:=mainst+copy(Bazafield(6),1,actualsize[5])+' ';
    if actualsize[6]<>0 then mainst:=mainst+
      jezici^.keyval(Bazafield(7),actualsize[6])+' ';
    if actualsize[7]<>0 then mainst:=mainst+copy(Bazafield(8),1,actualsize[7])+' ';
    if actualsize[8]<>0 then mainst:=mainst+copy(Bazafield(9),1,actualsize[8])+' ';
    Val (Rtrim (Bazafield (11) ), Problems, Poz);
    if actualsize[9]<>0 then for i:=0 to 13 do begin
      if (problems and (8192 shr i)) <> 0 then
        mainst:=mainst+copy('FSVHNIRLBTAMO!',I+1,1)
       Else mainst:=mainst+'.';
    End;
  If Softbuf.Deleted=#$2A Then
    Mainst:= '  --- Record is marked for deletion  ---';
  ReportLine:= Mainst;
End;


type
  PInterior = ^TInterior;
  TInterior = object(TScroller)
    ukupno:word;
    actualsize:Asarr;
    firstmarked:integer;
    constructor Init(var Bounds: TRect; AHScrollBar,
      AVScrollBar: PScrollBar; pukupno:integer);
    procedure Draw; virtual;
  end;

  PPrintWindow = ^TPrintWindow;
  TPrintWindow = object(TDialog)
    ukupno:word;
    tapename:string;
    constructor Init(Bounds: TRect; WinTitle: String; WindowNo: Word;
       replinija:integer;pfirstmarked:word;var pactualsize:asarr;
       linija:integer;sirina:integer);
  end;

constructor TInterior.Init(var Bounds: TRect; AHScrollBar,
  AVScrollBar: PScrollBar;pukupno:integer);
begin
  TScroller.Init(Bounds, AHScrollBar, AVScrollBar);
  GrowMode := gfGrowHiX + gfGrowHiY;
  Options := Options or ofFramed;
  ukupno:=pukupno;
end;

procedure TInterior.Draw;
var
  Color: Byte;
  I, Y, strana,linustrani,linija,slog: Integer;
  B: TDrawBuffer;
  a,st:String;
begin
  TScroller.Draw;
  Color := GetColor(1);
  for Y := 0 to Size.Y - 1 do
  begin
    linija:=VSCrollBar^.Value+y;
    Strana:=Linija div printpars.replen;
    Linustrani:=Linija mod printpars.replen;
    str(strana+1:4,st);
    if Linustrani=0 then a:='   Page '+st
      else begin
        if Linustrani>=printpars.Printable then a:=''
          else begin
            slog:=(printpars.Printable-1)*Strana+Linustrani-1;
            if slog>=ukupno then a:=''
              else begin
                if printpars.which=1 then slog:=slog+firstmarked;
                a:=reportline(slog,actualsize);
              end
          end
      end;
    writestr(1,y,copy(a,delta.x+1,size.x),1);
  end;
end;

constructor TPrintWindow.Init(Bounds: TRect; WinTitle: String;
  WindowNo: Word;replinija:integer;pfirstmarked:word;var pactualsize:asarr;
  linija:integer;sirina:integer);
var
  HScrollBar, VScrollBar: PScrollBar;
  Interior: PInterior;
  R: TRect;
  maksimum:integer;
  S: string[3];
begin
  Str(WindowNo, S);
  TWindow.Init(Bounds, WinTitle , windowNo);
  flags:=15;
  ukupno:=replinija;
  VScrollBar := StandardScrollBar(sbVertical + sbHandleKeyboard);
  HScrollBar := StandardScrollBar(sbHorizontal + sbHandleKeyboard);
  GetExtent(Bounds);
  Bounds.Grow(-1,-1);
  Interior := New(PInterior, Init(Bounds, HScrollBar, VScrollBar,
    ukupno));
  Insert(Interior);
  Interior^.firstmarked:=pfirstmarked;
  Interior^.actualsize:=pactualsize;
  Interior^.Setlimit(sirina,
    printpars.Replen*(1+ukupno div printpars.printable)-1);
  helpctx:=hcscrreport;

end;

Procedure DBReport;
Type Drec= Record
             A: String [10];
             Lim: Word;
           End;
Const  Printers: Array [0..9 ] Of Drec= ( (A:''; Lim: 80),
     (A:''; Lim: 132),
     (A: Chr (27) + 'P'+ Chr (18); Lim: 80),
     (A: Chr (27) + 'P'+ Chr (15); Lim: 132),
     (A: Chr (27) + 'M'+ Chr (15); Lim: 160),
     (A: Chr (27) + 'P'+ Chr (18); Lim: 132),
     (A: Chr (27) + 'P'+ Chr (15); Lim: 200),
     (A: Chr (27) + 'M'+ Chr (15); Lim: 255),
     (A: Chr (27) + Chr (38) + Chr (107) + Chr (48) + Chr (83) + ' '; Lim: 80),
     (A: Chr (27) + Chr (38) + Chr (107) + Chr (50) + Chr (83) + ' '; Lim: 132) );
  Repfsmin: Asarr = (11, 08, 06, 06, 05, 05, 03, 11, 04, 15);
  Repfsmax: Asarr = (11, 31, 30, 30, 05, 05, 30, 11, 04, 15);

label kraj;
Var
  Dlg : Pdialog;
  R : TRect;
  Control, Hscroll : Pview;
  C, Sirina,minsirina, I,maksimum : Integer;
  Actualsize : AsArr;
  Ukupno, Linija, Strana, Linustrani, Slog,firstmarked : Word;
  BB:^Indexitem;
  window:PPrintwindow;
  datoteka:text;
  filename,st:string;
  cc:Boolean;
  daljinar:pprocenter2;
Begin
  R.Assign (3, 1, 76, 21);
  New (Dlg, Init (R, 'Report definition') );
  Dlg^.helpctx:=hcizvjbase;
  R.Assign (3, 3, 50, 8);
  Control := New (Pcheckboxes, Init (R,
    NewSItem ('Short name',
    NewSItem ('Full name',
    NewSItem ('Corporation name',
    NewSItem ('Category of program',
    NewSItem ('Year of production',
    NewSItem ('Computer type',
    NewSItem ('Message language',
    NewSItem ('File name',
    NewSItem ('Position',
    NewSItem ('Problems', Nil) ) ) ) ) ) ) ) ) ) ) );
  Dlg^.Insert (Control);

  R.Assign (7, 2, 25, 3);
  Dlg^.Insert (New (Plabel, Init (R, '~I~nclude to report', Control) ) );

  R.Assign (53, 2, 71,8);
  Control := New (Pradiobuttons, Init (R,
    NewSItem ('Printer LPT1',
    NewSItem ('Printer LPT2',
    NewSItem ('Printer COM1',
    NewSItem ('Printer COM2',
    NewSItem ('File',
    NewSItem ('Screen', Nil) ) ) ) ) ) ) );


  Dlg^.Insert (Control);

  R.Assign (54, 1, 68, 2);
  Dlg^.Insert (New (Plabel, Init (R, 'Output ~d~evice', Control) ) );

  R.Assign (3, 10, 11, 11);
  Control := New (Pnuminputline, Init (R, 6, 10, 200) );
  Dlg^.Insert (Control);

  R.Assign (2, 9, 16, 10);
  Dlg^.Insert (New (Plabel, Init (R, 'Report ~l~ength', Control) ) );

  R.Assign (19, 10, 27, 11);
  Control := New (Pnuminputline, Init (R, 6, 10, 200) );
  Dlg^.Insert (Control);

  R.Assign (18, 9, 34, 10);
  Dlg^.Insert (New (Plabel, Init (R, '~P~rintable lines', Control) ) );

  R.Assign (38, 10, 46, 11);
  Control := New (PnumInputline, Init (R, 6, 10, 255) );
  Dlg^.Insert (Control);

  R.Assign (37, 9, 50, 10);
  Dlg^.Insert (New (Plabel, Init (R, 'Report ~w~idth', Control) ) );

  R.Assign (54, 9, 68, 12);
  Control := New (Pradiobuttons, Init (R,
    NewSItem ('All',
    NewSItem ('Marked',
    NewSItem ('Unmarked', Nil) ) ) ) );
  Dlg^.Insert (Control);

  R.Assign (56, 8, 64, 9);
  Dlg^.Insert (New (Plabel, Init (R, '~R~ecords', Control) ) );

  R.Assign (3, 13, 60, 18);
  Control := New (Pradiobuttons, Init (R,
    NewSItem ('ASCII A4',
    NewSItem ('ASCII A3',
    NewSItem ('EPSON A4 PICA',
    NewSItem ('EPSON A4 COMPRESS',
    NewSItem ('EPSON A4 ELITE COMPRESS',
    NewSItem ('EPSON A3 PICA',
    NewSItem ('EPSON A3 COMPRESS',
    NewSItem ('EPSON A3 ELITE COMPRESS',
    NewSItem ('HP LASER JET COURIER',
    NewSItem ('HP LASER JET COMPRESSED', Nil) ) ) ) ) ) ) ) ) ) ) );
  Dlg^.Insert (Control);

  R.Assign (23, 12, 36, 13);
  Dlg^.Insert (New (Plabel, Init (R, 'Printer ~t~ype', Control) ) );

  R.Assign (61, 13, 71, 15);
  Control := New (Pbutton, Init (R, '~O~K', Cmok, bfNormal) );
  Dlg^.Insert (Control);

  R.Assign (61, 16, 71, 18);
  Control := New (Pbutton, Init (R, '~C~ancel', Cmcancel, bfNormal) );
  Dlg^.Insert (Control);

  Dlg^.Selectnext (False);
  Dlg^.Setdata(printpars);
  C:= Desktop^.Execview (Dlg);
  hourglascur;
  Dlg^.Getdata (printpars);
  Dispose (Dlg, Done);
  If C<>cmOK Then Exit;
  Sirina:=0;
  minsirina:=0;
  For I:=0 to 9 Do Begin
    Actualsize[I]:=0;
    If ((printpars.Stauklj) and (1 shl i))<>0 then begin
      actualsize[i]:=repfsmax[i];
      sirina:=sirina+repfsmax[i];
      minsirina:=minsirina+repfsmin[i];
    end
  end;
  if minsirina>printpars.repwid-6 then begin
     DisplayError('Report can not fit in report of such witdh',0);
     exit;
  end;
  i:=0;
  while sirina>printpars.repwid-6 do begin
    if (actualsize[i]>repfsmin[i]) and (actualsize[i]<>0) then begin
      dec(actualsize[i]);
      dec(sirina);
    end;
    inc(i);
    if i=10 then i:=0;
  end;
  Ukupno:=Ind^.Count;
  firstmarked:=ukupno;
  for i:=0 to Ind^.count-1 do begin
    BB:=Ind^.At(i);
    if bb^.indvalue[1]='*' then begin
      firstmarked:=i;
      goto kraj;
    end;
  end;
  kraj:
  if printpars.which=2 then ukupno:=firstmarked;
  if printpars.which=1 then ukupno:=ukupno-firstmarked;
  if printpars.device=5 then begin
    R.Assign(1,1, 79, 20);
    Window := New(PPrintWindow, Init(R, 'Screen Preview', 2,
      ukupno,firstmarked,actualsize,printpars.replen,printpars.repwid));
    c:=DeskTop^.execview(Window);
    hourglascur;
    dispose(Window,done);
    exit;
  end;
  case printpars.device of
    0: filename:='LPT1';
    1: filename:='LPT2';
    2: filename:='COM1';
    3: filename:='COM2';
    4: filename:=outputfile('ZXSHREP.TXT','');
  end;
  assign(datoteka,filename);
  if not(frewritet(datoteka)) then exit;
  maksimum:=printpars.Replen*(1+Ukupno div printpars.printable)-1;
  new(daljinar,init(16,10,52,0,maksimum));
  desktop^.insert(daljinar);

  For Linija:=0 to maksimum do begin
    daljinar^.update(linija,'');
    Strana:=Linija div printpars.replen;
    Linustrani:=Linija mod printpars.replen;
    cc:=true;
    str(strana+1:4,st);
    if Linustrani=0 then cc:=fwriteln (datoteka,'   Page '+st+' '+
      printers[printpars.Prntype].a)
      else begin
        if Linustrani>=printpars.Printable then cc:=fwriteln (datoteka,'')
          else begin
            slog:=(printpars.Printable-1)*Strana+Linustrani-1;
            if slog>=Ukupno then cc:=fwriteln(datoteka,'')
              else begin
                if printpars.which=1 then slog:=slog+firstmarked;
                cc:=fwriteln(datoteka,reportline(slog,actualsize));
              end
          end
      end;
      if not(cc) then begin
        desktop^.delete(daljinar);
        dispose(daljinar,done);
        cc:=fcloset(datoteka);
        exit
      end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  cc:=fcloset(datoteka);

End;


Procedure Readadittion;
Var Item:^Dictitem;
  Titem:^Fileitem;
  I:Integer;
Begin
  hourglascur;
  Proizvodjaci:= New (Pdictionary, Init (500, 10) );
  Proizvodjaci^.Bazafield:= 3;
  Proizvodjaci^.Slovo:= 'P';
  Jezici:= New (Pdictionary, Init (30, 4) );
  Jezici^.Bazafield:= 7;
  Jezici^.Slovo:= 'J';
  Tipovi:= New (Pdictionary, Init (100, 10) );
  Tipovi^.Bazafield:= 4;
  Tipovi^.Slovo:= 'V';
  Tapes:= New (Pfilelist, Init (100, 10) );
  Tapes^.Bazafield:= 8;
  New(Item);
  Item^.Key:='    ';
  Item^.Dictvalue:=#22;
  Item^.Pozicija:=0;
  Proizvodjaci^.Insert(Item);
  New(Item);
  Item^.Key:='    ';
  Item^.Dictvalue:=#22;
  Item^.Pozicija:=0;
  Jezici^.Insert(Item);
  New(Item);
  Item^.Key:='    ';
  Item^.Dictvalue:=#22;
  Item^.Pozicija:=0;
  Tipovi^.Insert(Item);

  {Intbaseput ('P    '+chr(22));
  Intbaseput ('J    '+chr(22));
  Intbaseput ('V    '+chr(22));
  Intbaseput ('T'+chr(22));}
  New (Titem);
  With Titem^ Do Begin
   Tip:= 'T';
   Filename:=#22;
   Pozicija:=0;
   Tapes^.Insert (Titem);
  end;
  OpenDFile(Addition);
  For I:= 1 To addition.reccount Do Begin
    Getdbrec (Addition,i,addbuf);
    if addbuf.deleted<>#$2A then begin
      If Addbuf.Redpodat [1] In ['P', 'J', 'V'] Then Begin
        New (Item);
        Item^.Key:= Copy (Addbuf.Redpodat, 2, 4);
        Item^.Dictvalue:= Copy (Addbuf.Redpodat, 6, 54);
        Item^.Pozicija:=i;
        Case Addbuf.Redpodat [1] Of
          'P': Proizvodjaci^.Insert (Item);
          'J': Jezici^.Insert (Item);
          'V': Tipovi^.Insert (Item);
        End;
      End;

      If Addbuf.Redpodat [1] In ['T', 'S', 'M','O'] Then Begin
        New (Titem);
        With Titem^ Do Begin
          Tip:= Addbuf.Redpodat [1];
          TItem^.Pozicija:=i;
          FileName:= Rtrim(Copy (Addbuf.Redpodat, 2, 10));
        End;
        Tapes^.Insert (Titem);
      End

    End;
  End;
  CloseDFile(Addition);
End;

Function Initdbase:Boolean;
Var I: Integer;
Begin
  Initdbase:=false;
  Baza.dbfname:=dbasedir+'SOFTWARE.DBF';
  Addition.dbfname:=dbasedir+'ADDITION.DBF';
  Specbase.dbfname:='SPECBASE.DBF';
  Dbtname:=dbasedir+'SOFTWARE.DBT';
  if not(fileexists(Baza.dbfname)) then
     Zap(Baza,emptySoftware,sizeof(emptysoftware));
  if not(fileexists(Addition.dbfname)) then
     Zap(Addition,emptyaddition,sizeof(emptyaddition));
  if not(fileexists(dbtname)) then
     Zapdbt(dbtname,emptydbt,sizeof(emptydbt));
  if not(Initdbf(Baza)) then exit;
  if not(Initdbf(Addition)) then exit;
  Initdbase:=true;
End;

Function Indexstring: String;
var Result1 : String[40];
  I: Integer;
Begin
  If indexedfields='S' then begin
    Indexstring:=bazafield(14)+bazafield(1);
    exit
  end;
  Result1:= BazaField (14);
  for I:=1 to length(Indexedfields) do Begin
    Case Indexedfields[i] of
      'S': Result1:=Result1+copy(Bazafield(1),1,indexsize[2]);
      'F': Result1:=Result1+upper(copy(Bazafield(2),1,indexsize[3]));
      'P': Result1:=Result1+upper(proizvodjaci^.keyval(Bazafield(3),
           indexsize[4]-4))+Bazafield(3);
      'C': Result1:=Result1+upper(tipovi^.keyval(Bazafield(4),
           indexsize[5]-4))+Bazafield(4);
      'Y': Result1:=Result1+Bazafield(5);
      'M': Result1:=Result1+Bazafield(6);
      'L': Result1:=Result1+upper(jezici^.keyval(Bazafield(7),
           indexsize[8]-4))+Bazafield(7);
      'T': Result1:=Result1+upper(Bazafield(8)+Bazafield(9))
    End
  End;
  if length(Result1)>allocsirindex-3 then Result1:=Copy(Result1,1,allocsirindex-3);
  Indexstring:=Result1;
End;

Type TCategBoxes=Object(TCheckBoxes)
  linkedit:pinputline;
  procedure press(Item:integer); virtual;
  procedure dodaj(znak:char);
  procedure oduzmi(znak:char);
end;
PCategBoxes=^TCategBoxes;
procedure tcategboxes.dodaj(znak:char);
begin
  if pos(znak,linkedit^.data^)<=0 then begin
    linkedit^.data^:=linkedit^.data^+znak;
    linkedit^.draw;
  end
end;

procedure tcategboxes.oduzmi(znak:char);
var pozicija:integer;
begin
  pozicija:=pos(znak,linkedit^.data^);
  if pozicija>0 then begin
    delete(linkedit^.data^,pozicija,1);
    linkedit^.draw;
  end
end;


procedure tcategboxes.press(item:integer);
const stringovi:array[0..7] of char=
   ('S','F','P','C','Y','M','L','T');

begin
  tcheckboxes.press(item);
  if ((1 shl item) and value)<>0 then dodaj(stringovi[item])
   else oduzmi(stringovi[item]);
end;


Procedure DBSort;
var
  Dlg : PDialog;
  R : TRect;
  HScroll,Control : PView;
  Boksovi: Pcategboxes;
  UlazLinija: PInputline;
  Bubble:PCheckBoxes;
  c,i:word;
  st1:string;
  zamijenio:Boolean;
  DataRec:Record
    dBoxovi : Word; {Checkboxes}
    dLinija : String[8]; {Inputline}
  end;
  helpbuf,helpbuf2:tsoftbuf;
const stringovi:array[0..7] of char=
   ('S','F','P','C','Y','M','L','T');


Begin
  With Datarec do begin
    dboxovi:=0;
    for i:=0 to 7 do
      if pos(stringovi[i],indexedfields)>0 then
        dboxovi:=dboxovi or (1 shl i);
  end;
  datarec.dLinija:=indexedfields;
  R.Assign(10,2,70,20);
  New(Dlg, Init(R, 'Order definition'));
  dlg^.helpctx:=hcsortbase;
  R.Assign(4,3,53,7);
  Boksovi := New(PCategboxes, Init(R,
    NewSItem ('Short name',
    NewSItem ('Full name',
    NewSItem ('Corporation name',
    NewSItem ('Category of program',
    NewSItem ('Year of production',
    NewSItem ('Computer type',
    NewSItem ('Message language',
    NewSItem ('File name and Position',nil))))))))));

  Dlg^.Insert(Boksovi);

    R.Assign(16,2,24,3);
    Dlg^.Insert(New(PLabel, Init(R, 'Sort ~b~y', Boksovi)));

  R.Assign(20,10,30,11);
  UlazLinija := New(PInputLine, Init(R, 8));
  Dlg^.Insert(UlazLinija);
  Boksovi^.linkedit:=UlazLinija;
  UlazLinija^.Options:=4;
    R.Assign(18,9,32,10);
    Dlg^.Insert(New(PLabel, Init(R, '~S~orting order', nil)));
  R.Assign(7,14,19,16);
  Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(29,14,41,16);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  Dlg^.Setdata(datarec);
  c:=desktop^.execview(dlg);
  hourglascur;
  Dlg^.Getdata(datarec);
  dispose(dlg,done);
  if c=cmOK then begin
    indexedfields:=Datarec.dlinija;
    reindex
  end
end;

function dbstart(koji:string;var progelem:exrecdata):Boolean;
var name:string;
  i,j:integer;
  daljinar:pprocenter2;
Begin
  dbstart:=false;
  if not(initdbase) then exit;
  if Baza.reccount=0 then Begin
    exit
  end;
  for i:=1 to length(koji) do koji[i]:=upcase(koji[i]);
  koji:=rtrim(koji);
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  for i:=1 to baza.reccount do begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    getdbrec(baza,i,softbuf);
    if koji=rtrim(bazafield(1)) then begin
      for j:=1 to addition.reccount do begin
        getdbrec(addition,j,addbuf);
        if copy(addfield(1),2,length(bazafield(8)))=bazafield(8) then begin
          with progelem.fi do begin
            Name:=addfield(1);
            Tip:= Name [1];
            Filename:= Copy (Name, 2, 10);
          end;
          progelem.Direct:= Copy (Name, 12, 36);
          progelem.Vollab:= Copy (Name, 49, 11);
          progelem.kompjuter:=bazafield(6);
          progelem.pozicija:=bazafield(9);
          progelem.simulopc:=bazafield(12);
          dbstart:=true;
          desktop^.delete(daljinar);
          dispose(daljinar,done);

          exit;
        End;
      End;
    End;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
End;


procedure SelectDBdir;
var
  D: PChDirDialog;
  c:word;
  stari,novi:string;
begin
  getdir(0,stari);
  D := New(PChDirDialog, Init(cdNormal, 101));
  D^.helpctx:=hcdbselect;
  c:=DeskTop^.ExecView(D);
  hourglascur;
  Dispose(D, Done);
  getdir(0,novi);
  chdir(stari);
  if novi[length(novi)]<>'\' then novi:=novi+'\';
  dbasedir:=novi;
  if not(Initdbase) then exit;;
end;

function posjock(st1,st2:string):integer;
var i,j,p:integer;
  nadjen:Boolean;
begin
  posjock:=0;
  for i:=1 to length(st2) do begin
    nadjen:=true;
    for j:=1 to length(st1) do begin
      p:=i+j-1;
      if p>length(st2) then exit;
      if (st1[j]<>'?') and (upcase(st1[j])<>upcase(st2[p]))
        then nadjen:=false;
    end;
    if nadjen then begin posjock:=i; exit end;
  end;
end;


function like(st1,st2:string):Boolean;
var i,j,zadnjigdje,gdje:integer;
  st3,st4:string;
  dzoker,prvidzoker:Boolean;
begin
  dzoker:=false;
  prvidzoker:=false;
  if (length(st1)>1) and (st1[1]='*') then prvidzoker:=true;
  like:=true;
  st4:=st2;
  i:=1;j:=1;
  zadnjigdje:=0;
  repeat
    st3:='';
    while (i<=length(st1)) and (st1[i]<>'*') do begin
      st3:=st3+st1[i];
      i:=i+1;
    end;
    if (st3<>'') or (st4<>'') then gdje:=posjock(st3,st4);
    if not(prvidzoker) and (st2=st4) and (gdje<>1) then like:=false;
    if gdje<>0 then st4:=copy(st4,gdje+length(st3),
      length(st4)-(gdje+length(st3))+1)
     else begin like:=false; exit end;
    if (i<=length(st1)) and (st1[i]='*') then dzoker:=true else dzoker:=false;
    if dzoker then i:=i+1 else begin
      if (st3<>'') and (st4<>'') then like:=false;
       exit;
    end;
  until false;
end;


function findmark(mode:integer):word;
{mode=1 mark
 mode=2 search}
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c,i,j:word;
  mijenjaj:Boolean;
  bb:^Indexitem;
  DataRec : record
    fShort : String[10]; {Inputline}
    ffull : String[30]; {Inputline}
    fcategory : String[30]; {Inputline}
    fproducer : String[30]; {Inputline}
    fmeslang : String[30]; {Inputline}
    fcomp : String[4]; {Inputline}
    fyear : String[4]; {Inputline}
    ftname : String[10]; {Inputline}
  end;
  daljinar:pprocenter2;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
  end;
Begin
  findmark:=0;
  R.Assign(3,1,76,21);
  case mode of
    1: begin
     New(Dlg, Init(R, 'Mark conditional'));
     Dlg^.helpctx:=hcbmarkcond;
    end;
    2: begin
       New(Dlg, Init(R, 'Search'));
       Dlg^.helpctx:=hcseek;
    end;
  end;
  R.Assign(7,17,34,18);
  Control := New(PStaticText, Init(R, 'Wildcards (*,?) are allowed'));
  Dlg^.Insert(Control);

  R.Assign(15,2,27,3);
  Control := New(PInputLine, Init(R, 10));
  Dlg^.Insert(Control);

  R.Assign(3,2,14,3);
  Dlg^.Insert(New(PLabel, Init(R, '~S~hort name', Control)));

  R.Assign(15,4,47,5);
  Control := New(PInputLine, Init(R, 30));
  Dlg^.Insert(Control);

  R.Assign(3,4,13,5);
  Dlg^.Insert(New(PLabel, Init(R, '~F~ull name', Control)));

  R.Assign(15,6,47,7);
  Control := New(PInputLine, Init(R, 30));
  Dlg^.Insert(Control);

  R.Assign(3,6,12,7);
  Dlg^.Insert(New(PLabel, Init(R, '~C~ategory', Control)));

  R.Assign(15,8,47,9);
  Control := New(PInputLine, Init(R, 30));
  Dlg^.Insert(Control);

  R.Assign(3,8,12,9);
  Dlg^.Insert(New(PLabel, Init(R, '~P~roducer', Control)));

  R.Assign(22,10,54,11);
  Control := New(PInputLine, Init(R, 30));
  Dlg^.Insert(Control);

  R.Assign(3,10,20,11);
  Dlg^.Insert(New(PLabel, Init(R, '~M~essage language', Control)));

  R.Assign(19,12,25,13);
  Control := New(PInputLine, Init(R, 4));
  Dlg^.Insert(Control);

  R.Assign(4,12,18,13);
  Dlg^.Insert(New(PLabel, Init(R, 'Comp~u~ter type', Control)));

  R.Assign(36,12,42,13);
  Control := New(PInputLine, Init(R, 4));
  Dlg^.Insert(Control);

  R.Assign(30,12,35,13);
  Dlg^.Insert(New(PLabel, Init(R, '~Y~ear', Control)));

  R.Assign(17,14,29,15);
  Control := New(PInputLine, Init(R, 10));
  Dlg^.Insert(Control);

  R.Assign(5,14,15,15);
  Dlg^.Insert(New(PLabel, Init(R, '~T~ape name', Control)));

  R.Assign(39,16,51,18);
  Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(55,16,67,18);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(Dlg);
  hourglascur;
  dlg^.getdata(datarec);
  dispose(dlg,done);
  if c=cmOK then begin
    With DataRec do begin
      fShort := Rtrim(fshort); {Inputline}
      ffull := rtrim(ffull); {Inputline}
      fcategory :=rtrim(fcategory); {Inputline}
      fproducer :=rtrim(fproducer); {Inputline}
      fmeslang :=rtrim(fmeslang); {Inputline}
      fcomp :=rtrim(fcomp); {Inputline}
      fyear :=rtrim(fyear); {Inputline}
      ftname :=rtrim(ftname); {Inputline}
    end;
    new(daljinar,init(16,10,52,1,baza.reccount));
    desktop^.insert(daljinar);

    For I:= 1 To baza.reccount Do Begin
      if (i mod 100) = 0 then daljinar^.update(i,'');
      Getdbrec (Baza,i,softbuf);
      mijenjaj:=true;
      With Datarec do begin
        if fShort<>'' then begin
          if not(like(fShort,Rtrim(BazaField(1)))) then mijenjaj:=false;
        end;
        if ffull<>'' then
          if not(like(ffull,Rtrim(BazaField(2)))) then mijenjaj:=false;
        if fproducer<>'' then
          if not(like(fproducer,Rtrim(proizvodjaci^.keyval(Bazafield(3),
            30)))) then
            mijenjaj:=false;
        if fcategory<>'' then
          if not(like(fcategory,
          Rtrim(tipovi^.keyval(Bazafield(4),30)))) then
            mijenjaj:=false;
        if fmeslang<>'' then
          if not(like(fmeslang,
          Rtrim(jezici^.keyval(Bazafield(7),30)))) then
            mijenjaj:=false;
        if fcomp<>'' then
          if not(like(fcomp,Rtrim(BazaField(6)))) then mijenjaj:=false;
        if ftname<>'' then
          if not(like(ftname,Rtrim(BazaField(8)))) then mijenjaj:=false;
        if fyear<>'' then
          if not(like(fyear,Rtrim(BazaField(5)))) then mijenjaj:=false;
        case mode of
          1: if mijenjaj then begin
            BazaReplace (14, '*');
            Putdbrec (Baza,i,softbuf);
          end;
          2: if mijenjaj then begin
            for j:=0 to ind^.count-1 do begin
              bb:=Ind^.At(j);
              if bb^.position=i then begin
                findmark:=j;
                killdalj;
                exit;
              end
            end
          end;
        End;
      End;
    End;
    killdalj;
    Reindex;
  end;
end;

procedure markselected;
var c:word;
begin
  c:=findmark(1)
end;


procedure unmarkall;
var i:integer;
  daljinar:pprocenter2;
  IndItem:^Indexitem;
begin
  hourglascur;

  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  makeemptyind;
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,softbuf);
    BazaReplace (14, ' ');
    Putdbrec (Baza,I,softbuf);
    GetMem(IndItem,Allocsirindex);
    IndItem^.Indvalue:= Indexstring;
    IndItem^.Position:= Baza.recnum;
    Ind^.Insert (IndItem);

  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);

end;

procedure swappriority;
var i:integer;
  daljinar:pprocenter2;
  IndItem:^Indexitem;
begin
  hourglascur;
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  opendfile(baza);
  makeemptyind;
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100)=0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,softbuf);
    If bazafield(14)<>' ' then BazaReplace (14, ' ')
      else BazaReplace (14, '*');
    Putdbrec (Baza,I,softbuf);
    GetMem(IndItem,Allocsirindex);
    IndItem^.Indvalue:= Indexstring;
    IndItem^.Position:= Baza.recnum;
    Ind^.Insert (IndItem);

  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  closedfile(baza);
end;

procedure deallocdb;
begin
  dispose(ind,done);
  dispose(proizvodjaci,done);
  dispose(tipovi,done);
  dispose(jezici,done);
  dispose(tapes,done);
end;

Procedure allocdb;
var cc:Boolean;
Begin
  cc:=Initdbase;
  Readadittion;
  CreateIndex;
end;

type
  tMain=record
    Emulator:byte;
    filename:string[12];
    filesize:longint;
    timedate:longint;
    crc:array[1..7] of byte;
    prgname:word;
    producer:word;
    filepath:word;
    remark:string[80];
    memoryrequest:byte;
    unknown:array[1..26] of byte;
    year:word;
    progtype:word;
    author:word;
    language:word;
    special:word;
    screen:word;
    nesto:byte;
    problems:word;
    nesto2:word;
    volume:word;
    startup:word;
    block:word;
    paddle:array[1..90] of byte;
end;
tprogname=record
  id:word;
  value:string[61];
end;
tprogtype=record
  id:word;
  value:string[29];
end;
tproducer=record
  id:word;
  value:string[29];
end;
tprogpath=record
  id:word;
  value:string[77];
end;
tauthors=record
  id:word;
  value:string[29];
end;
tcountry=record
  id:word;
  value:string[13];
end;
tspecials=record
  id:word;
  value:string[29];
end;
tpictures=record
  id:word;
  value:string[61];
end;
tvolumes=record
  id:word;
  value:string[17];
end;
tstartups=record
  id:word;
  value:string[63];
end;

Procedure WarajevoSpecPicRainbow(rainbow:Boolean);

var
  outfile,outfile2,Dbt:file;
  progname:tprogname;
  progtype:tprogtype;
  producer:tproducer;
  authors:tauthors;
  specials:tspecials;
  country:tcountry;
  progpath:tprogpath;
  volumes:tvolumes;
  startups:tstartups;
  main:tmain;
  daljinar:PProcenter;
  i,j,dOpisDuz,c:word;
  n:char;
  pozicija:longint;
  cc:Boolean;
  procedure we;
  begin
    cc:=fclose(outfile);
  end;
  procedure we2;
  begin
    cc:=fclose(outfile2);
    cc:=fclose(outfile);
  end;
  procedure we3;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    closedfile(baza);
    cc:=fclose(outfile);
  end;
begin
 if messagebox('Convert database?',
  nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;

  hourglascur;

  assign(outfile,'PROGNAME.TBL');
  if not(frewriteu(outfile,1)) then exit;
  if rainbow then assign(outfile2,'STARTUP.TBL')
    else assign(outfile2,'SPECIALS.TBL');
  if not(frewriteu(outfile2,1)) then begin we;exit end;
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  opendfile(baza);
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,I,softbuf);
    progname.value:=rtrim(bazafield(2));
    progname.id:=1;
    specials.value:=rtrim(bazafield(12));
    specials.id:=1;
    if not(fblockwrite(outfile,progname,sizeof(tprogname))) then begin
      we2; exit
    end;
    if not(fblockwrite(outfile2,specials,sizeof(tspecials))) then begin
      we2; exit
    end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  closedfile(baza);
  close(outfile);
  close(outfile2);

  assign(outfile,'PROGTYPE.TBL');
  if not(frewriteu(outfile,1)) then exit;
  new(daljinar,init(16,10,52,1,Tipovi^.count));
  desktop^.insert(daljinar);
  For I:= 1 To Tipovi^.count-1 Do Begin
    daljinar^.update(i,'');
    progtype.value:=rtrim(Pdictitem (Tipovi^. At (i) )^.DictValue);
    progtype.id:=1;
    if not(fblockwrite(outfile,progtype,sizeof(tprogtype))) then begin
      we; exit
    end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  close(outfile);

  assign(outfile,'PRODUCER.TBL');
  if not(frewriteu(outfile,1)) then exit;;
  assign(outfile2,'AUTHORS.TBL');
  if not(frewriteu(outfile2,1)) then begin we; exit end;
  new(daljinar,init(16,10,52,1,Proizvodjaci^.count));
  desktop^.insert(daljinar);
  For I:= 1 To Proizvodjaci^.count-1 Do Begin
    daljinar^.update(i,'');
    producer.value:=rtrim(Pdictitem (Proizvodjaci^. At (i) )^.DictValue);
    producer.id:=1;
    authors.id:=1;
    authors.value:=producer.value;
    if not(fblockwrite(outfile,producer,sizeof(tproducer))) then begin
      we2; exit
    end;
    if not(fblockwrite(outfile2,authors,sizeof(tauthors))) then begin
      we2; exit
    end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  cc:=fclose(outfile);
  cc:=fclose(outfile2);

  assign(outfile,'COUNTRY.TBL');
  if not(frewriteu(outfile,1)) then exit;
  new(daljinar,init(16,10,52,1,jezici^.count));
  desktop^.insert(daljinar);
  For I:= 1 To jezici^.count-1 Do Begin
    daljinar^.update(i,'');
    country.value:=rtrim(Pdictitem (jezici^. At (i) )^.DictValue);
    country.id:=1;
    if not(fblockwrite(outfile,country,sizeof(tcountry))) then begin
       we;exit
    end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  cc:=fclose(outfile);


  assign(outfile,'PROGPATH.TBL');
  if not(frewriteu(outfile,1)) then begin exit end;
  new(daljinar,init(16,10,52,1,tapes^.count));
  desktop^.insert(daljinar);
  For I:= 1 To tapes^.count-1 Do Begin
    daljinar^.update(i,'');
    progpath.value:=rtrim(Directory(Pfileitem (tapes^. At (i) )));
    progpath.id:=1;
    if not(fblockwrite(outfile,progpath,sizeof(tprogpath))) then begin
      we; exit ;
    end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  cc:=fclose(outfile);

  if rainbow then begin
     assign(outfile,'VOLUMES.TBL');
     if not(frewriteu(outfile,1)) then begin exit end;
     new(daljinar,init(16,10,52,1,tapes^.count));
     desktop^.insert(daljinar);
     For I:= 1 To tapes^.count-1 Do Begin
       daljinar^.update(i,'');
       volumes.value:=rtrim(Vollabel(Pfileitem (tapes^. At (i) )));
       volumes.id:=1;
       if not(fblockwrite(outfile,volumes,sizeof(tvolumes))) then begin
         we; exit ;
       end;
     End;
     desktop^.delete(daljinar);
     dispose(daljinar,done);
     cc:=fclose(outfile);
  end;

  assign(outfile,'DATABASE.TBL');
  if not(frewriteu(outfile,1)) then exit;
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);
  opendfile(baza);
  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100) = 0 then daljinar^.update(i,'');
    Getdbrec (Baza,i,softbuf);
    with main do begin
      filename:=Rtrim(BazaField (8));

      filesize:=0;
      timedate:=0;
      for j:=1 to 7 do crc[j]:=0;
      prgname:=i;
      producer:=Proizvodjaci^.Finditem (1, BazaField (3) ) ;
      filepath:=Tapes^.Finditem (Rtrim(BazaField (8)) );
      if rainbow then volume:=Tapes^.Finditem (Rtrim(BazaField (8)) );
      if filepath<>0 then begin
        n:=Pfileitem (tapes^. At (filepath) )^.tip;
        case n of
          'T': begin if rainbow then emulator:=$45 else emulator:=$85; filename:=filename+'.TAP' end;
          'S': begin emulator:=1; filename:=filename+'.Z80' end;
          'M': begin if rainbow then emulator:=$85 else emulator:=6; filename:=filename+'.MDR' end;
        end;
      end;
{      remark:string[80];}
      Val (Rtrim (BazaField (13) ), Pozicija, j);
      remark:='';
      For j:= 1 To 80 Do remark:=remark+' ';
      If Pozicija<> 0 Then Begin
        Assign (Dbt, dbtname);
        if not(fResetu (Dbt, 1)) then begin we3; exit end;
        if not(fSeek (Dbt, Pozicija* 512)) then begin
           cc:=fclose(dbt); we3 ; exit
        end;
        if not(fBlockRead (Dbt, remark[1], 80)) then begin
           cc:=fclose(dbt);we3 ; exit
        end;
        if not(fClose (Dbt)) then begin we3; exit end;
      end;
      dOpisduz:= 1;
      While (dOpisduz< 80) And (remark[dOpisduz] <> #$1A) Do Inc (dOpisduz);
      Dec (dOpisduz);
      For j:= dOpisDuz To 80 Do remark[j]:=' ';

      If  (BazaField (6)= '16  ') or(BazaField (6)= '48  ') then
        memoryrequest:=0
       else memoryrequest:=1;
      for j:=1 to 26 do unknown[j]:=0;
      val(rtrim(BazaField(5)),year,j);
      progtype:=Tipovi^.Finditem (1, BazaField (4));
      author:=Proizvodjaci^.Finditem (1, BazaField (3) ) ;
      language:=Jezici^.Finditem (1, BazaField (7) );
      if rainbow then startup:=i else special:=i;
      screen:=0;
      nesto:=0;
      nesto2:=0;
      if rainbow then begin
        Val (Rtrim (BazaField (11) ), problems, C);
        Val (Rtrim (BazaField (9) ), block, C);
      end
       else begin
         problems:=0;
         volume:=0;
         startup:=0;
         block:=0;
       end;
      for j:=1 to 90 do paddle[j]:=0;
    end;
    if not(fblockwrite(outfile,main,sizeof(tmain))) then begin we3;exit end;
  End;
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  closedfile(baza);
  close(outfile);

end;
procedure WarajevoSpecpic;
begin
   WarajevoSpecPicRainbow(false);
end;
procedure WarajevoRainbow;
begin
   WarajevoSpecPicRainbow(true);
end;

procedure warajevosgd;
var mainst,prazan,temp:string[137];
filepath,i,j:word;
izlaz:text;
daljinar:PProcenter;
n:char;
cc:Boolean;
procedure we;
begin
  closedfile(baza);
  cc:=fcloset(izlaz);
end;
begin
 if messagebox('Convert database?',
  nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;
  hourglascur;
  prazan:= '';
  for j:=1 to 137 do prazan:=prazan+' ';
  assign(izlaz,'GAMES.DAT');
  if not(frewritet(izlaz)) then exit;
  opendfile(baza);
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);

  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100)= 0 then daljinar^.update(i,'');
    mainst:=prazan;
    Getdbrec (Baza,i,softbuf);
    temp:=bazafield(2);
    for j:=1 to 30 do mainst[j]:=temp[j];
    temp:=proizvodjaci^.keyval(Bazafield(3),30);
    for j:=1 to 30 do
      mainst[j+42]:=temp[j];
    temp:=tipovi^.keyval(bazafield(4),7);
    for j:=1 to 7 do
      mainst[j+106]:=temp[j]; {tip}
    temp:=bazafield(5);
    for j:=1 to 4 do
      mainst[j+37]:=temp[j]; {godina}
    temp:=bazafield(6);
    if temp[3]=' ' then temp:=' '+copy(temp,1,2);
    for j:=1 to 3 do
      mainst[j+79]:=temp[j]; {masina}
    temp:=bazafield(8);
    for j:=1 to 8 do
      mainst[j+93]:=temp[j]; {filename}
    mainst[102]:='.';
    filepath:=Tapes^.Finditem (Rtrim(BazaField (8)) );
    if filepath<>0 then begin
      n:=Pfileitem (tapes^. At (filepath) )^.tip;
      case n of
        'T': begin mainst[103]:='T';mainst[104]:='A';mainst[105]:='P'; end;
        'S': begin mainst[103]:='Z';mainst[104]:='8';mainst[105]:='0'; end;
        'M': begin mainst[103]:='M';mainst[104]:='D';mainst[105]:='R'; end;
      end;
    end;
    mainst[125]:='0';    {duzina fajla}
    mainst[84]:='1';     {broj igraca}
    mainst[134]:='0';    {?}
    mainst[137]:='2';    {warajevo}
    mainst[115]:='1';    {direktorij}
    temp:=bazafield(12);
    if pos('/jk',temp)>0 then mainst[88]:='K';
    if pos('/JK',temp)>0 then mainst[88]:='K';
    if pos('/ja',temp)>0 then mainst[88]:='K';
    if pos('/JA',temp)>0 then mainst[88]:='K';
    if pos('/jm',temp)>0 then mainst[88]:='K';
    if pos('/JM',temp)>0 then mainst[88]:='K';
    if pos('/js',temp)>0 then mainst[89]:='1';
    if pos('/JS',temp)>0 then mainst[89]:='1';
    if pos('/jb',temp)>0 then mainst[90]:='2';
    if pos('/JB',temp)>0 then mainst[90]:='2';
    if pos('/jc',temp)>0 then mainst[91]:='C';
    if pos('/JC',temp)>0 then mainst[91]:='C';

    if not(fwriteln(izlaz,mainst)) then begin we; exit end;
  end;
  desktop^.delete(daljinar);
  dispose(daljinar,done);

  cc:=fcloset(izlaz);
  closedfile(baza);
end;
type
TPaths= Object (TCollection)
            Procedure Freeitem (Item: Pointer); Virtual;
End;
PPaths= ^TPaths;
Procedure TPaths.FreeItem(Item:Pointer);
begin
  Dispose(Item)
end;

function putcode(Dictionary:PDictionary;StrValue:String):String;
var Inicod:String[4];
  I:Integer;
  bb:PDictItem;
  gotovo:Boolean;
begin
  if rtrim(StrValue)='' then begin putcode:='0000'; exit end;
  Inicod:= Strvalue [1] + '000';        {Ubrzaj sa Search}
  For I:= 0 To Dictionary^.Count- 1 Do Begin
    Bb:= Dictionary^.At (I);

    If Rtrim (Bb^.Dictvalue) = Rtrim (Strvalue) Then Begin
      Inicod:=BB^.Key;
      putcode:=inicod;
      Exit;
    End;
    If (Strvalue [1] = Bb^.Key [1] ) And (Bb^.Key> Inicod) Then
      Inicod:= Bb^.Key
  End;
  Gotovo:= False;
  I:= 4;
  Repeat
    Inc (Inicod [I] );
    If Inicod [I] > '9' Then Inicod [I] := '0' Else Gotovo:= True;
    I:= I- 1;
  Until Gotovo;
  New (Bb);
  Bb^.Dictvalue:= Strvalue;
  Bb^.Key:= Inicod;
  Dictionary^.Insert (Bb);

  if not(Initdbf (Addition)) then exit;
  Fillchar(Addbuf,sizeof(Addbuf),' ');
  AddReplace (1, Dictionary^.Slovo+ Inicod+ Strvalue);
  Adddbrec(Addition,addbuf);
  bb^.pozicija:=addition.reccount;
  putcode:=inicod;
end;
procedure specbasewarajevo;
var tmp,dir,vollab,d,n,e:string;
  elem:^String;
  from,whpath,code:integer;
  ff:PFileItem;
  dlg:PFileDIalog;
  fname:string;
  c,i,j:word;
var datoteka:Text;
cc:Boolean;
procedure we;
begin
  closedfile(specbase);
  closedfile(baza);
  reindex;
end;
begin
 if messagebox('Convert database?',
  nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;
  hourglascur;
  if not(initdbf(specbase)) then begin
    DisplayError('No SPECBASE files',0);
    exit;
  end;
  opendfile(baza);
  opendfile(specbase);

  For I:= 1 To SpecBase.Reccount Do Begin
    Getdbrec (specbase,i,spbase);
    Fillchar(softbuf,sizeof(softbuf),' ');
    with softbuf do with spbase do begin
      for j:=1 to 10 do skraceno[j]:=filename[j];
      for j:=1 to 10 do traka[j]:=filename[j];
      for j:=1 to 30 do punoime[j]:=full_name[j];
      for j:=1 to 3 do kompjuter[j]:=machine[j];
      for j:=1 to 4 do godiste[j]:=year[j];
      tmp:='';
      for j:=1 to 30 do tmp:=tmp+developer[j];
      bazareplace(3,putcode(proizvodjaci,tmp));
      tmp:='';
      for j:=1 to 15 do tmp:=tmp+language[j];
      bazareplace(7,putcode(jezici,tmp));
      tmp:='';
      for j:=1 to 15 do tmp:=tmp+category[j];
      bazareplace(4,putcode(tipovi,tmp));
      Dir:='';
      for j:=1 to 60 do dir:=dir+path[j];
      Vollab:='NONAME';
      tmp:='';
      for j:=1 to 12 do tmp:=tmp+filename[j];
    end;
    New(FF);
    With FF^ do Begin
      fsplit(tmp,d,n,e);
      tip:='S';
      if e='.Z80' then Tip:='S';
      if e='.TAP' then Tip:='T';
      if e='.MDR' then Tip:='M';
      Filename:=n;
    End;
    Tapes^.Insert (FF);
    Fillchar(addbuf,sizeof(addbuf),' ');
    With ff^ Do Begin
      While Length (Filename) < 10 Do
        Filename:= Filename+ ' ';
      While Length (Dir) < 36 Do
        Dir:= Dir+ ' ';
      While Length (Vollab) < 11 Do
        Vollab:= Vollab+ ' ';
      AddReplace (1, Tip+ Filename+ Dir+ '('+ Vollab+ ')');
    End;
    Adddbrec(Addition,addbuf);
    ff^.pozicija:=addition.reccount;
    adddbrec(baza,softbuf);
  end;
  we;
end;


procedure warajevospecbase;
var
temp:string;
filepath,i,j,c:word;
Pozicija:Longint;
izlaz:text;
daljinar:PProcenter;
n:char;
dbt:File;
dOpis:array[1..512] of char;
cc:Boolean;
procedure we;
begin
  closedfile(baza);
  closedfile(specbase);
end;
begin
 if messagebox('Convert database?',
  nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;
  hourglascur;
  if not(fileexists(Specbase.dbfname)) then
     Zap(SpecBase,emptySpecbase,sizeof(emptyspecbase));
  if not(fileexists('SPECBASE.DBT')) then
     Zapdbt('SPECBASE.DBT',emptydbt,sizeof(emptydbt));

  if not(initdbf(specbase)) then begin
    DisplayError('No SPECBASE files',0);
    exit;
  end;
  opendfile(baza);
  opendfile(specbase);
  new(daljinar,init(16,10,52,1,baza.reccount));
  desktop^.insert(daljinar);

  For I:= 1 To Baza.Reccount Do Begin
    if (i mod 100)= 0 then daljinar^.update(i,'');
    Getdbrec (Baza,i,softbuf);
    fillchar(spbase,sizeof(spbase),' ');
    temp:=bazafield(2);
    for j:=1 to 30 do spbase.full_name[j]:=temp[j];
    temp:=proizvodjaci^.keyval(Bazafield(3),30);
    for j:=1 to 30 do spbase.developer[j]:=temp[j];
    temp:=tipovi^.keyval(bazafield(4),15);
    for j:=1 to 15 do spbase.category[j]:=temp[j]; {tip}
    temp:=jezici^.keyval(bazafield(7),15);
    for j:=1 to 15 do spbase.language[j]:=temp[j]; {language}
    temp:=bazafield(5);
    for j:=1 to 4 do spbase.year[j]:=temp[j]; {godina}
    temp:=bazafield(6);
    if temp[3]=' ' then temp:=' '+copy(temp,1,2);
    for j:=1 to 3 do  spbase.machine[j]:=temp[j]; {masina}
    temp:=copy(bazafield(8),1,8);
    temp:=temp+'.';
    filepath:=Tapes^.Finditem (Rtrim(BazaField (8)) );
    if filepath<>0 then begin
      n:=Pfileitem (tapes^. At (filepath) )^.tip;
      case n of
        'T': temp:=temp+'TAP';
        'S': temp:=temp+'Z80';
        'M': temp:=temp+'MDR';
      end;

    end;
    for j:=length(temp) to 12 do temp:=temp+' ';
    for j:=1 to 12 do spbase.filename[j]:=temp[j];
    if filepath<>0 then begin
      temp:=(Directory(Pfileitem (tapes^. At(filepath))));
      for j:=1 to length(temp) do spbase.path[j]:=temp[j];
    end;
    temp:='SPEC48    ';
    for j:=1 to 10 do spbase.emulator[j]:=temp[j];
    temp:='Z80    ';
    for j:=1 to 7 do spbase.size[j]:=temp[j];
    temp:='Spectrum file ';
    for j:=1 to 14 do spbase.file_type[j]:=temp[j];
    Val (Rtrim (BazaField (13) ), Pozicija, C);
    If Pozicija<> 0 Then Begin
      Assign (Dbt, dbtname);
      if not(fResetu (Dbt, 1)) then exit;
      if not(fSeek (Dbt, Pozicija* 512)) then begin we; exit end;
      if not(fBlockRead2 (Dbt, dOpis, 512,DummyBytes)) then begin we; exit end;
      cc:=fClose (Dbt);
      Assign(Dbt,'SPECBASE.DBT');
      if not(fResetu (Dbt, 1)) then exit;
      if not(fseek(dbt,filesize(dbt))) then begin we; exit; end;
      if not(fBlockwrite (Dbt, dOpis, 512)) then begin we; exit end;
      Pozicija:=filesize(dbt) div 512;
      if not(fSeek (Dbt, 0)) then begin we; exit end;
      if not(fBlockWrite (Dbt, Pozicija, 4)) then begin we;exit end;
      cc:=fclose(dbt);
      str(pozicija-1,temp);
      for j:=1 to length(temp) do spbase.remarks[j]:=temp[j];
    End;

    adddbrec(specbase,spbase);
  end;
  desktop^.delete(daljinar);
  dispose(daljinar,done);

  closedfile(specbase);
  closedfile(baza);
end;

procedure sgdwara;
var sgdline,tmp,dir,vollab:string;
  Paths:PPaths;
  elem:^String;
  from,whpath,code:integer;
  ff:PFileItem;
  dlg:PFileDIalog;
  dlg1:PDialog;
  fname:string;
  c:word;
var datoteka:Text;
cc,pt:Boolean;
R:TRect;
Control:PView;
var
  ConvOpt : record
    Nosort : Word; {Checkboxes}
    NoProducer : Word; {Checkboxes}
    NoFileNames : Word; {Checkboxes}
    end;


procedure we;
begin
  cc:=fcloset(datoteka);
  dispose(Paths,done);
  reindex;
end;
procedure we1;
begin
  dispose(Paths,done);
  reindex;
end;
begin
R.Assign(10,2,46,12);
New(Dlg1, Init(R, 'Convert from SGD'));
dlg1^.helpctx:=hcSgdWara;
R.Assign(2,2,25,3);
Control := New(PCheckboxes, Init(R,
  NewSItem('Do not ~s~ort',Nil)));
Dlg1^.Insert(Control);

R.Assign(2,3,25,4);
Control := New(PCheckboxes, Init(R,
  NewSItem('Ignore ~p~roducers',Nil)));
Dlg1^.Insert(Control);

R.Assign(2,4,25,5);
Control := New(PCheckboxes, Init(R,
  NewSItem('Ignore ~f~ile names',Nil)));
Dlg1^.Insert(Control);

R.Assign(3,6,16,8);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg1^.Insert(Control);

R.Assign(19,6,31,8);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg1^.Insert(Control);

Dlg1^.SelectNext(False);
c:=desktop^.execview(dlg1);
dlg1^.getdata(convopt);
dispose(dlg1,done);
if c=cmCancel then exit;

  dlg:=new(PFileDIalog,Init('*.DAT','Select file name','~N~ame',
     fdOKButton,100));
  c:=desktop^.execview(dlg);
  hourglascur;
  if c<>cmCancel then begin
    dlg^.getfilename(fname);
    fname:=fexpand(fname);
    dispose(dlg,done);
  end
  else begin
    dispose(dlg,done);
    exit;
  end;
  pt:=fileexists('SGD.INI');
  if pt then begin
    assign(datoteka,'SGD.INI');
    if not(fresett(datoteka)) then exit;
    Paths:=New(PPaths,Init (50,5));
    while not eof(datoteka) do begin
      if not(freadln(datoteka,sgdline)) then begin;we; exit end;
      if Sgdline[1]<>'#' then if pos('GameDir',sgdline)>0 then begin
        from:=Pos('=',sgdline)+1;
        while sgdline[from]=' ' do inc(from);
        new(elem);
        elem^:=copy(sgdline,from,length(sgdline)-from+1);
        Paths^.Insert(Elem);
      end;
   end;
   if not(Fcloset(datoteka)) then begin we1; exit; end;
  end;
  assign(datoteka,fname);
  if not(fresett(datoteka)) then begin we1; exit; end;
  while not eof(datoteka) do begin
    if not(freadln(datoteka,sgdline)) then begin we; exit; end;
    Fillchar(softbuf,sizeof(softbuf),' ');
    softbuf.pozicija[3]:='0';
    softbuf.problemi[5]:='0';

    bazareplace(1,copy(sgdline,94,8));
    bazareplace(2,copy(sgdline,1,36));
    if convopt.noproducer=0 then
      bazareplace(3,putcode(proizvodjaci,copy(sgdline,43,34)));
    bazareplace(4,putcode(tipovi,copy(sgdline,107,7)));
    bazareplace(5,copy(sgdline,38,4));
    bazareplace(7,putcode(jezici,copy(sgdline,143,3)));

    tmp:=copy(sgdline,80,3);
    if tmp=' 48' then tmp:='48';
    bazareplace(6,tmp);
    bazareplace(8,copy(sgdline,94,8));
    tmp:='';
    if pos('K',copy(sgdline,88,5))>0 then tmp:=tmp+'/JK ';
    if pos('1',copy(sgdline,88,5))>0 then tmp:=tmp+'/JS ';
    if pos('2',copy(sgdline,88,5))>0 then tmp:=tmp+'/JB ';
    if pos('C',copy(sgdline,88,5))>0 then tmp:=tmp+'/JC ';
    bazareplace(12,tmp);
    if sgdline[129]='Y' then bazareplace(11,'1');
    tmp:=copy(sgdline,115,3);
    while tmp[1]=' ' do tmp:=copy(tmp,2,length(tmp)-1);
    val(tmp,whpath,code);
    if convopt.nofilenames=0 then begin
      if pt and (whpath<Paths^.Count) then dir:=PString(Paths^.At(whpath))^
        else dir:='C:\';
      vollab:='VOLUME'+copy(sgdline,131,4);
      New (FF);
      With FF^ do Begin
        tip:='O';
        if copy(sgdline,103,3)='Z80' then Tip:='S';
        if copy(sgdline,103,3)='TAP' then Tip:='T';
        if copy(sgdline,103,3)='MDR' then Tip:='M';
        Filename:=copy(sgdline,94,8);
        if tip='O' then Dir:=dir+RTrim(FileName)+copy(sgdline,102,4);
      End;
      Tapes^.Insert (FF);
      Fillchar(addbuf,sizeof(addbuf),' ');
      With ff^ Do Begin
        While Length (Filename) < 10 Do
          Filename:= Filename+ ' ';
        While Length (Dir) < 36 Do
          Dir:= Dir+ ' ';
        While Length (Vollab) < 11 Do
          Vollab:= Vollab+ ' ';
        AddReplace (1, Tip+ Filename+ Dir+ '('+ Vollab+ ')');
      End;
      Adddbrec(Addition,addbuf);
      ff^.pozicija:=addition.reccount;
     end;
     adddbrec(baza,softbuf);
  end;
  if not(fcloset(datoteka)) then begin we; exit; end;
  if pt then dispose(Paths,done);
  if convopt.nosort<>0 then indexedfields:='';
  reindex;
end;
procedure specpicrainWara(rainbow:Boolean);
var fglavna,ftipovi,fproizvodjaci,fpathovi,fvolumesi,
    fjezici,fnames,fstartupi:File;
    sglavna:TMain;
    stipovi:TProgtype;
    sproizvodjaci:TProducer;
    spathovi:TProgPath;
    svolumesi:TVolumes;
    sstartupi:TStartups;
    snames:TProgname;
    sjezici:TCountry;
    tmp,d,n,e:String;
    i:Integer;
    tp:Char;
    ff:PFileItem;
procedure we;
begin
    Close(ftipovi); close(fproizvodjaci);
    close(fpathovi);close(fjezici);close(fnames);
    if rainbow then begin close(fstartupi); close(fvolumesi); end;
    reindex;
end;
begin
 if messagebox('Convert database?',
  nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;

  hourglascur;
  Assign(fglavna,'DATABASE.TBL');
  Assign(ftipovi,'PROGTYPE.TBL');
  Assign(fproizvodjaci,'PRODUCER.TBL');
  Assign(fpathovi,'PROGPATH.TBL');
  if rainbow then Assign(fvolumesi,'VOLUMES.TBL');
  Assign(fjezici,'COUNTRY.TBL');
  Assign(fnames,'PROGNAME.TBL');
  if rainbow then Assign(fstartupi,'STARTUP.TBL');
  if not(fresetu(fglavna,1)) then exit;
  if not(fresetu(ftipovi,1)) then begin close(fglavna);exit; end;
  if not(fresetu(fproizvodjaci,1)) then begin
    close(fglavna); Close(ftipovi) ;
    exit;
  end;
  if not(fresetu(fpathovi,1)) then begin
   close(fglavna);
   Close(ftipovi); close(fproizvodjaci);
    exit;
  end;
  if rainbow then if not(fresetu(fvolumesi,1)) then begin close(fglavna);
   Close(ftipovi); close(fproizvodjaci);close(fpathovi);
  exit; end;
  if not(fresetu(fjezici,1)) then begin close(fglavna);
    Close(ftipovi); close(fproizvodjaci);close(fpathovi);
    if rainbow then close(fvolumesi);
    exit;
  end;
  if not(fresetu(fnames,1)) then begin
    Close(ftipovi); close(fproizvodjaci);
    close(fpathovi); close(fjezici);
    if rainbow then close(fvolumesi);
    exit;
  end;

  if rainbow then if not(fresetu(fstartupi,1)) then begin
    Close(ftipovi); close(fproizvodjaci);Close(fnames);
    close(fpathovi);close(fvolumesi);close(fjezici);
    exit;
  end;
  while not eof(fglavna) do begin
    fillchar(softbuf,sizeof(softbuf),' ');
    if not(fblockread(fglavna,sglavna,sizeof(sglavna))) then begin we; exit; end;
    {bazareplace(1,copy(sglavna.filename,1,8));}
    if not(fseek(fnames,sizeof(snames)*(sglavna.prgname-1))) then tmp:=''
     else begin
       if not(fblockread(fnames,snames,sizeof(snames)))then begin we;exit;end;
       tmp:=snames.value;
     end;
    bazareplace(2,tmp);
    tmp:=rtrim(copy(tmp,1,8));
    for i:=1 to length(tmp) do begin tmp[i]:=Upcase(tmp[i]);
      if tmp[i]=' ' then tmp[i]:='_';
    end;
    bazareplace(1,tmp);
    if sglavna.producer=0 then tmp:='' else
    if not(fseek(fproizvodjaci,sizeof(sproizvodjaci)*(sglavna.producer-1))) then tmp:=''
     else begin
       if not(fblockread(fproizvodjaci,sproizvodjaci,sizeof(sproizvodjaci)))
         then tmp:='' else tmp:=sproizvodjaci.value;
     end;
     bazareplace(3,putcode(proizvodjaci,tmp));
    if sglavna.progtype=0 then tmp:='' else
    if not(fseek(ftipovi,sizeof(stipovi)*
      (sglavna.progtype-1))) then tmp:=''
     else begin
       if not(fblockread(ftipovi,stipovi,sizeof(stipovi)))
         then tmp:='' else tmp:=stipovi.value;
     end;
     bazareplace(4,putcode(tipovi,tmp));
     str(sglavna.year:4,tmp);
     bazareplace(5,tmp);
     if sglavna.memoryrequest=0 then tmp:='48  ' else tmp:='128 ';
     bazareplace(6,tmp);
    if sglavna.language=0 then tmp:='' else
    if not(fseek(fjezici,sizeof(sjezici)*
      (sglavna.language-1))) then tmp:=''
     else begin
       if not(fblockread(fjezici,sjezici,sizeof(sjezici)))
         then tmp:='' else tmp:=sjezici.value;
     end;
     bazareplace(7,putcode(jezici,tmp));
     if rainbow then str(sglavna.block:3,tmp) else tmp:='0';
     bazareplace(9,tmp);
     if rainbow then begin str(sglavna.problems:5,tmp); bazareplace(11,tmp) end;
     if rainbow then begin
       if sglavna.startup=0 then tmp:='' else
       if not(fseek(fstartupi,sizeof(sstartupi)*
         (sglavna.startup-1))) then tmp:=''
        else begin
          if not(fblockread(fstartupi,sstartupi,sizeof(sstartupi)))
            then tmp:='' else tmp:=sstartupi.value;
        end;
        bazareplace(12,tmp);
      end;
      tmp:=sglavna.filename;
      fsplit(tmp,d,n,e);
      bazareplace(8,n);
      while length(n)<10 do n:=n+' ';
      if sglavna.filepath=0 then tmp:='' else
      if not(fseek(fpathovi,sizeof(spathovi)*
        (sglavna.filepath-1))) then tmp:=''
       else begin
         if not(fblockread(fpathovi,spathovi,sizeof(spathovi)))
           then tmp:='' else tmp:=spathovi.value;
       end;
       d:=copy(tmp,1,36);
       While Length (D) < 36 Do
         D:= D+' ';

       tp:='O';
       if e='.Z80' then tp:='S';
       if (e='.TAP') and ((sglavna.emulator=$45) or (sglavna.emulator=$85))
        then tp:='T';
       if e='.MDR' then tp:='M';
       if rainbow then begin
         if sglavna.volume=0 then tmp:='' else
         if not(fseek(fpathovi,sizeof(svolumesi)*
           (sglavna.volume-1))) then tmp:=''
         else begin
           if not(fblockread(fvolumesi,svolumesi,sizeof(svolumesi)))
             then tmp:='' else tmp:=svolumesi.value;
         end;
       end
        else tmp:='';
       tmp:=copy(tmp,1,11);
       while length(tmp)<11 do tmp:=tmp+' ';
       New(ff);
       Tapes^.Insert (FF);
       Fillchar(addbuf,sizeof(addbuf),' ');
       With ff^ Do Begin
           tip:=tp;
           Filename:= n;
         AddReplace (1, Tip+ Filename+ D+ '('+ tmp+ ')');
       End;
       Adddbrec(Addition,addbuf);
       ff^.pozicija:=Addition.reccount;
       adddbrec(baza,softbuf);
    end;
  we;

end;



End.


