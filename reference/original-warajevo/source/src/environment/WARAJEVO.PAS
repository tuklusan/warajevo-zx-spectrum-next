

{$V-}
program ZXTOOLS;
uses
   tvgraph,editors,
   Objects, Drivers,Views, Menus, App,dialogs,iorutine,dos,gadgets
   ,tapes,dosmenu,snapshot,menucons,helpfile,msgbox,helpctx,memory,overlay,
   setup,chain,microd,histlist,dbase,endprogram,moucur,setstate,start,timex,
   konvtap;
{$O TAPES}
{$O HISTLIST}
{$O KONVTAP}
{$O SNAPSHOT}
{$O STDDLG}
{$O OPNDLG}
{$O HELPFILE}
{$O HEXEDITO}
{$O DOSMENU}
{$O APP}
{$O MSGBOX}
{$O IMPREK}
{$O SETUP}
{$O DBASE}
{$O EMPTY}
{$O EDITORS}
{$O MICROD}
{$O CHAIN}
{$O DETECT}
{$O FNT}
{$O SETSTATE}
{$O START}
{$F+}
function SetUnReal:byte; external;
(* Vraca: 0 - instalacija uspjesna
          1 - procesor nije 80386+
          2 - procesor je u V86 modu *)

{$L PASUNRL}

var
  c:word;
  modealloc:word;
const  toemul:Boolean=false;
  mdbase=1;
  mtapes=2;
{ **********************************************************
        Prozor sa imenima trake, snapshot i MDV fajla
 ******************************************************
 }
type
  TNamesWindow=object(TWindow)
    procedure draw;virtual;
  end;
  PNamesWindow=^TNamesWindow;

  procedure TNamesWindow.draw;
  var tapetype:String;
  begin
     twindow.draw;
     if radstrakom.waratape then tapetype:='Warajevo '
       else tapetype:='Lunter Z80 ';
     writestr(1,1,tapetype+'tape file: '+RadsTrakom.tapename,1);
     writestr(1,2,'Snapshot file: '+RadsSnapom.snapname,1);
     if emultype <>2 then begin
       writestr(1,3,'Microdrive file 1: '+Radsmdv.namewdef(1),1);
       writestr(1,4,'Microdrive file 2: '+Radsmdv.namewdef(2),1);
       writestr(1,5,'Microdrive file 3: '+Radsmdv.namewdef(3),1);

       writestr(1,6,'Microdrive file 4: '+Radsmdv.namewdef(4),1);
       writestr(1,7,'Microdrive file 5: '+Radsmdv.namewdef(5),1);
       writestr(1,8,'Microdrive file 6: '+Radsmdv.namewdef(6),1);
       writestr(1,9,'Microdrive file 7: '+Radsmdv.namewdef(7),1);
       writestr(1,10,'Microdrive file 8: '+Radsmdv.namewdef(8),1);
       writestr(1,11,'Network options: '+netopts,1);
     end
      else
       writestr(1,4,'Dock cartridge file: '+Radsdockom.dockname,1);

     writestr(1,13,'Database files directory: '+dbasedir,1);
     writestr(1,14,'Emulator file: '+curemul,1);
     writestr(1,15,'Temporary directory: '+tempdir,1);
     writestr(1,16,'Command parameters: '+comopts,1);
  end;
  var prozorim:PNamesWindow;

procedure rundbase; forward;
type
  TMyApp = object(TApplication)
    heap:PHeapView;
    procedure HandleEvent(var Event: TEvent); virtual;
    procedure adjustmenu;
    procedure Getevent(var Event:Tevent); virtual;
    procedure InitMenuBar; virtual;
    procedure InitStatusLine; virtual;
    procedure Idle;virtual;
    function getpalette:PPalette; virtual;
    constructor Init;
    destructor Done; virtual;
  end;

constructor TMyApp.Init;
var r:TRect;
  i:integer;
begin
   TApplication.Init;
   RegisterHelpFile;
   r.assign(4,3,70,21);
   prozorim:=new(pnameswindow,init(r,'Names',1));
   prozorim^.state:=prozorim^.state and not (sfshadow);
   r.assign(70,22,76,23);
   getextent(r);
   Dec(R.B.X);
   R.A.X := R.B.X - 9; R.A.Y := R.B.Y - 1;
   Heap := New(PHeapView, Init(R));
   Insert(Heap);
   desktop^.insert(prozorim);
