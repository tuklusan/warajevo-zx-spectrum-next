


{$V-,O+,F+}
unit snapshot;
interface
uses objects,VIEWS,drivers;
type z80snaptype=record
   z80a: byte;
   z80f: byte;
   z80bc:word;
   z80hl:word;
   z80oldpc:word;
   z80sp:word;
   z80i:byte;
   z80r:byte;
   z80stat1:byte;
   z80de:word;
   z80bcp:word;
   z80dep:word;
   z80hlp:word;
   z80ap:byte;
   z80fp:byte;
   z80iy:word;
   z80ix:word;
   z80iff1:byte;
   z80iff2:byte;
   z80stat2:byte;

   z80addhdr:word;
   z80pc:word;
   z80hrdmode:byte;
   z80mmu:byte;
   z80if1paged:byte;
   z80rldir:byte;
   z80fffd:byte;

   z80sndreg:array [0..15] of byte;

   z80Tlow:word;
   z80Thi:byte;
   z80spectator:byte;
   z80MGT:byte;
   z80multiface:byte;
   z80lowRAM:byte;
   z80midRAM:byte;
   z80userjoy:array[1..10] of byte;
   z80userjoyasc:array[1..10] of byte;
   z80MGTtype:byte;
   z80discbutt:byte;
   z80inhiflag:byte

  end;

  z80pagehdrtype=record
    datalength:word;
    pagenumber:byte
  end;

  snaspanvgashdrtype=record
    case boolean of
    true: (
      snai: byte;
      snahlp, snadep, snabcp: word;
      snafp, snaap: byte;
      snahl, snade, snabc, snaiy, snaix: word;
      snaiff2: byte;
      snar: byte;
      snaf, snaa: byte;
      snasp: word;
      snaimode: byte;
      snaborder: byte;
    );
    false: (
      spabc:word; spade:word; spahl:word;
      spaf:byte; spaa:byte; spaix:word;
      spaiy:word; spabcp:word; spadep:word;
      spahlp:word; spafp:byte; spaap:byte;
      spar:byte; spai:byte;spasp:word; spapc:word;
      spanot1:word; spaborder:byte;
      spainfc:byte; {IN F,(C)}
      spastatus: byte; {bits 0-7:IFF1,IM 2, xx, LD AH,IRQ,fle{ faza,xx,ESC key}
      spanot2:byte;
    );
  end;
  prghdrtype=record
    prgname:array[1..10] of char;
    prgnulbyte:byte;
    prgsig61,prgsig35,prgsig03:byte; {Signatura}
    prgempty:array[1..206] of byte; {Prazno}
    prgiy,prgix,prgdep,prgbcp,prghlp:word;
    prgfp,prgap:byte;
    prgde,prgbc,prghl:word;
    prgiff2:byte;
    prgi:byte;
    prgsp:word;
    prgempty2:array[1..14] of byte;
  end;
  snptailtype=record {Garabik}
    snpf,snpa:byte;
    snpres,snpbc,snpde,snphl,
    snppc,snpsp,snpix,snpiy:word;
    snpiff2,snpiff1,snpim,snpr,snpi:byte;
    snpfp,snpap:byte;
    snpbcp,snpdep,snphlp:word;
  end;
  sithdrtype=record {Spectrum 2.00}
    sitbc,sitde,sithl:word;
    sitf,sita:byte;
    sitix,sitiy,sitsp,sitpc:word;
    sitr,siti:byte;
    sitbcp,sitdep,sithlp:word;
    sitfp,sitap:byte;
    sitinter,sitborder:byte;
  end;
  semtailtype=record {SPECEMUG}
    sema,semf:byte;
    sembc,semde,semhl:word;
    semap,semfp:byte;
    sembcp,semdep,semhlp:word;
    sempc,semsp,semix,semiy:word;
    semi,semr,semiff1,semiff2,semim:word;
  end;
  riffchunktype=record {ZX32}
    riffa,rifff:byte;
    riffbc,riffde,riffhl:word;
    riffap,rifffp:byte;
    riffbcp,riffdep,riffhlp:word;
    riffix,riffiy,riffpc,riffsp:word;
    riffi,riffr,riffiff1,riffiff2,riffim:byte;
    dwticks:longint;
    riffunk:byte;
  end;
  memorija1=array[0..65519] of byte;
  memorija2=array[16384..65535] of byte;

const snapemul:integer=0;
  scrwinvisible:Boolean=false;
type TSnapshotfile=object(TObject)
   snapname:string;
   allowed:Boolean;
   z80pagehdr:z80pagehdrtype;
   uncompressed:^memorija2;
   compressed:^memorija1;
   snaphdr:z80snaptype;
   z80snptype:integer; {0 nepoznat,1 1.45 nekompr,2 1.45 kompr,3 2.0,4 3.0}
   procedure setregs;
   procedure getheader;
   procedure saveheader;
   procedure load48k;
   procedure load128kpage(pn:integer);
   procedure savehdrand48K(hsize:integer);
   procedure sethrdw;
   procedure decompsnap(bx,max:word; var di: word);
   procedure compsnap(bx,max:word; var di:word);
   constructor init(snpnme:string;mp1,mp2:Pointer);
   function select:Boolean;
   procedure unselect;
   procedure srename;
   function is48Ksnap:Boolean;
   procedure editsnap;
   procedure warning48;
   procedure z80tosna;
   procedure z80toSpanishVGASP(tip:integer);
   procedure z80toSpanish;
   procedure z80toVGASPEC;
   procedure z80toprg;
   procedure z80tosna128;
   procedure sna128toz80;
   procedure Z80toSEM;
   procedure SNAVGASPAtoZ80(tip:integer);
   procedure SNAtoZ80;
   procedure SNPtoZ80;
   procedure Z80toSNP;
   procedure SITtoZ80;
   procedure Z80toSIT;
   procedure SLTtoZ80TAP;
   procedure SPanishtoZ80;
   procedure VGASpecToZ80;
   procedure PRGtoZ80;
   procedure SEMtoZ80;
   procedure RIFFtoZ80;
   procedure z80toRIFF;
   procedure Z80anytoZ8048K;
   procedure Z80anytoZ80128K;
   procedure Z80TAPtoSLT;
   function  IsTimex:Boolean;

   procedure View;
   procedure snapshottotap;
end;
PSnapshotFile=^TSnapshotFile;
type tslika=array[0..6911] of byte;
type tscrwindow=object(TWindow)
  slika:^tslika;
  constructor Init(var Bounds: TRect; ATitle: TTitleStr);
  procedure draw;virtual;
  procedure handleevent(var Event:TEvent);virtual;
  destructor done;virtual;

end;
pscrwindow=^tscrwindow;

procedure Clive;
var picprozor:pscrwindow;

implementation
uses hexedito,app,imprek,helpctx,dialogs,tapes,
  iorutine,stddlg,dos,konvtap,msgbox,moucur,memory,menucons;
const prazna:array[1..12] of byte=(4,0,0,0,0,0,0,0,255,255,255,255);
procedure DrawClive(var CeloPas);external;
{$L ENVCELO.OBJ}
constructor TscrWindow.Init(var Bounds: TRect; ATitle: TTitleStr);
var i:integer;
d:file;
begin
  scrwinvisible:=true;
{  TDialog.Init(Bounds, ATitle);}
  TWindow.Init(Bounds,Atitle,2);
  Flags:=flags and not(wfGrow) and not(wfZoom);
  new(slika);
  for i:=0 to 6143 do slika^[i]:=0;
  for i:=6144 to 6911 do slika^[i]:=56;
end;
procedure Clive;
begin
  DrawClive(picprozor^.slika^)
end;
destructor TScrWindow.done;
begin
{  Tdialog.Done;}
  TWindow.Done;
  dispose(slika);
  scrwinvisible:=false;
end;
procedure TScrWindow.HandleEvent(var Event:TEvent);
begin
  if (Event.Double) and (Event.What=EvMouseUp) then begin
    ClearEvent(Event);
    Event.What:=evCommand;
    Event.Command:=cm_Emulator;
    PutEvent(Event)
  end
   else  TWindow.HandleEvent(Event);
end;
procedure TScrWindow.Draw;
var colbuf:array[0..65] of byte;
  i,j,k,ofs:integer;
  adrtop,adrbot:word;
  value,paper,ink,swapper:byte;
  P:Pview;
const
topchars:array[1..12] of word=
  (0,64,128,192,2048,2112,2176,2240,4096,4160,4224,4288);
bottomchars:array[1..12] of word=
  (32,96,160,224,2080,2144,2208,2272,4128,4192,4256,4320);
topattr:array[1..12] of word=
  (6144,6208,6272,6336,6400,6464,6528,6592,6656,6720,6784,6848);
botattr:array[1..12] of word=
  (6176,6240,6304,6368,6432,6496,6560,6624,6688,6752,6816,6880);
colorset:array[0..7] of byte = (0,1,4,5,2,3,14,7);
begin
  colbuf[0]:=0; colbuf[1]:=255;
  TWindow.Draw;

  for j:=1 to 12 do begin
    adrtop:=topchars[j];
    adrbot:=bottomchars[j];

    for i:=1 to 32 do begin
      value:=slika^[topattr[j]+i-1];
      paper:=(value shr 3) and 7;
      ink:=value and 7;
      paper:=colorset[paper];
      ink:=colorset[ink];
      if value >=128 then begin
        swapper:=paper;
        paper:=ink;
        ink:=swapper;
      end;
      for k:=0 to 7 do begin
        ofs:=2+(k shl 2);
        value:=slika^[adrtop];
        asm
        @ATTRADJ0:
          mov bh,paper
          mov bl,ink
          mov cx,4
        @ATTRLOOP0:
          mov ah,value
          shr bh,1
          jc @INKSET0
          shr bl,1
          jc @IZLAZ0
          xor ah,ah
          jmp @IZLAZ0
       @INKSET0:
          shr bl,1
          jc @PAPSET0
          not ah
          jmp @IZLAZ0
       @PAPSET0:
          mov ah,255
       @IZLAZ0:
          push ax
          loop @attrloop0
          pop ax
          mov value,ah
        end;
        colbuf[ofs+3]:=value;
        asm
          pop ax
          mov value,ah
        end;
        colbuf[ofs+2]:=value;
        asm
          pop ax
          mov value,ah
        end;
        colbuf[ofs+1]:=value;
        asm
          pop ax
          mov value,ah
        end;

        colbuf[ofs]:=value;
        inc(adrtop,256);
      end;
      dec(adrtop,2047);
      value:=slika^[botattr[j]+i-1];
      paper:=(value shr 3) and 7;
      ink:=value and 7;
      paper:=colorset[paper];
      ink:=colorset[ink];
      if value >=128 then begin
        swapper:=paper;
        paper:=ink;
        ink:=swapper;
      end;

      for k:=8 to 15 do begin
        ofs:=2+(k shl 2);
        value:=slika^[adrbot];
        asm
        @ATTRADJ1:
          mov bh,paper
          mov bl,ink
          mov cx,4
        @ATTRLOOP1:
          mov ah,value
          shr bh,1
          jc @INKSET1
          shr bl,1
          jc @IZLAZ1
          xor ah,ah
          jmp @IZLAZ1
       @INKSET1:
          shr bl,1
          jc @PAPSET1
          not ah
          jmp @IZLAZ1
       @PAPSET1:
          mov ah,255
       @IZLAZ1:
          push ax
          loop @attrloop1
          pop ax
          mov value,ah
        end;
        colbuf[ofs+3]:=value;
        asm
          pop ax
          mov value,ah
        end;
        colbuf[ofs+2]:=value;
        asm
          pop ax
          mov value,ah
        end;
        colbuf[ofs+1]:=value;
        asm
          pop ax
          mov value,ah
        end;

        colbuf[ofs]:=value;
        inc(adrbot,256);

      end;
      dec(adrbot,2047);
       writeline (i,j,1,1,colbuf);
    end;
  end;
end;


constructor TSnapshotFile.init(snpnme:string;mp1,mp2:pointer);
begin
  snapname:='';
  if snpnme='' then allowed:=false
    else begin
      allowed:=true;
      snapname:=snpnme;
      getheader;
    end;
  uncompressed:=mp1;
  compressed:=mp2;

end;

procedure TSnapshotFile.getheader;
var cc:Boolean;
  datoteka:file;
  procedure we;
  begin
    cc:=fclose(datoteka);
    z80snptype:=0;
    allowed:=false;
  end;
begin
  hourglascur;
  with snaphdr do begin
    assign(datoteka,snapname);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    if filesize(datoteka)<30 then begin we; exit end;
    if not(fblockread(datoteka,snaphdr,30)) then begin we; exit end;
    if z80oldpc<>0 then begin
      if (z80stat1 and $20)=0 then z80snptype:=1 else z80snptype:=2;
      z80pc:=z80oldpc;
      cc:=fclose(datoteka);
      exit
    end;

    if not(fresetu(datoteka,1)) then begin we; exit end;
    if filesize(datoteka)<55 then begin we; exit end;
    if not(fblockread(datoteka,snaphdr,55)) then begin we; exit end;
    if z80addhdr=23 then begin
      z80snptype:=3;
      cc:=fclose(datoteka);
      exit
    end;

    if not(fresetu(datoteka,1)) then begin we; exit end;
    if filesize(datoteka)<86 then begin we; exit end;
    if not(fblockread(datoteka,snaphdr,86)) then begin we; exit end;
    if z80addhdr=54 then begin
      z80snptype:=4;
      cc:=fclose(datoteka);
      exit
    end;
    we
  end
end;

function TSnapshotFile.is48Ksnap:  boolean;
begin
  is48Ksnap:= ((z80snptype=3) and (snaphdr.z80hrdmode<=2)) or
          ((z80snptype=4) and (snaphdr.z80hrdmode<=3)) or (z80snptype<3)
end;
function TSnapshotFile.isTimex:Boolean;
begin
  IsTimex:=snaphdr.Z80hrdmode=128;
end;
procedure TSnapshotFile.saveheader;
var izbor:integer;
var cc:Boolean;
  datoteka:file;
  procedure we;
  begin
    cc:=fclose(datoteka);
    z80snptype:=0;
    allowed:=false;
  end;

begin
  hourglascur;
  assign(datoteka,snapname);
  if fileexists(snapname) then begin
    if not(fresetu(datoteka,1)) then begin we; exit; end;
  end
   else begin
    if not(fresetu(datoteka,1)) then begin we; exit; end;
   end;
  case z80snptype of
    0: begin
         displayerror('Header not written. Type incompatible',0);
         we; exit
       end;
    1,2: begin
       if not(fblockwrite(datoteka,snaphdr,30)) then begin we; exit end;
       end;
      3: begin
       if not(fblockwrite(datoteka,snaphdr,55)) then begin we; exit end;
       end;
      4: begin
       if not(fblockwrite(datoteka,snaphdr,86)) then begin we; exit end;
       end;
    end;
   cc:=fclose(datoteka);
end;

procedure TSnapshotFile.load48K; {Stranica na 49152-65535}
var hdsize,di,i:word;
  datoteka:file;
  cc:boolean;
  procedure we;
  begin
    cc:=fclose(datoteka);
    z80snptype:=0;
    allowed:=false;
  end;

begin
  hourglascur;
  for i:=16384 to 65519 do compressed^[i]:=0;
  case z80snptype of
    0: exit;
    1: hdsize:=30;
    2: hdsize:=30;
    3: hdsize:=55;
    4: hdsize:=86
  end;
  assign(datoteka,snapname);
  if not(fresetu(datoteka,1)) then begin we; exit end;
  if not(fblockread(datoteka,compressed^,hdsize)) then begin we; exit end;
  case z80snptype of
    1: begin
      if not(fblockread(datoteka,uncompressed^,49152)) then begin we; exit end;
      cc:=fclose(datoteka)
    end;
    2: begin
      if not(fblockread(datoteka,compressed^,filesize(datoteka)-30)) then begin
        we; exit
      end;
      di:=16384;
      decompsnap(0,filesize(datoteka)-30-1,di);
      cc:=fclose(datoteka);
    end;
    3,4: begin

      while not eof(datoteka) do begin
        if not(fblockread(datoteka,z80pagehdr,3)) then begin we; exit end;
        if not(fblockread(datoteka,compressed^,
         z80pagehdr.datalength)) then begin
          we; exit
        end;
        if not(is48Ksnap) then begin
          if z80pagehdr.pagenumber=5 then z80pagehdr.pagenumber:=4;
          if (z80pagehdr.pagenumber)=3+snaphdr.z80mmu and 7 then
            z80pagehdr.pagenumber:=5;
        end;
        case z80pagehdr.pagenumber of
          4: begin di:=$8000; decompsnap(0,z80pagehdr.datalength-1,di);end;
          8: begin di:=$4000; decompsnap(0,z80pagehdr.datalength-1,di);end;
          5: begin di:=$C000; decompsnap(0,z80pagehdr.datalength-1,di);end;

        end;

      end;
      if not(fclose(datoteka)) then begin we; exit; end;
    end;
  end;
end;


procedure TSnapshotFile.load128Kpage(pn:integer); {Stranica na 49152-65535}
var hdsize,di,i:word;
  datoteka:file;
  cc:boolean;
  procedure we;
  begin
    cc:=fclose(datoteka);
    z80snptype:=0;
    allowed:=false;
  end;

begin
  hourglascur;
  for i:=16384 to 65519 do compressed^[i]:=0;
  case z80snptype of
    0,1,2: exit;
    3: hdsize:=55;
    4: hdsize:=86
  end;
  assign(datoteka,snapname);
  if not(fresetu(datoteka,1)) then begin we; exit end;
  if not(fblockread(datoteka,compressed^,hdsize)) then begin we; exit end;
  while not eof(datoteka) do begin
    if not(fblockread(datoteka,z80pagehdr,3)) then begin we; exit end;
    if not(fblockread(datoteka,compressed^,
     z80pagehdr.datalength)) then begin
      we; exit
    end;
    if not(is48ksnap) then begin
      if z80pagehdr.pagenumber=pn then begin
        di:=$C000;
        decompsnap(0,z80pagehdr.datalength-1,di)
      end
    end;
  end;
  if not(fclose(datoteka)) then begin we; exit end;
end;


procedure TSnapshotFIle.savehdrand48K(hsize:integer);
var datoteka:file;
  cc:boolean;
  di:word;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
    allowed:=false;
  end;

begin
  hourglascur;
  assign(datoteka,snapname);
  if not(frewriteu(datoteka,1)) then begin we; exit end;
  if not(fblockwrite(datoteka,snaphdr,hsize)) then begin we; exit end;

  di:=0;
  compsnap(32768,49151,di);
  z80pagehdr.datalength:=di;
  z80pagehdr.pagenumber:=4;
  if not(fblockwrite(datoteka, z80pagehdr,3)) then begin we; exit end;
  if not(fblockwrite(datoteka,compressed^,di)) then begin we; exit end;
  di:=0;
  compsnap(49152,65535,di);
  z80pagehdr.datalength:=di;
  z80pagehdr.pagenumber:=5;
  if not(fblockwrite(datoteka, z80pagehdr,3)) then begin we; exit end;
  if not(fblockwrite(datoteka,compressed^,di)) then begin we; exit end;
  di:=0;
  compsnap(16384,32767,di);
  z80pagehdr.datalength:=di;
  z80pagehdr.pagenumber:=8;
  if not(fblockwrite(datoteka, z80pagehdr,3)) then begin we; exit end;
  if not(fblockwrite(datoteka,compressed^,di)) then begin we; exit end;
  cc:=fclose(datoteka)
end;

{
********************************************
    Opcija Select snap
********************************************
}

function TSnapshotFile.select:Boolean;
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
  a:=new(PFileDIalog,Init('*.Z80','Select snapshot name','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcselect;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(fname);
    fname:=fexpand(fname);
    if not(fileexists(fname)) then begin
      displayerror('Snapshot file does not exist',0);
      we; exit;
    end;
    snapname:=fname;
    allowed:=true;
    getheader;
    if is48ksnap then snapemul:=0 else snapemul:=1;
    select:=true;
    if scrwinvisible then begin
      getheader;
      load48k;
      move(uncompressed^[16384],picprozor^.slika^[0],6912);
      picprozor^.drawview;
    end;


  end
   else select:=false;
  dispose(a,done);
end;

procedure TSnapshotfile.unselect;
var d,n,e:string;
  i:word;
begin
  hourglascur;
  snapname:='';
  allowed:=false;
    if scrwinvisible then begin
{      for i:=0 to 6143 do picprozor^.slika^[i]:=0;
      for i:=6144 to 6911 do picprozor^.slika^[i]:=56;}
      DrawClive(picprozor^.slika^);

      picprozor^.drawview;
    end
end;

procedure TSnapshotfile.srename;
var d,n,e,newfull:string;
  datoteka:file;
  NName:string[8];
  c:word;
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
Begin
  R.Assign(10,2,46,13);
  New(Dlg, Init(R, 'Rename snapshot'));
  Dlg^.helpctx:=hcsrename;

  R.Assign(7,3,17,4);
  Control := New(PInputLine, Init(R, 8));
  Dlg^.Insert(Control);

  R.Assign(6,2,24,3);
  Dlg^.Insert(New(PLabel, Init(R, '~N~ew snapshot name', Control)));

  R.Assign(3,7,15,9);
  Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(19,7,31,9);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  Dlg^.getdata(NName);
  dispose(dlg,done);
  if c=cmOK then begin
    fsplit(nname,d,n,e);
    nname:=n;
    fsplit(snapname,d,n,e);
    newfull:=d+Nname+e;
    assign(datoteka,snapname);

    if frename(datoteka,newfull) then begin
      snapname:=fexpand(newfull);
      getheader
    end;
  end
end;

procedure TsnapshotFile.compsnap(bx,max:word; var di:word);
var
  cx:word;
procedure repeating;
begin
  if (uncompressed^[bx-1]=237) or (cx>=5) then begin
    compressed^[di]:=$ED;
    di:=di+1;
    compressed^[di]:=$ED;
    di:=di+1;
    compressed^[di]:=lo(cx);
    di:=di+1;
    compressed^[di]:=uncompressed^[bx-1];
    di:=di+1;
  end
   else begin
     repeat
       compressed^[di]:=uncompressed^[bx-1];
       di:=di+1;
       cx:=cx-1;
     until cx=0;
   end
end;
begin
  hourglascur;
  max:=max+1;
  bx:=bx+1;
  repeat
    if uncompressed^[bx-1]=uncompressed^[bx] then begin
      cx:=1;
      repeat
        cx:=cx+1;
        bx:=bx+1;
        if bx=max then begin repeating; exit; end;
      until (cx=255) or (uncompressed^[bx-1]<>uncompressed^[bx]);
      repeating;
      bx:=bx+1;
    end
     else begin
       if uncompressed^[bx-1]=237 then begin
         compressed^[di]:=uncompressed^[bx-1];
         di:=di+1; bx:=bx+1;
         if bx=max then begin
           compressed^[di]:=uncompressed^[bx-1];
           di:=di+1;
           exit
         end
       end;
       compressed^[di]:=uncompressed^[bx-1];
       di:=di+1;
       bx:=bx+1;
     end
  until bx=max;
  compressed^[di]:=uncompressed^[bx-1];
  di:=di+1
end;

procedure TSnapshotFIle.decompsnap(bx,max:word; var di: word);
  procedure errdecomp;
  begin
    displayerror('Error in snapshot file',0);
  end;
var
  cx:word;
begin
  hourglascur;
  max:=max+1;
  repeat
    if compressed^[bx]=237 then begin
      bx:=bx+1;
      if bx=max then begin
        uncompressed^[di]:=compressed^[bx-1];
        di:=di+1;
        exit
      end;
      if compressed^[bx]=237 then begin
        bx:=bx+1;
        if bx=max then begin errdecomp; exit end;
        cx:=compressed^[bx];
        if cx=0 then begin
          if uncompressed^[di-1]=0 then di:=di-1 else errdecomp;
          exit
        end;
        bx:=bx+1;
        if bx=max then errdecomp;
        repeat
          uncompressed^[di]:=compressed^[bx];
          di:=di+1;
          cx:=cx-1;
        until cx=0
      end
       else begin
         uncompressed^[di]:=compressed^[bx-1];
         di:=di+1;
         uncompressed^[di]:=compressed^[bx];
         di:=di+1;
       end
    end
     else begin
       uncompressed^[di]:=compressed^[bx];
       di:=di+1;
     end;
    bx:=bx+1;
  until (bx=max) or (di=0);
end;
{ ********************************
  Postavljanje registara
  ********************************
}


type
PNumber16bit=^TNumber16bit;
PNumber8bithi=^TNumber8bithi;
PNumber8bitlo=^TNumber8bitlo;

Tnumber8bithi=object(TNumInputLine)
  linked16bit:PNumber16bit;
  constructor init(var bounds:Trect);
  procedure handleevent(var event:Tevent); virtual;
end;


Tnumber8bitlo=object(TNumInputLine)
  linked16bit:PNumber16bit;
  constructor init(var bounds:Trect);
  procedure handleevent(var event:Tevent); virtual;
end;



Tnumber16bit=object(TNumInputLine)
  highpart:PNumber8bithi;
  lopart:PNumber8bitlo;
  constructor init(var bounds:Trect);
  procedure handleevent(var event:Tevent); virtual;
end;

constructor TNumber16bit.init(var bounds:TRect);

begin
  TNumInputLine.init(bounds,5,0,65535);
end;

procedure TNumber16bit.handleevent(var event:Tevent);
  var  pod:record
     podlo:byte;
     podhi:byte;
     podignore:word;
  end;
  sethi,setlo,vr:Longint;
  code:integer;
begin
  TNumInputLine.handleevent(event);
  val(data^,vr,code);
  move(vr,pod,4);
  if highpart<>nil then begin
    sethi:=pod.podhi;
    highpart^.setdata(sethi);
    highpart^.draw;
  end;
  if lopart<>nil then begin
    setlo:=pod.podlo;
    lopart^.setdata(setlo);
    lopart^.draw;
  end;
end;

constructor TNumber8bithi.init(var bounds:TRect);

begin
  TNumInputLine.init(bounds,3,0,255);
end;

procedure TNumber8bithi.handleevent(var event:Tevent);
  var  pod:record
     podlo:byte;
     podhi:byte;
     podignore:word;
  end;
  vr:Longint;
  code:integer;
begin
  TNumInputLine.handleevent(event);
  val(data^,vr,code);
  move(vr,pod,4);
  if linked16bit<>nil then begin
    linked16bit^.getdata(pod);
    pod.podhi:=lo(vr);
    linked16bit^.setdata(pod);
  end;
end;


constructor TNumber8bitlo.init(var bounds:TRect);

begin
  TNumInputLine.init(bounds,3,0,255);
end;

procedure TNumber8bitlo.handleevent(var event:Tevent);
  var  pod:record
     podlo:byte;
     podhi:byte;
     podignore:word;
  end;
  vr:Longint;
  code:integer;
begin
  TNumInputLine.handleevent(event);
  val(data^,vr,code);
  move(vr,pod,4);
  if linked16bit<>nil then begin
    linked16bit^.getdata(pod);
    pod.podlo:=lo(vr);
    linked16bit^.setdata(pod);
  end;
end;

procedure link816(var a:pnumber16bit;var b:pNumber8bithi;
  var c:pNumber8bitlo);
begin
  a^.highpart:=b;
  a^.lopart:=c;
  b^.linked16bit:=a;
  c^.linked16bit:=a
end;


procedure TSnapshotfile.setregs;

type tdata=record
  ta,tf,taf,tb,tc,tbc,td,te,tde,th,tl,thl,txh,txl,tix,tyh,tyl,tiy:longint;
  tap,tfp,tafp,tbp,tcp,tbcp,tdp,tep,tdep,thp,tlp,thlp,tsp,tpc:longint;
  ti:longint;
  tiff1,tiff2,tim:word;
  tr:longint;
end;

var a:Pdialog;
  r,r1,r2:trect;
  Bruce:PVIew;
  p8hi:pnumber8bithi;
  p8lo:pnumber8bitlo;
  p16:PNumber16bit;
  c,i:word;
  vrijednosti:Tdata;
begin
  if not(allowed) then exit;
   getheader;
   with vrijednosti do with snaphdr do begin
      ta:=z80a;
      tb:=hi(z80bc);
      tc:=lo(z80bc);
      td:=hi(z80de);
      te:=lo(z80de);
      th:=hi(z80hl);
      tl:=lo(z80hl);
      tf:=z80f;
      ti:=z80i;
      tr:=z80r and $7F+128*(z80stat1 and 1);
      tap:=z80ap;
      tbp:=hi(z80bcp);
      tcp:=lo(z80bcp);
      tdp:=hi(z80dep);
      tep:=lo(z80dep);
      thp:=hi(z80hlp);
      tlp:=lo(z80hlp);
      tfp:=z80fp;
      tbc:=z80bc;
      tde:=z80de;
      thl:=z80hl;
      tbcp:=z80bcp;
      tdep:=z80dep;
      thlp:=z80hlp;
      tix:=z80ix;
      txh:=hi(z80ix);
      txl:=lo(z80ix);

      tiy:=z80iy;
      tyh:=hi(z80iy);
      tyl:=lo(z80iy);

      tsp:=z80sp;
      if z80snptype>2 then tpc:=z80pc else tpc:=z80oldpc;
      if z80iff1=0 then tiff1:=0 else tiff1:=1;
      if z80iff2=0 then tiff2:=0 else tiff2:=1;
      taf:=256*ta+tf;
      tafp:=256*tap+tfp;
      tim:=z80stat2 and 3;

  end;

  r.assign(2,1,78,20);
  a:=new(Pdialog,Init(r,'Processor registers'));
  a^.helpctx:=hcseditreg;
  r.assign(2,1,5,2);
  Bruce:=new(Plabel,init(r,'A=',nil));
  a^.insert(Bruce);

  r.assign(11,1,15,2);
  Bruce:=new(Plabel,init(r,'F=',nil));
  a^.insert(Bruce);

  r.assign(5,2,9,3);
  Bruce:=new(Plabel,init(r,'AF=',nil));
  a^.insert(Bruce);

  
  r.assign(6,1,11,2);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(P8HI);


  r.assign(16,1,21,2);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,2,17,3);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);
  link816(p16,p8hi,p8LO);

  r.assign(2,4,5,5);
  Bruce:=new(Plabel,init(r,'B=',nil));
  a^.insert(Bruce);

  r.assign(11,4,15,5);
  Bruce:=new(Plabel,init(r,'C=',nil));
  a^.insert(Bruce);

  r.assign(5,5,9,6);
  Bruce:=new(Plabel,init(r,'BC=',nil));
  a^.insert(Bruce);

  
  r.assign(6,4,11,5);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(p8hi);

  r.assign(16,4,21,5);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,5,17,6);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);
  link816(p16,p8hi,p8LO);



  r.assign(2,7,5,8);
  Bruce:=new(Plabel,init(r,'D=',nil));
  a^.insert(Bruce);


  r.assign(11,7,15,8);
  Bruce:=new(Plabel,init(r,'E=',nil));
  a^.insert(Bruce);

  r.assign(5,8,9,9);
  Bruce:=new(Plabel,init(r,'DE=',nil));
  a^.insert(Bruce);

  
  r.assign(6,7,11,8);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(p8hi);

  r.assign(16,7,21,8);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,8,17,9);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);
  link816(p16,p8hi,p8LO);



  r.assign(2,10,5,11);
  Bruce:=new(Plabel,init(r,'H=',nil));
  a^.insert(Bruce);

  r.assign(11,10,15,11);
  Bruce:=new(Plabel,init(r,'L=',nil));
  a^.insert(Bruce);
  p16^.setdata(vrijednosti.taf);

  r.assign(5,11,9,12);
  Bruce:=new(Plabel,init(r,'HL=',nil));
  a^.insert(Bruce);

  
  r.assign(6,10,11,11);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(p8hi);

  r.assign(16,10,21,11);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,11,17,12);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);
  link816(p16,p8hi,p8LO);



  r.assign(2,13,6,14);
  Bruce:=new(Plabel,init(r,'XH=',nil));
  a^.insert(Bruce);

  r.assign(11,13,15,14);
  Bruce:=new(Plabel,init(r,'XL=',nil));
  a^.insert(Bruce);

  r.assign(5,14,9,15);
  Bruce:=new(Plabel,init(r,'IX=',nil));
  a^.insert(Bruce);

  
  r.assign(6,13,11,14);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(p8hi);

  r.assign(16,13,21,14);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,14,17,15);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);


  r.assign(2,16,6,17);
  Bruce:=new(Plabel,init(r,'YH=',nil));
  a^.insert(Bruce);

  r.assign(11,16,15,17);
  Bruce:=new(Plabel,init(r,'YL=',nil));
  a^.insert(Bruce);

  r.assign(5,17,9,18);
  Bruce:=new(Plabel,init(r,'IY=',nil));
  a^.insert(Bruce);

  
  r.assign(6,16,11,17);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(p8hi);

  r.assign(16,16,21,17);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(10,17,17,18);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);


  r.assign(22,1,26,2);
  Bruce:=new(Plabel,init(r,'A''=',nil));
  a^.insert(Bruce);

  r.assign(31,1,35,2);
  Bruce:=new(Plabel,init(r,'F''=',nil));
  a^.insert(Bruce);

  r.assign(25,2,30,3);
  Bruce:=new(Plabel,init(r,'AF''=',nil));
  a^.insert(Bruce);

  
  r.assign(26,1,31,2);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(P8HI);

  r.assign(36,1,41,2);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(30,2,37,3);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);



  r.assign(22,4,26,5);
  Bruce:=new(Plabel,init(r,'B''=',nil));
  a^.insert(Bruce);

  r.assign(31,4,35,5);
  Bruce:=new(Plabel,init(r,'C''=',nil));
  a^.insert(Bruce);

  r.assign(25,5,30,6);
  Bruce:=new(Plabel,init(r,'BC''=',nil));
  a^.insert(Bruce);



  
  r.assign(26,4,31,5);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(P8HI);

  r.assign(36,4,41,5);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(30,5,37,6);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);



  r.assign(22,7,26,8);
  Bruce:=new(Plabel,init(r,'D''=',nil));
  a^.insert(Bruce);

  r.assign(31,7,35,8);
  Bruce:=new(Plabel,init(r,'E''=',nil));
  a^.insert(Bruce);

  r.assign(25,8,30,9);
  Bruce:=new(Plabel,init(r,'DE''=',nil));
  a^.insert(Bruce);


  
  r.assign(26,7,31,8);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(P8HI);

  r.assign(36,7,41,8);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(30,8,37,9);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);




  r.assign(22,10,26,11);
  Bruce:=new(Plabel,init(r,'H''=',nil));
  a^.insert(Bruce);

  r.assign(31,10,35,11);
  Bruce:=new(Plabel,init(r,'L''=',nil));
  a^.insert(Bruce);

  r.assign(25,11,30,12);
  Bruce:=new(Plabel,init(r,'HL''=',nil));
  a^.insert(Bruce);


  
  r.assign(26,10,31,11);
  p8hi:=new(PNumber8bithi,init(R));
  a^.insert(P8HI);

  r.assign(36,10,41,11);
  p8lo:=new(PNumber8bitlo,init(R));
  a^.insert(p8LO);

  r.assign(30,11,37,12);
  p16:=new(PNumber16bit,Init(R));
  a^.insert(p16);

  link816(p16,p8hi,p8LO);


  r.assign(25,14,30,15);
  Bruce:=new(Plabel,init(r,'SP=',nil));
  a^.insert(Bruce);

  r.assign(30,14,37,15);
  Bruce:=new(PNumInputLine,init(R,5,0,65535));
  a^.insert(bruce);

  r.assign(25,17,30,18);
  Bruce:=new(Plabel,init(r,'PC=',nil));
  a^.insert(Bruce);

  r.assign(30,17,37,18);
  Bruce:=new(PNumInputLine,init(R,5,0,65535));
  a^.insert(bruce);

  r.assign(41,1,44,2);
  Bruce:=new(Plabel,init(r,'I=',nil));
  a^.insert(Bruce);


  r.assign(45,1,50,2);
  Bruce:=new(PNumInputLine,init(R,3,0,255));
  a^.insert(bruce);

  r.assign(41,2,47,3);
  Bruce:=new(Plabel,init(r,'IFF1=',nil));
  a^.insert(Bruce);
  R.Assign(48,2, 55,4);
  Bruce := New(PRadioButtons, Init(R,
     NewSItem('DI',
     NewSItem('EI',
     nil))
  ));
  a^.Insert(Bruce);

  r.assign(55,2,61,3);
  Bruce:=new(Plabel,init(r,'IFF2=',nil));
  a^.insert(Bruce);

  R.Assign(62,2, 69,4);
  Bruce := New(PRadioButtons, Init(R,
     NewSItem('DI',
     NewSItem('EI',
     nil))
  ));
  a^.Insert(Bruce);

  r.assign(41,5,57,6);
  Bruce:=new(Plabel,init(r,'Interrupt mode=',nil));
  a^.insert(Bruce);

  R.Assign(58,5, 67,8);
  Bruce := New(PRadioButtons, Init(R,
     NewSItem('IM 0',
     NewSItem('IM 1',
     NewSItem('IM 2',
     nil)))
  ));
  a^.Insert(Bruce);


  r.assign(41,9,44,10);
  Bruce:=new(Plabel,init(r,'R=',nil));
  a^.insert(Bruce);

  r.assign(45,9,50,10);
  Bruce:=new(PNumInputLine,init(R,3,0,255));
  a^.insert(bruce);


   R.Assign(45,15,55,17);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(57,15,67,17);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   a^.setdata(vrijednosti);
   c:=desktop^.execview(a);
   hourglascur;
   if c=cmOK then begin
     a^.getdata(vrijednosti);
     with vrijednosti do with snaphdr do begin
        z80a:=ta;
        z80bc:=tbc;
        z80de:=tde;
        z80hl:=thl;
        z80ix:=tix;
        z80iy:=tiy;

        if z80snptype>2 then z80pc:=tpc else z80oldpc:=tpc;

        z80f:=tf;
        z80i:=ti;
        z80r:=tr;
        if (tr and 128)=0 then  z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80ap:=tap;
        z80bcp:=tbcp;
        z80dep:=tdep;
        z80hlp:=thlp;
        z80fp:=tfp;
        z80sp:=tsp;
        z80rldir:=z80rldir or 1;
        z80stat2:=z80stat2 and $FC or tim;
        if tiff1=0 then z80iff1:=0 else z80iff1:=4;
        if tiff2=0 then z80iff2:=0 else z80iff2:=4;

   end;
        saveheader;
   end;
  dispose(a,done);
