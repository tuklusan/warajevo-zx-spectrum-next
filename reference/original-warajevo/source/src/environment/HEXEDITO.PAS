unit hexedito;
{$F+,O+,X+,S-,D-}
interface

uses Objects, Drivers, Dialogs,Views;

type

  { Same as TInputLine, except invalid if empty }
  PKeyInputLine = ^TKeyInputLine;
  TKeyInputLine = object(TInputLine)
    function Valid(Command: Word): Boolean; virtual;
  end;

  { Accepts only valid numeric input between Min and Max }
  PNumInputLine = ^TNumInputLine;
  TNumInputLine = object(TInputLine)
    Min: Longint;
    Max: Longint;
    constructor Init(var Bounds: TRect; AMaxLen: Integer;
      AMin, AMax: Longint);
    constructor Load(var S: TStream);
    function DataSize: Word; virtual;
    procedure GetData(var Rec); virtual;
    procedure SetData(var Rec); virtual;
    procedure Store(var S: TStream);
    function Valid(Command: Word): Boolean; virtual;
  end;

procedure RegisterFields;

const
  RKeyInputLine: TStreamRec = (
     ObjType: 10060;
     VmtLink: Ofs(TypeOf(TKeyInputLine)^);
     Load:    @TKeyInputLine.Load;
     Store:   @TKeyInputLine.Store
  );
  RNumInputLine: TStreamRec = (
     ObjType: 10061;
     VmtLink: Ofs(TypeOf(TNumInputLine)^);
     Load:    @TNumInputLine.Load;
     Store:   @TNumInputLine.Store
  );

function Hexeditor(startaddr:Pointer;editsize:word;inidispadd:word):word;

type TProcenter2=object(Tscrollbar)
  activestring:string;
  okvir:pstatictext;
  procedure draw; virtual;
  procedure update(numval:word; infostring:string);
  constructor init(x,y,w,minrng,maxrng:word);
  destructor done; virtual;
end;
PProcenter2=^TProcenter2;



type TProcenter=object(TDialog)
  scrollbar:PScrollbar;
  activestring:string;
  procedure draw; virtual;
  procedure update(numval:word; infostring:string);
  function wantcancel:Boolean;
  constructor init(x,y,w,minrng,maxrng:word);
end;
PProcenter=^TProcenter;

implementation
uses app, MsgBox;

procedure RegisterFields;
begin
  RegisterType(RKeyInputLine);
  RegisterType(RNumInputLine);
end;

{ TKeyInputLine }

function TKeyInputLine.Valid(Command: Word): Boolean;
var
  Ok: Boolean;
begin
  Ok := True;
  if (Command <> cmCancel) and (Command <> cmValid) then
  begin
    if Data^ = '' then
    begin
      Select;
      MessageBox('This field cannot be empty.', nil, mfError + mfOkButton);
      Ok := False;
    end;
  end;
  if Ok then Valid := TInputLine.Valid(Command)
  else Valid := False;
end;

{ TNumInputLine }
constructor TNumInputLine.Init(var Bounds: TRect; AMaxLen: Integer;
  AMin, AMax: Longint);
begin
  TInputLine.Init(Bounds, AMaxLen);
  Min := AMin;
  Max := AMax;
end;

constructor TNumInputLine.Load(var S: TStream);
begin
  TInputLine.Load(S);
  S.Read(Min, SizeOf(LongInt) * 2);
end;

function TNumInputLine.DataSize: Word;
begin
  DataSize := SizeOf(LongInt);
end;

procedure TNumInputLine.GetData(var Rec);
var
  Code: Integer;
begin
  Val(Data^, Longint(Rec), Code);
end;

procedure TNumInputLine.Store(var S: TStream);
begin
  TInputLine.Store(S);
  S.Write(Min, SizeOf(Longint) * 2);
end;

procedure TNumInputLine.SetData(var Rec);
var
  S: string[12];
begin
  Str(Longint(Rec), Data^);
  SelectAll(True);
end;

function TNumInputLine.Valid(Command: Word): Boolean;
var
  Code: Integer;
  Value: Longint;
  Params: array[0..1] of LongInt;
  Ok: Boolean;
