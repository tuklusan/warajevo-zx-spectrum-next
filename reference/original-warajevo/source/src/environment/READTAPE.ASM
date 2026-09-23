
           .model  tpascal

           public  TESTSB,READTAPE,SAMPLETAPE,COPYTODOS

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
LASTAL    db      ?
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
;           add     dx,4                ; MIXER port ima offset 4
;           mov     al,REG
;           out     dx,al
;           inc     dx
;           in      al,dx
;           sub     dx,5
           endm

WRMIX      macro   REG,VAL
;           add     dx,4
;           mov     al,REG
;           out     dx,al
;           inc     dx
;           mov     al,VAL
;           out     dx,al
;           sub     dx,5
           endm
;
; function ReadTape(var Buffer; LTC1,LTC2,STC1,STC2,TC1,TC2,TC3,TC4:byte;
;   SBAddr:word; BusyBit:Boolean; var Sound,CRC:Boolean):byte;
;

BUFFERPTR  equ     dword ptr [bp+34]
LTC1       equ     byte ptr [bp+32]
LTC2       equ     byte ptr [bp+30]
STC1       equ     byte ptr [bp+28]
STC2       equ     byte ptr [bp+26]
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

MKRD2:     mov STATUS,0
           CALL INIT_DEVICE
           cmp STATUS,1
           jumps
           je ERROR
           cmp STATUS,4
           je END_LOAD
           nojumps
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
;           in      al,dx
;           and     al,80h
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

LDLEADER:  mov     ch,LTC1             ; 6500 T
           CSIZE   21
           call    LDEDGE2
           CSIZE   12
           jnc     LDBREAK
           CSIZE   -5
           cmp     ch,LTC2             ; 3000 T korekcija
           CSIZE   23
           jb      LDSTART
           CSIZE   -5
           inc     bh
           CSIZE   16
           jne     LDLEADER
           CSIZE   -5

LDSYNC:    mov     ch,STC1             ; 3655 T
           CSIZE   21
           call    LDEDGE1
           CSIZE   12
           jnc     LDBREAK
           CSIZE   -5
           cmp     ch,STC2
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

;           int     3  ; DEBUG !!!

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
           jumps
           jz      EDGE_BAD
           nojumps
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

;    in al,dx
           call    READBIT
           mov     SAVEAL,al
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
;           or      al,8
           push    ax
           mov     ah,al  ; Zbog drugih portova boje su malo pomjesane
           cmp     ah,7   ; White
           jne     CL_GREEN
           mov     ah,14  ; Bijela u zutu
           jmp     short SET_BORDER
CL_GREEN:  cmp     ah,2
           jne     CL_YELLOW
           mov     ah,3   ; Zelena u Cyan
           jmp     short SET_BORDER
CL_YELLOW: cmp     ah,6
           jne     SET_BORDER
           mov     ah,4   ; Zuta u crvenu
SET_BORDER:
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
           rol     al,1
           rol     al,1
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
           rol     al,1
           rol     al,1
           out     61H,al
RDBULV:    pop     ax
           ret

READBUSY   endp

READSB     proc    near
           push    dx
           add     dx,0eh          ; DSP Data available
           in      al,dx
           or      al,al
           jns     NOTAVAIL
           sub     dx,4
           in      al,dx
           cmp     al,76H
           ja      VAL_OK
           xor     al,al
VAL_OK:    and     al,80H              ; u najvisem bitu imamo
           mov     LASTAL,al
           add     dx,2
READYCMD:  in      al,dx
           shl     al,1
           jc      READYCMD
           mov     al,20h
           out     dx,al
NOTAVAIL:  mov     al,LASTAL
           push    ax
           cmp     SPEAKERON,0
           je      RDSBLV
           rol     al,1
           rol     al,1
           out     61H,al
RDSBLV:    pop     ax
           pop     dx
           ret

READSB     endp

INIT_DEVICE proc near
           xor     ax,ax
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
           mov     STATUS,1
           jmp     INITEND

NO_WIN:    mov     dx,SBADDR           ; Testiraj SB port adresu.
           or      dx,dx
           jnz     SB_GOOD
           jmp     NO_SB

