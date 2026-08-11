           .model  tpascal

           public  TESTSB,READTAPE

           warn

           .data

INPUT_S    db      ?
LINE_VOL   db      ?
CD_VOL     db      ?
MIC_VOL    db      ?
MASTER_VOL db      ?

COPY61     db      ?

LPTPORT    dw      ?

SPEAKERON  db      ?
CRCON      db      ?

STATUS     db      ?

RAFA       dw      ?
RDE        dw      ?

SAVEAL     db      ?
CNTSAVE    dw      ?

READBIT    dw      ?                   ; Indirektni vektor

           .code

VALUE10    db      10

READPIT    macro                       ; Cita timer 0
           xor     al,al
           out     43H,al
           in      al,40H              ; Citaj nizi bajt
           mov     ah,al
           in      al,40H              ; Citaj visi bajt
           xchg    ah,al
           endm

INIT_COUNT macro
           READPIT
           mov     CNTSAVE,ax          ; Sacuvaj brojac u CNTSAVE
           endm

;
; Function TestSB:word;
;
;   Testira da li je SoundBlaster prisutan. Vraca adresu baznog porta ako
;   jeste, a 0 ako nije.
;

TESTSB     proc    far

           mov     al,36H              ; Inicijalizacija Timer-a
           out     43H,al
           xor     al,al
           out     40H,al
           out     40H,al
           mov     dx,0200H            ; Pretraga pocinje od ovog baznog porta
SBSCAN:    add     dx,6                ; Offset za RESET port je 6
           mov     al,1
           out     dx,al               ; Izvrsi reset
           INIT_COUNT
SBWAIT:    READPIT
           sub     ax,CNTSAVE
           neg     ax
           cmp     ax,30
           jc      SBWAIT              ; Cekaj neko vrijeme
           xor     al,al
           out     dx,al
           mov     cx,100
           add     dx,10-6             ; Offset za READ port je 10
IS_DSP:    in      al,dx
           cmp     al,0AAH             ; READY signal
           je      FOUND_DSP
           loop    IS_DSP
           add     dx,16-10            ; Probaj slijedecu baznu adresu
           cmp     dx,02A0H
           jnz     SBSCAN
           xor     ax,ax
           ret
FOUND_DSP: sub     dx,10
           mov     ax,dx
           ret

TESTSB     endp



CSIZE      macro   N
           lea     di,[di+N]
           endm

TESTBRK    macro
           local   NOESC,NOSOUND,LEAVEBRK
           local   NOKEY,TEST2
           in      al,60H              ; Citaj port tastature
           or      al,al
           js      NOKEY
           cmp     al,1                ; Tipka ESC
           jne     NOESC
           mov     STATUS,3
           jmp     LEAVE_PRG
NOESC:     cmp     al,31               ; Tipka 'S'
           jnz     NOSOUND
           mov     SPEAKERON,1
NOSOUND:   cmp     al,49               ; Tipka 'N'
           jnz     LEAVEBRK
           mov     SPEAKERON,0
LEAVEBRK:  cmp     al,2                ; Tipka '1'
           jnz     TEST2
           mov     CRCON,0
TEST2:     cmp     al,3                ; Tipka '2'
           jnz     NOKEY
           mov     CRCON,1
NOKEY:
           endm

RDMIX      macro   REG
           add     dx,4                ; MIXER port ima offset 4
           mov     al,REG
           out     dx,al
           inc     dx
           in      al,dx
           sub     dx,5
           endm

WRMIX      macro   REG,VAL
           add     dx,4
           mov     al,REG
           out     dx,al
           inc     dx
           mov     al,VAL
           out     dx,al
           sub     dx,5
           endm

;
; function ReadTape(var Buffer; TC1,TC2,TC3,TC4:byte; SBAddr:word;
;   BusyBit:Boolean; var Sound,CRC:Boolean):byte;
;
;



