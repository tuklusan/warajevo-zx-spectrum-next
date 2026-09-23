


unit setup;
{$O+,V-}
interface
USES TAPES,SNAPSHOT,MICROD,TIMEX;
const
  comopts: string[80]='';
  netopts: string[80]='';
  deftapedir:string[80]='';
  emultype:integer=0;
  emulators:array[0..2] of string[12]=('SPEC48.EXE','SPEC128.EXE','TS2068.EXE');
  dbasedir: string[80]='';
  Indexedfields : String[9]='F';

var
  Interfsetting : record
    Switch : Word; {RadioButtons  0 default, 1 8K, 2 16 K}
    OldRom : Word;
    end;

  Speedsetting : record
    Slowdown : LongInt; {Inputlong}
    MaxSpeed:Word;
  end;

   Joysetting: record
    Joy1 : Word; {Checkboxes}
    Joy2 : Word; {Checkboxes}
    Xmin : LongInt; {Inputlong}
    Xmax : LongInt; {Inputlong}
    Ymin : LongInt; {Inputlong}
    Ymax : LongInt; {Inputlong}
    end;

   Printersetting : record
    ZXPrinter : Word; {RadioButtons}
    end;

   ROMsetting : record
    Spec2 : Word; {Checkboxes}
    FastBas : Word; {Checkboxes}
    Invcur : Word; {Checkboxes}
    end;

  SoundSetting : record
    Hear : Word; {Checkboxes}
    AYChan : Word; {Checkboxes}
    AudioDev : Word; {RadioButtons}
    MICEAR : Word; {RadioButtons}
    Beeper : Word; {RadioButtons}
    Autodetect : Word; {Checkboxes}
    SBAddr : LongInt; {Inputlong}
    DMA : LongInt; {Inputlong}
    end;

var
  VideoSetting: record
    VIDEO : Word; {RadioButtons}
    Border : Word; {RadioButtons}
    Attrmode : Word; {RadioButtons}
    Hires : Word; {Checkboxes}
    RowFirst : LongInt; {Inputlong}
    ExtraULA : LongInt; {Inputlong}
    StartUla : LongInt; {Inputlong}
    end;



  tests : record
    emb8086 : Word; {Checkboxes}
    Tst128 : Word; {Checkboxes}
    TimeQuit : LongInt; {Inputlong}
    QuitDOS : Word; {Checkboxes}
    end;


  Network : record
    NetFile : String[60]; {Inputline}
    PSWait : LongInt; {Inputlong}
    FreeNet : LongInt; {Inputlong}
    BusyWait : LongInt; {Inputlong}
    end;

  WinSetting : record
    CheckVMM:Word; {Checkboxes}
    WinTests : Word; {Checkboxes}
  end;


  TZX : record
    CheckBoxes : Word; {Checkboxes}
    end;

  palettestr:string[48];

procedure defaultfill;
procedure Inter1;
procedure Interrupt;
procedure Joystick;
procedure printer;
procedure ROM;
procedure sound;
procedure video;
procedure selectemu;
procedure setcomopts;
procedure test;
procedure networkSetup;
procedure tzxset;
procedure mswindows;
function Parse(FileName:string;tape:Ptapefile;snap:PSnapshotfile;
  mdv:PMdvFile;dck:PDOckFile):Boolean;
function SaveCfg(FileName:string;tape:Ptapefile;snap:PSnapshotfile;
  mdv:PMdvFile;dck:PDOckFile; sco:Boolean):Boolean;
procedure TimexKillsZX(mdv:PMdvFile);


implementation
uses views,dialogs,objects,App,hexedito,
  iorutine,dos,drivers,helpctx,stddlg;

procedure TimexKillsZX(mdv:PMdvFile);
var i:integer;
begin
  netopts:='';
  videosetting.hires:=0;
  videosetting.rowfirst:=156;
  interfsetting.switch:=0;
  for i:=1 to 8 do Mdv^.mdvnames[i]:='';
end;

procedure setcomopts;
var numstr1, numstr2:string;
  i:integer;
  procedure setsb;
  begin
       comopts:=comopts+'/SB'; {Sound Blaster}
       if (soundsetting.autodetect=0) then begin
         str(soundsetting.sbaddr,numstr1);
         comopts:=comopts+numstr1;
         if soundsetting.dma<>1 then begin
           str(soundsetting.dma,numstr1);
           comopts:=comopts+';'+numstr1;
         end
       end;
       comopts:=comopts+' ';
  end;
  procedure anjoy;
  var i:integer;
  begin
  with joysetting do begin
      comopts:=comopts+'/JA';
      str(xmin:3,numstr1);
      for i:=1 to 3 do if numstr1[i]=' ' then numstr1[i]:='0';
      comopts:=comopts+numstr1;
      str(xmax:3,numstr1);
      for i:=1 to 3 do if numstr1[i]=' ' then numstr1[i]:='0';
      comopts:=comopts+numstr1;
      str(ymin:3,numstr1);
      for i:=1 to 3 do if numstr1[i]=' ' then numstr1[i]:='0';
      comopts:=comopts+numstr1;
      str(ymax:3,numstr1);
      for i:=1 to 3 do if numstr1[i]=' ' then numstr1[i]:='0';
      comopts:=comopts+numstr1+' ';
  end
  end;
