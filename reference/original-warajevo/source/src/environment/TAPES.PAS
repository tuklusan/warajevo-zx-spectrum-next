
{$V-,N+,E+,O+,F+}
unit tapes;
interface
uses objects;
Type
  schar=set of char;
  Blocktype=record
    naziv:string[50];
    pocetak:longint;
    duzina1:word;
    flag:byte;
    duzdec,duzcomp,duzsign:word;
  end; {Blocktype}
  Blockarray=array[1..999] of Blocktype;
  Blockarrayp=^Blockarray;

  PTapeFIle=^TtapeFile;
  TTapeFile=Object(TObject)
    buffer,izlbuffer:Pointer;
    noofblocks:word;
    tapename:String;
    trakadat:File;
    blocks:BlockArrayp;
    allowed:Boolean;
    doreadblocks:Boolean;
    waratape:Boolean;
    constructor init(tapefile:String);
    function openfile:Boolean;
    procedure unselect;
    procedure readblocks(FileName:string);
    procedure prepis;
    procedure Reorder;
    procedure delete;
    procedure extract;
    procedure exclude;
    procedure view;
    procedure tapepar;
    procedure add;
    procedure ChgPos;
    procedure printscreen;
    procedure printtoprinterorfile(mode:integer);
    procedure printtoprinter;
    procedure printtofile;
    procedure edit;
    procedure copytonew;
    procedure unpack;
    procedure pack(izbor:integer);
    procedure Linearize;
    procedure compressAll;
    procedure compressSelected;
    procedure efficiency;
    procedure sendtospectrum;
    procedure receivefromspectrum;
    procedure cfgrs232;
    function loadblock(rb:integer):word;
    function generalconvert(kontekst:integer):word;
    function GeneralAsciiToOther(var pozicija:longint;
      var filename:string; kontekst:integer):boolean;
    function isheader(rb:integer):Boolean;
    function loadheader(rb:integer; var hedduzina,stvduzina:word):Boolean;
    procedure asciibasic;
    procedure basascii;
    procedure asciigens;
    procedure gensascii;
    procedure asciiforth;
    procedure forthascii;

    procedure tlwascii;
    procedure asciiml;
    procedure mlascii;
    procedure asciipas;
    procedure pasascii;
    procedure asciic;
    procedure Cascii;
    procedure asciilogo;
    procedure logoascii;
    procedure screentifbw;
    procedure screentifcl;
    procedure tswrdascii(verzija:integer);
    procedure asciitlwtswrd(verzija:integer);
    procedure romwara;
    procedure wararoman;
    procedure lunterwara;
    procedure waralunter;
    procedure warairish;
    procedure lunterpolishwara(tip:integer);
    procedure waralunterpolish(tip:integer);
    procedure polishwara;
    procedure warapolish;
    procedure spect20wara;
    procedure garabikwara;
    procedure waraspect20;
    procedure waragarabik;
    procedure warabrukner;
    procedure waratzx;
    procedure waravoc;
    procedure tzxwara;
    procedure vocwara;
    procedure bruknerwara;
    procedure irishwara;
    procedure warashalaev;
    procedure shalaevwara;
    procedure waramuseum;
    procedure museumwara;
    procedure warazx32;
    procedure zx32wara;
    procedure receivecass;
    procedure receivesample(unr:byte);
    procedure advcset;
    destructor done;virtual;
  end;
  var
    Tapesetting : record
      position : Word;  {Listbox}
      Autostart : Word; {Checkboxes}
      Hidebor : Word; {Checkboxes}
      loadmeth : Word; {RadioButtons}
      Stopmode : Word; {Checkboxes}
      Initattr : LongInt; {Inputlong}
      end;


implementation
uses
   Drivers, Views, Menus, App,dialogs,iorutine,dos
   ,imprek,stddlg,hexedito,msgbox,konvtap,helpctx,moucur,textview
   ;

const prazna:array[1..12] of byte=(4,0,0,0,0,0,0,0,255,255,255,255);
constructor TTapeFile.Init(tapefile:string);
var datoteka:file;
  signature:longint;

  procedure we;
  var cc:Boolean;
  begin
    allowed:=false;
    tapename:='';
    cc:=fclose(datoteka);
  end;

begin
  doreadblocks:=false;
  new(blocks);
  getmem(buffer,65520);
  getmem(izlbuffer,65520);

  if fileexists(tapefile) then begin
    assign(datoteka,tapefile);
    if not(fresetu(datoteka,1)) then begin we; exit; end;
    if filesize(datoteka)<12 then waratape:=false else begin
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if signature<>$FFFFFFFF then begin
        waratape:=false
      end
        else waratape:=true;
    end;

    if not(fclose(datoteka)) then begin exit; end;
    readblocks(tapefile);
    allowed:=true;
    tapename:=tapefile;
  end
   else begin

     allowed:=false;
     tapename:='';
   end;
end;

destructor TTapeFile.Done;
begin
  dispose(blocks);
  freemem(buffer,65520);
  freemem(izlbuffer,65520);
end;

procedure TTapefile.Readblocks(Filename:string);
var
  p1,naredni,prethodni,trenutni:longint;
  w1,w2,w3,duzina,duzdec1,duzcomp1,duzsign1:word;
  fleg,tip:byte;
  znak:char;
  i:integer;
  cc:Boolean;
  dumstr:string;
  procedure we;
  begin
    cc:=fclose(trakadat);
    noofblocks:=0;
    filename:=''
  end;
procedure createinfoline;
var i:integer;
begin
with Blocks^[noofblocks] do begin
  if (duzina<>17) or (flag<>0) or(tip>=4) then begin
    naziv:=naziv+'Headerless len '+dumstr;
    str (flag,dumstr);
    naziv:=naziv+'  F:'+dumstr
  end
   else begin
     for i:=1 to 10 do begin
       if not(fblockread(trakadat,znak,1)) then begin
         we; exit
       end;
       if (znak<' ') or (znak>'~') then znak:='þ';
       naziv:=naziv+znak
     end;
     if not(fblockread(trakadat,w1,2))then begin we;exit end;
     if not(fblockread(trakadat,w2,2))then begin we;exit end;
     if not(fblockread(trakadat,w3,2))then begin we;exit end;
     naziv:=naziv+' ';
     case tip of
       0: begin
         str(w2,dumstr);
         if w2<=9999 then naziv:=naziv+'run '+dumstr+', '
          else naziv:=naziv+'bas ';
         str(w3,dumstr);
         naziv:=naziv+dumstr;
         str(w1-w3,dumstr);
         naziv:=naziv+'+'+dumstr;

       end;
       1: begin
         naziv:=naziv+'num '+chr(((w2 div 256) and $1f)+64)+'()';
         str(w1,dumstr);
         naziv:=naziv+', '+dumstr;
       end;
       2: begin
         naziv:=naziv+'chr '+chr(((w2 div 256) and $1f)+64)+'$()';
         str(w1,dumstr);
         naziv:=naziv+', '+dumstr;
       end;
       3: begin
         str(w2,dumstr);
         naziv:=naziv+'byt '+dumstr;
         str(w1,dumstr);
         naziv:=naziv+', '+dumstr;
       end;
     end; {case}
   end; {else}
   fillchar(dumstr[1],32,' ');
   naziv:=naziv+dumstr;
end;
end;
begin
hourglascur;
noofblocks:=0;
  filerec(trakadat).mode:=fmInput;
  assign(trakadat,filename);
  if not(fresetu(trakadat,1)) then exit;
  if waratape then begin
     if not(fblockread(trakadat,p1,4)) then begin we;exit end;
     if not(fseek(trakadat,p1)) then begin we;exit end;
     if not(fblockread(trakadat,prethodni,4))then begin we;exit end;
     if not(fblockread(trakadat,naredni,4)) then begin we;exit  end;
     trenutni:=p1;
     while not (naredni=$FFFFFFFF) do begin
       if not(fseek(trakadat,trenutni))then begin we;exit end;
       if not(fblockread(trakadat,prethodni,4)) then begin we;exit end;
       if not(fblockread(trakadat,naredni,4)) then begin we;exit end;
       if not(fblockread(trakadat,duzina,2))  then begin we;exit end;
       if not(fblockread(trakadat,fleg,1)) then begin we;exit end;
       if duzina>=65534 then begin
         if not(fblockread(trakadat,duzdec1,2)) then begin we;exit end;
         if not(fblockread(trakadat,duzcomp1,2)) then begin we;exit end;
         if not(fblockread(trakadat,duzsign1,2)) then begin we;exit end;
       end
        else
         if duzina=0 then tip:=4
          else begin
            if not(fblockread(trakadat,tip,1)) then begin we;exit end;
          end;
         if naredni<>$ffffffff then begin
           noofblocks:=noofblocks+1;
           with blocks^[noofblocks] do begin
             pocetak:=trenutni;
             duzina1:=duzina;
             flag:=fleg;
             if duzina>=65534 then begin
               duzdec:=duzdec1;
               duzcomp:=duzcomp1;
               duzsign:=duzsign1;
             end;
             str(noofblocks:3,dumstr);
             naziv:=dumstr+'. ';
             str(duzina,dumstr);
             (*  Specijalno: duzina=65535  *)
             if duzina=65535 then begin
               naziv:=naziv+'Compressed len ';
               str(duzdec,dumstr);
               naziv:=naziv+dumstr;
               str(duzcomp,dumstr);
               naziv:=naziv+' ('+dumstr+')';
             end
               else if (duzina=65534) then begin
                 naziv:=naziv+'EAR Sample len ';
                 str(duzdec,dumstr);
                 naziv:=naziv+dumstr;
                 str(duzcomp,dumstr);
                 naziv:=naziv+' ('+dumstr+')';
                end
              else CreateInfoLine;
           end;  {With}
           if not(fseek(trakadat,naredni)) then begin we;exit end;
           trenutni:=naredni;
         end; {if}
     end; {While}
  end
    else begin {Lunter tape}
      while not(eof(trakadat)) do begin
        inc(noofblocks);
        with blocks^[noofblocks] do begin
        pocetak:=filepos(trakadat);
        if not(fblockread(trakadat,duzina,2)) then begin we; exit; end;
        duzina:=duzina-2;
        duzina1:=duzina;
        if not(fblockread(trakadat,flag,1)) then begin we; exit; end;
        if duzina1=0 then tip:=4 else
          if not(fblockread(trakadat,tip,1)) then begin we; exit; end;
        str(noofblocks:3,dumstr);
        naziv:=dumstr+'. ';
        str(duzina,dumstr);
        createinfoline;
        if not(fseek(trakadat,pocetak+duzina+4)) then begin we;exit end;
      end;
      end;
    end;
    cc:=fclose(trakadat);
end;  {Readblocks}
{
********************************************
    Opcija Select tape
********************************************
}


function TTapeFile.OpenFile:Boolean;
var a:PFileDialog;
  c:integer;
  signature:Longint;
  fname:string;
  datoteka:file;

  procedure we;
  var cc:boolean;
  begin
    cc:=fclose(datoteka);
    allowed:=false;
    dispose(a,done);
    openfile:=false;
  end;
