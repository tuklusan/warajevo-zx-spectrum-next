{$A-,B-,D+,E+,F+,G-,I+,L+,N-,O+,R-,S-,V-,X-}
{$M 16384,0,655360}
unit TvGraph;

(* Include this unit before any other TVision unit and the program will *)
(* Work entirely in graphics mode and look identical ( although slower) *)
(* to the original text display. Using this in conjunction with a       *)
(* companion unit TVGOBJ, will allow you to do graphics using the full  *)
(* capability of TURBO VISION Menus,Dialog boxes etc.                   *)

interface

uses
  Drivers,Objects,Views,detect,dialogs;

const
  GraphXMax=10000;
  GraphYMax=07500;
  smEGA = $0010;
  smVGA = $0012;
  smGraphAutoDetect=$00FF;   (* Autodetect *)
  wrmOVERWRITE = 0;
  wrmAND = 1;
  wrmOR = 2;
  wrmXOR = 3;

var
  GraphMouseLoc:TPoint; (* Graphics location of Mouse *)


type
 PGRect=^GRect;
 GRect=Trect;

 PGPoint=^GPoint;
 GPoint=TPoint;

procedure initialgr;
procedure patch;
procedure MouseCursorOn;
type
  ABitFont = array[0..255] of array[0..15] of byte;
  (* Store upto 32 line high font *)
  PAbitFont = ^ABitFont;
  GWAv=array[0..4095] of byte;
  PGWAv=^GWAv;
var
  GraphWriteAvail:GWAv;
  FontTable:PABitFont;   (* In Memory font table                         *)
implementation
uses fnt;

const
  (********************************************************************)
  (*                                                                  *)
  (*    CRITICAL - WARNING LOOK AT THIS CONSTANT - VERSION DEPENDANT  *)
  (*                                                                  *)
  (********************************************************************)
  OfsFromInit=$261-$222;
  (* Offset between where the Views.Tview.Init procedure is and where *)
  (* the code starts for the Checksnow = TRUE code                    *)
  (* This value is absolutely critical and may change between         *)
  (* Turbovision releases  WATCH OUT CRITICAL                         *)

  CurrentGraphWindow:byte=1;
  CurrentLineStyle:word=$FFFF;
                        (* Current Line style in 1=line 0=no line       *)
  CurrentLineWriteMode:byte=$00;
                        (* Current Write mode for lines                 *)

type
  (* The pointer to a copy of the currently in use font *)
  PtrRec = record O,S:word; end;  (* used to separate Ofs and Seg     *)
