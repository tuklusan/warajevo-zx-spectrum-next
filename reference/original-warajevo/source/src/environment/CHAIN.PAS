
{$S-,R-,I-,V-,F-,B-,O+}

  {*********************************************************}
  {*                    CHAIN.PAS 6.00                     *}
  {*        Copyright (c) TurboPower Software 1987.        *}
  {*                 All rights reserved.                  *}
  {*********************************************************}

unit Chain;
  {-Chaining facility for Turbo 4, 5, 5.5, 6.0}

interface

const
  CloseFilesBeforeChaining : Boolean = True;
  {If False: no files are closed before chaining,
       True: all files but StdIn, StdOut, StdErr, StdPrn are closed}

function Chain4(Path, CmdLine : string) : Word;
  {-Chain to file named in Path
    CmdLine must be no longer than 82 characters
    If Chain4 returns, a DOS error code is in the result}

procedure ChainHalt(Path, CmdLine : string);
  {-Execute all exit handlers after the CHAIN unit, then chain as specified}

procedure SetMaxHeap(Bytes : LongInt);
  {-Set maximum heap and adjust DOS memory allocation block}

procedure GetMemDos(var P : Pointer; Bytes : LongInt);
  {-Allocate memory from DOS, returning a pointer to the new block
    Shrink Turbo allocation and relocate free list if forced to
    Returns P = nil if unable to allocate space}

function Pointer2String(P : Pointer) : string;
  {-Convert a pointer to a string suitable for passing on command line}

function String2Pointer(S : string) : Pointer;
  {-Convert a string formatted by Pointer2String to a pointer
    Returns nil if S is an invalid string}

  {==========================================================================}

implementation

var
  SaveExit : pointer;
  ChainPath : string[79];
  ChainCmdLine : string[83];

  {$L CHAIN}
  {$L PTR2STR}

(*

  {$IFDEF Ver60}
  {$L GETMEM6}
  {$ELSE}
  {$L GETMEM}
  {$ENDIF}
*)

{$L GETMEM6}

  function Chain4(Path, CmdLine : string) : Word;
    external {CHAIN} ;

  procedure SetIntVec(Num : Byte; Vec : Pointer);
    external {CHAIN} ;

  procedure SetMaxHeap(Bytes : LongInt);
    external {GETMEM(6)} ;

  procedure GetMemDos(var P : Pointer; Bytes : LongInt);
    external {GETMEM(6)} ;

  function Pointer2String(P : Pointer) : string;
    external {PTR2STR} ;

  function String2Pointer(S : string) : Pointer;
    external {PTR2STR} ;

  procedure RestoreVectors;
    {-Restore SYSTEM interrupt vectors}
  begin
    SetIntVec($00, SaveInt00);
    SetIntVec($02, SaveInt02);
    {$IFNDEF Ver40}
    SetIntVec($1B, SaveInt1B);
    {$ENDIF}
    SetIntVec($23, SaveInt23);
    SetIntVec($24, SaveInt24);
    {$IFNDEF Ver40}
    SetIntVec($34, SaveInt34);
    SetIntVec($35, SaveInt35);
    SetIntVec($36, SaveInt36);
    SetIntVec($37, SaveInt37);
    SetIntVec($38, SaveInt38);
    SetIntVec($39, SaveInt39);
    SetIntVec($3A, SaveInt3A);
    SetIntVec($3B, SaveInt3B);
    SetIntVec($3C, SaveInt3C);
    SetIntVec($3D, SaveInt3D);
    SetIntVec($3E, SaveInt3E);
    SetIntVec($3F, SaveInt3F);
    {$ENDIF}
    SetIntVec($75, SaveInt75);
  end;

  {$F+}
  procedure ChainExit;
    {-Trap on exit, chain if requested}
  var
    Status : Word;
  begin
    ExitProc := SaveExit;
    if Length(ChainPath) <> 0 then begin
      Status := Chain4(ChainPath, ChainCmdLine);
      if Status <> 0 then
        Halt(Status);
    end;
  end;
  {$F-}

  procedure ChainHalt(Path, CmdLine : string);
    {-Execute all exit handlers after the CHAIN unit, then chain as specified}
  begin
    {Save the path and command line to chain to}
    ChainPath := Path;
    ChainCmdLine := CmdLine;
    {Loop through exit chain}
    Halt(0);
  end;

{begin
  SaveExit := ExitProc;
  ExitProc := @ChainExit;
  ChainPath := '';
  }
end.