begin
  Ok := True;
  if (Command <> cmCancel) and (Command <> cmValid) then
  begin
    if Data^ = '' then Data^ := '0';
    Val(Data^, Value, Code);
    if (Code <> 0) or (Value < Min) or (Value > Max) then
    begin
      Select;
      Params[0] := Min;
      Params[1] := Max;
      MessageBox('Number must be from %D to %D.', @Params, mfError + mfOkButton);
      SelectAll(True);
      Ok := False;
    end;
  end;
  if Ok then Valid := TInputLine.Valid(Command)
  else Valid := False;
end;


{
  *******************************
    Hex editor
  *******************************

}
Type

TRadioAffButtons=Object(TRadioButtons)
  procedure handleevent(var event:TEvent);virtual;
end;
pRadioAffButtons=^TRadioAffButtons;

procedure TRadioAffButtons.handleevent(var event:TEvent);
begin
  TRadioButtons.handleevent(event);
  owner^.redraw;
end;


type  THexeditor=object(TListBox)
  byteinrow:word;
  startaddr:Pointer;
  viewsize:word;
  initial:word;
  modekey:byte;
  radio:PRadioAffButtons;
  lastradio,lastfocused:word;
  lastbyteinrow:byte;
  procedure handleevent(var Event:TEvent);virtual;
  function gettext(Item:integer; MaxLen: Integer): String; virtual;
  constructor init(var Bounds:TRect; ANumCols:
    Integer; AScrollBar: PScrollBar; stad:Pointer; cviewsize:word;
    ini:word);

end;
  pHexEditor=^THexEditor;

procedure THexeditor.HandleEvent(var Event:TEvent);
var xpos:integer;
  adresa:word;
  segment:word;
  bajtadr,cifra,taster:byte;
  currentspot:TPoint;
  procedure nextbyt;
  begin
    if word(16*focused+byteinrow)>=viewsize then exit;
    inc(byteinRow);
    if byteinRow>=16 then begin byteinRow:=0; inc(focused) end;
    modekey:=0;
  end;
begin
  if (radio^.value<>lastradio) or (focused<>lastfocused)
    or (byteinrow<>lastbyteinrow) then modekey:=0;
  lastradio:=radio^.value;
  lastbyteinrow:=byteinrow;
  lastfocused:=focused;
  adresa:=word(16*focused+byteinRow+ofs(startaddr^));
  segment:=seg(startaddr^);
  bajtadr:=mem[segment:adresa];
  if Event.What = evMouseDown then
  begin
      if MouseInView(Event.Where) then
      begin
        MakeLocal(Event.Where, CurrentSpot);
        case radio^.value of
          0: byteinrow:=(currentspot.x-6) div 3;
          1: byteinrow:=(currentspot.x-7) div 4;
          2: byteinrow:=(currentspot.x-7) div 2;
        end;
        if byteinrow<0 then byteinrow:=0;
        if byteinrow>15 then byteinrow:=15;
        draw;
      end;
  end;
  if not ((Event.What=EvKeyDown) and (lo(event.KeyCode)=32)) then
  TListBox.HandleEvent(Event);

  if Event.What=EvKeyDown then begin
    case Event.KeyCode of
      kbRight: if byteinRow<15 then inc(byteinRow);
      kbLeft: if byteinRow>0 then dec(byteinRow);
    end;
    taster:=lo(Event.KeyCode);
    case radio^.value of
      0: begin
          case modekey of
          0: begin
            bajtadr:=lo(bajtadr-16*(bajtadr div 16));
            if chr(taster) in ['0'..'9'] then begin
              mem[segment:adresa]:=lo(bajtadr+16*(taster-ord('0')));
              inc(modekey);
              clearevent(Event);
            end;
            if chr(taster) in ['A'..'F'] then begin
              mem[segment:adresa]:=lo(bajtadr+16*(taster-ord('A')+10));
              inc(modekey);
              clearevent(Event);

            end;
            if chr(taster) in ['a'..'f'] then begin
              mem[segment:adresa]:=lo(bajtadr+16*(taster-ord('a')+10));
              inc(modekey);
              clearevent(Event);

            end;

          end;
          1: begin
            bajtadr:=lo(bajtadr-(bajtadr mod 16));
            if chr(taster) in ['0'..'9'] then begin
              mem[segment:adresa]:=lo(bajtadr+(taster-ord('0')));
              modekey:=0;
              clearevent(Event);
              nextbyt;

            end;
            if chr(taster) in ['A'..'F'] then begin
              mem[segment:adresa]:=lo(bajtadr+(taster-ord('A')+10));
              modekey:=0;
              clearevent(Event);
              nextbyt;
            end;
            if chr(taster) in ['a'..'f'] then begin
              mem[segment:adresa]:=lo(bajtadr+(taster-ord('a')+10));
              modekey:=0;
              clearevent(Event);
              nextbyt;
            end;

          end;
          else modekey:=0;
        end;

      end;
      1: begin
          case modekey of
          0: begin
            bajtadr:=lo(bajtadr-100*(bajtadr div 100));
            if chr(taster) in ['0'..'9'] then begin
              mem[segment:adresa]:=lo(bajtadr+100*(taster-ord('0')));
              inc(modekey);
              clearevent(Event);
            end;
          end;
          1: begin
            bajtadr:=lo(bajtadr-10*((bajtadr div 10) mod 10));
            if chr(taster) in ['0'..'9'] then begin
              mem[segment:adresa]:=lo(bajtadr+10*(taster-ord('0')));
              inc(modekey);
              clearevent(Event);
            end;
          end;

          2: begin
            bajtadr:=lo(bajtadr-(bajtadr mod 10));
            if chr(taster) in ['0'..'9'] then begin
              mem[segment:adresa]:=lo(bajtadr+(taster-ord('0')));
              modekey:=0;
              clearevent(Event);
              nextbyt;
            end;
          end;
          else modekey:=0;
        end;
      end;
      2: begin
        if modekey=0 then begin
          if  (taster>=32) and (taster<=255) then begin
            mem[segment:adresa]:=lo(taster);
            clearevent(Event);
            nextbyt;
          end
        end
         else modekey:=0;
      end;
    end;
    draw;
  end;

