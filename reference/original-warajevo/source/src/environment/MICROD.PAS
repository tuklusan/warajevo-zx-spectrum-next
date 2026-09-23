
unit microd;
{$V-,O+}
interface
type

namesarr=array[1..8] of string[80];
tsekopis=array[0..300] of string[80];
recntype=array[1..10] of char;
tfileopis=array[0..300] of record
  prikaz:string[80];
  prviblok:integer;
  pravoime:recntype;
end;
tsortelem=record
  prikaz:string[20];
  starisec:integer;
end;

tmdvsortsec=array[0..300] of tsortelem;

Tmdvfile=object
  allowed:Boolean;
  default:integer;
  mdvnames: namesarr;
  sekopis:^tsekopis;
  fileopis:^tfileopis;
  mdvsortsec:^tmdvsortsec;
  ukupnosektora:integer;
  noofdat:integer;
  constructor init(names:namesarr;bafer:Pointer);
  function OpenMDV:integer;
  procedure unselect;
  function Namewdef(i:integer):String;
  procedure wrprot;
  procedure wrunprot;
  procedure setdefault;
  procedure format;
  procedure rename;
  procedure readsecs;
  procedure readfiles;
  procedure view;
  procedure verify;
  procedure edit;
  procedure directory;
  procedure filrename;
  procedure filhide;
  procedure filunhide;
  procedure fildelete;
  procedure filcopy;
  procedure optimize;
  procedure filrenhidunh(sta:integer);
  procedure waramdrlunter;
end;
Pmdvfile=^TMdvfile;

implementation
uses views,stddlg,hexedito,objects,iorutine,app,
  dos,dialogs,helpctx,msgbox,moucur;
const onesector=543;

type mdvsector=record
  hdflag:byte;
  hdnumb:byte;
  notused:word;
  hdname:recntype;
  hdchk:byte;
  recflg:byte;
  recnum:byte;
  reclen:word;
  recnam:recntype;
  deschk:byte;
  datablock: array[1..512] of byte;
  dchk:byte;
end;

constructor Tmdvfile.init(names:namesarr;bafer:pointer);
var i:integer;
begin
  mdvnames:=names;
  allowed:=false;
  default:=0;
  for i:=1 to 8 do begin
    if mdvnames[i]<>'' then begin
      allowed:=true;
      default:=i;
    end
  end;
  sekopis:=bafer;
  fileopis:=bafer;
  mdvsortsec:=bafer;
end;

function Tmdvfile.namewdef(i:integer):String;
begin
  namewdef:='  '+mdvnames[i];
  if i=default then namewdef[1]:='*';
end;


function TMdvfile.OpenMDV:integer;
var a:PFileDialog;
  c,sektora:integer;
  mdvnum,i,sizef:Longint;
  fname:string;
  datoteka:file;
  broj,sectors:PNumInputLine;
  r:trect;
  bruce:pointer;
  prazna:array[1..onesector] of byte;
  daljinar:pprocenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;
  procedure we;
  var cc:boolean;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
    allowed:=false;
    dispose(a,done);
    openMDV:=0;
  end;

begin
  daljinar:=nil;
  for i:=1 to onesector do prazna[i]:=255;
  a:=new(PFileDIalog,Init('*.MDR','Select microdrive name','~N~ame',
     fdOpenButton,100));
  a^.helpctx:=hcmdvsel;
  r.assign(40,10,43,11);
  mdvnum:=default+1;
  if (mdvnum<1) or (mdvnum>8) then mdvnum:=1;
  Broj:=new(PNumInputLine,Init(r,1,1,8));
  broj^.setdata(mdvnum);
  a^.insert(broj);
  r.assign(38,9,46,10);
  bruce:=new(PLabel,Init(r,'MDV ~N~umber',broj));
  a^.insert(bruce);

  r.assign(40,14,45,15);
  Sectors:=new(PNumInputLine,Init(r,3,10,254));
  a^.insert(Sectors);
  sizef:=254;
  sectors^.setdata(sizef);

  r.assign(38,12,46,13);
  bruce:=new(PLabel,Init(r,'~S~ectors size',sectors));
  a^.insert(bruce);
  a^.selectnext(false);

  if desktop^.execview(a)<> cmcancel then begin
    hourglascur;
    sectors^.getdata(sizef);

    a^.getfilename(fname);
    fname:=fexpand(fname);
    if not(fileexists(fname)) then begin
      assign(datoteka,fname);
      if not(frewriteu(datoteka,1)) then begin we; exit; end;
      new(daljinar,Init(16,10,52,1,sizef));
      desktop^.insert(daljinar);
      for i:=1 to sizef do begin
        if not(fblockwrite(datoteka,prazna,onesector)) then begin we; exit; end;
        daljinar^.update(i,'');
      end;
      killdalj;
      if not(fblockwrite(datoteka,prazna,1)) then begin we; exit; end;
      if not(fclose(datoteka)) then begin allowed:=false end;

    end;
    assign(datoteka,fname);
    if not(fresetu(datoteka,1)) then begin we; exit end;
    sektora:=filesize(datoteka) div onesector;
    if (sektora<10) or (sektora>254) then begin
      displayerror('Bad microdrive file size',0);
      we; exit
    end;
    if not(fclose(datoteka)) then begin allowed:=false end;

    allowed:=true;
    broj^.getdata(mdvnum);
    openMDV:=lo(mdvnum);
    default:=lo(mdvnum);
    mdvnames[lo(mdvnum)]:=fname;
  end
   else openMDV:=0;
  dispose(a,done);