begin
  comopts:='';
  if (tests.emb8086=1) then comopts:=comopts+'/?86 ';
  if emultype=0 then
    if (tests.tst128=1) then comopts:=comopts+'/?128 ';
  if tests.TimeQuit<>0 then begin
    str(tests.timequit,numstr1);
    case tests.quitdos of
      0:comopts:=comopts+'/@@'+numstr1+' ';
      1:comopts:=comopts+'/@'+numstr1+' ';
    end;
  end;
  if emultype=1 then
    if (romsetting.spec2=1) then comopts:=comopts+'/2 ';
  if (romsetting.fastbas=1) then comopts:=comopts+'/B ';
  if emultype<>2 then begin
    if (interfsetting.switch=1) then case interfsetting.oldrom of
      0: comopts:=comopts+'/E ';
      1: comopts:=comopts+'/EO ';
    end;
    if (interfsetting.switch=2) then case interfsetting.oldrom of
      0: comopts:=comopts+'/EX ';
      1: comopts:=comopts+'/EOX ';
    end;
  end;
  with joysetting do if emultype in [0,1] then begin
    if (Joy1 and 1)<>0 then comopts:=comopts+'/JK ';
    if (Joy1 and 2)<>0 then comopts:=comopts+'/JS ';
    if (Joy1 and 4)<>0 then comopts:=comopts+'/JB ';
    if (Joy1 and 8)<>0 then comopts:=comopts+'/JC ';
    if (Joy1 and 16)<>0 then comopts:=comopts+'/JM ';
    if (Joy1 and 32)<>0 then comopts:=comopts+'/JA ';
    if (Joy1 and 64)<>0 then anjoy;
  end;
  with joysetting do if (emultype =2) and (joy2<>0) then begin
    if (Joy2 and 1)<>0 then comopts:=comopts+'/JK ';
    if (Joy2 and 2)<>0 then comopts:=comopts+'/JM ';
    if (Joy2 and 4)<>0 then comopts:=comopts+'/JA ';
    if (Joy2 and 8)<>0 then anjoy;
    if (Joy2 and $0F)=0 then comopts:=comopts+'/JN ';
    if (Joy2 and $10)<>0 then comopts:=comopts+'/JK ';
    if (Joy2 and $20)<>0 then comopts:=comopts+'/JM ';
    if (Joy2 and $40)<>0 then comopts:=comopts+'/JA ';
    if (Joy2 and $80)<>0 then anjoy;
    if (Joy2 and $F0)=0 then comopts:=comopts+'/JN ';
    if (Joy2 and $100)<>0 then comopts:=comopts+'/JS ';
    if (Joy2 and $200)<>0 then comopts:=comopts+'/JB ';
    if (Joy2 and $400)<>0 then comopts:=comopts+'/JC ';
  end;

  if (romsetting.Invcur=1) then comopts:=comopts+'/K ';
  if not (videosetting.video in [0,7]) then begin
    if videosetting.Attrmode=1 then comopts:=comopts+'/M1 ';
    if videosetting.Attrmode=2 then comopts:=comopts+'/M2 ';
  end;

  if (emultype in [1,2]) and (soundsetting.audiodev=0) then case soundsetting.aychan of
    0: comopts:=comopts+'/S7 ';
    1: comopts:=comopts+'/S6 ';
    2: comopts:=comopts+'/S5 ';
    3: comopts:=comopts+'/S4 ';
    4: comopts:=comopts+'/S3 ';
    5: comopts:=comopts+'/S2 ';
    6: comopts:=comopts+'/S1 ';
  end;
  if emultype in [1,2] then case soundsetting.audiodev of
    1: comopts:=comopts+'/SX '; {Digitized PC Speaker}
    2: comopts:=comopts+'/SO '; {OPL 2 Adlib compatible}
    3: comopts:=comopts+'/SA '; {OPL 3 Adlib compatible}
    4: setsb {Sound Blaster DAC}
  end;
  if soundsetting.micear=1 then comopts:=comopts+'/L6 ';
  if soundsetting.micear=2 then begin
    comopts:=comopts+'/L7 ';
    if (soundsetting.audiodev<>4) or (emultype=0) then setsb;
  end;
  case printersetting.zxprinter of
    1: comopts:=comopts+'/PE ';
    2: comopts:=comopts+'/PB ';
    3: comopts:=comopts+'/PL ';
    4: comopts:=comopts+'/PP ';

  end;

  if (soundsetting.hear=0) and
    (videosetting.border=0)then comopts:=comopts+'/U0 ';
  if(soundsetting.hear=0) and
    (videosetting.border=1)then comopts:=comopts+'/U1 ';
  if(soundsetting.hear=1) and
    (videosetting.border=0)then comopts:=comopts+'/U2 ';
  if(soundsetting.hear=0) and
    (videosetting.border=2)then comopts:=comopts+'/U4 ';
  if(soundsetting.hear=1) and
    (videosetting.border=2)then comopts:=comopts+'/U5 ';
  netopts:='';
  With Network do begin
    if NetFile<>'' then begin
      netopts:=Netfile;
      if (PSWait<>30) or (FreeNet<>5) or (BusyWait<>5) then begin
        str(PSWait,numstr1);
        netopts:=netopts+';'+numstr1;
        if (FreeNet<>5) or (BusyWait<>5) then begin
          str(FreeNet,numstr1);
          netopts:=netopts+';'+numstr1;
          if (BusyWait<>5) then begin
            str(BusyWait,numstr1);
            netopts:=netopts+';'+numstr1;
          end;
        end;
      end;
    end;
  end;

  case videosetting.video of
    1: comopts:=comopts+'/VC ';
    2: comopts:=comopts+'/VW ';
    3: comopts:=comopts+'/VM ';
    4: comopts:=comopts+'/VE ';
    5: comopts:=comopts+'/VH ';
    6: comopts:=comopts+'/VX ';
    7: comopts:=comopts+'/VV ';
  end;
  if videosetting.hires=0 then begin
    if videosetting.RowFirst<>156 then begin
      str (videosetting.RowFirst,numstr1);
      comopts:=comopts+'/Q'+numstr1+' ';
    end;
  end
   else begin
    comopts:=comopts+'/QX';
    if  (videosetting.extraula<>0) or (videosetting.startula<>0)
     then begin
      str (videosetting.extraula,numstr1);
      comopts:=comopts+numstr1;
      if videosetting.startula<>0 then begin
        str (videosetting.startula,numstr1);
        comopts:=comopts+';'+numstr1;
      end;
    end;
    comopts:=comopts+' ';
   end;
  str (speedsetting.slowdown,numstr1);
  if numstr1<>' ' then numstr1:=' '+numstr1;
  numstr1[1]:='%';
  if (speedsetting.maxspeed and 1)<>0 then comopts:=comopts+'/%MAX ' else
    if speedsetting.slowdown<>100 then comopts:=comopts+'/'+numstr1+' ';
  if (speedsetting.maxspeed and 2)<>0 then comopts:=comopts+'/%LMAX ';

  With Tapesetting do begin
    if position<>0 then begin
      str(position+1,numstr1);
      if numstr1<>' ' then numstr1:=' '+numstr1;
      numstr1[1]:='F';
      comopts:=comopts+'/'+numstr1+' ';
    end;
    if Autostart=1 then comopts:=comopts+'/A ';
    if Autostart=2 then comopts:=comopts+'/A- ';

    if Hidebor=1 then comopts:=comopts+'/H ';
    if soundsetting.micear=0 then
    case loadmeth of
      0: if Stopmode=1 then comopts:=comopts+'/L3 ';
      1: if Stopmode=1 then comopts:=comopts+'/L4 '
          else comopts:=comopts+'/L1 ';
      2: if Stopmode=1 then comopts:=comopts+'/L5 '
          else comopts:=comopts+'/L2 ';
    end;
    if Initattr<>7 then begin
      str(Initattr,Numstr1);
      if numstr1<>' ' then numstr1:=' '+numstr1;
      numstr1[1]:='Y';
      comopts:=comopts+'/'+numstr1+' ';
    end;
  end;

  with WinSetting do
    if CheckVMM=0 then begin
      comopts:=comopts+'/!';
      str(WinTests,numstr1);
      if wintests<>0 then comopts:=comopts+numstr1;
      comopts:=comopts+' ';
     end;

  comopts:=comopts+'/N';
  curemul:=emuldir+emulators[emultype];
{  desktop^.redraw;}
end;

Function get_value (blaster:string;letter:char;hex:boolean;var value:longint):boolean;
{---------------------------------------------------------------------------------}
{ Pomocna lokalna funkcija koja uzima iz stringa BLASTER vrijednost LETTER.
  Uzima se i slucaj kada je vrijednost heksadecimalna HEX. U VALUE je trazena
  vrijednost. Ako se uopste nadje LETTER GET_VALUE je TRUE, inace false.}

var pomstr1,pomstr2: string;
    err : integer;

begin
pomstr1:=blaster+' ';
delete(pomstr1,1,pos(letter,pomstr1));
pomstr2:=copy(pomstr1,1,pos(' ',pomstr1)-1);
if hex then val('$'+pomstr2,value,err)
       else val(pomstr2,value,err);
if err <>0 then get_value:=false
           else get_value:=true;
end;  { --- GET_VALUE --- }



procedure defaultfill;
var Blaster:String;
begin
  With Interfsetting do begin
    Switch:=0;
    OldRom:=0;
  end;

  With Speedsetting do begin
    maxspeed:=0;
    Slowdown:=100;
  end;

  With Joysetting do begin
    Joy1:=0;
    Joy2:=0;
    Xmin:=0;
    Xmax:=0;
    Ymin:=0;
    Ymax:=0;
  end;
  With Network do begin
    NetFile :=''; {Inputline}
    PSWait :=30; {Inputlong}
    FreeNet :=5; {Inputlong}
    BusyWait :=5; {Inputlong}
  end;


  With Printersetting do begin
    ZXPrinter:=0;
   end;

  With ROMsetting do begin
    Spec2:=0;
    FastBas:=0;
    Invcur:=0
  end;

  With soundsetting do begin
    Hear :=1;
    AYChan :=7;
    AudioDev :=0;
    MICEAR :=0;
    Beeper :=0;
    Autodetect :=1;
    Blaster:=GetEnv('BLASTER');
    if not(get_value (blaster,'A',TRUE,sbaddr)) then sbaddr:=$220;
    if not(get_value (blaster,'D',false,dma)) then DMA:=1;
    end;


  With tests do begin
    emb8086 :=0;
    Tst128 :=0;
    TimeQuit :=0;
    QuitDOS :=0;
    end;

  With Videosetting do begin
    Video:=0;
    Border:=1;
    AttrMode:=0;
    Hires :=0;
    RowFirst :=156;
    ExtraULA :=0;
    StartUla :=0;

  end;

  With Tapesetting do begin
    position:=0;
    Autostart:=0;
    Hidebor:=0;
    loadmeth:=0;
    Stopmode:=0;
    Initattr:=7;
  end;
  with winsetting do begin
    CheckVMM:=1;
    Wintests:=0;
  end;

  tzx.checkboxes:=65022;
end;

Type TBrowseButton=Object(TButton)
  resultline:PInputLine;
  procedure press; virtual;
end;
PBrowseButton=^TBrowseButton;

Procedure TBrowseButton.Press;
var dlg:PFileDialog;
  c:Word;
  fname:String;
begin
  dlg:=new(PFileDIalog,Init('*.*','Select file name','~N~ame',
     fdOKButton,100));
  c:=desktop^.execview(dlg);
  if c<>cmCancel then begin
    dlg^.getfilename(fname);
    fname:=fexpand(fname);
    resultline^.SetData(fname);
  end;
  dispose(dlg,done)
end;

procedure NetworkSetup;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:Word;
  BButton:PBrowseButton;
  nf:PInputLine;
Begin
R.Assign(10,2,70,20);
New(Dlg, Init(R, 'Network setting'));
dlg^.helpctx:=hcnetwork;
R.Assign(2,3,42,4);
nf := New(PInputLine, Init(R, 60));
Dlg^.Insert(nf);

  R.Assign(2,2,15,3);
  Dlg^.Insert(New(PLabel, Init(R, '~N~etwork file', nf)));