end;
destructor TMyapp.done;
begin
  TProgram.Done;
  DoneHistory;
  DoneSysError;
  DoneEvents;
  if not(toemul) then DoneVideo;
  DoneMemory;
end;

{ CalcHelpName }

function CalcHelpName: PathStr;
var
  EXEName: PathStr;
  Dir: DirStr;
  Name: NameStr;
  Ext: ExtStr;
begin
  if Lo(DosVersion) >= 3 then EXEName := ParamStr(0)
  else EXEName := FSearch('WARAJEVO.EXE', GetEnv('PATH'));
  FSplit(EXEName, Dir, Name, Ext);
  if Dir[Length(Dir)] = '\' then Dec(Dir[0]);
  CalcHelpName := FSearch('WARAJEVO.HLP', dir);
end;
function TMyApp.GetPalette: PPalette;
const
  CNewColor = CColor + CHelpColor;
  CNewBlackWhite = CBlackWhite + CHelpBlackWhite;
  CNewMonochrome = CMonochrome + CHelpMonochrome;
  P: array[apColor..apMonochrome] of string[Length(CNewColor)] =
    (CNewColor, CNewBlackWhite, CNewMonochrome);
begin
  GetPalette := @P[AppPalette];
end;
procedure TMyApp.GetEvent(var Event: TEvent);
var
  W: PWindow;
  HFile: PHelpFile;
  HelpStrm: PDosStream;
  c:word;
const
  HelpInUse: Boolean = False;
begin
  TApplication.GetEvent(Event);
  case Event.What of
    evCommand:
      if (Event.Command = cmHelp) and not HelpInUse then
      begin
        HelpInUse := True;
        HelpStrm := New(PDosStream, Init(CalcHelpName, stOpenRead));
        HFile := New(PHelpFile, Init(HelpStrm));
        if HelpStrm^.Status <> stOk then
        begin
          c:=MessageBox('Could not open help file.', nil,
            mfError + mfOkButton);
          Dispose(HFile, Done);
        end
        else
        begin
          W := New(PHelpWindow,Init(HFile, GetHelpCtx));
          W^.state:=W^.state and not (sfshadow);
          if ValidView(W) <> nil then
          begin
            c:=ExecView(W);
            Dispose(W, Done);
            if GMode and (picprozor<>nil) 
              then picprozor^.drawview;
          end;
          ClearEvent(Event);
        end;
        HelpInUse := False;
      end;
    evMouseDown:
      if Event.Buttons <> 1 then Event.What := evNothing;
  end;
end;


procedure TMyApp.Idle;
begin
  heap^.update;
  if gmode then MouseCursorOn;
  TApplication.Idle;
  if actcur=HOURGLAS then arrowcur;
end;

procedure TMyApp.InitMenuBar;
var R: TRect;
begin
  GetExtent(R);
  R.B.Y := R.A.Y + 1;
  setmenu(r);
end;
procedure TMyapp.adjustmenu;
begin
  if radstrakom.allowed then begin
    if radstrakom.waratape then enablecommands([cm_tfirst..cm_tlast])
    else begin
      enablecommands([cm_tfirst..cm_tlast]);
      disablecommands([
        cm_texclude,cm_tchgpos,cm_tcompall,cm_tlinonly,cm_tcompsel,
        cm_tdecomp,cm_teff,cm_tcasssample,
        cm_tromwara,cm_twaralun, cm_tirishwara..cm_twarapolish,
        cm_twaraspec20..cm_tzx32wara])
    end
  end
  else begin
     disablecommands([cm_tfirst..cm_tlast]);
     enablecommands([cm_tselect,cm_sendcprg,cm_cfgrs232,cm_tadvcassetting]);
  end;
  if radssnapom.allowed then enablecommands([cm_sfirst..cm_slast])
  else begin
     disablecommands([cm_sfirst..cm_slast]);
     enablecommands([cm_sselect,cm_sspwara,cm_svgawara,cm_sirishwara,
       cm_sjppwara,cm_sgermanwara,cm_ssltwara,
       cm_ssitwara,cm_ssna128wara,cm_ssnpwara,cm_szx32wara]);
  end;
  if radssnapom.allowed and radstrakom.allowed then
    enablecommands([cm_swaraslt])
   else disablecommands([cm_swaraslt]);
  if emultype=2 then radsmdv.allowed:=false else radsdockom.allowed:=false;
  if radsmdv.allowed then enablecommands([cm_mdvfirst..cm_mdvlast])
  else begin
     disablecommands([cm_mdvfirst..cm_mdvlast]);
     if emultype<>2 then enablecommands([cm_mdvsel]);
  end;
  if radsdockom.allowed then enablecommands([cm_dockfirst..cm_docklast])
  else begin
     disablecommands([cm_dockfirst..cm_docklast]);
     if emultype=2 then enablecommands([cm_dockselect,cm_binaros,
     cm_binlros,cm_romtodock]);
  end;