begin
  a:=new(PFileDIalog,Init('*.TAP','Select tape name','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcfsdialog;
  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    a^.getfilename(fname);
    fname:=fexpand(fname);
    if not(fileexists(fname)) then begin
      assign(datoteka,fname);
      if not(frewriteu(datoteka,1)) then begin we; exit; end;
      if not(fblockwrite(datoteka,prazna,12)) then begin we; exit; end;
      if not(fclose(datoteka)) then begin allowed:=false end;
    end;
    assign(datoteka,fname);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    if filesize(datoteka)<12 then waratape:=false else begin
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if not(fblockread(datoteka,signature,4)) then begin we; exit; end;
      if signature<>$FFFFFFFF then begin
        waratape:=false
      end
        else waratape:=true;
    end;
    if not(fclose(datoteka)) then begin we; exit end;
    tapename:=fname;
    allowed:=true;
    readblocks(tapename);
    openfile:=true;
    With Tapesetting do begin
      position:=0;
      Autostart:=0;
      Hidebor:=0;
      loadmeth:=0;
      Stopmode:=0;
    end;

  end
   else openfile:=false;
  dispose(a,done);
end;

procedure TTapefile.unselect;
begin
  tapename:='';
  allowed:=false;
  With Tapesetting do begin
   position:=0;
   Autostart:=0;
   Hidebor:=0;
   loadmeth:=0;
   Stopmode:=0;
   Initattr:=7;
  end;

end;


procedure TtapeFile.prepis;

var
  i:integer;
  cc:Boolean;
  d,n,e:String;
  datoteka,datoteka2:file;
  daljinar:PProcenter;

  procedure we;
  begin
    readblocks(tapename);
  end;

  procedure we1;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    readblocks(tapename);
  end;

begin
  hourglascur;
  fsplit(tapename,d,n,e);
  assign(datoteka,tapename);
  if fileexists(n+'.BAK') then begin
    assign (datoteka2,n+'.BAK');
    if not(ferase(datoteka2)) then begin we;exit end;
  end;
  if not(frename(datoteka,n+'.BAK')) then begin
     we; exit
  end;
  assign(datoteka2,tapename);
  if not(fresetu(datoteka,1)) then begin we;exit end;
  if not(frewriteu(datoteka2,1)) then begin
    cc:=frename(datoteka,n+'.TAP');
    cc:=fclose(datoteka);
    we;
    exit;
  end;
  if waratape then
    if not(fblockwrite(datoteka2,prazna,12)) then begin
      cc:=fclose(datoteka);
      cc:=fclose(datoteka2);
      we;exit
    end;
  if not(fclose(datoteka)) then begin
    cc:=fclose(datoteka2);we;exit
  end;
  if not(fclose(datoteka2)) then begin we;exit end;


  new(daljinar,Init(16,10,52,1,noofblocks));
  desktop^.insert(daljinar);

  for i:=1 to noofblocks do begin
    daljinar^.update(i,blocks^[I].naziv);
    if blocks^[i].naziv[5]=' ' then begin
      if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
        we1;
        exit;
      end;
      if not(save(waratape,tapename,blocks^[i].duzina1,4,
        blocks^[i].flag,buffer^)) then begin
          we1;exit
      end;
    end;
  end;
  readblocks(tapename);
  desktop^.delete(daljinar);
  dispose(daljinar,done);
end; {prepis}

{ ****************************************
   Opcija Reorder
  ****************************************
}

{ *************************************
  Klasa listboxa za pomjeranje blokova
  *************************************
  }

type
  TMovableBlocksLister=object(tlistbox)
    blocks:BlockArrayP;
    swapaj:boolean;
    function GetText(item:integer; maxlen:integer):string; virtual;
    procedure HandleEvent(var Event: TEvent); virtual;
    procedure exchange(x,y:integer); virtual;
    procedure selectitem(item:integer); virtual;
    constructor init(var Bounds:TRect; ANumCols:Integer; AScrollBar:
       PScrollBar; pblocks:Pointer);
  end;
  pMovableBlocksLister=^tMovableBlocksLister;

constructor TMovableBlocksLister.init(var Bounds:TRect; ANumCols:Integer;
  AScrollBar: PScrollBar; pblocks:Pointer);
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  blocks:=pblocks;
  swapaj:=false
end;

procedure TMovableBlocksLister.exchange(x,y:integer);
var dummy:blocktype;
begin
  dummy:=blocks^[x+1];
  blocks^[x+1]:=blocks^[y+1];
  blocks^[y+1]:=dummy
end; {zamijeni}



function TMovableBlocksLister.GetText(item:integer; maxlen:integer):string;
begin
   GetText:=copy(blocks^[item+1].naziv,1,maxlen);
end;

procedure TMovableBlocksLister.selectitem(item:integer);
begin
  swapaj:=not(swapaj);
end;

procedure TMovableBlocksLister.HandleEvent(var Event: TEvent);
var oldfoc,f:integer;

begin
  oldfoc:=focused;
  Tlistviewer.HandleEvent(Event);
  if event.What=evKeyDown then
     if event.keycode=kbEnter then selectitem(focused);

  if swapaj then begin
     if focused>oldfoc then for f:=oldfoc to focused-1 do
      if f<range then exchange(f,f+1);

     if focused<oldfoc then for f:=oldfoc downto focused+1 do
      if f>0 then exchange(f,f-1);
  end;
  Draw;
end;



procedure TTapeFile.Reorder;
var a:PDialog;
    R:TRect;
    c:word;
    ll:PMovableBlocksLister;
    Bruce:Pointer;
    skrolnica:pscrollbar;
begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   if noofblocks=1 then begin
     displayerror('Tape #'+tapename+'# contains only one block',0);
     exit
   end;
   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Reorder'));
   a^.helpctx:=hcReorder;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'Cancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PMovableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then prepis;
   readblocks(tapename);
end;




{ ****************************************
   Opcija Delete
  ****************************************
}

{ *************************************
  Klasa listboxa za izbor blokova
  *************************************
  }

type
  TTagableBlocksLister=object(tlistbox)
    blocks:BlockArrayP;
    function GetText(item:integer; maxlen:integer):string; virtual;
    procedure HandleEvent(var Event: TEvent); virtual;
    procedure selectitem(item:integer); virtual;
    constructor init(var Bounds:TRect; ANumCols:Integer; AScrollBar:
       PScrollBar; pblocks:Pointer);
  end;
  pTagableBlocksLister=^tTagableBlocksLister;

constructor TTagableBlocksLister.init(var Bounds:TRect; ANumCols:Integer;
  AScrollBar: PScrollBar; pblocks:Pointer);
var i:integer;
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  blocks:=pblocks;
  for i:=0 to range-1 do blocks^[i].naziv[5]:=' ';
end;

function TTagableBlocksLister.GetText(item:integer; maxlen:integer):string;
begin
   GetText:=copy(blocks^[item+1].naziv,1,maxlen);
end;

procedure TTagableBlocksLister.Selectitem(item:integer);
begin
   if blocks^[item+1].naziv[5]=' ' then blocks^[item+1].naziv[5]:=''
     else blocks^[item+1].naziv[5]:=' '
end;

procedure TTagableBlocksLister.HandleEvent(var Event: TEvent);

begin
  if event.What=evKeyDown then
     if event.keycode=kbEnter then event.keycode:=32;

  Tlistviewer.HandleEvent(Event);
  Draw;
end;


procedure TTapeFile.Delete;
var a:PDialog;
    R:TRect;
    c:word;
    ll:PTagableBlocksLister;
    Bruce:Pointer;
    skrolnica:pscrollbar;
begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Delete'));
   a^.helpctx:=hcdelete;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PTagableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then prepis;
   readblocks(tapename);
end;
{
**********************************************
Opcija Extract
**********************************************
}
procedure TTapeFile.Extract;
var a:PDialog;
    R:TRect;
    c:word;
    ll:PTagableBlocksLister;
    radio:pradiobuttons;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    radiodata:word;
    cc:Boolean;
    d,n,e,dummystr:string;
    datoteka:file;
    i:integer;

   procedure we;
   begin
     cc:=fclose(datoteka);
     readblocks(tapename);
   end;

begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   R.assign(4,2,76,22);
   a:=new(PDialog,Init(R,'Extract'));
   a^.helpctx:=hcextract;
   R.assign(41,2,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(43,7, 70,9);
   Radio := New(PRadioButtons, Init(R,
      NewSItem('Flagbyte is ~F~irst byte',
      NewSItem('Flagbyte is ~E~xtension',
      nil))
   ));
   a^.Insert(Radio);

   R.Assign(45,1,55,3);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,4,55,6);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,2,40,18);
   ll:=new(PTagableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   r.assign(1,1,40,2);
   a^.insert(New(plabel,Init(r,'~B~locks to be extracted',ll)));

   c:=desktop^.execview(a);
   hourglascur;
   radio^.getdata(radiodata);
   fsplit(tapename,d,n,e);
   dispose(a,done);
   if c=cmOK then begin

     for i:=1 to noofblocks do begin
       if blocks^[i].naziv[5]<>' ' then begin
         if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin
           we;exit
         end;
         str(i,dummystr);


         if radiodata=0 then begin
           assign(datoteka,n+'.'+dummystr);
           if not(frewriteu(datoteka,1)) then begin
             we;
             readblocks(tapename);
             exit;
           end;
           if not(fblockwrite(datoteka,blocks^[i].flag,1)) then begin
             we;
             exit
           end
         end
          else begin
            fsplit(tapename,d,n,e);
            if length(n)>5 then n:=copy(n,1,5);
            n:=n+dummystr;
            str(blocks^[i].flag,dummystr);
            assign(datoteka,n+'.'+dummystr);
            if not(frewriteu(datoteka,1)) then begin
              cc:=ferase(datoteka);
              readblocks(tapename);
              exit;
            end;
          end; {else /izbor <>2/}
          (* Specijalno ako je duzina 65535 *)
          if blocks^[i].duzina1>=65534 then begin
            decompress(ptr(seg(buffer^),ofs(buffer^)+6)
             ,izlbuffer,blocks^[i].duzdec,
             blocks^[i].duzcomp,blocks^[i].duzsign);
            if not(fblockwrite(datoteka,
              izlbuffer^,blocks^[i].duzdec-1)) then begin
                we;
                exit
            end;
          end
           else if not(fblockwrite(
             datoteka,buffer^,blocks^[i].duzina1)) then begin
               we;
               exit
           end;
          if not(fclose(datoteka)) then begin
             readblocks(tapename);exit
          end
       end; {if blocks^[i].naziv<>' '}
     end
   end {else /regular /};
   readblocks(tapename);
end;

{ Exclude
}
{
  ******************************************
   Opcija Exclude
  ******************************************
}
Type
   TSimpleBlocksView=Object(TListBox)
     blocks:BlockArrayP;
     function GetText(item:integer; maxlen:integer):string;virtual;
     function Datasize:Word; virtual;
     procedure SetData(var Rec); virtual;
     procedure GetData (var Rec); virtual;
      constructor init(var Bounds:TRect; ANumCols:Integer; AScrollBar:
       PScrollBar; pblocks:Pointer);
   end;
   PSimpleBlocksView=^TSimpleBlocksView;

constructor TSimpleBlocksView.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar; pblocks:Pointer);
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  blocks:=pblocks;
end;

function TSimpleBlocksView.Datasize:Word;
begin
  Datasize:=2;
end;

procedure TSimpleBlocksView.getdata(var Rec);
begin
  move(focused,rec,2);
end;

procedure TSimpleBlocksView.setdata(var Rec);
begin
  focusitem (word(rec));
end;

function TSimpleBlocksView.GetText(item:integer; maxlen:integer):string;
begin
   GetText:=copy(blocks^[item+1].naziv,1,maxlen);
end;


procedure TTapeFile.Exclude;
var a:PDialog;
    R:TRect;
    c,rb:word;
    datoteka:file;
    ll:PSimpleBlocksView;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    prethodni,naredni:longint;
    cc:Boolean;

    procedure we;
    begin
      cc:=fclose(datoteka);
      readblocks(tapename);
    end;

begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Exclude'));
   a^.helpctx:=hcexclude;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PSimpleBlocksView,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then begin
     rb:=ll^.focused+1;
     assign(datoteka,tapename);
     if not(fresetu(datoteka,1)) then begin readblocks(tapename);exit;end;
     if not(fseek(datoteka,blocks^[rb].pocetak)) then begin
       we;exit
     end;
     if not(fblockread(datoteka,prethodni,4)) then begin we;exit end;
     if not(fblockread(datoteka,naredni,4)) then begin we;exit end;
     if not(fseek(datoteka,naredni)) then begin we;exit end;
     if not(fblockwrite(datoteka,prethodni,4))then begin we;exit end;
     if not(fseek(datoteka,prethodni+4)) then begin we;exit end;
     if prethodni=0 then if not(fseek(datoteka,0))then begin
        we;exit
     end;
     if not(fblockwrite(datoteka,naredni,4)) then begin we;exit end;
     cc:=fclose(datoteka);
     readblocks(tapename);
   end;

   readblocks(tapename);
end;
{ ********************************************************
   View Tape
  ********************************************************
}

procedure TTapeFile.View;
var a: PDialog;
  R:TRect;
  skrolnica:PScrollBar;
  ll:PSImpleBlocksView;
  Bruce:Pointer;
  c:word;
begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'View'));
   a^.helpctx:=hcview;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PSimpleBlocksView,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
end;

{ ****************************************
   Tape parameters
  ****************************************
}

Procedure TTapefile.Tapepar;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  skrolnica:PScrollBar;
  ll:PSImpleBlocksView;
  c:word;

Begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

  R.Assign(4,1,72,21);
  New(Dlg, Init(R, 'Tape parameters'));
  Dlg^.helpctx:=hctparameters;
  R.Assign(56,3,57,11);
  Skrolnica := New(PScrollBar, Init(R));
  Dlg^.Insert(Skrolnica);

  R.Assign(4,3,56,11);
  ll:=new(PSimpleBlocksView,Init(R,1,skrolnica,blocks));
  ll^.setrange(noofblocks);
  ll^.focusitem(0);
  Dlg^.Insert(ll);

  R.Assign(4,1,46,2);
  Dlg^.Insert(New(PLabel, Init(R, 'Tape ~p~osition', ll)));

  R.Assign(4,12,23,13);
  Control := New(PCheckboxes, Init(R,
    NewSItem('Autostart',Nil)));
  Dlg^.Insert(Control);

  R.Assign(4,14,21,15);
  Control := New(PCheckboxes, Init(R,
    NewSItem('Hide BORDER',Nil)));
  Dlg^.Insert(Control);

  R.Assign(33,13,43,16);
  Control := New(PRadiobuttons, Init(R,
    NewSItem('Auto',
    NewSItem('Slow',
    NewSItem('Fast',Nil)))));
  Dlg^.Insert(Control);

  R.Assign(31,12,46,13);
  Dlg^.Insert(New(PLabel, Init(R, 'Loading ~m~ethod', Control)));

  R.Assign(31,16,46,17);
  Control := New(PCheckboxes, Init(R,
    NewSItem('Stop mode',Nil)));
  Dlg^.Insert(Control);

  R.Assign(5,17,10,18);
  Control := New(PNumInputLine, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(4,16,28,17);
  Dlg^.Insert(New(PLabel, Init(R, 'Initial color ~a~ttribute', Control)));

  R.Assign(50,12,63,14);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(50,15,63,17);
  Control := New(PButton, Init(R, 'Cancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  dlg^.setdata(tapesetting);
  c:=desktop^.execview(Dlg);
  hourglascur;
  if c=cmOK then dlg^.getdata(tapesetting);
  dispose(dlg,done);
end;


{
 ***********************************************
           Add block
 ***********************************************
}
type TBlocksViewWithLast=Object(TListBox)
     blocks:BlockArrayP;
     function GetText(item:integer; maxlen:integer):string;virtual;
     procedure setrange(arange:integer);

      constructor init(var Bounds:TRect; ANumCols:Integer; AScrollBar:
       PScrollBar; pblocks:Pointer);
   end;
   PBlocksViewWithLast=^TBlocksViewWithLast;

constructor TBlocksViewWithLast.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar; pblocks:Pointer);
begin
  TListBox.init(Bounds,AnumCols,AscrollBar);
  blocks:=pblocks;
end;
procedure TBlocksViewWithLast.setrange(arange:integer);
begin
  TListbox.setrange(arange+1);
end;

function TBlocksViewWithLast.GetText(item:integer; maxlen:integer):string;
begin
   if item<>range-1 then
     GetText:=copy(blocks^[item+1].naziv,1,maxlen)
   else GetText:=copy('   - End of tape - ',1,maxlen);
end;

procedure TTapeFile.Add;

var a:PDialog;
    fd:PFileDialog;
    R:TRect;
    c,rb:word;
    ll:PBlocksViewWithLast;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    radio:PRadioButtons;
    radiodata:word;
    cc:Boolean;
    d,n,e,dummystr,blockname,ekstenzija:string;
    datoteka:file;
    i,dummy:integer;
    pozicija,fs:Longint;
    fleg:byte;

   procedure we;
   begin
      cc:=fclose(datoteka);
      readblocks(tapename);
   end;

begin
   if not(allowed) then exit;
   fd:=new(PFileDIalog,Init('*.*','Select block to add','~N~ame',
      fdOpenButton,100));
   fd^.helpctx:=hcadd;
   if desktop^.execview(fd)<> cmcancel then begin
     hourglascur;
     fd^.getfilename(blockname);
     blockname:=fexpand(blockname);
     if not(fileexists(blockname)) then begin dispose(fd,done); exit end;
   end
    else begin dispose(fd,done); exit end;
   dispose(fd,done);
   assign(datoteka,blockname);
   if not(fresetu(datoteka,1)) then begin we; exit  end;
   if filesize(datoteka)>65520 then begin
     displayerror('This file is too long for Spectrum',0);
     we; exit;
   end;
   if not(fclose(datoteka)) then begin we; exit end;
   R.assign(4,2,76,22);
   a:=new(PDialog,Init(R,'Add block'));
   a^.helpctx:=hcadd;
   R.Assign(43,7, 70,9);
   Radio := New(PRadioButtons, Init(R,
     NewSItem('Flagbyte is ~F~irst byte',
     NewSItem('Flagbyte is ~E~xtension',
     nil))
   ));
   a^.Insert(Radio);

   R.assign(41,2,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,1,55,3);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,4,55,6);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,2,40,18);
   ll:=new(PBlocksViewWithLast,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   if not(waratape) then begin
     ll^.focusitem(noofblocks);
     ll^.options:=4;
     A^.SelectNext(false);
   end;
   r.assign(1,1,40,2);
   a^.insert(New(plabel,Init(r,'~S~tore before',ll)));

   c:=desktop^.execview(a);
   hourglascur;
   radio^.getdata(radiodata);
   dispose(a,done);
   if c=cmOK then begin
     rb:=ll^.focused+1;
     if rb<>noofblocks+1 then pozicija:=blocks^[rb].pocetak
      else
        if (noofblocks=0) or not(waratape) then pozicija:=4
         else begin
           assign(datoteka,tapename);
           if not(fresetu(datoteka,1)) then begin
             readblocks(tapename);exit;
           end;
           if not(fseek(datoteka,blocks^[noofblocks].pocetak)) then begin
             we;exit
           end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fclose(datoteka)) then begin readblocks(tapename);exit end;
         end; {else /noofblocks<>0/}
     assign(datoteka,blockname);
     if not(fresetu(datoteka,1)) then begin readblocks(tapename);exit end;
     case radiodata of
      0: begin
        if not(fblockread(datoteka,fleg,1)) then begin we;exit end;
        fs:=filesize(datoteka)-1;
        if not(fblockread(datoteka,buffer^,fs)) then begin we;exit end;
        if not(fclose(datoteka)) then begin readblocks(tapename);exit end;
        cc:=save(waratape,tapename,fs,pozicija,fleg,buffer^);
        readblocks(tapename);
      end;
      1: begin
        fsplit(blockname,d,n,e);
        ekstenzija:=e+' ';
        ekstenzija[1]:='0';
        ekstenzija:=copy(ekstenzija,1,pos(' ',ekstenzija)-1);
        val(ekstenzija,fleg,dummy);
        if dummy<>0 then fleg:=255;
        fs:=filesize(datoteka);
        if not(fblockread(datoteka,buffer^,fs)) then begin we;exit end;
        if not(fclose(datoteka)) then begin readblocks(tapename);exit end;
        cc:=save(waratape,tapename,fs,pozicija,fleg,buffer^);
        readblocks(tapename);
      end
    end;  {Case}
   end;
end;

{
  **************************************
    Change position
  **************************************
}
type TMovableOnlyOnce=Object(TMovableBlocksLister)
      Procedure SelectItem(Item:integer); virtual;
    end;
    PMovableOnlyOnce=^TMovableOnlyOnce;

Procedure TMovableOnlyOnce.SelectItem(Item:integer);
begin
  if not(swapaj) then swapaj:=true
end;
procedure TTapeFile.ChgPos;
var a:PDialog;
    R:TRect;
    c,rb:word;
    ll:PMovableOnlyOnce;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    datoteka:file;
    cc:Boolean;
    prethodni,naredni,iskljuceni,vrijednost:longint;

  procedure we;
  begin
    cc:=fclose(datoteka);
    readblocks(tapename);
  end;

begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   if noofblocks=1 then begin
     displayerror('Tape #'+tapename+'# contains only one block',0);
     exit
   end;
   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Change position'));
   a^.helpctx:=hcchgpos;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PMovableOnlyOnce,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then begin
      rb:=ll^.focused+1;
      assign(datoteka,tapename);
      if not(fresetu(datoteka,1)) then begin readblocks(tapename);exit end;
      iskljuceni:=blocks^[rb].pocetak;
      if not(fseek(datoteka,blocks^[rb].pocetak)) then begin
        we;exit
      end;
      if not(fblockread(datoteka,prethodni,4)) then begin we;exit end;
      if not(fblockread(datoteka,naredni,4)) then begin we;exit end;
      if not(fseek(datoteka,naredni)) then begin we;exit end;
      if not(fblockwrite(datoteka,prethodni,4))then begin we;exit end;
      if not(fseek(datoteka,prethodni+4)) then begin we;exit end;
      if prethodni=0 then if not(fseek(datoteka,0)) then begin
        we;exit
      end;
      if not(fblockwrite(datoteka,naredni,4)) then begin we;exit end;
      if rb=1 then begin
        if not(fseek(datoteka,0)) then begin we;exit end;
        if not(fblockwrite(datoteka,iskljuceni,4)) then begin
          we;exit
        end;
        if not(fseek(datoteka,iskljuceni))then begin we;exit end;
        vrijednost:=0;
        if not(fblockwrite(datoteka,vrijednost,4))then begin
          we;exit
        end;
        if not(fblockwrite(datoteka,
          blocks^[rb+1].pocetak,4)) then begin we;exit end;

        if not(fseek(datoteka,blocks^[rb+1].pocetak)) then begin
          we;exit
        end;
        if not(fblockwrite(datoteka,iskljuceni,4)) then begin
          we;exit
        end
      end;
      if rb=noofblocks then begin
        if not(fseek(datoteka,4)) then begin we;exit end;
        if not(fblockwrite(datoteka,iskljuceni,4))then begin
          we;exit
        end;
        if not(fseek(datoteka,iskljuceni))then begin we;exit end;
        if not(fblockwrite(datoteka,
          blocks^[rb-1].pocetak,4)) then begin we;exit end;
        vrijednost:=4;
        if not(fblockwrite(datoteka,vrijednost,4))then begin
          we;exit
        end;
        if not(fseek(datoteka,blocks^[rb-1].pocetak+4)) then begin
          we;exit
        end;
        if not(fblockwrite(datoteka,iskljuceni,4))then begin
          we;exit
        end;
      end;
      if (rb<>1) and (rb<>noofblocks) then begin
        if not(fseek(datoteka,blocks^[rb-1].pocetak+4))then begin
          we;exit
        end;
        if not(fblockwrite(datoteka,iskljuceni,4))then begin
          we;exit
        end;
        if not(fseek(datoteka,blocks^[rb+1].pocetak))then begin
          we;exit
        end;
        if not(fblockwrite(datoteka,iskljuceni,4)) then begin
          we;exit
        end;
        if not(fseek(datoteka,iskljuceni))then begin we;exit end;
        if not(fblockwrite(datoteka,
          blocks^[rb-1].pocetak,4)) then begin we;exit end;
        if not(fblockwrite(datoteka,
          blocks^[rb+1].pocetak,4))then begin we;exit end;
      end;

   end;
   readblocks(tapename);
end;

{
 ******************************
   Print to screen
 ******************************
 }
 const linija=72;

 type
  PInterior = ^TInterior;
  TInterior = object(TScroller)
    blocks:blockarrayp;
    granica:word;
    tapename:string;
    constructor Init(var Bounds: TRect; AHScrollBar,
      AVScrollBar: PScrollBar; pgranica:integer; pblocks:blockarrayp;
      tn:string);
    procedure Draw; virtual;
  end;

  PPrintWindow = ^TPrintWindow;
  TPrintWindow = object(TDialog)
    blocks:blockarrayp;
    granica:word;
    tapename:string;
    constructor Init(Bounds: TRect; WinTitle: String; WindowNo: Word;
       noofblocks:integer; pblocks:blockarrayp;tn:string);
    procedure MakeInterior(Bounds: TRect);
  end;

constructor TInterior.Init(var Bounds: TRect; AHScrollBar,
  AVScrollBar: PScrollBar;pgranica:integer; pblocks:blockarrayp;
  tn:string);
begin
  TScroller.Init(Bounds, AHScrollBar, AVScrollBar);
  GrowMode := gfGrowHiX + gfGrowHiY;
  Options := Options or ofFramed;
  granica:=pgranica;
  blocks:=pblocks;
  tapename:=tn;
end;

procedure TInterior.Draw;
var
  Color: Byte;
  I, Y, rowtoprint,k,dist,rb: Integer;
  B: TDrawBuffer;
  a:String;
begin
  TScroller.Draw;
  Color := GetColor(1);
  for Y := 0 to Size.Y - 1 do
  begin
    rowtoprint:=VSCrollBar^.Value+y;
    case rowtoprint mod (linija+1) of
      0: begin a:=''; for i:=1 to 255 do a:=a+'=' end;
      2,linija,linija-1,linija-2: a:='';
      1: begin
         k:=(rowtoprint div (linija+1))+1;
         str (k,a);
         a:='Listing of tape '+tapename+'  Page '+a;
       end;
      3..linija-3: begin
        a:='';
        k:=rowtoprint div (linija+1);
        dist:=rowtoprint mod (linija+1)-2;
        rb:=k*(linija-5)*2+dist;
        if rb<=granica then a:=copy(a+blocks^[rb].naziv,1,37);
        if rb+linija-5<=granica then a:=a+
          copy(blocks^[rb+linija-5].naziv,1,37);
      end;
    end;

    writestr(1,y,copy(a,delta.x+1,size.x),1);
  end;
end;


constructor TPrintWindow.Init(Bounds: TRect; WinTitle: String;
  WindowNo: Word;noofblocks:integer;pblocks:BlockArrayp;tn:string);
var
  S: string[3];
begin
  Str(WindowNo, S);
  TWindow.Init(Bounds, WinTitle , windowNo);
  helpctx:=hcprscr;
  flags:=15;
  granica:=noofblocks;
  blocks:=pblocks;
  tapename:=tn;
  MakeInterior(Bounds);
end;

procedure TPrintWindow.MakeInterior(Bounds: TRect);
var
  HScrollBar, VScrollBar: PScrollBar;
  Interior: PInterior;
  R: TRect;
begin
  VScrollBar := StandardScrollBar(sbVertical + sbHandleKeyboard);
  HScrollBar := StandardScrollBar(sbHorizontal + sbHandleKeyboard);
  GetExtent(Bounds);
  Bounds.Grow(-1,-1);
  Interior := New(PInterior, Init(Bounds, HScrollBar, VScrollBar,
    granica,blocks,tapename));
  Insert(Interior);
  Interior^.Setlimit(80,trunc(1+granica/(2*linija+2))*(linija+1));
end;


procedure TTapeFile.PrintScreen;
var R:TRect;
  window:PPrintWindow;
  c:integer;
begin
  if not(allowed) then exit;
  R.Assign(1,1, 79, 20);
  Window := New(PPrintWindow, Init(R, 'Screen Preview', 2,
    noofblocks,blocks,tapename));
  c:=DeskTop^.execview(Window);
  hourglascur;
  dispose(Window,done);
end;

{
  *****************************
   Print to printer
  *****************************
}

type

TButtonsForSize=object(TRadioButtons)
  vezstr:PNumInputline;
  procedure handleevent(var event:Tevent); virtual;
  constructor Init(var bounds:TRect; unosni:PNumInputLine;AStrings:PSitem);
end;
PButtonsForSize=^TButtonsForSize;

procedure TButtonsForSize.HandleEvent(var event:Tevent);
var n:integer;
  num:longint;
begin
  TradioButtons.HandleEvent(event);
  getdata(n);
  case n of
    0: begin
      num:=66;
      vezstr^.setdata(num);
    end;
    1: begin
      num:=72;
      vezstr^.setdata(num);
    end;
  end;
  owner^.draw;
end;

constructor TButtonsForSize.Init(var bounds:TRect;
   unosni:PNumInputLine;AStrings:PSitem);
begin
  TRadioButtons.Init(Bounds,AStrings);
  vezstr:=unosni;
end;


procedure ttapefile.printtoprinterorfile(mode:integer);
{**************
  0 printer
  1 datoteka
 **************
 }
var r:Trect;
  a:PDialog;
  Bruce:Pointer;
  c:word;
  pagesize:PNumInputLine;
  Buttons:PButtonsForSize;
  filename,strana:string;
  izlaz:text;
  cc:Boolean;
  linija,num:longint;
  page,blokova,i:integer;

  Printerport:PRadioButtons;
  daljinar:PProcenter;

  procedure we;
  begin
    readblocks(tapename);
  end;
  procedure we1;
  begin
    we;
    desktop^.delete(daljinar);
    dispose(daljinar,done);
  end;
begin
  if not(allowed) then exit;
  r.assign(4,2,60,20);
  a:=new(PDialog,Init(r,'Print'));
  a^.helpctx:=hcprpri;
  if mode=1 then a^.helpctx:=hcprfile;

  r.assign(18,3,24,4);
  Pagesize:=new(PNumInputLine,Init(r,2,10,99));
  a^.insert(PageSize);
  r.assign(4,3,16,4);
  Bruce:=new(PLabel,Init(r,'~P~aper size: ',pagesize));
  a^.insert(Bruce);

  num:=72;
  pagesize^.setdata(num);
  R.Assign(4,6,30,9);
  Buttons := New(PButtonsForSize, Init(R,Pagesize,
    NewSItem('~6~6 lines paper',
    NewSItem('~7~2 lines paper',
    NewSItem('~U~ser defined size',
    nil)))
  ));
  a^.Insert(Buttons);
  i:=1;
  buttons^.setdata(i);
  if mode=0 then begin
    R.assign(4,12,30,16);
    printerport:=new(PradioButtons,Init(R,
      NewSItem('~L~PT 1',
      NewSItem('LP~T~ 2',
      NewSItem('~C~OM 1',
      NewSItem('C~O~M 2',nil))))));
    a^.insert(printerport);
    r.assign(4,10,20,11);
    Bruce:=new(PLabel,Init(r,'~D~estination: ',printerport));
    a^.insert(Bruce);

  end;
  R.Assign(35,6,45,8);
  Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
  a^.insert(Bruce);

  R.Assign(35,2,45,4);
  Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
  a^.insert(Bruce);
  c:=desktop^.execview(a);
  hourglascur;
  if c<>cmcancel then begin
    if mode=0 then begin
      printerport^.getdata(c);
      case c of
        0: filename:='LPT1';
        1: filename:='LPT2';
        2: filename:='COM1';
        3: filename:='COM2';
      end;
    end;
    if mode=1 then begin
      filename:=outputfile('PRTLIST.TXT',tapename);
      if filename='' then begin
        dispose(a,done);
        we;
        exit
      end;
    end;
    Buttons^.getdata(c);
    Pagesize^.getdata(linija);
    case c of
      0: linija:=66;
      1: linija:=72;
    end;
    assign(izlaz,filename);
    if not(frewritet(izlaz)) then exit;
    page:=1;
    blokova:=0;
    i:=1;
    dispose (a,done);
    new(daljinar,Init(16,10,52,1,noofblocks));
    desktop^.insert(daljinar);
    repeat
      str(page:2,strana);
      if not(fwriteln
       (izlaz,'   Listing of tape '+tapename+' Page '+strana)) then begin
        we1;
        exit
      end;
      if not(fwriteln(izlaz,'')) then begin we;exit end;
      repeat
        if i+linija-5<=noofblocks then
          cc:=fwriteln(izlaz,'   '+copy(blocks^[i].naziv,1,37)+'  '
           +copy(blocks^[i+linija-5].naziv,1,37))
         else cc:=fwriteln(izlaz,'   '+copy(blocks^[i].naziv,1,37));
        if not(cc) then begin we1;exit end;
        inc(i);
        inc(blokova);
      until (blokova=linija-5) or (i>noofblocks);
      inc(page);
      blokova:=0;
      if not(fwriteln(izlaz,'')) then begin we1;exit end;
      if not(fwriteln(izlaz,'')) then begin we1;exit end;
      i:=i+linija-5;
      if not(fwriteln (izlaz,'')) then begin we1;exit end;
      daljinar^.update(i,blocks^[i].naziv);
    until i>noofblocks;
    close (izlaz);
    desktop^.delete(daljinar);
    dispose(daljinar,done);

   end
    else dispose(a,done);
end;

procedure TTapefile.printtoprinter;
begin
  printtoprinterorfile(0)
end;
procedure TTapefile.printtofile;
begin
  printtoprinterorfile(1)
end;


{ *****************************
   Hex editor
  *****************************
}


procedure TTapefile.edit;
var a: PDialog;
  R:TRect;
  skrolnica:PScrollBar;
  ll:PSImpleBlocksView;
  Bruce:Pointer;
  c,rb,j:word;
  datoteka:file;
  paritet,bajt:Byte;
type konvtip=array[0..65520] of byte;
  var cc:Boolean;

    procedure we;
    var cc:Boolean;
    begin
      cc:=fclose(datoteka);
      readblocks(tapename);
    end;

begin
  if not(allowed) then exit;
  if noofblocks=0 then begin
    displayerror('Tape #'+tapename+'#is empty',0);
    exit;
  end;

  R.assign(10,2,70,22);
  a:=new(PDialog,Init(R,'Edit'));
  a^.helpctx:=hcedit;
  R.assign(41,1,42,18);
  skrolnica:=new(pscrollbar,Init(r));
  a^.insert(skrolnica);
  R.Assign(45,2,55,4);
  Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
  a^.insert(Bruce);
  R.Assign(45,8,55,10);
  Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
  a^.insert(Bruce);
  R.assign(1,1,40,18);
  ll:=new(PSimpleBlocksView,Init(R,1,skrolnica,blocks));
  ll^.setrange(noofblocks);
  ll^.focusitem(0);
  a^.insert(ll);
  c:=desktop^.execview(a);
  hourglascur;
  dispose(a,done);
  if c=cmOK then begin
    rb:=ll^.focused+1;
    if not(load(waratape,tapename,blocks^[rb].pocetak,buffer^)) then begin
      we;exit
    end;

    (* Specijalno ako je duzina 65535 *)
    if blocks^[rb].duzina1>=65534 then begin
      displayerror('Editor can not modify #compressed files and samples',0);
      readblocks(tapename);
      exit
    end;
    if hexeditor(buffer,blocks^[rb].duzina1,0)<>cmCancel then begin
      assign(datoteka,tapename);
      if not(fresetu(datoteka,1))then begin readblocks(tapename);exit end;
      if waratape then begin
        if not(fseek(datoteka,blocks^[rb].pocetak+11)) then begin
          we;exit
        end;
      end
      else begin
        if not(fseek(datoteka,blocks^[rb].pocetak+3)) then begin
          we;exit
        end;
      end;

      (* Specijalno ako je duzina 65535 *)
      if not(fblockwrite(datoteka,
        buffer^,blocks^[rb].duzina1)) then begin we;exit end;
      if not(waratape) then begin
        paritet:=blocks^[rb].flag;
        for j:=0 to blocks^[rb].duzina1-1 do begin
          bajt:=konvtip(buffer^)[j];
          paritet:=paritet xor bajt;
        end;
      if not(fblockwrite(datoteka,
        paritet,1)) then begin we;exit end;

      end;
      cc:=fclose(datoteka);
      readblocks(tapename);
    end  {if hexeditor}
  end;
  readblocks(tapename);
end;

{
  *********************************
  CopyToNew
  *********************************
}

