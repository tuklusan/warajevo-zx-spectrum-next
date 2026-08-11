{$V-,N+,E+,O+,F+}
unit konvtap;
interface
procedure Sendcommprogram;
function OutputFile(default,never:string):string;
procedure basicascii1 (var ulazni;uduzina:word);
procedure gensascii1(var ulazni;duzina:word);
procedure tswrdascii1(var ulazni;duzina:word;verzija:integer);
procedure forthascii1(var ulazni;duzina:word);
procedure tlwascii1(var ulazni;duzina:word);
procedure mlascii1(var ulazni;duzina:word);
procedure pasascii1(var ulazni;duzina:word);
procedure Cascii1(var ulazni;duzina:word; var saljijos:Boolean;
  filename:string);
procedure logoascii1(var ulazni;duzina:word; saljijos:Boolean;
  filename:string);
procedure screentiffbw1(var ulazni,duzina);
procedure screentiffcl1(var ulazni,duzina);
procedure asciibasic1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciigens1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciiml1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciiC1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciipas1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciilogo1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciiforth1(var izlazni;imedatoteke:string;var duzina:word);
procedure asciitlwtswrd1(var izlazni;imedatoteke:string;
  var duzina:word; verzija:integer);
procedure DoTzx(tzxname,tapename:String;buffer,izlbuffer:Pointer; var runsnap:Boolean);
procedure DoShalaWara(tapename:String;buffer:Pointer);
procedure DoZX32TapWara(tapename:String;buffer:Pointer);
procedure DoVoc(vocname,tapename:String;buffer,izlbuffer:Pointer);
procedure DoMuseumWara(museumname,tapename:string; buffer:Pointer);
function makevoc(vocname:string):Boolean;
function block2voc(vocname:string;buffer,izlbuffer:Pointer;poslatibajtova:word;
  freq:word):Boolean;
function finishvoc(vocname:string):Boolean;
function sample2voc(vocname:string;buffer,izlbuffer:Pointer;poslatibajtova:word;
  freq:word):Boolean;

procedure doadvset;
function getsampper(sm:Byte):Word;
const
  Casspars : record
    Leadermin : LongInt; {Inputlong}
    leadermax : LongInt; {Inputlong}
    synctotal : LongInt; {Inputlong}
    syncfirst : LongInt; {Inputlong}
    cdelay : LongInt; {Inputlong}
    MAXIMPULS : LongInt; {Inputlong}
    TRESHOLD : LongInt; {Inputlong}
    Soundbladdr : LongInt; {Inputlong}
  end=(Leadermin : $C6;leadermax : $9C;synctotal : $C9;
    syncfirst : $D4;cdelay : $16;maximpuls: $B2;treshold : $CB;
    Soundbladdr : 1);

  Tapestdset : record
    Sound : Word; {Checkboxes}
    errchk : Word; {Checkboxes}
    Speed : LongInt; {Inputlong}
    Device : Word; {RadioButtons}
  end=(Sound:1;errchk:1;speed:1500;Device:0);


implementation
uses setup,stddlg,app,views,dos,iorutine,msgbox,moucur,dialogs,drivers,hexedito,
objects,helpctx;

var fpozicija,kumuldecim:word; {Varijable za cuvanje registara, globalne radi lakseg pristupa}
  flastbit:byte;
  savebx,savecx,savees:word;

function OutputFile(default,never:string):string;
var a:PFileDialog;
  c:integer;
  fname:string;
begin
  a:=new(PFileDIalog,Init(default,'Select file name','~F~ile',
     fdOKButton,100));
  if desktop^.execview(a)<> cmcancel then begin
    a^.getfilename(fname);
    fname:=fexpand(fname);
    if fname=fexpand(never) then begin
      Displayerror ('Use another name, because it is active file',0);
      fname:='';
      exit;
    end;
    if (fileexists(fname)) and (fname<>default) then begin
      if messagebox(^C'Overwrite '+#13+chr(3)+fname+'?',
        nil,mfOKButton+mfCancelButton+mfconfirmation)<>cmok
        then fname:='';
    end;
  end
   else fname:='';
  outputfile:=fname;
  dispose(a,done);
end;