end;
procedure drawwin;
begin
  if prozorim<>nil then begin
    if prozorim^.exposed then prozorim^.draw;
  end;
end;
var filestart:string;

procedure TMyApp.HandleEvent(var Event: TEvent);
var r:trect;
 n:boolean;
 c:word;
 filetip:integer;
begin
  TApplication.HandleEvent(Event);
  if Event.What = evCommand then
  begin
    if (Event.command>=cm_tfirst) and (Event.command<=cm_tlast) and
      Radstrakom.doreadblocks and Radstrakom.allowed then begin
      Radstrakom.Readblocks(radstrakom.tapename);
      Radstrakom.doreadblocks:=false;
    end;
    if event.command<>cm_start then filestart:='';
    case Event.command of
      cm_tfirst..cm_tlast,
      cm_sfirst..cm_slast,
      cm_dockfirst..cm_docklast,
      cm_mdvfirst..cm_mdvlast,
      cm_dbselect,cm_dbunselect: begin
        if modealloc=mdbase then begin
          deallocdb;
          Radstrakom.Init(radstrakom.tapename);
          radssnapom.init(radssnapom.snapname,
            radstrakom.buffer,radstrakom.izlbuffer);
          radsmdv.init(radsmdv.mdvnames,radstrakom.buffer);
          modealloc:=mtapes;
        end;
      end;
      cm_dbfirst..cm_dblast: begin
        if modealloc=mtapes then begin
          Radstrakom.Done;
          allocdb;
          modealloc:=mdbase;
        end;

      end;
    end;
    case Event.Command of
      cm_start: begin
        prepareStart(filestart,filetip,@radssnapom);
        case filetip of
          1,3,5: with radstrakom do begin
            tapename:=filestart;
            if filetip=5 then waratape:=false else waratape:=true;
            allowed:=true;
            readblocks(tapename);
            With Tapesetting do begin
              position:=0;
              Autostart:=1;
              Hidebor:=0;
              loadmeth:=0;
              Stopmode:=0;
            end;
            if filetip in [1,5] then  radssnapom.unselect
              else with radssnapom do begin
                snapname:=tempdir+'TEMP.Z80';
                getheader;
                if is48ksnap then emultype:=0 else emultype:=1;
                if istimex then emultype:=2;
              end;
            setcomopts;
          end;
         2,4: with radssnapom do begin
           if filetip=2 then snapname:=filestart
             else snapname:=tempdir+'TEMP.Z80';
           getheader;
           if is48ksnap then emultype:=0 else emultype:=1;
           if istimex then emultype:=2;
           if emultype<>1 then radstrakom.unselect;
           if filetip=4 then radsmdv.mdvnames[1]:=filestart;
           setcomopts;
         end;
         6: begin
           radssnapom.unselect;
           radsdockom.dockname:=filestart;
           emultype:=2;
           setcomopts;
         end;
         0: Filestart:='';
        end;
        if filetip in [1,2,3,4,5,6] then begin
          toemul:=true;
          n:=savecfg(tempdir+'SPECSIM.CFG',@radstrakom,@radssnapom,
            @radsmdv,@radsdockom,true);
          prozorim^.drawview;
          if not(n) then displayerror('Error writing configuration file',0);
          EndModal(cmquit);
          Exit;
        end
      end;

      cm_tselect: begin
        n:=radstrakom.openfile ;setcomopts;drawwin
      end;
      cm_tunselect: begin
        radstrakom.unselect ; setcomopts; drawwin
      end;

      cm_ttapepar:begin
        radstrakom.tapepar;
        if tapesetting.autostart<>0 then radssnapom.unselect;
        soundsetting.micear:=0;
        setcomopts;
        prozorim^.drawview;
      end;
      cm_tview: radstrakom.view;

      cm_tprscr: radstrakom.printscreen;
      cm_tprpri: radstrakom.printtoprinter;
      cm_tprfile: radstrakom.printtofile;
      cm_tcopynew: radstrakom.copytonew;

      cm_treorder: radstrakom.reorder;
      cm_textract: radstrakom.extract;
      cm_tdelete: radstrakom.delete;
      cm_tadd: radstrakom.add;
      cm_tedit: radstrakom.edit;
      cm_texclude: radstrakom.exclude;
      cm_tchgpos: radstrakom.chgpos;

      cm_tcompall: radstrakom.compressall;
      cm_tlinonly: radstrakom.linearize;
      cm_tcompsel: radstrakom.compressSelected;
      cm_tdecomp: radstrakom.unpack;
      cm_teff: radstrakom.efficiency;

      cm_tsendtsp: radstrakom.sendtospectrum;
      cm_trecsp: radstrakom.receivefromspectrum;
      cm_sendcprg: sendcommprogram;
      cm_cfgrs232: radstrakom.cfgrs232;
      cm_tcasstransfer:radstrakom.receivecass;
      cm_tadvcassetting:radstrakom.advcset;
      cm_tcasssample:radstrakom.receivesample(setunreal);

      cm_tascbas: radstrakom.asciibasic;
      cm_tbasasc: radstrakom.basascii;
      cm_tascgens: radstrakom.asciigens;
      cm_tascc: radstrakom.asciic;
      cm_tascpas: radstrakom.asciipas;
      cm_tgensasc: radstrakom.gensascii;
      cm_tcasc: radstrakom.cascii;
      cm_tpasasc: radstrakom.pasascii;
      cm_tasct2: radstrakom.asciitlwtswrd(2);
      cm_tasct3: radstrakom.asciitlwtswrd(3);
      cm_tt2asc: radstrakom.tswrdascii(2);
      cm_tt3asc: radstrakom.tswrdascii(3);
      cm_tasctlw: radstrakom.asciitlwtswrd(1);
      cm_tlwasc: radstrakom.tlwascii;
      cm_tascml: radstrakom.asciiml;
      cm_tmlasc: radstrakom.mlascii;
      cm_tascforth: radstrakom.asciiforth;
      cm_tforthasc: radstrakom.forthascii;
      cm_tasclogo: radstrakom.asciilogo;
      cm_tlogoasc: radstrakom.logoascii;
      cm_tscrcoltif: radstrakom.screentifcl;
      cm_tscrbwtif: radstrakom.screentifbw;
      cm_tromwara: radstrakom.romwara;
      cm_twararoman: radstrakom.wararoman;

      cm_tlunwara: radstrakom.lunterwara;
      cm_twaralun: radstrakom.waralunter;
      cm_twarairish: radstrakom.warairish;
      cm_tirishwara: radstrakom.irishwara;
      cm_tpolishwara: radstrakom.polishwara;
      cm_twarapolish: radstrakom.warapolish;
      cm_tspec20wara: radstrakom.spect20wara;
      cm_twaraspec20: radstrakom.waraspect20;
      cm_tgarabikwara: radstrakom.garabikwara;
      cm_twaragarabik: radstrakom.waragarabik;
      cm_tbruknerwara: radstrakom.bruknerwara;
      cm_twarabrukner: radstrakom.warabrukner;
      cm_twaratzx: radstrakom.waratzx;
      cm_ttzxwara: radstrakom.tzxwara;
      cm_tvocwara: radstrakom.vocwara;
      cm_tzxsetup: tzxset;
      cm_twaravoc: radstrakom.waravoc;
      cm_twaramuseum: radstrakom.waramuseum;
      cm_tmuseumwara: radstrakom.museumwara;
      cm_twarashalaev:radstrakom.warashalaev;
      cm_tshalaevwara:radstrakom.shalaevwara;
      cm_twarazx32:radstrakom.warazx32;
      cm_tzx32wara:radstrakom.zx32wara;
      cm_sselect: begin
        n:=radssnapom.select ;
        if radssnapom.istimex then snapemul:=2;
        emultype:=snapemul;setcomopts;
        prozorim^.drawview;
      end;
      cm_sunselect: begin
        radssnapom.unselect ; drawwin
      end;
      cm_srename: begin radssnapom.srename;drawwin end;
      cm_sinfo: radssnapom.view;

      cm_sreg: radssnapom.setregs;
      cm_shard: radssnapom.sethrdw;
      cm_smem: radssnapom.editsnap;

      cm_sspwara: radssnapom.SpanishtoZ80;
      cm_swarasp: radssnapom.z80toSpanish;
      cm_svgawara: radssnapom.VGASpecToZ80;
      cm_swaravga: radssnapom.z80toVGASpec;
      cm_sirishwara: radssnapom.PRGtoZ80;
      cm_swarairish: radssnapom.z80toprg;
      cm_sjppwara: radssnapom.SNAtoZ80;
      cm_swarajpp: radssnapom.z80toSNA;
      cm_sgermanwara: radssnapom.SEMtoZ80;
      cm_swaragerman: radssnapom.Z80toSEM;
      cm_ssnpwara: radssnapom.SNPtoZ80;
      cm_swarasnp: radssnapom.Z80toSNP;
      cm_swarasna128: radssnapom.z80tosna128;
      cm_ssna128wara: radssnapom.sna128toz80;
      cm_swaraslt: radssnapom.Z80TAPtoSLT;
      cm_ssltwara:radssnapom.SLTtoZ80TAP;
      cm_ssitwara: radssnapom.SITtoZ80;
      cm_swarasit: radssnapom.Z80toSIT;
      cm_szx32wara: radssnapom.RIFFtoZ80;
      cm_swarazx32: radssnapom.Z80toRIFF;
      cm_sz8048: radssnapom.z80anytoz8048k;
      cm_sz80128: radssnapom.z80anytoz80128k;
      cm_ssntap: radssnapom.snapshottotap;

      cm_emulsel: begin
        c:=emultype;selectemu;
        if ((c=1) and (emultype=0)) or (c=2) or (emultype=2) then
          radssnapom.unselect;
        if emultype=2 then 
           TimexKillsZX(@RadSMdv);
        prozorim^.drawview;
      end;
      cm_inter1: begin inter1;prozorim^.drawview; end;
      cm_speed: begin interrupt;prozorim^.drawview; end;
      cm_joystick: begin joystick;prozorim^.drawview; end;
      cm_printer: begin printer;prozorim^.drawview; end;
      cm_ROM: begin rom;prozorim^.drawview; end;
      cm_test: begin test;prozorim^.drawview; end;
      cm_sound: begin sound;prozorim^.drawview; end;
      cm_video: begin video;prozorim^.drawview; end;
      cm_mswindows: begin mswindows;prozorim^.drawview; end;
      cm_network: begin networkSetup;prozorim^.drawview; end;
      cm_mdvsel: begin
        c:=radsmdv.openmdv;
        if (radsmdv.allowed) and (interfsetting.switch=0) then
          interfsetting.switch:=1;
        setcomopts;
        prozorim^.drawview;

      end;
      cm_munselect: begin
        radsmdv.unselect ; drawwin
      end;
      cm_mdvdef: radsmdv.setdefault;
      cm_mdvcat: radsmdv.directory;
      cm_mdvwrprot: radsmdv.wrprot;
      cm_mdvwrunpr: radsmdv.wrunprot;
      cm_mdvformat: radsmdv.format;
      cm_mdvrename: radsmdv.rename;
      cm_mdvview: radsmdv.view;
      cm_mdvunfrag: radsmdv.optimize;

      cm_mdvsverify: radsmdv.verify;
      cm_mdvsedit: radsmdv.edit;

      cm_mdvfrename: radsmdv.filrename;
      cm_mdvfhide: radsmdv.filhide;
      cm_mdvfunhide: radsmdv.filunhide;
      cm_mdvfdelete: radsmdv.fildelete;
      cm_mdvfcopy: radsmdv.filcopy;
      cm_mdvcwaralunter: radsmdv.waramdrlunter;
      cm_dockselect: begin Radsdockom.Select; drawwin; end;
      cm_dockunselect: begin Radsdockom.Unselect; drawwin; end;
      cm_dockview: Radsdockom.view;
      cm_dockmerge: Radsdockom.merge;
      cm_binaros:radsdockom.binaros;
      cm_binlros:radsdockom.binlros;
      cm_romtodock:radsdockom.romdck;

      cm_dbselect: begin
        selectdbdir; drawwin
      end;
      cm_dbunselect: begin
        dbasedir:=emuldir; drawwin
      end;
      cm_dbrun:rundbase;
      cm_editbase: dbdisplay;
      cm_sortbase: dbsort;
      cm_izvjbase: dbreport;
      cm_bmarkcond: markselected;
      cm_bunmark: unmarkall;
      cm_bswapmark: swappriority;
      cm_cwaraspecpic: warajevospecpic;
      cm_cwarasgd: warajevosgd;
      cm_sgdwara: sgdwara;
      cm_cwararainbow: warajevorainbow;
      cm_crainbowwara: specpicrainwara(true);
      cm_cspecpicwara: specpicrainwara(false);
      cm_cwaraspecbase: warajevospecbase;
      cm_cspecbasewara:specbasewarajevo;
      cm_about: radsdosom.about;
      cm_dir: radsdosom.directory;
      cm_osshell:begin radsdosom.osshell; draw; end;
      cm_cd: radsdosom.changedir;

      cm_emulator: begin
        toemul:=true;
        n:=savecfg(tempdir+'SPECSIM.CFG',@radstrakom,@radssnapom,
         @radsmdv,@radsdockom,true);
        prozorim^.drawview;
        if not(n) then displayerror('Error writing configuration file',0);
        EndModal(cmquit);
        Exit;
      end;

    else
      Exit;
    end;
    adjustmenu;
    ClearEvent(Event);
  end;