end;

procedure TMdvfile.unselect;
var i:integer;
begin
  for i:=1 to 8 do mdvnames[i]:='';
  allowed:=false;
  default:=0;
end;

Procedure TMdvfile.setdefault;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  vrijednost:longint;
  c:word;
Begin
  R.Assign(25,4,61,15);
  New(Dlg, Init(R, 'Select default microdrive'));
  Dlg^.helpctx:=hcmdvdef;

  R.Assign(2,7,14,9);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(17,7,29,9);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(14,4,17,5);
  Control := New(PNumInputLine, Init(R, 1, 1, 8));
  Dlg^.Insert(Control);

  R.Assign(6,3,25,4);
  Dlg^.Insert(New(PLabel, Init(R, '~D~efault microdrive', Control)));

  c:=desktop^.execview(dlg);

  if c=cmok then begin
    dlg^.getdata(vrijednost);
    if mdvnames[lo(vrijednost)]<>'' then
      default:=lo(vrijednost)
     else displayerror('Microdrive cartridge not inserted',0);
    desktop^.redraw;
  end;
  dispose(dlg,done);
end;

procedure TMdvfile.wrprot;
var datoteka:file;
begin
  if default in [1..8] then if mdvnames[default]<>'' then begin
    assign(datoteka,mdvnames[default]);
    {$I-}
    setfattr(datoteka,readonly);
    {$I+}
    if ioresult<>0 then displayerror('Error changing file attribute',0);
  end;
end;

procedure TMdvfile.wrunprot;
var datoteka:file;
begin
  if default in [1..8] then if mdvnames[default]<>'' then begin
    assign(datoteka,mdvnames[default]);
    {$I-}
    setfattr(datoteka,0);
    {$I+}
    if ioresult<>0 then displayerror('Error changing file attribute',0);

  end;
end;

function checksum255(var sec; start,kraj:integer):byte;
type convniz=array[0..onesector-1] of byte;
var
  suma:Longint;
  i:integer;
begin
  suma:=0;
  for i:= start to kraj do
    suma:=suma+convniz(sec)[i];
  checksum255:=suma mod 255
end;

procedure Tmdvfile.format;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  i,j:integer;
  sektora:longint;
  cc:Boolean;
  sec:mdvsector;
  datoteka:file;
  DataRec : record
    Cname : String[10]; {Inputline}
  end;
  daljinar:pprocenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;

  procedure we;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
  end;
Begin
  daljinar:=nil;
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  R.Assign(14,4,65,16);
  New(Dlg, Init(R, 'Format cartridge'));
  Dlg^.helpctx:=hcMdvFormat;
  R.Assign(27,4,39,5);
  Control := New(PInputLine, Init(R, 10));
  Dlg^.Insert(Control);

  R.Assign(8,4,23,5);
  Dlg^.Insert(New(PLabel, Init(R, 'Cardridge ~n~ame', Control)));

  R.Assign(4,8,17,10);
  Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(29,8,42,10);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  dlg^.getdata(datarec);
  dispose(dlg,done);
  if c=cmOK then begin
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      sektora:=254-(filesize(datoteka) div onesector)+2;
      new(daljinar,init(16,10,52,sektora,254));
      desktop^.insert(daljinar);
      for i:=254 downto sektora do with sec do begin
        daljinar^.update(254-i,'');
        fillchar(sec,onesector,0);
        hdflag:=1;
        hdnumb:=i;
        notused:=0;
        for j:=1 to 10 do hdname[j]:=' ';
        for j:=1 to length(datarec.cname) do hdname[j]:=datarec.cname[j];
        hdchk:=checksum255(sec,0,13);
        if not(fblockwrite(datoteka,sec,onesector))then begin we;exit end;
      end;
      desktop^.delete(daljinar);
      dispose(daljinar,done);
      daljinar:=nil;
      if not(fclose(datoteka)) then exit;
    end;
  end;
end;

procedure Tmdvfile.rename;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  i,j:integer;
  sektora:longint;
  cc:Boolean;
  sec:mdvsector;
  datoteka:file;
  DataRec : record
    Cname : String[10]; {Inputline}
  end;
  daljinar:pprocenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;

  procedure we;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
  end;
