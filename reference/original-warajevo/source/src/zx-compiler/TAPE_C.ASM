;
;          WARAJEVO Spectrum Simulator    Release 1.6
;                (C) Zeljko Juric & Samir Ribic
;

           .186

           extrn   NOP_:near

           public  EXPLODE,DEC_LEN,PARAMSBUF
           public  BLOCKLOAD,BLOCKSAVE
           public  EASYLOAD,EASYSAVE,ENDSS

           if      VER_128

           public  DEC_LEN_2

           endif

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   MEMSEG:word,STEKSEG:word

           if      VER_128

           extrn   MMUVAR:word

           endif

EXPBUFSEG  dw      ZXEXPBUF            ; Pokazivac na ZXEXPBUF
DSEG2SEG   dw      ZXDSEG2             ; Pokazivac na ZXDSEG2

           if      VER_128

DSEG3SEG   dw      ZXDSEG3             ; Pokazivac na ZXDSEG3

           endif

IXDENAME   db      8 dup (0),'.DAT',0

SSEG1      ends

SSEG2      segment public

           extrn   IX:word,FLAGSL:byte,R:byte

           if      VER_128

           extrn   ACT_AUX:word

           endif

PARAMSBUF  db      128 dup (?)         ; Bafer sa ugradjenim parametrima

POINT_LOW  dw      ?                   ; Niza rijec pointera na RAM blok
POINT_HIGH dw      ?                   ; Visa rijec pointera na RAM blok

EXP_LEN    dw      ?                   ; Duzina ulaznog bafera      NE
SIG_LEN    dw      ?                   ; Duzina signatura          MRDAJ !

DEC_LEN    dw      ?                   ; Duzina otpakovanog bloka

           if      VER_128

DEC_LEN_2  dw      ?                   ; Duzina drugog dijela otpakovanog bloka

           endif

LDLEN      dw      ?                   ; Duzina bloka za ucitavanje
HANDLER    dw      ?                   ; Privremeni handler

ENDSS      label   byte                ; Ovdje je kraj stek segmenta

SSEG2      ends

CSEG2      segment public

           extrn   SETTIMER:far,RESTTIMER:far
           extrn   BUZZ:far

           assume  cs:CSEG2,ss:SSEG

;
; Rutina za otpakivanje. DS pokazuje na PSP.
;

TREE       macro   L0,L1
           call    NEXT_BIT
           jc      L1
           jmp     short L0
           endm

EXPLODE    proc    far
           pusha
           mov     es,ds:[002CH]
           xor     di,di
           xor     al,al
           cld
FINDN1:    cmp     al,es:[di]
           je      FINDN2
           mov     cx,65535
           repne   scasb
           jmp     FINDN1
FINDN2:    add     di,3
           mov     dx,di
           push    es
           pop     ds
           mov     ax,3D00H
           int     21H
           mov     bx,ax
           push    ss
           pop     ds
           mov     ax,4202H            ; Nadji prvi pointer
           mov     cx,-1
           mov     dx,-132
           int     21H
           mov     cx,4                ; Ucitaj ga
           lea     dx,POINT_LOW
           mov     ah,3FH
           int     21H
           mov     cx,128              ; Ucitaj parametarski blok
           lea     dx,PARAMSBUF
           mov     ah,3FH
           int     21H
           mov     ax,4202H            ; Pomjeri se tamo gdje pokazuje pointer
           mov     cx,POINT_HIGH
           mov     dx,POINT_LOW
           int     21H
           mov     cx,4
           if      VER_128
           push    dx                  ; Zapamti ovo mjesto
           push    ax
           lea     dx,POINT_LOW        ; Ucitaj drugi pointer
           mov     ah,3FH
           int     21H
           mov     ax,4201H            ; Pomjeri se tamo gdje on pokazuje
           mov     cx,POINT_HIGH
           mov     dx,POINT_LOW
           int     21H
           mov     cx,4                ; Ucitaj duzinu i duzinu signatura
           lea     dx,EXP_LEN
           mov     ah,3FH
           int     21H
           mov     cx,65535            ; Ucitaj prvi blok
           xor     dx,dx
           mov     ds,EXPBUFSEG
           mov     ah,3FH
           int     21H
           mov     es,DSEG2SEG         ; Otpakuj ga u DSEG2
           push    bx
           call    EXPL_MAIN
           pop     bx
           pop     dx
           pop     cx
           mov     ax,4200H            ; Pomjeri se na zapamceno mjesto
           int     21H
           push    ss
           pop     ds
           mov     cx,4                ; Ucitaj pokazivac
           lea     dx,POINT_LOW
           mov     ah,3FH
           int     21H
           mov     cx,4                ; Ucitaj duzine za drugi blok
           lea     dx,EXP_LEN
           mov     ah,3FH
           int     21H
           mov     cx,65535            ; Ucitaj drugi blok
           xor     dx,dx
           mov     ds,EXPBUFSEG
           mov     ah,3FH
           int     21H
           mov     ah,3EH              ; Zatvori datoteku
           int     21H
           mov     es,DSEG3SEG         ; Otpakuj blok u DSEG3
           push    DEC_LEN
           call    EXPL_MAIN
           mov     ax,DEC_LEN
           mov     DEC_LEN_2,ax
           pop     DEC_LEN
           else
           lea     dx,EXP_LEN
           mov     ah,3FH
           int     21H
           mov     cx,65535
           xor     dx,dx
           mov     ds,EXPBUFSEG
           mov     ah,3FH
           int     21H
           mov     ah,3EH
           int     21H
           mov     es,DSEG2SEG
           call    EXPL_MAIN
           endif
           popa
           ret
