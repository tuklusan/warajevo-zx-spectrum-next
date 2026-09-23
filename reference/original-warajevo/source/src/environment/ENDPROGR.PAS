{$F+}
unit endprogram;
interface
procedure err;
var oldexit:Pointer;

implementation
uses crt,dos,setup,drivers,moucur;
function multchar(c:char;count:byte):string;
var value:String;
begin
  if count<=0 then begin multchar:=''; exit end;
  value[0]:=chr(count);
  fillchar(value[1],count,c);
  multchar:=value;
end;

procedure err;
var
  errstr:string;
  a,duzina:integer;

begin
  exitproc:=oldexit;
  a:=exitcode;
  case a of
      0:errstr:='Program finished';
      1:errstr:='Invalid DOS function code';
      2:errstr:='File not found';
      3:errstr:='Path not found';
      4:errstr:='Too many files';
      5:errstr:='File acess denied';
      6:errstr:='Invalid file handle';
      8:errstr:='Not enough memory';
     12:errstr:='Invalid file access code';
     15:errstr:='Invalid drive number';
     16:errstr:='Can not remove current directory';
     17:errstr:='Can not rename accross drives';
    100:errstr:='Disk read error';
    101:errstr:='Disk write error';
    102:errstr:='File not assigned';
    103:errstr:='File not open';
    104:errstr:='File not open for input';
    105:errstr:='File not open for output';
    106:errstr:='Invalid numeric format';
    150:errstr:='Drive is write protected';
    151:errstr:='Unknown unit';
    152:errstr:='Drive not ready';
    153:errstr:='Unknown command';
    154:errstr:='CRC error in data';
    155:errstr:='Bad request of size';
    156:errstr:='Disk seek error';
    157:errstr:='Unrecognized media';
    158:errstr:='Sector not found';
    159:errstr:='Printer out of paper';
    160:errstr:='Device write fault';
    161:errstr:='Device read fault';
    162:errstr:='Hardware failure';
    200:errstr:='Division by zero';
    201:errstr:='Range check error';
    202:errstr:='Stack overflow error';
    203:errstr:='Heap overflow error';
    204:errstr:='Invalid pointer operation';
    205:errstr:='Floating point overflow';
    206:errstr:='Floating point underflow';
    207:errstr:='Invalid floating point operation';
    208:errstr:='Overlay manager not installed';
    209:errstr:='Overlay file read error';
    210:errstr:='Object not initialized';
    211:errstr:='Call to abstract method';
    212:errstr:='Stream registration error';
    213:errstr:='Collection index out of range';
    214:errstr:='Collection overflow error';
     else errstr:='Runtime error';
  end;
  if a=0 then halt;
  duzina:=length(errstr);
  if not(odd(duzina)) then begin errstr:=errstr+' '; inc(duzina) end;
  textcolor(0); textbackground(7);
  gotoxy(20,7);
  writeln('É'+chr(205)+'['+chr(254)+']'+multchar(chr(205),31)+'»');
  gotoxy(20,8);
  writeln('º'+multchar(' ',8)+'Unrecoverable error'+multchar(' ',8)+'º');
  gotoxy(20,9);
  writeln('º'+multchar(' ',(35-duzina) div 2)+errstr+
    multchar(' ',(35-duzina) div 2)+'º');
  gotoxy(20,10);
  writeln('º'+multchar(' ',7)+'Press any key to stop'+multchar(' ',7)+'º');
  gotoxy(20,11);
  writeln('È'+multchar(chr(205),35)+'¼');
  delay(2000);
  textcolor(7); textbackground(0);
  repeat until keypressed;
  clrscr;
  if gmode then donevideo;
  halt;
end;

end.