procedure TTapefile.copytoNew;
type konvtip=array[0..65520] of byte;
var
  sourcetape,targettape:string;
  ll:PTagableBlocksLister;
  ld:PBlocksViewWithLast;
  R:Trect;
  a:PDialog;
  c,rb:word;
  bajt,paritet,j:word;
  skrolnica:PScrollBar;
  Bruce:Pointer;
  noofbldest,poslatibajtova:word;
  datoteka:file;
  pozicija:Longint;
  i:integer;
  daljinar:PProcenter;
  srcwaratape,dstwaratape:Boolean;
    procedure we;
    var cc:Boolean;
    begin
      cc:=fclose(datoteka);
      waratape:=srcwaratape;
      readblocks(tapename);
    end;

    procedure we1;
    begin
      we;
      desktop^.delete(daljinar);
      dispose(daljinar,done)
    end;
begin
   if not(allowed) then exit;
   sourcetape:=tapename;
   srcwaratape:=waratape;
   if not(openfile) then begin
     tapename:=sourcetape;
     readblocks(tapename);
     exit
   end;
   dstwaratape:=waratape;
   targettape:=tapename;
   readblocks(tapename);
   move(blocks^,buffer^,sizeof(blocks^));
   noofbldest:=noofblocks;
   tapename:=sourcetape;
   waratape:=srcwaratape;
   readblocks(tapename);
   waratape:=dstwaratape;
   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Copy To New'));
   a^.helpctx:=hccopynew;
   R.assign(41,10,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.assign(1,10,40,18);
   ld:=new(PBlocksViewWithLast,Init(R,1,skrolnica,buffer));
   ld^.setrange(noofbldest);
   ld^.focusitem(0);
   if not(waratape) then begin ld^.options:=4; ld^.focusitem(noofbldest) end;
   a^.insert(ld);
   R.assign(1,18,40,19);
   bruce:=new(Plabel,init(r,'~D~estination :'+targettape,ld));
   a^.insert(bruce);

   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,8,55,10);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(41,1,42,8);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.assign(1,1,40,8);
   ll:=new(PTagableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   R.assign(1,8,40,9);
   bruce:=new(Plabel,init(r,'~S~ource :'+sourcetape,ll));
   a^.insert(bruce);

   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then begin
     rb:=ld^.focused+1;
     if rb<>noofbldest+1 then pozicija:=blockarrayp(buffer)^[rb].pocetak
      else
        if (noofbldest=0) or (not(dstwaratape)) then pozicija:=4
         else begin
           assign(datoteka,targettape);
           if not(fresetu(datoteka,1)) then begin
             readblocks(tapename);exit;
           end;
           if not(fseek(datoteka,blockarrayp(buffer)^[noofbldest].pocetak))
            then begin we;exit end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fclose(datoteka)) then begin
             readblocks(tapename);
             exit
           end;
         end; {else /noofbldest<>0/}
     new(daljinar,Init(16,10,52,1,noofblocks));
     desktop^.insert(daljinar);
     for i:=1 to noofblocks do begin
       daljinar^.update(i,blocks^[i].naziv);
       if blocks^[i].naziv[5]<>' ' then begin
         if not(load(srcwaratape,sourcetape,blocks^[i].pocetak,buffer^)) then begin
           we1;exit
         end;
         poslatibajtova:=blocks^[i].duzina1;
         if srcwaratape and not(dstwaratape) then begin
            poslatibajtova:=blocks^[i].duzina1;
            if poslatibajtova=65534 then begin
              DisplayError('Sample not allowed',0);
              we1; exit
            end;
            if blocks^[i].duzina1=65535 then begin
              decompress(ptr(seg(buffer^),ofs(buffer^)+6)
              ,izlbuffer,blocks^[i].duzdec,
               blocks^[i].duzcomp,blocks^[i].duzsign);
               poslatibajtova:=blocks^[i].duzdec-1;
            end;
            paritet:=blocks^[i].flag;
            for j:=0 to poslatibajtova-1 do begin
              if blocks^[i].duzina1=65535 then bajt:=konvtip(izlbuffer^)[j]
                else bajt:=konvtip(buffer^)[j];
              paritet:=paritet xor bajt;
            end;

         end;
         if not(save(waratape,targettape,poslatibajtova,
           pozicija,blocks^[i].flag,buffer^)) then begin
             we1;exit
         end;
       end;
     end;
     desktop^.delete(daljinar);
     dispose(daljinar,done);

   end;
   waratape:=srcwaratape;
   readblocks(tapename);
end;

{************************
   Decompress
 ************************
}


procedure TTapeFile.unpack;

type
  konvtip=array[1..4] of word;

var
  i:integer;
  duzdec,duzcomp,duzsign:word;
  cc:Boolean;
  d,n,e:string;
  datoteka,datoteka2:file;
  daljinar:PProcenter;
  r:TRect;
  event:Tevent;

  procedure we;
  begin
    readblocks(tapename)
  end;
  procedure we1;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    readblocks(tapename)
  end;