BUFFERPTR  equ     dword ptr [bp+26]
TC1        equ     byte ptr [bp+24]
TC2        equ     byte ptr [bp+22]
TC3        equ     byte ptr [bp+20]
TC4        equ     byte ptr [bp+18]
SBADDR     equ     word ptr [bp+16]
USEBUSY    equ     byte ptr [bp+14]
SOUNDPTR   equ     dword ptr [bp+10]
CRCPTR     equ     dword ptr [bp+6]

READTAPE   proc    far

           push    bp
           mov     bp,sp

           les     di,SOUNDPTR         ; Prebaci varijable u zgodniji format
           mov     al,es:[di]
           and     al,1
           mov     SPEAKERON,al
           les     di,CRCPTR
           mov     al,es:[di]
           and     al,1
           mov     CRCON,al

           mov     READBIT,offset READSTR
           cmp     USEBUSY,0
           je      MKRD1
           mov     READBIT,offset READBUSY
MKRD1:     cmp     SBADDR,0
           je      MKRD2
           mov     READBIT,offset READSB

MKRD2:     xor     ax,ax
           mov     es,ax
           mov     ax,es:[0408H]       ; Nadji adresu LPT1 porta.
           inc     ax
           mov     LPTPORT,ax

           mov     al,36H              ; Inicijalizacija timera
           out     43H,al
           xor     al,al
           out     40H,al
           out     40H,al

           mov     ax,1600H            ; Windows test
           int     2FH
           cmp     ax,1600H
           jz      NO_WIN
           mov     ax,1
           jmp     ERROR

NO_WIN:    mov     dx,SBADDR           ; Testiraj SB port adresu.
           or      dx,dx
           jnz     SB_GOOD
           jmp     NO_SB

SB_GOOD:   RDMIX   0CH                 ; Sacuvaj MIXER registre prije nego sto
           mov     INPUT_S,al          ;   ih postavis na nove vrijednosti
           RDMIX   02EH
           mov     LINE_VOL,al
           RDMIX   028H
           mov     CD_VOL,al
           RDMIX   0AH
           mov     MIC_VOL,al
           RDMIX   022H
           mov     MASTER_VOL,al

           WRMIX   0CH,7               ; Input source, Line-in, filter HighPass
           WRMIX   02EH,0EEH           ; Line-in voluce
           WRMIX   028H,0              ; CD volume
           WRMIX   0AH, 0              ; MIC volume
           WRMIX   022H, 0FFH          ; Master volume

NO_SB:     in      al,61H              ; Sacuvaj stanje porta 61H
           mov     COPY61,al

BEGIN:     cli
           les     si,BUFFERPTR
           add     si,2

           mov     dx,SBADDR
           or      dx,dx
           jnz     LOADBLOCK
           mov     dx,LPTPORT

LOADBLOCK: TESTBRK
           mov     RDE,65535
           mov     RAFA,40H
           call    READBIT
           or      al,2
           mov     bl,al
           cmp     al,al
LDBREAK:   jz      LDSTART
LDSTART:   mov     ch,1
           INIT_COUNT
           xor     di,di
           call    LDEDGE1
           jnc     LDBREAK
           xor     bh,bh
LDWAIT:    mov     cx,10000
           shr     ch,1
LDW2:      nop
           loop    LDW2
           dec     bh
           jne     LDWAIT
           xor     ch,ch
           INIT_COUNT
           mov     di,17
           call    LDEDGE2
           CSIZE   12
           jnc     LDBREAK
           CSIZE   -5

LDLEADER:  mov     ch,09CH             ; 6500 T
           CSIZE   21
           call    LDEDGE2
           CSIZE   12
           jnc     LDBREAK
           CSIZE   -5
           cmp     ch,0C6H             ; 3000 T korekcija
           CSIZE   23
           jb      LDSTART
           CSIZE   -5
           inc     bh
           CSIZE   16
           jne     LDLEADER
           CSIZE   -5