Begin
  daljinar:=nil;
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  R.Assign(14,4,65,16);
  New(Dlg, Init(R, 'Rename cartridge'));
  Dlg^.helpctx:=hcmdvrename;
  R.Assign(27,4,39,5);
  Control := New(PInputLine, Init(R, 10));
  Dlg^.Insert(Control);

  R.Assign(8,4,23,5);
  Dlg^.Insert(New(PLabel, Init(R, 'Cardridge ~n~ame', Control)));

  R.Assign(4,8,17,10);
  Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(29,8,42,10);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  dlg^.getdata(datarec);
  dispose(dlg,done);
  if c=cmOK then begin
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      sektora:=(filesize(datoteka) div onesector);
      if (sektora<10) or (sektora>254) then begin
        displayerror('Bad microdrive file size',0);
        we; exit
      end;
      new(daljinar,Init(16,10,52,1,sektora));
      desktop^.insert(daljinar);
      for i:=1 to sektora do with sec do begin
        daljinar^.update(i,'');
        if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
        if hdchk=checksum255(sec,0,13) then begin
          seek(datoteka,filepos(datoteka)-onesector);
          for j:=1 to 10 do hdname[j]:=' ';
          for j:=1 to length(datarec.cname) do
            hdname[j]:=datarec.cname[j];
          hdchk:=checksum255(sec,0,13);
          if not(fblockwrite(datoteka,sec,onesector))then begin
            we;exit
          end;
        end;
      end;
      killdalj;
      if not(fclose(datoteka)) then exit;
    end;
  end;
end;

procedure Tmdvfile.readsecs;
label izlaz;
var datoteka:file;
  st,st1:string;
  sec:mdvsector;
  i,j:integer;

  procedure we;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka);
    ukupnosektora:=0;
  end;
begin
  ukupnosektora:=0;
  if default in [1..8] then if mdvnames[default]<>'' then begin
    assign(datoteka,mdvnames[default]);
    if not(fresetu(datoteka,1))  then exit;
    ukupnosektora:=filesize(datoteka) div onesector;
    for i:=0 to ukupnosektora-1 do begin
      if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
      with sec do begin
        if (hdchk<>checksum255(sec,0,13))
         or ((reclen=0) and (recflg and 2<>0))
         or ((reclen<>0) and (dchk<>checksum255(sec,30,541)))
         or ((reclen<>0) and (deschk<>checksum255(sec,15,28)))
         then begin
          st:='Unformated or bad sector';
          goto izlaz
        end;
        str(hdnumb:3,st1);
        st:=st1+' ';
        if (reclen=0) and (recflg and 2=0) then begin
          st:=st+'Unalocated';
          goto izlaz
        end;
        for j:=1 to 10 do
          if (recnam[j]>=#32) and (recnam[j]<=#128) then st:=st+recnam[j]
            else st:=st+chr(254);
        str(recnum:3,st1);
        st:=st+' Part '+st1+', ';
        str(reclen:3,st1);
        st:=st+'Size '+st1;
        if (recflg and 4)=0 then st:=st+', PRINT file'
          else if recnum=0 then begin
            case datablock[1] of
              0: st:=st+', BASIC';
              1: st:=st+', N.Arr';
              2: st:=st+', C.Arr';
              3: st:=st+', CODE '
            end
          end;

        if (recflg and 2)<>0 then st:=st+', EOF ';

      izlaz:
        sekopis^[i]:=st;
      end
    end;
    if not(fclose(datoteka)) then exit;
  end
end;


procedure Tmdvfile.readfiles;
label izlaz;
var datoteka:file;
  st,st1:string;
  sec:mdvsector;
  i,j:integer;
  w1,w2,w3,w4:word;

  procedure we;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka);
    ukupnosektora:=0;
  end;
