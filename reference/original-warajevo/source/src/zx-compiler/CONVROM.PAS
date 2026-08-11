var f1,f2:text;
  i,j:integer;
  c1,c2:char;

begin
  assign(f1,'ZX48.ROM'); reset(f1);
  assign(f2,'ROM48.ASM'); rewrite(f2);
  writeln(f2,'ZXDSEG      segment public');
  writeln(f2);
  for i:=1 to 16384 div 16 do begin
    write(f2,'            dw      ');
    for j:=1 to 8 do begin
      read(f1,c1,c2); write(f2,256*ord(c2)+ord(c1));
      if j<>8 then write(f2,',') else writeln(f2)
    end
  end;
  writeln(f2); writeln(f2,'            db 49151 dup (?)');
  writeln(f2); writeln(f2,'ZXDSEG      ends');
  writeln(f2); writeln(f2,'            end');
  close(f1); close(f2)
end.