R.Assign(45,3,57,5);
BButton := New(PBrowseButton, Init(R, '~B~rowse', cmYes, bfNormal));
BButton^.ResultLine:=nf;
Dlg^.Insert(BButton);

R.Assign(39,6,47,7);
Control := New(PNumInputLine, Init(R, 6, 0, 1800));
Dlg^.Insert(Control);

  R.Assign(2,6,37,7);
  Dlg^.Insert(New(PLabel, Init(R, 'Post sending ~w~ait (18th of second)', Control)));

R.Assign(35,8,43,9);
Control := New(PNumInputLine, Init(R, 6, 0, 255));
Dlg^.Insert(Control);

  R.Assign(2,8,32,9);
  Dlg^.Insert(New(PLabel, Init(R, '~F~ree network state autorepeat', Control)));

R.Assign(31,10,39,11);
Control := New(PNumInputLine, Init(R, 6, 0, 255));
Dlg^.Insert(Control);

  R.Assign(2,10,28,11);
  Dlg^.Insert(New(PLabel, Init(R, 'B~u~sy net state autorepeat', Control)));

R.Assign(3,13,17,15);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(28,13,40,15);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
dlg^.setdata(Network);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(Network);
  setcomopts;
end;
dispose(dlg,done)

end;




procedure inter1;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
Begin
R.Assign(10,2,44,16);
New(Dlg, Init(R, 'Interface 1 emulation'));
Dlg^.helpctx:=hcinter1;
R.Assign(2,3,32,6);
Control := New(PRadiobuttons, Init(R,
  NewSItem('Do not emulate interf. 1',
  NewSItem('Switch 8 K ROM',
  NewSItem('Switch 16 K ROM',Nil)))));
Dlg^.Insert(Control);

  R.Assign(1,2,23,3);
  Dlg^.Insert(New(PLabel, Init(R, '~I~nterface 1 emulation', Control)));
R.Assign(2,7,32,8);
Control := New(PCheckboxes, Init(R,
  NewSItem('Use old Interface 1 ROM',Nil)));
Dlg^.Insert(Control);