begin
  ukupnosektora:=0;
  noofdat:=0;
  if default in [1..8] then if mdvnames[default]<>'' then begin
    assign(datoteka,mdvnames[default]);
    if not(fresetu(datoteka,1))  then exit;
    ukupnosektora:=filesize(datoteka) div onesector;
    for i:=0 to ukupnosektora-1 do begin
      if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
      with sec do begin
        if recnum=0 then
          if (hdchk=checksum255(sec,0,13))
          and (deschk=checksum255(sec,15,28))
          and ((reclen=512) or (recflg and 2<>0))
          and (dchk=checksum255(sec,30,541)) then begin
            st:='';
            for j:=1 to 10 do
              if (recnam[j]>=#32) and (recnam[j]<=#128) then
                st:=st+recnam[j]
               else st:=st+chr(254);
            if (recflg and 4)=0 then st:=st+', PRINT file'
             else begin
               w1:=datablock[2]+256*datablock[3];
               w2:=datablock[4]+256*datablock[5];
               w3:=datablock[6]+256*datablock[7];
               w4:=datablock[8]+256*datablock[9];

               case datablock[1] of
                 0: begin
                   str(w4,st1);
                   if w4<=9999 then st:=st+'run '+st1+', '
                    else st:=st+'bas ';
                   str(w3,st1);
                   st:=st+st1;
                   str(w1-w3,st1);
                   st:=st+'+'+st1;

                 end;
                 1: begin
                   st:=st+'num '+chr(((w3 mod 256) and $1f)+64)+'()';
                   str(w1,st1);
                   st:=st+', '+st1;
                 end;
                 2: begin
                   st:=st+'chr '+chr(((w3 mod 256) and $1f)+64)+'$()';
                   str(w1,st1);
                   st:=st+', '+st1;
                 end;
                 3: begin
                   str(w2,st1);
                   st:=st+'byt '+st1;
                   str(w1,st1);
                   st:=st+', '+st1;
                 end;

               end;
             end;
            fileopis^[noofdat].prikaz:=st;
            fileopis^[noofdat].prviblok:=i;
            fileopis^[noofdat].pravoime:=recnam;
            inc(noofdat);
          end;
      end
    end;
    if not(fclose(datoteka)) then exit;
  end
end;



type TSectorsView=object(Tlistbox)
  sekopis:^tsekopis;
  constructor Init(var Bounds:TRect; ANumCols:
    Integer; AScrollBar: PScrollBar; pblocks:Pointer);
  function GetText(item:integer; maxlen:integer):string;virtual;
end;
PSectorsView=^TSectorsView;

constructor TSectorsView.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar; pblocks:Pointer);
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  sekopis:=pblocks;
end;

function TSectorsView.GetText(item:integer; maxlen:integer):string;
begin
   GetText:=copy(sekopis^[item],1,maxlen);
end;


procedure Tmdvfile.view;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  ll:psectorsview;
  c:word;
Begin
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readsecs;
  R.Assign(2,2,77,20);
  New(Dlg, Init(R, 'View sectors'));
  Dlg^.helpctx:=hcmdvview;
  R.Assign(73,2,74,12);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,2,73,12);
  ll := New(PSectorsView, Init(R, 1, PScrollBar(Control),sekopis));
  Dlg^.Insert(ll);
  ll^.setrange(ukupnosektora);
  R.Assign(1,1,9,2);
  Dlg^.Insert(New(PLabel, Init(R, '~S~ectors', ll)));

  R.Assign(27,13,42,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  dispose(dlg,done);
end;


procedure Tmdvfile.verify;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  ll:psectorsview;
  c:word;
  izabrani:longint;
  datoteka:file;
  cc:Boolean;
  sec:Mdvsector;
    procedure we;
    begin
      cc:=fclose(datoteka);
    end;
Begin
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readsecs;
  R.Assign(2,2,77,20);
  New(Dlg, Init(R, 'Verify sectors'));
  Dlg^.helpctx:=hcmdvsverify;
  R.Assign(73,2,74,12);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,2,73,12);
  ll := New(PSectorsView, Init(R, 1, PScrollBar(Control),sekopis));
  Dlg^.Insert(ll);
  ll^.setrange(ukupnosektora);
  R.Assign(1,1,9,2);
  Dlg^.Insert(New(PLabel, Init(R, '~S~ectors', ll)));

  R.Assign(27,13,42,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  izabrani:=ll^.focused;
  dispose(dlg,done);
  if c=cmOK then begin
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1))  then exit;
      if not(fseek(datoteka,izabrani*onesector)) then begin we; exit end;
      if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
      with sec do begin
        if (hdchk<>checksum255(sec,0,13))
         or ((reclen=0) and (recflg and 2<>0))
         or (deschk<>checksum255(sec,15,28))
         or (dchk<>checksum255(sec,30,541)) then begin
           if messagebox(^C'Adjust this sector?',
            nil,mfOKButton+mfCancelButton+mfConfirmation)=cmOK then begin
              if ((reclen=0) and ((recflg and 2)<>0)) then recflg:=0;
              hdchk:=checksum255(sec,0,13);
              deschk:=checksum255(sec,15,28);
              dchk:=checksum255(sec,30,541);
              if not(fseek(datoteka,izabrani*onesector)) then begin
                we; exit
              end;
              if not(fblockwrite(datoteka,sec,onesector))then begin
                we; exit
              end;

           end {if 'Adjust'}
        end {if checksums}
         else
           c:=messagebox(^C'This sector is OK',nil,mfOKButton);

      end; {with}
      if not(fclose(datoteka)) then exit;
    end
  end
end;



procedure Tmdvfile.edit;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  ll:psectorsview;
  Rb:Pradiobuttons;
  c,c1,area:word;
  izabrani:longint;
  datoteka:file;
  cc:Boolean;
  sec:Mdvsector;
    procedure we;
    begin
      cc:=fclose(datoteka);
    end;