begin
  if not(allowed) then exit;
  if noofblocks=0 then
    displayerror('Tape# '+tapename+'# is empty',0)
   else begin
    if messagebox(^C'Decompress '+#13+chr(3)+tapename+'?',
      nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK  then exit;
     hourglascur;

     fsplit(tapename,d,n,e);
     assign(datoteka,tapename);
     if fileexists(n+'.BAK') then begin
       assign (datoteka2,n+'.BAK');
       if not(ferase(datoteka2)) then begin we;exit end;
     end;
     if not(frename(datoteka,n+'.BAK')) then begin
       we;
       exit
     end;

     assign(datoteka2,tapename);
     if not(fresetu(datoteka,1)) then begin we;exit end;
     if not(frewriteu(datoteka2,1)) then begin
       cc:=ferase(datoteka2);
       cc:=frename(datoteka,n+'.TAP');
       cc:=fclose(datoteka);
       we;
       exit;
     end;
     if not(fblockwrite(datoteka2,prazna,12)) then begin
       cc:=fclose(datoteka);
       cc:=fclose(datoteka2);
       we;
       exit
     end;
     if not(fclose(datoteka)) then begin
       cc:=fclose(datoteka2);
       we;
       exit
     end;
     if not(fclose(datoteka2)) then begin we;exit end;

     new(daljinar,Init(16,10,52,1,noofblocks));
     desktop^.insert(daljinar);

     for i:=1 to noofblocks do begin
       daljinar^.update(i,blocks^[i].naziv);
       if blocks^[i].duzina1<65534 then begin
         if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
           we1;exit
         end;
         if not(save(waratape,tapename,blocks^[i].duzina1,
           4,blocks^[i].flag,buffer^)) then begin we1;exit end;
       end
        else begin
          if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
            we1;exit
          end;
          if blocks^[i].duzina1=65534 then begin
            if blocks^[i].duzcomp=blocks^[i].duzdec then begin
              if not(save(waratape,tapename,65534,
               4,blocks^[i].flag,buffer^)) then begin we1;exit end;
            end
             else begin
               decompress(ptr(seg(buffer^),ofs(buffer^)+6)
                 ,ptr(seg(izlbuffer^),ofs(izlbuffer^)+6),blocks^[i].duzdec,
                 blocks^[i].duzcomp,blocks^[i].duzsign);
               konvtip(izlbuffer^)[1]:=blocks^[i].duzdec-1;
               konvtip(izlbuffer^)[2]:=blocks^[i].duzdec-1;
               konvtip(izlbuffer^)[3]:=0;
             end;
             if not(save(waratape,tapename,65534,
              4,blocks^[i].flag,izlbuffer^)) then begin we1;exit end;
          end
          else begin
            decompress(ptr(seg(buffer^),ofs(buffer^)+6)
              ,izlbuffer,blocks^[i].duzdec,
              blocks^[i].duzcomp,blocks^[i].duzsign);
            if not(save(waratape,tapename,blocks^[i].duzdec-1,
             4,blocks^[i].flag,izlbuffer^)) then begin we1;exit end;
          end
        end;
        readblocks(tapename);
     end;
   desktop^.delete(daljinar);
   dispose(daljinar,done);
   end;  {else /noofblocks <>0/}
end; {unpack}
{
 *********************************
   Compress selected
 *********************************
}

procedure TTapefile.pack(izbor:integer);
{
  1: all blocks
  2: linearize
  3: selected
}


type
  konvtip=array[1..4] of word;
var i,j:integer;
  duzdec,duzcomp,duzsign,ff:word;
  cc:Boolean;
  d,n,e:string;
  datoteka,datoteka2:file;
  daljinar,kolikokomp:PProcenter;

  procedure we;
  begin
    readblocks(tapename);
    desktop^.helpctx:=hcNoContext;
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    desktop^.delete(kolikokomp);
    dispose(kolikokomp,done);

  end;

begin
  if (izbor=1) or (izbor=3) then
    if memavail<111000 then begin
      displayerror('Compression not allowed# because free RAM is#'+
       'less than 110000 bytes.',0);
      exit
    end;
  new(kolikokomp,Init(16,12,52,1,noofblocks));
  desktop^.insert(kolikokomp);

  new(daljinar,Init(12,7,52,1,noofblocks));
  desktop^.insert(daljinar);

  case izbor of
    1: desktop^.helpctx:=hccompall;
    2: desktop^.helpctx:=hclinonly;
    3: desktop^.helpctx:=hccompsel;
  end;

  if izbor=1 then begin
    for i:=1 to noofblocks do blocks^[i].naziv[5]:='';
    izbor:=3;
  end;
  if noofblocks=0 then
    displayerror('Tape# '+tapename+'# is empty',0)
   else begin
     hourglascur;
     fsplit(tapename,d,n,e);
     assign(datoteka,tapename);
     if fileexists(n+'.BAK') then begin
       assign (datoteka2,n+'.BAK');
       if not(ferase(datoteka2)) then begin
         readblocks(tapename);
         exit
       end
     end;
     if not(frename(datoteka,n+'.BAK')) then begin
       readblocks(tapename);
       exit
     end;
     assign(datoteka2,tapename);
     if not(fresetu(datoteka,1)) then begin we;exit end;
     if not(frewriteu(datoteka2,1)) then begin;
       cc:=ferase(datoteka2);
       cc:=frename(datoteka,n+'.TAP');
       cc:=fclose(datoteka);
       we;
       exit;
     end;
     if not(fblockwrite(datoteka2,prazna,12)) then begin
       cc:=fclose(datoteka2);
       cc:=fclose(datoteka);
       we;
       exit
     end;
     if not(fclose(datoteka)) then begin
       cc:=fclose(datoteka2);
       we;
       exit
     end;
     if not(fclose(datoteka2)) then begin we;exit end;
     for i:=1 to noofblocks do begin
       daljinar^.update(i,blocks^[i].naziv);
       if daljinar^.wantcancel then begin
       if messagebox(^C'Cancel compressing? ',
        nil,mfYesButton+mfNoButton+mfconfirmation)=cmyes  then
           for j:=i to noofblocks do blocks^[j].naziv[5]:=' ';
       end;
       if ((izbor=3) and (blocks^[i].naziv[5]=' ')) or
         (izbor=2) or (blocks^[i].duzina1=17)
         or (blocks^[i].duzina1=65535) then begin
           if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
             we;exit
           end;
           if not(save(waratape,tapename,blocks^[i].duzina1,
             4,blocks^[i].flag,buffer^)) then begin
               we;exit
           end
       end
        else begin
          if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
            we;exit
          end;
          duzdec:=blocks^[i].duzina1;
          if duzdec=65534 then duzdec:=konvtip(buffer^)[1];
          if (blocks^[i].duzina1=65534) then begin
            if (konvtip(buffer^)[1]<>konvtip(buffer^)[2]) then begin
              duzcomp:=konvtip(buffer^)[2];
              duzsign:=konvtip(buffer^)[3];
              move(buffer^,izlbuffer^,duzcomp+6);
            end
             else begin
                compress(ptr(seg(buffer^),ofs(buffer^)+6),izlbuffer,duzdec+1,
                 duzcomp,duzsign,kolikokomp);
             end;
          end
           else compress(buffer,izlbuffer,duzdec+1,
             duzcomp,duzsign,kolikokomp);
          if (duzcomp+6<blocks^[i].duzina1) or
            (blocks^[i].duzina1=65534) then begin
            konvtip(buffer^)[1]:=duzdec;
            konvtip(buffer^)[2]:=duzcomp;
            konvtip(buffer^)[3]:=duzsign;
            move(izlbuffer^,konvtip(buffer^)[4],duzcomp);
            if blocks^[i].duzina1=65534 then ff:=65534 else ff:=65535;
            if not(save(waratape,tapename,ff,4,
              blocks^[i].flag,buffer^)) then begin we;exit end;
          end
           else begin
             if not(load(waratape,n+'.BAK',blocks^[i].pocetak,buffer^)) then begin
               we;exit
             end;
             if not(save(waratape,tapename,blocks^[i].duzina1,
               4,blocks^[i].flag,buffer^)) then begin we;exit end;
           end;
        end; {else /duzina nije 17 ili compr./}
     end;
     readblocks(tapename);
   end; {else noofblocks <> 0}
  desktop^.delete(daljinar);
  dispose(daljinar,done);
  desktop^.delete(kolikokomp);
  dispose(kolikokomp,done);

end; {pack}

{ ************************
   Compress selected
  ************************
}
procedure TTapeFile.CompressSelected;
var a:PDialog;
    R:TRect;
    c:word;
    ll:PTagableBlocksLister;
    Bruce:Pointer;
    skrolnica:pscrollbar;
begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;

   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Compress'));
   a^.helpctx:=hccompsel;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PTagableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then pack(3);
   readblocks(tapename);
end;
{
  ********************************
  Compress all
  ********************************
}
procedure TTapefile.compressAll;
begin
  if not(allowed) then exit;
  if messagebox(^C'Compress '+#13+chr(3)+tapename+'?',
    nil,mfOKButton+mfCancelButton+mfconfirmation)<>cmOK  then exit;
  pack(1);
end;

{
  ********************************
  Linearize all
  ********************************
}
procedure TTapefile.Linearize;
begin
  if not(allowed) then exit;
  if messagebox(^C'Compress '+#13+chr(3)+tapename+'?',
    nil,mfOKButton+mfCancelButton)<>cmOK  then exit;
  pack(2);
end;
{
  ********************************
  Efficiency
  ********************************
}

procedure TTapefile.efficiency;
var
  nekomprim,komprim:longint;
  i:integer;
  dumstr,dumstr1:string;
  datoteka:file;
  dummy:integer;
begin
  if not(allowed) then exit;
  komprim:=12;
  nekomprim:=12;
  for i:=1 to noofblocks do
  with blocks^[i] do begin
    if duzina1<65534 then begin
      nekomprim:=nekomprim+11+duzina1;
      komprim:=komprim+11+duzina1;
    end
     else begin
       nekomprim:=nekomprim+11+duzdec+1;
       komprim:=komprim+17+duzcomp;
     end;
  end; {With}
  dumstr:='Original size   ';
  str(nekomprim:7,dumstr1);
  dumstr:=dumstr+dumstr1+#13+'Compressed size ';
  str(komprim:7,dumstr1);
  dumstr:=dumstr+dumstr1+#13+'Result in percent     ';
  str((komprim/nekomprim)*100:7:2,dumstr1);
  dumstr:=dumstr+dumstr1+#13+'Actual size     ';
  assign(datoteka,tapename);
  if not(fresetu(datoteka,1)) then exit;
  str(filesize(datoteka):7,dumstr1);
  if not(fclose(datoteka)) then exit;
  dumstr:=dumstr+dumstr1;

  dummy:=messagebox(dumstr,nil,mfOKButton+mfInformation);
end; {ratio}

{ ******************************************
     Send to spectrum
  ******************************************
}

procedure TTapefile.sendtospectrum;

type
  konvtip=array[0..65520] of byte;
var
  rb,dummy:integer;
  datoteka:file of byte;
  i,j:word;
  poslatibajtova,bajtupet:word;
  paritet,bajt:byte;
  daljinar:PProcenter;
  st:string;
  a:PDialog;
  R:TRect;
  c:word;
  ll:PTagableBlocksLister;
  Bruce:Pointer;
  skrolnica:pscrollbar;
  timing:word;
  event1,event2:tevent;

  function wantcancel:boolean;
  begin
      if memw[$46:$c]-timing>18 then begin
        timing:=memw[$46:$c];
           if daljinar^.eventavail then wantcancel:=daljinar^.wantcancel
          else wantcancel:=false;
      end
        else wantcancel:=false;
  end;


  procedure we;
  begin
    dispose(daljinar,done);
    dummy:=modem_dtr(0);
    readblocks(tapename);
    dummy:=comm_remove;
    desktop^.helpctx:=hcNoContext
  end;  {we}


begin
  if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;


   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Send to Spectrum'));
   a^.helpctx:=hcsendtsp;
   R.assign(41,1,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,1,40,18);
   ll:=new(PTagableBlocksLister,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   new(daljinar,Init(16,12,52,1,noofblocks));
   desktop^.insert(daljinar);

   desktop^.helpctx:=hcsendtsp;
   if c=cmOK then begin
     for i:=1 to noofblocks do if blocks^[i].naziv[5]<>' ' then begin
       if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin
         we;exit
       end;
       daljinar^.update(i,blocks^[i].naziv);
       timing:=memw[$46:$c];
       dummy:=comm_install(dfltrs232);
       comm_setparam(baudrates[slbdrt]);
       dummy:=modem_dtr(0);

       poslatibajtova:=blocks^[i].duzina1;
       if blocks^[i].duzina1=65534 then begin
         DisplayError('Sample can not be send over RS232',0);
         we;exit
       end;

       if blocks^[i].duzina1=65535 then begin
         decompress(ptr(seg(buffer^),ofs(buffer^)+6)
         ,izlbuffer,blocks^[i].duzdec,
          blocks^[i].duzcomp,blocks^[i].duzsign);
          poslatibajtova:=blocks^[i].duzdec-1;
       end;
       bajtupet:=poslatibajtova-1;
       poslatibajtova:=poslatibajtova+3;

       bajt:=lo(poslatibajtova);
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       bajt:=255-bajt;
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       bajt:=hi(poslatibajtova);
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       bajt:=255-bajt;
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       bajt:=blocks^[i].flag;
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       bajt:=255-bajt;
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
         comm_put(bajt);
       paritet:=blocks^[i].flag;
       for j:=0 to bajtupet do begin
         if (j mod 256)=0 then begin
            str(j:6,st);
            daljinar^.writestr(1,4,st,4);
         end;
         if blocks^[i].duzina1=65535 then bajt:=konvtip(izlbuffer^)[j]
           else bajt:=konvtip(buffer^)[j];
         paritet:=paritet xor bajt;
         while modem_status<>51 do
             if wantcancel then begin
             we;
             exit
           end;
           comm_put(bajt);
       end;
       while modem_status<>51 do
           if wantcancel then begin
           we;
           exit
         end;
       bajt:=paritet;
       comm_put(bajt);

     end; {if blocks^[i].naziv<>' '}
     readblocks(tapename);
   end; {c=cmOK }
we;
end; {sendtospectrum}
{
 ********************************
 ReceiveFromSpectrum
 ********************************
}

procedure TTapefile.receivefromspectrum;
type
  konvtip=array[0..65520] of byte;
var
  znak,znak1,znak2,znak3,znak4,znak5,znak6,dummy,dreg:integer;
  ceksum,fleg:byte;
  sizeok:Boolean;
  de,duzina,ix,oldix,oldde:word;
  regs:registers;
  kolikopren:pprocenter;
  st:string;
  timing:word;

  procedure we;
  begin
    dispose(kolikopren,done);
    dummy:=modem_dtr(0);
    readblocks(tapename);
    dummy:=comm_remove;
    desktop^.helpctx:=hcNoContext;
  end;  {we}

  function wantcancel:boolean;
  begin
   if memw[$46:$c]-timing>18 then begin
    timing:=memw[$46:$c];
    dummy:=modem_dtr(0);
    if kolikopren^.eventavail then wantcancel:=kolikopren^.wantcancel
      else wantcancel:=false;
    dummy:=modem_dtr(1);
   end
    else wantcancel:=false;
  end;


begin  {receivefromspectrum}
  if not(allowed) then exit;
  desktop^.helpctx:=hcRecSp;

  new(kolikopren,Init(12,7,52,1,noofblocks));
  desktop^.insert(kolikopren);

  repeat
    dummy:=comm_install(dfltrs232);
    comm_setparam(baudrates[slbdrt]);
    dummy:=modem_dtr(1);
    timing:=memw[$46:$c];
    repeat
      if wantcancel then begin
        we;
        exit
      end;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak1:=comm_get;
      until znak1<>-1;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak2:=comm_get;
      until znak2<>-1;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak3:=comm_get;
      until znak3<>-1;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak4:=comm_get;
      until znak4<>-1;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak5:=comm_get;
      until znak5<>-1;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak6:=comm_get;
      until znak6<>-1;
      if (znak1=255-znak2)and(znak3=255-znak4)and(znak5=255-znak6) then begin
        sizeok:=true;
        dummy:=modem_dtr(0);
        kolikopren^.update(1,'Signature OK');
        dummy:=modem_dtr(1);
      end
       else begin
         sizeok:=false;
         displayerror('Bad signature. Trasfer aborted.',0);
         we;
         exit;
       end;
    until sizeok;
    de:=lo(znak1)+256*lo(znak3);
    duzina:=de;
    fleg:=znak5;
    kolikopren^.scrollbar^.setrange(0,de div 2);
    kolikopren^.scrollbar^.setvalue(1);

    ix:=0;
    repeat
      ceksum:=0;
      oldix:=ix;
      oldde:=de;
      repeat
        repeat
          if wantcancel then begin
            we;
            exit
          end;
          znak:=comm_get;
        until znak<>-1;
        konvtip(buffer^)[ix]:=znak;
        ceksum:=lo(ceksum+znak);
        inc (ix);
        dec (de);
      until lo(de)=0;
      repeat
        if wantcancel then begin
          we;
          exit
        end;
        znak:=comm_get;
      until znak<>-1;
      if znak=ceksum then begin
         dummy:=modem_dtr(0);
         str(ix,st);
         kolikopren^.update(ix div 2,st);
         dummy:=modem_dtr(1);
      end
       else begin
         dummy:=modem_dtr(0);
         displayerror('Error in block transfer.# Transfer aborted',0);
         we;
         exit;
       end
    until de=0;
    dummy:=modem_dtr(0);
    kolikopren^.update(ix div 2,'Saving ......');
    if not(save(waratape,tapename,duzina,4,fleg,buffer^)) then begin
      we;exit
    end
  until false;
end; {receivefromspectrum}

{
 ********************************
       Configure RS 232
 ********************************
}

procedure TTapefile.cfgrs232;
var izbor:integer;
  r:TRect;
  a:PDialog;
  comport,baudrate:PRadiobuttons;
  c:word;
  Bruce:Pointer;
begin
  r.assign(4,2,60,20);
  a:=new(PDialog,Init(r,'Configure'));
  a^.helpctx:=hccfgrs232;
  r.assign(4,3,20,5);
  comport:=new(PradioButtons,Init(R,
    NewSItem('COM ~1~',
    NewSItem('COM ~2~',nil))));
  a^.insert(comport);
  r.assign(30,3,50,11);
  baudrate:=new(PradioButtons,Init(R,
    NewSItem('110 bd',
    NewSItem('150 bd',
    NewSItem('300 bd',
    NewSItem('600 bd',
    NewSItem('1200 bd',
    NewSItem('2400 bd',
    NewSItem('4800 bd',
    NewSItem('9600 bd',nil))))))))));
  a^.insert(baudrate);
  R.Assign(5,13,20,15);
  Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
  a^.insert(Bruce);
  R.Assign(30,13,45,15);
  Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
  a^.insert(Bruce);

  c:=slbdrt-1;
  baudrate^.setdata(c);
  comport^.setdata(dfltrs232);
  c:=desktop^.execview(a);
  hourglascur;
  if c=cmOK then begin
    dfltrs232:=comport^.value;
    slbdrt:=baudrate^.value+1;
  end;
  dispose(a,done);
end;


{
******************************
  Convert
******************************
}

function TTapeFile.GeneralConvert(kontekst:integer): word;
var a:PDialog;
    R:TRect;
    c,rb:word;
    ll:PSimpleBlocksView;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    prethodni,naredni:longint;
    cc:Boolean;
begin
   generalconvert:=0;
   R.assign(10,2,70,22);
   a:=new(PDialog,Init(R,'Convert'));
   a^.helpctx:=kontekst;
   R.assign(41,2,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,2,55,4);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,6,55,8);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,2,40,18);
   ll:=new(PSimpleBlocksView,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   a^.insert(ll);
   r.assign(1,1,40,2);
   a^.insert(New(plabel,Init(r,'~H~eader of file to be converted',ll)));

   c:=desktop^.execview(a);
   hourglascur;
   if c=cmOK then generalconvert:=ll^.focused+1;
   dispose(a,done);
end;

function TTapefile.loadblock(rb:integer):word;
begin
  if not(load(waratape,tapename,blocks^[rb].pocetak,buffer^))then begin
    loadblock:=0;
    exit
  end;
  if blocks^[rb].duzina1=65534 then begin
    DisplayError ('Sample can not be converted',0);
    loadblock:=0;
    exit
  end;
  if blocks^[rb].duzina1=65535 then begin
    decompress(ptr(seg(buffer^),ofs(buffer^)+6)
     ,izlbuffer,blocks^[rb].duzdec,
    blocks^[rb].duzcomp,blocks^[rb].duzsign);
    move(izlbuffer^,buffer^,blocks^[rb].duzdec-1);
    loadblock:=blocks^[rb].duzdec-1;
  end
   else  loadblock:=blocks^[rb].duzina1;

end;

function TTapefile.isheader(rb:integer):Boolean;
begin
  isheader:=(blocks^[rb].duzina1=17)and(blocks^[rb].flag=0)
end;

function TTapefile.loadheader(rb:integer; var hedduzina,stvduzina:word)
  :Boolean;
var duzina:word;
type konvtip=array[1..65520] of byte;

begin
  duzina:=loadblock(rb);
  hedduzina:=konvtip(buffer^)[12]+256*konvtip(buffer^)[13];
  stvduzina:=blocks^[rb+1].duzina1;
  if stvduzina>=65534 then stvduzina:=blocks^[rb+1].duzdec;
  loadheader:=true;
end;





procedure TTapefile.basascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina,basonlyduz:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of BASIC program',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcbasasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>0)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  basonlyduz:=konvtip(buffer^)[16]+256*konvtip(buffer^)[17];
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  basicascii1(buffer^,basonlyduz);
end;

{ *********************************
    Gens to ASCII
  *********************************
}


procedure TTapefile.gensascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of GENS program',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcgensasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  gensascii1(buffer^,duzina);
end;

{ ****************************
  Tassword to ASCII
  verzija: 2
  verzija: 3
  ****************************
}


procedure TTapefile.tswrdascii(verzija:integer);
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of #Tassword text file',0)
  end;

begin
  if not(allowed) then exit;
  case verzija of
    2: rb:=generalconvert(hctt2asc);
    3: rb:=generalconvert(hctt3asc)
  end;
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  tswrdascii1(buffer^,duzina,verzija);
end;

{ *****************************************
   Abersoft Forth to ASCII
  *****************************************
}

procedure TTapefile.forthascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of #Abersoft forth RAM disk',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcforthasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  forthascii1(buffer^,duzina);
end;

{********************************
  The last word to ASCII
 ********************************
}

procedure TTapefile.tlwascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of The last word text',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hctlwasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  tlwascii1(buffer^,duzina);
end;
{ *************************
  Machine lightning to ASCII
  *************************
}


procedure TTapefile.mlascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of# Machine lightning source',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcmlasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  mlascii1(buffer^,duzina);
end;

{ ***************************
   Hisoft Pascal to ASCII
  ***************************
}

procedure TTapefile.pasascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of Pascal program',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcpasasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>stvduzina) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  pasascii1(buffer^,duzina);
end;

{
  ***********************
  Hisoft C to ASCII
  ***********************
}

procedure TTapefile.Cascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;
  saljijos: Boolean;
  filename:string;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of C program',0)
  end;

begin
  if not(allowed) then exit;
  saljijos:=false;
  rb:=generalconvert(hccasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>0) then begin
    doerr; exit
  end;
  filename:=outputfile('cascii.txt',tapename);
  if filename='' then exit;
  repeat
    inc(rb);
    if rb>noofblocks then begin doerr; exit end;
    if blocks^[rb].flag<>255 then begin doerr; exit end;
    duzina:=loadblock(rb);
    if duzina=0 then begin doerr; exit end;
    cascii1(buffer^,duzina,saljijos,filename);
  until not(saljijos);
end;

{ *************************
    Sinclair LOGO to ASCII
  *************************
}

procedure TTapefile.logoascii;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;
  saljijos: Boolean;
  filename:string;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of Sinclair Logo program',0)
  end;

begin
  if not(allowed) then exit;
  saljijos:=false;
  rb:=generalconvert(hclogoasc);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<128) then begin
    doerr; exit
  end;
  filename:=outputfile('logoasci.txt',tapename);
  if filename='' then exit;
  repeat
    inc(rb);
    if rb>noofblocks then begin doerr; exit end;
    if blocks^[rb].flag<>255 then begin doerr; exit end;
    duzina:=loadblock(rb);
    if duzina=0 then begin doerr; exit end;
    logoascii1(buffer^,duzina,saljijos,filename);
  until not(saljijos);
end;
{
 *****************************
 SCREEN$ to B/W TIFF
 *****************************
}

procedure TTapefile.screentifbw;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of SCREEN$ picture',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcscrbwtif);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>6912) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  screentiffbw1(buffer^,duzina);
end;


{**************************
  SCREEN$ to color TIFF
 **************************
}


procedure TTapefile.screentifcl;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of SCREEN$ picture',0)
  end;

begin
  if not(allowed) then exit;
  rb:=generalconvert(hcscrcoltif);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if (konvtip(buffer^)[1]<>3)or(hedduzina<>6912) then begin
    doerr; exit
  end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; exit end;
  screentiffcl1(buffer^,duzina);
end;

{ ************************************
  Op{ta rutina za konverziju ASCII
  formata u TAP file
  ************************************
}
function TTapeFile.GeneralAsciiToOther(var pozicija:longint;
  var filename:string;kontekst:integer):boolean;

var a:PDialog;
    fd:PFileDialog;
    R:TRect;
    c,rb:word;
    ll:PBlocksViewWithLast;
    Bruce:Pointer;
    skrolnica:pscrollbar;
    cc:Boolean;
    d,n,e,dummystr,blockname,ekstenzija:string;
    datoteka:file;
    i,dummy:integer;
    fs:Longint;
    fleg:byte;

   procedure we;
   begin
      cc:=fclose(datoteka);
      readblocks(tapename);
   end;

begin
   generalAsciitoother:=false;
   if not(allowed) then exit;
   fd:=new(PFileDIalog,Init('*.*','Select ASCII file to convert','~N~ame',
      fdOpenButton,100));
   fd^.helpctx:=kontekst;
   if desktop^.execview(fd)<> cmcancel then begin
     hourglascur;
     fd^.getfilename(blockname);
     filename:=blockname;
     blockname:=fexpand(blockname);
     if not(fileexists(blockname)) then begin dispose(fd,done); exit end;
   end
    else begin dispose(fd,done); exit end;
   dispose(fd,done);
   assign(datoteka,blockname);
   if not(fresetu(datoteka,1)) then begin we; exit  end;
   if filesize(datoteka)>65520 then begin
     displayerror('This file is too long for Spectrum',0);
     we; exit;
   end;
   if not(fclose(datoteka)) then begin we; exit end;
   R.assign(4,2,76,22);
   a:=new(PDialog,Init(R,'Convert'));
   a^.helpctx:=kontekst;
   R.assign(41,2,42,18);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.Assign(45,1,55,3);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(45,4,55,6);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);
   R.assign(1,2,40,18);
   ll:=new(PBlocksViewWithLast,Init(R,1,skrolnica,blocks));
   ll^.setrange(noofblocks);
   ll^.focusitem(0);
   if not(waratape) then begin
     ll^.focusitem(noofblocks);
     ll^.options:=4;
     A^.SelectNext(false);
   end;

   a^.insert(ll);
   r.assign(1,1,40,2);
   a^.insert(New(plabel,Init(r,'~S~tore before',ll)));

   c:=desktop^.execview(a);
   hourglascur;
   dispose(a,done);
   if c=cmOK then begin
     rb:=ll^.focused+1;
     if rb<>noofblocks+1 then pozicija:=blocks^[rb].pocetak
      else
        if noofblocks=0 then pozicija:=4
         else begin
           assign(datoteka,tapename);
           if not(fresetu(datoteka,1)) then begin
             readblocks(tapename);exit;
           end;
           if not(fseek(datoteka,blocks^[noofblocks].pocetak)) then begin
             we;exit
           end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fblockread(datoteka,pozicija,4)) then begin we;exit end;
           if not(fclose(datoteka)) then begin readblocks(tapename);exit end;
         end; {else /noofblocks<>0/}
   GeneralAsciiToOther:=true;
   end;