var
  (* All the hardware dependant routines called indirectly *)
  Do_GraphMOV_X,
  NewJmp_GraphMOV_X:pointer;
  Draw_Any_Line_X:procedure (X1,Y1,X2,Y2:word;Color:byte);
  Draw_Horiz_Line_X:procedure (X1,X2,Y:word;Color:byte);
  Draw_Vert_Line_X:procedure (X,Y1,Y2:word;Color:byte);

  P1,P2:Pointer;        (* General pointers used for copies             *)
  BytesPerLine:word;    (* Bytes per line in graphics mode              *)
  VideoBufferSeg:word;  (* Video Segment for display                    *)
  GTVEventRoutine:PtrRec;
                        (* Address of TVision mouse event handler       *)
  GTVEventMask:word;    (* Mask as passed by call to handler install    *)
  OldINT33h:Pointer;
                        (* Old pointers to call for INT 33 and INT 10   *)
  Int33h:Pointer absolute $0000:$00CC;
                        (* INT 33h actual address                       *)
  RomFont:PABitFont absolute $0000:$010C;
                        (* Address of current FONT                      *)
  ColorList:word;       (* Pointer to List of color masks in EGA memory *)
  Count,StartCount:word;(* Counts down the number of char's in GRAPHMOV *)
  DestOfs:word;         (* Destination in EGA of the current mask       *)
  TextSource:PtrRec;    (* Source for next Ch/Attr pair                 *)
  TextDest:word;        (* Normal destination for text 0..screen size   *)
  SaveJump:word;        (* Save jump location for GraphMov              *)
  TvGraphLoc,TvGraphCursor:word;
                        (* Store current text x,y and top/bottom cursor *)
  TvViziFlag:byte;      (* Toggles ON=FF and OFF=00 for cursor          *)
  PtrFontTable:PABitFont;(* Pointer to In Memory font table             *)
  (* one byte for each text position on the screen, 0=text screen and   *)
  (* a number 1-254 representing distinct graphics windows and 255      *)
  (* represents the mouse cursor positions                              *)
  GraphIdSet:set of byte;
  CharWidth,CharHeight:byte;
                        (* Width and Height in text mode                *)
  GraphWidth,GraphHeight:word;
                        (* Width and height in graphics                 *)
  GrView:Byte;
  ErasedCur:Byte;
procedure DisableInterrupts; inline($FA);
procedure EnableInterrupts; inline($FB);

procedure MouseCursorOff;assembler;
asm
 mov ax,0002h
 cli
 pushf
 call [OldInt33h]
 mov erasedcur,1
 sti
end;

procedure MouseCursorOn;assembler;
asm
 mov ax,0001h
 cli
 pushf
 call [OldInt33h]
 mov erasedcur,0
 sti
end;

(*****************************************************************************)
(*                                                                           *)
(*  TVGRAPH Version 1.0 (C) C.L.Burke. Portions (C) Borland                  *)
(*                                                                           *)
(*  Display Specific routines                                                *)
(*                                                                           *)
(*  0. Do_Set_XXXXXX    - sets all of the functions into the procedure array *)
(*                                                                           *)
(*  1. Do_INIT_XXXXXX   - should initialise all the specific hardware        *)
(*  2. Do_DONE_XXXXXX   - should reset all specific hardware                 *)
(*  3. Do_PixelAddr_XXXXXX                                                   *)
(*                      - in ax=y, bx=x | out AH=unshifted bit mask          *)
(*                      - CL=bits to shift BX = byte offset                  *)
(*  4. TVGraphCursorOn_XXXXXX                                                *)
(*                        These 3 routines assume GLOBAL TvGraphCursor       *)
(*                        has Start in H and end in L. GLOBAL TvGraphLoc     *)
(*                        X in L and Y in H                                  *)
(*                      - Turn Cursor on check TvViziFlag                    *)
(*  5. TVGraphCursorOff_XXXXXX                                               *)
(*                      - Turn Cursor off check TvViziFlag                   *)
(*  6. TVUpdateCursor_XXXXXX                                                 *)
(*                      - Update cursor to current specifications            *)
(*  7. Do_GraphMOV_XXXXX- expect same parameters at REP MOVSW to text video  *)
(*                      - CX = Number of words (HI=Attr,LO=Char) to move     *)
(*                      - DS:SI = Source of words                            *)
(*                      - ES:DI = Normal TEXT destination                    *)
(*  8. NewJmp_GraphMOV_XXXXXX                                                *)
(*                      - Replaces the CheckSnow code in Views unit          *)
(*                      - important that the following structure is kept     *)
(*                                                                           *)
(*                        asm                                                *)
(*                         call far ptr Do_GraphMOV_XXXXXX                   *)
(*                         mov si,es                                         *)
(*                         pop ax                                            *)
(*                         pop cx                                            *)
(*                         pop di                                            *)
(*                         pop ds                                            *)
(*                         pop es                                            *)
(*                         db 0C3H   ( RET NEAR - can't use RET FAR!!!  )    *)
(*                        end;                                               *)

const  (* EGA Specific constants *)
  EGAVideoMemory     =$A000;
  EGASequencer       =$03C4;

  BytesInLoop        =10;   (* Bytes in the simulated loop *)
  NumberOfLoops      =16;   (* Number of coded loops *)
var  (* GLobal to free BP in draw line routines *)
 VARSlope:byte; (* 0 = slope < 1 , 1= slope >1 *)
 BitShift,ReInitBit,NegBitShift,
 CharMapPtr,VARVertIncr,VARIncr1,VARIncr2:word;
procedure Do_GraphMOV_EGAVGA; assembler;
asm
(* Expect CX = Number of words (HI=Attr,LO=Char) to move *)
(* Expect DS:SI = Source of words *)
(* Expect ES:DI = Normal TEXT destination *)

    push es
    push bp
    push dx
    push bx  (* Save register that may be changes by a TP procedure *)
    push bp
             (* Instead patching code, patching screen *)
    push si
    push cx
    add cx,cx
@scpatch:
    cmp  byte ptr [si],'('
    je  @testradio
    cmp  byte ptr [si],'['
    je  @testcheck
    cmp byte ptr [si],223
    jne @tstrbt  
    jmp @butbot {Dno buttona}

@tstrbt:  { Desna sjenka buttona}
    cmp byte ptr [si],220
    jne @tstrbtl   
    mov byte ptr[si],'å'

@tstrbtl:
    jmp @newpatch
@testradio:
    cmp byte ptr[si+4],')'
    jne @nw
    cmp byte ptr[si+2],' '
    je @radiooff
    cmp byte ptr[si+2],7
    jne @nw
    mov byte ptr[si],'Ä'
    mov byte ptr[si+2],131
    mov byte ptr[si+4],'Ç'
@nw:
    jmp @newpatch


@newpatch:
    inc si
    loop @scpatch
    pop cx
    pop si
    jmp @normal
    @radiooff:
    mov byte ptr[si],'Ä'
    mov byte ptr[si+2],'Å'
    mov byte ptr[si+4],'Ç'
    
    jmp @newpatch

@testcheck:
    cmp byte ptr[si+4],']'
    jne @nw
    cmp byte ptr [si+2],254
    je @closebox
    cmp byte ptr [si+2],$12
    je @zoombox
    cmp byte ptr [si+2],$18
    je @unzoombox
 
    cmp byte ptr[si+2],' '
    je @checkoff
    cmp byte ptr[si+2],'X'
    jne @nw
    mov byte ptr[si],'Ñ'
    mov byte ptr[si+2],135
    mov byte ptr[si+4],'Ü'
    jmp @newpatch


@butbot:
    push bx
    mov bx,0
@butbotlp:
    cmp byte ptr [bx+si],223
    jne @endbut
    mov byte ptr [bx+si],$89
    add bx,2
    jmp @butbotlp
@endbut:
    mov byte ptr [si+bx-2],$8A
    mov byte ptr [si],'â'
    mov byte ptr [si-2],$88
    pop bx
    jmp @newpatch
@checkoff:
    mov byte ptr[si],'Ñ'
    mov byte ptr[si+2],'Ö'
    mov byte ptr[si+4],'Ü'
    
    jmp @newpatch

@closebox:
    mov byte ptr[si],'Ñ'
    mov al,byte ptr[si+1]
    mov byte ptr[si+3],al
    mov byte ptr[si+5],al
    mov byte ptr[si+2],'ï'
    mov byte ptr[si+4],'Ü'
    cmp byte ptr[si-4],'…'
    jne @endtit
    push si
    sub si,4
@titcaplp:
    cmp byte ptr [si],'ª'
    je @titlpfn
    cmp byte ptr [si],'…'
    je @titcapfl
    cmp byte ptr [si],205
    je @titcapfl
    mov byte ptr [si+1],$F1
    add si,2
    jmp @titcaplp
@titcapfl:
    mov byte ptr[si],' '
    mov byte ptr [si+1],$F1
    add si,2
    jmp @titcaplp
@titlpfn:
    mov byte ptr[si],' '
    mov byte ptr [si+1],$F1
    pop si
@endtit:
    jmp @newpatch

@zoombox:
    mov byte ptr[si],'Ñ'
    mov al,byte ptr[si+1]
    mov byte ptr[si+3],al
    mov byte ptr[si+5],al
    mov byte ptr[si+2],$97
    mov byte ptr[si+4],'Ü'
    jmp @newpatch

@unzoombox:
    mov byte ptr[si],'Ñ'
    mov al,byte ptr[si+1]
    mov byte ptr[si+3],al
    mov byte ptr[si+5],al
    mov byte ptr[si+2],150
    mov byte ptr[si+4],'Ü'
    jmp @newpatch

@normal:
    mov dx,ds
    mov ax,seg @data  (* Set up data segment *)
    mov ds,ax
    mov es,ax
    mov [TextSource.S],dx
    mov [TextSource.O],si
    mov [Count],cx
    mov [StartCount],cx

    mov ax,di
    xor dx,dx
    mov cx,0A0h
    div cx            (* dx= O mod 160 , ax= O div 160 *)
    shr dx,1
    mov bx,dx         (* X=bx  , Y=ax *)

    mul [CharHeight]            (* ax := Y(text)*Rowsperline *)

    mov dx,80 (*BytesPerLine*)
    mul dx
    add bx,ax         (* End Routine PixelAddr10 *)
                      (* BX -> offset to Buffer *)
    mov [DestOfs],bx

    shr di,1          (* Convert index to one with no attributes *)
    add di,offset GraphWriteAvail
    mov [TextDest],di

    mov al,NumberOfLoops
    mov bl,BytesInLoop
    les si,[textsource]
    cmp erasedcur,1
    je @prikazi
    call MouseCursorOff
@prikazi:
    sub al,[CharHeight]    (* Number of loop segments to skip *)
    and al,0Fh             (* ensure in range 0..15 *)
    mul bl                 (* Number of bytes to skip   *)
    add ax,OFFSET @STARTLP1 (* Location to jump to *)
    mov [SaveJump],ax              (* DX has offset to jump to using jmp [dx] *)
@GLP1:
    les si,[TextSource]
    mov ax,es:[si]

    mov bl,ah          (* BL has Attr *)
    mov bh,ah
    mov cl,4
    and bl,00Fh
    and bh,0F0h
    shr bh,cl           (* bh = background, bl = foreground *)
    cmp ah,255           (* graficki rezim *)
    mov di,[TextDest]
    je  @NotText
    mov byte ptr [di],0
    jmp @IsText
@NotText:
    mov byte ptr [di],0
    mov bx,0
    cmp [startcount],1
    je @jedan
    jmp @charplotted
@jedan:
    push ds
    inc si
    inc si
    mov ch,[CharHeight]
    mov bx,[DestOfs]
    mov ds,[VideoBufferSeg]

    mov dx,EgaSequencer
    mov ax,$0102
    out dx,ax
    push si
    push bx
    mov cl,ch
@PLANE0LP:
    mov al,es:[si]
    mov [bx],al
    add si,4
    add bx,80
    dec cl
    jnz @PLANE0LP
    pop bx
    pop si

    mov ax,$0202
    out dx,ax
    push si
    push bx
    inc si
    mov cl,ch
@PLANE1LP:
    mov al,es:[si]
    mov [bx],al
    add si,4
    add bx,80
    dec cl
    jnz @PLANE1LP
    pop bx
    pop si

    mov ax,$0402
    out dx,ax
    push si
    push bx
    add si,2
    mov cl,ch
@PLANE2LP:
    mov al,es:[si]
    mov [bx],al
    add si,4
    add bx,80
    dec cl
    jnz @PLANE2LP
    pop bx
    pop si

    mov ax,$0802
    out dx,ax
    push si
    push bx
    add si,3
    mov cl,ch
@PLANE3LP:
    mov al,es:[si]
    mov [bx],al
    add si,4
    add bx,80

    dec cl
    jnz @PLANE3LP
    mov cx,si
    sub cx,5 (* Nakon dva INC-a, TextSource ce pokazivati gdje treba *)
    pop bx
    pop si
    sti
    pop ds


    mov [TextSource.O],si
    mov cx,10
    mov di,100
    jmp @EndGrap

@IsText:
    xor ah,ah           (* AX has character *)
    les di,[PtrFontTable]  (* shl by 4 = *16 for index into font table *)
@Paint:
    shl ax,cl
    add di,ax           (* ES:DI = font table for character *)

    xor ch,ch
    mov cl,[CharHeight] (* CL has height = times to do @L10 *)

    push ds
    mov ax,[VideoBufferSeg] (* VideoBufferSeg  *)
    mov si,[DestOfs]
    mov ds,ax           (* DS:SI = Screen Address *)

    (* ES:DI = Font, DS:SI = SCreen , AX=Temp, BX= attr, CX=Count, DX=port *)
    (* DS:BP = location of color in ega memory *)
    mov bp,cx
    cli
    mov cx,bp            (* jump to start should be 0 for 16, 2 for 14 *)
    mov dx,EgaSequencer

    push si
    push di
    mov ax,$0102
    out dx,ax

@STARTLP0:
(* 0 *)
    mov ah,es:[di]

@ATTRADJ0:
    push bx
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
    pop bx

    mov [si],ah                    (*  4     /  2    *)
    inc di                         (*  2     /  1    *)
    add si,80 (*BytesPerLine*)     (*  2     /  3    *)
    loop @STARTLP0
    mov cx,bp
    mov ax,$0202
    out dx,ax
    pop di
    pop si
    push si
    push di

    shr bh,1
    shr bl,1

@STARTLP1:
(* 0 *)
    mov ah,es:[di]                         (* CYCLES / BYTES *)

@ATTRADJ1:
    push bx
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
    pop bx

    mov [si],ah                    (*  4     /  2    *)
    inc di                         (*  2     /  1    *)
    add si,80 (*BytesPerLine*)     (*  2     /  3    *)
    loop @STARTLP1
    mov cx,bp
    mov ax,$0402
    out dx,ax
    pop di
    pop si
    push si
    push di

    shr bh,1
    shr bl,1

@STARTLP2:
(* 0 *)
    mov ah,es:[di]

@ATTRADJ2:
    push bx
    shr bh,1
    jc @INKSET2
    shr bl,1
    jc @IZLAZ2
    xor ah,ah
    jmp @IZLAZ2

@INKSET2:
    shr bl,1
    jc @PAPSET2
    not ah
    jmp @IZLAZ2
@PAPSET2:
    mov ah,255
@IZLAZ2:
    pop bx

    mov [si],ah                    (*  4     /  2    *)
    inc di                         (*  2     /  1    *)
    add si,80 (*BytesPerLine*)     (*  2     /  3    *)
    loop @STARTLP2
    mov cx,bp
    mov ax,$0802
    out dx,ax
    pop di
    pop si
    push si
    push di
    shr bh,1
    shr bl,1

@STARTLP3:
(* 0 *)
    mov ah,es:[di]

@ATTRADJ3:
    push bx
    shr bh,1
    jc @INKSET3
    shr bl,1
    jc @IZLAZ3
    xor ah,ah
    jmp @IZLAZ3

@INKSET3:
    shr bl,1
    jc @PAPSET3
    not ah
    jmp @IZLAZ3
@PAPSET3:
    mov ah,255
@IZLAZ3:
    pop bx

    mov [si],ah                    (*  4     /  2    *)
    inc di                         (*  2     /  1    *)
    add si,80 (*BytesPerLine*)     (*  2     /  3    *)
    loop @STARTLP3
    pop di
    pop si
    shr bh,1
    shr bl,1


    pop ds
@CharPlotted:
    inc [TextSource.O]
    inc [TextSource.O]
    inc [TextDest]
    inc [DestOfs]
    dec [Count]
    jnz @GLP1
@Endgrap:
{    call MouseCursorOn}
    pop bp
    pop bx   (* Restore registers *)
    pop dx
    pop bp
    pop es

    retf

end;

procedure NewJmp_GraphMOV_EGAVGA; assembler;
asm
 call far ptr Do_GraphMOV_EGAVGA (* 5 *)
 mov si,es                (* 2 *)  (* Normal Return code *)
 pop ax                   (* 1 *)
 pop cx                   (* 1 *)
 pop di                   (* 1 *)
 pop ds                   (* 1 *)
 pop es                   (* 1 *)
 db 0C3H   (* RET NEAR *) (* 1 *)
end;                    (*TOTAL 13 bytes*)


procedure Do_Set_EGAVGA;
begin
  Do_GraphMOV_X:=@Do_GraphMOV_EGAVGA;
  NewJmp_GraphMOV_X:=@NewJmp_GraphMOV_EGAVGA;
end;

procedure New_EventHandler;assembler;
asm
(* Old event handler divides mouse co-ordinates by 8 all the time   *)
(* New Event handler which multiplies by 8 and divides by character *)
(* and then calls the old event handler resulting in the correct    *)
(* division.                                                        *)
    push si
    push ax
    push bx
    mov si,seg @Data
    mov ds,si

    mov [GraphMouseLoc.X],cx
    mov [GraphMouseLoc.Y],dx

    mov ax,8
    mul dx             (* DX:AX = 8*Y *)
    xor bh,bh
    mov bl,[CharHeight]
    div bx          (* DX:AX = 8*Y/CharHeight *)
    mov dx,ax          (* DX = 8*Y/CharHeight *)

    push dx

    mov ax,8       (* X co-ord *)
    mul cx         (* AX = X*8 *)
    xor bh,bh
    mov bl,[CharWidth]
    div bx         (* AX = X*8/CharWidth *)
    mov cx,ax      (* X <- X*8/CharWidth *)

    pop dx

    pop bx
    pop ax
    pop si
    jmp dword [GTVEventRoutine]
end;

procedure New_INT33h; assembler;
(* New mouse interrupt handler to fix graphics *)
asm
 cli
 push ds
 pushf                        (* call interrupt will call as INT 33h *)
 push ax
 mov ax,seg @Data
 mov ds,ax
 pop ax
 cmp ax,0ch
 jz  @change1
 call [OldInt33h]
 pop ds
 sti
 iret

@Change1:   (* Intercept the change Event routine and save the values *)
 mov [GTVEventMask],CX
 mov cx,es
 mov [GTVEventRoutine.S],Cx
 mov [GTVEventRoutine.O],Dx
 mov CX,[GTVEventMask]
 mov dx,SEG New_EventHandler  (*our Event handler *)
 mov es,dx
 mov dx,OFFSET New_EventHandler
 call [OldInt33h] (* has been intercepted *)

 pop ds
 sti
 iret
end;

(* Turn off Events *)
procedure ResetMouse; assembler;
asm
 mov ax,0
 int 33h
 ret
end;

(* Normal text mode video routines *)
procedure SetVidParams(var Smode,Cline:word);
begin
  asm
    les bx,Smode
    mov ax,es:[bx]
    mov ah,0
    int 10h
    mov ah,1
    les bx,Cline
    mov cx,es:[bx]
    int 10h
  end;
end;

procedure GetVidParams(var Smode,Cline:word);
begin
  asm
    mov ah,0fh          (* Save video mode *)
    int 10h
    mov ah,0
    les bx,Smode
    mov es:[bx],ax
    mov ah,03h
    int 10h
    les bx,Cline
    mov es:[bx],cx
  end;
end;

(* Copy ROM font table to RAM *)
procedure CopyROMfont;
var
  FontPtr:pointer;
  Cnt:byte;
  datoteka:file;
begin
  FontPtr:=RomFont;
  for cnt:=0 to 255 do
  begin
   move(FontPtr^,FontTable^[cnt],CharHeight); (* Insert New Jump for GRAPH_MOV *)
   inc(longint(FontPtr),CharHeight);
  end;
  PtrFontTable:=FontTable;
  makefont;
end;

(* This function replaces the DoneVideo procedure command in Drivers *)
procedure Do_DoneVideo;
var
 CLine,SMode:word;
begin
 DisableInterrupts;
 Int33h:=OldInt33h;
 EnableInterrupts;
 SetVidParams(StartupMode,CursorLines);
end;

(* This code is copied over start of the DoneVideo procedure in Drivers *)
procedure NewJmp_DoneVideo; assembler;
asm
 jmp far ptr Do_DoneVideo (* 5 *)
end;                    (*TOTAL 5 bytes*)

(* This function replaces the SetVideoMode procedure command in Drivers *)
procedure Do_SetVideoMode(Mode:word);
var
  Driver:Vsystem;
begin
  Driver := WhatVsystem;
  if hi(Mode)=$01 then HiresScreen:=TRUE;
  Mode:=Mode and $FF;
  if (Mode>$13) or
    not (Driver in GraphScreenInfo[Mode].SuitableAdapters) or
    (GraphScreenInfo[Mode].ScreenSeg= $0000) then
   begin
     case Driver of
       Mono,Herc:Mode:=smMono;
       EGA:Mode:=smEGA;
       VGA:Mode:=smVGA;
       else Mode:=smCO80;
     end;
   end;
  if GraphScreenInfo[Mode].ScreenWriteType=_GRAPHICS then
  begin
    case Mode of
      smEGA:Do_Set_EGAVGA;
      smVGA:Do_Set_EGAVGA;
    end;
    ScreenMode:=Mode;
    SetVidParams(ScreenMode,CursorLines);
    P1:=@Views.Tview.Init;
    dec(PtrRec(P1).O,OfsFromInit);
    P2:=NewJmp_GraphMOV_X;
    move(P2^,P1^,13); (* Insert New Jump for GRAPH_MOV *)
    HiresScreen:=FALSE;
    CharWidth:=8;
    CharHeight:=Mem[$40:$85];
    ScreenWidth:=GraphScreenInfo[Mode].Xres div CharWidth;
    ScreenHeight:=GraphScreenInfo[Mode].YRes div CharHeight;
    GraphWidth:=GraphScreenInfo[Mode].Xres;
    GraphHeight:=GraphScreenInfo[Mode].YRes;
    CheckSnow:=TRUE;
    BytesPerLine:= ScreenWidth;
    VideoBufferSeg:=GraphScreenInfo[Mode].ScreenSeg;
    TvViziFlag:=0;
    TvGraphLoc:=0;
    TvGraphCursor:=$2000;
    ScreenBuffer:=Ptr(GraphScreenInfo[Mode].ScreenSeg,$0000);
  end else
  begin
    SetVidParams(StartupMode,CursorLines);
    writeln('That mode not supported in TVGRAPH yet ');
    writeln;
    halt(255);
  end;
end;

(* This code is copied over start of the SetVideoMode procedure in Drivers *)
procedure NewJmp_SetVideoMode; assembler;
asm
 jmp far ptr Do_SetVideoMode (* 5 *)
end;                    (*TOTAL 5 bytes*)


(* This function replaces the InitVideo procedure command in Drivers *)
procedure Do_InitVideo;
begin
  GetVidParams(StartupMode,CursorLines);
  Do_SetVideoMode(smGraphAutoDetect);
  ResetMouse;
  DisableInterrupts;
  OldInt33h:=Int33h;
  Int33h:=@New_Int33h;
  EnableInterrupts;
  CopyROMFont;
  fillchar(GraphWriteAvail,sizeof(GraphWriteAvail),#0);
end;

(* This code is copied over start of the InitVideo procedure in Drivers *)
procedure NewJmp_InitVideo; assembler;
asm
 jmp far ptr Do_InitVideo (* 5 *)
end;                (*TOTAL 5 bytes*)

(* This routine patches visual look of window frame, buttons,
   scrollbars, checkboxes and radio buttons. It is HIGHLY version
   dependant, and works with given font
*)

procedure patch;
var p1,p2,p3:Pointer;
  ofschr:word;
  s1:string;
const drawadj:array[0..37] of byte=
   ( $8b,$46,$fb, {mov ax,[bp-5]}
     $31,$FF, {xor di,di}
     $8b,$4e,$f6, {mov cx,[bp-0a]}
     $89,$83,$ec,$fe, {petlja: mov [bp+di+feec],ax}
     $83,$c7,$02, {add di,2}
     $e2,$f7, {loop petlja}
     $3c,$20, {cmp al,' '}
     $74,$19, {jz draw}
     $c6,$86,$ec,$fe,$88, {mov byte ptr [bp+feec],88}
     $8b,$7e,$f6, {mov di,[bp-0a]}
     $01,$ff, {add di,di}
     $c6,$83,$ea,$fe,$8a, {mov byte ptr [bp+di+feea],8a}
     $eb,$08 {jmp draw}
   );
begin
  {**************************
  Patching TButton
  **************************}
  

  p1:=@TButton.DrawState;
  inc(PtrRec(p1).o,$130);
  char(p1^):='å';
  inc(PtrRec(p1).o,$C);
  char(p1^):='ã';
  inc(PtrRec(p1).o,$4);
  char(p1^):='â';
  inc(PtrRec(p1).o,$6c);
  move(drawadj,p1^,38);
  {******************************************
  Patching TFrame
  *******************************************}

  p1:=@TFrame.Draw;
  p2:=p1;
  {Close button}
  dec(PtrRec(p2).o,$11);
  s1:='ÑïÜ';
  move(s1,p2^,4);
  inc(PtrRec(p2).o,6);
  {minimize button}
  s1:='Ñ'#150'Ü';
  move(s1,p2^,4);
  p2:=p1;
  {CTitle colors}
  inc(PtrRec(p2).o,$1D);
  byte(p2^):=$70;
  inc(PtrRec(p2).o,$1F);
  byte(p2^):=$47;
  inc(PtrRec(p2).o,$11);
  byte(p2^):=$47;
  {Eliminate reading predefined palette for title replacing CTitle:=...
  with 3 NOP instructions}
  inc(PtrRec(p2).o,$22);
  byte(p2^):=$90;
  inc(PtrRec(p2).o);
  byte(p2^):=$90;
  inc(PtrRec(p2).o);
  byte(p2^):=$90;
  {Clearing of topmost line in frame}
  p2:=p1;
  inc(PtrRec(p2).o,$186);
  byte(p2^):=$B8; {MOV AX,0 to replace B[I-1] with B[0] }
  inc(PtrRec(p2).o);
  byte(p2^):=$0;
  inc(PtrRec(p2).o);
  byte(p2^):=$0;
  inc(PtrRec(p2).o);
  byte(p2^):=$90; {NOP}
  p2:=p1;
  inc(PtrRec(p2).o,$19C);
  byte(p2^):=$50; {Length is full row }
  { Adjusting of buttons colors}
  inc(PtrRec(p2).o,$74);
  byte(p2^):=$FC; { PUSH[BP-02] -> PUSH [BP-04] }
  inc(PtrRec(p2).o,$2E);
  byte(p2^):=$FC; { PUSH[BP-02] -> PUSH [BP-04] }
  { Adjusting unzoom button}
  inc(PtrRec(p2).o,$3A);
  byte(p2^):=$97; {new character }
 

{*********************************************
  Patching TScrollBar
 *********************************************}
  p1:=@TScrollBar.Init;
  inc(PtrRec(p1).O,$60);
  move(p1^,ofschr,2);
  {Chars of vertical scrollbar}
  mem[dseg:ofschr+0]:=143;
  mem[dseg:ofschr+1]:=144;
  mem[dseg:ofschr+2]:=145;
  mem[dseg:ofschr+3]:=147;
  mem[dseg:ofschr+4]:=178;

  p1:=@TScrollBar.Init;
  inc(PtrRec(p1).O,$81);
  move(p1^,ofschr,2);
  {Chars for horisontal scrollbar}
  mem[dseg:ofschr+0]:=141;
  mem[dseg:ofschr+1]:=142;
  mem[dseg:ofschr+2]:=146;
  mem[dseg:ofschr+3]:=148;
  mem[dseg:ofschr+4]:=178;
{*********************************************
  Patching TRadioButtons
 *********************************************}

  p2:=@TRadioButtons.Draw;
  dec(PtrRec(p2).o,6);
  s1:=' ÄÅÇ ';
  move(s1,p2^,6);

  inc(PtrRec(p2).o,$1C);
  byte(p2^):=131; {middle char when selected}
{*********************************************
  Patching TCheckBoxes
 *********************************************}

  p2:=@TCheckBoxes.Draw;
  dec(PtrRec(p2).o,6);
  s1:=' ÑÖÜ ';
  move(s1,p2^,6);

  inc(PtrRec(p2).o,$1C);
  byte(p2^):=135; {middle char when selected}
{*********************************************
  Patching TInputLine
 *********************************************
  p2:=@TInputLine.Draw;
  inc(PtrRec(p2).o,$12B-$57+1);
  byte(p2^):=$D3;
  p2:=@TInputLine.Draw;
  inc(PtrRec(p2).o,$1FF-$57);
  longint(p2^):=$7EC41BEB;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$458B2606;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$452B2626;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$C7894028;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$BB8DE7D1;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$C636FEF2;
  inc(PtrRec(p2).o,4);
  longint(p2^):=$EB670145;
  inc(PtrRec(p2).o,4);
  byte(p2^):=$BF;
  }
end;
procedure initialgr;
begin
  (* overwrite the start of each of the main functions *)
  New(FontTable);
  P1:=@Drivers.InitVideo;
  P2:=@NewJmp_InitVideo;
  move(P2^,P1^,5);

  P1:=@Drivers.SetVideoMode;
  P2:=@NewJmp_SetVideoMode;
  move(P2^,P1^,5);

  P1:=@Drivers.DoneVideo;
  P2:=@NewJmp_DoneVideo;
  move(P2^,P1^,5);

  fillchar(graphidset,sizeof(graphidset),#0);
end;
end.
