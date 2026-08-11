{$X+}
unit start;
{Implements start button}
{$O+,V-}
interface
uses snapshot;
  procedure preparestart(var defaultfile:string; var defaulttype:integer; radssnapom:PSnapshotFile);
implementation
uses OPNdlg,iorutine,app,views,dos,setup,konvtap,dialogs,objects,helpctx,moucur;
type
  othersnaptype=record
    case integer of
    1: (
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
    2: (
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
    3: (
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
    );
    4: (
      snpf,snpa:byte;
      snpres,snpbc,snpde,snphl,
      snppc,snpsp,snpix,snpiy:word;
      snpiff2,snpiff1,snpim,snpr,snpi:byte;
      snpfp,snpap:byte;
      snpbcp,snpdep,snphlp:word;
    );
    5: (
      sitbc,sitde,sithl:word;
      sitf,sita:byte;
      sitix,sitiy,sitsp,sitpc:word;
      sitr,siti:byte;
      sitbcp,sitdep,sithlp:word;
      sitfp,sitap:byte;
      sitinter,sitborder:byte;
    );
    6: (
      sema,semf:byte;
      sembc,semde,semhl:word;
      semap,semfp:byte;
      sembcp,semdep,semhlp:word;
      sempc,semsp,semix,semiy:word;
      semi,semr,semiff1,semiff2,semim:word;
    );
    7: (
      riffa,rifff:byte;
      riffbc,riffde,riffhl:word;
      riffap,rifffp:byte;
      riffbcp,riffdep,riffhlp:word;
      riffix,riffiy,riffpc,riffsp:word;
      riffi,riffr,riffiff1,riffiff2,riffim:byte;
      dwticks:longint;
      riffunk:byte;
    );
   end;
  memorija1=array[0..65519] of byte;
  memorija2=array[16384..65535] of byte;

VAR  ZIPDefaultZIPOpts : STRING [16];
VAR  ZIPFileName       : STRING [50];
VAR  ZIPDPath          : STRING [50];

VAR  EraseCount        : WORD;
     EraseSizeK        : LONGINT;
     ShowOnWrite       : BOOLEAN;
     I29H              : POINTER;
{$F+}
PROCEDURE Int29Handler (AX, BX, CX, DX, SI, DI, DS, ES, BP : WORD); INTERRUPT;
VAR
  Dummy : BYTE;
BEGIN
  Asm
    Sti
  END;
  IF ShowOnWrite THEN WRITE (CHAR (LO (Ax) ) );
  Asm
    Cli
  END;
END;

PROCEDURE ReallocateMemory (P : POINTER); ASSEMBLER;
ASM
  MOV  AX, PrefixSeg
  MOV  ES, AX
  MOV  BX, WORD PTR P + 2
  CMP  WORD PTR P, 0
  JE   @OK
  INC  BX

 @OK :
  SUB  BX, AX
  MOV  AH, 4Ah
  INT  21h
  JC   @X
  LES  DI, P
  MOV  WORD PTR HeapEnd, DI
  MOV  WORD PTR HeapEnd + 2, ES
 @X :
END;


FUNCTION EXECUTE (Name : PathStr ; Tail : STRING) : WORD; ASSEMBLER;
ASM
  {$IFDEF CPU386}
  DB      66h
  PUSH    WORD PTR HeapEnd
  DB      66h
  PUSH    WORD PTR Name
  DB      66h
  PUSH    WORD PTR Tail
  DB      66h
  PUSH    WORD PTR HeapPtr
  {$ELSE}
  PUSH    WORD PTR HeapEnd + 2
  PUSH    WORD PTR HeapEnd
  PUSH    WORD PTR Name + 2
  PUSH    WORD PTR Name
  PUSH    WORD PTR Tail + 2
  PUSH    WORD PTR Tail
  PUSH    WORD PTR HeapPtr + 2
  PUSH    WORD PTR HeapPtr
  {$ENDIF}

  CALL ReallocateMemory
  CALL SwapVectors
  CALL DOS.EXEC
  CALL SwapVectors
  CALL ReallocateMemory
  MOV  AX, DosError
  OR   AX, AX
  JNZ  @OUT
  MOV  AH, 4Dh
  INT  21h
 @OUT :
END;
{$F-}

FUNCTION ExecuteCommand(p,s : STRING; quiet : BOOLEAN) : INTEGER;
var n:word;
BEGIN
ShowOnWrite := NOT quiet;   {turn NT 29 }
GETINTVEC ($29, I29H);
SETINTVEC ($29, @Int29Handler);         { Install interrupt handler }
n:=Execute(p,s);
SETINTVEC ($29, I29h);
IF DosError = 0 THEN ExecuteCommand := DosExitCode   ELSE ExecuteCommand := DosError;
END;

FUNCTION AddBackSlash (dName : STRING) : STRING;
BEGIN
  IF dName [LENGTH (dName) ] IN ['\', ':', #0] THEN
    AddBackSlash := dName
  ELSE
    AddBackSlash := dName + '\';
END;
{$I-}
FUNCTION EraseFile ( S : PathStr ) : BOOLEAN ;

VAR F : FILE;

BEGIN

EraseFile := FALSE;

ASSIGN (F, S);
RESET (F);

IF IORESULT<>  0 THEN EXIT;

  CLOSE (F);
  ERASE (F);
  EraseFile := (IORESULT = 0);

END;

FUNCTION FileExists ( S : PathStr ) : BOOLEAN ;

VAR F : FILE;

BEGIN

FileExists := FALSE;
FileMode:=0;
ASSIGN (F, S);
RESET (F);
FileMode:=2;
IF IORESULT<> 0 THEN EXIT; 

  CLOSE (F);
  FileExists := (IORESULT = 0);
END;

PROCEDURE CleanUpFile (WorkDir : STRING; SR : searchRec);
VAR l    : LONGINT;

BEGIN
    WITH SR DO BEGIN
        l := size DIV 512;
        IF (attr AND 31) = 0 THEN  BEGIN
            IF l = 0 THEN l := 1;
            EraseSizeK := EraseSizeK + l;
            EraseFile (AddBackSlash (WorkDir) + name);
            INC (EraseCount);
        END
    END;
END;

PROCEDURE CleanUpDir (WorkDir, FileMask : STRING);
VAR Frec : SearchRec;
    s    : STRING [64];
    BEGIN
    s := '';
    FINDFIRST (AddBackSlash (WorkDir) + FileMask, anyfile, Frec);
    WHILE doserror = 0 DO
        BEGIN
        CleanUpFile (WorkDir, Frec);
        FINDNEXT (Frec);
        END;
    END;


FUNCTION  UnZIPFile ( ZIPOpts, ZIPName, DPath, fspec : STRING; qt : BOOLEAN) : BOOLEAN;
VAR s, zname,PKUNZIP     : STRING;
    i, j,ziperror         : INTEGER;
    BEGIN
    PKUNZIP := FSearch('PKUNZIP.EXE',GetEnv('PATH'));
    UnZIPFile := TRUE;
    s := '';
    s := s + ZIPOpts;
    zname := ZIPName;
    IF NOT FileExists (zname) THEN
        BEGIN
        UnZIPFile := FALSE;
        EXIT;
    END;

    s := s + ' ' + zname;

    s := s + ' ' + DPath;
    s := s + ' ' + fspec;
    ZIPError := ExecuteCommand (PKUNZIP,s,qt);
    IF ZIPError > 0 THEN
         BEGIN

         DisplayError ('PKUNZIP start failed ',0);
         UnZIPFile := FALSE;
         END
    ELSE BEGIN
         i := POS ('*', fspec);
         j := POS ('?', fspec);
         IF (i = 0) AND (j = 0) THEN
             BEGIN
             IF NOT FileExists (DPath + fspec) THEN
                  BEGIN
                  UnZIPFile := FALSE;
                  ZIPError := 98;
                  END;
             END;
         END;
    END;
{$I+}
procedure mdrloader; forward;
procedure preparestart(var defaultfile:string; var defaulttype:integer; radssnapom:PSnapshotFile);
label izbor,filenamefound;
  type TBlock=record
      tip:word;
      blkid:word;
      size:longint;
    end;

var fd:PFilteredFileDIalog;
  filename,d,n,e,outputname,tmp,chunk:string;
  other:othersnaptype;
  datoteka,izlaz:file;
  cc,rifffile:Boolean;
  signature,fs:longint;
  duzina,i,t1,di:word;
  fleg,paritet,sigsize,sigread,border:byte;
  buffer,izlbuffer:Pointer;
  startaddr:array[0..7] of longint;
  curaddr,pozicija,size,upto:longint;
  ext:extstr;
  sr:Searchrec;
  trenutni,poredbeni:array[1..6] of char;
  sltfilename,TAPFileName,Z80FileName:string;
  blokovi:array[1..1000] of TBlock;
  tb:TBlock;
  a:PFilteredFIleDialog;
  izadji,obradi,dosnap:Boolean;
  trenblok:word;
  fmtzinfo:record wVersion,WHardModel,wHardFlags,wComprMeth:word end;
  tapeinfo:record block,pos,tsize:Longint end;

  procedure we;
  begin
    cc:=fclose(datoteka)
  end;
  procedure we2;
  begin
    cc:=fclose(datoteka);
    cc:=fclose(izlaz);
  end;
const
  prazna:array[1..12] of byte=(4,0,0,0,0,0,0,0,255,255,255,255);
type psearchrec=^tsearchrec;
begin
  buffer:=radssnapom^.uncompressed;
  izlbuffer:=radssnapom^.compressed;

izbor:
  defaulttype:=0;
  if defaultfile='' then begin
    fd:=new(PFilteredFileDIalog,Init('*.*','','~N~ame',
    ' .Z80 .TAP .BLK .SPC .LTP .TZX .ZXS .ZXT .VOC .SLT .SNA .SP .PRG'+
    ' .SEM .SIT .SNP .SCR .MDR .DCK .ZIP',
       fdOpenButton,100));
    fd^.helpctx:=hcsbutton;
    if desktop^.execview(fd)<>cmcancel then begin
      hourglascur;
      fd^.getfilename(filename);
      filename:=fexpand(filename);
      if not(iorutine.fileexists(filename)) then begin dispose(fd,done); exit end;
    end
     else begin dispose(fd,done); exit end;
    dispose(fd,done);
  end
   else filename:=defaultfile;

filenamefound:
  fsplit(filename,d,n,e);
  if e='.TZX' then begin
    outputname:=tempdir+'TEMP.TAP';
    assign(datoteka,outputname);
    if not(frewriteu(datoteka,1)) then begin we; exit; end;
    if not(fresetu(datoteka,1)) then begin we;exit;end;
    if not(fblockwrite(datoteka,prazna,12)) then begin; we; exit; end;
    cc:=fclose(datoteka);

    DoTZX(filename,outputname,buffer,izlbuffer,dosnap);
    defaultfile:=outputname;
    if dosnap then defaulttype:=3 else defaulttype:=1;
  end;
  if e='.VOC' then begin
    outputname:=tempdir+'TEMP.TAP';
    assign(datoteka,outputname);
    if not(frewriteu(datoteka,1)) then begin we; exit; end;
    if not(fresetu(datoteka,1)) then begin we;exit;end;
    if not(fblockwrite(datoteka,prazna,12)) then begin; we; exit; end;
    cc:=fclose(datoteka);

    DoVOC(filename,outputname,buffer,izlbuffer);
    defaultfile:=outputname;
    defaulttype:=1;
  end;
  if e='.TAP' then begin
    defaultfile:=filename;
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit; end;
    if filesize(datoteka)<12 then defaulttype:=5 else begin
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if signature<>$FFFFFFFF then begin
        defaulttype:=5
      end
        else defaulttype:=1;
    end;
    exit
  end;
  if (e='.LTP') or (e='.BLK') or (e='.SPC') then begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
    if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
    if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
    begin
      cc:=fclose(datoteka);
      outputname:=tempdir+'TEMP.TAP';
      assign(datoteka,outputname);
      if not(frewriteu(datoteka,1)) then begin we; exit; end;
      if not(fresetu(datoteka,1)) then begin we;exit;end;
      if not(fblockwrite(datoteka,prazna,12)) then begin; we; exit; end;
      cc:=fclose(datoteka);
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      while not eof(datoteka) do begin
        if not(fblockread(datoteka,duzina,2)) then begin we;exit;end;
        if (e='.SPC') or (e='.LTP') then duzina:=duzina+2;
        if (duzina<2) or (duzina>65500) then begin
          displayerror('Error in tape format',0);
          we; exit
        end;
        if not(fblockread(datoteka,fleg,1)) then begin we;exit;end;
        if not(fblockread(datoteka,buffer^,duzina-2))then begin we;exit;end;
        if not(fblockread(datoteka,paritet,1)) then begin we;exit;end;
        if not(save(true,outputname,duzina-2,4,fleg,buffer^)) then begin
          we;exit
        end;
      end;
      defaultfile:=outputname;
      defaulttype:=1;
      exit;
    end
  end;
  if e='.SLT' then with radssnapom^ do begin
    TAPFileName:=tempdir+'TEMP.TAP';
    Z80FileName:=tempdir+'TEMP.Z80';
    fillchar(blokovi,sizeof(blokovi),0);
    defaultfile:=TAPFileName;
    poredbeni[1]:=#0; poredbeni[2]:=#0; poredbeni[3]:=#0;
    poredbeni[4]:='S'; poredbeni[5]:='L'; poredbeni[6]:='T';
    for i:=1 to 6 do trenutni[i]:=#0;
    sltfilename:=filename;
    assign(datoteka,SLtFileName);
    assign(izlaz,z80filename);
    if not(fresetu(datoteka,1)) then exit;
    if not(frewriteu(izlaz,1)) then begin cc:=fclose(datoteka); exit end;
    pozicija:=0;
    izadji:=false;
    while not (eof(datoteka)) and not(izadji) do begin
      for i:=1 to 5 do trenutni[i]:=trenutni[i+1];
      if not(fblockread(datoteka,trenutni[6],1)) then begin we2; exit end;
      if not(fblockwrite(izlaz,trenutni[6],1)) then begin we2; exit end;
      inc(pozicija);
      if trenutni=poredbeni then izadji:=true;
    end;
    if not(fseek(izlaz,pozicija-6)) then begin we2; exit end;
    {$I-}
    truncate(izlaz);
    {$I+}
    if ioresult<>0 then begin we2; exit end;
    if not(fclose(izlaz)) then begin we; exit end;
    trenblok:=0;
    repeat
      if not(fblockread(datoteka,tb,8)) then begin we; exit end;
      if trenblok<tb.blkid then begin
        trenblok:=tb.blkid;
      end;
      if tb.tip=0 then inc(trenblok);
      blokovi[trenblok]:=tb;
      inc(pozicija,8);
    until blokovi[trenblok].tip=0;
    assign(izlaz,TAPFileName);
    if not(frewriteu(izlaz,1)) then begin we; exit end;
    if not(fblockwrite(izlaz,prazna,12)) then begin we2; exit end;
    if not(fclose(izlaz)) then begin we; exit end;
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
    defaulttype:=3;
  end;
  rifffile:=false;
  if (e='.ZXS') or (e='.ZXT') then begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    if filesize(datoteka)<4 then rifffile:=false
      else begin
        tmp[0]:=#4;
        if not(fblockread(datoteka,tmp[1],4)) then begin we; exit end;
        if tmp='RIFF' then rifffile:=true else rifffile:=false;
      end;
    if (e='.ZXS') and (not(rifffile)) then begin
      outputname:=tempdir+'TEMP.TAP';
      assign(datoteka,outputname);
      if not(frewriteu(datoteka,1)) then begin we; exit; end;
      if not(fresetu(datoteka,1)) then begin we;exit;end;
      if not(fblockwrite(datoteka,prazna,12)) then begin; we; exit; end;
      cc:=fclose(datoteka);

      DoMuseumWara(filename,outputname,buffer);
      defaultfile:=outputname;
      defaulttype:=1;
    end
  end;
  if e='.DCK' then
  begin
    defaultfile:=filename;
    defaulttype:=6;
  end;
  {$R+}
  if rifffile and ((e='.ZXS') or (e='.ZXT')) then with radssnapom^ do begin
    TAPFileName:=tempdir+'TEMP.TAP';
    Z80FileName:=tempdir+'TEMP.Z80';
    DefaultFile:=TAPFileName;
    assign(izlaz,TapFileName);
    if not(frewriteu(izlaz,1)) then exit;
    if not(fblockwrite(izlaz,prazna,12)) then begin cc:=fclose(izlaz); exit end;
    if not(fclose(izlaz)) then exit;
    assign(datoteka,filename);
    assign(izlaz,z80filename);
    if not(fresetu(datoteka,1)) then exit;
    if not(frewriteu(izlaz,1)) then begin we; exit end;
    size:=0;
    chunk[0]:=#4;
    if not(fblockread(datoteka,chunk[1],4)) then begin we2; exit end;
    if not(fblockread(datoteka,size,4)) then begin we2; exit end;
    if not(fblockread(datoteka,chunk[1],4)) then begin we2; exit end;
    if chunk<>'SNAP' then begin
      DisplayError('This is not ZX32 RIFF file',0); we2; exit
    end;
    with other do with snaphdr do begin
       z80oldpc:=0;z80addhdr:=23;z80hrdmode:=0;
       z80mmu:=0;z80if1paged:=0;z80rldir:=0; z80fffd:=0;
    end;
    if not(fblockwrite(izlaz,snaphdr,55)) then begin we2; exit end;
    while not eof(datoteka) do begin
      if not(fblockread(datoteka,chunk[1],4)) then begin we2; exit end;
      if not(fblockread(datoteka,size,4)) then begin we2; exit end;
      obradi:=true;
      if chunk='rZ80' then begin
        if not(fblockread(datoteka,other,34)) then begin we2; exit end;
        with other do with snaphdr do begin
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
        if not(fblockread(datoteka,uncompressed^[49152],16384)) then begin we2; exit end;
        di:=0;
        compsnap(49152,65535,di);
        z80pagehdr.datalength:=di;
        z80pagehdr.pagenumber:=i;
        if not(fblockwrite(izlaz, z80pagehdr,3)) then begin we2; exit end;
        if not(fblockwrite(izlaz,compressed^,di)) then begin we2; exit end;
        obradi:=false;
      end;
      if chunk='fmtz' then begin
        with snaphdr do begin
          if not(fblockread(datoteka,fmtzinfo,8)) then begin we2; exit end;
          if fmtzinfo.whardmodel=$30 then snaphdr.z80hrdmode:=3;
          if fmtzinfo.wComprMeth<>$FFFF then begin
            DisplayError('Compressed chunks are not available',0);
            we2; exit
          end;
        end;
        obradi:=false;
      end;

      if chunk='r128' then begin
        with snaphdr do begin
          if not(fblockread(datoteka,z80mmu,1)) then begin we2; exit end;
          if not(fblockread(datoteka,z80fffd,1)) then begin we2; exit end;
          if not(fblockread(datoteka,z80sndreg,16)) then begin we2; exit end;
          z80hrdmode:=3;
        end;
        obradi:=false;
      end;
      if chunk='r048' then begin
        with snaphdr do begin
          if not(fblockread(datoteka,border,1)) then begin we2; exit end;
          z80stat1:=lo((border and $38)*4+32);
          if not(fseek(datoteka,filepos(datoteka)+9)) then begin we2; exit end;
        end;
        obradi:=false;
      end;
      if chunk='tape' then begin
         if not(fblockread(datoteka,tapeinfo,12)) then begin we2; exit end;
         upto:=filepos(datoteka)+tapeinfo.tsize;
         while filepos(datoteka)<upto do begin
           if not(fblockread(datoteka,duzina,2)) then begin we2;exit;end;
           if not(fblockread(datoteka,fleg,1)) then begin we2;exit;end;
           if not(fblockread(datoteka,buffer^,duzina-2))then begin we2;exit;end;
           if not(fblockread(datoteka,paritet,1)) then begin we2;exit;end;
           if not(save(true,tapfilename,duzina-2,4,fleg,buffer^)) then begin
             we2;exit
           end;
         end;
        obradi:=false;
      end;

      if obradi then begin
         if not(fseek(datoteka,filepos(datoteka)+size)) then begin we2; exit end
      end
    end;
    if not(fseek(izlaz,0)) then begin we2; exit end;
    if not(fblockwrite(izlaz,snaphdr,55)) then begin we2; exit end;
    we2;
    if e='.ZXT' then defaulttype:=1 else defaulttype:=3;
    exit;
  end;

  if e='.SNA' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if fs=49179 then begin
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;

      if not(fblockread(datoteka,other,27)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      with other do with snaphdr do begin
        z80A:=snaa;z80f:=snaf;z80bc:=snabc;z80hl:=snahl;
        z80sp:=snasp;z80i:=snai;z80r:=snar;
        z80stat1:=snaborder*2+32;
        if (snar and 128)=0 then  z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=snade; z80bcp:=snabcp; z80dep:=snadep; z80hlp:=snahlp;
        z80ap:=snaap; z80fp:=snafp; z80iy:=snaiy; z80ix:=snaix;
        z80iff1:=snaiff2; z80iff2:=snaiff2; z80stat2:=snaimode and 3;
        z80pc:=114; {RETN} z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0;
        z80mmu:=0; z80if1paged:=0;z80rldir:=0;
        z80fffd:=0; tmp:=snapname; for i:=0 to 15 do z80sndreg[i]:=0;
        snapname:=tempdir+'TEMP.Z80';
        if snapname<>'' then savehdrand48k(55);
        defaulttype:=2;
        snapname:=tmp;
        exit;
      end {With}
    end;


    if (fs=131103) or(fs= 147487) then begin
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if not(fblockread(datoteka,other,27)) then begin we; exit end;
      with other do with snaphdr do begin
         z80A:=snaa;z80f:=snaf; z80bc:=snabc;  z80hl:=snahl;
         z80sp:=snasp; z80i:=snai; z80r:=snar; z80stat1:=snaborder*2+32;
         if (snar and 128)=0 then  z80stat1:=z80stat1 and $FE
          else z80stat1:=z80stat1 or 1;
         z80de:=snade; z80bcp:=snabcp; z80dep:=snadep; z80hlp:=snahlp;
         z80ap:=snaap; z80fp:=snafp; z80iy:=snaiy; z80ix:=snaix;
         z80iff1:=snaiff2; z80iff2:=snaiff2; z80stat2:=snaimode and 3;
         if not(fseek(datoteka,49179)) then begin we; exit end;
         if not(fblockread(datoteka,z80pc,2)) then begin we; exit end;
       end; {With}

       with snaphdr do begin
         t1:=z80hrdmode; z80oldpc:=0;
         if is48ksnap then begin
           z80mmu:=16;for i:=0 to 15 do z80sndreg[i]:=0;z80fffd:=0;
         end
          else begin
            if not(fseek(datoteka,49181)) then begin we; exit end;
            if not(fblockread(datoteka,z80mmu,1)) then begin we; exit end;
          end;
         z80addhdr:=23; z80hrdmode:=3; z80if1paged:=0; z80rldir:=0;
         tmp:=snapname; snapname:=tempdir+'TEMP.Z80';
         if snapname='' then begin snapname:=tmp; getheader; exit end;
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
       DefaultType:=2;
       exit;
    end
  end;
  if e='.SP' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if (fs=49190) or (fs=49184) then begin
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if fs=49190 then
        if not(fblockread(datoteka,other,6)) then begin we; exit end;
      if not(fblockread(datoteka,other,32)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      with other do with snaphdr do begin
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
        z80pc:=114; {RETN} z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0;
        z80mmu:=0; z80if1paged:=0;z80rldir:=0;
        z80fffd:=0; tmp:=snapname; for i:=0 to 15 do z80sndreg[i]:=0;
        snapname:=tempdir+'TEMP.Z80';
        if snapname<>'' then savehdrand48k(55);
        defaulttype:=2;
        snapname:=tmp;
        exit;
      end {With}
    end;
  end;
  if e='.SCR' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if (fs=6912) then begin
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^[16384],6912)) then begin
        we; exit
      end;
      uncompressed^[23296]:=24; {JR -2}
      uncompressed^[23297]:=254;
      if not(fclose(datoteka)) then begin we; exit end;
      with other do with snaphdr do begin
        z80pc:=23296;
        z80iff1:=0;
        z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0;
        z80mmu:=0; z80if1paged:=0;z80rldir:=0;
        z80fffd:=0; tmp:=snapname; for i:=0 to 15 do z80sndreg[i]:=0;
        snapname:=tempdir+'TEMP.Z80';
        if snapname<>'' then savehdrand48k(55);
        defaulttype:=2;
        snapname:=tmp;
        exit;
      end {With}
    end;
  end;

  if e='.PRG' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if fs=49152+256 then begin
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if not(fblockread(datoteka,other,256)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin we;exit end;
      if not(fclose(datoteka)) then begin we; exit end;
      with other do with snaphdr do begin
        z80A:=uncompressed^[prgsp+3];
        z80f:=uncompressed^[prgsp+2];
        z80bc:=prgbc;z80hl:=prghl;z80sp:=prgsp+6;
        z80i:=prgi;z80r:=uncompressed^[prgsp+1];
        z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
        if (uncompressed^[prgsp+1] and 128)=0 then z80stat1:=z80stat1 and $FE
           else z80stat1:=z80stat1 or 1;
        z80de:=prgde;z80bcp:=prgbcp; z80dep:=prgdep; z80hlp:=prghlp;z80ap:=prgap;
        z80fp:=prgfp;z80iy:=prgiy;z80ix:=prgix;z80iff1:=uncompressed^[prgsp+0];
        z80iff2:=z80iff1;
        if z80i=63 then z80stat2:=1 else z80stat2:=2;
        z80pc:=uncompressed^[prgsp+4]+256*uncompressed^[prgsp+5];
      end; {With}
      with snaphdr do begin
        z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0; z80mmu:=0; z80if1paged:=0;
        z80rldir:=0; z80fffd:=0; for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=defaultfile;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end; {With}
      defaulttype:=2;
      exit;
    end;
  end;
  if e='.SEM' then with radssnapom^ do begin
    defaultfile:=tempdir+'TEMP.Z80';
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    if not(fblockread(datoteka,sigsize,1)) then begin we; exit end;
    for i:=1 to sigsize do
      if not(fblockread(datoteka,sigread,1)) then begin we; exit end;
    if not(fblockread(datoteka,uncompressed^,49152)) then begin we; exit end;
    if not(fblockread(datoteka,other,34)) then begin we; exit end;
    if not(fclose(datoteka)) then begin we; exit end;
    with other do with snaphdr do begin
      z80A:=sema;z80f:=semf;z80bc:=sembc;z80hl:=semhl;
      z80sp:=semsp;z80i:=semi;z80r:=semr;
      z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
      if (semr and 128)=0 then z80stat1:=z80stat1 and $FE
       else z80stat1:=z80stat1 or 1;
      z80de:=semde; z80bcp:=sembcp; z80dep:=semdep; z80hlp:=semhlp;
      z80ap:=semap; z80fp:=semfp; z80iy:=semiy; z80ix:=semix; z80iff1:=semiff1;
      z80iff2:=semiff2; z80stat2:=semim; z80pc:=sempc;
    end; {With}
    with snaphdr do begin
      z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0; z80mmu:=0; z80if1paged:=0;
      z80rldir:=0; z80fffd:=0; for i:=0 to 15 do z80sndreg[i]:=0;
      tmp:=snapname; snapname:=defaultfile;
      if snapname<>'' then savehdrand48k(55);
      snapname:=tmp;
    end; {With}
    DefaultType:=2;
    exit;
  end;
  if e='.SIT' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if fs>=65564 then begin
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if not(fblockread(datoteka,other,28)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,16384)) then begin
        we; exit
      end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fclose(datoteka)) then begin we; exit end;
      with other do with snaphdr do begin
        z80A:=sita; z80f:=sitf;z80bc:=sitbc; z80hl:=sithl;
        z80sp:=sitsp; z80i:=siti; z80r:=sitr;
        z80stat1:=sitborder shl 1;
        if (sitr and 128)=0 then  z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=sitde;z80bcp:=sitbcp; z80dep:=sitdep; z80hlp:=sithlp;
        z80ap:=sitap; z80fp:=sitfp; z80iy:=sitiy; z80ix:=sitix;
        if (sitinter and 1) <>0 then z80iff1:=4 else z80iff1:=0;
        z80iff2:=z80iff1;
        if (sitinter and 2) <>0 then z80stat2:=2 else z80stat2:=0;
        z80pc:=sitpc;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0; z80addhdr:=23;z80hrdmode:=0; z80mmu:=0;
        z80if1paged:=0;z80rldir:=0;z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=defaultfile;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end; {With}
      DefaultType:=2;
      exit;
    end
  end;
  if e='.SNP' then with radssnapom^ do begin
    assign(datoteka,filename);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    fs:=filesize(datoteka);
    cc:=fclose(datoteka);
    defaultfile:=tempdir+'TEMP.Z80';
    if fs=49183 then begin
      assign(datoteka,filename);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      if not(fblockread(datoteka,uncompressed^,49152)) then begin
        we; exit
      end;
      if not(fblockread(datoteka,other,31)) then begin we; exit end;
      if not(fclose(datoteka)) then begin we; exit end;
      with snaphdr do with other do begin
        z80A:=snpa;z80f:=snpf;z80bc:=snpbc;z80hl:=snphl;
        z80sp:=snpsp; z80i:=snpi; z80r:=snpr;
        z80stat1:=lo((uncompressed^[23624] and $38)*4+32);
        if (snpr and 128)=0 then z80stat1:=z80stat1 and $FE
         else z80stat1:=z80stat1 or 1;
        z80de:=snpde; z80bcp:=snpbcp; z80dep:=snpdep; z80hlp:=snphlp;
        z80ap:=snpap; z80fp:=snpfp;z80iy:=snpiy; z80ix:=snpix;
        z80iff1:=snpiff1; z80iff2:=snpiff2; z80stat2:=snpim; z80pc:=snppc;
      end; {With}
      with snaphdr do begin
        z80oldpc:=0; z80addhdr:=23; z80hrdmode:=0; z80mmu:=0;
        z80if1paged:=0; z80rldir:=0; z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=defaultfile;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
      end; {With}
      DefaultType:=2;
      exit;
    end

  end;
  if e='.Z80' then with radssnapom^ do begin
    snapname:=filename;
    getheader;
    if (z80snptype>2)and (snaphdr.z80hrdmode=2) then begin
      defaultfile:=tempdir+'TEMP.Z80';
      load48k;
      warning48;
      with snaphdr do begin
        z80oldpc:=0;
        z80addhdr:=23;
        z80hrdmode:=0;
        z80mmu:=0;
        z80rldir:=0;
        z80fffd:=0;
        for i:=0 to 15 do z80sndreg[i]:=0;
        tmp:=snapname;
        snapname:=defaultfile;
        if snapname<>'' then savehdrand48k(55);
        snapname:=tmp;
        DefaultType:=2;
        exit;
      end; {With}
      getheader;
    end
     else begin
       DefaultFile:=FileName;
       DefaultType:=2;
       exit;
     end;
  end;
  if e='.MDR' then begin
    mdrloader;
    defaultfile:=filename;
    defaulttype:=4;
    exit;
  end;
  if e='.ZIP' then begin
    tmp:=tempdir+'ziprun';
    {$I-}
    chdir(tmp);
    if ioresult<>0 then begin
      mkdir(tmp);
      if ioresult<>0 then begin DisplayError('Error creating temp directory',0); exit end;
      chdir(tmp);
      if ioresult<>0 then begin DisplayError('Error changing temp directory',0); exit end;
    end;
    {$I+}
    cleanupdir(tmp,'*.*');
    if UnZIPFile ('/d', filename, '','' ,true) then begin
      findfirst('*.*',AnyFile and not(Directory),sr);
      if doserror=0 then filename:=fexpand(sr.name) else goto izbor;
      findnext(sr);
      if doserror<>0 then goto filenamefound;

      goto izbor;
    end

  end
end;
procedure mdrloader;
const mdrloadarray:array[1..1221] of byte=(
$10,$92,$FF,$00,$00,$4E,$00,$00,$44,$FF,$3F,$36,$0E,$00,$4E,$21,
$17,$9B,$36,$00,$00,$00,$44,$3A,$5C,$D4,$03,$01,$01,$01,$17,$00,
$5B,$0E,$01,$4E,$00,$03,$54,$4C,$00,$00,$00,$00,$00,$01,$00,$01,
$53,$44,$4B,$00,$00,$00,$00,$04,$01,$04,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,
$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$40,$00,$E5,$01,
$05,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$57,
$00,$F3,$0D,$CE,$0B,$E2,$50,$CE,$0B,$E3,$50,$1E,$17,$DC,$0A,$CE,
$0B,$E4,$50,$1D,$17,$DC,$0A,$D7,$18,$02,$00,$38,$00,$0D,$19,$DB,
$02,$4D,$00,$6C,$42,$94,$00,$6B,$42,$84,$18,$5C,$0E,$00,$46,$02,
$18,$53,$18,$C9,$0D,$6E,$0D,$C6,$1E,$57,$FF,$76,$1B,$03,$13,$00,
$3E,$00,$3C,$42,$42,$7E,$42,$42,$00,$00,$7C,$42,$7C,$42,$42,$7C,
$00,$00,$3C,$42,$40,$40,$42,$3C,$00,$00,$78,$44,$42,$42,$44,$78,
$00,$00,$7E,$40,$7C,$40,$40,$7E,$00,$00,$7E,$40,$7C,$40,$40,$40,
$00,$00,$3C,$42,$40,$4E,$42,$3C,$00,$00,$42,$42,$7E,$42,$42,$42,
$00,$00,$3E,$08,$08,$08,$08,$3E,$00,$00,$02,$02,$02,$42,$42,$3C,
$00,$00,$44,$48,$70,$48,$44,$42,$00,$00,$ED,$ED,$05,$40,$7E,$00,
$00,$42,$66,$5A,$42,$42,$42,$00,$00,$42,$62,$52,$4A,$46,$42,$00,
$00,$3C,$42,$42,$42,$42,$3C,$00,$00,$7C,$42,$42,$7C,$40,$40,$00,
$00,$3C,$42,$42,$52,$4A,$3C,$00,$00,$7C,$42,$42,$7C,$44,$42,$00,
$00,$3C,$40,$3C,$02,$42,$3C,$00,$00,$FE,$ED,$ED,$05,$10,$00,$00,
$ED,$ED,$05,$42,$3C,$00,$9C,$01,$08,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$18,$00,$ED,$ED,$FF,
$38,$ED,$ED,$FF,$38,$ED,$ED,$FF,$38,$38,$38,$38,$ED,$ED,$FF,$00,
$00,$FF,$00,$00,$00,$0D,$04,$23,$0D,$0D,$23,$05,$ED,$ED,$05,$00,
$01,$00,$06,$00,$0B,$00,$01,$00,$01,$00,$06,$00,$10,$ED,$ED,$1A,
$00,$3C,$40,$00,$FF,$8C,$00,$54,$FF,$ED,$ED,$06,$00,$FE,$FF,$01,
$38,$00,$00,$CB,$5C,$00,$00,$B6,$5C,$BB,$5C,$CB,$5C,$CE,$5C,$CA,
$5C,$CC,$5C,$CD,$5C,$CD,$5C,$00,$00,$CF,$5C,$CF,$5C,$CF,$5C,$1E,
$92,$5C,$00,$02,$ED,$ED,$08,$00,$AC,$1A,$00,$00,$87,$00,$00,$58,
$FF,$00,$00,$21,$00,$5B,$21,$17,$00,$40,$E0,$50,$21,$18,$21,$17,
$01,$38,$00,$38,$ED,$ED,$22,$00,$57,$FF,$FF,$FF,$F4,$09,$A8,$10,
$4B,$F4,$09,$C4,$15,$53,$81,$0F,$C4,$15,$52,$F4,$09,$C4,$15,$50,
$80,$80,$F7,$0D,$80,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,$00,$ED,$ED,$FF,
$00,$ED,$ED,$54,$00);
var datoteka:file;
cc:Boolean;
begin
  assign(datoteka,tempdir+'TEMP.Z80');
  if not(frewriteu(datoteka,1)) then exit;
  if not(fblockwrite(datoteka,mdrloadarray,1221)) then;
  cc:=fclose(datoteka)
end;

end.