end;


function THexeditor.gettext(Item:integer; MaxLen: integer): String;
var xpos,prvi,zadnji,i:word;
  preamble:char;
  a:string;
  v:byte;
const hcif:array[0..15] of  char=('0','1','2','3','4','5','6','7','8',
  '9','A','B','C','D','E','F');

  function hex(v:byte): string;
  var st:string[2];
  begin
    st:='  ';
    st[1]:=hcif[v div 16];
    st[2]:=hcif[v mod 16];
    hex:=st;
  end;

  function dec(v:byte):string;
  var v1:byte;
    st:string[3];
  begin
    st:='   ';
    st[1]:=hcif[v div 100];
    st[2]:=hcif[(v div 10) mod 10];
    st[3]:=hcif[v mod 10];
    dec:=st;
  end;
begin
  if word(focused*16+byteinRow)>viewsize then
    byteinrow:=lo(viewsize-focused*16);

  case radio^.value of
    1,2: begin
      str(initial+word(item*16):5,a);
      for i:=1 to 5 do if a[i]=' ' then a[i]:='0';
      a:=a+' '
    end;
    0: a:=hex((initial+word(item*16)) div 256)+
          hex((initial+word(item*16))mod 256)+' ';
  end;
  prvi:=word(item*16);
  zadnji:=word(item*16+15);
  if prvi>viewsize then prvi:=viewsize;
  if zadnji>viewsize then zadnji:=viewsize;
  for i:=prvi to zadnji do begin
    v:=mem[seg(startaddr^):ofs(startaddr^)+i];
    case radio^.value of
      0: begin
        a:=a+' '+hex(v)
      end;
      1: begin
        a:=a+' '+dec(v)
      end;
      2: begin
        a:=a+' '+chr(v)
      end;
    end;
  end;
  a:=a+' ';
  xpos:=0;
  if item=focused then case radio^.value of
    0: begin
      xpos:=6+byteinrow*3;
      a[xpos]:='';
      a[xpos+3]:=''
    end;
    1: begin
      xpos:=7+byteinrow*4;
      a[xpos]:='';
      a[xpos+4]:=''
    end;
    2: begin
      xpos:=7+byteinrow*2;
      a[xpos]:='';
      a[xpos+2]:=''
    end;

  end;
  gettext:=a;
end;

constructor THexeditor.init(var Bounds:TRect; ANumCols:
  Integer; AScrollBar: PScrollBar; stad:Pointer; cviewsize:word;
  ini:word);
