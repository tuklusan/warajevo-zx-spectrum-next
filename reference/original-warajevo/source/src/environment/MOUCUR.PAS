unit moucur;
interface
type whichcur=(ARROW,HOURGLAS);
var actcur:whichcur;
gmode:Boolean;
procedure hourglascur;
procedure arrowcur;

implementation


Const
   arrcursor: Array [0..31] Of Word = (
	$3fff,$1fff,$fff,$7ff,$3ff,$1ff,$ff,$7f,
	$3f,$1f,$1f,$e0ff,$f07f,$f07f,$f87f,$f87f,
	$0,$4000,$6000,$7000,$7800,$7c00,$7e00,$7f00,
	$7f80,$7fc0,$c00,$600,$600,$300,$300,$0);
{   hsx : Word = 1;
   hsy : Word = 0;
}
Const
   hourcursor: Array [0..31] Of Word = (
	$8001,$8001,$8001,$8001,$c003,$e007,$f00f,$f81f,
	$f81f,$f00f,$e007,$c003,$8001,$8001,$8001,$8001,
	$0,$3ffc,$3ffc,$2024,$1288,$890,$420,$240,
	$2c0,$420,$9d0,$1768,$2dd4,$377c,$3ffc,$0);
{   hsx : Word = 8;
   hsy : Word = 8;}
procedure hourglascur;
begin
  if not(gmode) then exit;
  actcur:=HOURGLAS;
  asm
   mov ax,9;
   mov bx,8;
   mov cx,8;
   mov dx,SEG hourcursor
   mov es,dx
   mov dx,OFFSET hourcursor
   int $33
   mov ax,1
   int $33
  end;
end;
procedure arrowcur;
begin
  if not(gmode) then exit;
  actcur:=ARROW;
  asm
   mov ax,9;
   mov bx,1;
   mov cx,0;
   mov dx,SEG arrcursor
   mov es,dx
   mov dx,OFFSET arrcursor
   int $33
  end
end;
end.