EXPLODE    endp

EXPL_MAIN: mov     si,SIG_LEN          ; Bajtovi pocinju iza signatura
           inc     si
           xor     bp,bp               ; Pokazivac na signature
           xor     di,di               ; Pokazivac u izlazni bafer
           mov     ah,ds:[bp]          ; Pripremi prvu signaturu
           mov     dl,8                ; Brojac bitova
           cld
EXP_LOOP:  TREE    E_GET,GOBACK        ; Zavisno od signature prepisi bajt
E_GET:     movsb                       ; ili se vrati
           cmp     si,EXP_LEN
           jbe     EXP_LOOP
           dec     di
           mov     DEC_LEN,di
           ret
GOBACK:    TREE    B0,B1               ; Hafmanovo stablo za broj bajtova koji
B0:        TREE    B00,B01             ; se prepisuju prilikom vracanja
B1:        TREE    B10,B11
B11:       TREE    B110,B111
B01:       TREE    B010,B011
B10:       TREE    B100,B101
B110:      TREE    B1100,B1101
B111:      TREE    B1110,B1111
B010:      mov     cx,2                ; Duzina 2 moze da ima samo jednobajtni
           lodsb                       ; doseg, inace nema nikakve ustede
           xor     bh,bh
           mov     bl,al
           jmp     GB_END
B00:       mov     cx,3                ; Duzine bajtova mozemo zakljuciti iz
           jmp     short V             ; signatura preko Hafmanovog stabla
B100:      mov     cx,4
           jmp     short V
B101:      mov     cx,5
           jmp     short V
B011:      lodsb                       ; Ovo je signatura za vise od 9 bajtova
           xor     ch,ch               ; ciji se tacan broj dobije posebno
           mov     cl,al
           add     cx,10
           jmp     short V
B1100:     mov     cx,6
           jmp     short V
B1101:     mov     cx,7
           jmp     short V
B1110:     mov     cx,8
           jmp     short V
B1111:     mov     cx,9
V:         lodsb                       ; Nizi bajt udaljenosti sekvence
           xor     bh,bh
           mov     bl,al
           TREE    V0,GB_END           ; Hafmanovim stablom racunamo visi bajt
V0:        TREE    V00,V01             ; udaljenosti sekvence
V00:       TREE    V000,V001
V01:       rept    4                   ; Visi bajt veci od 6
           call    NEXT_BIT
           rcl     bh,1
           endm
           add     bh,7
           jmp     short GB_END
V000:      TREE    V0000,V0001
V001:      TREE    V0010,V0011
V0010:     TREE    V00100,V00101
V0011:     TREE    V00110,V00111
V0000:     mov     bh,1
           jmp     short GB_END
V0001:     mov     bh,2
           jmp     short GB_END
V00100:    mov     bh,3
           jmp     short GB_END
V00101:    mov     bh,4
           jmp     short GB_END
V00110:    mov     bh,5
           jmp     short GB_END
V00111:    mov     bh,6
GB_END:    neg     bx                  ; Pocinje prepis CX bajtova iz izlaznog
GB_LOOP:   mov     al,es:[di+bx+1]     ; bafera u izlazni
           stosb
           loop    GB_LOOP
           cmp     si,EXP_LEN
           ja      EXP_END
           jmp     EXP_LOOP
EXP_END:   dec     di
           mov     DEC_LEN,di
           ret

NEXT_BIT:  shl     ah,1
           dec     dl
           jz      NEXT_SIG
           ret
NEXT_SIG:  inc     bp
           mov     ah,ds:[bp]
           mov     dl,8
           ret