end;

{ *************************************
    ASCII to BASIC
  *************************************
}

procedure TTapeFile.asciiBasic;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascbas)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=0;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciibasic1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    zaglavlje[16]:=lo(duzina);
    zaglavlje[17]:=hi(duzina);

    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
  end;


{ *************************************
  ASCII to Pascal
  *************************************
}

procedure TTapeFile.asciiPas;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascpas)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciipas1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
  end;



{***********************************
  ASCII to GENS
 ***********************************
}

procedure TTapeFile.asciigens;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascgens)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciigens1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
  end;


{ ****************************
  ASCII to Machine lightning
  ****************************
}

procedure TTapeFile.asciiml;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascml)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciiml1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
  end;


{ **************************
   Ascii to C
  **************************
}


procedure TTapeFile.asciiC;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1,bfpos:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascc)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciic1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    bfpos:=1;
    repeat
      duzina:=konvtip(buffer^)[bfpos]+256*konvtip(buffer^)[bfpos+1];
      if duzina>32768 then duz1:=duzina-32768 else duz1:=duzina;
      if not(save(waratape,tapename,514,pozicija,255,konvtip(buffer^)[bfpos]))then begin
        we;exit
      end;
      bfpos:=bfpos+duz1+2;
    until duzina>32768;
  we;
  end;



{ ***********************************
  ASCII to Logo
  ***********************************
}

procedure TTapeFile.asciiLogo;
type konvtip=array[1..65520] of byte;
var
  duzina,duz1,bfpos:word;
  i:integer;
  zaglavlje:array[1..17] of byte;
  pozicija:longint;
  fname,d,n,e:string;

  procedure we;
  begin
    readblocks(tapename)
  end;

begin
  if not(generalasciitoother(pozicija,fname,hcasclogo)) then exit;
  fsplit(fname,d,n,e);
  n:=n+'        ';

  for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
  zaglavlje[1]:=3;
  for i:=12 to 17 do zaglavlje[i]:=0;
  zaglavlje[8]:=32;
  zaglavlje[9]:=76;
  zaglavlje[10]:=79;
  zaglavlje[11]:=71;
  zaglavlje[14]:=$3A;
  zaglavlje[15]:=$C2;
  zaglavlje[16]:=$20;
  zaglavlje[17]:=$00;


  duzina:=duzina+128;
  asciilogo1(buffer^,fname,duzina);
  if duzina=0 then exit;
  zaglavlje[12]:=lo(duzina);
  zaglavlje[13]:=hi(duzina);
  if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
  bfpos:=1;
  repeat
    if not(save(waratape,tapename,128,4,
      255,konvtip(buffer^)[bfpos]))then begin
        we;exit
    end;
    bfpos:=bfpos+128;
  until bfpos>=duzina-128;
  we;
end;



{ ***********************************
   Ascii to Abersoft forth
  ***********************************
}