Begin
  if not(default  in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readsecs;
  R.Assign(2,2,77,21);
  New(Dlg, Init(R, 'Edit sector'));
  Dlg^.helpctx:=hcmdvsedit;
  R.Assign(73,2,74,12);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);


  R.Assign(2,2,73,12);
  ll := New(PSectorsView, Init(R, 1, PScrollbar(Control),Sekopis));
  Dlg^.Insert(ll);
  ll^.setrange(ukupnosektora);

  R.Assign(1,1,9,2);
  Dlg^.Insert(New(PLabel, Init(R, '~S~ectors', ll)));

  R.Assign(45,13,69,15);
  Rb := New(PRadiobuttons, Init(R,
    NewSItem('~D~ata bytes (512)',
    NewSItem('~W~hole sector (543)',Nil))));
  Dlg^.Insert(Rb);

  R.Assign(44,12,54,13);
  Dlg^.Insert(New(PLabel, Init(R, '~E~dit area', Rb)));

  R.Assign(2,15,15,17);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(18,15,30,17);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  izabrani:=ll^.focused;
  rb^.getdata(area);
  dispose(dlg,done);
  if c=cmOK then begin
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1))  then exit;
      if not(fseek(datoteka,izabrani*onesector)) then begin we; exit end;
      if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
      case area of
        0: begin
          if ((sec.recflg and 4)<>0) and (sec.recnum=0) then
            c1:=hexeditor(addr(sec.datablock[10]),512-9,0)
           else
             c1:=hexeditor(addr(sec.datablock),512,0);
          sec.hdchk:=checksum255(sec,0,13);
          sec.deschk:=checksum255(sec,15,28);
          sec.dchk:=checksum255(sec,30,541);
        end;
        1: begin
          c1:=hexeditor(addr(sec),onesector,0);
        end
      end;
      if c1=cmOK then begin
        if not(fseek(datoteka,izabrani*onesector)) then begin
          we; exit
        end;
        if not(fblockwrite(datoteka,sec,onesector))then begin
          we; exit
        end;
      end;
      if not(fclose(datoteka)) then exit;
    end
  end
end;

type TFilesView=object(Tlistbox)
  fileopis:^tfileopis;
  constructor Init(var Bounds:TRect; ANumCols:
    Integer; AScrollBar: PScrollBar; pfiles:Pointer);
  function GetText(item:integer; maxlen:integer):string;virtual;
end;
PFilesView=^TFilesView;

constructor TFilesView.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar; pfiles:Pointer);
begin
  Tlistbox.init(Bounds,AnumCols,AscrollBar);
  fileopis:=pfiles;
end;

function TFilesView.GetText(item:integer; maxlen:integer):string;
begin
   GetText:=copy(fileopis^[item].prikaz,1,maxlen);
end;


procedure Tmdvfile.directory;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  ll:pfilesview;
  c:word;
Begin
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readfiles;
  R.Assign(2,2,77,20);
  New(Dlg, Init(R, 'Directory'));
  Dlg^.helpctx:=hcmdvcat;
  R.Assign(73,2,74,12);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,2,73,12);
  ll := New(PFilesView, Init(R, 1, PScrollBar(Control),fileopis));
  Dlg^.Insert(ll);
  ll^.setrange(noofdat);
  R.Assign(1,1,9,2);
  Dlg^.Insert(New(PLabel, Init(R, '~F~iles', ll)));

  R.Assign(27,13,42,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  c:=desktop^.execview(dlg);
  hourglascur;
  dispose(dlg,done);
end;

const
  renaming=1;
  hiding=2;
  unhiding=3;

procedure Tmdvfile.filrenhidunh(sta:integer);
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  ll:Pfilesview;
  i,j:integer;
  sektora:longint;
  cc:Boolean;
  sec:mdvsector;
  datoteka:file;
  OldName,Replname : Recntype; {Inputline}
  Newname:String;
  newnamp:Pinputline;
  daljinar:PProcenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;

  procedure we;
  begin
    if daljinar<>nil then dispose(daljinar,done);
    cc:=fclose(datoteka);
  end;
Begin
  daljinar:=nil;
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readfiles;
  if noofdat=0 then begin
    Displayerror('Microdrive cartridge has no files',0);
    exit
  end;
  R.Assign(10,2,72,20);
  case sta of
    renaming: Begin
      New(Dlg, Init(R, 'Rename file'));
      Dlg^.helpctx:=hcmdvfrename
    end;
    hiding: Begin
      New(Dlg, Init(R, 'Hide file'));
      Dlg^.helpctx:=hcmdvfhide
    end;
    unhiding: Begin
      New(Dlg, Init(R, 'Unhide file'));
      Dlg^.helpctx:=hcmdvfunhide
    end;

  end;
  R.Assign(60,1,61,10);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,1,60,10);
  ll := New(PFilesView, Init(R, 1, PScrollBar(Control),fileopis));
  ll^.setrange(noofdat);
  Dlg^.Insert(ll);
  if sta=renaming then begin
    R.Assign(13,11,25,12);
    Newnamp := New(PInputLine, Init(R, 10));
    Dlg^.Insert(Newnamp);

    R.Assign(4,11,13,12);
    Dlg^.Insert(New(PLabel, Init(R, '~N~ew name', Newnamp)));
  end;
  R.Assign(11,13,23,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(32,13,44,15);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);

  c:=desktop^.execview(dlg);
  hourglascur;

  Oldname:=fileopis^[ll^.focused].pravoime;
  case sta of
    renaming: Newnamp^.getdata(Newname);

    hiding: begin
      if Oldname[1]<>#0 then begin
        NewName:=#0;
        for i:=1 to 9 do Newname:=Newname+OldName[i];
      end
       else begin
         Newname:='';
         for i:=1 to 10 do Newname:=Newname+OldName[i];
       end
    end;
    unhiding: begin
      if Oldname[1]=#0 then begin
        NewName:='';
        for i:=2 to 10 do Newname:=Newname+OldName[i];
      end
       else begin
         Newname:='';
         for i:=1 to 10 do Newname:=Newname+OldName[i];
       end
    end;
  end;
  dispose(dlg,done);
  if c=cmOK then begin

    for j:=1 to 10 do replname[j]:=' ';
    for j:=1 to length(newname) do
      replname[j]:=newname[j];

    for i:=0 to noofdat-1 do begin
      if fileopis^[i].pravoime=Replname then begin
        Displayerror('File with this name already exists',0);
        exit
      end
    end;

    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      sektora:=filesize(datoteka) div onesector;
      New(daljinar,Init(16,10,52,1,sektora));
      Desktop^.Insert(daljinar);

      for i:=1 to sektora do with sec do begin
        Daljinar^.update(i,'');
        if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
        if (hdchk=checksum255(sec,0,13)) and (recnam=Oldname) then begin
          seek(datoteka,filepos(datoteka)-onesector);
          recnam:=replname;
          hdchk:=checksum255(sec,0,13);
          deschk:=checksum255(sec,15,28);
          if not(fblockwrite(datoteka,sec,onesector))then begin
            we;exit
          end;
        end;
      end;
      killdalj;
      if not(fclose(datoteka)) then exit;
    end;
  end;