CSEG2      ends

CSEG       segment public

           assume  cs:CSEG,ss:SSEG

MAKENAME:  mov     ax,IX               ; Pravi ime zasnovano na DE i IX
           lea     si,IXDENAME
           mov     ds,STEKSEG
           call    ONECH
           call    ONECH
           call    ONECH
           call    ONECH
           mov     ax,dx
           call    ONECH
           call    ONECH
           call    ONECH
ONECH:     rol     ax,4
           mov     bl,al
           and     bl,15
           add     bl,'0'
           cmp     bl,'9'
           jbe     ONECH1
           add     bl,7
ONECH1:    mov     [si],bl
           inc     si
           ret

EASYLOAD:  inc     R                   ; Ucitavanje bloka sa kreiranim imenom
           pusha
           push    ds
           call    MAKENAME
           lea     si,IXDENAME
           call    BLMAIN
           pop     ds
           popa
           jmp     NOP_

EASYSAVE:  inc     R                   ; Snimanje bloka sa kreiranim imenom
           pusha
           push    ds
           call    MAKENAME
           lea     si,IXDENAME
           call    BSMAIN
           pop     ds
           popa
           jmp     NOP_

BLOCKLOAD: inc     R                   ; Ucitavanje bloka sa zadanim imenom
           inc     si
           call    BLMAIN
           jmp     NOP_

BLMAIN:    pusha                       ; Jezgro rutine za ucitavanje
           push    ds
           pushf
           push    0
           popf
           call    RESTTIMER
           mov     LDLEN,dx
           mov     ax,3D00H
           mov     dx,si               ; Ime je odmah iza instrukcije osim
           int     21H                 ; ako nije EASYLOAD kada se ime kreira
           jc      BLERROR             ; iz IX i DE
           mov     HANDLER,ax
           mov     bx,ax
           mov     ds,DSEG2SEG
           xor     dx,dx
           mov     ah,3FH
           mov     cx,65535            ; Ucitaj blok u bafer
           int     21H
           jc      BLERROR
           cmp     ax,LDLEN
           jne     BLERROR
           mov     ah,3EH
           int     21H
           jc      BLERROR
           mov     ds,MEMSEG
           xor     di,di
           mov     bx,IX
           mov     cx,LDLEN
LDBLKLOOP: push    es                  ; Prenos bafera u glavnu memoriju
           mov     es,DSEG2SEG
           mov     ah,es:[di]
           pop     es
           cmp     bh,high 16384
           jb      SKIPBYTE
           mov     [bx],ah
           if      VER_128
           mov     es:[bx],ah
           cmp     bx,MMUVAR
           jbe     SKIPBYTE
           call    ACT_AUX
           endif
SKIPBYTE:  inc     bx
           inc     di
           loop    LDBLKLOOP
           mov     FLAGSL,1            ; C fleg se setuje ako je sve OK
LDEXIT:    call    SETTIMER
           popf
           pop     ds
           popa
LDSKIPLP:  cmp     byte ptr [si],0     ; Preskoci ime i azuriraj PC
           lea     si,[si+1]
           jne     LDSKIPLP
           dec     si
           ret

BLERROR:   mov     FLAGSL,0            ; C se resetuje u slucaju greske
BSERROR:   mov     ah,3EH
           int     21H
           call    BUZZ
           jmp     LDEXIT

BLOCKSAVE: inc     R                   ; Snimanje bloka sa zadanim imenom
           inc     si
           call    BSMAIN
           jmp     NOP_

BSMAIN:    pusha                       ; Jezgro rutine za snimanje
           push    ds
           pushf
           push    0
           popf
           call    RESTTIMER
           mov     LDLEN,dx
           mov     ah,3CH
           mov     dx,si               ; Ime je kao za BLMAIN
           xor     cx,cx
           int     21H
           jc      BSERROR
           mov     HANDLER,ax
           mov     bx,ax
           mov     dx,IX
           mov     ds,MEMSEG
           mov     cx,LDLEN
           mov     ah,40H
           int     21H
           jc      BSERROR
           cmp     ax,cx
           jne     BSERROR
           mov     ah,3EH
           int     21H
           jmp     LDEXIT

CSEG       ends

ZXDSEG     segment public
ZXDSEG     ends

           if      VER_128

ZXDSEG3    segment public
ZXDSEG3    ends

           endif

ZXDSEG2    segment public
ZXDSEG2    ends

;
; Ovaj segment se koristi za dekompresiju i snapshoot.
;

ZXEXPBUF   segment public
ZXEXPBUF   ends

           end