end;

{
 *******************************
  Hardverska konfiguracija
 *******************************
}

procedure TSnapshotFile.sethrdw;
var a:PDialog;
  r:TRect;
  bruce:pview;
  c,i,izbor:word;
  is48K:Boolean;
  vrijednosti:record
    tmodel,tintpaged:word;
    tborder,tmmu,tlastsnd:longint;
    tsound:array[0..15] of longint;
    timcontr:longint;
  end;
begin
  if not allowed then exit;
  with snaphdr do with vrijednosti do begin
    if z80hrdmode in [1,2,4] then tmodel:=1 else tmodel:=0;
    if z80hrdmode=128 then tmodel:=2;
    tborder:=(z80stat1 div 2) and 7;
    tmmu:=z80mmu;
    tlastsnd:=z80fffd;
    if z80if1paged<>255 then tintpaged:=1 else tintpaged:=0;
    for i:=0 to 15 do tsound[i]:=z80sndreg[i];
    timcontr:=z80if1paged;
  end;
  with snaphdr do begin
    r.assign(2,1,78,20);
    a:=new(Pdialog,Init(r,'Hardware ports'));
    a^.helpctx:=hcsedithard;
    r.assign(2,1,25,2);
    Bruce:=new(Plabel,init(r,'Hardware configuration',nil));
    a^.insert(Bruce);
    if istimex then bruce^.hide;
    r.assign (3,2,27,4);
    if (is48ksnap) then begin
        Bruce := New(PRadioButtons, Init(R,
           NewSItem('48K',
           NewSItem('48K+Interface 1',
           nil))
        ));
        is48k:=true;
     end
      else begin
        Bruce := New(PRadioButtons, Init(R,
           NewSItem('128K',
           NewSItem('128K+Interface 1',
           nil))
        ));
        is48k:=false;
      end;
    a^.insert(Bruce);
    if istimex then bruce^.hide;
    r.assign(2,5,15,6);
    Bruce:=new(Plabel,init(r,'Interface 1',nil));
    a^.insert(Bruce);
    if istimex then bruce^.hide;
    r.assign(3,6,20,8);
    Bruce := New(PRadioButtons, Init(R,
           NewSItem('Active',
           NewSItem('Switched off',
           nil))
        ));
    a^.insert(Bruce);
    if istimex then bruce^.hide;

    r.assign(2,9,9,10);
    Bruce:=new(Plabel,init(r,'Border',nil));
    a^.insert(Bruce);

    r.assign(12,9,15,10);
    Bruce := New(PNumInputLine, Init(R,1,0,7));
    a^.insert(Bruce);

    if not(is48K) then begin

      r.assign(2,11,17,12);
      if istimex then
        Bruce:=new(Plabel,init(r,'Horizontal MMU register',nil))
      else
        Bruce:=new(Plabel,init(r,'MMU port value',nil));

      a^.insert(Bruce);

      r.assign(19,11,24,12);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);


      r.assign(2,14,33,15);
      Bruce:=new(Plabel,init(r,'Last selected sound register',nil));
      a^.insert(Bruce);

      r.assign(35,14,40,15);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,1,56,2);
      Bruce:=new(Plabel,init(r,'Sound registers',nil));
      a^.insert(Bruce);

      r.assign(41,2,45,3);
      Bruce:=new(Plabel,init(r,'R0',nil));
      a^.insert(Bruce);

      r.assign(46,2,51,3);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);


      r.assign(41,3,45,4);
      Bruce:=new(Plabel,init(r,'R1',nil));
      a^.insert(Bruce);

      r.assign(46,3,51,4);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,4,45,5);
      Bruce:=new(Plabel,init(r,'R2',nil));
      a^.insert(Bruce);

      r.assign(46,4,51,5);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,5,45,6);
      Bruce:=new(Plabel,init(r,'R3',nil));
      a^.insert(Bruce);

      r.assign(46,5,51,6);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,6,45,7);
      Bruce:=new(Plabel,init(r,'R4',nil));
      a^.insert(Bruce);

      r.assign(46,6,51,7);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,7,45,8);
      Bruce:=new(Plabel,init(r,'R5',nil));
      a^.insert(Bruce);

      r.assign(46,7,51,8);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,8,45,9);
      Bruce:=new(Plabel,init(r,'R6',nil));
      a^.insert(Bruce);

      r.assign(46,8,51,9);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(41,9,45,10);
      Bruce:=new(Plabel,init(r,'R7',nil));
      a^.insert(Bruce);

      r.assign(46,9,51,10);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,2,58,3);
      Bruce:=new(Plabel,init(r,'R8',nil));
      a^.insert(Bruce);

      r.assign(59,2,64,3);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,3,58,4);
      Bruce:=new(Plabel,init(r,'R9',nil));
      a^.insert(Bruce);

      r.assign(59,3,64,4);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,4,58,5);
      Bruce:=new(Plabel,init(r,'R10',nil));
      a^.insert(Bruce);

      r.assign(59,4,64,5);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,5,58,6);
      Bruce:=new(Plabel,init(r,'R11',nil));
      a^.insert(Bruce);

      r.assign(59,5,64,6);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,6,58,7);
      Bruce:=new(Plabel,init(r,'R12',nil));
      a^.insert(Bruce);

      r.assign(59,6,64,7);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,7,58,8);
      Bruce:=new(Plabel,init(r,'R13',nil));
      a^.insert(Bruce);

      r.assign(59,7,64,8);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,8,58,9);
      Bruce:=new(Plabel,init(r,'R14',nil));
      a^.insert(Bruce);

      r.assign(59,8,64,9);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);

      r.assign(54,9,58,10);
      Bruce:=new(Plabel,init(r,'R15',nil));
      a^.insert(Bruce);

      r.assign(59,9,64,10);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      a^.insert(Bruce);
      r.assign(2,13,17,14);
      Bruce:=new(Plabel,init(r,'Timex control register',nil));
      if not(istimex) then Bruce^.Hide;
      a^.insert(Bruce);
      r.assign(19,13,27,14);
      Bruce := New(PNumInputLine, Init(R,3,0,255));
      if not(istimex) then Bruce^.Hide;
      a^.insert(Bruce);

    end;

    R.Assign(45,15,55,17);
    Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
    a^.insert(Bruce);

    R.Assign(57,15,67,17);
    Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
    a^.insert(Bruce);


  end;
  a^.setdata(vrijednosti);
  c:=desktop^.execview(a);
  hourglascur;
  if c=cmOK then begin
    a^.getdata(vrijednosti);
    with snaphdr do with vrijednosti do begin
      z80stat1:=(z80stat1 and $F1) or (tborder*2);
      z80mmu:=tmmu;
      z80fffd:=tlastsnd;
      if tintpaged=0 then z80if1paged:=255 else z80if1paged:=0;
      for i:=0 to 15 do z80sndreg[i]:=tsound[i];
      izbor:=1+tmodel;
      if not(is48k) then izbor:=izbor+2;
      if z80hrdmode<>128 then case izbor of
        1: begin
          case z80snptype of
            1,2: z80stat1:=z80stat1 or $EF;
            3,4: z80hrdmode:=0
          end
        end;
        2: begin
          case z80snptype of
            1,2:  z80stat1:=z80stat1 or $EF;
            3,4: z80hrdmode:=1
          end
        end;
        3: begin
          case z80snptype of
            3: z80hrdmode:=3;
            4: z80hrdmode:=4;
          end
        end;
        4: begin
          case z80snptype of
            3: z80hrdmode:=4;
            4: z80hrdmode:=5;
          end
        end;
      end;
      if istimex then z80if1paged:=timcontr;
    end;
    saveheader;
  end;