end;

procedure TMyApp.InitStatusLine;
var R: TRect;
begin
  GetExtent(R);
  R.A.Y := R.B.Y - 1;
  setstatline(r);
end;
var
  MyApp: TMyApp;

{ *******************************************
  Postavljanje parametara preko baze podataka
  *******************************************}

function transbase(var progelem:exrecdata):Boolean;
var
  cc:Boolean;
  check,d,n,e:string;
  i:integer;
begin
  transbase:=false;
  progelem.direct:=rtrim(progelem.direct);
  progelem.fi.filename:=rtrim(copy(progelem.fi.filename,1,8));
  if progelem.direct[length(progelem.direct)]='\' then
    check:=progelem.direct+progelem.fi.filename
   else check:=progelem.direct+'\'+progelem.fi.filename;
  fsplit(check,d,n,e);
  case progelem.fi.tip of
    'T': e:='.TAP';
    'S': e:='.Z80';
    'M': e:='.MDR';
  end;
  check:=d+n+e;
  if progelem.fi.tip='O' then check:=progelem.direct;
  while not(fileexists(check)) do begin
    if MessageBox(^C'Selected program not found.'+#13+
     ^C'Insert diskette labeled with'+#13+
     #3+progelem.vollab+#13+
     ^C'or check the database!'  , nil,
          mfError + mfOkButton+mfCancelButton)=cmcancel then exit;
  end;
  comopts:=rtrim(progelem.simulopc)+' ';
  with progelem do begin
    If  kompjuter = '16  ' Then emultype:=0;
    If  kompjuter = '48  ' Then emultype:=0;
    If  kompjuter = '2068' Then emultype:=2;
    If  kompjuter = '128 ' Then emultype:=1;
    If  kompjuter = '+2  ' Then begin emultype:=1;comopts:=comopts+'/2 ';end;
    If  kompjuter = '+3  ' Then begin emultype:=1;comopts:=comopts+'/3 ';end;
    If  kompjuter = 'SAM ' Then begin emultype:=0;end;
  end;
  Radstrakom.Init(radstrakom.tapename);
  radssnapom.init(radssnapom.snapname,
    radstrakom.buffer,radstrakom.izlbuffer);
  radsmdv.init(radsmdv.mdvnames,radstrakom.buffer);
  RadsDockom.Init(radsdockom.dockname,radstrakom.buffer);

  modealloc:=mtapes;

  radstrakom.unselect;
  radsmdv.unselect;
  radssnapom.unselect;
  radsdockom.unselect;
  progelem.pozicija:=rtrim(progelem.pozicija);
  for i:=1 to length(progelem.pozicija) do
    if progelem.pozicija[i]=' ' then progelem.pozicija[i]:='0';

  comopts:=comopts+'/D'+progelem.direct+' ';
  case progelem.fi.tip of
    'T': begin
      comopts:=comopts+'/T'+progelem.fi.filename+' /F'+progelem.pozicija;
      comopts:=' /A '+comopts;
    end;
    'S': comopts:=comopts+'/R'+progelem.fi.filename+' ';
    'M': comopts:=comopts+'/#1;'+progelem.fi.filename+' ';
  end;
  cc:=savecfg(tempdir+'SPECSIM.CFG',@radstrakom,@radssnapom,@radsmdv,
  @radsdockom,false);
  if not(cc) then begin
    displayerror('Error writing configuration file',0);
    prozorim^.drawview;
    exit
  end;
  cc:=parse(tempdir+'SPECSIM.CFG',@radstrakom,@radssnapom,@radsmdv,@radsdockom);
  if not(cc) then begin
    displayerror('Error reading configuration file',0);
    prozorim^.drawview;
    exit
  end;
  prozorim^.drawview;
  transbase:=true;