LDSYNC:    mov     ch,0C9H             ; 3655 T
           CSIZE   21
           call    LDEDGE1
           CSIZE   12
           jnc     LDBREAK
           CSIZE   -5
           cmp     ch,0D4H
           CSIZE   23
           jnc     LDSYNC
           CSIZE   -5
           CSIZE   17
           call    LDEDGE1
           jc      SCAN
           jmp     short RETNC

SCAN:      xor     bl,3
           xor     bh,bh
           mov     ch,TC1
           CSIZE   46
           jmp     short LDMARKER

LDLOOP:    mov     al,byte ptr RAFA    ; ?????
           mov     es:[si],ah
           CSIZE   49

LDNEXT:    inc     si
           dec     RDE
           CSIZE   27
           mov     ch,TC2

LDMARKER:  CSIZE   7
           mov     ah,1

LD8BITS:   CSIZE   17
           call    LDEDGE2
           jnc     RETNC
           cmp     ch,TC3
           cmc
           rcl     ah,1
           mov     ch,TC1
           CSIZE   40
           jnc     LD8BITS
           xor     bh,ah
           cmp     RDE,0
           CSIZE   32
           jne     LDLOOP
           CSIZE   -5

           cmp     bh,0
           je      RETNC
           mov     al,bh
           cmp     al,1
RETNC:

END_LOAD:  ; Kako je ovo cisto citanje sa trake bez pomoci emulatora, nema
           ; nacina da znamo kada je kraj. Zato se ovdje dolazi jedino nakon
           ; greske u glavnoj rutini.

           sti

           mov     bx,si
           les     si,BUFFERPTR
           sub     bx,si

           cmp     bx,3                ; Duzina nula? ????
           ja      SA_BLOCK

           mov     STATUS,4
           jmp     short LEAVE_PRG     ; Izadji ...

SA_BLOCK:  sub     bx,3
           mov     word ptr es:[si],bx ; Duzina bloka u .TAP datoteci
           add     bx,2
           mov     STATUS,0            ; Signal "OK"
           cmp     CRCON,0
           jz      LEAVE_PRG

           xor     al,al
           add     si,bx
TESTPAR:   xor     al,es:[si]
           dec     bx
           dec     si
           cmp     bx,1
           ja      TESTPAR
           or      al,al
           jz      LEAVE_PRG
           mov     STATUS,2            ; Signal "CRC greska"

LEAVE_PRG: mov     al,COPY61           ; Obnovi port 61H
           out     61H,al

           int     3  ; DEBUG !!!

           push    dx                  ; Obnovi boju BORDER-a
           mov     dx,03DAH
           in      al,dx
           mov     dx,03C0H
           mov     al,11H
           out     dx,al
           xor     al,al
           out     dx,al
           mov     al,20H
           out     dx,al
           pop     dx

           sti
           cmp     SBADDR,0
           jz      NOSBREST

           mov     al,INPUT_S          ; Obnovi MIXER registre
           WRMIX   0CH,al
           mov     al,LINE_VOL
           WRMIX   2EH,al
           mov     al,CD_VOL
           WRMIX   28H,al
           mov     al,MIC_VOL
           WRMIX   0AH,al
           mov     al,MASTER_VOL
           WRMIX   22H,al

NOSBREST:  mov     ah,2                ; Obnovi DOS sat iz RTC-a
           int     1AH
           mov     al,ch
           call    BCD2DEC
           mov     ch,al
           mov     al,cl
           call    BCD2DEC
           mov     cl,al
           mov     al,dh
           call    BCD2DEC
           mov     dh,al
           xor     dl,dl
           mov     ah,2DH
           int     21H

ERROR:     les     di,SOUNDPTR         ; Azuriraj VAR parametre
           mov     al,SPEAKERON
           mov     es:[di],al
           les     di,CRCPTR
           mov     al,CRCON
           mov     es:[di],al
           mov     al,STATUS           ; Obnovi status

           mov     sp,bp
           pop     bp
           ret