end;

procedure TSnapshotfile.warning48;
var c:word;
begin
  if (z80snptype>2)and (snaphdr.z80hrdmode>2) then begin
    c:=messagebox(^C'Memory of Spectrum 128 '+#13+
     ^C' and Disciple data will be lost',nil,mfWarning+mfOKButton);
  end;
  if (z80snptype>2)and (snaphdr.z80hrdmode=2) then begin
    c:=messagebox(^C'Memory of SamRam will be lost '+#13
    ,nil,mfWarning+mfOKButton);
  end;
  if (z80snptype>2)and  (snaphdr.z80if1paged=$FF) then begin
    c:=messagebox(^C'Program may not work,'+#13+
    ^C' because Interface 1 is paged'+#13
    ,nil,mfWarning+mfOKButton);
  end;
end;


{ ***************************
   Z80 -> JPP
  ***************************
}
procedure TSnapshotFile.z80toSNA;
var snafilehdr:snaspanvgashdrtype;
  pc:word;
  izlazna,datoteka:file;
  izlime,d,n,e:string;
  cc:boolean;
  procedure we;

  begin
    cc:=fclose(datoteka);
  end;

begin

  if not allowed then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with snafilehdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    snai:=z80i;
    snahlp:=z80hlp;
    snadep:=z80dep;
    snabcp:=z80bcp;
    snafp:=z80fp;
    snaap:=z80ap;
    snahl:=z80hl;
    snade:=z80de;
    snabc:=z80bc;
    snaiy:=z80iy;
    snaix:=z80ix;
    if z80iff1=0 then snaiff2:=0 else snaiff2:=4;
    snar:=lo(z80r+128*(z80stat1 and 1));
    snaf:=z80f;
    snaa:=z80a;
    if z80sp>=16384 then begin
      snasp:=z80sp-2;
      uncompressed^[snasp]:=lo(z80pc);
      uncompressed^[snasp+1]:=hi(z80pc);
    end
     else begin
       displayerror('Stack pointer points to ROM',0);
       we; exit;
     end;
    snaimode:=z80stat2 and 3;
    snaborder:=(z80stat1 and 14) div 2;
  end;
  izlime:=outputfile(n+'.SNA',snapname);
  hourglascur;
  if izlime='' then begin we; exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,snafilehdr,27)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  cc:=fclose(izlazna)