end;


procedure rundbase;
var c,i:word;
  progelem:exrecdata;
  event:tevent;
begin
    c:=dbrun(progelem);
    if c=0 then exit;
    deallocdb;
    if not(transbase(progelem)) then begin
       allocdb;
       modealloc:=mdbase;
       exit;
    end;
    if c=1 then begin
      event.what:=evCommand;
      event.command:=cm_emulator;
      Myapp.putevent(event)
    end;
    if c=3 then begin
      event.what:=evCommand;
      event.command:=cm_start;
      filestart:=progelem.direct;
      Myapp.putevent(event)
    end;
   prozorim^.drawview;
end;

{ ****************************************
    Start aplikacije
  ****************************************
  }

procedure preinit;
var i:integer;
  d,n,e,st:string;
begin

  fsplit(Fexpand(paramstr(0)),d,n,e);
  ovrinit(d+'warajevo.exe');
  tempdir:='';
  if (paramstr(1)='/G') or (paramstr(1)='/g') then gmode:=true else gmode:=false;
  st:=getenv('WARAJEVO');
  if (st='g') or (st='G') then gmode:=true;
  if length(st)>2 then begin
    if st[2]=' ' then begin
      tempdir:=copy(st,3,255);
      if upcase(st[1])='G' then gmode:=true;
    end
    else begin
      if st[length(st)-1]=' ' then begin
        tempdir:=copy(st,1,length(st)-2);
        if upcase(st[length(st)])='G' then gmode:=true;
      end
        else tempdir:=st;
    end;
  end;
  if tempdir<>'' then if tempdir[length(tempdir)]<>'\'
    then tempdir:=tempdir+'\';
  if  gmode=true then begin initialgr;
  (* patch *)
  end else picprozor:=nil;
  hourglascur;

  emuldir:=d;
  radstrakom.tapename:='';
  radssnapom.snapname:='';
  radsdockom.dockname:='';
  for i:=1 to 8 do radsmdv.mdvnames[i]:='';
  curemul:='';