R.Assign(2,9,12,11);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(17,9,27,11);
Control := New(PButton, Init(R, 'Cancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

dlg^.setdata(Interfsetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(Interfsetting);
  setcomopts;
end;
dispose(dlg,done)
end;

procedure selectemu;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
Begin
R.Assign(10,2,44,17);
New(Dlg, Init(R, 'Select emulator'));
dlg^.helpctx:=hcemulsel;
R.Assign(2,3,32,6);
Control := New(PRadiobuttons, Init(R,
  NewSItem('48K emulation',
  NewSItem('128K emulation',
  NewSItem('Timex 2068 emulation',
  Nil)))));
Dlg^.Insert(Control);

  R.Assign(1,2,23,3);
  Dlg^.Insert(New(PLabel, Init(R, '~S~elect emulator', Control)));

R.Assign(2,9,12,11);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(17,9,27,11);
Control := New(PButton, Init(R, 'Cancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

dlg^.setdata(emultype);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(emultype);
  setcomopts;
end;
dispose(dlg,done);


end;
type TNumInputlinewithscr=object(TNumInputline)
  linkscroll:PScrollbar;
  procedure handleevent(var event:tevent); virtual;
end;
PNumInputlinewithscr=^TNumInputlinewithscr;

procedure TNumInputlinewithscr.handleevent(var event:tevent);
var vr:longint;
  code:word;
begin
  TNumInputline.handleevent(event);
  val(data^,vr,code);
  if linkscroll<>nil then begin
    linkscroll^.value:=integer(vr);
    linkscroll^.draw;
  end;
end;

type TScrollbarwithnum=object(TScrollbar)
  linknum:PNumInputline;
  procedure handleevent(var event:tevent); virtual;
end;
PScrollbarwithnum=^TScrollbarwithnum;

procedure TScrollbarwithnum.handleevent(var event:tevent);
var vr:longint;
  code:word;
begin
  TScrollbar.handleevent(event);
  if linknum<> nil then linknum^.setdata(value);
end;

procedure interrupt;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  slowskr:Pscrollbarwithnum;
  slowval:PNumInputlinewithscr;
Begin
R.Assign(1,3,76,17);
New(Dlg, Init(R, 'Speed'));
dlg^.helpctx:=hcspeed;

R.Assign(4,3,71,4);
slowskr := New(PScrollBarwithnum, Init(R));
slowskr^.setrange(0,800);
Dlg^.Insert(slowskr);

  R.Assign(3,2,43,3);
  Dlg^.Insert(New(PLabel, Init(R, '~S~peed in % relative to ZX Spectrum', slowskr)));

R.Assign(44,2,52,3);
slowval := New(pnuminputlinewithscr, Init(R, 6, 10, 800));
slowval^.linkscroll:=Slowskr;
Slowskr^.linknum:=Slowval;

Dlg^.Insert(Slowval);


R.Assign(5,4,73,5);
Control := New(PStaticText, Init(R, '0   50  100 150 200 250 300 350 400 450'+
     ' 500 550 600 650 700 750 800 850 900 950 1000'));
Dlg^.Insert(Control);

R.Assign(5,6,32,8);
Control := New(PCheckboxes, Init(R,
  NewSItem(' Maximal speed mode',
  NewSItem(' Maximal loading speed mode',Nil))));
Dlg^.Insert(Control);


R.Assign(5,10,17,12);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(31,10,41,12);
Control := New(PButton, Init(R, 'Cancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

dlg^.setdata(Speedsetting);
with Speedsetting do begin
    slowskr^.value:=slowdown;
end;

c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(speedsetting);
  setcomopts;
end;
dispose(dlg,done)

end;

type Tjoyboxes=Object(Tcheckboxes)
  function Valid(Command: Word): Boolean; virtual;
end;
Pjoyboxes=^Tjoyboxes;

function Tjoyboxes.Valid(Command: Word):Boolean;
type tresursi=(lt,rt,an,cu,mou,sa,sb,pro);
var nizi:byte;
resursi:set of tresursi;
v1:Boolean;
function opt(num:integer;r1,r2:TResursi):Boolean;
begin
  opt:=true;
  if (value shr (num)) and 1 <> 0 then begin
    if (r1 in resursi) or (r2 in resursi) then opt:=false;
    resursi:=resursi+[r1,r2]
  end
end;
begin
If emultype in [0,1] then begin
  nizi:=lo(value);
  if (nizi in [0, 1, 2, 4, 8, 16, 32, 64, 18, 20, 24, 34, 36, 40, 66, 68, 72])
    or (command<>cmOK) then valid:=TCheckboxes.valid(Command)
     else begin
       Displayerror('Invalid combination',0);
       valid:=false;
     end
end
 else begin
   resursi:=[ ];
   v1:=
     opt(0,lt,cu) and opt(1,lt,mou) and opt(2,lt,an)
     and opt(3,lt,an) and
     opt(4,rt,cu) and opt(5,rt,mou) and opt(6,rt,an)
     and opt(7,rt,an) and opt (8,cu,sa) and opt (9,cu,sb) and
     opt(10,cu,pro);
     valid:=v1;
     if v1 or (command<>cmOK) then valid:=TCheckboxes.valid(Command)
     else Displayerror('Invalid combination',0);
 end;
end;


procedure joystick;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;

Begin
R.Assign(1,1,71,21);
New(Dlg, Init(R, 'Joystick'));
Dlg^.helpctx:=hcjoystick;
R.Assign(3,2,61,13);
Control := New(Pjoyboxes, Init(R,
  NewSItem('Kempston joystick by cursor keys ',
  NewSItem('Sinclair joystick A by cursor keys',
  NewSItem('Sinclair joystick B by cursor keys',
  NewSItem('AGF/Protek joystick by cursor keys',
  NewSItem('Kempston joystick by MS mouse',
  NewSItem('Kempston joystick by analog autocalibrated',
  NewSItem('Kempston by analog, manually calibrated',Nil)))))))));
Dlg^.Insert(Control);
if emultype=2 then control^.hide;
Control := New(Pjoyboxes, Init(R,
  NewSItem('Left Timex joystick by cursor keys ',
  NewSItem('Left Timex joystick by MS mouse',
  NewSItem('Left Timex joystick by analog autocalibrated',
  NewSItem('Left Timex by analog, manually calibrated',
  NewSItem('Right Timex joystick by cursor keys ',
  NewSItem('Right Timex joystick by MS mouse',
  NewSItem('Right Timex joystick by analog autocalibrated',
  NewSItem('Right Timex by analog, manually calibrated',
  NewSItem('Sinclair joystick A by cursor keys',
  NewSItem('Sinclair joystick B by cursor keys',
  NewSItem('AGF/Protek joystick by cursor keys',nil)))))))))))));
Dlg^.Insert(Control);
if emultype in [0,1] then control^.hide;
  R.Assign(2,1,21,2);
  Dlg^.Insert(New(PLabel, Init(R, '~J~oystick emulation', Control)));

R.Assign(4,16,12,17);
Control := New(pnuminputline, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(3,15,8,16);
  Dlg^.Insert(New(PLabel, Init(R, '~X~min', Control)));

R.Assign(18,16,26,17);
Control := New(pnuminputline, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(17,15,22,16);
  Dlg^.Insert(New(PLabel, Init(R, 'X~m~ax', Control)));

R.Assign(40,16,48,17);
Control := New(pnuminputline, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(39,15,44,16);
  Dlg^.Insert(New(PLabel, Init(R, '~Y~min', Control)));

R.Assign(54,16,62,17);
Control := New(pnuminputline, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(53,15,58,16);
  Dlg^.Insert(New(PLabel, Init(R, 'Ym~a~x', Control)));

R.Assign(20,14,41,15);
Control := New(PStaticText, Init(R, 'Calibration constants'));
Dlg^.Insert(Control);

R.Assign(12,17,23,19);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(37,17,49,19);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);


dlg^.setdata(joysetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(joysetting);
  setcomopts;
end;
dispose(dlg,done)

end;

procedure printer;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;

Begin
R.Assign(10,2,70,20);
New(Dlg, Init(R, 'Printer'));
Dlg^.helpctx:=hcprinter;
R.Assign(10,5,38,10);
Control := New(PRadiobuttons, Init(R,
  NewSItem('Do not emulate',
  NewSItem('Epson printer, normal ',
  NewSItem('Epson printer, big',
  NewSItem('HP Laser jet, normal',
  NewSItem('HP Laser jet, big',Nil)))))));
Dlg^.Insert(Control);

  R.Assign(8,4,19,5);
  Dlg^.Insert(New(PLabel, Init(R, '~Z~X Printer', Control)));

R.Assign(8,12,21,14);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(33,12,45,14);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

dlg^.setdata(printersetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(printersetting);
  setcomopts
end;
dispose(dlg,done)

end;
procedure test;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
Begin
R.Assign(12,2,71,18);
New(Dlg, Init(R, 'Tests'));

R.Assign(6,4,38,5);
Control := New(PCheckboxes, Init(R,
  NewSItem('Test for embeded ~8~086 code',Nil)));
Dlg^.Insert(Control);

R.Assign(6,6,31,7);
Control := New(PCheckboxes, Init(R,
  NewSItem('Test for ~1~28K ports',Nil)));
Dlg^.Insert(Control);

R.Assign(18,8,26,9);
Control := New(PNumInputLine, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(27,8,34,9);
  Dlg^.Insert(New(PLabel, Init(R, '1/50 s', Control)));

  R.Assign(6,8,17,9);
  Dlg^.Insert(New(PLabel, Init(R, '~Q~uit after', Control)));

R.Assign(35,8,57,9);
Control := New(PCheckboxes, Init(R,
  NewSItem('with exit to ~D~OS',Nil)));
Dlg^.Insert(Control);

R.Assign(6,12,20,14);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(22,12,34,14);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

Dlg^.setData(tests);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(tests);
  setcomopts;
end;
dispose(dlg,done)



end;
procedure ROM;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;

Begin
R.Assign(10,2,47,19);
New(Dlg, Init(R, 'ROM parameters'));
Dlg^.helpctx:=hcrom;

R.Assign(2,5,27,6);
Control := New(PCheckboxes, Init(R,
  NewSItem('Emulate Spectrum +2',Nil)));
Dlg^.Insert(Control);
if emultype<>1 then Control^.state:=sfDisabled;


R.Assign(2,7,18,8);
Control := New(PCheckboxes, Init(R,
  NewSItem('Fast BASIC',Nil)));
Dlg^.Insert(Control);

R.Assign(2,9,24,10);
Control := New(PCheckboxes, Init(R,
  NewSItem(' Inverse cursor',Nil)));
Dlg^.Insert(Control);

R.Assign(3,12,14,14);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(20,12,30,14);
Control := New(PButton, Init(R, 'Cancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);

dlg^.setdata(ROMsetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(ROMsetting);
  setcomopts;
end;
dispose(dlg,done)
end;
type TSoundRadioButtons=Object(TRadioButtons)
  cb:PCheckBoxes;
  procedure HandleEvent(var Event:TEvent);Virtual;
end;
PSoundRadioButtons=^TSoundRadioButtons;
procedure TSoundRadioButtons.HandleEvent(var Event:TEvent);
begin
  TRadioButtons.HandleEvent(Event);
  if value<>0 then begin cb^.value:=7; cb^.draw;
  cb^.setstate(sfDisabled,true) end else
    cb^.setstate(sfDisabled,false);
end;

procedure sound;
var
    AYChans : PCheckboxes;
    SoundRadio :PSoundRadiobuttons;
    c:word;
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
Begin
R.Assign(1,1,75,21);
New(Dlg, Init(R, 'Sound Setting'));
dlg^.helpctx:=hcsound;
R.Assign(28,2,44,3);
Control := New(PCheckboxes, Init(R,
  NewSItem('~H~ear Sound',Nil)));
Dlg^.Insert(Control);

R.Assign(3,5,19,10);
AYChans := New(PCheckboxes, Init(R,
  NewSItem('Channel A',
  NewSItem('Channel B',
  NewSItem('Channel C',Nil)))));
Dlg^.Insert(AYChans);
if emultype=0 then AYChans^.state:=sfDisabled;

R.Assign(19,5,63,10);
SoundRadio := New(PSoundRadiobuttons, Init(R,
  NewSItem('PC Speaker with continual tone',
  NewSItem('PC Speaker with sampling',
  NewSItem('ADLIB like FM synthesis OPL2',
  NewSItem('ADLIB like FM synthesis OPL3',
  NewSItem('Sound Blaster DAC and MIDI',Nil)))))));
Dlg^.Insert(SoundRadio);
if emultype=0 then SoundRadio^.state:=sfDisabled;
if emultype<>0 then begin
  R.Assign(20,4,37,5);
  Dlg^.Insert(New(PLabel, Init(R, '~A~Y Output device', SoundRadio)));

  R.Assign(3,4,13,5);
  Dlg^.Insert(New(PLabel, Init(R, 'AY Cha~n~nel', AYChans)));
end;
R.Assign(3,12,43,15);
Control := New(PRadiobuttons, Init(R,
  NewSItem('Tape file or random value',
  NewSItem('BG Services at LPT1',
  NewSItem('Sound Blaster (EAR only)',Nil)))));
Dlg^.Insert(Control);

  R.Assign(3,11,18,12);
  Dlg^.Insert(New(PLabel, Init(R, '~M~ic/Ear device', Control)));

R.Assign(45,12,61,13);
Control := New(PRadiobuttons, Init(R,
  NewSItem('PC Speaker',Nil)));
Dlg^.Insert(Control);

  R.Assign(45,11,60,12);
  Dlg^.Insert(New(PLabel, Init(R, '~B~eeper device', Control)));
R.Assign(2,16,55,17);
Control := New(PCheckboxes, Init(R,
  NewSItem('~U~se autodetected Sound Blaster address and DMA=1',Nil)));
Dlg^.Insert(Control);

R.Assign(30,17,38,18);
Control := New(PNumInputLine, Init(R, 6, 0, 65535));
Dlg^.Insert(Control);

  R.Assign(2,17,29,18);
  Dlg^.Insert(New(PLabel, Init(R, '~S~ound blaster base address', Control)));

R.Assign(44,17,48,18);
Control := New(PNumInputLine, Init(R, 6, 0, 3));
Dlg^.Insert(Control);

  R.Assign(39,17,43,18);
  Dlg^.Insert(New(PLabel, Init(R, '~D~MA', Control)));


R.Assign(60,14,72,16);
Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(60,16,72,18);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);


Dlg^.SelectNext(False);

Dlg^.SelectNext(False);
soundRadio^.cb:=AYChans;

dlg^.setdata(soundsetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(soundsetting);
  setcomopts;
end;
dispose(dlg,done)
end;
type
THiresCheckBoxes=Object(TCheckBoxes)
  AtmodeLab,RowLab,ExtraLab,StartLab:PLabel;
  MinusLab,nulalab,  tristalab:PStaticText;
  RPaint : PScrollBarwithnum;
  RPaintNum : PNumInputLineWithScr;
  SUDelay : PScrollBarwithnum;
  SUDelayNum : PNumInputLineWithScr;
  UDelay : PScrollBarWithNum;
  UDelayNum : PNumInputLineWithScr;
  procedure showfast;
  procedure showhcr;
  procedure HandleEvent(var Event:TEvent);virtual;
end;

PHiresCheckBoxes=^THiresCheckBoxes;

TBorderRadioButtons=object(TRadioButtons)
  hccb:PHiresCheckBoxes;
  procedure HandleEvent(var Event:TEvent);virtual;
end;
PBorderRadioButtons=^TBorderRadioButtons;

TCardRadioButtons=object(TRadioButtons)
  attribm:pRadioButtons;
  atmodelab:PLabel;
  borderm:pBorderRadioButtons;
  hccb:PHiresCheckBoxes;
  procedure HandleEvent(var Event:TEvent);virtual;
  procedure adjust;
end;
PCardRadioButtons=^TCardRadioButtons;
Procedure TCardRadioButtons.Adjust;
var
atr1,atr2,atr3:String;
nn:Word;
begin
 case value of
    0:begin
      atr1:='Attribute mode 1';
      atr2:='Attribute mode 2';
      atr3:='Attribute mode 3';
    end;
    1,3,5,6:begin
      atr1:='Shade attributes';
      atr2:='White on black  ';
      atr3:='Black on white  ';

    end;
    2:begin
      atr1:='Blk Cya Mgn Whi ';
      atr2:='Blk Red Gre Brn ';
      atr3:='Blu Red Gre Brn ';
    end;
  end;
  if value in [4,7] then begin
    attribm^.Hide;
    atmodelab^.Hide;
   end
   else begin
    move (atr1,attribm^.Strings.At(0)^,length(atr1)+1);
    move (atr2,attribm^.Strings.At(1)^,length(atr2)+1);
    move (atr3,attribm^.Strings.At(2)^,length(atr3)+1);
    attribm^.Show;
    atmodelab^.Show;
    Attribm^.Draw;
  end;
  if value in [0,7] then begin
    borderm^.strings.count:=3;
  end
   else begin
    borderm^.strings.count:=2;
    nn:=1;
    if borderm^.value=2 then borderm^.SetData(nn);
   end;
  borderm^.Draw;
  if (value in [0,7]) and (borderm^.value=2) and (emultype in [0,1])
  then hccb^.Show else begin
      nn:=0;hccb^.setdata(nn);
      hccb^.Hide;
      hccb^.ShowFast;
    end; 
  hccb^.Draw;

end;
Procedure TCardRadioButtons.HandleEvent(var Event:TEvent);
var
oldvalue:integer;
begin
 oldvalue:=value;
  TRadioButtons.HandleEvent(Event);
  if value<>oldvalue then begin
    adjust;
    Select;
  end;
end;
Procedure TBorderRadioButtons.HandleEvent(var Event:TEvent);
var oldvalue,n:word;
begin
  oldvalue:=value;
  TRadioButtons.HandleEvent(Event);
  if value<>oldvalue then begin
    if (value=2) and (emultype in [0,1]) then begin
      hccb^.Show;
      n:=0;hccb^.setdata(n)
      end
      else begin
      n:=0;hccb^.setdata(n);
      hccb^.Hide;
      Hccb^.Showfast;
    end;
    hccb^.Draw;
    Select;
  end;
end;

Procedure THiresCheckBoxes.HandleEvent(var Event:TEvent);
var oldvalue:word;
begin
  oldvalue:=value;
  TCheckBoxes.HandleEvent(Event);
  if value<>oldvalue then begin
    if value=1 then begin showhcr
    end
    else begin showfast
    end;
    Owner^.Redraw;
    Select;
  end;
end;
Procedure THiresCheckBoxes.showfast;
begin
      RowLab^.Show;ExtraLab^.Hide;
      StartLab^.Hide;MinusLab^.Hide;
      NulaLab^.Hide;TristaLab^.Show;
      RPaint^.Show;RPaintNum^.Show;
      SUDelay^.hide;SUDelayNum^.hide;
      UDelay^.hide;UDelayNum^.hide;

end;
Procedure THiresCheckBoxes.showhcr;
begin
      RowLab^.Hide;ExtraLab^.Show;
      StartLab^.Show;MinusLab^.Show;
      NulaLab^.Show;TristaLab^.Hide;
      RPaint^.Hide;RPaintNum^.Hide;
      SUDelay^.Show;SUDelayNum^.Show;
      UDelay^.Show;UDelayNum^.Show;

end;
procedure video;
 var
    AttribMode : PRadiobuttons;
    BorderMode : PBorderRadiobuttons;
    RPaint : PScrollBarwithnum;
    RPaintNum : PNumInputLineWithScr;
    SUDelay : PScrollBarwithnum;
    SUDelayNum : PNumInputLineWithScr;
    UDelay : PScrollBarWithNum;
    UDelayNum : PNumInputLineWithScr;
    VideoMode : PCardRadiobuttons;
    Hires:PHiresCheckBoxes;
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  AtmodeLab,RowLab,ExtraLab,StartLab:PLabel;
  MinusLab,nulalab,  tristalab:PStaticText;
  a:integer;
Begin
R.Assign(1,1,77,22);
New(Dlg, Init(R, 'Video setting'));
Dlg^.helpctx:=hcvideo;
R.Assign(2,2,33,10);
VideoMode := New(PCardRadiobuttons, Init(R,
  NewSItem('Autodetect',
  NewSItem('CGA monochrome',
  NewSItem('CGA color',
  NewSItem('EGA monochrome',
  NewSItem('EGA color',
  NewSItem('Hercules mono standard',
  NewSItem('Hercules mono full scree',
  NewSItem('VGA',Nil))))))))));
Dlg^.Insert(VideoMode);

  R.Assign(2,1,15,2);
  Dlg^.Insert(New(PLabel, Init(R, '~V~ideo device', VideoMode)));

R.Assign(34,2,49,5);
BorderMode := New(PBorderRadiobuttons, Init(R,
  NewSItem('Hidden',
  NewSItem('Standard',
  NewSItem('Rainbow',Nil)))));
Dlg^.Insert(BorderMode);

  R.Assign(34,1,41,2);
  Dlg^.Insert(New(PLabel, Init(R, '~B~order', BorderMode)));

R.Assign(51,2,73,5);
AttribMode := New(PRadiobuttons, Init(R,
  NewSItem('Attribute mode 1  ',
  NewSItem('Attribute mode 2  ',
  NewSItem('Attribute mode 3  ',Nil)))));
Dlg^.Insert(AttribMode);

  R.Assign(51,1,66,2);
  ATmodelab:=New(PLabel, Init(R, '~A~ttribute mode', AttribMode));
  Dlg^.Insert(ATmodelab);
R.Assign(2,11,29,12);
Hires := New(PHiresCheckboxes, Init(R,
  NewSItem('High color resolution',Nil)));
Dlg^.Insert(Hires);

R.Assign(68,7,75,8);
RPaintNum := New(PNumInputLineWithScr, Init(R, 6, 0, 311));
Dlg^.Insert(RPaintNum);

R.Assign(34,8,67,9);
tristalab := New(PStaticText, Init(R, '0    50   100  150  200  250  300'));
Dlg^.Insert(tristalab);

R.Assign(34,7,67,8);
RPaint := New(PScrollBarwithnum, Init(R));
Rpaint^.SetRange(0,311);
Dlg^.Insert(RPaint);

  R.Assign(34,6,69,7);
  rowlab:=New(PLabel, Init(R, '~R~ow of screen where picture paints', RPaint));
  Dlg^.Insert(rowlab);
R.Assign(2,15,35,16);
minuslab := New(PStaticText, Init(R, ' -150 -100 -50   0   50  100  150 '));
Dlg^.Insert(minuslab);

R.Assign(2,14,35,15);
UDelay := New(PScrollBarWithNum, Init(R));
UDelay^.SetRange(-128,127);
Dlg^.Insert(UDelay);

  R.Assign(2,13,18,14);
  ExtraLab:=New(PLabel, Init(R, '~E~xtra ULA Delay', UDelay));
  Dlg^.Insert(ExtraLab);
R.Assign(36,14,44,15);
UDelayNum := New(PNumInputLineWithScr, Init(R, 6, -128, 127));
Dlg^.Insert(UDelayNum);

R.Assign(2,18,27,19);
SUDelay := New(PScrollBarwithnum, Init(R));
SUDelay^.SetRange(0,255);
Dlg^.Insert(SUDelay);

  R.Assign(2,17,18,18);
  StartLab:=New(PLabel, Init(R, '~S~tart ULA Delay', SUDelay));
  Dlg^.Insert(StartLab);
R.Assign(2,19,27,20);
nulalab := New(PStaticText, Init(R, ' 0   50  100 150 200 250'));
Dlg^.Insert(nulalab);

R.Assign(28,18,35,19);
SUDelayNum := New(PNumInputLineWithScr, Init(R, 6, 0, 255));
Dlg^.Insert(SUDelayNum);

R.Assign(47,16,59,18);
Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(62,16,74,18);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
RPaint^.linknum:=RPaintNum;
RPaintNum^.LinkScroll:=RPaint;
SUDelay^.LinkNum:=SUDelayNum;
SUDelayNum^.LinkScroll:=SuDelay;
UDelay^.Linknum:=UDelayNum;
UDelayNum^.LinkScroll:=UDelay;
VideoMode^.AttribM:=AttribMode;
VideoMode^.AtModelab:=AtModeLab;
VideoMode^.BorderM:=BorderMode;
VideoMode^.Hccb:=Hires;
BorderMode^.Hccb:=Hires;
Hires^.AtmodeLab:=AtmodeLab;
Hires^.RowLab:=RowLab;
Hires^.ExtraLab:=ExtraLab;
Hires^.StartLab:=StartLab;
Hires^.MinusLab:=MinusLab;
Hires^.nulalab:=NulaLab;
Hires^.tristalab:=TristaLab;
Hires^.RPaint:=RPaint;
Hires^.RPaintNum:=RPaintNum;
Hires^.SUDelay:=SUDelay;
Hires^.SUDelayNum:=SUDelayNum;
Hires^.UDelay:=UDelay;
Hires^.UDelayNum:=UDelayNum;
dlg^.setdata(videosetting);
if (videosetting.video in [0,7]) and
  (videosetting.border=2) and
  (videosetting.hires=1) then hires^.showhcr
  else hires^.showfast;
if (videosetting.video in [0,7]) and
  (videosetting.border=2) and (emultype in [0,1]) then hires^.show
  else hires^.hide;
Videomode^.Adjust;
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(videosetting);
  setcomopts;
end;
dispose(dlg,done);

end;


procedure mswindows;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:Word;
Begin
R.Assign(10,2,54,14);
New(Dlg, Init(R, 'MS Windows compatibility'));
Dlg^.helpctx:=hcmswindows;
R.Assign(6,2,38,3);
Control := New(PCheckboxes, Init(R,
  NewSItem('Test for virtual 386 mode',Nil)));
Dlg^.Insert(Control);

R.Assign(6,5,41,7);
Control := New(PCheckboxes, Init(R,
  NewSItem('Install task switch handler',
  NewSItem('Prevent access to printer',Nil))));
Dlg^.Insert(Control);

R.Assign(9,8,22,10);
Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(24,8,36,10);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
dlg^.setdata(winsetting);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(winsetting);
  setcomopts;
end;
dispose(dlg,done);

if winsetting.checkvmm<>0 then winsetting.wintests:=0;
end;
procedure tzxset;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
Begin
R.Assign(9,1,65,22);
New(Dlg, Init(R, 'TZX Settings'));
Dlg^.helpctx:=hctzxsetup;
R.Assign(1,1,55,18);
Control := New(PCheckboxes, Init(R,
  NewSItem('Sample standard blocks rather than copy them',
  NewSItem('Sample pause after standard block',
  NewSItem('Sample Turbo blocks rather than copy them',
  NewSItem('Sample pause after turbo block and pure data',
  NewSItem('Sample pure tone and sequence of pulses',
  NewSItem('Sample pure data rather than copy them',
  NewSItem('Embed messages in converted tape',
  NewSItem('Convert direct recording',
  NewSItem('Put all blocks and groups in separate samples',
  NewSItem('Allow negative jumps (not yet implemented)',
  NewSItem('Show text description blocks',
  NewSItem('Show message blocks',
  NewSItem('Extract snapshot block',
  NewSItem('Show warning for nonimplemented blocks',
  NewSItem('Consider hardware and emulation info',Nil)))))))))))))))));
Dlg^.Insert(Control);

R.Assign(4,18,18,20);
Control := New(PButton, Init(R, '~O~K', cmOK, bfNormal));
Dlg^.Insert(Control);

R.Assign(24,18,39,20);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
dlg^.setdata(TZX);
c:=desktop^.execview(dlg);
if c=cmOK then begin
  dlg^.getdata(TZX);
  setcomopts;
end;
dispose(dlg,done)
end;



(*
 

*)
function dirforma(a:string):string;
var d,n,e:string;
  datoteka:file;
begin
  dirforma:='';
  if a[length(a)]<>'\' then a:=a+'\';
  a:=a+'p';
  fsplit(fexpand(a),d,n,e);
  assign(datoteka,d+'$$.$$$');
  if not(frewriteu(datoteka,1)) then exit;
  if not(fclose(datoteka)) then exit;
  if not(ferase(datoteka)) then exit;
  dirforma:=d;
end;

function Parse(FileName:string;tape:Ptapefile;snap:PSnapshotfile;
  mdv:PMdvFile;dck:PDockFile):Boolean;
(* Vraca false ako ima sintaksna greska *)
 
var Dat:text;
  ParseStr,FileStr,snapstr,tapestr,dockstr,dirstr,chdirstr,D,N,E:string;
  mdvstr:array[1..8] of string;
  Ch,Opt:char;
  I,Err,Err1,trenutni:integer;
  Signature:Longint;
  datoteka:file;
  cc:boolean;
  timtrap:integer;
begin
  timtrap:=-1;
  snapstr:='';
  dirstr:='';
  tapestr:='';
  dockstr:='';
  for i:=1 to 8 do mdvstr[i]:='';
  Defaultfill; Parse:=false;
  Assign(Dat,FileName);
  if not (fResett(Dat)) then exit;
  if not(fReadc(Dat,Ch)) then begin
    cc:=fcloset(dat);
    exit
  end;

  repeat
    ParseStr:='';
    while not (Ch in [' ',#13,#10]) and not Eof(Dat) do begin
      ParseStr:=ParseStr+Ch;
      if not(fReadc(Dat,Ch)) then begin
        cc:=fcloset(dat);exit
      end;
    end;
    if ParseStr<>'' then begin
      if (ParseStr[1]<>'/') or (Length(ParseStr)=1) then Exit;
      Opt:=UpCase(ParseStr[2]); Delete(ParseStr,1,2);
      for I:=1 to Length(ParseStr) do ParseStr[I]:=UpCase(ParseStr[I]);
      if (Opt in ['2','B','C','H','K','N','O']) and (ParseStr<>'') then
        Exit;
      case Opt of
        '&': begin
          if length(parsestr)<1 then exit;
          case parsestr[1] of
            'E':begin
                  if (length(parsestr)<>2) or
                    not(parsestr[2] in ['0','1','2']) then exit;
                  emultype:=ord(parsestr[2])-48;
                  curemul:=emulators[emultype];
                end;
            'D':begin
                  dbasedir:=copy(parsestr,2,length(parsestr)-1);
                  dbasedir:=dirforma(dbasedir);
                end;
            'C':begin
                  if (length(parsestr)<>2) or
                    not(parsestr[2] in ['1','2']) then exit;
                    dfltrs232:=ord(parsestr[2])-49;
                end;
            'S':begin
                  if (length(parsestr)<>2) or
                    not(parsestr[2] in ['1'..'8']) then exit;
                    slbdrt:=ord(parsestr[2])-48;
                end;
            'O':begin
                  IndexedFields:=copy(parsestr,2,length(parsestr)-1);
                end;
             'T':begin
                  Val(copy(ParseStr,2,length(ParseStr)-1),TZX.CheckBoxes,Err);
                  if (TZX.CheckBoxes<0) or (Err<>0) then Exit
                 end;
            Else exit;
          end;
        end;
        '!':begin
              WinSetting.CheckVMM:=0;
              if parsestr<>'' then begin
                 Val(ParseStr,WinSetting.Wintests,Err);
                 if (WinSetting.Wintests<0) or
                   (WinSetting.Wintests>3) or
                 (Err<>0) then Exit
               end;
         end;
        '#':begin
              if (Upcase(ParseStr[1])='N') then begin
                if (length(ParseStr)<4) then exit;
                if (ParseStr[2]<>';') then exit;
                ParseStr:=Copy(ParseStr,3,255);
                i:=1;Network.Netfile:='';
                while (i<=length(ParseStr)) and (ParseStr[i]<>';') do begin
                  Network.Netfile:=Network.Netfile+ParseStr[i];
                  inc(i);
                end;
                ParseStr:=Copy(ParseStr,i+1,255);

                Network.PSWait:=30;
                Network.FreeNet:=5;
                Network.BusyWait:=5;
                if ParseStr<>'' then begin
                  val(ParseStr,Network.PSWait,Err);
                  if Err>0 then if ParseStr[Err]<>';' then exit;
                  if Err>0 then begin
                    val(Copy(ParseStr,1,Err-1),Network.PSWait,Err1);
                    ParseStr:=Copy(ParseStr,Err+1,255);

                    if ParseStr<>'' then begin
                      val(ParseStr,Network.FreeNet,Err);
                      if Err>0 then if ParseStr[Err]<>';' then exit;
                      if Err>0 then begin
                        val(Copy(ParseStr,1,Err-1),Network.FreeNet,Err1);
                        ParseStr:=Copy(ParseStr,Err+1,255);

                        if ParseStr<>'' then begin
                          val(ParseStr,Network.BusyWait,Err);
                          if Err>0 then exit;
                          ParseStr:=Copy(ParseStr,Err+1,255);
                        end
                      end
                    end
                  end
                end
              end
               else begin
                 if (ParseStr[2]<>';') or not (ParseStr[1] in ['1'..'8']) then
                   Exit;
                 FileStr:=Copy(ParseStr,3,255);
{*************************}

                 mdvstr[Ord(ParseStr[1])-48]:=FileStr
               end
            end;
        '%':begin
              if ParseStr='LMAX' then begin
                speedsetting.maxspeed:=speedsetting.maxspeed or 2;
              end
              else begin
                if ParseStr='MAX' then begin
                  speedsetting.slowdown:=800;
                  speedsetting.maxspeed:=speedsetting.maxspeed or 1;
                end
                  else begin
                    Val(ParseStr,speedsetting.slowdown,Err);
                    if (speedsetting.slowdown<0) or (Err<>0) then Exit
                  end;
                end
            end;

        '?':begin
          if ParseStr='128' then tests.tst128:=1 else
            if ParseStr='86' then tests.emb8086:=1 else Exit;

        end;
        '2':RomSetting.Spec2:=1;
        'A':if ParseStr='' then Tapesetting.autostart:=1
            else if ParseStr='-' then Tapesetting.autostart:=2 else Exit;
        'B':RomSetting.FastBas:=1;
        'C':; {CONFFILE:=1;}


        'D':if ParseStr<>'' then dirstr:=ParseStr else Exit;

{*************************}
        'E':if emultype in [0,1] then begin
              if ParseStr='' then with InterfSetting do begin
                switch:=1;
                OldROM:=0;
              end
              else with InterfSetting do begin
                if ParseStr='X' then begin switch:=2; oldrom:=0; end else
                if ParseStr='O' then begin switch:=1; oldrom:=1; end else
                if ParseStr='OX' then begin switch:=2; oldrom:=1; end else
                Exit;
              end;
            end
             else begin
               if parsestr='' then exit else dockstr:=parsestr;
             end;
        'F':begin
              Val(ParseStr,Tapesetting.position,Err);
              dec(tapesetting.position);
              if tapesetting.position=-1 then tapesetting.position:=0;
              if (Tapesetting.position<0) or (Err<>0) then Exit
            end;
        'H':TapeSetting.Hidebor:=1;
        'J':begin
              inc(timtrap);
              if timtrap=2 then timtrap:=1;
              if (Length(ParseStr)<>1) and (ParseStr[1]<>'A') then Exit;
              case ParseStr[1] of
                'N':;
                'K':begin
                    Joysetting.Joy1:=Joysetting.Joy1 or 1;
                    Joysetting.Joy2:=Joysetting.Joy2 or (1 shl (timtrap*4));
                end;
                'S':begin
                   Joysetting.Joy1:=Joysetting.Joy1 or 2;
                   Joysetting.Joy2:=Joysetting.Joy2 or $100;
                end;
                'B':begin
                   Joysetting.Joy1:=Joysetting.Joy1 or 4;
                   Joysetting.Joy2:=Joysetting.Joy2 or $200;
                 end;
                'C':begin
                   Joysetting.Joy1:=Joysetting.Joy1 or 8;
                   Joysetting.Joy2:=Joysetting.Joy2 or $400;
                end;
                'M':begin
                   Joysetting.Joy1:=Joysetting.Joy1 or 16;
                   Joysetting.Joy2:=Joysetting.Joy2 or (2 shl (timtrap*4));
                end;
                'A':if ParseStr='A' then begin
                    Joysetting.Joy1:=Joysetting.Joy1 or 32;
                    Joysetting.Joy2:=Joysetting.Joy2 or (4 shl (timtrap*4));
                end
                  else begin
                      if Length(ParseStr)<>13 then Exit;
                      Joysetting.Joy1:=Joysetting.Joy1 or 64;
                      Joysetting.Joy2:=Joysetting.Joy2 or (8 shl (timtrap*4));
                      Val(Copy(ParseStr,2,3),Joysetting.Xmin,Err);
                      if (Joysetting.Xmin<0) or (Err<>0) then Exit;
                      Val(Copy(ParseStr,5,3),Joysetting.Xmax,Err);
                      if (Joysetting.Xmax<0) or (Err<>0) then Exit;
                      Val(Copy(ParseStr,8,3),Joysetting.Ymin,Err);
                      if (Joysetting.ymin<0) or (Err<>0) then Exit;
                      Val(Copy(ParseStr,11,3),Joysetting.ymax,Err);
                      if (Joysetting.ymax<0) or (Err<>0) then Exit
                    end;
                else Exit
              end
            end;
        'K':Romsetting.invcur:=1;
        'L':begin
              if (Length(ParseStr)<>1) or not (ParseStr[1] in ['0'..'7']) then
                Exit;
              if parsestr[1] in ['6','7'] then SoundSetting.MicEar:=
                Ord(ParseStr[1])-48-5
              else begin
                Tapesetting.LoadMeth:=Ord(ParseStr[1])-48;
                if Tapesetting.LoadMeth>2 then begin
                  Tapesetting.stopmode:=1;
                  Tapesetting.loadmeth:=Tapesetting.loadmeth-3
                end
              end;
            end;
        'M':begin
              if (Length(ParseStr)<>1) or not (ParseStr[1] in ['0'..'3']) then
                Exit;
              Videosetting.Attrmode:=Ord(ParseStr[1])-48
            end;
        'N':; {NOLOGO:=1;}
        'O':; {FILTER:=1;}
        'P':begin
              if Length(ParseStr)<>1 then Exit;
              case ParseStr[1] of
                'E':Printersetting.ZXprinter:=1;
                'B':Printersetting.ZXprinter:=2;
                'L':Printersetting.ZXprinter:=3;
                'P':Printersetting.ZXprinter:=4;
                else Exit
              end
            end;
        'Q':begin
             if parsestr='' then exit;
             if parsestr='X' then begin
               VideoSetting.StartUla:=0;
               VideoSetting.ExtraUla:=0
             end
             else
             if parsestr[1]='X' then begin
                i:=pos(';',parsestr);
                if i=0 then begin
                  val(copy(parsestr,2,length(parsestr)-1),VideoSetting.ExtraULA,
                    Err);
                  if Err>0 then Exit
                end
                 else begin
                  val(copy(parsestr,2,i-2),VideoSetting.ExtraULA,Err);
                  if Err>0 then Exit;
                  val(copy(parsestr,i+1,length(parsestr)-1),
                    VideoSetting.StartULA,Err);
                  if Err>0 then Exit
                 end;
               videoSetting.Hires:=1;

             end
             else begin
               videosetting.hires:=0;
               val(parsestr,VideoSetting.RowFirst,Err);
               if Err<>0 then exit;
             end;
           end;
        'R':if ParseStr='' then snapstr:='SNAP.Z80'
          else snapstr:=ParseStr;
{************************************}

        'S':begin
              if (Length(ParseStr)=1) and (ParseStr[1] in ['0'..'7']) then begin
                SoundSetting.AYChan:=55-Ord(ParseStr[1]);
                SoundSetting.AudioDev:=0; {Radio button}
              end
              else if (Length(ParseStr)=1) and (upcase(ParseStr[1])='X' ) then begin
                SoundSetting.AYChan:=7;
                SoundSetting.AudioDev:=1; {Radio button}
              end
              else if (Length(ParseStr)=1) and (upcase(ParseStr[1])='O' ) then begin
                SoundSetting.AYChan:=7;
                SoundSetting.AudioDev:=2; {Radio button}
              end
              else if (Length(ParseStr)=1) and (upcase(ParseStr[1])='A' ) then begin
                SoundSetting.AYChan:=7;
                SoundSetting.AudioDev:=3; {Radio button}
              end
              else if (Length(ParseStr)=1) and (upcase(ParseStr[1])='B' ) then begin
                SoundSetting.AYChan:=7;
                SoundSetting.AudioDev:=4; {Radio button}
                SoundSetting.Autodetect:=1;
              end
              else if (Length(ParseStr)>1) and (upcase(ParseStr[1])='B' ) then begin
                SoundSetting.AYChan:=7;
                SoundSetting.AudioDev:=4; {Radio button}
                SoundSetting.Autodetect:=0;
                i:=pos(';',parsestr);
                if i=0 then begin
                  val(copy(parsestr,2,length(parsestr)-1),SoundSetting.SBAddr,
                    Err);
                  if Err>0 then Exit
                end
                 else begin
                  val(copy(parsestr,2,i-2),SoundSetting.SBAddr,Err);
                  if Err>0 then Exit;
                  val(copy(parsestr,i+1,length(parsestr)-1),
                    SoundSetting.DMA,Err);
                  if Err>0 then Exit
                 end;
              end
              else exit;
            end;
        'T':if ParseStr='' then Exit else tapestr:=ParseStr;

{*************************************}
        'U':begin
              if (Length(ParseStr)<>1) or not (ParseStr[1] in ['0'..'5']) then
                Exit;
              if (ParseStr[1]<'2') or (ParseStr[1]='4') then Soundsetting.Hear:=0;
              if ParseStr[1] in ['0','2'] then VideoSetting.Border:=0;
              if ParseStr[1] in ['4','5'] then VideoSetting.Border:=2;
            end;
        'V':begin
              if (Length(ParseStr)<>1) and (Length(ParseStr)<>49) then Exit;
              case ParseStr[1] of
                'C':Videosetting.Video:=1;
                'W':Videosetting.Video:=2;
                'M':Videosetting.Video:=3;
                'E':Videosetting.Video:=4;
                'H':Videosetting.Video:=5;
                'X':Videosetting.Video:=6;
                'V':Videosetting.Video:=7;
                'P':begin
                  if Length(ParseStr)<>49 then Exit;
                  palettestr:=copy(Parsestr,2,48);
                end;
                else Exit
              end
            end;
        'Y':begin
              Val(ParseStr,Tapesetting.initattr,Err);
              if (Tapesetting.initattr<0) or
                (Tapesetting.initattr>255) or (Err<>0) then Exit
            end;
      end
    end;
    while Ch in [' ',#13,#10] do
      if not(fReadc(Dat,Ch)) then begin
        cc:=fcloset(dat); exit
      end;

  until Eof(Dat);
  if not(fCloset(Dat)) then exit;
  chdirstr:=dirstr;
  if dirstr<>'' then begin
    if dirstr[length(dirstr)]='\' then dec(dirstr[0]);
    dirstr:=dirstr+'\a';
    FSplit(Fexpand(Dirstr), D, N, E);
    Dirstr:=D;
  end;
  if tapestr<>'' then begin
    FSplit(Tapestr, D, N, E);
    if D[Length(D)] = '\' then Dec(D[0]);
    if E='' then E:='.TAP';
    if dirstr<>'' then tapestr:=dirstr+N+E else
      Tapestr:=FExpand(tapestr);
    fsplit(tapestr,deftapedir,n,e);
    if length(deftapedir)>3 then
      if deftapedir[Length(deftapedir)] = '\' then Dec(Deftapedir[0]);
    if not(fileexists(tapestr)) then begin
      Displayerror ('Tape file does not exist',0);
      exit;
    end;
    with tape^ do if tapename='' then begin
      assign(datoteka,tapestr);
      if not(fresetu(datoteka,1)) then begin  exit end;
      if filesize(datoteka)<12 then waratape:=false else begin
        if not(fblockread(datoteka,signature,4)) then begin
          cc:=fclose(datoteka); exit; end;
        if not(fblockread(datoteka,signature,4)) then begin
          cc:=fclose(datoteka); exit; end;
        if not(fblockread(datoteka,signature,4)) then begin
          cc:=fclose(datoteka); exit; end;
        if signature<>$FFFFFFFF then begin
          waratape:=false
        end
          else waratape:=true;

        if not(fclose(datoteka)) then begin exit; end;
        tapename:=tapestr;
        allowed:=true;
        doreadblocks:=true;
      end
    end;
  end;
  if snapstr<>'' then  begin
    FSplit(Snapstr, D, N, E);
    if D[Length(D)] = '\' then Dec(D[0]);
    if E='' then E:='.Z80';
    if (dirstr<>'') and (D='') then snapstr:=dirstr+N+E else
      Snapstr:=FExpand(Snapstr);
    if not(fileexists(Snapstr)) then begin
      Displayerror ('Snapshot file does not exist',0);
      exit;
    end;

    with snap^ do if snapname='' then begin
      allowed:=true;
      snapname:=snapstr;
      getheader;
      if is48ksnap then emultype:=0 else emultype:=1;
      if istimex then emultype:=2;
      curemul:=emuldir+emulators[emultype];
    end;
  end;
  if dockstr<>'' then  begin
    FSplit(dockstr, D, N, E);
    if D[Length(D)] = '\' then Dec(D[0]);
    if E='' then E:='.DCK';
    if (dirstr<>'') and (D='') then dockstr:=dirstr+N+E else
      dockstr:=FExpand(dockstr);
    if not(fileexists(dockstr)) then begin
      Displayerror ('Dock file does not exist',0);
      exit;
    end;
    with dck^ do if dockname='' then begin
      allowed:=true;
      dockname:=dockstr;
    end;
  end;

  if emultype=2 then joysetting.joy1:=0 else joysetting.joy2:=0;
  for i:=1 to 8 do begin
    if mdvstr[i]<>'' then begin
      FSplit(fexpand(mdvstr[i]), D, N, E);
      if E='' then E:='.MDR';
      if D[Length(D)] = '\' then Dec(D[0]);
      if (dirstr<>'') and (D='') then mdvstr[i]:=dirstr+N+E else
        mdvstr[i]:=FExpand(mdvstr[i]);
      if not(fileexists(mdvstr[i])) then begin
        Displayerror ('Microdrive file does not exist',0);
        exit;
      end;

      with mdv^ do if mdvnames[i]='' then begin
        mdvnames[i]:=mdvstr[i];
        allowed:=true;
        if interfsetting.switch=0 then interfsetting.switch:=1;
        default:=i;
      end;
   end
  end;
  Parse:=true;
  setcomopts;
  if chdirstr<>'' then begin
    {$I-}
    chdir(chdirstr);
    i:=ioresult;
    {I+}
  end;
end;

function SaveCfg(FileName:string;tape:Ptapefile;snap:PSnapshotfile;
  mdv:PMdvFile;dck:PDockFile;sco:Boolean):Boolean;
var datoteka:text;
  d,n,e:string;
  i:integer;
  procedure we;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka);
  end;
begin
  Savecfg:=false;
  if sco then setcomopts;
  assign(datoteka,filename);
  if not(frewritet(datoteka)) then exit;
  if not(fwriteln(datoteka,comopts)) then begin we; exit; end;
  if tape^.tapename<>'' then begin
    fsplit(tape^.tapename,d,n,e);
    if (length(d)>3) and (d[length(d)]='\') then dec(d[0]);
    if not(fwriteln(datoteka,'/D'+d+' ')) then begin we; exit; end;
    if not(fwriteln(datoteka,'/T'+n+e+' ')) then begin we; exit; end;
  end
   else Begin
    if deftapedir<>'' then
      if not(fwriteln(datoteka,'/D'+deftapedir+' ')) then begin we; exit; end;
   end;
  if (netopts<>'') and (emultype<>2) then
     if not(fwriteln(datoteka,'/#N;'+netopts+' ')) then begin we; exit; end;

  if snap^.snapname<>'' then begin
    if not(fwriteln(datoteka,'/R'+snap^.snapname+' ')) then
      begin we; exit;
    end;
  end;
  if dck^.dockname<>'' then if emultype=2 then begin
    if not(fwriteln(datoteka,'/E'+dck^.dockname+' ')) then
      begin we; exit;
    end;
  end;

  if emultype<>2 then
    for i:=1 to 8 do begin
      if mdv^.mdvnames[i]<>'' then begin
        if not(fwriteln(datoteka,'/#'+chr(48+i)+';'+
        mdv^.mdvnames[i]+' ')) then
        begin we; exit;
      end;
    end;
  end;

  if not(fwriteln(datoteka,'/&E'+chr(emultype+48))) then begin we;exit end;
  if not(fwriteln(datoteka,'/&D'+dbasedir)) then begin we; exit end;
  if not(fwriteln(datoteka,'/&C'+chr(dfltrs232+49))) then begin we;exit end;
  if not(fwriteln(datoteka,'/&S'+chr(slbdrt+48))) then begin we;exit end;
  if not(fwriteln(datoteka,'/&O'+indexedfields)) then begin we;exit end;
  str(tzx.checkboxes,d);
  if not(fwriteln(datoteka,'/&T'+d)) then begin we;exit end;
  if not(fcloset(datoteka)) then begin exit end;
  Savecfg:=true;
end;

end.