end;

{ ***************************
   Z80 -> SP_UKV
  ***************************
}
procedure TSnapshotFile.z80toSNA128;
const trdosflag:byte=0;
  pagniz:array[1..6] of integer=(0,1,3,4,6,7);

var snafilehdr:snaspanvgashdrtype;
  pc:word;
  izlazna,datoteka:file;
  izlime,d,n,e:string;
  cc:boolean;
  i:integer;
  procedure we;

  begin
    cc:=fclose(datoteka);
  end;

begin
  if not allowed then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  with snafilehdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    snai:=z80i;
    snahlp:=z80hlp;
    snadep:=z80dep;
    snabcp:=z80bcp;
    snafp:=z80fp;
    snaap:=z80ap;
    snahl:=z80hl;
    snade:=z80de;
    snabc:=z80bc;
    snaiy:=z80iy;
    snaix:=z80ix;
    if z80iff1=0 then snaiff2:=0 else snaiff2:=4;
    snar:=lo(z80r+128*(z80stat1 and 1));
    snaf:=z80f;
    snaa:=z80a;
    snasp:=z80sp;
    snaimode:=z80stat2 and 3;
    snaborder:=(z80stat1 and 14) div 2;
  end;
  izlime:=outputfile(n+'.SNA',snapname);
  hourglascur;
  if izlime='' then begin we; exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,snafilehdr,27)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  if not(fblockwrite(izlazna,PC,2)) then begin we; exit end;
  if not(fblockwrite(izlazna,snaphdr.z80mmu,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,trdosflag,1)) then begin we; exit end;
  for i:=1 to 6 do begin
    if not(is48ksnap) then load128kpage(pagniz[i]+3);
    if pagniz[i] <> snaphdr.z80mmu and 7 then
      if not(fblockwrite(izlazna,uncompressed^[49152],16384)) then begin
        we; exit
      end
  end;

  cc:=fclose(izlazna)
end;


{ ***************************
   Z80 -> Irish PRG
  ***************************
}
procedure TSnapshotFile.z80toPRG;
var prgfilehdr:prghdrtype;
  pc:word;
  a,f,iff2:byte;
  izlazna,datoteka:file;
  izlime,d,n,e:string;
  cc:boolean;
  i: integer;

  procedure we;
  begin
    cc:=fclose(datoteka);
  end;

begin

  if not allowed then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with prgfilehdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    for i:=1 to 10 do prgname[i]:=' ';
    prgnulbyte:=0;
    prgsig61:=$61; prgsig35:=$35;prgsig03:=$03;
    for i:=1 to 206 do prgempty[i]:=0;
    for i:=1 to 14 do prgempty2[i]:=0;
    prgi:=z80i;
    prghlp:=z80hlp;
    prgdep:=z80dep;
    prgbcp:=z80bcp;
    prgfp:=z80fp;
    prgap:=z80ap;
    prghl:=z80hl;
    prgde:=z80de;
    prgbc:=z80bc;
    prgiy:=z80iy;
    prgix:=z80ix;
    if z80iff1=0 then prgiff2:=0 else prgiff2:=4;
    if z80sp>=16384 then begin
      prgsp:=z80sp-6;
      uncompressed^[prgsp+0]:=z80iff2;
      uncompressed^[prgsp+1]:=lo(z80r+128*(z80stat1 and 1));
      uncompressed^[prgsp+2]:=z80f;
      uncompressed^[prgsp+3]:=z80a;
      uncompressed^[prgsp+4]:=lo(z80pc);
      uncompressed^[prgsp+5]:=hi(z80pc);
    end
     else begin
       displayerror('Stack pointer points to ROM',0);
       we; exit;
     end;
  end;
  izlime:=outputfile(n+'.PRG',snapname);
  hourglascur;
  if izlime='' then begin we; exit; end;
  for i:=1 to 10 do if i<length(izlime) then
    prgfilehdr.prgname[i]:=izlime[i];

  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,prgfilehdr,256)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  cc:=fclose(izlazna)
end;

{ ****************************
   Z80 to old and new SP
  ****************************
  tip=1 VGASPEC
  tip=2 Spanish SP
}


procedure TSnapshotFile.z80toSpanishVGASP(tip:integer);
var
  spanishhdr:snaspanvgashdrtype;
  pc:word;
  izlazna,datoteka:file;
  cc:boolean;
  izlime,d,n,e:string;
const
  sign1:longint=$C0005053;
  sign2:longint=$4000;
  procedure we;
  begin
    cc:=fclose(izlazna);
  end;