end;

procedure Tmdvfile.fildelete;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  ll:Pfilesview;
  i,j:integer;
  sektora:longint;
  cc:Boolean;
  sec:mdvsector;
  datoteka:file;
  OldName : Recntype; {Inputline}
  Daljinar:PProcenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;

  procedure we;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
  end;
Begin
  daljinar:=nil;
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readfiles;
  if noofdat=0 then begin
    Displayerror('Microdrive cartridge has no files',0);
    exit
  end;
  R.Assign(10,2,72,20);
  New(Dlg, Init(R, 'Delete file'));
  Dlg^.helpctx:=hcmdvfdelete;
  R.Assign(60,1,61,10);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,1,60,10);
  ll := New(PFilesView, Init(R, 1, PScrollBar(Control),fileopis));
  ll^.setrange(noofdat);
  Dlg^.Insert(ll);

  R.Assign(11,13,23,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(32,13,44,15);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);

  c:=desktop^.execview(dlg);
  hourglascur;

  Oldname:=fileopis^[ll^.focused].pravoime;
  dispose(dlg,done);
  if c=cmOK then begin
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      if not(fresetu(datoteka,1)) then begin we; exit end;
      sektora:=filesize(datoteka) div onesector;
      new(daljinar,init(16,10,52,1,sektora));
      desktop^.insert(daljinar);
      for i:=1 to sektora do with sec do begin
        daljinar^.update(i,'');
        if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
        if (hdchk=checksum255(sec,0,13)) and (recnam=Oldname) then begin
          seek(datoteka,filepos(datoteka)-onesector);
          with sec do begin
            recflg:=0;
            recnum:=0;
            reclen:=0;
            for j:=1 to 10 do recnam[j]:=#0;
            deschk:=0;
            for j:=1 to 512 do datablock[j]:=0;
            dchk:=0;
          end;
          if not(fblockwrite(datoteka,sec,onesector))then begin
            we;exit
          end;
        end;
      end;
      killdalj;
      if not(fclose(datoteka)) then exit;
    end;
  end;
end;


procedure TMdvfile.filrename;
begin
  filrenhidunh(renaming);
end;
procedure TMdvfile.filhide;
begin
  filrenhidunh(hiding);
end;
procedure TMdvfile.filunhide;
begin
  filrenhidunh(unhiding);
end;

procedure tmdvfile.optimize;
var datoteka,datoteka2:file;
  i,j,k,z:longint;
  sec1,sec2:mdvsector;
  st1,st,d,n,e,preimenovano:string;
  cc:boolean;
  sws:tsortelem;
  daljinar:pprocenter;
  function log2fiz(log:integer):longint;
  var fiz:longint;
  begin
    if log<=z div 2 then fiz:=2*log else fiz:=(log-(z div 2)-1)*2+1;
    log2fiz:=fiz*onesector;
  end;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;

  procedure we;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
  end;
  procedure we1;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
    cc:=fclose(datoteka2);
  end;