procedure TTapeFile.asciiforth;
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    if not(generalasciitoother(pozicija,fname,hcascforth)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=2 to 11 do zaglavlje[i]:=32;
    zaglavlje[2]:=68;
    zaglavlje[3]:=73;
    zaglavlje[4]:=83;
    zaglavlje[5]:=67;
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciiforth1(buffer^,fname,duzina);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
  end;



{*****************************
  ASCII to Tassword 2, 3 &
    The last word
******************************
}

procedure TTapeFile.asciitlwtswrd(verzija:integer);
  type konvtip=array[1..65520] of byte;
  var
    duzina,duz1:word;
    i,kontekst:integer;
    zaglavlje:array[1..17] of byte;
    pozicija:longint;
    fname,d,n,e:string;

    procedure we;
    begin
      readblocks(tapename)
    end;

  begin
    case verzija of
      1: kontekst:=hcasctlw;
      2: kontekst:=hctasct2;
      3: kontekst:=hctasct3
    end;
    if not(generalasciitoother(pozicija,fname,kontekst)) then exit;
    fsplit(fname,d,n,e);
    n:=n+'        ';
    for i:=1 to 10 do zaglavlje[i+1]:=ord(n[i]);
    zaglavlje[1]:=3;
    for i:=12 to 17 do zaglavlje[i]:=0;
    duzina:=0;
    asciitlwtswrd1(buffer^,fname,duzina,verzija);
    if duzina=0 then exit;
    zaglavlje[12]:=lo(duzina);
    zaglavlje[13]:=hi(duzina);
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we;exit end;
    if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
       we;exit
    end;
  we;
end;
{ ***************************
   Roman to Warajevo
*****************************
}


procedure TTapefile.romwara;
var
  cc:Boolean;
  brbl,code:integer;
  num:longint;
  brtrst,brblst,romdat:string;
  c:word;
  a: PDialog;
  tapenumber: PNuminputline;
  datoteka:file;
  r:trect;
  Bruce:pointer;
  fs,brtr:longint;
  fleg:byte;

  procedure we;
  begin
    readblocks(tapename);
    cc:=fclose(datoteka);
  end;
begin

  if not(allowed) then exit;
  r.assign(15,6,60,15);
  a:=new(PDialog,Init(r,'Convert'));
  a^.helpctx:=hcromwara;
  r.assign(2,2,30,3);
  Bruce:=new(PLabel,Init(r,'Tape number: ',nil));
  a^.insert(Bruce);
  r.assign(35,2,39,3);
  tapenumber:=new(PNumInputLine,Init(r,2,0,99));
  a^.insert(tapenumber);
  num:=1;
  tapenumber^.setdata(num);
  R.Assign(25,5,35,7);
  Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
  a^.insert(Bruce);
  c:=desktop^.execview(a);
  hourglascur;
  tapenumber^.getdata(brtr);
  dispose(a,done);
  if c=cmok then begin
    str(brtr,brtrst); if brtr<10 then brtrst:='0'+brtrst;
    brbl:=0;
    repeat
      inc(brbl);
      str(brbl,brblst);
      if brbl<10 then brblst:='0'+brblst;
      if brbl<100 then brblst:='0'+brblst;
      romdat:='zxt'+brtrst+brblst;
      if messagebox('Adding file '+romdat,nil,mfOKButton+mfCancelButton+
        mfInformation)<>cmOK then begin
        readblocks(tapename);
        exit
      end;
      if fileexists(romdat+'.h') then assign (datoteka,romdat+'.h') else
       if fileexists(romdat+'.b') then assign (datoteka,romdat+'.b') else begin
          code:=Messagebox('No more blocks',nil,
           mfOKButton+mfCancelButton+mfInformation);
          readblocks(tapename);
          exit;
       end;
      if not(fresetu(datoteka,1)) then begin we;exit end;
      fs:=filesize(datoteka)-1;
      if fs>65520 then begin
        displayerror('This file is too long for Spectrum',4017);
        we; exit
      end;
      if not(fblockread(datoteka,fleg,1)) then begin we;exit end;
      if not(fblockread(datoteka,buffer^,fs)) then begin we;exit end;
      if not(fclose(datoteka)) then begin readblocks(tapename);exit end;
      cc:=save(waratape,tapename,fs,4,fleg,buffer^);
    until brbl=999;
    readblocks(tapename);
  end;
end;
{
**********************************************
Opcija Warajevo to Roman & Easy
**********************************************
}
procedure TTapeFile.Wararoman;
var
    c:word;
    cc:Boolean;
    d,n,e,dummystr,red,brtr,ime:string;
    datoteka:file;
    i,j:integer;
    traka,listatraka:text;
    brojtrake:word;
   procedure we;
   begin
     cc:=fclose(datoteka);
     cc:=fcloset(traka);
     readblocks(tapename);
   end;

begin
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;
   hourglascur;
   fsplit(tapename,d,n,e);
   if fileexists('tapes.zx') then chdir ('files');

   while fileexists('..\'+n+'.zxt') do inc(n[1]);
   assign(listatraka,'..\tapes.zx');
   if not(fresett(listatraka)) then begin
     displayerror('No file TAPES.ZX in parent directory',0);
     exit
   end;
   brojtrake:=0;
   while not eof(listatraka) do begin
     brojtrake:=brojtrake+1;
     if not(freadln(listatraka,red)) then begin
       cc:=fcloset(listatraka); exit
     end
   end;
   cc:=fcloset(listatraka);
   inc(brojtrake);
   str(brojtrake:2,brtr);
   for j:=1 to 2 do if brtr[j]=' ' then brtr[j]:='0';

   append(listatraka);
   if not(fwriteln(listatraka,n)) then begin
     cc:=fcloset(listatraka); exit
   end;
   cc:=fcloset(listatraka);
   assign(traka,'..\'+n+'.zxt');
   if not(frewritet(traka)) then exit;

   for i:=1 to noofblocks do begin
     if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin we;exit end;
     str(i:3,dummystr);
     for j:=1 to 3 do if dummystr[j]=' ' then dummystr[j]:='0';
     ime:='zxt'+brtr+dummystr;
     if blocks^[i].duzina1=17 then ime:=ime+'.h' else ime:=ime+'.b';
     assign(datoteka,ime);
     if not(frewriteu(datoteka,1)) then begin
       cc:=fcloset(traka);
       readblocks(tapename);
       exit;
     end;
     if not(fblockwrite(datoteka,blocks^[i].flag,1)) then begin we; exit end;
     { Specijalno ako je duzina 65535 }
     if blocks^[i].duzina1=65534 then begin
       DisplayError('Sample can not be converted',0);
       we; exit
     end;
     if blocks^[i].duzina1=65535 then begin
       decompress(ptr(seg(buffer^),ofs(buffer^)+6)
        ,izlbuffer,blocks^[i].duzdec,
         blocks^[i].duzcomp,blocks^[i].duzsign);
       if not(fblockwrite(datoteka,
       izlbuffer^,blocks^[i].duzdec-1)) then begin we; exit end;
     end
      else if not(fblockwrite(
        datoteka,buffer^,blocks^[i].duzina1)) then begin  we; exit end;
     if not(fclose(datoteka)) then begin
       cc:=fcloset(traka);
       readblocks(tapename);exit
     end;
     ime:=copy(blocks^[i].naziv,6,10);
     if blocks^[i].duzina1=17 then ime:=ime+'.h' else ime:=ime+'.b';
     if not(fwriteln(traka,ime)) then begin
       cc:=fcloset(traka); exit
     end;
   end {else /regular /};
   cc:=fcloset(traka);
   readblocks(tapename);
end;

{****************************
  Lunter Z80 to Warajevo
  Polish to Warajevo
 ****************************
}

procedure TTapefile.lunterpolishwara(tip:integer);
{ tip=1 Z80
  tip=2 polish
}
var ulazna:file;
  duzina:longint;
  fleg,paritet:byte;
  rb:integer;
  fd:PFileDialog;
  filename:string;

procedure we;
var cc:boolean;
begin
  cc:=fclose(ulazna);
  readblocks(tapename);
end;

begin
  duzina:=0;
  if not(allowed) then exit;
  case tip of
    1: begin
      fd:=new(PFileDIalog,Init('*.TAP','Select TAP file to convert','~N~ame',
       fdOpenButton,100));
      fd^.helpctx:=hclunwara;
    end;
    2: begin
      fd:=new(PFileDIalog,Init('TAPE_ZX.SPC','Select SPC file to convert','~N~ame',
       fdOpenButton,100));
      fd^.helpctx:=hcpolishwara;
    end;
    3: begin
      fd:=new(PFileDIalog,Init('*.BLK','Select BLK file to convert','~N~ame',
       fdOpenButton,100));
      fd^.helpctx:=hcspec200wara;
    end;
    4: begin
      fd:=new(PFileDIalog,Init('*.LTP','Select LTP file to convert','~N~ame',
       fdOpenButton,100));
      fd^.helpctx:=hcgarabikwara;
    end;

  end;
  if desktop^.execview(fd)<> cmcancel then begin
    hourglascur;
    fd^.getfilename(filename);
    filename:=fexpand(filename);
    if not(fileexists(filename)) then begin dispose(fd,done); exit end;
  end
   else begin dispose(fd,done); exit end;
  dispose(fd,done);
  assign(ulazna,filename);
  if not(fresetu(ulazna,1)) then begin we;exit;end;
  while not eof(ulazna) do begin
    if not(fblockread(ulazna,duzina,2)) then begin we;exit;end;

    if (tip=2) or (tip=4) then duzina:=duzina+2;
    if (duzina<2) or (duzina>65500) then begin
      displayerror('This is not Lunter Z80 tape format',0);
      we; exit
    end;
    if not(fblockread(ulazna,fleg,1)) then begin we;exit;end;
    if not(fblockread(ulazna,buffer^,duzina-2))then begin we;exit;end;
    if not(fblockread(ulazna,paritet,1)) then begin we;exit;end;
    if not(save(waratape,tapename,duzina-2,4,fleg,buffer^)) then begin
      we;exit
    end;
  end;
  we;
end;

{****************************
  ZX Museum to Warajevo
 ****************************
}

procedure TTapefile.museumwara;
begin
 if not(allowed) then exit;
 hourglascur;
 DoMuseumWara('',tapename, buffer);
 readblocks(tapename);
end;

procedure TTapefile.zx32wara;
begin
 if not(allowed) then exit;
 hourglascur;
 DoZX32TapWara(tapename, buffer);
 readblocks(tapename);
end;

procedure TTapefile.lunterwara;
var datoteka:file;
wtapename,d,n,e:string;
i:integer;
cc:Boolean;
procedure we;
begin
  cc:=fclose(datoteka);
end;
begin
  hourglascur;
  if waratape then lunterpolishwara(1)
  else begin
    fsplit(tapename,d,n,e);
    wtapename:=outputfile('W'+n+'.TAP',tapename);
    if wtapename='' then exit;
    assign(datoteka,wtapename);
    if not(frewriteu(datoteka,1)) then begin  exit end;
    if not(fblockwrite(datoteka,prazna,12)) then begin we; exit end;
    if not(fclose(datoteka)) then begin  exit end;
    for i:=1 to noofblocks do begin
      if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin
         exit
      end;
      if not(save(true,wtapename,blocks^[i].duzina1,
         4,blocks^[i].flag,buffer^)) then begin
         exit
      end;
    end;
  end;
end;
procedure TTapefile.polishwara;
begin
  lunterpolishwara(2)
end;
procedure TTapefile.spect20wara;
begin
  lunterpolishwara(3)
end;
procedure TTapefile.garabikwara;
begin
  lunterpolishwara(4)
end;
{***************************
  Tomaz Kac TZX to Warajevo
 ***************************}


procedure TTapefile.TZXWara;
var cc:Boolean;
begin
  if not(allowed) then exit;
  hourglascur;
  DoTzx('',tapename,buffer,izlbuffer,cc);
  readblocks(tapename);
end;
{***************************
  VOC to Warajevo
 ***************************}


procedure TTapefile.VOCWara;
begin
  if not(allowed) then exit;
  hourglascur;
  DoVOC('',tapename,buffer,izlbuffer);
  readblocks(tapename);
end;
{ *******************************
   Warajevo to Lunter Z80
   Warajevo to Polish SP
   Warajevo to Spectrum 2.00 BLK
   Warajevo to Garabik LPT
   Warajevo to Kac TZX
   Warajevo to ZX Museum ZXS
   Warajevo to ZX32 ZXT
  *******************************
}

procedure TTapefile.waralunterpolish(tip:integer);
type
  konvtip=array[0..65520] of byte;
var izlazna:file;
  paritet,bajt:byte;
  dummy:word;
  i,j,poslatibajtova,ukupnaduz:longint;
  filename,d,n,e:string;

  procedure we;
  var cc:boolean;
  begin
    cc:=fclose(izlazna);
    readblocks(tapename);
  end;
var samplehdr:array[-1..7] of byte;
const tzxempty:array[1..10] of char=('Z','X','T','a','p','e','!',#$1A,#1,#1);
  tzxblockhdr:array[1..3] of byte=($10,0,2);
  riffempty:array[1..48] of char=('R','I','F','F',#0,#0,#0,#0,
  'S','N','A','P',
  'f','m','t','z',#8,#0,#0,#0, #0,#1, #0,#0, #0,#0, #255, #255,
  't','a','p','e',#0,#0,#0,#0, #0,#0,#0,#0, #0,#0,#0,#0, #0,#0,#0,#0);
begin
   fsplit(tapename,d,n,e);
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;   {5,33}
   end;
   case tip of
     1: filename:=outputfile('L'+n+'.TAP',tapename);
     2: filename:=outputfile('TAPE_ZX.SPC',tapename);
     3: filename:=outputfile(n+'.BLK',tapename);
     4: filename:=outputfile(n+'.LTP',tapename);
     5: filename:=outputfile(n+'.TZX',tapename);
     6: filename:=outputfile(n+'.ZXS',tapename);
     7: filename:=outputfile(n+'.ZXT',tapename);

   end;
   hourglascur;
   if filename='' then exit;
   assign(izlazna,filename);
   if not(frewriteu(izlazna,1)) then begin we;exit;end;
   if tip=5 then
     if not(fblockwrite(izlazna,tzxempty,10)) then begin we; exit end;
   if tip=7 then begin
{     riffempty[37]:=chr(lo(tapesetting.position));
     riffempty[38]:=chr(hi(tapesetting.position));}
     if not(fblockwrite(izlazna,riffempty,48)) then begin we; exit end;
   end;
   for i:=1 to noofblocks do begin
     if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin
       we;exit
     end;
     if (blocks^[i].duzina1=65534) and (tip<>5) then begin
       DisplayError('Sample not allowed',0);
       we;exit
     end;
     if (blocks^[i].duzina1=65534) and (tip=5) then begin     (************************)
       poslatibajtova:=blocks^[i].duzdec;
       if blocks^[i].duzcomp=blocks^[i].duzdec then
         move(konvtip(buffer^)[6],izlbuffer^,blocks^[i].duzdec)
       else decompress(ptr(seg(buffer^),ofs(buffer^)+6)
         ,izlbuffer,blocks^[i].duzdec,
          blocks^[i].duzcomp,blocks^[i].duzsign);

       samplehdr[-1]:=$15;
       samplehdr[0]:=lo(getsampper(blocks^[i].flag));
       samplehdr[1]:=hi(getsampper(blocks^[i].flag));
       samplehdr[2]:=0;samplehdr[3]:=0;
       samplehdr[4]:=(blocks^[i].flag and $07)+1;
       samplehdr[5]:=lo(poslatibajtova);
       samplehdr[6]:=hi(poslatibajtova);
       samplehdr[7]:=0;
       if not(fblockwrite(izlazna,samplehdr,9)) then begin we; exit end;
       if not(fblockwrite(izlazna,izlbuffer^,poslatibajtova)) then
       begin we; exit end;
     end
     else begin
       poslatibajtova:=blocks^[i].duzina1;
       if blocks^[i].duzina1=65535 then begin
         decompress(ptr(seg(buffer^),ofs(buffer^)+6)
         ,izlbuffer,blocks^[i].duzdec,
          blocks^[i].duzcomp,blocks^[i].duzsign);
          poslatibajtova:=blocks^[i].duzdec-1;
       end;
       paritet:=blocks^[i].flag;
       for j:=0 to poslatibajtova-1 do begin
         if blocks^[i].duzina1=65535 then bajt:=konvtip(izlbuffer^)[j]
           else bajt:=konvtip(buffer^)[j];
         paritet:=paritet xor bajt;
       end;
       case tip of
         1,3,5,7: ukupnaduz:=poslatibajtova+2;
         2,4: ukupnaduz:=poslatibajtova;
         6: ukupnaduz:=poslatibajtova+7;
       end;
       if tip=5 then
         if not(fblockwrite(izlazna,tzxblockhdr,3)) then begin we; exit end;
       if not(fblockwrite(izlazna,ukupnaduz,2)) then begin
         we;exit
       end;
       if tip=6 then begin
         dummy:=0;
         if not(fblockwrite(izlazna,dummy,2)) then begin we; exit end;
         if not(fblockwrite(izlazna,dummy,1)) then begin we; exit end;
       end;

       if not(fblockwrite(izlazna,blocks^[i].flag,1)) then begin
         we;exit
       end;
       if blocks^[i].duzina1=65535 then begin
         if not(fblockwrite(izlazna,izlbuffer^,poslatibajtova)) then begin
           we;exit
         end
       end
        else begin
         if not(fblockwrite(izlazna,buffer^,poslatibajtova)) then begin
           we;exit
         end
        end;
       if tip=2 then paritet:=paritet xor blocks^[i].flag;
       if not(fblockwrite(izlazna,paritet,1)) then begin we;exit;end;
     end
   end;
   if tip=6 then begin
     dummy:=0;
     if not(fblockwrite(izlazna,dummy,2)) then begin we;exit;end;
   end;
   if tip=7 then begin
     ukupnaduz:=filepos(izlazna);
     if odd(ukupnaduz) then begin
       dummy:=0;
       if not(fblockwrite(izlazna,dummy,2)) then begin we;exit;end;
       inc(ukupnaduz);
     end;
     ukupnaduz:=ukupnaduz-8;
     if not(fseek(izlazna,4)) then begin we; exit; end;
     if not(fblockwrite(izlazna,ukupnaduz,4)) then begin we; exit; end;
     ukupnaduz:=ukupnaduz-40;
     if not(fseek(izlazna,32)) then begin we; exit; end;
     if not(fblockwrite(izlazna,ukupnaduz,4)) then begin we; exit; end;
     ukupnaduz:=ukupnaduz-12;
     if not(fseek(izlazna,44)) then begin we; exit; end;
     if not(fblockwrite(izlazna,ukupnaduz,4)) then begin we; exit; end;

   end;
   we
end;

procedure TTapefile.waralunter;
begin
  waralunterpolish(1)
end;
procedure TTapefile.warapolish;
begin
  waralunterpolish(2)
end;
procedure TTapefile.waraspect20;
begin
  waralunterpolish(3)
end;
procedure TTapefile.waragarabik;
begin
  waralunterpolish(4)
end;
procedure TTapefile.waratzx;
begin
  waralunterpolish(5)
end;
procedure TTapefile.waramuseum;
begin
  waralunterpolish(6)
end;
procedure TTapefile.warazx32;
begin
  waralunterpolish(7)
end;

{
 *****************************
 Warajevo to Irish
 *****************************
}

procedure TTapefile.waraIrish;
type konvtip=array[1..65520] of byte;

var rb:integer;
  duzina,hedduzina,stvduzina:word;
  filename:string[12];
  datoteka:file;
  cc:Boolean;
  i:integer;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of block for convert',0)
  end;

  procedure we;
  begin
    cc:=fclose(datoteka);
  end;
begin
  if not(allowed) then exit;
  rb:=generalconvert(hcwarairish);
  hourglascur;
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  filename:='        .   ';
  case (konvtip(buffer^)[1]) of
    0: filename[12]:='B';
    1: filename[12]:='N';
    2: filename[12]:='R';
    3: filename[12]:='C'
  end;
  for i:=1 to 8 do begin
    if chr(konvtip(buffer^)[i+1]) in ['0'..'9','a'..'z','A'..'Z'] then
      filename[i]:=chr(konvtip(buffer^)[i+1])
     else filename[i]:='_';
  end;
  for i:=10 to 11 do begin
    if chr(konvtip(buffer^)[i]) in ['0'..'9','a'..'z','A'..'Z'] then
      filename[i]:=chr(konvtip(buffer^)[i])
     else filename[i]:='_';
  end;
  assign(datoteka,filename);
  if not(frewriteu(datoteka,1)) then begin we; exit end;
  if not(fblockwrite(datoteka,buffer^,17)) then begin we; exit end;
  duzina:=loadblock(rb+1);
  if duzina=0 then begin doerr; we; exit end;
  if not(fblockwrite(datoteka,buffer^,duzina)) then begin we; exit end;
  cc:=fclose(datoteka);
end;

{ ****************************
  IRISH to Warajevo
  ****************************
}

procedure TTapeFile.IrishWara;
type konvtip=array[1..65520] of byte;
var
  duzina,duz1:word;
  i:integer;
  zaglavlje:array[1..17] of byte;
  pozicija:longint;
  fname,d,n,e:string;
  datoteka:file;
  cc:Boolean;

  procedure we;
  begin
    cc:=fclose(datoteka);
    readblocks(tapename)
  end;

  procedure we1;
  begin
    readblocks(tapename)
  end;

begin
  if not(generalasciitoother(pozicija,fname,hcirishwara)) then exit;
  hourglascur;
  assign(datoteka,fname);
  if not(fresetu(datoteka,1)) then begin we; exit; end;
  if not(fblockread(datoteka,zaglavlje,17)) then begin we; exit; end;
  duzina:=zaglavlje[12]+256*zaglavlje[13];
  if (duzina+17)<>filesize(datoteka) then begin
     displayerror('This is not format of Irish Specem',0);
     we; exit
  end;
  if not(fblockread(datoteka,buffer^,duzina))
    then begin we; exit end;
  if not(fclose(datoteka)) then begin we1; exit end;
  if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we1;exit end;
  if not(save(waratape,tapename,duzina,pozicija,255,buffer^)) then begin
     we1;exit
  end;
  we1
end;
{
 *****************************
 Warajevo to Shalaev
 *****************************
}

procedure TTapefile.waraShalaev;
type konvtip=array[1..65520] of byte;
  Tdirentry=record
    name:array[1..8] of char;
    tip:char;
    startcode:word;
    codelen:word;
    sectorslen:byte;
    clusterofs:byte;
    clusterseg:byte;
  end;
var rb:integer;
  preventry,curentry,empentry:tdirentry;
  duzina,hedduzina,stvduzina,freezone,entnum:word;
  pozicioniraj:longint;
  datoteka:file;
  cc:Boolean;
  i:integer;
  trdfile:string;
  uslov:Boolean;
  a:PFileDialog;

  procedure doerr;
  begin
    Displayerror('You must select HEADER of block for convert',0)
  end;

  procedure we;
  begin
    cc:=fclose(datoteka);
  end;
begin

  if not(allowed) then exit;
  uslov:=false;
repeat
  fillchar(empentry,sizeof(empentry),0);
  rb:=generalconvert(hcWaraTRD);
  if rb=0 then exit;
  if not(isheader(rb)) then begin doerr;  exit end;
  if not(loadheader(rb,hedduzina,stvduzina)) then begin exit end;
  if rb+1>noofblocks then begin doerr; exit end;
  if blocks^[rb+1].flag<>255 then begin doerr; exit end;
  if not(uslov) then begin
    a:=new(PFileDIalog,Init('*.TRD','Select TRD disk name','~N~ame',
     fdOpenButton,100));
    a^.helpctx:=hctrdwara;
    if desktop^.execview(a)<> cmcancel then begin
      hourglascur;
      a^.getfilename(trdfile);
      trdfile:=fexpand(trdfile);
    end
      else exit;
    dispose(a,done);
  end;
  assign(datoteka,trdfile);
  hourglascur;
  if not(fresetu(datoteka,1)) then exit;
  entnum:=0;
  repeat
    preventry:=curentry;
    if not(fblockread(datoteka,curentry,16)) then begin we; exit end;
    inc(entnum);
    if entnum>128 then begin
      DisplayError('Too much entries',0);
      we; exit
    end;
  until (curentry.name=empentry.name);
  with curentry do begin
    case (konvtip(buffer^)[1]) of
      0: tip:='B';
      1,2: tip:='D';
      3: tip:='C';
    end;
    for i:=1 to 8 do name[i]:=chr(konvtip(buffer^)[i+1]);
    codelen:=konvtip(buffer^)[12]+256*konvtip(buffer^)[13];
    if tip='B' then startcode:=konvtip(buffer^)[16]+256*konvtip(buffer^)[17]
       else startcode:=konvtip(buffer^)[14]+256*konvtip(buffer^)[15];

    sectorslen:=codelen div 256;
    if codelen mod 256<>0 then inc (sectorslen);

    if entnum=1 then begin
      clusterofs:=0;
      clusterseg:=1;
    end
     else begin
       freezone:=preventry.clusterseg*16+preventry.clusterofs+
         preventry.sectorslen;
       clusterofs:=freezone mod 16;
       clusterseg:=freezone div 16;
     end;
  end;
  if not(fseek(datoteka,filepos(datoteka)-16)) then begin we; exit; end;
  if not(fblockwrite(datoteka,curentry,16)) then begin we; exit end;
duzina:=loadblock(rb+1);
if duzina=0 then begin doerr; we; exit end;
pozicioniraj:=longint(256)*(curentry.clusterseg*16+curentry.clusterofs);
if not(fseek(datoteka,pozicioniraj)) then begin we; exit end;
if not(fblockwrite(datoteka,buffer^,duzina)) then begin we; exit end;
  cc:=fclose(datoteka);
if not(uslov) then if messagebox(^C'More blocks?',
      nil,mfOKButton+mfCancelButton+mfConfirmation)=cmOK  then uslov:=true;

until not(uslov);
end;

procedure TTapefile.ShalaevWara;
begin
  if not(allowed) then exit;
  hourglascur;
  doshalawara(tapename,buffer);
  readblocks(tapename);
end;

{
**********************************************
Brukner to Warajevo
**********************************************
}
procedure TTapeFile.BruknerWara;
type konvtip=array[1..65520] of byte;
var
  duzina,duz1:word;
  i:integer;
  zaglavlje:array[1..17] of byte;
  pozicija:longint;
  fname,d,n,e:string;
  datoteka:file;
  cc:Boolean;
  blocktype,flag,checksum:byte;

  procedure we;
  begin
    cc:=fclose(datoteka);
    readblocks(tapename)
  end;

  procedure we1;
  begin
    readblocks(tapename)
  end;

begin
  if not(generalasciitoother(pozicija,fname,hcbruknerwara)) then exit;
  hourglascur;
  assign(datoteka,fname);
  if not(fresetu(datoteka,1)) then begin we; exit; end;
  if not(fblockread(datoteka,blocktype,1)) then begin we; exit; end;
  if not(fblockread(datoteka,flag,1)) then begin we; exit; end;
  if blocktype=0 then begin
    if not(fblockread(datoteka,zaglavlje,17)) then begin we; exit; end;
    duzina:=zaglavlje[12]+256*zaglavlje[13];
    if (duzina+22)<>filesize(datoteka) then begin
       displayerror('This is not format of Brukner''s ZX',0);
       we; exit
    end;
   if not(fblockread(datoteka,checksum,1)) then begin we; exit; end;
   if not(fblockread(datoteka,flag,1)) then begin we; exit; end;
  end
   else duzina:=filesize(datoteka)-3;
  if not(fblockread(datoteka,buffer^,duzina)) then begin we; exit end;
  if not(fclose(datoteka)) then begin we1; exit end;
  if blocktype=0 then
    if not(save(waratape,tapename,17,pozicija,0,zaglavlje)) then begin we1;exit end;
  if not(save(waratape,tapename,duzina,pozicija,flag,buffer^))then begin we1;exit end;
  we1
end;

{
**********************************************
WARAJEVO to Brukner
**********************************************
}
procedure TTapeFile.WaraBrukner;
type
  konvtip=array[0..65520] of byte;

var a:PDialog;
    R:TRect;
    c:word;
    cc:Boolean;
    d,n,e,dummystr:string;
    datoteka:file;
    i,j,poslatibajtova:word;
    p,paritet,bajt:byte;
   procedure we;
   begin
     cc:=fclose(datoteka);
     readblocks(tapename);
   end;

begin
   hourglascur;
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;
   p:=1;
   fsplit(tapename,d,n,e);
   for i:=1 to noofblocks do begin
     if not(load(waratape,tapename,blocks^[i].pocetak,buffer^)) then begin
       we;exit
     end;
     str(i:3,dummystr);
     for j:=1 to length(dummystr) do
       if dummystr[j]=' ' then dummystr[j]:='0';
     assign(datoteka,n+'.'+dummystr);
     if not(frewriteu(datoteka,1)) then begin
        we;
        readblocks(tapename);
        exit;
     end;
     if not(fblockwrite(datoteka,p,1)) then begin we;exit end;
     if not(fblockwrite(datoteka,blocks^[i].flag,1)) then begin we;exit end;
     (* Specijalno ako je duzina 65535 *)
     if blocks^[i].duzina1=65534 then begin
       DisplayError('Samples can not be converted',0);
       we; exit
     end;
     if blocks^[i].duzina1=65535 then begin
        decompress(ptr(seg(buffer^),ofs(buffer^)+6)
         ,izlbuffer,blocks^[i].duzdec,
        blocks^[i].duzcomp,blocks^[i].duzsign);
        if not(fblockwrite(datoteka, izlbuffer^,blocks^[i].duzdec-1)) then begin
          we;exit
        end;
     end
      else if not(fblockwrite(
        datoteka,buffer^,blocks^[i].duzina1)) then begin we; exit end;
     poslatibajtova:=blocks^[i].duzina1;
     if blocks^[i].duzina1=65535 then begin
        poslatibajtova:=blocks^[i].duzdec-1;
     end;
     paritet:=blocks^[i].flag;
     for j:=0 to poslatibajtova-1 do begin
       if blocks^[i].duzina1=65535 then bajt:=konvtip(izlbuffer^)[j]
         else bajt:=konvtip(buffer^)[j];
       paritet:=paritet xor bajt;
     end;

     if not(fblockwrite(datoteka,paritet,1)) then begin we;exit end;
     if not(fclose(datoteka)) then begin readblocks(tapename);exit end
   end {else /regular /};
   readblocks(tapename);
end;
{
**********************************************
WARAJEVO to VOC
**********************************************
}
procedure TTapeFile.WaraVOC;
type
  konvtip=array[0..65520] of byte;

var a:PDialog;
    R:TRect;
    c:word;
    cc:Boolean;
    d,n,e,dummystr,vocname:string;
    datoteka:file;
    i,j,poslatibajtova:word;
    p,paritet,bajt:byte;
   procedure we;
   begin
     readblocks(tapename);
   end;

begin
   hourglascur;
   if not(allowed) then exit;
   if noofblocks=0 then begin
     displayerror('Tape #'+tapename+'#is empty',0);
     exit;
   end;
   p:=1;
   fsplit(tapename,d,n,e);
   vocname:=outputfile(N+'.voc',tapename);
   if vocname='' then exit;
   if not(makevoc(vocname)) then exit;
   for i:=1 to noofblocks do begin
     if not(load(waratape,tapename,blocks^[i].pocetak,konvtip(buffer^)[1])) then begin
       we;exit
     end;
   if blocks^[i].duzina1<>65534 then begin
     if blocks^[i].duzina1=65535 then begin
        decompress(ptr(seg(buffer^),ofs(buffer^)+7)   {pomjeren za 1 bajt}
         ,izlbuffer,blocks^[i].duzdec,
        blocks^[i].duzcomp,blocks^[i].duzsign);
        move(izlbuffer^,konvtip(buffer^)[1],blocks^[i].duzdec-1);
     end;
     poslatibajtova:=blocks^[i].duzina1;
     if blocks^[i].duzina1=65535 then begin
        poslatibajtova:=blocks^[i].duzdec-1;
     end;
     paritet:=blocks^[i].flag;
     for j:=0 to poslatibajtova-1 do begin
       bajt:=konvtip(buffer^)[j+1];
       paritet:=paritet xor bajt;
     end;
     poslatibajtova:=poslatibajtova+2;
     konvtip(buffer^)[0]:=blocks^[i].flag;
     konvtip(buffer^)[poslatibajtova-1]:=paritet;
     if not(block2voc(vocname,buffer,izlbuffer,poslatibajtova,115
       )) then begin we; exit end;
   end;
     if blocks^[i].duzina1=65534 then begin
       poslatibajtova:=blocks^[i].duzdec;
       if blocks^[i].duzcomp=blocks^[i].duzdec then
         move(konvtip(buffer^)[7],izlbuffer^,blocks^[i].duzdec)
       else decompress(ptr(seg(buffer^),ofs(buffer^)+7)
         ,izlbuffer,blocks^[i].duzdec,
          blocks^[i].duzcomp,blocks^[i].duzsign);
       if not(sample2voc(vocname,izlbuffer,buffer,poslatibajtova,
       blocks^[i].flag)) then begin we;exit end;
     end;
   end {else /regular /};
   if not(finishvoc(vocname)) then begin readblocks(tapename);exit end;
   readblocks(tapename);
end;

{************************************
  ADVANCED CASSETTE INTERFACE SETTING
 ************************************}


function ReadTape(var Buffer; LTC1,LTC2,STC1,STC2,TC1,TC2,TC3,TC4:byte;
  SBAddr:word; UseBusy:Boolean; var Sound,CRC:Boolean):byte; external;
function SampleTape(Delay,FromAddr,ToAddr:Longint;sbaddr:word;
 UseBusy,Sound:Boolean):Longint; external;
procedure CopyToDos(FromAddr:Longint; ToAddr:Pointer; SizeBlk:Word);external;
function testsb:word;external;
(* 0 => OK, 1 => Windows, 2 => CRC error, 3 => escaped, 4=> zero length *)

{$L READTAPE.OBJ}


procedure TTapefile.Advcset;
Begin
  if casspars.soundbladdr=1 then casspars.soundbladdr:=TestSB;
  doadvset;
end;
{*********************************
  RECEIVE USING CASSETTE INTERFACE
 *********************************}

{    Status:=ReadTape(Buffer,$9C,$C6,$C9,$D4,$B0,$B2,$CB,$16,0,true,SoundOn,
      CRCOn);
}

Procedure TTapefile.Receivecass;
type
  konvtip=array[0..65520] of byte;
procedure we;
begin
  readblocks(tapename);
end;
var
  Dlg : PDialog;
  Trm:PTerminal;
  R : TRect;
  Control, HScroll : PView;
  c,oldspeed,i:word;
  usebusy,soundop,crc:Boolean;
  sbaddr,duzina,status:word;
  fleg:Byte;
  st,st1:string;
  TBuf:Textbuf;
  T:Text;
Begin
  R.Assign(12,2,65,20);
  New(Dlg, Init(R, 'Load'));

  Dlg^.helpctx:=hccasstransfer;
  R.Assign(5,2,36,3);
  Control := New(PCheckboxes, Init(R,
    NewSItem('~H~ear loading sound',Nil)));
  Dlg^.Insert(Control);

  R.Assign(5,4,35,5);
  Control := New(PCheckboxes, Init(R,
    NewSItem('Tape loading error ~c~heck',Nil)));
  Dlg^.Insert(Control);

  R.Assign(22,6,28,7);
  Control := New(PNumInputLine, Init(R, 4, 1000, 9999));
  Dlg^.Insert(Control);

  R.Assign(5,6,20,7);
  Dlg^.Insert(New(PLabel, Init(R, '~E~fective speed', Control)));

  R.Assign(5,9,35,12);
  Control := New(PRadiobuttons, Init(R,
    NewSItem('B G Services at LPT1',
    NewSItem('Spanish at LPT1',
    NewSItem('Sound blaster',Nil)))));
  Dlg^.Insert(Control);

  R.Assign(5,8,18,9);
  Dlg^.Insert(New(PLabel, Init(R, 'Input ~d~evice', Control)));

  R.Assign(10,14,22,16);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(28,14,40,16);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  Dlg^.setdata(Tapestdset);
  oldspeed:=Tapestdset.speed;
  c:=Desktop^.execview(Dlg);
  hourglascur;
  if c=cmOk then  Dlg^.getdata(Tapestdset);
  dispose(dlg,done);
  if c<>cmOK then exit;
  if oldspeed<>Tapestdset.speed then begin
    with Tapestdset do with casspars do begin
      maximpuls:=trunc(265-129871/speed);
      if maximpuls>255 then maximpuls:=255;
      if maximpuls<0 then maximpuls:=0;
      treshold:=trunc(263-88548/speed);
      if treshold>254 then treshold:=254;
      if treshold<0 then treshold:=0;
      cdelay:=trunc(34-speed*7.7419e-3);
      if cdelay<1 then cdelay:=1;
      if cdelay>255 then cdelay:=255;
    end;
  end;
  with tapestdset do begin
    if sound=1 then soundop:=true else soundop:=false;
    if errchk=1 then crc:=true else crc:=false;
    case device of
      0: begin
        sbaddr:=0;
        usebusy:=true;
      end;
      1: begin
        sbaddr:=0;
        usebusy:=false;
      end;
      2: begin
        if casspars.soundbladdr=1 then casspars.soundbladdr:=TestSB;
        sbaddr:=Casspars.soundbladdr;

        usebusy:=true;
      end;
    end;
  end;
  R.Assign(12,2,65,20);
  New(Dlg, Init(R, 'Load'));
  R.Assign(1,10,50,16);
  New(Trm,Init(R,nil,Dlg^.StandardScrollBar(sbVertical),200));
  Dlg^.Insert(Trm);
  AssignDevice(T,Trm);
  rewrite(t);
  Desktop^.insert(Dlg);
  dlg^.writestr(1,1,' Tape loading routine',1);
  dlg^.writestr(1,2,' Based on TAPE2TAP by Rui Fernando Ferreira Ribeiro',1);

  dlg^.writestr(1,4,' ESC - exit',1);
  dlg^.writestr(1,5,' S - Sound on',1);
  dlg^.writestr(1,6,' N - Sound off',1);
  dlg^.writestr(1,7,' 1 - Disable error checking',1);
  dlg^.writestr(1,8,' 2 - Enable error checking',1);
  hourglascur;
  while true do begin
    with Casspars do
      Status:=ReadTape(buffer^,leadermax,leadermin,synctotal,
        syncfirst,maximpuls-2,
        maximpuls,treshold,cdelay,sbaddr,usebusy,soundop,crc);
    case status of
      0: begin
        duzina:=konvtip(buffer^)[0]+256*konvtip(buffer^)[1]-1;
        fleg:=konvtip(buffer^)[2];
        st:='Headerless ';
        if duzina=17 then begin
          case konvtip(buffer^)[3] of
            0: st:='Program: ';
            1: st:='Number array: ';
            2: st:='Character array: ';
            3: st:='Bytes: ';
          end;
          for i:=4 to 13 do st:=st+chr(konvtip(buffer^)[i]);
        end;
        str(duzina:5,st1);
        st:=st+'  (length '+st1+')';
        writeln(T,st);
        trm^.ScrollTo(1,6);

{        dlg^.writestr(1,10,st,1);}
        if not(save(waratape,tapename,duzina,4,fleg,konvtip(buffer^)[3])) then begin
          dispose(dlg,done);
          close(t);
          we; exit;
        end;
      end;
      1: begin
        Displayerror('Can not work under Windows',0);
        dispose(dlg,done);close(t);
        we; exit;
      end;
      2: begin
        writeln(t,'Tape loading error');
      end;
      3: begin
        dispose(dlg,done);close(t);
        we; exit;
      end;
    end;
  end;
end;

Procedure TTapeFile.ReceiveSample(unr:byte);
type konvtip=array[0..65520] of byte;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  Last,Adresa:Longint;
  DFlag:Byte;
  P,q,f:Word;
  SoundOp,usebusy:Boolean;
  sbaddr:Word;
var
  DataRec : record
    Lowest : LongInt; {Inputlong}
    Highest : LongInt; {Inputlong}
    Sound : Word; {Checkboxes}
    Freq : Word; {RadioButtons}
    Device : Word; {RadioButtons}
    end;
const frequences:Array[0..3] of word=(233,159,116,79);

procedure we;
begin
  readblocks(tapename);
end;

Begin
R.Assign(10,1,70,21);
New(Dlg, Init(R, 'Sample from tape'));
Dlg^.HelpCtx:=hccasssample;
R.Assign(30,1,45,2);
Control := New(PNumInputLine, Init(R, 10, 1114112, 2147483647));
Dlg^.Insert(Control);

  R.Assign(7,1,29,2);
  Dlg^.Insert(New(PLabel, Init(R, '~L~owest linear address', Control)));

R.Assign(30,3,45,4);
Control := New(PNumInputLine, Init(R, 10, 1114112, 2147483647));
Dlg^.Insert(Control);

  R.Assign(6,3,29,4);
  Dlg^.Insert(New(PLabel, Init(R, '~H~ighest linear address', Control)));

R.Assign(3,12,56,16);
Control := New(PStaticText, Init(R, 'WARNING: Use this option on your own re'+
     'sponsibility.'^M+
     'Program directly access memory addresses. Avoid any'^M+
     'drivers that use RAM above 1M as Windows, QEMM,'^M+
     'EMM386, RAMDRIVE, VDISK, SMARTDRV, HIMEM etc.'));
Dlg^.Insert(Control);


R.Assign(7,4,26,5);
Control := New(PCheckboxes, Init(R,
  NewSItem('Loading ~s~ound',Nil)));

Dlg^.Insert(Control);

R.Assign(5,7,29,11);
Control := New(PRadiobuttons, Init(R,
  NewSItem('15000 Hz',
  NewSItem('22050 Hz',
  NewSItem('30303 Hz',
  NewSItem('44100 Hz',
  Nil))))));
Dlg^.Insert(Control);

  R.Assign(5,6,30,7);
  Dlg^.Insert(New(PLabel, Init(R, 'Sampling ~F~requence', Control)));


R.Assign(30,7,55,10);
Control := New(PRadiobuttons, Init(R,
  NewSItem('BG Services at LPT1',
  NewSItem('Spanish at LPT1',
  NewSItem('Sound Blaster',Nil)))));
Dlg^.Insert(Control);

  R.Assign(30,6,39,7);
  Dlg^.Insert(New(PLabel, Init(R, '~D~evice', Control)));

R.Assign(10,17,25,19);
Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
Dlg^.Insert(Control);

R.Assign(31,17,45,19);
Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
Dlg^.Insert(Control);

Dlg^.SelectNext(False);
with Datarec do begin
  Lowest :=1114112;
  Highest := 1024*2048;
  Sound := 1;
  Device :=0;
  Freq:=1;
end;
Dlg^.SetData(DataRec);
if Desktop^.ExecView(Dlg)<>cmCancel then begin
  dlg^.GetData(DataRec);
  with datarec do begin
    if sound=1 then soundop:=true else soundop:=false;
    case device of
      0: begin sbaddr:=0; usebusy:=true;end;
      1: begin sbaddr:=0; usebusy:=false;end;
      2: begin
        if casspars.soundbladdr=1 then casspars.soundbladdr:=TestSB;
        sbaddr:=Casspars.soundbladdr;
        usebusy:=true;
      end;
    end;
    f:=trunc((frequences[Freq]*0.34)*2);
  case UnR of
    0:;
    1:begin DisplayError('Error: 386+ required!',0); exit; end;
    2:begin DisplayError('Error: cannot work in V86 mode!',0); exit; end;
  end;
  writeln('Unreal mode');
    Last:=SampleTape(f,lowest,highest,sbaddr,usebusy,soundop);
  end;
  dispose(dlg,done);
  Adresa:=DataRec.Lowest;
  While Adresa<Last do begin
    p:=65000;
    if Last-Adresa<65000 then p:=Last-Adresa;
    CopyToDos(Adresa,ptr(seg(buffer^),ofs(buffer^)+6),p);
    dflag:=7+(lo(DataRec.Freq) shl 3);
    move (p,konvtip(buffer^)[0],2);
    move (p,konvtip(buffer^)[2],2);
    konvtip(buffer^)[4]:=0;
    konvtip(buffer^)[5]:=0;
    if not(save(true,tapename,65534,4,dflag,buffer^)) then begin  exit end;
    Adresa:=Adresa+p;
  end;
  we;exit;
end;
dispose(dlg,done);
we;
end;

end.