SB_GOOD:
        mov al,1
        add dx,6            ;dsp_reset
        out dx,al           ;reset
        in al,dx
        in al,dx
        in al,dx
        in al,dx
        mov al,0
        out dx,al
           RDMIX   0CH                 ; Sacuvaj MIXER registre prije nego sto
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
           WRMIX   02EH,0FFH           ; Line-in voluce
           WRMIX   028H,0              ; CD volume
           WRMIX   0AH, 0              ; MIC volume
           WRMIX   022H, 0FFH          ; Master volume
datawait:
        mov dx,SBADDR
        add dx,0eh           ;DSP_DATA_AVAIL
        in al,dx
        add al,al
        jc gotdata          ;a byte is waiting...
        loop datawait
noblaster:
        mov STATUS,4
        jmp short INITEND
gotdata:
        mov dx,SBADDR
        add dx,0ah           ;DSP_READ_DATA
        in al,dx
        cmp al,0AAh         ;Is it the ID byte?
        je YepSB            ;Found the ID byte
        loop datawait
        jmp noblaster
YepSB:
        mov dx,SBADDR
        add dx,0ch           ;DSP_WRITE_DATA and DSP_WRITE_STATUS
waitforsb:
        in al,dx
        shl al,1
        jc waitforsb        ;wait until ok to write
        mov al,020h          ;DIRECT_ADC (tell SB to sample a byte.  Will take some
        out dx,al           ; time to complete)
        mov dx,SBADDR
NO_SB:    in      al,61H              ; Sacuvaj stanje porta 61H
          mov     COPY61,al
INITEND:  ret
INIT_DEVICE endp


.386
SIZEB      equ word ptr [bp+6]
TOADDR     equ dword ptr [bp+8]
FROMADDR   equ dword ptr [bp+12]

COPYTODOS  proc far
           push bp
           mov bp,sp
           push ds
           les di,TOADDR
           mov esi,FROMADDR
           xor ax,ax
           mov ds,ax
           mov cx,SIZEB
CPLP:      mov al,[ds:esi]
           mov [es:di],al
           inc esi
           inc di
           loop CPLP
           pop ds
           mov sp,bp
           pop bp
           ret
COPYTODOS  endp

SOUND      equ byte ptr [bp+6]
BUSYBIT    equ byte ptr [bp+8]
SBADDR     equ word ptr [bp+10]
TOADD      equ dword ptr [bp+12]
FROMADD    equ dword ptr [bp+16]
DELAY      equ word ptr [bp+20]

SAMPLETAPE  proc far
            push bp
            mov bp,sp
            mov ax,0
            mov es,ax
            mov al,SOUND
            mov SPEAKERON,al
           mov     READBIT,offset READSTR
           cmp     BUSYBIT,0
           je  short    SKRD1
           mov     READBIT,offset READBUSY
SKRD1:     cmp     SBADDR,0
           je  short    SKRD2
           mov     READBIT,offset READSB

SKRD2:     mov STATUS,0

            call INIT_DEVICE
           mov     dx,SBADDR
           or      dx,dx
           jnz short    STARTSAMP
           mov     dx,LPTPORT

STARTSAMP: mov ebx,FROMADD
           mov ah,1
           CLI

MAINLOOP:
            mov cx,8
BYTELOOP:  ; in al,dx  ; za sad samo busy pin
            call READBIT
            rcl al,1
            rcl ah,1
COMPENS:    push ax
COMPLOOPS:  READPIT
            mov si,ax
            sub ax,CNTSAVE
            neg ax
            cmp ax,DELAY
            jb short COMPLOOPS
            mov CNTSAVE,si
            pop ax
            loop BYTELOOP
NEXTBYT:    mov [es:ebx],ah
            mov byte ptr [es:0B8142h],ah
            mov ah,1
            inc ebx
            cmp ebx,TOADD
            jae short EXIT
            in al,60h
            or al,al
            js MAINLOOP ;NEMA TIPKE
            cmp al,1 ; ESC
            je short EXIT
            jmp short MAINLOOP
EXIT:

           push ebx
           sti
           cmp     SBADDR,0
           jz  short    NOSBREST1

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

NOSBREST1:  mov     ah,2                ; Obnovi DOS sat iz RTC-a
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
           mov al,20h
           out 20h,al ; EOI da se ne koci tastatura
           pop ebx
           push ebx
           pop ax
           pop dx


            mov sp,bp
            pop bp
            ret
SAMPLETAPE  endp
            end