begin
  if not(allowed) then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with spanishhdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    spabc:=z80bc;
    spade:=z80de;
    spahl:=z80hl;
    spaf:=z80f;
    spaa:=z80a;
    spaix:=z80ix;
    spaiy:=z80iy;
    spabcp:=z80bcp;
    spadep:=z80dep;
    spahlp:=z80hlp;
    spafp:=z80fp;
    spaap:=z80ap;
    spar:=z80r;
    spai:=z80i;
    spasp:=z80sp;
    spapc:=z80pc;
    spanot1:=0;
    spaborder:=(z80stat1 and 14) div 2;
    spainfc:=0;
    spastatus:=0;
    if z80iff1<>0 then inc(spastatus);
    if (z80stat2 and 3)=2 then spastatus:=spastatus+2;
    spanot2:=0
  end;
  case tip of
    1: izlime:=outputfile(n+'.SP',snapname);
    2: izlime:=outputfile(n+'.SP',snapname)
  end;
  hourglascur;
  if izlime='' then begin exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if tip=2 then begin
    if not(fblockwrite(izlazna,sign1,4)) then begin we; exit end;
    if not(fblockwrite(izlazna,sign2,2)) then begin we; exit end;
  end;
  if not(fblockwrite(izlazna,spanishhdr,32)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  cc:=fclose(izlazna)
end;

procedure TSnapshotFile.Z80ToVGASPEC;
begin
  z80toSpanishVGASP(1);
end;

procedure TSnapshotFile.Z80ToSpanish;
begin
  z80toSpanishVGASP(2);
end;
{ ***********************
  Z80 to German SEM
  ***********************
}
procedure TSnapshotFile.Z80toSEM;
var
  germanhdr:semtailtype;
  izlazna,datoteka:file;
  cc:boolean;
  pc:word;
  izlime,d,n,e:string;
const
  sign:array[0..5] of char=(#5,'S','P','E','C','1');
  procedure we;
  begin
    cc:=fclose(izlazna);
  end;

begin
  if not(allowed) then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with germanhdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    sembc:=z80bc;
    semde:=z80de;
    semhl:=z80hl;
    semf:=z80f;
    sema:=z80a;
    semix:=z80ix;
    semiy:=z80iy;
    sembcp:=z80bcp;
    semdep:=z80dep;
    semhlp:=z80hlp;
    semfp:=z80fp;
    semap:=z80ap;
    semr:=lo(z80r+128*(z80stat1 and 1));

    semi:=z80i;
    semsp:=z80sp;
    sempc:=pc;
    if z80iff1=0 then semiff1:=0 else semiff1:=1;
    if z80iff2=0 then semiff2:=0 else semiff2:=1;
    semim:=0;
    if (z80stat2 and 3)=2 then semim:=semim+2;
  end;
    izlime:=outputfile(n+'.SEM',snapname);
    hourglascur;
  if izlime='' then begin exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,sign,6)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  if not(fblockwrite(izlazna,germanhdr,34)) then begin we; exit end;

  cc:=fclose(izlazna)
end;

{ ***********************
  Z80 to Garabik
  ***********************
}
procedure TSnapshotFile.Z80toSNP;
var
  garabhdr:snptailtype;
  izlazna,datoteka:file;
  cc:boolean;
  pc:word;
  izlime,d,n,e:string;
  procedure we;
  begin
    cc:=fclose(izlazna);
  end;

begin
  if not(allowed) then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with garabhdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    snpbc:=z80bc;
    snpde:=z80de;
    snphl:=z80hl;
    snpf:=z80f;
    snpa:=z80a;
    snpix:=z80ix;
    snpiy:=z80iy;
    snpbcp:=z80bcp;
    snpdep:=z80dep;
    snphlp:=z80hlp;
    snpfp:=z80fp;
    snpap:=z80ap;
    snpr:=z80r;
    snpi:=z80i;
    snpsp:=z80sp;
    snppc:=pc;
    if z80iff1=0 then snpiff1:=0 else snpiff1:=1;
    if z80iff2=0 then snpiff2:=0 else snpiff2:=1;
    snpim:=0;
    if (z80stat2 and 3)=2 then snpim:=snpim+2;
  end;
    izlime:=outputfile(n+'.SNP',snapname);
  hourglascur;
  if izlime='' then begin exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;
  if not(fblockwrite(izlazna,garabhdr,31)) then begin we; exit end;

  cc:=fclose(izlazna)
end;

{ ***********************
  Z80 to Spectrum 2.0
  ***********************
}
procedure TSnapshotFile.Z80toSIT;
var
  sithdr:sithdrtype;
  izlazna,datoteka,romfile:file;

  cc:boolean;
  pc:word;
  izlime,d,n,e:string;
  procedure we;
  begin
    cc:=fclose(izlazna);
    getheader;
  end;

begin
  if not(allowed) then exit;
  fsplit(snapname,d,n,e);
  getheader;
  load48k;
  warning48;
  with sithdr do with snaphdr do begin
    if z80oldpc=0 then pc:=z80pc else pc:=z80oldpc;
    sitbc:=z80bc;
    sitde:=z80de;
    sithl:=z80hl;
    sitf:=z80f;
    sita:=z80a;
    sitix:=z80ix;
    sitiy:=z80iy;
    sitbcp:=z80bcp;
    sitdep:=z80dep;
    sithlp:=z80hlp;
    sitfp:=z80fp;
    sitap:=z80ap;
    sitr:=lo(z80r+128*(z80stat1 and 1));

    siti:=z80i;
    sitsp:=z80sp;
    sitpc:=pc;
    if z80iff1=0 then siti:=0 else siti:=1;
    if (z80stat2 and 3)=2 then siti:=siti+2;
  end;
    izlime:=outputfile(n+'.SIT',snapname);
  hourglascur;
  if izlime='' then begin exit; end;
  assign(izlazna,izlime);
  if not(frewriteu(izlazna,1)) then begin we; exit end;
  if not(fblockwrite(izlazna,sithdr,28)) then begin we; exit end;
  assign(romfile,emuldir+'ZX48.ROM');
  if not(fresetu(romfile,1)) then begin we; exit end;
  if not(fblockread(romfile,compressed^,16384)) then begin
    cc:=fclose(romfile);  we; exit end;
  if not(fclose(romfile)) then begin we; exit end;
  if not(fblockwrite(izlazna,compressed^,16384)) then begin
    we; exit
  end;

  if not(fblockwrite(izlazna,uncompressed^[16384],49152)) then begin
    we; exit
  end;

  cc:=fclose(izlazna)
end;

{ *****************************
  SNA, VGASPEC, Spanish to Z80
  1 VGASPEC
  2 SPANISH
  3 SNA
  *****************************
}


procedure TSnapshotFile.SNAVGASPAtoZ80(tip:integer);
var snafilehdr:snaspanvgashdrtype;
  datoteka:file;
  cc:boolean;
  SNAfilename,search,tmp,d,n,e:string;
  elem,rb,i:integer;
  fs,hs:word;
  dirinfo: searchrec;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
  end;

begin
  case tip of
    3: begin
      search:='.SNA';
      fs:=49179;
      hs:=27;
    end;
    2: begin
      search:='.SP';
      fs:=49190;
      hs:=32;
    end;
    1: begin
      search:='.SP';
      fs:=49184;
      hs:=32;
    end;

  end;
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  case tip of
    1: a^.helpctx:=hcsvgawara;
    2: a^.helpctx:=hcsspwara;
    3: a^.helpctx:=hcjppwara;
  end;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(Snafilename);
    dispose (a,done);
    Snafilename:=fexpand(Snafilename);
    if not(fileexists(Snafilename)) then
     displayerror('Snapshot file#'+Snafilename+'#not found',0)
    else begin
      fsplit(snafilename,d,n,e);
      assign(datoteka,snafilename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if filesize(datoteka)<> fs then begin
        displayerror('This is not '+search+' format',0);
        we; exit
      end;
      if tip=2 then
        if not(fblockread(datoteka,snafilehdr,6)) then begin we;exit end;
      if not(fblockread(datoteka,snafilehdr,hs)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      if tip=3 then with snafilehdr do with snaphdr do begin
        z80A:=snaa;
        z80f:=snaf;
        z80bc:=snabc;
        z80hl:=snahl;
        z80sp:=snasp;
        z80i:=snai;
        z80r:=snar;
        z80stat1:=snaborder*2+32;
        if (snar and 128)=0 then  z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=snade;
        z80bcp:=snabcp;
        z80dep:=snadep;
        z80hlp:=snahlp;
        z80ap:=snaap;
        z80fp:=snafp;
        z80iy:=snaiy;
        z80ix:=snaix;
        z80iff1:=snaiff2;
        z80iff2:=snaiff2;
        z80stat2:=snaimode and 3;
        z80pc:=114 {RETN}
      end; {With}
      if (tip=2) or (tip=1)  then with snafilehdr do with snaphdr do begin
        z80A:=spaa;
        z80f:=spaf;
        z80bc:=spabc;
        z80hl:=spahl;
        z80sp:=spasp;
        z80pc:=spapc;
        z80i:=spai;
        z80r:=spar;
        z80stat1:=spaborder*2+32;
        if (spar and 128)=0 then  z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=spade;
        z80bcp:=spabcp;
        z80dep:=spadep;
        z80hlp:=spahlp;
        z80ap:=spaap;
        z80fp:=spafp;
        z80iy:=spaiy;
        z80ix:=spaix;
        z80iff1:=spastatus and 1;
        z80iff2:=z80iff1;
        if (spastatus and 2)<>0 then z80stat2:=2 else z80stat2:=1;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80if1paged:=0;
        z80rldir:=0;
        z80fffd:=0;
        tmp:=snapname;
        for i:=0 to 15 do z80sndreg[i]:=0;
        snapname:=outputfile(n+'.Z80',snapname);
        hourglascur;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end {With}
    end
  end
   else dispose(a,done);
  getheader;
end;

procedure TSnapshotFile.snatoz80;
begin
  snavgaspatoz80(3);
end;
procedure TSnapshotFile.spanishtoz80;
begin
  snavgaspatoz80(2);
end;
procedure TSnapshotFile.vgaspectoz80;
begin
  snavgaspatoz80(1);
end;
{ **********************************
   SNA 128 u Z80
  **********************************
}


procedure TSnapshotfile.SNA128toZ80;
var i:integer;
  tmp,snafilename,d,n,e:string;
  datoteka,izlaz:file;
  di,j,t1:word;
  startaddr:array[0..7] of longint;
  curaddr:longint;
  cc:Boolean;
  snafilehdr:snaspanvgashdrtype;

  a:PFileDialog;

  procedure we;
  begin
    getheader  ;
    cc:=fclose(datoteka);
    cc:=fclose(izlaz);
  end;
begin
  a:=new(PFileDIalog,Init('*.SNA','Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcspukvWara;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(Snafilename);
    dispose(a,done);
    Snafilename:=fexpand(Snafilename);
    if not(fileexists(Snafilename)) then
      displayerror('Snapshot file#'+Snafilename+'#not found',0)
     else begin
       assign(datoteka,snafilename);
       fsplit(snafilename,d,n,e);
       if not(fresetu(datoteka,1)) then begin we; exit end;
       if (filesize(datoteka)<> 131103) and
        (filesize(datoteka)<> 147487) then begin
         displayerror('This is not SNA 128 format',0);
         we; exit
       end;
       if not(fblockread(datoteka,snafilehdr,27)) then begin we; exit end;
       with snafilehdr do with snaphdr do begin
         z80A:=snaa;
         z80f:=snaf;
         z80bc:=snabc;
         z80hl:=snahl;
         z80sp:=snasp;
         z80i:=snai;
         z80r:=snar;
         z80stat1:=snaborder*2+32;
         if (snar and 128)=0 then  z80stat1:=z80stat1 and $FE
          else z80stat1:=z80stat1 or 1;
         z80de:=snade;
         z80bcp:=snabcp;
         z80dep:=snadep;
         z80hlp:=snahlp;
         z80ap:=snaap;
         z80fp:=snafp;
         z80iy:=snaiy;
         z80ix:=snaix;
         z80iff1:=snaiff2;
         z80iff2:=snaiff2;
         z80stat2:=snaimode and 3;
         if not(fseek(datoteka,49179)) then begin we; exit end;
         if not(fblockread(datoteka,z80pc,2)) then begin we; exit end;
       end; {With}

       with snaphdr do begin
         t1:=z80hrdmode;
         z80oldpc:=0;
         if is48ksnap then begin
           z80mmu:=16;
           for i:=0 to 15 do z80sndreg[i]:=0;
           z80fffd:=0;
         end
          else begin
            if not(fseek(datoteka,49181)) then begin we; exit end;
            if not(fblockread(datoteka,z80mmu,1)) then begin we; exit end;
          end;
         z80addhdr:=23;
         z80hrdmode:=3;
         z80if1paged:=0;
         z80rldir:=0;

         tmp:=snapname;
         snapname:=outputfile(n+'.Z80',snapname);
         hourglascur;
         if snapname='' then begin
           snapname:=tmp;
           getheader;
           exit
         end;
         assign(izlaz,snapname);
         snapname:=tmp;
         if not(frewriteu(izlaz,1)) then begin we; exit end;
         if not(fblockwrite(izlaz,snaphdr,55)) then begin we; exit end;
         z80hrdmode:=t1;
       end; {With}
       for i:=0 to 7 do startaddr[i]:=0;
       startaddr[snaphdr.z80mmu and 7]:=27+32768;
       startaddr[2]:=27+16384;
       startaddr[5]:=27;
       curaddr:=49183;
       for i:=0 to 7 do begin
         if startaddr[i]=0 then begin
           startaddr[i]:=curaddr;
           curaddr:=curaddr+16384;
         end;
       end;
       for i:=3 to 10 do begin
         if not(fseek(datoteka,startaddr[i-3])) then begin we; exit end;
         if not(fblockread(datoteka,uncompressed^[49152],16384)) then begin we; exit end;
         di:=0;
         compsnap(49152,65535,di);
         z80pagehdr.datalength:=di;
         z80pagehdr.pagenumber:=i;
         if not(fblockwrite(izlaz, z80pagehdr,3)) then begin we; exit end;
         if not(fblockwrite(izlaz,compressed^,di)) then begin we; exit end;
       end;
       cc:=fclose(izlaz);
       cc:=fclose(datoteka);
       getheader;
     end;
  end
   else dispose(a,done);
end;
{ *****************************
  Irish SPECEM to Z80
  *****************************
}


procedure TSnapshotFile.PRGtoZ80;
var prgfilehdr:prghdrtype;
  datoteka:file;
  cc:boolean;
  prgfilename,search,tmp,d,n,e:string;
  elem,rb,i:integer;
  fs,hs:word;
  dirinfo: searchrec;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
  end;

begin
  search:='.PRG';
  fs:=49152+256;
  hs:=256;
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcsirishwara;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(Prgfilename);
    dispose(a,done);
    Prgfilename:=fexpand(Prgfilename);
    if not(fileexists(Prgfilename)) then
     displayerror('Snapshot file#'+Prgfilename+'#not found',0)
    else begin
      assign(datoteka,prgfilename);
      fsplit(prgfilename,d,n,e);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if filesize(datoteka)< fs then begin
        displayerror('This is not '+search+' format',0);
        we; exit
      end;
      if not(fblockread(datoteka,prgfilehdr,hs)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      with prgfilehdr do with snaphdr do begin
        z80A:=uncompressed^[prgsp+3];

        z80f:=uncompressed^[prgsp+2];

        z80bc:=prgbc;
        z80hl:=prghl;
        z80sp:=prgsp+6;
        z80i:=prgi;
        z80r:=uncompressed^[prgsp+1];
        z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
        if (uncompressed^[prgsp+1] and 128)=0 then
          z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=prgde;
        z80bcp:=prgbcp;
        z80dep:=prgdep;
        z80hlp:=prghlp;
        z80ap:=prgap;
        z80fp:=prgfp;
        z80iy:=prgiy;
        z80ix:=prgix;
        z80iff1:=uncompressed^[prgsp+0];
        z80iff2:=z80iff1;
        if z80i=63 then z80stat2:=1 else z80stat2:=2;
        z80pc:=uncompressed^[prgsp+4]+256*uncompressed^[prgsp+5];
      end; {With}
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80if1paged:=0;
        z80rldir:=0;
        z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=outputfile(n+'.Z80',snapname);
        hourglascur;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end {With}
    end
  end
   else dispose(a,done);
  getheader;
end;

{ *****************************
  German SPECEMUG to Z80
  *****************************
}


procedure TSnapshotFile.SEMtoZ80;
var semfilehdr:semtailtype;
  datoteka:file;
  cc:boolean;
  semfilename,search,tmp,d,n,e:string;
  elem,rb,i:integer;
  sigsize,sigread:byte;
  dirinfo: searchrec;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
  end;

begin
  search:='.SEM';
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcgermanwara;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(semfilename);
    dispose(a,done);
    semfilename:=fexpand(semfilename);
    if not(fileexists(semfilename)) then
     displayerror('Snapshot file#'+semfilename+'#not found',0)
    else begin
      assign(datoteka,semfilename);
      fsplit(semfilename,d,n,e);
      if not(fresetu(datoteka,1)) then begin we; exit end;

      if not(fblockread(datoteka,sigsize,1)) then begin we; exit end;
      for i:=1 to sigsize do
        if not(fblockread(datoteka,sigread,1)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fblockread(datoteka,semfilehdr,34)) then begin we; exit end;
      if not(fclose(datoteka)) then begin we; exit end;
      with semfilehdr do with snaphdr do begin
        z80A:=sema;

        z80f:=semf;

        z80bc:=sembc;
        z80hl:=semhl;
        z80sp:=semsp;
        z80i:=semi;
        z80r:=semr;
        z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
        if (semr and 128)=0 then
          z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=semde;
        z80bcp:=sembcp;
        z80dep:=semdep;
        z80hlp:=semhlp;
        z80ap:=semap;
        z80fp:=semfp;
        z80iy:=semiy;
        z80ix:=semix;
        z80iff1:=semiff1;
        z80iff2:=semiff2;
        z80stat2:=semim;
        z80pc:=sempc;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80if1paged:=0;
        z80rldir:=0;
        z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=outputfile(n+'.Z80',snapname);
        hourglascur;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end {With}
    end
  end
   else dispose(a,done);
  getheader;
end;


{ *****************************
  SPECTRUM 2.00 to Z80
  *****************************
}


procedure TSnapshotFile.SITtoZ80;
var sitfilehdr:sithdrtype;
  datoteka,romdat:file;
  cc:boolean;
  sitfilename,romname,search,tmp,d,n,e:string;
  elem,rb,i:integer;
  sigsize,sigread:byte;
  dirinfo: searchrec;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
  end;

begin
  search:='.SIT';
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcspec200wara;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(sitfilename);
    dispose(a,done);
    sitfilename:=fexpand(sitfilename);
    if not(fileexists(sitfilename)) then
     displayerror('Snapshot file#'+sitfilename+'#not found',0)
    else begin
      assign(datoteka,sitfilename);
      fsplit(sitfilename,d,n,e);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if filesize(datoteka)< 65564 then begin
        displayerror('This is not '+search+' format',0);
        we; exit
      end;

      if not(fblockread(datoteka,sitfilehdr,28)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,16384)) then begin
        we; exit
      end;
      if messagebox('Extract ROM from snapshot?',nil,mfYesButton+mfNoButton)=
        cmYes then begin
          romname:=outputfile(n+'.ROM',snapname);
          assign(romdat,romname);
          if not(frewriteu(romdat,1)) then begin cc:=fclose(romdat); we end;
          if not(fblockwrite(romdat,uncompressed^,16384)) then begin
            cc:=fclose(romdat); we
          end;
          if not(fclose(romdat)) then begin we; exit end;
        end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      with sitfilehdr do with snaphdr do begin
        z80A:=sita;

        z80f:=sitf;

        z80bc:=sitbc;
        z80hl:=sithl;
        z80sp:=sitsp;
        z80i:=siti;
        z80r:=sitr;
        z80stat1:=sitborder shl 1;
        if (sitr and 128)=0 then
          z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=sitde;
        z80bcp:=sitbcp;
        z80dep:=sitdep;
        z80hlp:=sithlp;
        z80ap:=sitap;
        z80fp:=sitfp;
        z80iy:=sitiy;
        z80ix:=sitix;
        if (sitinter and 1) <>0 then z80iff1:=4 else z80iff1:=0;
        z80iff2:=z80iff1;
        if (sitinter and 2) <>0 then z80stat2:=2 else z80stat2:=0;
        z80pc:=sitpc;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80if1paged:=0;
        z80rldir:=0;
        z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=outputfile(n+'.Z80',snapname);
        hourglascur;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end {With}
    end
  end
   else dispose(a,done);
  getheader;
end;



{ *****************************
  Garabik SNP to Z80
  *****************************
}


procedure TSnapshotFile.SNPtoZ80;
var snpfilehdr:snptailtype;
  datoteka:file;
  cc:boolean;
  semfilename,search,tmp,d,n,e:string;
  elem,rb,i:integer;
  sigsize,sigread:byte;
  dirinfo: searchrec;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
    getheader;
  end;

begin
  search:='.SNP';
  a:=new(PFileDIalog,Init('*'+search,'Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcgarabikwara;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(semfilename);
    dispose(a,done);
    semfilename:=fexpand(semfilename);
    if not(fileexists(semfilename)) then
     displayerror('Snapshot file#'+semfilename+'#not found',0)
    else begin
      assign(datoteka,semfilename);
      fsplit(semfilename,d,n,e);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if filesize(datoteka)< 49183 then begin
        displayerror('This is not '+search+' format',0);
        we; exit
      end;

      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fblockread(datoteka,snpfilehdr,31)) then begin we; exit end;
      if not(fclose(datoteka)) then begin we; exit end;
      with snpfilehdr do with snaphdr do begin
        z80A:=snpa;

        z80f:=snpf;

        z80bc:=snpbc;
        z80hl:=snphl;
        z80sp:=snpsp;
        z80i:=snpi;
        z80r:=snpr;
        z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
        if (snpr and 128)=0 then
          z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=snpde;
        z80bcp:=snpbcp;
        z80dep:=snpdep;
        z80hlp:=snphlp;
        z80ap:=snpap;
        z80fp:=snpfp;
        z80iy:=snpiy;
        z80ix:=snpix;
        z80iff1:=snpiff1;
        z80iff2:=snpiff2;
        z80stat2:=snpim;
        z80pc:=snppc;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80if1paged:=0;
        z80rldir:=0;
        z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=outputfile(n+'.Z80',snapname);
        hourglascur;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end {With}
    end
  end
   else dispose(a,done);
  getheader;
end;
{ ***************************
   ZX32 RIFF to Z80
  ***************************}
procedure TSnapshotFIle.RIFFtoZ80;
var chunk:String[4];
size,upto:Longint;
ulazna,izlaz:file;
fd:PFileDialog;
filename,tmp,d,n,e:string;
riffregs:riffchunktype;
border:byte;
i,di:word;
sss:string;
obradi:Boolean;
fmtzinfo:record
  wVersion,WHardModel,wHardFlags,wComprMeth:word
end;
procedure we;
var cc:boolean;
begin
  cc:=fclose(ulazna);
  cc:=fclose(izlaz);
  getheader;
end;

begin
  fd:=new(PFileDIalog,Init('*.ZXS','Select RIFF file to extract snap','~N~ame',
   fdOpenButton,100));
  fd^.helpctx:=hcvaggwara;
  if desktop^.execview(fd)<> cmcancel then begin
    hourglascur;
    fd^.getfilename(filename);
    filename:=fexpand(filename);
    if not(fileexists(filename)) then begin dispose(fd,done); exit end;
  end
   else begin dispose(fd,done); exit end;
  dispose(fd,done);
  assign(ulazna,filename);
  fsplit(filename,d,n,e);
  if not(fresetu(ulazna,1)) then begin exit;end;
  tmp:=snapname;
  snapname:=outputfile(n+'.Z80',snapname);
  hourglascur;
  if snapname='' then begin
    snapname:=tmp;
    getheader;
    exit
  end;
  assign(izlaz,snapname);
  snapname:=tmp;
  if not(frewriteu(izlaz,1)) then begin close(ulazna); exit end;

  chunk[0]:=#4;
  if not(fblockread(ulazna,chunk[1],4)) then begin we; exit end;
  if chunk<>'RIFF' then begin
    DisplayError('This is not ZX32 RIFF file',0); we; exit
  end;
  if not(fblockread(ulazna,size,4)) then begin we; exit end;
  if not(fblockread(ulazna,chunk[1],4)) then begin we; exit end;
  if chunk<>'SNAP' then begin
    DisplayError('This is not ZX32 RIFF file',0); we; exit
  end;
  with snaphdr do begin
     z80oldpc:=0;
     z80addhdr:=23;
     z80hrdmode:=0;
     z80mmu:=0;
     z80if1paged:=0;
     z80rldir:=0;
     z80fffd:=0;
  end;
  if not(fblockwrite(izlaz,snaphdr,55)) then begin we; exit end;
  while not eof(ulazna) do begin
    if not(fblockread(ulazna,chunk[1],4)) then begin we; exit end;
    if not(fblockread(ulazna,size,4)) then begin we; exit end;
    obradi:=true;
    if chunk='rZ80' then begin
      if not(fblockread(ulazna,riffregs,34)) then begin we; exit end;
      with riffregs do with snaphdr do begin
        z80A:=riffa;z80f:=rifff;z80bc:=riffbc;
        z80hl:=riffhl;z80sp:=riffsp;z80i:=riffi;
        z80r:=riffr;z80de:=riffde;z80bcp:=riffbcp;
        z80dep:=riffdep;z80hlp:=riffhlp;z80ap:=riffap;
        z80fp:=rifffp; z80iy:=riffiy;  z80ix:=riffix;
        z80iff1:=riffiff1; z80iff2:=riffiff2; z80stat2:=riffim;
        z80pc:=riffpc;
      end; {With}
      obradi:=false;
    end;
    if copy(chunk,1,3)='ram' then begin
      if snaphdr.z80hrdmode=3 then
        i:=((ord(chunk[4])-ord('0')) and 7)+3
      else
        case ((ord(chunk[4])-ord('0')) and 7) of
        0,1: i:=5;
        2: i:=4;
        5:i:=8
        end;
      if not(fblockread(ulazna,uncompressed^[49152],16384)) then begin we; exit end;
      di:=0;
      compsnap(49152,65535,di);
      z80pagehdr.datalength:=di;
      z80pagehdr.pagenumber:=i;
      if not(fblockwrite(izlaz, z80pagehdr,3)) then begin we; exit end;
      if not(fblockwrite(izlaz,compressed^,di)) then begin we; exit end;
      obradi:=false;
    end;
    if chunk='fmtz' then begin
      with snaphdr do begin
        if not(fblockread(ulazna,fmtzinfo,8)) then begin we; exit end;
        if fmtzinfo.whardmodel=$30 then snaphdr.z80hrdmode:=3;
        if fmtzinfo.wComprMeth<>$FFFF then begin
          DisplayError('Compressed chunks are not available',0);
          we; exit
        end;
      end;
      obradi:=false;
    end;

    if chunk='r128' then begin
      with snaphdr do begin
        if not(fblockread(ulazna,z80mmu,1)) then begin we; exit end;
        if not(fblockread(ulazna,z80fffd,1)) then begin we; exit end;
        if not(fblockread(ulazna,z80sndreg,16)) then begin we; exit end;
        z80hrdmode:=3;
      end;
      obradi:=false;
    end;
    if chunk='r048' then begin
      with snaphdr do begin
        if not(fblockread(ulazna,border,1)) then begin we; exit end;
        z80stat1:=lo((border and $38)*4+32);
        if not(fseek(ulazna,filepos(ulazna)+9)) then begin we; exit end;
      end;
      obradi:=false;
    end;

    if obradi then begin
       if not(fseek(ulazna,filepos(ulazna)+size)) then begin we; exit end
    end
  end;
  if not(fseek(izlaz,0)) then begin we; exit end;
  if not(fblockwrite(izlaz,snaphdr,55)) then begin we; exit end;

  we;
end;


procedure TSnapshotfile.Z80ToRIFF;
const
  riffempty:array[1..12] of char=('R','I','F','F',#0,#0,#0,#0,
  'S','N','A','P');
var i:integer;
  tmp,d,n,e:string;
  datoteka:file;
  di,j,t1,wd:word;
  cc:Boolean;
  chunk:String[4];
  size:Longint;
  riffregs:riffchunktype;

  procedure we;
  begin
    getheader  ;
    cc:=fclose(datoteka);
  end;
procedure convpage(i:integer);
var j:word;
begin
  if is48ksnap then begin
    load48k;
    case i of
      5: move(uncompressed^[32768],uncompressed^[49152],16384);
      8: move(uncompressed^[16384],uncompressed^[49152],16384);
      3: ;
      else for j:=49152 to 65535 do uncompressed^[j]:=0;
    end;
  end
   else load128kpage(i);
  chunk:='ram ';
  chunk[4]:=chr(i-3+ord('0'));size:=16384;
  if not(fblockwrite(datoteka,chunk[1],4)) then begin we; exit end;
  if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
  if not(fblockwrite(datoteka,uncompressed^[49152],16384)) then begin we;exit end;

end;
begin
  if not allowed then exit;
  getheader;
  with snaphdr do begin
    t1:=z80hrdmode;
    z80oldpc:=0;
    if is48ksnap then begin
      z80mmu:=16;
      for i:=0 to 15 do z80sndreg[i]:=0;
      z80fffd:=0;
     end;

    z80addhdr:=23;
    z80hrdmode:=3;
    z80if1paged:=0;
    z80rldir:=0;

    tmp:=snapname;
    fsplit(snapname,d,n,e);
    snapname:=outputfile(n+'.ZXS',snapname);
    hourglascur;
    if snapname='' then begin
      snapname:=tmp;
      getheader;
      exit
    end;
    assign(datoteka,snapname);
    snapname:=tmp;
    if not(frewriteu(datoteka,1)) then begin we; exit end;
    if not(fblockwrite(datoteka,riffempty,12)) then begin we; exit end;

    chunk:='fmtz';size:=8;
    if not(fblockwrite(datoteka,chunk[1],4)) then begin we; exit end;
    if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
    wd:=$100;if not(fblockwrite(datoteka,wd,2)) then begin we; exit end;
    wd:=$30;if not(fblockwrite(datoteka,wd,2)) then begin we; exit end;
    wd:=$0;if not(fblockwrite(datoteka,wd,2)) then begin we; exit end;
    wd:=$FFFF;if not(fblockwrite(datoteka,wd,2)) then begin we; exit end;

    chunk:='rZ80'; size:=33;
    with riffregs do  begin
        riffa:=z80a;rifff:=z80f;riffbc:=z80bc;
        riffhl:=z80hl;riffsp:=z80sp;riffi:=z80i;
        riffr:=z80r;riffde:=z80de;riffbcp:=z80bcp;
        riffdep:=z80dep;riffhlp:=z80hlp;riffap:=z80ap;
        rifffp:=z80fp; riffiy:=z80iy;riffix:=z80ix;
        riffiff1:=z80iff1; riffiff2:=z80iff2; riffim:=z80stat2 and 3;
        riffpc:=z80pc; dwticks:=0; riffunk:=0;
    end; {With}
    if not(fblockwrite(datoteka,chunk[1],4)) then begin we; exit end;
    if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
    if not(fblockwrite(datoteka,riffregs,34)) then begin we; exit end;

    chunk:='r048';size:=9;
    if not(fblockwrite(datoteka,chunk[1],4)) then begin we; exit end;
    if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
    if not(fblockwrite(datoteka,z80stat1,1)) then begin we; exit end;
    size:=$0;if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
    size:=$0;if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
    if not(fblockwrite(datoteka,RIFFEMPTY[5],1)) then begin we; exit end;
    convpage(3);
    convpage(5);
    convpage(8);
    if not(is48ksnap) then begin
      chunk:='r128';size:=18;
      if not(fblockwrite(datoteka,chunk[1],4)) then begin we; exit end;
      if not(fblockwrite(datoteka,size,4)) then begin we; exit end;
      if not(fblockwrite(datoteka,z80mmu,1)) then begin we; exit end;
      if not(fblockwrite(datoteka,z80fffd,1)) then begin we; exit end;
      if not(fblockwrite(datoteka,z80sndreg,16)) then begin we; exit end;
    end;
    convpage(4);
    convpage(6);
    convpage(7);
    convpage(9);
    convpage(10);


  end; {With}

  if not(fseek(datoteka,4)) then begin we; exit end;
  size:=filesize(datoteka)-8;
  if not(fblockwrite(datoteka,size,4)) then begin we; exit end;

  cc:=fclose(datoteka);
  getheader;
end;

{ **********************************
   Konverzija svih Z80 fajlova u 48
  **********************************
}




procedure TSnapshotfile.Z80anyToZ8048k;
var i:integer;
  tmp,d,n,e:string;
begin
  if not allowed then exit;
  getheader;
  load48k;
  warning48;
  with snaphdr do begin
    z80oldpc:=0;
    z80addhdr:=23;
    z80hrdmode:=0;
    z80mmu:=0;
    z80if1paged:=0;
    z80rldir:=0;
    z80fffd:=0;
    for i:=0 to 15 do z80sndreg[i]:=0;
    tmp:=snapname;
    fsplit(snapname,d,n,e);
    snapname:=outputfile('4'+n+'.Z80',snapname);
    hourglascur;
    if snapname<>'' then savehdrand48k(55);
    snapname:=tmp;
  end; {With}
  getheader;
end;

{ **********************************
   Konverzija svih Z80 fajlova u 128
  **********************************
}


procedure TSnapshotfile.Z80anyToZ80128k;
var i:integer;
  tmp,d,n,e:string;
  datoteka:file;
  di,j,t1:word;
  cc:Boolean;

  procedure we;
  begin
    getheader  ;
    cc:=fclose(datoteka);
  end;
begin
  if not allowed then exit;
  getheader;
  with snaphdr do begin
    t1:=z80hrdmode;
    z80oldpc:=0;
    if is48ksnap then begin
      z80mmu:=16;
      for i:=0 to 15 do z80sndreg[i]:=0;
      z80fffd:=0;
     end;

    z80addhdr:=23;
    z80hrdmode:=3;
    z80if1paged:=0;
    z80rldir:=0;

    tmp:=snapname;
    fsplit(snapname,d,n,e);
    snapname:=outputfile('1'+n+'.Z80',snapname);
    hourglascur;
    if snapname='' then begin
      snapname:=tmp;
      getheader;
      exit
    end;
    assign(datoteka,snapname);
    snapname:=tmp;
    if not(frewriteu(datoteka,1)) then begin we; exit end;
    if not(fblockwrite(datoteka,snaphdr,55)) then begin we; exit end;
    z80hrdmode:=t1;
  end; {With}

  for i:=3 to 10 do begin
    if is48ksnap then begin
      load48k;
      case i of
        5: move(uncompressed^[32768],uncompressed^[49152],16384);
        8: move(uncompressed^[16384],uncompressed^[49152],16384);
        3: ;
        else for j:=49152 to 65535 do uncompressed^[j]:=0;
      end;
    end
     else load128kpage(i);
    di:=0;
    compsnap(49152,65535,di);
    z80pagehdr.datalength:=di;
    z80pagehdr.pagenumber:=i;
    if not(fblockwrite(datoteka, z80pagehdr,3)) then begin we; exit end;
    if not(fblockwrite(datoteka,compressed^,di)) then begin we; exit end;
  end;
  cc:=fclose(datoteka);
  getheader;
end;
{ ************************************
   View Snapshot
  ************************************
}


Type
   TSnapheaderView=Object(TListBox)
     snaphdrv:z80snaptype;
     function GetText(item:integer; maxlen:integer):string;virtual;
     constructor init(var Bounds:TRect; ANumCols:
      Integer; AScrollBar: PScrollBar; snp:z80snaptype);
   end;
   PSnapHeaderView=^TSnapHeaderView;

constructor TSnapHeaderView.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar;  snp:z80snaptype);
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  snaphdrv:=snp;
end;

function TSnapHeaderView.GetText(item:integer; maxlen:integer):string;
type convtip=array[0..85] of byte;
var number,st:string;

  function this:byte;
  begin
     this:=convtip(snaphdrv)[item]
  end;

  function is48Ksnap:  boolean;
  begin
    with snaphdrv do is48Ksnap:= ((z80addhdr=23) and (z80hrdmode<=2)) or
            ((z80addhdr=54) and (z80hrdmode<=3)) or
            (z80oldpc<>0);
  end;

  function isint1: Boolean;
  begin
    isint1:=false;
    with snaphdrv do begin
      if (z80hrdmode in [1,2,4]) and (z80addhdr=23) then isint1:=true;
      if (z80hrdmode in [1,2,5]) and (z80addhdr=54) then isint1:=true;
    end
  end;

begin
  str(item:3,number);
  st:=' '+number+':';
  str(convtip(snaphdrv)[item]:3,number);
  st:=st+number+' ';
  case item of
    0: st:=st+'(F)';
    1: st:=st+'(A)';
    2: st:=st+'(C)';
    3: st:=st+'(B)';
    4: st:=st+'(L)';
    5: st:=st+'(H)';
    6: if snaphdrv.z80oldpc<>0 then st:=st+'(PC - low)' else
          st:=st+'(Warajevo or Z80 2.0+ snapshot)';
    7: if snaphdrv.z80oldpc<>0 then st:=st+'(PC - high)' else
          st:=st+'(Warajevo or Z80 2.0+ snapshot)';
    8: st:=st+'(SP - low)';
    9: st:=st+'(SP - high)';
   10: st:=st+'(I)';
   11: st:=st+'(R without highest bit )';
   12: begin
     str((this and 1):1,number);
     st:=st+'(highest bit of R is '+number+' , BORDER=';
     str(((this shr 1) and 1):1,number);
     st:=st+number;
     if snaphdrv.z80oldpc<>0 then begin
       if (this and 16)<>0 then begin
         st:=st+', SAMROM on';
         st[1]:='*'
       end;
       if (this and 32)<>0 then st:=st+', compressed';
     end;
     st:=st+')';
   end;
   13: st:=st+'(E)';
   14: st:=st+'(D)';
   15: st:=st+'(C'')';
   16: st:=st+'(B'')';
   17: st:=st+'(E'')';
   18: st:=st+'(D'')';
   19: st:=st+'(L'')';
   20: st:=st+'(H'')';
   21: st:=st+'(A'')';
   22: st:=st+'(F'')';
   23: st:=st+'(YL)';
   24: st:=st+'(YH)';
   25: st:=st+'(XL)';
   26: st:=st+'(XH)';
   27: begin
      st:=st+'(IFF1 ';
      if this = 0 then st:=st+' disabled)' else st:=st+' enabled)'
     end;
   28: begin
      st:=st+'(IFF2 ';
      if this = 0 then st:=st+' disabled)' else st:=st+' enabled)'
     end;
   29: st:=st+'(interrupt mode)';
   30: begin
     st:=st+'(size of extra header low -';
     if this=23 then st:=st+' Lunter Z80 2.0 or Warajevo)';
     if this=54 then st:=st+' Lunter Z80 3.0)';
   end; 
   31: st:=st+'(size of extra header high)';
   32: st:=st+'(PC - low)';
   33: st:=st+'(PC - high)';
   34: if snaphdrv.z80hrdmode=128 then st:=st+'(Timex 2068)'
      else begin
      if snaphdrv.z80addhdr=23 then begin
        case snaphdrv.z80hrdmode of
          0: st:=st+'(Spectrum 48K)';
          1: st:=st+('Spectrum 48K+Interface 1');
          2: begin
            st:=st+'(Spectrum 48K+Interface 1+SAMRAM)';
            st[1]:='*';
          end;
          3: st:=st+'(Spectrum 128K)';
          4: st:=st+'(Spectrum 128K+Interface 1)';
        end;
      end;
      if snaphdrv.z80addhdr=54 then begin
        case snaphdrv.z80hrdmode of
          0: st:=st+'(Spectrum 48K)';
          1: st:=st+'(Spectrum 48K+Interface 1)';
          2: begin
            st:=st+'(Spectrum 48K+Interface 1+SAMRAM)';
            st[1]:='*';

          end;
          3: begin
            st:=st+'(Spectrum 48K+Disciple)';
            st[1]:='*';

          end;
          4: st:=st+'(Spectrum 128K)';
          5: st:=st+'(Spectrum 128K+Interface 1)';
          6: begin
            st:=st+'(Spectrum 128K+ Disciple)';
            st[1]:='*';
          end;
        end;
      end;
   end;
   35: 
      if snaphdrv.z80hrdmode=128 then st:=st+'(horizontal memory configuration)' else
      if not(is48ksnap) then st:=st+'(memory configuration)'
         else st:=st+'(ignored in 48K snapshot)';
   36:
      if snaphdrv.z80hrdmode=128 then st:=st+'(Timex control register)' else
      begin
        if isint1 then begin
          if this=255 then st:=st+'(Interface 1 ROM active)'
            else st:=st+'(non-active Interface 1 ROM)';
        end
         else st:=st+'(ignored without interface 1)';
      end;
   37: if (this and 1)=1 then st:=st+'(full R emulation)'
     else st:=st+'(fast R emulation)';
   38: if is48ksnap then st:=st+'(ignored in 48K snapshot)'
        else st:=st+'(last selected sound chip)';
   39..54: begin
     if is48ksnap then st:=st+'(ignored in 48K snapshot)'
     else begin
       str(item-39:2,number);
       st:=st+'(Sound register '+number+')';
     end;
   end;
   55..85: st:=st+'(ignored data of Z80 3.0 snapshot)';
end;
{


      * 34      1       Hardware mode (see below)
      * 35      1       If in SamRam mode, bitwise state of 74ls259.
                        For example, bit 6=1 after an OUT 31,13 (=2*6+1)
                        If in 128 mode, contains last OUT to 7ffd

}
  gettext:=st;
end;


procedure TSnapshotFile.View;
var a: PDialog;
  R:TRect;
  skrolnica:PScrollBar;
  ll:PSnapheaderView;
  Bruce:Pointer;
  c:word;
  hdsize:integer;
begin
   if not(allowed) then exit;
   R.assign(5,2,75,22);
   a:=new(PDialog,Init(R,'View'));
   a^.helpctx:=hcsinfo;
   R.assign(68,1,69,15);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(55,16,65,18);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.assign(1,1,67,15);
   ll:=new(PSnapHeaderView,Init(R,1,skrolnica,snaphdr));
   case z80snptype of
    0: exit;
    1: hdsize:=30;
    2: hdsize:=30;
    3: hdsize:=55;
    4: hdsize:=86
   end;

   ll^.setrange(hdsize);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);

   dispose(a,done);
end;


procedure TSnapshotFile.EditSnap;
var a: PDialog;
  R:TRect;
  Bruce:Pointer;
  c,radio,startdisp,di:word;
  hdsize,i:integer;
  datoteka,datoteka2:file;
  cc:Boolean;
  d,n,e:string;

  procedure we;
  begin
    getheader;
    cc:=fclose(datoteka);
    cc:=fclose(datoteka2);
    exit
  end;

begin
   if not(allowed) then exit;
   case z80snptype of
     0: exit;
     1,2: begin
       c:=messagebox('This snapshot file will be upgraded'+#13+
          ^C'to Warajevo compatible Z80 format before editing',
          nil, mfOKButton+mfCancelButton+mfWarning);
       if c<>cmOK then begin getheader;exit end;
       getheader;
       load48k;
       with snaphdr do begin
         z80oldpc:=0;
         z80addhdr:=23;
         z80hrdmode:=0;
         z80mmu:=0;
         z80if1paged:=0;
         z80rldir:=0;
         z80fffd:=0;
         for i:=0 to 15 do z80sndreg[i]:=0;
       end;
       savehdrand48k(55);
       hdsize:=55;
       z80snptype:=3;
     end;
     3: hdsize:=55;
     4: hdsize:=86
   end;
   if not(is48ksnap) and not(istimex) then begin
     R.assign(20,4,64,18);
     a:=new(PDialog,Init(R,'Select page'));
     a^.helpctx:=hceditmeme;
     R.Assign(5,10,15,12);
     Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
     a^.insert(Bruce);
     R.Assign(20,10,30,12);
     Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
     a^.insert(Bruce);
     R.Assign(1,1, 43,9);
     Bruce := New(PRadioButtons, Init(R,
       NewSItem('Page 0 (49152-65535)',
       NewSItem('Page 1 (49152-65535)',
       NewSItem('Page 2 (49152-65535 or 32768-49152)',
       NewSItem('Page 3 (49152-65535)',
       NewSItem('Page 4 (49152-65535)',
       NewSItem('Page 5 (49152-65535 or 16384-32767)',
       NewSItem('Page 6 (49152-65535)',
       NewSItem('Page 7 (49152-65535)',

       nil))))))))
      ));
     a^.Insert(Bruce);

     c:=desktop^.execview(a);
     hourglascur;
     a^.getdata(radio);
     dispose(a,done);
   end
    else c:=cmOK;
   if c=cmOK then begin
     hourglascur;
     if (is48ksnap) or (istimex) then begin
       getheader;
       load48k;
       if hexeditor(uncompressed,49152,16384)=cmOK then
         savehdrand48k(hdsize)
     end
      else begin
        load128kpage(radio+3);
        startdisp:=49152;
        if radio=2 then startdisp:=32768;
        if radio=5 then startdisp:=16384;
        if hexeditor(addr(uncompressed^[49152])
          ,16384,startdisp)<>cmOK then begin
          getheader;exit
        end;
        fsplit(snapname,d,n,e);
        assign(datoteka,snapname);
        if fileexists(n+'.BAK') then begin
          assign (datoteka2,n+'.BAK');
          if not(ferase(datoteka2)) then begin we;exit end;
        end;
        if not(frename(datoteka,n+'.BAK')) then begin
           we; exit
        end;
        assign(datoteka2,snapname);
        if not(fresetu(datoteka,1)) then begin we; exit end;
        if not(frewriteu(datoteka2,1)) then begin we; exit end;
        if not(fblockread(datoteka,compressed^,hdsize))then begin
          we;exit
        end;
        if not(fblockwrite(datoteka2,compressed^,hdsize)) then begin
          we; exit
        end;
        while not eof(datoteka) do begin
          if not(fblockread(datoteka,z80pagehdr,3)) then begin we; exit end;
          if not(fblockread(datoteka,compressed^,
           z80pagehdr.datalength)) then begin
            we; exit
          end;

          if z80pagehdr.pagenumber=radio+3 then begin
            di:=0;
            compsnap(49152,65535,di);
            z80pagehdr.datalength:=di;
          end;
          if not(fblockwrite(datoteka2, z80pagehdr,3)) then begin
            we; exit
          end;
          if not(fblockwrite(datoteka2,compressed^,
            z80pagehdr.datalength)) then begin
            we; exit
          end;
       end;
       if not(fclose(datoteka)) then begin we; exit end;
       if not(fclose(datoteka2)) then begin we; exit end;
      end
   end
end;

procedure TSnapshotFile.snapshottotap;
const
  setaf=$18b;
  setafp=$189;
  setbc=$185;
  setbcp=$dd;
  setbor=$174;
  setde=$181;
  setdep=$e0;
  sethl=$183;
  sethlp=$da;
  seti=$e8;
  setiff=$16f;
  setim=$16e;
  setix=$187;
  setiy=$e5;
  setmix=$17c;
  setmmu=$180;
  setpc=$171;
  setr=$17e;
  setsnd=$13c;
  setsp=$16b;
  settip=441;
  setname=451;
  setrstek1=23915-23755;
  setrstek2=24018-23755;
  setrstek3=24041-23755;
  setvelika=23964-23755;
  setmala1=23991-23755;
  setmala2=24021-23755;
  setmala3=24044-23755;
  set1366=24057-23755;
  setnodisciple=24320-23755;
  setwafausr=24299-23755;
  setincname=23897-23755;
  loader:array[0..575] of byte=(

    $00,$01,$A2,$01,$EA,$0D,$FF,$18,
    $00,$21,$1F,$00,$09,$5E,$23,$56,
    $23,$7A,$B3,$28,$2B,$EB,$09,$D5,
    $E5,$5E,$23,$56,$EB,$09,$EB,$E1,
    $73,$23,$72,$E1,$18,$E7,$96,$00,
    $A8,$00,$BF,$00,$F2,$00,$06,$01,
    $0C,$01,$5A,$00,$7B,$00,$80,$00,
    $91,$00,$C7,$00,$4F,$01,$00,$00,
    $DD,$2A,$4B,$5C,$DD,$7E,$04,$FE,
    $30,$28,$04,$DD,$36,$04,$31,$DD,
    $7E,$03,$FE,$34,$28,$05,$3E,$39,
    $DD,$77,$03,$DD,$86,$04,$D6,$61,
    $32,$4E,$01,$C5,$E1,$23,$36,$68,
    $DD,$7E,$05,$D6,$30,$4F,$06,$00,
    $C9,$3E,$02,$CD,$01,$16,$3E,$16,
    $D7,$AF,$D7,$AF,$D7,$DD,$2A,$4B,
    $5C,$2A,$4F,$01,$7E,$23,$22,$4F,
    $01,$32,$5C,$5B,$DD,$34,$12,$DD,
    $7E,$05,$D6,$30,$4F,$06,$00,$21,
    $4E,$01,$35,$C0,$21,$6C,$01,$11,
    $E4,$57,$01,$1A,$00,$ED,$B0,$DD,
    $7E,$03,$FE,$34,$28,$17,$21,$35,
    $01,$AF,$01,$FD,$FF,$ED,$79,$01,
    $FD,$BF,$56,$ED,$51,$23,$3C,$FE,
    $10,$20,$EF,$18,$08,$21,$55,$01,
    $36,$00,$23,$36,$00,$21,$51,$01,
    $11,$E5,$56,$01,$1B,$00,$ED,$B0,
    $F3,$21,$00,$00,$01,$00,$00,$11,
    $00,$00,$D9,$FD,$21,$00,$00,$3E,
    $00,$ED,$47,$11,$E0,$56,$01,$05,
    $00,$DD,$7E,$04,$FE,$30,$28,$14,
    $21,$2B,$01,$ED,$B0,$21,$00,$40,
    $11,$00,$5B,$01,$00,$08,$31,$E4,
    $57,$C3,$E0,$56,$21,$30,$01,$ED,
    $B0,$DD,$21,$06,$00,$11,$11,$00,
    $AF,$CD,$56,$05,$F3,$31,$E4,$57,
    $21,$E0,$56,$E5,$DD,$21,$00,$5B,
    $11,$00,$08,$37,$3E,$FF,$08,$C3,
    $62,$05,$ED,$B0,$C1,$ED,$41,$00,
    $C1,$ED,$41,$D0,$00,$00,$00,$00,
    $00,$00,$00,$00,$00,$00,$00,$00,
    $00,$00,$00,$00,$10,$11,$13,$14,
    $16,$17,$10,$10,$10,$01,$45,$01,
    $0D,$F1,$E1,$45,$ED,$61,$30,$F9,
    $ED,$4F,$D1,$E1,$C1,$DD,$E1,$F1,
    $08,$F1,$31,$00,$00,$ED,$56,$FB,
    $C3,$00,$00,$FE,$00,$00,$00,$FF,
    $07,$00,$00,$BF,$00,$01,$00,$7F,
    $00,$00,$00,$00,$00,$00,$00,$00,
    $00,$00,$00,$00,$00,$23,$56,$7B,
    $B2,$28,$07,$21,$FB,$FF,$19,$41,
    $41,$41,$41,$41,$41,$41,$41,$41,
    $41,$41,$41,$41,$41,$0D,$00,$02,
    $31,$00,$FD,$B0,$22,$32,$35,$33,
    $34,$33,$22,$3A,$F1,$78,$24,$3D,
    $22,$34,$30,$34,$22,$3A,$F1,$6E,
    $24,$3D,$22,$20,$20,$20,$20,$20,
    $20,$20,$20,$20,$30,$22,$3A,$DA,
    $B0,$22,$38,$22,$3A,$D9,$B0,$22,
    $38,$22,$0D,$00,$03,$19,$00,$EC,
    $C0,$B0,$22,$37,$2B,$BE,$32,$33,
    $36,$33,$35,$2B,$32,$35,$36,$2A,
    $BE,$32,$33,$36,$33,$36,$22,$0D,
    $00,$04,$08,$00,$EF,$6E,$24,$AF,
    $3A,$EC,$A7,$0D,$00,$05,$10,$00,
    $EF,$2A,$22,$6D,$22,$3B,$BC,$A7,
    $3B,$6E,$24,$AF,$3A,$EC,$A7,$0D,
    $00,$06,$15,$00,$F9,$C0,$B0,$22,
    $31,$35,$33,$36,$33,$22,$3A,$EA,
    $3A,$EF,$6E,$24,$AF,$3A,$EC,$A7,
    $0D,$00,$07,$0B,$00,$EF,$44,$31,
    $2C,$6E,$24,$AF,$3A,$EC,$A7,$0D

  );
  pagniz:array[1..6] of integer=(3,4,  6,7,  9,10);

var pozicija:longint;
  datoteka:file;
  naziv,tapename,d,n,e:string;
  i,pages:integer;
  zaglavlje:record
    tip:byte;
    ime:array[1..10] of char;
    duz,w2,w3: word
  end;
  cc:Boolean;
  setzag:byte;
var
  DataRec : record
    destdevice : Word; {RadioButtons}
    routaddr : LongInt; {Inputlong}
    stackaddr : LongInt; {Inputlong}
    end;


  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;


procedure we;
begin
  cc:=fclose(datoteka);
  getheader;
end;

Begin
    with DataRec do begin
      destdevice:=0;
      routaddr:=22240;
      stackaddr:=22500;
    end;


  R.Assign(3,2,77,22);
  New(Dlg, Init(R, 'Snapshot to tape'));
  dlg^.helpctx:=hcsnaptap;
  R.Assign(3,3,72,10);
  Control := New(PRadiobuttons, Init(R,
    NewSItem('~U~sable on Warajevo and original machine (lowest screen damage)',
    NewSItem('Ready to convert to ~Z~80 emulator (medium screen damage)',
    NewSItem('Ready to convert to ~P~olish emulator (big screen damage)',
    NewSItem('Prepared for copy to ~M~icrodrive or Opus',
    NewSItem('Prepared for copy to ~W~afadrive',
    NewSItem('Prepared for copy to ~D~isciple or Plus D',
    NewSItem('Prepared for copy to ~T~R-DOS',
    Nil)))))))));

  Dlg^.Insert(Control);

  R.Assign(26,2,47,3);
  Dlg^.Insert(New(PLabel, Init(R, '~P~urpose of tape file', Control)));

  R.Assign(29,11,37,12);
  Control := New(PNumInputLine, Init(R, 6, 16384, 65535));
  Dlg^.Insert(Control);

  R.Assign(4,11,28,12);
  Dlg^.Insert(New(PLabel, Init(R, 'Startup ~R~outine address', Control)));

  R.Assign(36,13,44,14);
  Control := New(PNumInputLine, Init(R, 6, 16384, 65535));
  Dlg^.Insert(Control);

  R.Assign(4,13,35,14);
  Dlg^.Insert(New(PLabel, Init(R, 'Startup register ~S~tack address', Control)));
  R.Assign(13,15,25,17);
  Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(39,15,51,17);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  Dlg^.Setdata(datarec);
  if desktop^.execview(dlg)= cmcancel then begin
    dispose(dlg,done);
    exit;
  end;
  dlg^.getdata(datarec);
  dispose(dlg,done);
  fsplit(snapname,d,n,e);
  tapename:=outputfile(n+'.TAP',snapname);
  hourglascur;
  if tapename<>'' then begin
    pozicija:=4;
    load48k;
    if is48ksnap then warning48;
    naziv:=n+'          ';
    if datarec.destdevice<>6 then  for i:=1 to 9 do
       loader[setname-1+i]:=ord(naziv[i])
     else begin
       loader[setname-1]:=32;
       loader[setname]:=32;
       loader[setname+1]:=34;
       for i:=1 to 7 do loader[setname+1+i]:=ord(naziv[i])
     end;
    zaglavlje.tip:=0;
    for i:=1 to 10 do zaglavlje.ime[i]:=naziv[i];
    zaglavlje.duz:=576;
    zaglavlje.w2:=1;
    zaglavlje.w3:=576;
    assign(datoteka,tapename);
    if not(frewriteu(datoteka,1)) then begin we; exit end;
    if not(fblockwrite(datoteka,prazna,12)) then begin we; exit end;
    if not(fclose(datoteka)) then begin we; exit end;
    if not(save(true,tapename,17,4,0,zaglavlje)) then begin we; exit end;
    if not(is48ksnap) then loader[settip]:=ord('8');
    setzag:=10;
    case datarec.destdevice of
      0: begin loader[settip+1]:=ord('0');loader[settip+2]:=ord('4'); end;
      1: begin
        loader[settip+1]:=ord('0');
        loader[settip+2]:=ord('4');
        loader[set1366]:=0;
        loader[set1366+1]:=195;
        loader[set1366+2]:=86;
        loader[set1366+3]:=5;
      end;
      2: begin loader[settip+1]:=ord('1');loader[settip+2]:=ord('4'); end;
      3: begin loader[settip+1]:=ord('1');loader[settip+2]:=ord('5'); end;
      4: begin
        loader[settip+1]:=ord('1');
        loader[settip+2]:=ord('6');
        for i:=0 to 7 do loader[setnodisciple+i]:=32;
        end;
      5: begin loader[settip+1]:=ord('1');loader[settip+2]:=ord('7'); end;
      6: begin setzag:=8;
          loader[settip+1]:=ord('1');loader[settip+2]:=ord('6');
          for i:=0 to 7 do loader[setnodisciple+i]:=32;
          for i:=9 downto 7 do loader[setnodisciple-i]:=32;
          loader[setnodisciple-6]:=175;
          loader[setwafausr+2]:=ord('6');
          loader[setwafausr+3]:=ord('1');
          loader[setwafausr+4]:=ord('9');
          loader[setincname]:=16;
       end;
    end;
    with datarec do begin
      loader[setrstek1]:=lo(stackaddr);
      loader[setrstek1+1]:=hi(stackaddr);
      loader[setrstek2]:=lo(stackaddr);
      loader[setrstek2+1]:=hi(stackaddr);
      loader[setrstek3]:=lo(stackaddr);
      loader[setrstek3+1]:=hi(stackaddr);

      loader[setvelika]:=lo(routaddr+5);
      loader[setvelika+1]:=hi(routaddr+5);
      loader[setmala1]:=lo(routaddr);
      loader[setmala1+1]:=hi(routaddr);
      loader[setmala2]:=lo(routaddr);
      loader[setmala2+1]:=hi(routaddr);
      loader[setmala3]:=lo(routaddr);
      loader[setmala3+1]:=hi(routaddr);
    end;
    with snaphdr do begin
      loader[setaf]:=z80f;
      loader[setaf+1]:=z80a;
      loader[setafp]:=z80fp;
      loader[setafp+1]:=z80ap;
      loader[setbc]:=lo(z80bc);
      loader[setbc+1]:=hi(z80bc);
      loader[setde]:=lo(z80de);
      loader[setde+1]:=hi(z80de);
      loader[sethl]:=lo(z80hl);
      loader[sethl+1]:=hi(z80hl);
      loader[setix]:=lo(z80ix);
      loader[setix+1]:=hi(z80ix);
      loader[setiy]:=lo(z80iy);
      loader[setiy+1]:=hi(z80iy);
      loader[setbcp]:=lo(z80bcp);
      loader[setbcp+1]:=hi(z80bcp);
      loader[setdep]:=lo(z80dep);
      loader[setdep+1]:=hi(z80dep);
      loader[sethlp]:=lo(z80hlp);
      loader[sethlp+1]:=hi(z80hlp);
      loader[setpc]:=lo(z80pc);
      loader[setpc+1]:=hi(z80pc);
      loader[setsp]:=lo(z80sp);
      loader[setsp+1]:=hi(z80sp);
      loader[seti]:=z80i;
      loader[setmmu]:=z80mmu;
      loader[setr]:=z80r-13;
      if (z80stat1 and 1)<>0 then loader[setr]:=loader[setr] or 128
        else loader[setr]:=loader[setr] and $7F;
      for i:=0 to 15 do loader[setsnd+i]:=z80sndreg[i];
      loader[setsnd+7]:=0;
      loader[setmix]:=z80sndreg[7];
      loader[setbor]:=(z80stat1 div 2) and 7;
      if z80iff1=0 then loader[setiff]:=$F3 {DI}
       else loader[setiff]:=$FB;  {EI}
      if (z80stat2 and 3)=2 then loader[setim]:=$5e {IM 2}
       else loader[setim]:=$56;
    end;
    if not(save(true,tapename,zaglavlje.duz,4,255,loader)) then begin we; exit end;
    zaglavlje.ime[setzag]:='0';
    zaglavlje.tip:=3;
    zaglavlje.duz:=23808;
    zaglavlje.w2:=25344;
    if not(save(true,tapename,17,4,0,zaglavlje)) then begin we; exit end;
    if not(save(true,tapename,23808,4,255,uncompressed^[25344])) then begin
      we; exit
    end;
    if is48ksnap then pages:=1 else pages:=6;
    for i:=1 to pages do begin
      if not(is48ksnap) then load128kpage(pagniz[i]);
      inc(zaglavlje.ime[setzag]);
      zaglavlje.tip:=3;
      zaglavlje.duz:=16384;
      zaglavlje.w2:=49152;
      if not(save(true,tapename,17,4,0,zaglavlje)) then begin we; exit end;
      if not(save(true,tapename,16384,4,255,uncompressed^[49152])) then begin
        we; exit
      end;
    end;
    inc(zaglavlje.ime[setzag]);
    zaglavlje.tip:=3;
    zaglavlje.duz:=6912;
    zaglavlje.w2:=16384;
    if not(save(true,tapename,17,4,0,zaglavlje)) then begin we; exit end;
    if not(save(true,tapename,6912,4,255,uncompressed^[16384])) then begin
      we; exit
    end;

    inc(zaglavlje.ime[setzag]);
    zaglavlje.tip:=3;
    zaglavlje.duz:=2048;
    zaglavlje.w2:=16384;
    if not(save(true,tapename,17,4,0,zaglavlje)) then begin we; exit end;
    if not(save(true,tapename,2048,4,255,uncompressed^[23296])) then begin
      we; exit
    end;
  end;
end;

procedure TSnapshotFile.slttoZ80TAP;
type TBlock=record
    tip:word;
    blkid:word;
    size:longint;
  end;

var trenutni,poredbeni:array[1..6] of char;
  datoteka,izlaz:file;
  i:integer;
  pozicija:longint;
  sltfilename,TAPFileName,Z80FileName,d,n,e:string;
  blokovi:array[1..1000] of TBlock;
  tb:TBlock;
  a:PFIleDialog;
  izadji,cc:Boolean;
  trenblok,di:word;

  procedure we;
  begin
    cc:=fclose(datoteka);
    cc:=fclose(izlaz);
  end;
  procedure we1;
  begin
    cc:=fclose(datoteka);
  end;

begin
  a:=new(PFileDIalog,Init('*.SLT','Select file to be converted','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcx128wara;
  fillchar(blokovi,sizeof(blokovi),0);
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(SLTfilename);
    fsplit(SLTFilename,d,n,e);
    dispose(a,done);
  end
   else begin
     dispose(a,done);
     exit
   end;
  TAPFileName:=OutputFile(n+'.TAP',snapname);
  Z80FileName:=OutputFile(n+'.Z80',snapname);
  hourglascur;
  poredbeni[1]:=#0;
  poredbeni[2]:=#0;
  poredbeni[3]:=#0;
  poredbeni[4]:='S';
  poredbeni[5]:='L';
  poredbeni[6]:='T';
  for i:=1 to 6 do trenutni[i]:=#0;
  assign(datoteka,SLtFileName);
  assign(izlaz,z80filename);
  if not(fresetu(datoteka,1)) then exit;
  if not(frewriteu(izlaz,1)) then begin cc:=fclose(datoteka); exit end;
  pozicija:=0;
  izadji:=false;
  while not (eof(datoteka)) and not(izadji) do begin
    for i:=1 to 5 do trenutni[i]:=trenutni[i+1];
    if not(fblockread(datoteka,trenutni[6],1)) then begin we; exit end;
    if not(fblockwrite(izlaz,trenutni[6],1)) then begin we; exit end;
    inc(pozicija);
    if trenutni=poredbeni then izadji:=true;
  end;
  if not(fseek(izlaz,pozicija-6)) then begin we; exit end;
  {$I-}
  truncate(izlaz);
  {$I+}
  if ioresult<>0 then begin we; exit end;
  if not(fclose(izlaz)) then begin we1; exit end;
  trenblok:=0;
  repeat
    if not(fblockread(datoteka,tb,8)) then begin we1; exit end;
    if trenblok<tb.blkid then begin
      trenblok:=tb.blkid;
    end;
    if tb.tip=0 then inc(trenblok);
    blokovi[trenblok]:=tb;
    inc(pozicija,8);
  until blokovi[trenblok].tip=0;
  assign(izlaz,TAPFileName);
  if not(frewriteu(izlaz,1)) then begin we1; exit end;
  if not(fblockwrite(izlaz,prazna,12)) then begin we; exit end;
  if not(fclose(izlaz)) then begin we1; exit end;
  for i:=1 to trenblok do begin
    if blokovi[i].size=0 then begin
      if not(save(true,tapfilename,1,4,i,uncompressed^[16384])) then begin
        we; exit
      end;
    end
    else begin
      if blokovi[i].tip=1 then begin
        if not(fblockread(datoteka,compressed^,blokovi[i].size)) then begin
          we; exit
        end;
        di:=16384;
        decompsnap(0,blokovi[i].size,di);
        if not(save(true,tapfilename,di-16384-1,4,lo(blokovi[i].blkid),uncompressed^[16384])) then begin
          we; exit
        end;

      end
        else begin
          seek(datoteka,filepos(datoteka)+blokovi[i].size);
        end;
    end
  end;
  cc:=fclose(datoteka);
end;
{**********************************
  SNAPSHOT & TAP -> SLT
 **********************************}
procedure TSnapshotFile.Z80TAPtoSLT;

const signatura:array[1..6] of char=(#0,#0,#0,'S','L','T');
var
FromF, ToF: file;
NumRead, NumWritten, duzina,di: Word;
filename,d,n,e:string;
buf: array[1..2048] of Char;
cc:boolean;
i:integer;
id:record
  tip:word;
  idword:word;
  size:longint;
end;

begin
  with Radstrakom do begin
  fsplit(snapname,d,n,e);
  filename:=outputfile(n+'.SLT',snapname);
  hourglascur;
  if filename='' then exit;
  Assign(FromF, snapname);
  if not(fResetu(FromF, 1)) then exit;
  Assign(ToF, filename);
  if not(fRewriteu(ToF, 1)) then begin cc:=fclose(fromf); exit end;
  repeat
    BlockRead(FromF,buf,SizeOf(buf),NumRead);
    BlockWrite(ToF,buf,NumRead,NumWritten);
  until (NumRead = 0) or (NumWritten <> NumRead);
  cc:=fClose(FromF);
  if not(fBlockWrite(ToF,signatura,6))
    then begin cc:=fclose(fromf);cc:=fclose(tof); exit end;
  for i:=1 to noofblocks do begin
    if not(load(true,tapename,blocks^[i].pocetak,buffer^)) then begin
      cc:=fclose(toF);exit
    end;
    (* Specijalno ako je duzina 65535 *)
    if blocks^[i].duzina1=65534 then begin
      DisplayError('Sample can not be converted',0);
      cc:=fclose(toF);exit
    end;
    if blocks^[i].duzina1=65535 then begin
       decompress(ptr(seg(buffer^),ofs(buffer^)+6)
        ,izlbuffer,blocks^[i].duzdec,
       blocks^[i].duzcomp,blocks^[i].duzsign);
       duzina:=blocks^[i].duzdec-1;
       move(izlbuffer^,buffer^,duzina)
    end
     else duzina:=blocks^[i].duzina1;
    uncompressed:=buffer;
    compressed:=izlbuffer;
    di:=0;
    compsnap(16384,16384+duzina-1,di);
    duzina:=di;
    id.tip:=1;
    id.idword:=i;
    id.size:=duzina;
    if not(fblockwrite(tof,id,8)) then begin cc:=fclose(tof);exit end;
  end;
  id.tip:=0;
  id.idword:=0;
  id.size:=0;
  if not(fblockwrite(tof,id,8)) then begin cc:=fclose(tof);exit end;
  for i:=1 to noofblocks do begin
    if not(load(true,tapename,blocks^[i].pocetak,buffer^)) then begin
      cc:=fclose(toF);exit
    end;
    (* Specijalno ako je duzina 65535 *)
    if blocks^[i].duzina1=65535 then begin
       decompress(ptr(seg(buffer^),ofs(buffer^)+6)
        ,izlbuffer,blocks^[i].duzdec,
       blocks^[i].duzcomp,blocks^[i].duzsign);
       duzina:=blocks^[i].duzdec-1;
       move(izlbuffer^,buffer^,duzina)
    end
     else duzina:=blocks^[i].duzina1;
    uncompressed:=buffer;
    compressed:=izlbuffer;
    di:=0;
    compsnap(16384,16384+duzina-1,di);
    duzina:=di;
    if not(fblockwrite(tof,izlbuffer^,duzina)) then begin
      cc:=fclose(tof);
    exit end;
  end;

  cc:=fClose(ToF);
  end;
end;



end.