{ **********************************
   Basic to ASCII
  **********************************
}
const token:array[#128..#255] of string[10]=
  ('KEYWORDS','DEF PROC','PROC','END PROC','RENUM',
   'WINDOW','AUTO','DELETE','REF','JOIN','EDIT','KEYIN',
   'LOCAL','DEFAULT','DEF KEY','CSIZE','ALTER','BLANK',
   'CLOCK','DO','ELSE','FILL','GET','BLANK','EXIT IF',
   'WHILE','UNTIL','LOOP','SORT','ON ERROR','ON','DPOKE',
   'POP','ROLL','SCROLL','SPECTRUM','PLAY',

   'RND','INKEY$','PI','FN','POINT','SCREEN$','ATTR','AT','TAB',
   'VAL$','CODE','VAL','LEN','SIN','COS','TAN','ASN','ACS','ATN',
   'LN','EXP','INT','SQR','SGN','ABS',
   'PEEK','IN','USR','STR$','CHR$',
   'NOT','BIN','OR','AND','<=','>=','<>',
   'LINE','THEN','TO','STEP','DEF FN',
   'CAT','FORMAT','MOVE','ERASE',
   'OPEN #','CLOSE #','MERGE','VERIFY','BEEP',
   'CIRCLE','INK','PAPER','FLASH',
   'BRIGHT','INVERSE','OVER','OUT','LPRINT',
   'LLIST','STOP','READ','DATA','RESTORE',
   'NEW','BORDER','CONTINUE','DIM',
   'REM','FOR','GO TO','GO SUB',
   'INPUT','LOAD','LIST','LET','PAUSE','NEXT',
   'POKE','PRINT','PLOT','RUN','SAVE','RANDOMIZE',
   'IF','CLS','DRAW','CLEAR','RETURN','COPY');
  linktoken:array[#1..#12] of string[10]=
    ('DEFFN','GOTO','GOSUB','OPEN#','CLOSE#',
     'DEFPROC','ENDPROC','DEFKEY','EXITIF','ONERROR',
     'TRACE','USING');
  pastoken:array[#$81..#$A3] of string[10]=
    ('PROGRAM','DIV','CONST','PROCEDURE','FUNCTION','NOT','OR',
     'AND','MOD','VAR','OF','TO','DOWNTO','THEN','UNTIL',
     'END','DO','ELSE','REPEAT','CASE','WHILE',
     'FOR','IF','BEGIN','WITH','GOTO','SET','ARRAY','FORWARD',
     'RECORD','TYPE','IN','LABEL','NIL','PACKED');
  type cvtobyt=array[1..65520] of byte;


procedure basicascii1 (var ulazni;uduzina:word);
var
  izlazna:text;
  duzina,linbroj,pozicija:word;
  navodnik,lastspace:Boolean;
  kolorkod,attab1,attab2,visilin,nizilin,pp:byte;
  znak:char;
  dummy:byte;
  dumstr,dumstr2,dumstr3,filename:string;

  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;

  procedure whenexit;
  var cc:Boolean;
  begin
    displayerror('Incorrect BASIC input file',4029);
    cc:=fcloset(izlazna);
  end;
begin
  filename:=outputfile('BASASCII.TXT','');
  if filename='' then exit;
  assign(izlazna,filename);
  if not(frewritet(izlazna)) then exit;
  lastspace:=false;
  navodnik:=false;
  pozicija:=1;
  repeat
    if (pozicija>uduzina) then begin
      whenexit;
      exit
    end;
    visilin:=novibajt;
    if (pozicija>uduzina) then begin
        whenexit;
        exit
    end;
    nizilin:=novibajt;
    if visilin>63 then begin
      exit;
    end;
    linbroj:=256*visilin+nizilin;
    str(linbroj:4,dumstr);
    if not(fwrite(izlazna,dumstr)) then begin
      whenexit;
      exit
    end;
    if (pozicija>uduzina) then begin
        whenexit;
        exit
    end;
    duzina:=novibajt;
    if (pozicija>uduzina) then begin
        whenexit;
        exit
    end;
    duzina:=novibajt;

    repeat
      if (pozicija>uduzina) then begin
          whenexit;
          exit
      end;

      znak:=chr(novibajt);
      if not(navodnik) and (znak in [#128..#164]) then begin
        if not(lastspace) then begin
          if not(fwrite(izlazna,' '+token[znak]+' ')) then begin
            whenexit;
            exit
          end
        end
         else begin
           if not(fwrite(izlazna,token[znak]+' ')) then begin
             whenexit;
             exit
           end
         end
      end
       else case znak of
         #0..#5,#7,#10..#12,#24..#31: if not(fwrite(izlazna,'?')) then begin
           whenexit;
           exit
         end;
         #13: if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
         #14: for pp:=1 to 5 do begin
           if (pozicija>uduzina) then begin
               whenexit;
               exit
           end;
           dummy:=novibajt;
         end;
         #6: if not(fwrite(izlazna,'                ')) then begin
           whenexit;
           exit
         end;
         #8: if navodnik then
           if not(fwrite (izlazna,'"+CHR$ 8+"')) then begin
             whenexit;
             exit
         end;
         #9: ;
         #16..#21: begin
           if (pozicija>uduzina) then begin
               whenexit;
               exit
           end;

           kolorkod:=novibajt;
           if navodnik then begin
             str(ord(znak),dumstr);
             str(ord(kolorkod),dumstr2);
             if not(fwrite (izlazna,'"+ CHR$ '+dumstr+'+ CHR$ '+
              dumstr2+'+"')) then begin whenexit;exit end
           end
         end;
         #22,#23: begin
           if (pozicija>uduzina) then begin
               whenexit;
               exit
           end;

           attab1:=novibajt;
           if (pozicija>uduzina) then begin
               whenexit;
               exit
           end;

           attab2:=novibajt;
           if navodnik then begin
             str(ord(znak),dumstr);
             str(ord(attab1),dumstr2);
             str(ord(attab2),dumstr3);
             if not(fwrite (izlazna,'"+ CHR$ '+dumstr+'+ CHR$ '+
              dumstr2+'+ CHR$ '+dumstr3+'+"')) then begin whenexit;exit end;
           end;
         end;
         '"': begin
           if not(fwrite(izlazna,'"')) then begin whenexit; exit end;
           if navodnik then navodnik:=false else navodnik:=true;
         end;
         ' ','!','#'..'_','a'..'~':if not(fwrite(izlazna,znak)) then begin
           whenexit; exit
         end;
          #96: if not(fwrite(izlazna,'œ')) then begin whenexit; exit end;
         #127: if not(fwrite(izlazna,'é')) then begin whenexit; exit end;
         #128: if not(fwrite(izlazna,' ')) then begin whenexit; exit end;
         #143: if not(fwrite(izlazna,'Û')) then begin whenexit; exit end;
         #140: if not(fwrite(izlazna,'Ü')) then begin whenexit; exit end;
         #131: if not(fwrite(izlazna,'ß')) then begin whenexit; exit end;
         #133: if not(fwrite(izlazna,'Þ')) then begin whenexit; exit end;
         #138: if not(fwrite(izlazna,'Ý')) then begin whenexit; exit end;
         #129,#130,#132,#134,#135,#136,#137,#141,#142:
           if not(fwrite(izlazna,'þ')) then begin whenexit; exit end;
         #144..#164:if not(fwrite(izlazna, chr(ord(znak)-16))) then begin
           whenexit; exit
         end;
         #202..#210,#213..#255,#197,#198:
           if not(lastspace) then begin
             if not(fwrite(izlazna,' '+token[znak]+' ')) then begin
               whenexit; exit
             end
           end
            else begin
              if not(fwrite(izlazna,token[znak]+' ')) then begin
                whenexit; exit
              end
            end;
         #165..#167:   if not(fwrite(izlazna,token[znak])) then begin
           whenexit; exit
         end;
         #168..#196,#211,#212:
          if not(fwrite(izlazna,token[znak]+' ')) then begin
            whenexit; exit
          end;
         #199: if not(fwrite(izlazna,'<=')) then begin whenexit; exit end;
         #200: if not(fwrite(izlazna,'>=')) then begin whenexit; exit end;
         #201: if not(fwrite(izlazna,'<>')) then begin whenexit; exit end;
       end; {Case}
      if (znak in [' ',#6]) or (not(navodnik) and (znak in [#128..#164]))
        or (znak in ([#202..#255]-[#199,#200,#201])) then lastspace:=true
       else lastspace:=false;
    until znak=#13;
  until (pozicija>uduzina);
  if not(fcloset(izlazna)) then begin whenexit; exit; end;
end;


{ *********************************
    Gens to ASCII
  *********************************
}


procedure gensascii1(var ulazni;duzina:word);
var
  izlazna:text;
  linbroj,i:integer;
  pozicija:word;
  spejsova:byte;
  znak:char;
  filename:string;

  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;
begin
  filename:=outputfile('gensasci.txt','');
  if filename='' then exit;
  assign(izlazna,filename);
  if not(frewritet(izlazna)) then exit;
  pozicija:=1;
  while pozicija<=duzina do begin
    linbroj:=novibajt;
    if pozicija<=duzina then linbroj:=novibajt
     else begin
       whenexit;
       exit;
     end;
    znak:=' ';
    if pozicija<=duzina then while znak<>#13 do begin
      if pozicija>duzina then begin
         displayerror('Incorrect input file',4030);
         whenexit;
         exit
      end;
      znak:=chr(novibajt);
      if znak in [#9,#32..#127] then
        if not(fwrite(izlazna,znak)) then begin whenexit; exit; end;
    end;
    if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
  end;
  whenexit;
end;
{
  Tassword to ASCII
  verzija: 2
  verzija: 3
  ****************************
}


procedure tswrdascii1(var ulazni;duzina:word;verzija:integer);
var
  izlazna:text;
  linbroj,i:integer;
  pozicija:word;
  znak:char;
  slova64:byte;
  filename:string;

  function novibajt:byte;
  begin
    inc(slova64);
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  filename:=outputfile('tswrdasc.txt','');

  assign(izlazna,filename);
  if not(frewritet(izlazna)) then exit;
  pozicija:=1;
  slova64:=1;
  while pozicija<=duzina do begin
    znak:=' ';
    if pozicija<=duzina then while ((znak<>#13) and (verzija=2)) or
     ((znak<>#0) and (verzija=3)) do begin
      if pozicija>duzina then begin
        whenexit;
        exit
      end;
      znak:=chr(novibajt);
      if znak in [#9,#32..#127] then begin
        if not(fwrite(izlazna,znak)) then begin
          whenexit; exit
        end
      end;
      if ((slova64=65) and (verzija=2)) then begin
        if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
        slova64:=1
      end
    end;
    if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
    slova64:=1;
  end;
  whenexit;
end;

{ *****************************************
   Abersoft Forth to ASCII
  *****************************************
}

procedure forthascii1(var ulazni;duzina:word);
var
  izlazna:text;
  linbroj,i:integer;
  pozicija:word;
  znak:char;
  slova32:byte;
  filename:string;
  function novibajt:byte;
  begin
    inc(slova32);
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  filename:=outputfile('forthasc.txt','');

  assign(izlazna,filename);
  if not(frewritet(izlazna)) then exit;
  pozicija:=1;
  slova32:=1;
  while pozicija<=duzina do begin
    znak:=' ';
    if pozicija<=duzina then begin
      if pozicija>duzina then begin
        whenexit;
        exit
      end;
      znak:=chr(novibajt);
      if znak in [#32..#127] then begin
        if not(fwrite(izlazna,znak)) then begin whenexit; exit end;
      end
       else begin
         if not(fwrite(izlazna,'?')) then begin whenexit; exit end;
       end;
      if (slova32=33)  then begin
        if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
        slova32:=1
      end
    end;
  end;
  if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
  whenexit
end;

{********************************
  The last word to ASCII
 ********************************
}

procedure tlwascii1(var ulazni;duzina:word);
var
  izlazna:text;
  linbroj,i:integer;
  pozicija:word;
  znak:char;
  filename:string;

  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  filename:=outputfile('tlwascii.txt','');
  if filename='' then exit;

  assign(izlazna,filename);
  if not(frewritet(izlazna)) then begin whenexit; exit end;
  pozicija:=1;
  while pozicija<=duzina do begin
    znak:=' ';
    if pozicija<=duzina then while (znak<>#13) do begin
      if pozicija>duzina then begin
        whenexit;
        exit
      end;
      znak:=chr(novibajt);
      if znak in [#9,#32..#127] then begin
        if not(fwrite(izlazna,znak)) then begin whenexit; exit end;
      end;
      if (znak=#12) then begin
         if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
      end;
    end;
    if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
    if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
  end;
  whenexit
end;

{ *************************
  Machine lightning to ASCII
  *************************
}


procedure mlascii1(var ulazni;duzina:word);
var
  izlazna:text;
  linbroj,i:integer;
  pozicija:word;
  znak:char;
  filename:string;

  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  filename:=outputfile('mlascii.txt','');
  if filename='' then exit;

  assign(izlazna,filename);
  if not(frewritet(izlazna)) then begin whenexit; exit end;
  pozicija:=1;
  while pozicija<=duzina do begin
    znak:=' ';
    if pozicija<=duzina then while (znak<>#13)  do begin
      if pozicija>duzina then begin
        whenexit;
        exit
      end;
      znak:=chr(novibajt);
      if znak in [#32..#127] then
          if not(fwrite(izlazna,znak)) then begin whenexit; exit end;
      if znak=#$1C then
        if not(fwrite(izlazna,'               ')) then begin
          whenexit; exit
        end
    end;
    znak:=chr(novibajt);
    if znak=#$1f then if not(fwriteln(izlazna,'')) then begin
      whenexit; exit
    end;

    if znak=#$1A then begin
      if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
      whenexit;
      exit
    end;

  end;
  whenexit
end;

{ ***************************
   Hisoft Pascal to ASCII
  ***************************
}

procedure pasascii1(var ulazni;duzina:word);
var
  izlazna:text;
  linbroj,i:integer;spejsova:byte;
  znak:char;
  pozicija:word;
  filename:string;

  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;


begin
  filename:=outputfile('pasascii.txt','');
  if filename='' then exit;

  assign(izlazna,filename);
  pozicija:=1;

  if not(frewritet(izlazna)) then exit;
  while pozicija<=duzina do begin
    linbroj:=novibajt;
    if pozicija<=duzina then linbroj:=novibajt
     else begin
       whenexit;
       exit;
     end;
    if pozicija>duzina then begin
        Displayerror('Incorrect input file',4030);
        whenexit;
        exit
    end;
    spejsova:=novibajt;
    for i:=1 to spejsova do if not(fwrite(izlazna,' ')) then begin
      whenexit; exit
    end;
    znak:=' ';
    if pozicija<=duzina then while znak<>#13 do begin
      if pozicija>duzina then begin
         exit
      end;
      znak:=chr(novibajt);
      if znak in [#32..#128] then if not(fwrite(izlazna,znak)) then begin
         whenexit; exit
      end;

      if znak in [#$81..#$A3] then
        if not(fwrite(izlazna,pastoken[znak])) then begin
          whenexit; exit
        end;

    end;
    if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
  end;
  whenexit
end;

{
  ***********************
  Hisoft C to ASCII
  ***********************
}


procedure Cascii1(var ulazni;duzina:word; var saljijos:Boolean;
  filename:string);
var
  izlazna:text;
  pozicija,duzblok,i:word;
  nizi,visi:byte;
  znak:char;
  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  assign(izlazna,filename);
  if not(saljijos) then begin
    if not(frewritet(izlazna)) then begin whenexit; exit end
  end
   else append(izlazna);
  pozicija:=1;
  nizi:=novibajt;
  visi:=novibajt;
  duzblok:=word(nizi+256*visi);
  if (duzblok and $8000)=0 then saljijos:=true else saljijos:=false;
  duzblok:=duzblok and $7FFF;
  if duzblok+2>duzina then begin
     Displayerror('Incorrect input file',4030);
     whenexit;
     exit
  end;
  for i:=1 to duzblok do begin
    znak:=chr(novibajt);
    case znak of
      #29: if not(fwrite(izlazna,'<=')) then begin whenexit; exit end;
      #30: if not(fwrite(izlazna,'>=')) then begin whenexit; exit end;
      #31: if not(fwrite(izlazna,'!=')) then begin whenexit; exit end;
      #32..#127: if not(fwrite(izlazna,znak)) then begin whenexit; exit end;

      #10: begin
        if not(fwriteln(izlazna,''))then begin whenexit; exit end;
      end
      else if not(fwrite(izlazna,'?')) then begin whenexit; exit end;

    end
  end;
  whenexit;
end;

{ *************************
    Sinclair LOGO to ASCII
  *************************
}


procedure logoascii1(var ulazni;duzina:word; saljijos:Boolean;
  filename:string);
var
  izlazna:text;
  pozicija,duzblok,i:word;
  entera:byte;
  znak:char;
  function novibajt:byte;
  begin
    novibajt:=cvtobyt(ulazni)[pozicija];inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(izlazna);
  end;

begin
  entera:=0;
  assign(izlazna,filename);
  if not(saljijos) then begin
    if not(frewritet(izlazna)) then begin whenexit; exit end
  end
   else append(izlazna);
  pozicija:=1;
  for i:=1 to 128 do begin
    znak:=chr(novibajt);
    case znak of
      #0:  begin whenexit;saljijos:=false;entera:=0;exit end;
      #$D: begin
             inc(entera);
             if not(fwriteln(izlazna,'')) then begin whenexit; exit end;
             if entera=4 then begin whenexit;saljijos:=false;exit end;
      end;
      #32..#127:begin
           entera:=0;
           if not(fwrite(izlazna,znak)) then begin whenexit; exit end;
      end
      else if not(fwrite(izlazna,'?')) then begin whenexit; exit end;

    end
  end;
  whenexit;
  saljijos:=true;
end;

{
 *****************************
 SCREEN$ to B/W TIFF
 *****************************
}



procedure screentiffbw1(var ulazni,duzina);
const
  tiffhdr:array[0..$79] of byte=(
     $49,$49,                                            {Intel}
     $2A,$00,                                            {Version 5.0}
     $08,$00,$00,$00,                                    {IFD Offset}
     $09,$00,                                            {NoOfTags}
     $FE,$00, $04,$00, $01,$00,$00,$00, $00,$00,$00,$00, {NewSubfileType}
     $00,$01, $04,$00, $01,$00,$00,$00, $00,$01,$00,$00, {ImageWidth}
     $01,$01, $04,$00, $01,$00,$00,$00, $C0,$00,$00,$00, {ImageLength}
     $02,$01, $03,$00, $01,$00,$00,$00, $01,$00,$00,$00, {Bits per sample}
     $06,$01, $03,$00, $01,$00,$00,$00, $01,$00,$00,$00, {PhotometrocInterp}
     $11,$01, $04,$00, $01,$00,$00,$00, $7A,$00,$00,$00, {StripOffsets}
     $01,$00, $03,$00, $01,$00,$00,$00, $01,$00,$00,$00, {SamplesPerPixel}
     $16,$01, $04,$00, $01,$00,$00,$00, $C0,$00,$00,$00, {RowsPerStrip}
     $17,$01, $04,$00, $01,$00,$00,$00, $00,$18,$00,$00, {StripByteCount}
     $00,$00,$00,$00);                                   {IFD Offset}
var izlazna:file;
  trecina,karakter,unutar,adresa:word;
  filename:string;

  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(izlazna);
  end;


begin
  filename:=outputfile('bwtiff.tif','');
  if filename='' then exit;

  assign(izlazna,filename);
  if not(frewriteu(izlazna,1)) then exit;
  if not(fblockwrite(izlazna,tiffhdr,$7A)) then begin whenexit; exit end;
  for trecina:=0 to 2 do
    for karakter:=0 to 7 do
      for unutar:=0 to 7 do begin
        adresa:=trecina*2048+karakter*32+unutar*256;
        if not(fblockwrite(izlazna,cvtobyt(ulazni)[adresa+1],32)) then begin
          whenexit;
          exit
        end
      end;
  whenexit;
end;


{**************************
  SCREEN$ to color TIFF
 **************************
}


procedure screentiffcl1(var ulazni,duzina);
const
  tiffhdr:array[0..$E5] of byte=(
     $49,$49,                                            {Intel}
     $2A,$00,                                            {Version 5.0}
     $08,$00,$00,$00,                                    {IFD Offset}
     $0A,$00,                                            {NoOfTags}
     $FE,$00, $04,$00, $01,$00,$00,$00, $00,$00,$00,$00, {NewSubfileType}
     $00,$01, $04,$00, $01,$00,$00,$00, $00,$01,$00,$00, {ImageWidth}
     $01,$01, $04,$00, $01,$00,$00,$00, $C0,$00,$00,$00, {ImageLength}
     $02,$01, $03,$00, $01,$00,$00,$00, $04,$00,$00,$00, {Bits per sample}
     $06,$01, $03,$00, $01,$00,$00,$00, $03,$00,$00,$00, {PhotometricInterp}
     $11,$01, $04,$00, $01,$00,$00,$00, $E6,$00,$00,$00, {StripOffsets}
     $01,$00, $03,$00, $01,$00,$00,$00, $01,$00,$00,$00, {SamplesPerPixel}
     $16,$01, $04,$00, $01,$00,$00,$00, $C0,$00,$00,$00, {RowsPerStrip}
     $17,$01, $04,$00, $01,$00,$00,$00, $00,$60,$00,$00, {StripByteCount}
     $40,$01, $03,$00, $30,$00,$00,$00, $86,$00,$00,$00, {Color Map}
     $00,$00,$00,$00,                                   {IFD Offset}

     $00,$00,$80,$80,$00,$00,     {Kolor mapa}
     $80,$80,$00,$00,$00,$80,     {Nemam pojma da li RGB }
     $00,$00,$80,$80,$C0,$C0,     {komponente zauzimaju po}
     $FF,$FF,$00,$00,$FF,$FF,     {jedan ili dva bajta}
     $00,$00,$FF,$FF,$00,$00,
     $FF,$FF,$00,$00,$00,$00,
     $80,$80,$80,$80,$00,$00,
     $00,$00,$80,$80,$80,$80,
     $C0,$C0,$00,$00,$FF,$FF,
     $FF,$FF,$00,$00,$00,$00,
     $FF,$FF,$FF,$FF,$00,$00,
     $00,$00,$00,$00,$00,$00,
     $80,$80,$80,$80,$80,$80,
     $80,$80,$C0,$C0,$00,$00,
     $00,$00,$00,$00,$FF,$FF,
     $FF,$FF,$FF,$FF,$FF,$FF);

     {Paznja ovo je paleta koju mi je izvezao Corel. Nesto se razlikuje
     od ocekivanog}

     boja:array[0..15] of byte=
       (0,4,1,5,2,6,3,7,0,12,9,13,10,14,11,15);

     maskaravan:array[0..3] of byte=(8,4,2,1);
     tackaravan:array[0..7] of byte=(128,64,32,16,8,4,2,1);

var izlazna:file;
  trecina,karakter,unutar,uredu,adresa:word;
  bitmapa,atribut,salji,paper,ink,tacka:byte;
  filename:string;

  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(izlazna);
  end;

begin
  filename:=outputfile('colortif.tif','');
  if filename='' then exit;

  assign(izlazna,filename);
  if not(frewriteu(izlazna,1)) then exit;
  if not(fblockwrite(izlazna,tiffhdr,$E6)) then begin whenexit; exit end;
    for trecina:=0 to 2 do
      for karakter:=0 to 7 do
        for unutar:=0 to 7 do
          for uredu:=0 to 31 do begin
            adresa:=trecina*2048+karakter*32+unutar*256+uredu;
            bitmapa:=cvtobyt(ulazni)[adresa+1];
            adresa:=6144+(trecina*8+karakter)*32+uredu;
            atribut:=cvtobyt(ulazni)[adresa+1];
            atribut:=atribut and $7F;
            paper:=(atribut and $38) div 8;
            ink:=atribut and 7;
            if atribut>63 then begin
              paper:=paper+8;
              ink:=ink+8;
            end;
            for tacka:=0 to 3 do begin
              if (tackaravan[2*tacka] and bitmapa)=0 then
                salji:=boja[paper]
               else
                 salji:=boja[ink];
              if (tackaravan[2*tacka+1] and bitmapa)=0 then
                salji:=boja[paper]+16*salji
               else
                 salji:=boja[ink]+16*salji;
              if not(fblockwrite(izlazna,salji,1)) then begin
                whenexit; exit
              end
            end
          end;
  whenexit;
end;


{ *************************************
    ASCII to BASIC
  *************************************
}


function bint (x:real):integer;
begin
  {$r-}
  if x>=0 then bint:=trunc(x) else bint:=trunc(x)-1
  {$r+}
end;
function checktoken(linija:string;pozicija:integer;var imaspejs:Boolean):char;
var i,tmp:char;
f:integer;substr:string;
begin
  tmp:=#0;
  if linija[pozicija-1] in ['a'..'z','A'..'Z'] then begin
    checktoken:=#0;
    exit
  end;
  for i:=#128 to #255 do begin
    if pozicija+length(token[i])<=length(linija) then
      if not(linija[pozicija+length(token[i])]
       in ['a'..'z','A'..'Z']) then begin
        substr:=copy(linija,pozicija,length(token[i]));
        for f:=1 to length(substr) do substr[f]:=upcase(substr[f]);
        if token[i]=substr  then begin
          tmp:=i;
          checktoken:=tmp;
          imaspejs:=true;
          exit
        end;
      end;
    end;
  for i:=#1 to #12 do begin
    if pozicija+length(linktoken[i])<=length(linija) then
      if not(linija[pozicija+length(linktoken[i])]
       in ['a'..'z','A'..'Z']) then begin
        substr:=copy(linija,pozicija,length(linktoken[i]));
        for f:=1 to length(substr) do substr[f]:=upcase(substr[f]);
        if linktoken[i]=substr  then begin
          tmp:=i;
          imaspejs:=false;
          if i>=#11 then imaspejs:=true;
        end;
      end;
    end;
    case tmp of
      #1: tmp:=#206;
      #2: tmp:=#236;
      #3: tmp:=#237;
      #4: tmp:=#211;
      #5: tmp:=#212;
      #6: tmp:=#129;
      #7: tmp:=#131;
      #8: tmp:=#142;
      #9: tmp:=#152;
      #10: tmp:=#157;
      #11: tmp:=#163;
      #12: tmp:=#164;

    end;
    checktoken:=tmp;
end;

procedure asciibasic1(var izlazni;imedatoteke:string;var duzina:word);
label broj,rbr;
var
  datoteka:text;
  dzlinije,ppozicija:word;
  linija,izllinija,linbroj,evltr:string;
  p,i,vrlin,pozicija,dummy,e:integer;
  navodnik,imaspejs,deffnfleg,bin:Boolean;
  nadjenitoken:char;
  realbr,y,m:double;
  realni:array[1..5] of byte;
  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[ppozicija]:=bajt;inc(ppozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  bin:=false;
  deffnfleg:=false;
  ppozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresett (datoteka)) then exit;
  while not eof (datoteka) do begin
    if not(freadln(datoteka,linija)) then begin whenexit;exit end;
    linija:=linija+' ';
    navodnik:=false;
    izllinija:='';
    pozicija:=0;
    repeat
      inc(pozicija);
    until linija[pozicija]<>' ';
    if not(linija[pozicija] in [' ','0'..'9']) then begin
      displayerror ('Line not begins with line number',0);
      whenexit;
      exit
    end;
    linbroj:='';
    while linija[pozicija] in ['0'..'9'] do begin
      linbroj:=linbroj+linija[pozicija];
      inc(pozicija);
    end;
    if linija[pozicija]=' ' then inc(pozicija);
    val (linbroj,vrlin,dummy);
    if vrlin>9999 then begin
      displayerror('Line number too big',4027);
      whenexit;
      exit;
    end;
    while pozicija <=length(linija) do begin
      if not(linija[pozicija] in ['0'..'9',' ']) then bin:=false;
      case linija[pozicija] of
        'œ':izllinija:=izllinija+#96;
        'é':izllinija:=izllinija+#127;
        'Û':izllinija:=izllinija+#143;
        'Ü':izllinija:=izllinija+#140;
        'ß':izllinija:=izllinija+#131;
        'Þ':izllinija:=izllinija+#133;
        'Ý':izllinija:=izllinija+#138;
        '"':begin
          izllinija:=izllinija+'"';
          if navodnik then navodnik:=false else navodnik:=true;
        end;
        ',':begin
           if deffnfleg then izllinija:=izllinija+chr(14)+chr(32)+chr(32)+
              chr(32)+chr(32)+chr(32);
           izllinija:=izllinija+',';
        end;
        ')':begin
           if deffnfleg then begin
             izllinija:=izllinija+chr(14)+chr(32)+chr(32)+
               chr(32)+chr(32)+chr(32);
             deffnfleg:=false;
           end;
           izllinija:=izllinija+')'
        end;
        ' ':if (pozicija<length(linija)) and (not(deffnfleg)) then
           izllinija:=izllinija+' ';
        '!','#'..'(','*'..'+','-'..'/',':',';','=','?','@','['..'_',
          '{'..'~': izllinija:=izllinija+linija[pozicija];
        '>': begin
          if pozicija<length(linija) then begin
            inc (pozicija);
            if linija[pozicija]='=' then izllinija:=izllinija+#200
             else begin
               dec(pozicija);
               izllinija:=izllinija+'>'
             end
          end;
        end;
        '<': begin
          if pozicija<length(linija) then begin
            inc (pozicija);
            if linija[pozicija]='=' then izllinija:=izllinija+#199
             else if linija[pozicija]='>' then izllinija:=izllinija+#201
               else begin
                 dec(pozicija);
                 izllinija:=izllinija+'<'
               end
            end;
         end;
        'a'..'z','A'..'Z': begin
          if not(navodnik) then
            nadjenitoken:=checktoken(linija,pozicija,imaspejs)
           else nadjenitoken:=#0;
          if nadjenitoken=#196 then begin
            bin:=true;
          end;
          if nadjenitoken=#0 then izllinija:=izllinija+linija[pozicija]
           else begin
             izllinija:=izllinija+nadjenitoken;
             pozicija:=pozicija+length(token[nadjenitoken]);
             if not(imaspejs) then dec(pozicija);
             if linija[pozicija]<>' ' then dec(pozicija);
             if nadjenitoken=#206 then deffnfleg:=true;
           end;
         end;
         '0'..'9': begin
           if (izllinija[length(izllinija)] in ['a'..'z','A'..'Z'])
             or navodnik then begin
               while linija[pozicija] in ['0'..'9'] do begin
                 izllinija:=izllinija+linija[pozicija];
                 inc(pozicija);
               end;
               dec(pozicija);
             end
            else begin
              if bin then begin
                realbr:=0;
                while linija[pozicija]in['0'..'1'] do begin
                  izllinija:=izllinija+linija[pozicija];
                  realbr:=realbr*2+ord(linija[pozicija])-48;
                  inc(pozicija);
                  if pozicija>length(linija) then begin
                    bin:=false;
                    goto rbr;
                  end;
                end;
                bin:=false;
                goto rbr;
              end;
              if linija[pozicija-1]='.' then evltr:='0.' else evltr:='';
              while linija[pozicija]in['0'..'9'] do begin
                izllinija:=izllinija+linija[pozicija];
                evltr:=evltr+linija[pozicija];
                inc(pozicija);
                if pozicija>length(linija) then goto broj;
              end;
              case linija[pozicija] of
                '.': begin
                  izllinija:=izllinija+linija[pozicija];
                  inc(pozicija);
                  if pozicija>length(linija) then goto broj;
                  if linija[pozicija] in ['0'..'9'] then evltr:=evltr+'.';
                  while linija[pozicija]in['0'..'9'] do begin
                    izllinija:=izllinija+linija[pozicija];
                    evltr:=evltr+linija[pozicija];
                    inc(pozicija);
                    if pozicija>length(linija) then goto broj;
                  end;
                  if linija[pozicija] in ['e','E'] then begin
                    izllinija:=izllinija+linija[pozicija];
                    evltr:=evltr+linija[pozicija];
                    inc(pozicija);
                    if pozicija>length(linija) then goto broj;
                    if linija[pozicija] in ['+','-'] then begin
                      izllinija:=izllinija+linija[pozicija];
                      evltr:=evltr+linija[pozicija];
                      inc(pozicija);
                      if pozicija>length(linija) then goto broj;
                    end;
                    while linija[pozicija]in['0'..'9'] do begin
                      izllinija:=izllinija+linija[pozicija];
                      evltr:=evltr+linija[pozicija];
                      inc(pozicija);
                      if pozicija>length(linija) then goto broj;
                    end;
                  end;
                end;
                'e','E': begin
                  izllinija:=izllinija+linija[pozicija];
                  evltr:=evltr+linija[pozicija];
                  inc(pozicija);
                  if pozicija>length(linija) then goto broj;
                  if linija[pozicija] in ['+','-'] then begin
                    izllinija:=izllinija+linija[pozicija];
                    evltr:=evltr+linija[pozicija];
                    inc(pozicija);
                    if pozicija>length(linija) then goto broj;
                  end;
                  while linija[pozicija]in['0'..'9'] do begin
                    izllinija:=izllinija+linija[pozicija];
                    evltr:=evltr+linija[pozicija];
                    inc(pozicija);
                    if pozicija>length(linija) then goto broj;
                  end;
                end;
              end; {case}

      broj:   dec(pozicija);
              val(evltr,realbr,dummy);
              if dummy<>0 then begin
                displayerror ('Incorrect real number ',4028);
                whenexit;
                exit
              end;
     rbr:     if not((frac(realbr)=0) and (realbr<65535)
                 and (realbr>-65535)) then begin
                y:=ln(abs(realbr))/(ln(2));
                e:=bint(y)+1;

                {$r-}
                realni[1]:=e+128;
                m:=ln(realbr)-(e)*ln(2);
                if m>-85 then m:=exp(m) else m:=0;
                while m>=1 do begin
                  m:=m/2;
                  inc(realni[1]);
                  inc(e);
                end;
                for i:=2 to 5 do begin
                  realni[i]:=bint(256*m);
                  m:=256*m-realni[i]
                end;
                if realbr>0 then realni[2]:=realni[2] and $7f;
                {$r+}
              end
               else begin
                 if realbr=0 then for i:=1 to 5 do realni[i]:=0;
                 if realbr>0 then begin
                   realni[1]:=0;
                   realni[2]:=0;
                   realni[3]:=bint(256*frac(realbr/256));
                   realni[4]:=bint(realbr/256);
                   realni[5]:=0
                 end;
                 if realbr<0 then begin
                   realni[1]:=0;
                   realni[2]:=255;
                   realni[3]:=255-bint(256*frac(abs(realbr)/256));
                   realni[4]:=255-bint(abs(realbr)/256);
                   realni[5]:=0
                 end;
               end;
              izllinija:=izllinija+#14+chr(realni[1])+
                chr(realni[2])+chr(realni[3])+
                chr(realni[4])+chr(realni[5])
            end {else}
        end;
        #128..#148: izllinija:=izllinija+chr(ord(linija[pozicija])+16);
      end; {case}
      inc(pozicija);
    end;
    dzlinije:=length(izllinija)+1;
    izllinija:=chr(hi(vrlin))+chr(lo(vrlin))+
      chr(lo(dzlinije))+chr(hi(dzlinije))+izllinija+chr(13);
    for p:=1 to length(izllinija) do saljibajt(ord(izllinija[p]))
  end;
  if not(fcloset(datoteka)) then begin whenexit;exit; end;
  duzina:=ppozicija-1;
end;

{ *************************************
  ASCII to Pascal
  *************************************
}


function checkpastoken(linija:string;pozicija:integer):char;
var i,tmp:char;
f:integer;substr:string;
begin
  if linija[pozicija-1] in ['a'..'z','A'..'Z'] then begin
    checkpastoken:=#0;
    exit
  end;
  for i:=#$81 to #$A3 do begin
    if pozicija+length(pastoken[i])<=length(linija) then
      if not(linija[pozicija+length(pastoken[i])]
       in ['a'..'z','A'..'Z']) then begin
        substr:=copy(linija,pozicija,length(pastoken[i]));
        for f:=1 to length(substr) do substr[f]:=upcase(substr[f]);
        if pastoken[i]=substr  then begin
          checkpastoken:=i;
          exit
        end;
      end;
    end;
  checkpastoken:=#0;
end;

procedure asciipas1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:text;
  pozicija:word;
  izlazna:file;
  linija,izllinija:string;
  q,brlinije,i:integer;
  spejsova:byte;
  nadjenitoken:char;
  navodnik:boolean;
  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  assign(datoteka,imedatoteke);
  if not(fresett(datoteka)) then exit;
  brlinije:=0;
  navodnik:=false;
  pozicija:=1;
  while not eof (datoteka) do begin
    if not(freadln(datoteka,linija)) then begin whenexit; exit end;
    linija:=' '+linija+' ';
    inc(brlinije,10);
    i:=1;spejsova:=0;
    izllinija:='';
    while (linija[i]=' ') and (i<=length(linija)) do begin
      inc(spejsova);
      inc(i)
    end;
    izllinija:=izllinija+chr(lo(brlinije))+chr(hi(brlinije))+chr(spejsova);
    while i<=length(linija) do begin
      if (linija[i]>#$A3) or (linija[i]<' ') then begin
        izllinija:=izllinija+'?';
        inc(i)
      end
       else begin
         if linija[i] in ['a'..'z','A'..'Z'] then
           nadjenitoken:=checkpastoken(linija,i)
          else nadjenitoken:=#0;
         if nadjenitoken=#0 then begin
           if linija[i]='''' then navodnik:=not(navodnik);
           if navodnik then izllinija:=izllinija+linija[i]
            else izllinija:=izllinija+upcase(linija[i]);
         end
          else begin
            izllinija:=izllinija+nadjenitoken;
            i:=i+length(pastoken[nadjenitoken]);
            dec(i);
          end;
         inc(i);
      end
    end;
    izllinija:=izllinija+chr(13);
    for q:=1 to length(izllinija) do saljibajt(ord(izllinija[q]));
  end;
  whenexit;
  duzina:=pozicija-1;
end;


{***********************************
  ASCII to GENS
 ***********************************
}


procedure asciigens1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:text;
  izlazna:file;
  linija,izllinija:string;
  p,brlinije,i:integer;
  spejsova:byte;
  nadjenitoken:char;
  pozicija:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresett (datoteka)) then exit;
  brlinije:=0;
  while not eof (datoteka) do begin
    if not(freadln(datoteka,linija)) then begin whenexit; exit end;
    inc(brlinije,10);
    i:=1;spejsova:=0;
    while (linija[i]=' ') and (i<=length(linija)) do begin
      inc(spejsova);
      inc(i)
    end;

    izllinija:='';
    izllinija:=izllinija+chr(lo(brlinije))+chr(hi(brlinije));
    if spejsova<>0 then izllinija:=izllinija+chr(9);
    while i<=length(linija) do begin
      case linija[i] of
        ' ',#9: izllinija:=izllinija+chr(9);
        #33..#128: izllinija:=izllinija+linija[i];
      end;
      inc(i);
    end;
    izllinija:=izllinija+chr(13);
    for p:=1 to length(izllinija) do saljibajt(ord(izllinija[p]));
  end;
  whenexit;
  duzina:=pozicija-1;
end;

{ ****************************
  ASCII to Machine lightning
  ****************************
}
procedure asciiml1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:text;
  izlazna:file;
  linija,izllinija:string;
  p,brlinije,i:integer;
  spejsova:byte;
  pozicija:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresett (datoteka)) then exit;
  brlinije:=0;
  while not eof (datoteka) do begin
    if not(freadln(datoteka,linija)) then begin whenexit;exit; end;
    izllinija:='';
    i:=1;
    while i<=length(linija) do begin
      case linija[i] of
        ' ',#9: izllinija:=izllinija+chr($1C);
        #33..#128: izllinija:=izllinija+linija[i];
      end;
      inc(i);
    end;
    izllinija:=izllinija+chr(13)+chr($1F);
    for p:=1 to length(izllinija) do saljibajt(ord(izllinija[p]));
  end;
  whenexit;
  saljibajt($1A);
  duzina:=i-1;
end;

{ **************************
   Ascii to C
  **************************
}


procedure asciiC1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:file;
  bafer:array[1..512] of byte;
  i,pozicija,duzblock:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresetu (datoteka,1)) then exit;
  while not(eof(datoteka)) do begin
    if filesize(datoteka)-filepos(datoteka)>512 then begin
      if not(fblockread(datoteka,bafer,512)) then begin whenexit;exit; end;
      duzblock:=512;
      saljibajt(lo(duzblock));
      saljibajt(hi(duzblock));
    end
     else begin
       duzblock:=filesize(datoteka)-filepos(datoteka);
       if not(fblockread(datoteka,bafer,duzblock)) then begin
         whenexit;
         exit
       end;
       duzblock:=duzblock+32768;
       saljibajt(lo(duzblock));
       saljibajt(hi(duzblock));
       duzblock:=duzblock-32768;
     end;
    for i:=1 to duzblock do
      if bafer[i]<>13 then saljibajt(bafer[i]) else saljibajt(32);
    if duzblock<512 then for i:=duzblock+1 to 512 do saljibajt(32)
  end;
  duzina:=pozicija;
  whenexit;
end;

{ ***********************************
  ASCII to Logo
  ***********************************
}

procedure asciilogo1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:file;
  bajt: byte;
  i,pozicija,duzblock:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fclose(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresetu (datoteka,1)) then exit;
  while not(eof(datoteka)) do begin
    if not(fblockread(datoteka,bajt,1)) then begin whenexit; exit end;
    if bajt<>10 then saljibajt(bajt);
  end;
  saljibajt(13);
  saljibajt(13);
  saljibajt(13);
  duzina:=pozicija-1;
  for i:=1 to 128 do saljibajt(0);
  whenexit;
end;


{ ***********************************
   Ascii to Abersoft forth
  ***********************************
}


procedure asciiforth1(var izlazni;imedatoteke:string;var duzina:word);
var datoteka:text;
  izlazna:file;
  linija,izllinija,dummy:string;
  p,brlinije,i:integer;
  spejsova:byte;
  pozicija:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresett (datoteka)) then exit;
  brlinije:=0;
  for i:=1 to 1024 do saljibajt(32);
  while not eof (datoteka) do begin
    if not(freadln(datoteka,linija)) then begin whenexit; exit end;
    izllinija:='';
    i:=1;
    while i<=length(linija) do begin
      if (linija[i]>=' ') and (linija[i]<='~') then
        izllinija:=izllinija+linija[i]
       else izllinija:=izllinija+' ';
      inc(i);
    end;
    for p:=1 to length(izllinija) do saljibajt(ord(izllinija[p]));
  end;
  whenexit;
  duzina:=pozicija-1;
  if duzina>11263 then begin
    str(duzina,dummy);
    displayerror('Source file too long #'+dummy+' bytes',4031)
  end;
  for i:=duzina to 10240 do saljibajt(32);
  for i:=10240 to 11263 do saljibajt(0);
  duzina:=11263;
end;


{*****************************
  ASCII to Tassword 2, 3 &
    The last word
******************************
}


procedure asciitlwtswrd1(var izlazni;imedatoteke:string;
  var duzina:word; verzija:integer);
var datoteka:text;
  izlazna:file;
  linija,izllinija:string;
  p,brlinije,i:integer;
  spejsova:byte;
  pozicija:word;

  procedure saljibajt(bajt:byte);
  begin
    cvtobyt(izlazni)[pozicija]:=bajt;inc(pozicija);
  end;
  procedure whenexit;
  var cc:Boolean;
  begin
    cc:=fcloset(datoteka)
  end;

begin
  pozicija:=1;
  assign(datoteka,imedatoteke);
  if not(fresett(datoteka)) then exit;
  brlinije:=0;
  while not eof (datoteka) do begin
    i:=1;
    if not(freadln(datoteka,linija)) then begin whenexit;exit end;
    izllinija:='';

    while i<=length(linija) do begin
      if (linija[i]>=' ') and (linija[i]<='~') then
        izllinija:=izllinija+linija[i]
       else izllinija:=izllinija+' ';
      case verzija of
        1: if i mod 40=0 then izllinija:=izllinija+#14;
        3: if i mod 64=0 then izllinija:=izllinija+#0
      end;
      inc(i);
    end;
    case verzija of
        1,2: izllinija:=izllinija+#13;
        3: izllinija:=izllinija+#0+#0
    end;
    for p:=1 to length(izllinija) do saljibajt(ord(izllinija[p]));
  end;
  whenexit;
  duzina:=pozicija-1;
end;

const sampspeed=79;  {170 optimum}

type konvtip=array[0..65520] of byte;
var samp:byte;
procedure setsampperiod(sampleper:word);
begin
  case sampleper of
   233: samp:=0;
   150..170: samp:=1;
   116: samp:=2;
   70..90: samp:=3;
   else samp:=1;
  end;
end;
function getsampper(sm:Byte):Word;
begin
  case (sm and $18) shr 3 of
    $0: getsampper:=233;   {15000 Hz}
    $1: getsampper:=159;   {22050 Hz}
    $2: getsampper:=116;   {30303 Hz}
    $3: getsampper:=79;    {44100 Hz}
  end;
end;
type TPulseType=(zero,one,zerotoone,onetozero);

procedure DoTzx(tzxname,tapename:String;buffer,izlbuffer:Pointer; var runsnap:Boolean);
  var filename:String;
  dat:file;
  header:record
    signatura:array[1..7] of char;
    eofsign:byte;
    majorver,minorver:byte;
  end;
  id,fleg,paritet:byte;
  prebuffer:array[0..40] of byte;
  curpos,relative,datalen:longint;
  cc,forstart,sampled:Boolean;
  duzina,i,tzxopt:word;
  duzinadir:longint;
  fd:PFileDialog;
  pozicija:word;
  lastbit:byte;
  amp:TPulseType;
  grupped,skipping:Boolean;
  snapdat:file;
  pilot,sb_pilot,sb_pulse,
  sb_bit0,sb_bit1,sb_bit,lastbyte,pause,
  datapos,sb_sync1,sb_sync2,dfreq:Word;
  bitcount,databyte,dflag:byte;
  repper,resttoskip:word; reppos:longint;

  {$R-}

  procedure toggleamp;
  begin
    if amp=zero then amp:=one else amp:=zero
  end;

  function dvobajt(b1:byte):word;
  var r:word;
  begin
    move(prebuffer[b1],r,2);
    dvobajt:=r;
  end;
  function trobajt(b1:byte):longint;
  var r:longint;
  begin
    move(prebuffer[b1],r,3);
    r:=r and $00FFFFFF;
    trobajt:=r;
  end;
  function cetvorobajt(b1:byte):longint;
  var r:longint;
  begin
    move(prebuffer[b1],r,4);
    cetvorobajt:=r;
  end;

  {$R+}
  procedure we;
  begin
    cc:=fclose(dat);
  end;
  procedure flushsample;
  var flag:byte;
  lbunus:word;
  begin
    lbunus:=pozicija-6;
    if lastbit=0 then begin
      lastbit:=7;
    end
      else begin dec(lastbit); inc(lbunus) end;
    
    move (lbunus,konvtip(izlbuffer^)[0],2);
    move (lbunus,konvtip(izlbuffer^)[2],2);
    flag:=lastbit or (samp shl 3);
    if not(save(true,tapename,65534,4,flag,izlbuffer^)) then begin
      we;exit
    end;
    pozicija:=6;
    lastbit:=7;
    setsampperiod(sampspeed); {158}
    sampled:=false;
  end;

  procedure testoverflow;
  begin
    if pozicija>65000 then flushsample;
    sampled:=true;
  end;



procedure playpulse(izlbuffer:Pointer;pulselen:word;
  var pozicija:word;var lastbit:byte;pulsetype:TPulsetype);

label zeroloop,oneloop,skipzero,skipone,krajasm,onepulse,fillpulse;

begin
  asm
            mov ax,pulselen
            xor dx,dx
            mov cx,sampspeed
            div cx

            add kumuldecim,dx
            cmp kumuldecim,sampspeed
            jb fillpulse
            sub kumuldecim,sampspeed
            inc ax

fillpulse:  les si,pozicija
            mov di,es:[si]
            les si,lastbit
            mov dl,es:[si]
            les bx,izlbuffer

            mov dh,01111111b
            mov cl,dl
            ror dh,cl

            mov cx,ax
            mov al,PulseType
            cmp al,1
            je onepulse

zeroloop:   and es:[bx+di],dh
            inc dl
            ror dh,1
            jc skipzero
            inc di
            sub dl,dl
            cmp di,65000
            jbe skipzero
            mov fpozicija,di
            mov flastbit,dl
            mov savebx,bx
            mov savecx,cx
            mov savees,es
  end;
  pozicija:=fpozicija;
  lastbit:=flastbit;
  flushsample;
  asm
            mov es,savees
            mov cx,savecx
            mov bx,savebx
            mov di,6
            mov dl,0
            mov dh,01111111b
skipzero:   loop zeroloop
            jmp krajasm

onepulse:   not dh
oneloop:    or es:[bx+di],dh
            inc dl
            ror dh,1
            jnc skipone
            inc di
            sub dl,dl
            cmp di,65000
            jbe skipone
            mov fpozicija,di
            mov flastbit,dl
            mov savebx,bx
            mov savecx,cx
            mov savees,es
  end;
  pozicija:=fpozicija;
  lastbit:=flastbit;
  flushsample;
  asm
           mov es,savees
           mov cx,savecx
           mov bx,savebx
           mov di,6
           mov dl,0
            mov dh,10000000b
skipone:    loop oneloop

krajasm:    mov ax,di
            sub ax,5
            mov es:[bx],ax
            mov es:[bx+2],ax

            les si,pozicija
            mov es:[si],di
            les si,lastbit
            mov es:[si],dl

  end;
  sampled:=true;
end;


  procedure saveblock;
  begin
    if sampled then flushsample;
    if not(fblockread(dat,fleg,1)) then begin we;exit;end;
    if not(fblockread(dat,buffer^,duzina-2))then begin we;exit;end;
    if not(fblockread(dat,paritet,1)) then begin we;exit;end;
    if not(save(true,tapename,duzina-2,4,fleg,buffer^)) then begin
      we;exit
    end;
  end;

  procedure playpause;
  begin
    if (pause<>0) then begin
      playpulse(izlbuffer,3500,pozicija,lastbit,amp);
      amp:=zero;
      if pause>1 then begin
        while pause>1 do begin
          playpulse(izlbuffer,3500,pozicija,lastbit,amp);
          dec(pause);
        end;
      end;
{      lastbit:=0;}
    end;
  end;

  procedure sampleblck;
  begin

    while (pilot<>0) do begin
      playpulse(izlbuffer,sb_pilot,pozicija,lastbit,Amp);
      ToggleAmp; Dec(pilot);
    end;
    if (sb_sync1<>0) then begin
      playpulse(izlbuffer,sb_sync1,pozicija,lastbit,Amp);
      ToggleAmp;
    end;
    if (sb_sync2<>0) then begin
      playpulse(izlbuffer,sb_sync2,pozicija,lastbit,Amp);
      ToggleAmp;
    end;
    datapos:=0;

    while (datalen<>0) do begin
      if not(sampled) then sampled:=true;
      if (datalen<>1) then bitcount:=8 else bitcount:=lastbyte;
      databyte:=Konvtip(buffer^)[datapos];
      while (bitcount<>0) do begin
        if (databyte and $80)<>0 then sb_bit:=sb_bit1
           else sb_bit:=sb_bit0;
         playpulse(izlbuffer,sb_bit,pozicija,lastbit,amp);
         ToggleAmp;
         playpulse(izlbuffer,sb_bit,pozicija,lastbit,amp);
         ToggleAmp;
         {$R-}
 	 databyte:=databyte shl 1;
         {$R+}
	 dec(bitcount);
      end;
      dec(datalen);inc(datapos);
      if datapos=65500 then begin
        if datalen<65500 then begin
          if not(fblockread(dat,buffer^,datalen))then begin we;exit;end;
        end
         else begin
           if not(fblockread(dat,buffer^,65500))then begin we;exit;end;
         end;
{        flushsample;}
        datapos:=0;
      end;
    end;
  end;

  begin
    sampled:=false;
    duzina:=0;
    grupped:=false;
    pozicija:=6;
    kumuldecim:=0;
    lastbit:=0;
    Amp:=zero;
    runsnap:=false;
    skipping:=false;
    resttoskip:=0;
    tzxopt:=Setup.Tzx.Checkboxes;
    setsampperiod(sampspeed);  {158}
    forstart:=tzxname<>'';
    if not(forstart) then begin
      fd:=new(PFileDIalog,Init('*.TZX','Select TZX file to convert','~N~ame',
        fdOpenButton,100));
      fd^.helpctx:=hctzxwara;
      if desktop^.execview(fd)<> cmcancel then begin
        hourglascur;
        fd^.getfilename(filename);
        filename:=fexpand(filename);
        if not(fileexists(filename)) then begin dispose(fd,done); exit end;
      end
       else begin dispose(fd,done); exit end;
      dispose(fd,done);
    end
     else filename:=tzxname;
    assign(dat,filename);
    if not(fresetu(dat,1)) then begin we;exit;end;

    if not(fblockread(dat,header,10)) then begin we; exit end;
    if (tzxopt and 256 =0) then grupped:=true;
    while not eof(dat) do begin
      if not(fblockread(dat,id,1)) then begin we; exit end;;
      curpos:=FilePos(dat);
      if skipping then begin
        dec(resttoskip);
        if resttoskip=0 then skipping:=false;
      end;
      case id of
        $10: begin {Standard}
          if not(fblockread(dat,prebuffer,4)) then begin we; exit end;
          if not(skipping) then begin
            duzina:=dvobajt(2);
            sb_pilot:=2168;
	    sb_sync1:=667;
	    sb_sync2:=735;
	    sb_bit0:=855;
	    sb_bit1:=1710;
	    pilot:=3220;  {8064}
	    lastbyte:=8;
	    datalen:=duzina;
            pause:=dvobajt(0);
            if tzxopt and 1 =0 then begin
              saveblock;
              if (tzxopt and 2)<>0 then if pause<>0 then begin
                playpause;
                if not(grupped) then flushsample;
              end;
            end
            else begin
              if not(fblockread(dat,buffer^,duzina))then begin we;exit;end;
              if konvtip(buffer^)[0]=0 then pilot:=8064;
              sampleblck;
              if (tzxopt and 2)<>0 then playpause;
              if not(grupped) then flushsample;
            end;
          end;
          relative:=dvobajt(2)+4
        end;

        $11: begin {Turbo}
          if not(fblockread(dat,prebuffer,$12)) then begin we; exit end;;
          if not(skipping) then begin
            if not(grupped) then amp:=zero;
            duzina:=dvobajt($F);  {Dugi blokovi, razmisli}
            sb_pilot:=dvobajt(0);
	    sb_sync1:=dvobajt(2);
	    sb_sync2:=dvobajt(4);
	    sb_bit0:=dvobajt(6);
	    sb_bit1:=dvobajt(8);
	    pilot:=dvobajt(10);
	    lastbyte:=prebuffer[12];
	    pause:=dvobajt(13);
            datalen:=trobajt(15);
            if tzxopt and 4 =0 then begin
              saveblock;
              if (tzxopt and 8)<>0 then if pause<>0 then begin
                playpause;
                if not(grupped) then flushsample;
              end;
            end
            else begin
              if datalen<65500 then begin
                if not(fblockread(dat,buffer^,duzina))then begin we;exit;end;
              end
                else begin
                  if not(fblockread(dat,buffer^,65500))then begin we;exit;end;
                end;
              sampleblck;
              if (tzxopt and 8)<>0 then playpause;
              if not(grupped) then flushsample;
            end;
          end;
          relative:=trobajt($F)+$12
        end;

        $12: begin {Pure tone}
          if not(fblockread(dat,prebuffer,4)) then begin we; exit end;
          if not(skipping) and ((tzxopt and 16)<>0) then begin
            sampled:=true;
            sb_pilot:=dvobajt(0);
            pilot:=dvobajt(2);
            while (pilot<>0) do begin
              playpulse(izlbuffer,sb_pilot,pozicija,lastbit,Amp);
              ToggleAmp;
              dec(pilot);
            end;
            if not(grupped) then flushsample;
          end;
          relative:=4;
        end;

        $13: begin {Sequence of pulses}
          if not(fblockread(dat,prebuffer,1)) then begin we; exit end;;
          if not(skipping) and ((tzxopt and 16)<>0) then begin
            pilot:=prebuffer[0];
            if not(fblockread(dat,buffer^,pilot*2))then begin we;exit;end;
            datapos:=0;
            sampled:=true;
            while pilot<>0 do begin
              sb_pulse:=Konvtip(buffer^)[datapos]+256*
                Konvtip(buffer^)[datapos+1];
              playpulse(izlbuffer,sb_pulse,pozicija,lastbit,Amp);
              ToggleAmp;
              dec(pilot);
              datapos:=datapos+2;
            end;
            if not(grupped) then flushsample;
          end;
          relative:=prebuffer[0]*2+1;
        end;

        $14: begin {Pure data}
          if not(fblockread(dat,prebuffer,10)) then begin we; exit end;;
          if not(skipping) then begin
            sampled:=true;
            duzina:=dvobajt($7);
            if tzxopt and 32 =0 then begin
              saveblock;
              if (tzxopt and 8)<>0 then if pause<>0 then begin
                playpause;
                if not(grupped) then flushsample;
              end;
            end
            else begin
              sb_bit0:=dvobajt(0);
	      sb_bit1:=dvobajt(2);
              lastbyte:=prebuffer[4];
              pause:=dvobajt(5);
              datalen:=trobajt(7);  {Pazi na duge blokove}
              if datalen<65500 then begin
                if not(fblockread(dat,buffer^,duzina))then begin we;exit;end;
              end
                else begin
                if not(fblockread(dat,buffer^,65500))then begin we;exit;end;
                end;

	      sb_pilot:=0;pilot:=0;sb_sync1:=0;sb_sync2:=0;
              sampleblck;
              if (tzxopt and 64)<>0 then playpause;
              if not(grupped) then flushsample;
            end;
          end;
          relative:=trobajt(7)+$A;
        end;

        $15: begin {Direct recording}
          if not(fblockread(dat,prebuffer,8)) then begin we; exit end;;
          if not(skipping) then if (tzxopt and 128<>0) then begin
            duzinadir:=trobajt(5);
            dfreq:=dvobajt(0);
            pause:=dvobajt(2);
            if (dfreq<>158) and (dfreq<>159) and (dfreq<>79) then begin
              Displayerror('Direct recording block out of standard frequence',0);
              we; exit
            end;
            setsampperiod(dfreq);
            dflag:=(prebuffer[4]-1)+(samp shl 3);
            while duzinadir<>0 do begin
              if duzinadir>65000 then begin
               duzina:=65000;
               dflag:=7+(samp shl 3);
              end
              else begin
                move(duzinadir,duzina,2);
                dflag:=(prebuffer[4]-1)+(samp shl 3);
              end;
              if not(fblockread(dat,konvtip(buffer^)[6],duzina)) then begin we; exit end;
              move (duzina,konvtip(buffer^)[0],2);
              move (duzina,konvtip(buffer^)[2],2);
              flushsample;
              if not(save(true,tapename,65534,4,dflag,buffer^)) then begin we; exit end;
              duzinadir:=duzinadir-duzina;
            end;
          end;
          relative:=trobajt(5)+$8;
        end;

        $20: begin {Pause block}
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then begin
            pause:=dvobajt(0);
            if (pause<>0) then begin
              playpulse(izlbuffer,3500,pozicija,lastbit,amp);
              amp:=zero;
              if pause>1 then
                while pause>1 do begin
                  playpulse(izlbuffer,3500,pozicija,lastbit,amp);
                  dec(pause);
                end;
              lastbit:=0;
              flushsample;
            end
             else begin
               if sampled then flushsample;
               if not(save(true,tapename,0,4,255,buffer^)) then begin we;exit end;
             end
          end;
          relative:=2;
        end;

        $21: begin {Group start}
          if not(fblockread(dat,prebuffer,1)) then begin we; exit end;;
          if not(skipping) then begin
            grupped:=true;
          end;
          relative:=prebuffer[0]+1;
        end;

        $22: begin {Group end}
        if not(skipping) then if (tzxopt and 256 <>0) then begin
          grupped:=false;
          if sampled then flushsample;
        end;
        relative:=0;
        end;
        $23: begin {Jumpblock}
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then begin
            resttoskip:=dvobajt(0);
            if resttoskip<=0 then displayerror('Negative jumps are not allowed',0)
             else skipping:=true;

          end;
          relative:=2;
        end;
        $24: begin {Loopstart}
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then begin
            repper:=dvobajt(0);
            reppos:=curpos+2;
          end;
          relative:=2;

        end;
        $25: begin {Loopend}
          if not(skipping) then begin
            dec(repper);
            if repper<>0 then
              if not(fseek(dat,reppos)) then begin we; exit end;
          end;
          relative:=0;
        end;
        $26: begin
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then begin
            if tzxopt and 8192<>0 then DisplayError('Call sequence block will be ignored',0);
          end;
          relative:=dvobajt(0)*2+2;
        end;
        $27: begin
          if not(skipping) then
            if tzxopt and 8192<>0 then DisplayError('Return from sequence block will be ignored',0);
          relative:=0;
        end;
        $28: begin
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then
            if tzxopt and 8192<>0 then DisplayError('Select block will be ignored',0);
          relative:=dvobajt(0)+2;
        end;
        $2A: begin {Stop if 48}
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then
            if emultype=0 then begin
              if sampled then flushsample;
              if not(save(true,tapename,0,4,255,buffer^)) then begin we;exit end;
            end;
          relative:=4;
        end;

        $30: begin {Text description}
          if not(fblockread(dat,prebuffer,1)) then begin we; exit end;;
          if not(skipping) then if tzxopt and 1024<>0 then begin
            for i:=0 to 17 do konvtip(buffer^)[i]:=32;
            if not(fblockread(dat,konvtip(buffer^)[1],prebuffer[0])) then begin we; exit end;;
            konvtip(buffer^)[0]:=prebuffer[0];
            if (tzxopt and 64) <>0 then begin {Prikazi kao Header chr array}
              konvtip(buffer^)[0]:=2;
              for i:=11 to 17 do konvtip(buffer^)[i]:=0;
              if sampled then flushsample;
              if not(save(true,tapename,17,4,0,buffer^)) then begin we;exit end;
            end
              else
                i:=messagebox(string(buffer^),nil,mfOKButton+mfinformation);
          end;
          relative:=prebuffer[0]+1;
        end;

        $31: begin {Message block}
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then if tzxopt and 2048<>0 then begin
            if not(fblockread(dat,konvtip(buffer^)[1],prebuffer[1])) then begin we; exit end;;
            konvtip(buffer^)[0]:=prebuffer[1];
            i:=messagebox(string(buffer^),nil,mfOKButton+mfinformation);
          end;
          relative:=prebuffer[1]+2;
        end;
        $32: begin
          if not(fblockread(dat,prebuffer,2)) then begin we; exit end;;
          if not(skipping) then
            if tzxopt and 8192<>0 then DisplayError('Archive info block will be ignored',0);
          relative:=dvobajt(0)+2;
        end;
        $33: begin {Hardware type}
          if not(fblockread(dat,prebuffer,1)) then begin we; exit end;;
          if not(skipping) then begin
            if tzxopt and 16384<>0 then begin
              if not(fblockread(dat,buffer^,prebuffer[0]*3)) then begin we; exit end;
              for i:=0 to prebuffer[0]-1 do begin  {0 Run, 1 use, 2 not use, 3 not run}
                case konvtip(buffer^)[i*3] of
                0: begin {Machines}
                  if (konvtip(buffer^)[i*3+1] in [$C,$D,$15..$1A]) and
                  ((konvtip(buffer^)[i*3+2] in [0,1])) then begin
                    DisplayError('The tape is not for Spectrum',0);we;exit
                  end;
                  if (konvtip(buffer^)[i*3+1] in [5,$B,$10..$14]) and
                  ((konvtip(buffer^)[i*3+2] =1)) then begin
                    DisplayError('The tape uses features of nonimplemented models',0);we;exit
                  end;
                  if (konvtip(buffer^)[i*3+1] =$3) and  {Spec128}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                    emultype:=1;romsetting.Spec2:=0;
                  end;
                  if (konvtip(buffer^)[i*3+1] in [6,7]) and  {Timex}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                    emultype:=2;
                  end;

                  if (konvtip(buffer^)[i*3+1] =$4) and {+2}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                    emultype:=1;romsetting.Spec2:=1;
                  end;
                end;
               1,$C: begin {Disk, Network}
                  if (konvtip(buffer^)[i*3+1] =$0) and {Microdrive}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                   interfsetting.switch:=1;
                  end;
               end;
               3,$D: begin {Sound,Keypad}
                  if (konvtip(buffer^)[i*3+1] =$0) and {128 AY}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                   emultype:=1;
                  end;
               end;
               4: begin {Joystick}
                  if ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                    case konvtip(buffer^)[i*3+1] of
                      0:{kemp}  joysetting.joy1:=1;
                      1:{cur} joysetting.joy1:=8;
                      2:{sinc 2} joysetting.joy1:=2;
                      3:{sinc 1} joysetting.joy1:=4;
                    end;
                  end;
               end;
               7: begin {Serial ports}
                  if ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                    case konvtip(buffer^)[i*3+1] of
                      0:{Int 1}  interfsetting.switch:=1;
                      1:{128} emultype:=1;
                    end;
                  end;

               end;
               9: begin {Printer}
                  if (konvtip(buffer^)[i*3+1] =$0) and {ZX Printer}
                  ((konvtip(buffer^)[i*3+2] in [0..1])) then begin
                   printersetting.zxprinter:=1;
                  end;
               end;

              end;
              end;
            end;
          end;
          relative:=prebuffer[0]*3+1;
        end;
        $34: begin {Emulation info}
          if not(fblockread(dat,prebuffer,8)) then begin we; exit end;
          if not(skipping) then
            if tzxopt and 16384<>0 then begin
              if (dvobajt(0) and $18)=0 then videosetting.rowfirst:=156;
              if (dvobajt(0) and $18)=$10 then videosetting.rowfirst:=156;
              if (dvobajt(0) and $18)=$8 then videosetting.rowfirst:=256;
              if (dvobajt(0) and $18)=$18 then videosetting.rowfirst:=64;
              if (dvobajt(0) and $20)=0 then videosetting.border:=0
                else videosetting.border:=1;
              if (dvobajt(0) and $4)<>0 then videosetting.border:=2;
  {            if (dvobajt(0) and $100)=0 then no auto start}
            end;
          relative:=8;
        end;
        $35: begin
          if not(fblockread(dat,prebuffer,$14)) then begin we; exit end;;
          if not(skipping) then
            if tzxopt and 8192<>0 then DisplayError('Custom info block will be ignored',0);
          relative:=cetvorobajt($10)+$14;
        end;
        $40: begin {Snapshot info block}
          if not(fblockread(dat,prebuffer,$A)) then begin we; exit end;
          if not(skipping) then
            if tzxopt and 4096<>0 then begin
              if prebuffer[0]<>0 then DisplayError('Only Z80 snapshots allowed',0)
              else begin
                assign(snapdat,'TEMP.Z80');
                if not(frewriteu(snapdat,1)) then begin we; exit end;
                duzinadir:=trobajt(1);
                while duzinadir<>0 do begin
                  if duzinadir>65000 then duzina:=65000 else
                    move(duzinadir,duzina,2);
                  if not(fblockread(dat,buffer^,duzina)) then begin
                    cc:=fclose(snapdat);
                    we; exit
                  end;
                  if not(fblockwrite(snapdat,buffer^,duzina)) then begin
                    cc:=fclose(snapdat);
                    we; exit
                  end;
                end;
                if not(fclose(snapdat)) then begin we;exit end;
                runsnap:=true;
              end
            end;
          relative:=trobajt($1)+$A;
        end;
        $5A: begin {Z block}
          if not(fblockread(dat,prebuffer,$9)) then begin we; exit end;;
          relative:=9;
        end;
        else begin
          if not(fblockread(dat,prebuffer,$4)) then begin we; exit end;;
          if not(skipping) then
            if tzxopt and 8192<>0 then DisplayError('Extension above 1.20 version of TZX will be ignored',0);
          relative:=cetvorobajt($0)+$4;
        end;
    end;
    if not(fSeek(dat,curpos+relative)) then begin we;exit end;
  end;
  if sampled then flushsample;
  cc:=fclose(dat);
end;
procedure DoMuseumWara(museumname,tapename:string; buffer:Pointer);
var ulazna:file;
  duzina,dummy:longint;
  kategorija:word;
  fleg,paritet:byte;
  rb:integer;
  fd:PFileDialog;
  filename:string;
  forstart:Boolean;
procedure we;
var cc:boolean;
begin
  cc:=fclose(ulazna);
end;

begin
  duzina:=0;

  forstart:=museumname<>'';
  if not(forstart) then begin
    fd:=new(PFileDIalog,Init('*.ZXS','Select ZXS file to convert','~N~ame',
     fdOpenButton,100));
    fd^.helpctx:=hczxswara;
    if desktop^.execview(fd)<> cmcancel then begin
      hourglascur;
      fd^.getfilename(filename);
      filename:=fexpand(filename);
      if not(fileexists(filename)) then begin dispose(fd,done); exit end;
    end
     else begin dispose(fd,done); exit end;
    dispose(fd,done);
  end
   else filename:=museumname;
  assign(ulazna,filename);
  if not(fresetu(ulazna,1)) then begin we;exit;end;
  while not eof(ulazna) do begin
    if not(fblockread(ulazna,kategorija,2)) then begin we;exit;end;
    case kategorija of
      0: begin we; exit end;
      1: begin displayerror('Direct loading not yet supported',0);
        we; exit
      end;
      2: ;
      3: if not(fblockread(ulazna,dummy,1)) then begin we; exit end;
      4: if not(fblockread(ulazna,dummy,2)) then begin we; exit end;
      5: if not(fblockread(ulazna,dummy,3)) then begin we; exit end;
      else begin
        dummy:=0;
        if not(fblockread(ulazna,dummy,3)) then begin we; exit end;
        if dummy<>0 then begin displayerror('Partial bytes not allowed',0); end;
        if not(fblockread(ulazna,fleg,1)) then begin we;exit;end;
        duzina:=kategorija-5;
        if not(fblockread(ulazna,buffer^,duzina-2))then begin we;exit;end;
        if not(fblockread(ulazna,paritet,1)) then begin we;exit;end;
        if not(save(true,tapename,duzina-2,4,fleg,buffer^)) then begin
          we;exit
        end;
      end;
    end;
  end;
  we;
end;
procedure DoVoc(vocname,tapename:String;buffer,izlbuffer:Pointer);
  var filename:String;
  dat:file;
  vocfreq,emulfreq:Longint;
  header:record
    signatura:array[0..$15] of char;
    majorver,minorver:byte;
    majorcpl,minorcpl:byte;
  end;
  id:record
    code:byte;
    size:longint;
  end;
  sndinfo: record
    samprate,comptype:byte
  end;
  prebuffer:array[0..40] of byte;
  curpos,relative:longint;
  duzina,i:word;
  duzinadir:longint;
  fd:PFileDialog;
  pozicija:word;
  lastbit:byte;
  dfreq,intpart,decimpart,kumuldecim:Word;
  dflag:byte;
  cc,forstart:Boolean;

  function dvobajt(b1:byte):word;
  var r:word;
  begin
    move(prebuffer[b1],r,2);
    dvobajt:=r;
  end;
  function trobajt(b1:byte):longint;
  var r:longint;
  begin
    move(prebuffer[b1],r,3);
    r:=r and $00FFFFFF;
    trobajt:=r;
  end;
  function cetvorobajt(b1:byte):longint;
  var r:longint;
  begin
    move(prebuffer[b1],r,4);
    cetvorobajt:=r;
  end;

  {$R+}
  procedure we;
  begin
    cc:=fclose(dat);
  end;
  procedure flushsample;
  var flag:byte;
  lbunus:word;
  begin
    lbunus:=pozicija-6;
    if lastbit=0 then lastbit:=7 else begin dec(lastbit); inc(lbunus) end;
    move (lbunus,konvtip(izlbuffer^)[0],2);
    move (lbunus,konvtip(izlbuffer^)[2],2);

    flag:=lastbit+(2 shl 3);
    if not(save(true,tapename,65534,4,flag,izlbuffer^)) then begin
      we;exit
    end;
    pozicija:=6;
    lastbit:=0;
  end;
  procedure testoverflow;
  begin
    if pozicija>65000 then flushsample;
  end;
const
  andmask:array[0..7] of byte=($7F,$BF,$DF,$EF,$F7,$FB,$FD,$FE);
  ormask:array[0..7] of byte=($80,$40,$20,$10,$08,$04,$02,$01);

procedure nb1;
begin
  konvtip(izlbuffer^)[pozicija]:=konvtip(izlbuffer^)[pozicija] or
   ormask [lastbit];
  inc(lastbit);
  if lastbit=8 then begin
    inc(pozicija);
    lastbit:=0;
    if pozicija>65000 then flushsample;
  end
end;

procedure nb0;
begin
  konvtip(izlbuffer^)[pozicija]:=konvtip(izlbuffer^)[pozicija] and
   andmask [lastbit];
  inc(lastbit);
  if lastbit=8 then begin
    inc(pozicija);
    lastbit:=0;
    if pozicija>65000 then flushsample;
  end
end;
procedure bytetobits;
var i,j:word;
begin
  while duzinadir<>0 do begin
    if duzinadir>65500 then begin
     duzina:=65500;
    end
    else begin
      move(duzinadir,duzina,2);
    end;
    if not(fblockread(dat,konvtip(buffer^),duzina)) then begin we; exit end;
{    pozicija:=6;}
    for i:=0 to duzina-1 do begin
      if konvtip(buffer^)[i]>$80 then begin
        for j:=1 to intpart do nb1;
        kumuldecim:=kumuldecim+decimpart;
        if kumuldecim>100 then begin
          kumuldecim:=kumuldecim-100;
          nb1;
        end
      end
       else begin
        for j:=1 to intpart do nb0;
        kumuldecim:=kumuldecim+decimpart;
        if kumuldecim>100 then begin
          kumuldecim:=kumuldecim-100;
          nb0;
        end
       end;
    end;
    duzinadir:=duzinadir-duzina;
  end;
{  flushsample;}
end;

  begin
    duzina:=0;
    pozicija:=6;
    lastbit:=0;
    forstart:=vocname<>'';
    if not(forstart) then begin
      fd:=new(PFileDIalog,Init('*.VOC','Select VOC file to convert','~N~ame',
        fdOpenButton,100));
      fd^.helpctx:=hcvocwara;  {************}
      if desktop^.execview(fd)<> cmcancel then begin
        hourglascur;
        fd^.getfilename(filename);
        filename:=fexpand(filename);
        if not(fileexists(filename)) then begin dispose(fd,done); exit end;
      end
       else begin dispose(fd,done); exit end;
      dispose(fd,done);
    end
     else filename:=vocname;
    assign(dat,filename);
    if not(fresetu(dat,1)) then begin we;exit;end;

    if not(fblockread(dat,header,$1A)) then begin we; exit end;
    while not eof(dat) do begin
      if not(fblockread(dat,id,1)) then begin we; exit end;
      if id.code<>0 then
        if not(fblockread(dat,id.size,3)) then begin we; exit end;
      id.size:=id.size and $00FFFFFF;
      curpos:=FilePos(dat);
      relative:=id.size;
      vocfreq:=44100;
      emulfreq:=30303;
      intpart:=1;
      decimpart:=0;
      case id.code of
         0: begin flushsample; we; exit end; {Terminator}
         1: begin
           if not(fblockread(dat,sndinfo,2)) then begin we; exit end;
           if sndinfo.comptype<>0 then begin
             DisplayError('Compression is not supported',0);
             we; exit
           end;
           vocfreq:=1000000 div (256-sndinfo.samprate);
           if vocfreq>60000 then begin
             DisplayError('VOC frequence too high',0);
             we; exit
           end;
           intpart:=trunc(emulfreq/vocfreq);
           decimpart:=trunc(100*frac(emulfreq/vocfreq));
           kumuldecim:=0;
           duzinadir:=id.size-2;
           bytetobits;
         end;
         2: begin
           duzinadir:=id.size;
           bytetobits;
         end;
         3: begin
           if not(forstart) then Displayerror('Silence block ignored',0);
         end;
         4: begin
           if not(forstart) then Displayerror('Marker block ignored',0);
         end;
         5: begin
           if not(forstart) then Displayerror('ASCII block ignored',0);
         end;
         6: begin
           if not(forstart) then Displayerror('Repeat block ignored',0);
         end;
         7: begin
           if not(forstart) then Displayerror('EndRepeat block ignored',0);
         end;
       end;
    if not(fSeek(dat,curpos+relative)) then begin we;exit end;
  end;
  cc:=fclose(dat);
end;
function makevoc(vocname:string):Boolean;
var datoteka:file;
  cc:Boolean;
const prazna:array[0..$19] of char=('C','r','e','a','t','i','v','e',
  ' ','V','o','i','c','e',' ','F','i','l','e',#$1A,#$1A,#0,#$0A,#1,#$29,#$11);

begin
  makevoc:=false;
  assign(datoteka,vocname);
  if not(frewriteu(datoteka,1)) then exit;
  if not(fblockwrite(datoteka,prazna,$1A)) then begin
    cc:=fclose(datoteka);
    exit;
  end;
  if not(fclose(datoteka)) then exit;
  makevoc:=true;
end;
function block2voc(vocname:string;buffer,izlbuffer:Pointer;poslatibajtova:word;
  freq:word):Boolean;
var
  pozicija:word;
  amp:Tpulsetype;

procedure flushsample;
var datoteka:file;
cc:Boolean;
var hdr:record
  id:byte;
  leng:word;
  lenghi:byte;
end;
procedure we;
begin
  cc:=fclose(datoteka);
end;
begin
  assign(datoteka,vocname);
  if not(fresetu(datoteka,1)) then begin exit end;
  if not(fseek(datoteka,filesize(datoteka))) then begin we; exit end;
  konvtip(izlbuffer^)[0]:=223; {30303 Hz}
  konvtip(izlbuffer^)[1]:=0; {No compression}
  with hdr do begin id:=1; leng:=pozicija; lenghi:=0 end;
  if not(fblockwrite(datoteka,hdr,4)) then begin we; exit; end;
  if not(fblockwrite(datoteka,konvtip(izlbuffer^)[0],pozicija)) then begin we;exit end;
  if not(fclose(datoteka)) then begin we; exit end;
  pozicija:=2;
end;
  procedure toggleamp;
  begin
    if amp=zero then amp:=one else amp:=zero
  end;

procedure playpulse(izlbuffer:Pointer;pulselen:word;
  var pozicija:word;pulsetype:TPulsetype);

procedure nb1;
begin
  konvtip(izlbuffer^)[pozicija]:=$F0;
  inc(pozicija);
  if pozicija>65000 then flushsample;
end;
procedure nb0;
begin
  konvtip(izlbuffer^)[pozicija]:=$10;
  inc(pozicija);
  if pozicija>65000 then flushsample;
end;

var bitpulse,i,stpos,sampleper:Word;
begin
  sampleper:=116; {30303Hz}
  bitpulse:=(pulselen div sampleper);
  case pulsetype of
    zero: for i:=1 to bitpulse do nb0;
    one: for i:=1 to bitpulse do nb1;
  end;
end;
var
  pilot,sb_pilot,sb_pulse,
  sb_bit0,sb_bit1,sb_bit,lastbyte,pause,datalen,
  datapos,sb_sync1,sb_sync2,dfreq,i:Word;
  bitcount,databyte:byte;

begin
if konvtip(buffer^)[0]=0 then pilot:=8064 else pilot:=3220;
sb_pilot:=2168;
sb_sync1:=667;
sb_sync2:=735;
sb_bit0:=885;
sb_bit1:=1710;
pozicija:=2;
amp:=zero;
datalen:=poslatibajtova;
    while (pilot<>0) do begin
      playpulse(izlbuffer,sb_pilot,pozicija,Amp);
      ToggleAmp; Dec(pilot);
    end;
    if (sb_sync1<>0) then begin
      playpulse(izlbuffer,sb_sync1,pozicija,Amp);
      ToggleAmp;
    end;
    if (sb_sync2<>0) then begin
      playpulse(izlbuffer,sb_sync2,pozicija,Amp);
      ToggleAmp;
    end;
    datapos:=0;

    while (datalen<>0) do begin
      bitcount:=8;
      databyte:=Konvtip(buffer^)[datapos];
      while (bitcount<>0) do begin
        if (databyte and $80)<>0 then sb_bit:=sb_bit1
           else sb_bit:=sb_bit0;
         playpulse(izlbuffer,sb_bit,pozicija,amp);
         ToggleAmp;
         playpulse(izlbuffer,sb_bit,pozicija,amp);
         ToggleAmp;
         {$R-}
 	 databyte:=databyte shl 1;
         {$R+}
	 dec(bitcount);
      end;
      dec(datalen);inc(datapos);
    end;
    playpulse(izlbuffer,3500,pozicija,amp);
    amp:=zero;
    for i:=1 to 1000 do playpulse(izlbuffer,3500,pozicija,amp);
    flushsample;
end;

function finishvoc(vocname:string):Boolean;
var datoteka:File;
var cc:Boolean;
zerome:byte;
procedure we;
begin
  cc:=fclose(datoteka);
end;
begin
  assign(datoteka,vocname);
  if not(fresetu(datoteka,1)) then begin exit end;
  if not(fseek(datoteka,filesize(datoteka))) then begin we; exit end;
  zerome:=0; {EOF}
  if not(fblockwrite(datoteka,zerome,1)) then begin we;exit end;
  if not(fclose(datoteka)) then begin  exit end;
end;


function sample2voc(vocname:string;buffer,izlbuffer:Pointer;poslatibajtova:word;
  freq:word):Boolean;
var
  pozicija,datalen,datapos:word;
  amp:Tpulsetype;
  vocspeed,bitcount,databyte:byte;
  vocfreq:Longint;


procedure flushsample;
var datoteka:file;
cc:Boolean;
var hdr:record
  id:byte;
  leng:word;
  lenghi:byte;
end;
procedure we;
begin
  cc:=fclose(datoteka);
end;
begin
  assign(datoteka,vocname);
  if not(fresetu(datoteka,1)) then begin exit end;
  if not(fseek(datoteka,filesize(datoteka))) then begin we; exit end;
  konvtip(izlbuffer^)[0]:=vocspeed;
  konvtip(izlbuffer^)[1]:=0; {No compression}
  with hdr do begin id:=1; leng:=pozicija; lenghi:=0 end;
  if not(fblockwrite(datoteka,hdr,4)) then begin we; exit; end;
  if not(fblockwrite(datoteka,konvtip(izlbuffer^)[0],pozicija)) then begin we;exit end;
  if not(fclose(datoteka)) then begin we; exit end;
  pozicija:=2;
end;
procedure nb1;
begin
  konvtip(izlbuffer^)[pozicija]:=$F0;
  inc(pozicija);
  if pozicija>65000 then flushsample;
end;
procedure nb0;
begin
  konvtip(izlbuffer^)[pozicija]:=$10;
  inc(pozicija);
  if pozicija>65000 then flushsample;
end;
begin
  vocfreq:=3500000 div getsampper(freq);
  vocspeed:=256-(1000000 div vocfreq);
  datalen:=poslatibajtova;
  datapos:=0;
  pozicija:=2;
  while (datalen<>0) do begin
    if (datalen<>1) then bitcount:=8 else bitcount:=1+(freq and 7);
    databyte:=konvtip(buffer^)[datapos];
    while (bitcount<>0) do begin
      if (databyte and $80)<>0 then nb1 else nb0;
      {$R-}
      databyte:=databyte shl 1;
      {$R+}
      dec(bitcount);
    end;
    dec(datalen);
    inc(datapos);
  end;
  flushsample;
end;

procedure DoZX32TapWara(tapename:String;buffer:Pointer);
var chunk:String[4];
size,upto:Longint;
duzina:longint;
fleg,paritet:byte;
ulazna:file;
tapeinfo:record
  block,pos,tsize:Longint
end;
fd:PFileDialog;
filename:string;
fmtzinfo:record
  wVersion,WHardModel,wHardFlags,wComprMeth:word
end;


procedure we;
var cc:boolean;
begin
  cc:=fclose(ulazna);
end;

begin
  duzina:=0;
  fd:=new(PFileDIalog,Init('*.ZX?','Select RIFF file to extract tape','~N~ame',
   fdOpenButton,100));
  fd^.helpctx:=hczx32wara;
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

  while not eof(ulazna) do begin
    if not(fblockread(ulazna,chunk[1],4)) then begin we; exit end;
    if not(fblockread(ulazna,size,4)) then begin we; exit end;
    if chunk='fmtz' then begin
     if not(fblockread(ulazna,fmtzinfo,8)) then begin we; exit end;
      if fmtzinfo.wComprMeth<>$FFFF then begin
         DisplayError('Compressed chunks are not available',0);
         we; exit
      end;
    end
    else
    if chunk<>'tape' then begin
       if not(fseek(ulazna,filepos(ulazna)+size)) then begin we; exit end
    end
     else begin
       if not(fblockread(ulazna,tapeinfo,12)) then begin we; exit end;
       upto:=filepos(ulazna)+tapeinfo.tsize;
       while filepos(ulazna)<upto do begin
         if not(fblockread(ulazna,duzina,2)) then begin we;exit;end;
         if not(fblockread(ulazna,fleg,1)) then begin we;exit;end;
         if not(fblockread(ulazna,buffer^,duzina-2))then begin we;exit;end;
         if not(fblockread(ulazna,paritet,1)) then begin we;exit;end;
         if not(save(true,tapename,duzina-2,4,fleg,buffer^)) then begin
           we;exit
         end;

       end
     end;
  end;
  we;
end;
procedure DOShalaWara(tapename:String;buffer:Pointer);
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
  header:record
    htip:byte;
    hname:array[1..10] of char;
    w1,w2,w3:word
  end;
  preventry,curentry,empentry:tdirentry;
  duzina,hedduzina,stvduzina,freezone,entnum:word;
  pozicioniraj,posheader:longint;
  datoteka:file;
  cc,kraj:Boolean;
  i:integer;
  trdfile:string;
  a:PFileDialog;
  procedure we;
  begin
    cc:=fclose(datoteka);
  end;
begin


  fillchar(empentry,sizeof(empentry),0);
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
  assign(datoteka,trdfile);
  if not(fresetu(datoteka,1)) then exit;
  entnum:=0;
  repeat
    preventry:=curentry;

    if not(fblockread(datoteka,curentry,16)) then begin we; exit end;
    posheader:=filepos(datoteka);
    pozicioniraj:=longint(256)*(curentry.clusterseg*16+curentry.clusterofs);
    if not(fseek(datoteka,pozicioniraj)) then begin we; exit end;
    with curentry do begin
      if tip='B' then duzina:=startcode else duzina:=codelen;
      for i:=1 to 10 do header.hname[i]:=' ';
      for i:=1 to 8 do header.hname[i]:=name[i];
      case tip of
        'B': begin
          header.htip:=0;
          header.w1:=startcode;
          header.w2:=0;
          header.w3:=codelen;
        end;
        'C': begin
          header.htip:=3;
          header.w1:=codelen;
          header.w2:=startcode;
          header.w3:=0;
        end;
        'D': begin
          header.htip:=1;
          header.w1:=codelen;
          header.w2:=0;
          header.w3:=0;
        end;

      end

  end;
  kraj:=(curentry.name=empentry.name);
  if not(kraj) then begin
    if not(save(true,tapename,17,4,0,header)) then begin we; exit end;
    if not(fblockread(datoteka,buffer^,duzina)) then begin we; exit end;
    if not(save(true,tapename,duzina,4,255,buffer^)) then begin we; exit end;
    inc(entnum);
    if entnum>=128 then begin
      we;exit
    end;
    if not(fseek(datoteka,posheader)) then begin we; exit end;
  end;
  until kraj;;
  cc:=fclose(datoteka);

end;

type Tcommanual=object(TDialog)
  st:string;
  mode:integer;
  procedure handleevent(var event:Tevent); virtual;
  procedure draw; virtual;
end;
Pcommanual=^TCommanual;

procedure Tcommanual.handleevent(var event:Tevent);
begin
   TDialog.handleevent(event);
   if event.what=evKeyDown then EndModal(cmOK);
end;

procedure Tcommanual.draw;
begin
  TDialog.draw;
  case mode of
    1: begin

      writestr(1,1,
      'Type on Spectrum in 128 BASIC this program and start it.',1);
      writestr(1,2,'10 CLEAR 24299: LET S=0',3);
      writestr(1,3,'20 FOR I=47000 TO 52999',3);
      writestr(1,4,'30 LET A$=INKEY$ #3: IF A$="" THEN GOTO 30',3);
      writestr(1,5,'40 LET V=CODE A$: POKE I,V: LET S=S+V',3);
      writestr(1,6,'50 NEXT I',3);
      writestr(1,7,'60 PRINT S',3);
    end;
    2: begin
      writestr (1,1,'If sum which Spectrum displayed matchs with '+st,1);
      writestr (1,2,'it is OK ,if not try again.',1);
      writestr (1,3,'Now type NEW to erase BASIC and type this program in',1);
      writestr (1,4,'128 BASIC mode',1);
      writestr (1,6,'10 CLEAR 24299: LOAD "" CODE: RANDOMIZE USR 52800',3);
      writestr (1,8,
        'finaly, save communication program /version 128K/ using',1);
      writestr (1,10,'SAVE "ZXCOMM" LINE 10: SAVE "COMM" CODE 47000,6000',3);
    end;
  end

end;

procedure Sendcommprogram;
var i,f :integer;
  b:longint;
  datoteka:file;
  bajt:byte;
  adresa:word;
  upustvo:Pcommanual;
  kolikopren:PProcenter;
  r:Trect;
  c:word;
  st:string;
  cc:Boolean;
  timing:word;

  procedure we;
  begin
    cc:=fclose(datoteka);
    desktop^.helpctx:=hcnocontext;
    dispose(upustvo,done);
    dispose(kolikopren,done);
  end;

  function wantcancel:boolean;
  begin
      if memw[$46:$c]-timing>18 then begin
        timing:=memw[$46:$c];
           if kolikopren^.eventavail then wantcancel:=kolikopren^.wantcancel
          else wantcancel:=false;
      end
        else wantcancel:=false;
  end;


begin
  i:=comm_install(dfltrs232);
  comm_setparam(baudrates[slbdrt]);
  i:=modem_dtr(0);
  b:=0;
  assign(datoteka,'comm128.bzx');
  {$i-}
  reset(datoteka,1);
  {$i+}
  if ioresult<>0 then begin
     displayerror('Can not find communication program',0);
     exit;
  end;
  adresa:=47000;
  r.assign(12,6,70,18);
  upustvo:=new(Pcommanual,Init(R,'Instructions'));
  upustvo^.mode:=1;
  upustvo^.helpctx:=hcsendcprg;
  c:=desktop^.execview(upustvo);
  hourglascur;
  desktop^.helpctx:=hcsendcprg;
  new(kolikopren,Init(12,7,52,0,6000));
  desktop^.insert(kolikopren);
  b:=0;

  timing:=memw[$46:$c];
  while not eof (datoteka) do begin
    if not(fblockread(datoteka,bajt,1)) then begin we; exit end;
    if wantcancel then exit;
    while modem_status<>51 do begin
       if wantcancel then begin we;exit end;
    end;
    comm_put(bajt);
    b:=b+bajt;
    if adresa mod 100 = 0 then begin
      str(adresa:5,st);
      kolikopren^.update(adresa-47000,st);
    end;
    inc(adresa)
  end;
  str(b,upustvo^.st);
  upustvo^.mode:=2;
  c:=desktop^.execview(upustvo);
  hourglascur;

  we;
  i:=comm_remove;
end;
procedure doadvset;
var
  Dlg : PDialog;
  R : TRect;
  Control, HScroll : PView;
  c:word;

begin
  R.Assign(1,1,77,22);
  New(Dlg, Init(R, 'Advanced cassete setting'));
  Dlg^.Helpctx:=hcadvcassetting;
  R.Assign(1,1,34,7);
  Control := New(PStaticText, Init(R, '             Leader'^M+
     '    ÚÄÄÄ¿   ÚÄÄÄ¿   ÚÄÄÄ¿   ÚÄÄÄ'^M+
     ' ÄÄÄÙ   ÀÄÄÄÙ   ÀÄÄÄÙ   ÀÄÄÄÙ         '^M+
     ' <ÄÄÄÄÄÄ>'^M+
     ' min length ö 255-Leader const 1'^M+
     ' max length ö 255-Leader const 2'));
  Dlg^.Insert(Control);

  R.Assign(35,1,75,7);
  Control := New(PStaticText, Init(R, '   Synchro impuls'^M+
     '    ÚÄÄÄÄÄÄ Total len ö 255-Sync total'^M+
     ' ÄÄÄÙ       First len ö 255-First part      '^M+
     ' <ÄÄ>'^M+
     ' <ÄÄÄÄÄÄÄÄÄ>'));
  Dlg^.Insert(Control);

  R.Assign(3,10,68,15);
  Control := New(PStaticText, Init(R, '           Data bytes'^M+
     '     ÚÄÄÄÄÄ¿  ÚÄÄ       min length of zero ö Const delay '^M+
     'ÄÄÄÄÄÙ     ÀÄÄÙ         max length of one ö 255-const max impuls'^M+
     '<ÄÄÄÄÄÄÄÄÄ><ÄÄÄÄ>       min length of one ö 255-treshold'^M+
     'logical 1   logical 0 '));
  Dlg^.Insert(Control);

  R.Assign(20,7,25,8);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(4,7,19,8);
  Dlg^.Insert(New(PLabel, Init(R, 'Leader const ~1~', Control)));

  R.Assign(20,8,25,9);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(4,8,19,9);
  Dlg^.Insert(New(PLabel, Init(R, 'Leader const ~2~', Control)));

  R.Assign(62,7,67,8);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(50,7,61,8);
  Dlg^.Insert(New(PLabel, Init(R, '~S~ync total', Control)));

  R.Assign(62,8,67,9);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(50,8,61,9);
  Dlg^.Insert(New(PLabel, Init(R, '~F~irst part', Control)));

  R.Assign(15,15,20,16);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(3,15,15,16);
  Dlg^.Insert(New(PLabel, Init(R, 'Const ~d~elay', Control)));

  R.Assign(20,16,25,17);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(3,16,20,17);
  Dlg^.Insert(New(PLabel, Init(R, 'Const ~m~ax impuls', Control)));

  R.Assign(19,17,24,18);
  Control := New(pnuminputline, Init(R, 3, 0, 255));
  Dlg^.Insert(Control);

  R.Assign(3,17,18,18);
  Dlg^.Insert(New(PLabel, Init(R, 'Const ~t~reshold', Control)));

  R.Assign(25,19,32,20);
  Control := New(pnuminputline, Init(R, 5, 0, 65535));
  Dlg^.Insert(Control);

  R.Assign(3,19,25,20);
  Dlg^.Insert(New(PLabel, Init(R, 'Sound ~b~laster address', Control)));

  R.Assign(45,17,57,19);
  Control := New(PButton, Init(R, '~O~k', cmOk, bfNormal));
  Dlg^.Insert(Control);

  R.Assign(61,17,73,19);
  Control := New(PButton, Init(R, '~C~ancel', cmCancel, bfNormal));
  Dlg^.Insert(Control);

  Dlg^.SelectNext(False);
  Dlg^.setdata(casspars);
  c:=desktop^.execview(Dlg);
  hourglascur;
  if c=cmOK then Dlg^.getdata(casspars);
  dispose(dlg,done);

end;

end.