READTAPE   endp

;
; Pomocna procedura za BCD/DEC konverziju.
;

BCD2DEC    proc    near

           mov     dl,al
           shr     al,1
           shr     al,1
           shr     al,1
           shr     al,1
           mul     VALUE10
           and     dl,0FH
           add     al,dl
           ret

BCD2DEC    endp

;
; Procedure LD_EDGE.
;

LDEDGE2    proc    near

           CSIZE   17
           call    LDEDGE1
           CSIZE   11
           jumps
           jnc     RETURN
           nojumps
           CSIZE   -6

LDEDGE1:   CSIZE   7
           mov     al,TC4
LDDELAY:   dec     al
           CSIZE   16
           jnz     LDDELAY

           CSIZE   -5
           push    ax
           TESTBRK
           pop     ax

           clc
           CSIZE   4

LD_SAMPLE: inc     ch
           jz      EDGE_BAD
           CSIZE   15
           push    ax
           push    dx
           push    bx
           mov     dx,0
           mov     bx,1193
           mov     ax,di
           mul     bx
           mov     bx,3500
           div     bx
           mov     di,ax
           pop     bx
           pop     dx

           add     di,4
           push    si

COMPLOOP:  READPIT
           mov     si,ax
           sub     ax,CNTSAVE
           neg     ax
           cmp     ax,di
           jb      COMPLOOP
           mov     CNTSAVE,si
           pop     si

           call    READBIT
           mov     SAVEAL,al
           mov     di,0
           pop     ax
           mov     al,SAVEAL
           xor     al,bl
           and     al,80H
           CSIZE   44
           jz      LD_SAMPLE
           CSIZE   -5
           mov     al,bl
           neg     al
           mov     bl,al
           and     al,7
           or      al,8
           push    ax
           mov     ah,al

           push    dx                  ; Postavi boju BORDER-a
           mov     dx,03DAH
           in      al,dx               ; Citanje VGA Input Status registra
           mov     dx,03C0H
           mov     al,11H              ; Overscan boja
           out     dx,al
           mov     al,ah
           out     dx,al
           mov     al,20H
           out     dx,al
           pop     dx

           stc
           pop     ax
           CSIZE   52
RETURN:    ret

EDGE_BAD:  clc
           ret

LDEDGE2    endp

;
; Procedure za citanje bita.
;

READSTR    proc    near

           in      al,dx
           and     al,10H
           shl     al,1
           shl     al,1
           shl     al,1
           push    ax
           cmp     SPEAKERON,0
           je      RDSTRLV
           shl     al,1
           shl     al,1
           out     61H,al
RDSTRLV:   pop     ax
           ret

READSTR    endp

READBUSY   proc    near

           in      al,dx
           and     al,80H              ; BUSY pin
           push    ax
           cmp     SPEAKERON,0
           je      RDBULV
           shl     al,1
           shl     al,1
           out     61H,al
RDBULV:    pop     ax
           ret

READBUSY   endp

READSB     proc    near

           add     dx,12               ; WRITE port ima offset 12
WRTWAIT:   in      al,dx
           or      al,al
           js      WRTWAIT
           mov     al,20H              ; Posalji READ komandu na WRITE port
           out     dx,al
           add     dx,14-12            ; DATA_AVAIL port ima offset 14
RDWAIT:    in      al,dx
           or      al,al
           jns     RDWAIT
           sub     dx,14-10            ; READ port ima offset 10
           in      al,dx
           sub     dx,10
           cmp     al,0E0H
           ja      VAL_OK
           xor     al,al
VAL_OK:    and     al,80H
           push    ax
           cmp     SPEAKERON,0
           je      RDSBLV
           shl     al,1
           shl     al,1
           out     61H,al
RDSBLV:    pop     ax
           ret

READSB     endp

           end