end;


procedure prerun;
label seeparameters;
var tapename,snapname,dockname,d,n,e,dat:String;
   dummy,i,j,zadnji:Integer;
   R:Trect;
   NazProzor:PnamesWindow;
   parseok,callintercfg,imamdr:Boolean;
   intercfg,defcfg,findprog,rundb:string;
   mdvname:namesarr;
   post,code:word;
   event:tevent;
   progelem:exrecdata;
begin
  zadnji:=0;
  tapename:='';
  snapname:='';
  findprog:='';
  filestart:='';
  rundb:='';
  dockname:='';
  for i:=1 to 8 do mdvname[i]:='';
  curemul:=emuldir+emulators[emultype];
  dbasedir:=emuldir;
  callintercfg:=false;
  if tempdir='' then tempdir:=emuldir;
  intercfg:=tempdir+'SPECSIM.CFG';
  defcfg:=tempdir+'DEFAULT.CFG';
  for i:=1 to paramcount do begin
    dat:=paramstr(i);
    for j:=1 to length(dat) do dat[j]:=upcase(dat[j]);
    if copy(dat,1,2)='/C' then begin
      callintercfg:=true;
      val(copy(dat,3,length(dat)-2),post,code);
    end;
    if (copy(dat,1,2)='/S') then begin
      event.what:=evcommand;
      event.command:=cm_start;
      filestart:=fexpand(copy(dat,3,length(dat)));
      if fileexists(filestart) then
        Myapp.putevent(event)
       else filestart:='';
    end;
    fsplit(dat,d,n,e);
    if (e='') and (dat[1]<>'/') then begin
     rundb:=dat; modealloc:=mdbase; goto seeparameters
    end
     else rundb:='';
    if fileexists(dat) then begin
      if e='.Z80' then snapname:=fexpand(dat);
      if e='.DCK' then dockname:=fexpand(dat);
      if e='.TAP' then tapename:=fexpand(dat);
      if e='.MDR' then begin
        zadnji:=(zadnji+1) mod 8;
        mdvname[zadnji]:=fexpand(dat);
        imamdr:=true;
      end;
    end;
  end;
  Radstrakom.Init(tapename);
  radssnapom.init(snapname,radstrakom.buffer,radstrakom.izlbuffer);
  radsmdv.init(mdvname,radstrakom.buffer);
  radsdockom.init(dockname,radstrakom.buffer);

  if radssnapom.snapname<>'' then begin
    if radssnapom.is48ksnap then emultype:=0 else emultype:=1;
    if radssnapom.istimex then emultype:=2;
  end;
  curemul:=emuldir+emulators[emultype];

  modealloc:=mtapes;
  seeparameters:
  defaultfill;
  if not(callintercfg) then if fileexists(defcfg) then begin
    parseok:=parse(defcfg,@radstrakom,@radssnapom,@radsmdv,@radsdockom);
    if not(parseok) then
      DisplayError ('Error in default configuration file',0)
  end;

  if callintercfg then begin
    if post<>0 then begin
      event.what:=evCommand;
      event.command:=post;
      Myapp.putevent(event)
    end;
    if fileexists(intercfg) then begin
      parseok:=parse(intercfg,@radstrakom,@radssnapom,@radsmdv,@radsdockom);
      if not(parseok) then
        DisplayError ('Error in interface configuration file',0);
    end;
  end;
  if rundb<>'' then if dbstart(rundb,progelem) then begin
       if transbase(progelem) then begin
          event.what:=evCommand;
          event.command:=cm_emulator;
          Myapp.putevent(event);
          exit;
        end
  end;
  if (radsmdv.allowed) and (interfsetting.switch=0) then begin
    interfsetting.switch:=1;
    setcomopts;
  end;
  prozorim^.drawview;
  if gmode then with radssnapom do begin
    asm
     mov ax,4
     mov cx,30
     mov dx,18
     int $33
    end;

    r.assign(28,8+5,62,22+5);
    picprozor:=new(pscrwindow,init(r,'Screen'));
    picprozor^.state:=picprozor^.state and not (sfshadow);
    desktop^.insert(picprozor);

    if allowed then if scrwinvisible then begin
      load48k;
       move(uncompressed^[16384],picprozor^.slika^[0],6912);
       picprozor^.drawview;
    end;
    if not(allowed) then if scrwinvisible then begin Clive;
      picprozor^.drawview;
    end;

 end;
end;
begin
 
  oldexit:=exitproc;
  exitproc:=@err;
  
  preinit;
  MyApp.Init;


  if gmode and toemul then StartupMode:=$92;
  hourglascur;
  prerun;
 
  MyApp.adjustmenu;
 
  MyApp.Run;
  MyApp.Done;
  if (emuldir[length(emuldir)]='\') and (length(emuldir)>3) then begin
    dec(emuldir[0]);
    chdir(emuldir);
  end;
  if gmode then begin donevideo end;
  if toemul then c:=chain4 (curemul,'/c');
end.

