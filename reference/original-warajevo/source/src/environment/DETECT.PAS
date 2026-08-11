{$O+}
unit detect;
interface
(*****************************************************************************)
(*                                                                           *)
(*  TVGRAPH Version 1.0 (C) C.L.Burke. Portions (C) Borland                  *)
(*  - Thanks to Richard Wilton for much EGA/VGA and general information      *)
(*  Display mode detection routines                                          *)
(*                                                                           *)
(*****************************************************************************)
const
  hcrt=$3b4;
type
  vsystem=(MONO,HERC,CGA,EGA,VGA,MCGA,OTHER);
  TextGraphics = (_TEXT,_GRAPHICS);
  (* flag if graphics / text mode *)
  GraphInfo = record
   XRes,YRes,CRes:word;
   ScreenWriteType:TextGraphics;
   ScreenSeg:word;
   SuitableAdapters:set of Vsystem;
  end;
  (* Record keeping information on each display mode *)
function whatvsystem:vsystem;

const
  GraphScreenInfo:array[0..$13] of GraphInfo =
  (
  (Xres: 40;Yres: 25;CRes: 16;ScreenWriteType:    _TEXT;ScreenSeg:$B800;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*0*)
  (Xres: 40;Yres: 25;CRes: 16;ScreenWriteType:    _TEXT;ScreenSeg:$B800;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*1*)
  (Xres: 80;Yres: 25;CRes: 16;ScreenWriteType:    _TEXT;ScreenSeg:$B800;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*2*)
  (Xres: 80;Yres: 25;CRes: 16;ScreenWriteType:    _TEXT;ScreenSeg:$B800;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*3*)
  (Xres:320;Yres:200;CRes:  4;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*4*)
  (Xres:320;Yres:200;CRes:  4;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*5*)
  (Xres:640;Yres:200;CRes:  2;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[CGA,EGA,VGA,MCGA]), (*6*)
  (Xres: 80;Yres: 25;CRes:  2;ScreenWriteType:    _TEXT;ScreenSeg:$B000;
  SuitableAdapters:[MONO,HERC,EGA,VGA]),(*7*)
  (Xres: 00;Yres: 00;CRes:  0;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[]),                 (*8*)
  (Xres: 00;Yres: 00;CRes:  0;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[]),                 (*9*)
  (Xres: 00;Yres: 00;CRes:  0;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[]),                 (*A*)
  (Xres: 00;Yres: 00;CRes:  0;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[]),                 (*B*)
  (Xres: 00;Yres: 00;CRes:  0;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[]),                 (*C*)
  (Xres:320;Yres:200;CRes: 16;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[EGA,VGA]),          (*D*)
  (Xres:640;Yres:200;CRes: 16;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[EGA,VGA]),          (*E*)
  (Xres:640;Yres:350;CRes:  2;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[EGA,VGA]),          (*F*)
  (Xres:640;Yres:350;CRes: 16;ScreenWriteType:_GRAPHICS;ScreenSeg:$A000;
  SuitableAdapters:[EGA,VGA]),          (*10*)
  (Xres:640;Yres:480;CRes:  2;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[VGA,MCGA]),         (*11*)
  (Xres:640;Yres:480;CRes: 16;ScreenWriteType:_GRAPHICS;ScreenSeg:$A000;
  SuitableAdapters:[VGA]),              (*12*)
  (Xres:320;Yres:200;CRes:256;ScreenWriteType:_GRAPHICS;ScreenSeg:$0000;
  SuitableAdapters:[VGA,MCGA]));        (*13*)
implementation

function find6845(addr:word):boolean; (* TRUE IF 6845 *)
var
 tmp:byte;
 dl:word;
begin
  port[addr]:=$F;
  tmp:=port[addr+1];
  port[addr+1]:=$66;
  for dl:=1 to 500 do;
  find6845:=port[addr+1]=$66;
  port[addr+1]:=tmp;
end;

function findmono:vsystem;
var
 cnt:word;
 tmp1,tmp2:byte;
begin
 if find6845(hcrt) then
 begin
   tmp1:=port[hcrt+6] and $80;
   repeat
     tmp2:=port[hcrt+6] and $80;
   until tmp1<>tmp2;
   if tmp1<>tmp2 then findmono:=HERC
                 else findmono:=MONO;
 end
 else (*Not Mono *)
   findmono:=OTHER;
end;

function findCGA:vsystem;
begin
 if find6845($3D4) then findCGA:=CGA
                   else findCGA:=OTHER;
end;

function findEGA:vsystem;
var
 rbx:word;
 rcl:byte;
begin
 asm
 mov bx,0010h
 mov ax,1200h
 int 010h
 mov rbx,bx
 mov rcl,cl
 end;
 if lo(rbx)<>$10 then
 begin
  case rcl div 2 of
    0,3:findEGA:=CGA;
    1,4:findEGA:=EGA;
    2,5:findEGA:=Herc;
  end
 end
 else (*No ega *)
   findEGA:=OTHER;
end;

function findPS2:vsystem;
var
 rax,rbx:word;
begin
 asm
   mov ax,1A00h
   int 10h
   mov rax,ax
   mov rbx,bx
 end;
 if lo(rax)=$1A then
 begin
   case lo(rbx) of
   0,3,6,9:findPS2:=other;
         1:findPS2:=MONO;
         2:findPS2:=CGA;
      4,10:findPS2:=EGA;
         5:findPS2:=HERC;
       7,8:findPS2:=VGA;
     11,12:findPS2:=MCGA;
   end
 end
 else
   findPS2:=OTHER;
end;

function whatvsystem:vsystem;
var
 ts:vsystem;
begin
 ts:=findPS2;
 if ts=other then
    ts:=findEGA;
 if ts=other then
    ts:=findmono;
 if ts=other then
    ts:=findCGA;
 whatvsystem:=ts;
end;

end.
