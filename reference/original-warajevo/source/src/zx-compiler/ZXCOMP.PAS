{$A+,B-,D-,E-,F+,I-,L-,N-,O-,R-,S-,V-}
{$M 5000,0,655300}

uses ImpComp,Dos,Obj48,Obj128;

const Obj48Len=32853;
  Obj128Len=48651;

type Buffer=array[0..65519] of char;

var InFile,OutFile:text;
  InFile1:file of char;
  InBuf,OutBuf,InBuf2:^Buffer;
  LongLen,OutLen:longint;
  Len,Len1,Len2,I,J,PackLen,SigLen,SegAddr,OfsAddr,ObjLen:word;
  Dummy,ErrCode,ErrCode1:integer;
  InName,OutName,ParsName,TapeName,Dir,Name,Ext,Par,Err:string;
  ExitSave:pointer;
  Ver128:Boolean;

procedure Error(Msg:string);

begin
  WriteLn(Msg); Halt(1);
end;

procedure Fatal;

begin
  if ErrorAddr<>nil then
    WriteLn(#13#10'Fatal error, may be there is not enough memory');
  ExitProc:=ExitSave; ExitCode:=1; ErrorAddr:=nil;
end;

begin
  Ver128:=false;
  ExitSave:=ExitProc; ExitProc:=@Fatal;
  if Paramcount=0 then begin
    WriteLn('ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿');
    WriteLn('³ Warajevo Z80 compiler (release 2.0), by Zeljko Juric & Samir Ribic ³');
    WriteLn('ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ');
    WriteLn;
    WriteLn('Syntax: ZXCOMP <z80_file> [<exe_file> [<initial_switches>]]');
    WriteLn;
    WriteLn('Type ZXCOMP /? to get quick help on switches (for more details, see manual)');
    Halt
  end;
  if ParamStr(1)='/?' then begin
    WriteLn('/!             Do not warn on 386 virtual mode');
    WriteLn('/%<num>        Run compiled code with <num> percent of normal Spectrum speed');
    WriteLn('/%MAX          Run compiled code on max possible speed');
    WriteLn('/J<code>       Joystick emulation setting:');
    WriteLn('                 A = Kempston with analog joystick, B = Sinclair 2 with arrows,');
    WriteLn('                 C = Cursor joystick with arrows, K = Kempston with arrows,');
    WriteLn('                 M = Kempston with mouse, S = Sinclair 1 with arrows');
    Write('/M<mode>       Atribute mode on non EGA/VGA cards (or pallete settings for /VW):');
    WriteLn('                 0 = shading, 1 = white on black, 2 = black on white');
    WriteLn('/Q<sync>       Set video sync to scan line  <sync> (range 0-311)');
    WriteLn('/S<mask>       AY channel mask using beeper (128 only)');
    WriteLn('                 1 = mask channel A, 2 = mask channel B, 3 = mask channel C');
    WriteLn('                 Add values to mask more than one channel');
    WriteLn('/SA            Use AdLib compatibile sound card for AY emulation (128 only)');
    WriteLn('/TAPE[:<name>] Use simple Save/Load support (see the manual)');
    WriteLn('/TAPE!         Use extended Save/Load support (see the manual)');
    WriteLn('/U<status>     Border and sound settings (sync border is VGA only):');
    WriteLn('                 0 = none, 1 = async border, 2 = sound, 3 = sound + async');
    WriteLn('                 border, 4 = sync border, 5 = sound + sync border (default 3)');
    WriteLn('/V<code>       Force graphic cards:');
    WriteLn('                 C = CGA mono, W = CGA color, E = EGA, V = VGA, M = EGA mono,');
    WriteLn('                 H = Hercules, X = Extended Hercules mode (if Herc present)');
    WriteLn('/Z<mask>       Inhibit some function keys:');
    Write('                 1 = F4, 2 = F5, 4 = F6 (add to inhibit more than one key)');
    Halt
  end;
  InName:=ParamStr(1);
  FSplit(InName,Dir,Name,Ext);
  if Ext='' then Ext:='.Z80';
  InName:=Dir+Name+Ext;
  if ParamCount=1 then begin
    OutName:=Dir+Name+'.EXE';
    TapeName:=Name+'.DAT'
  end
  else begin
    OutName:=ParamStr(2);
    FSplit(OutName,Dir,Name,Ext);
    if Ext='' then Ext:='.EXE';
    OutName:=Dir+Name+Ext;
    TapeName:=Name+'.DAT'
  end;
  for I:=1 to Length(InName) do InName[I]:=UpCase(InName[I]);
  for I:=1 to Length(OutName) do OutName[I]:=UpCase(OutName[I]);
  ParsName:=''; Err:='Invalid switch';
  if ParamCount>2 then
    for I:=3 to ParamCount do begin
      Par:=ParamStr(i);
      for J:=1 to Length(Par) do Par[j]:=UpCase(Par[J]);
      if Length(Par)<2 then Error(Err);
      if Par[1]<>'/' then Error(Err);
      if Par[2] in ['M','S','U','V','Z'] then
        if Length(Par)<>3 then Error(Err);
      case Par[2] of
        'V':if not (Par[3] in ['C','E','H','M','V','X','W']) then Error(Err);
        'J':begin
          if (Par[3]<>'A') and (Length(Par)<>3) then Error(Err);
          if not (Par[3] in ['A','B','C','K','M','S']) then Error(Err);
          if (Par[3]='A') and (Length(Par)<>3) then begin
            if Length(Par)<>15 then Error(Err);
            for j:=4 to 15 do if not (Par[J] in ['0'..'9']) then Error(Err)
          end
        end;
        'M':if not (Par[3] in ['0'..'2']) then Error(Err);
        'Z':if not (Par[3] in ['0'..'7']) then Error(Err);
        'S':if not (Par[3] in ['0'..'7','A']) then Error(Err);
        'U':if not (Par[3] in ['0'..'5']) then Error(Err);
        '!':if Length(Par)<>2 then Error(Err);
        '%':begin
          if Par<>'/%MAX' then begin
            Val(Copy(Par,3,255),Dummy,ErrCode);
            if (ErrCode<>0) or (Dummy=0) then error(Err)
          end
        end;
        '%':begin
          Val(Copy(Par,3,255),Dummy,ErrCode);
          if (ErrCode<>0) or (Dummy>311) then error(Err)
        end;
        'T':begin
          if Par='/TAPE' then Par:='/€'+TapeName
          else if Par='/TAPE!' then Par:='/'
          else if (Copy(Par,1,6)='/TAPE:') and (Length(Par)>6) then begin
            FSplit(Copy(Par,7,255),Dir,Name,Ext);
            if Ext='' then Ext:='.DAT';
            Par:='/€'+Dir+Name+Ext
          end
          else Error(Err)
        end
        else error(Err)
      end;
      ParsName:=ParsName+Par+' '
    end;
  for I:=0 to 127 do ParsName:=ParsName+#13;
  New(InBuf); New(OutBuf);
  Assign(InFile,InName); Reset(InFile);
  Assign(InFile1,InName); Reset(InFile1);
  if IOResult<>0 then Error('Error opening file '+InName);
  for I:=0 to 35 do Read(InFile,InBuf^[I]);
  Close(InFile); Reset(InFile);
  if IOResult<>0 then Error('Error reading file '+InName);
  if InBuf^[30]<>#23 then Error('File '+InName+' is not "Warajevo" Z80 file');
  if InBuf^[34]=#0 then Ver128:=false
  else if InBuf^[34]=#3 then Ver128:=true
  else Error('Cannot compile this hardware settings');
  LongLen:=FileSize(InFile1); Len:=FileSize(InFile1);
  if LongLen>131030 then Error('File '+InName+' is too long');
  if Ver128 then begin
    SegAddr:=Seg(Comp128); OfsAddr:=Ofs(Comp128); ObjLen:=Obj128Len;
    new(InBuf2);
    Len1:=LongLen div 2;
    Len2:=LongLen-Len1;
    for I:=0 to Len1-1 do Read(InFile,InBuf^[I]);
    for I:=0 to Len2-1 do Read(InFile,InBuf2^[I])
  end
  else begin
    SegAddr:=Seg(Comp48); OfsAddr:=Ofs(Comp48); ObjLen:=Obj48Len;
    for I:=0 to Len-1 do Read(InFile,InBuf^[I])
  end;
  Close(InFile); Close(InFile1);
  if IOResult<>0 then Error('Error reading file '+InName);
  WriteLn(#13#10'Length:    ',LongLen:6);
  Write('Compiling: ');
  Assign(OutFile,OutName); Rewrite(OutFile);
  for I:=0 to ObjLen-1 do Write(OutFile,Chr(Mem[SegAddr:OfsAddr+I]));
  if IOResult<>0 then Error('Error writing file '+OutName);
  if not Ver128 then begin
    Compress(InBuf,OutBuf,Len+1,PackLen,SigLen,0);
    WriteLn(#8#8#8#8#8#8,LongLen:6);
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,Chr(Lo(SigLen)),Chr(Hi(SigLen)));
    for I:=0 to PackLen-1 do Write(OutFile,OutBuf^[I]);
    OutLen:=PackLen+136;
    PackLen:=65400-PackLen;
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,#255#255)
  end
  else begin
    Compress(InBuf,OutBuf,Len1+1,PackLen,SigLen,0);
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,Chr(Lo(SigLen)),Chr(Hi(SigLen)));
    for I:=0 to PackLen-1 do Write(OutFile,OutBuf^[I]);
    OutLen:=PackLen+8;
    PackLen:=65528-PackLen;
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,#255#255);
    Write(#8#8#8#8#8#8);
    Compress(InBuf2,OutBuf,Len2+1,PackLen,SigLen,Len1);
    WriteLn(#8#8#8#8#8#8,LongLen:6);
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,Chr(Lo(SigLen)),Chr(Hi(SigLen)));
    for i:=0 to packlen-1 do write(outfile,outbuf^[i]);
    OutLen:=OutLen+PackLen+136;
    PackLen:=65396-PackLen;
    Write(OutFile,Chr(Lo(PackLen)),Chr(Hi(PackLen)));
    Write(OutFile,#255#255)
  end;
  for I:=1 to 128 do Write(OutFile,ParsName[I]);
  Close(OutFile);
  if IOResult<>0 then Error('Error writing file '+OutName);
  WriteLn('Code:      ',OutLen+ObjLen:6)
end.

