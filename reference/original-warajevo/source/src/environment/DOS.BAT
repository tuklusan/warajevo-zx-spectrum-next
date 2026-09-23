rem d:\jezici\turbopas\tpcx warajevo /ud:\jezici\turbopas;d:\jezici\turbopas\tvision;d:\jezici\turbopas\tvdemos; /$R- /$S- /$G+  /$L- /m
rem \jezici\masm\tasm readtape.asm
c:\dosapps\tp8\lib\dcc -cd warajevo /uc:\dosapps\tp8\lib /$R- /$S- /$G+ /$Q- /$D- /$L- /m
call overlay
tppatch warajevo.exe