begin
  TLIstBox.init(Bounds,AnumCols,AScrollBar);
  startaddr:=stad;
  setrange(1 + cviewsize div 16 );
  viewsize:=cviewsize-1;
  setrange(1 + viewsize div 16 );
  byteinrow:=0;
  modekey:=0;
  lastfocused:=0;
  lastbyteinrow:=0;
  initial:=ini;
end;


function Hexeditor(startaddr:Pointer;editsize:word; inidispadd:word):word;
var a: PDialog;
  R:TRect;
  skrolnica:PScrollBar;
  ll:PHexEditor;
  Bruce:Pointer;
  c:word;
begin
   R.assign(2,1,78,22);
   a:=new(PDialog,Init(R,'Edit'));
   R.assign(74,1,75,15);
   skrolnica:=new(pscrollbar,Init(r));
   a^.insert(skrolnica);
   R.assign(1,1,73,15);
   ll:=new(PHexeditor,Init(R,1,skrolnica,startaddr,editsize,inidispadd));
   ll^.focusitem(0);
   a^.insert(ll);
   R.Assign(40,16,50,18);
   Bruce:=new(Pbutton,Init(R,'~O~K',cmOk,0));
   a^.insert(Bruce);
   R.Assign(55,16,65,18);
   Bruce:=new(Pbutton,Init(R,'~C~ancel',cmCancel,0));
   a^.insert(Bruce);

   R.Assign(10,16, 28,19);
   Bruce := New(PRadioAffButtons, Init(R,
     NewSItem('~H~exadecimal',
     NewSItem('~D~ecimal',
     NewSItem('~A~SCII',
     nil)))
   ));
  a^.Insert(Bruce);
  ll^.radio:=Bruce;
  hexeditor:=desktop^.execview(a);
  dispose(a,done);
end;


constructor TProcenter.init(x,y,w,minrng,maxrng:word);
var r:Trect;
begin
  r.assign(x,y,x+w,y+6);
  TDialog.init(R,'');
  r.assign(1,3,w-1,4);
  scrollbar:=new(PScrollBar,Init(R));
  scrollbar^.setrange(minrng,maxrng);
  insert(scrollbar)
end;

procedure TProcenter.update(numval:word;infostring:string);
begin
  activestring:=infostring;
  scrollbar^.setvalue(numval);
  draw;
end;

procedure TProcenter.draw;
begin
  Twindow.draw;
  writestr(1,2,activestring,14);
end;

function TProcenter.wantcancel:Boolean;
var canc:Boolean;
  event: Tevent;
  R:Trect;
begin
  canc:=false;
  while eventavail do begin
    getevent(event);
    case event.what of
      evcommand: if event.command=cmClose then canc:=true;
      evmousedown: begin
        if (event.where.x=origin.x+3) and
          (event.where.y=origin.y+1) then canc:=true;
        r.assign(1,1,79,22);
        if (event.where.x>origin.x+3) and (event.where.x<origin.x+size.x) and
          (event.where.y=origin.y+1) then
          dragview(event,dmDragmove,r,size,size);
      end;
      evkeydown: if event.keycode=kbEsc then canc:=true;
    end;
    clearevent(event);
  end;
  wantcancel:=canc;
end;

constructor TProcenter2.init(x,y,w,minrng,maxrng:word);
var r:Trect;
  st:string;
begin
  r.assign(x+1,y+1,x+w-1,y+2);
  Tscrollbar.init(R);
  r.assign(x,y,x+w,y+3);
  st[0]:=chr(255);
  fillchar(st[1],255,chr(205));
  st[1]:=chr(201);
  st[w]:=chr(187);
  st[w+1]:=chr(186);
  st[2*w]:=chr(186);
  st[2*w+1]:=chr(200);
  st[3*w]:=chr(188);

  okvir:=new(pstatictext,init(r,st));
  desktop^.insert(okvir);
  setrange(minrng,maxrng);
end;

destructor Tprocenter2.done;
begin
  desktop^.delete(okvir);
  dispose(okvir,done);
end;

procedure TProcenter2.update(numval:word;infostring:string);
begin
  activestring:=infostring;
  setvalue(numval);
  if (numval mod 10)=0 then draw;
end;

procedure TProcenter2.draw;
begin
  Tscrollbar.draw;
{  writestr(1,2,activestring,14);}
end;


end.