Begin
  daljinar:=nil;
  if messagebox(^C'Optimize microdrive file?',
   nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK then exit;

  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;
  assign(datoteka,mdvnames[default]);
  if not(fresetu(datoteka,1)) then exit;
  z:=filesize(datoteka) div onesector-1;
  new(daljinar,Init(16,10,52,0,3*z));
  desktop^.insert(daljinar);
  for i:=0 to z do begin
    daljinar^.update(i,'');
    if not(fseek(datoteka,log2fiz(i))) then begin we; exit end;
    if not(fblockread(datoteka,sec1,onesector)) then begin we; exit end;
    if (sec1.hdchk<>checksum255(sec1,0,13))
     or ((sec1.reclen=0) and (sec1.recflg and 2<>0))
     or (sec1.deschk<>checksum255(sec1,15,28))
     or (sec1.dchk<>checksum255(sec1,30,541)) then st1:=chr(255)
      else begin
        st1:='          ';
        for k:=1 to 10 do st1[k]:=sec1.recnam[k];
        str((1000+sec1.recnum):4,st);
        st1:=st1+st;
      end;
    mdvsortsec^[i].prikaz:=st1;
    mdvsortsec^[i].starisec:=i;
  end;
  for i:=0 to z-1 do begin
    daljinar^.update(i+z,'');
    for j:=i+1 to z do begin
      if mdvsortsec^[i].prikaz>mdvsortsec^[j].prikaz then begin
        sws:=mdvsortsec^[i];
        mdvsortsec^[i]:=mdvsortsec^[j];
        mdvsortsec^[j]:=sws;
      end;
    end;
  end;
  if not(fclose(datoteka)) then begin we; exit end;
  fsplit(mdvnames[default],d,n,e);
  preimenovano:=d+n+'.BAK';
  if fileexists(preimenovano) then begin
    assign(datoteka2,preimenovano);
    if not(ferase(datoteka2)) then exit;
  end;
  if not(frename(datoteka,preimenovano)) then exit;
  if not(fresetu(datoteka,1)) then exit;
  assign(datoteka2,mdvnames[default]);
  if not(frewriteu(datoteka2,1)) then begin we;exit; end;
  if not(fseek(datoteka,0)) then begin we1; exit end;
  for i:=0 to z div 2 do begin
    daljinar^.update(i+2*z,'');
    if not(fseek(datoteka,log2fiz(mdvsortsec^[i].starisec))) then begin
      we1; exit
    end;
    if not(fblockread(datoteka,sec1,onesector)) then begin we1; exit end;
    if i+z div 2+1<=z then begin
      if not(fseek(datoteka,log2fiz
       (mdvsortsec^[i+z div 2+1].starisec)))then begin
         we1; exit
      end;
      if not(fblockread(datoteka,sec2,onesector)) then begin we1; exit end;
    end;
    sec1.hdnumb:=254-2*i;
    sec1.hdchk:=checksum255(sec1,0,13);
    if not(fblockwrite(datoteka2,sec1,onesector))then begin
      we1;exit
    end;
    if i+z div 2+1<=z then begin
      sec2.hdnumb:=254-2*i-1;
      sec2.hdchk:=checksum255(sec2,0,13);
      if not(fblockwrite(datoteka2,sec2,onesector))then begin
        we1;exit
      end;
    end;
  end;
  killdalj;
  if not(fclose(datoteka)) then exit;
  if not(fclose(datoteka2)) then exit;

end;
procedure Tmdvfile.filcopy;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;
  ll:Pfilesview;
  i,j,dnum,savedefault:integer;
  sektora,p,z:longint;
  cc,free:Boolean;
  sec,sec1:mdvsector;
  datoteka,datoteka2:file;
  dest:PNumInputline;
  OldName : Recntype;
  daljinar:PProcenter;
  procedure killdalj;
  begin
    desktop^.delete(daljinar);
    dispose(daljinar,done);
    daljinar:=nil;
  end;


  procedure we;
  begin
    if daljinar<>nil then killdalj;
    cc:=fclose(datoteka);
    cc:=fclose(datoteka2);
  end;

  function log2fiz(log:integer):longint;
  var fiz:longint;
  begin
    if log<=z div 2 then fiz:=2*log else fiz:=(log-(z div 2)-1)*2+1;
    log2fiz:=fiz*onesector;
  end;

Begin
  daljinar:=nil;
  if not(default in [1..8]) then exit;
  if mdvnames[default]='' then exit;

  readfiles;
  if noofdat=0 then begin
    Displayerror('Microdrive cartridge has no files',0);
    exit
  end;
  R.Assign(10,2,72,20);
  New(Dlg, Init(R, 'Copy file'));
  Dlg^.helpctx:=hcmdvfcopy;
  R.Assign(60,1,61,10);
  Control := New(PScrollBar, Init(R));
  Dlg^.Insert(Control);

  R.Assign(2,1,60,10);
  ll := New(PFilesView, Init(R, 1, PScrollBar(Control),fileopis));
  ll^.setrange(noofdat);
  Dlg^.Insert(ll);

  R.Assign(16,11,19,12);
  Dest := New(PNumInputLine, Init(R, 10,1,8));
  Dlg^.Insert(Dest);

  R.Assign(4,11,13,12);
  Dlg^.Insert(New(PLabel, Init(R, '~D~estination', Dest)));


  R.Assign(11,13,23,15);
  Control := New(PButton, Init(R, '~O~k', cmOK, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(32,13,44,15);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);

  c:=desktop^.execview(dlg);
  hourglascur;
  dest^.getdata(dnum);
  Oldname:=fileopis^[ll^.focused].pravoime;
  dispose(dlg,done);
  if c=cmOK then begin
    if default=dnum then begin
      Displayerror('File can not be copied into itself.',0);
      exit
    end;
    if mdvnames[dnum]='' then begin
      Displayerror('Destination not assigned',0);
      exit;
    end;


    free:=false;
    if default in [1..8] then if mdvnames[default]<>'' then begin
      assign(datoteka,mdvnames[default]);
      assign(datoteka2,mdvnames[dnum]);
      if not(fresetu(datoteka,1)) then begin exit end;
      if not(fresetu(datoteka2,1)) then begin we; exit end;
      sektora:=filesize(datoteka) div onesector;
      Z:=filesize(datoteka2) div onesector-1;
      new(daljinar,init(16,10,52,1,z+1+sektora));
      desktop^.insert(daljinar);
      for i:=1 to z+1 do with sec1 do begin
        daljinar^.update(i,'');
        if not(fblockread(datoteka2,sec1,onesector))then begin we; exit end;
        if (hdchk=checksum255(sec1,0,13)) and (recnam=Oldname) then begin
          seek(datoteka2,filepos(datoteka2)-onesector);
          with sec1 do begin
            recflg:=0;
            recnum:=0;
            reclen:=0;
            for j:=1 to 10 do recnam[j]:=#0;
            deschk:=0;
            for j:=1 to 512 do datablock[j]:=0;
            dchk:=0;
          end;
          if not(fblockwrite(datoteka2,sec1,onesector))then begin
            we;exit
          end;
        end;
      end;

      p:=-1;
      for i:=1 to sektora do with sec do begin
        daljinar^.update(i+z+1,'');
        if not(fblockread(datoteka,sec,onesector))then begin we; exit end;
        if (hdchk=checksum255(sec,0,13)) and (recnam=Oldname) then begin
          repeat
            inc(p);
            if p>z then begin
              Displayerror('Destination cartridge full',0);
              we;
              exit
            end;
            if not(fseek(datoteka2,log2fiz(p))) then begin we; exit end;
            if not(fblockread(datoteka2,sec1,onesector))then begin we; exit end;
            with sec1 do
              if (hdchk=checksum255(sec1,0,13))
               and ((reclen=0) and (recflg and 2=0)) then free:=true
              else free:=false;
          until free;
          sec1.recflg:=sec.recflg;
          sec1.recnum:=sec.recnum;
          sec1.reclen:=sec.reclen;
          sec1.recnam:=sec.recnam;
          sec1.deschk:=sec.deschk;
          sec1.datablock:=sec.datablock;
          sec1.dchk:=sec.dchk;
          if not(fseek(datoteka2,log2fiz(p))) then begin we; exit end;
          if not(fblockwrite(datoteka2,sec1,onesector))then begin we; exit end;
        end
      end;
      killdalj;
      cc:=fclose(datoteka);
      cc:=fclose(datoteka2);
    end;
  end
end;

procedure Tmdvfile.waramdrlunter;
var
  prazna:array[1..onesector] of byte;
  datoteka:file;
  i,j:integer;
  readonly:byte;
  sektora:word;
  cc:boolean;
  procedure we;
  begin
    cc:=fclose(datoteka)
  end;
begin
  if messagebox(^C'Enlarge microdrive file to 254 sectors?',
   nil,mfOKButton+mfCancelButton+mfConfirmation)<>cmOK then exit;
   if default in [1..8] then if mdvnames[default]<>'' then begin
     assign(datoteka,mdvnames[default]);
     for i:=1 to onesector do prazna[i]:=0;
     if not(fresetu(datoteka,1)) then exit;
     sektora:=filesize(datoteka) div onesector;
     if not(fseek(datoteka,sektora*onesector)) then begin we; exit end;
     for i:=sektora to 253 do begin
       if not(fblockwrite(datoteka,prazna,onesector)) then begin we;exit end;
     end;
     readonly:=0;
     if not(fblockwrite(datoteka,readonly,1)) then begin we; exit end;
     cc:=fclose(datoteka);
   end;
end;

end.