;
; TAPE2TAP.ASM  - (C) 1995 Rui Fernando Ferreira Ribeiro
;

           .286

           .model  tiny
           .code

           warn

CSIZE      macro   N
           lea     di,[di+n]
           endm

PRTSTR     macro
           mov     ah,9
           int     21H
           endm

TESTBRK    macro
           local   NOESC,NOSOUND,LEAVEBRK
           local   NOKEY,TEST2
           in      al,60h              ; Citaj port tastature
           or      al,al
           js      NOKEY
           cmp     al,1                ; Tipka ESC
           jne     NOESC
           jmp     LEAVE_PRG
NOESC:     cmp     al,31               ; Tipka 'S'
           jnz     NOSOUND
           mov     SPEAKER_ON,2
NOSOUND:   cmp     al,49               ; Tipka 'N'
           jnz     LEAVEBRK
           mov     SPEAKER_ON,0
LEAVEBRK:  cmp     al,2                ; Tipka '1'
           jnz     TEST2
           mov     CRCON,0
TEST2:     cmp     al,3                ; Tipka '2'
           jnz     NOKEY
           mov     CRCON,1
NOKEY:
           endm

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
           mov     bp,ax               ; Sacuvaj brojac u BP
           endm

DSP_WRITE  macro   BYTE
           local   WAIT
           add     dx,12               ; WRITE port ima offset 12
WAIT:      in      al,dx
           or      al,al
           js      WAIT
           mov     al,BYTE
           out     dx,al
           sub     dx,12
           endm

DSP_READ   macro
           local   WAIT
           add     dx,14               ; DATA_AVAIL port ima offset 14
WAIT:      in      al,dx
           or      al,al
           jns     WAIT
           sub     dx,14-10            ; READ port ima offset 10
           in      al,dx
           sub     dx,10
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

           org     0100h               ; TINY model

START:     jmp     short START1

RAFA       dw      40H
RDE        dw      65535
SAVEAL     db      0

INPUT_S    db      0
LINE_VOL   db      0
CD_VOL     db      0
MIC_VOL    db      0
MASTER_VOL db      0

SB_ON      db      1                   ; SB detection is ON by default
SPEAKER_ON db      1                   ; speaker is on by default

CRCON      db      1

FILE       db       'W1.TAP',0
VALUE10    db      10
COPY61     db      0

           ;
           ; Oslobodi memoriju (DOS alocira za .COM svu memoriju)
           ;

START1:    push    ds
           pop     es                  ; ES = segment memorijskog bloka
           mov     bx,4096             ; Zauzmi samo 64K (4096 paragrafa)
           mov     ah,4AH
           int     21H

           xor     ax,ax
           mov     ds,ax
           mov     ax,ds:[0408H]
           push    cs
           pop     ds
           inc     ax
           mov     LPTPORT,ax

           call    READCMD             ; Cita komandnu liniju

           ;
           ; Inicijalizacija timera
           ;

           mov     al,36H
           out     43H,al
           xor     al,al
           out     40H,al
           out     40H,al

           lea     dx,COPYRIGHT
           PRTSTR

           mov     ax,1600H            ; Windows test
           int     2FH
           cmp     ax,1600H
           jz      NO_WIN

           lea     dx,W_ERROR
           PRTSTR
           mov     ax,4C01H
           int     21H

NO_WIN:    lea     dx,INST
           PRTSTR

           mov     ah,48H              ; Alokacija bafera
           mov     bx,4096             ; 64K
           int     21H                 ; Segment u AX
           jnc     OK_MEM

MEMERR:    lea     dx,ERROR_MEM
           PRTSTR
           mov     ax,4C02H
           int     21H

OK_MEM:    cmp     bx,4096
           jnz     MEMERR

           mov     es,ax
           cmp     SB_ON,0
           jnz     SB_ENABLE
           jmp     NO_SB

SB_ENABLE: ;
           ; Test da li je SB prisutan
           ;

           mov     dx,0200H            ; Pretraga pocinje od ovog baznog porta

SBSCAN:    add     dx,6                ; Offset za RESET port je 6
           mov     al,1
           out     dx,al
           INIT_COUNT
SBWAIT:    READPIT
           sub     ax,bp               ; Nadji razliku
           neg     ax
           cmp     ax,30
           jc      SBWAIT
           xor     al,al
           out     dx,al

           mov     cx,100
           add     dx,10-6             ; Offset za READ port je 10
IS_DSP:    in      al,dx
           cmp     al,0AAH             ; READY signal
           jz      FOUND_DSP
           loop    IS_DSP

           sub     dx,10
           add     dx,0010H            ; Probaj slijedecu baznu adresu
           cmp     dx,02A0H
           jnz     SBSCAN

           lea     dx,SB_NO_MSG
           jmp     short SBEXIT

FOUND_DSP: sub     dx,10
           mov     SB_BASE,dx

           mov     ax,dx
           and     ax,0f0h
           shr     ax,4
           add     al,'0'
           mov     PORT_SB,al

           lea     dx,SB_FOUND
SBEXIT:    PRTSTR
           mov     dx,SB_BASE

           or      dx,dx
           jnz     SB_GOOD
           jmp     NO_SB

SB_GOOD:   cmp     SPEAKER_ON,2
           jz      NODISSND

;          mov     SPEAKER_ON,0

NODISSND:  ;
           ; Sacuvaj MIXER registre
           ;

           RDMIX   0CH
           mov     INPUT_S,al
           RDMIX   02EH
           mov     LINE_VOL,al
           RDMIX   028H
           mov     CD_VOL,al
           RDMIX   0AH
           mov     MIC_VOL,al
           RDMIX   022H
           mov     MASTER_VOL,al

           ;
           ; Postavi MIXER registre:
           ;

           WRMIX   0CH,7               ; Input source, Line-in, filter HighPass
           WRMIX   02EH,0EEH           ; Line-in voluce
           WRMIX   028H,0              ; CD volume
           WRMIX   0AH, 0              ; MIC volume
           WRMIX   022H, 0FFH          ; Master volume

NO_SB:     in      al,61H              ; Sacumaj stanje porta 61H
           mov     COPY61,al

BEGIN:     cli
           mov     si,2

           mov     SPEAKER_ON,0

           mov     dx,SB_BASE
           or      dx,dx           ; ?????????????
           jnz     LOADBLOCK

           mov     dx,LPTPORT

LOADBLOCK: TESTBRK
           mov     cx,65535
PRAZ:      mov     bx,cx
           mov     byte ptr es:[bx],0
           loop    PRAZ
           mov     si,2
           mov     RDE,65535
           mov     RAFA,40H
           in      al,dx               ; Ovdje je IN !!!
           and     al,80H
           or      al,2
           mov     bl,al
           cmp     al,al
LDBREAK:   jz      LDSTART

LDSTART:   mov     ch,1
           INIT_COUNT
           mov     di,0

           call    LDEDGE1
           jnc     LDBREAK

           xor     bh,bh
LDWAIT:    mov     cx,10000
           shr     ch,1
LDW2:      nop
           loop    LDW2
           dec     bh
           jne     LDWAIT
           mov     ch,0
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

SCAN:

           xor     bl,3
           xor     bh,bh
           mov     ch,0B0h
           CSIZE   46
           jmp     short LDMARKER
LDLOOP:
           mov     al,byte ptr RAFA
           mov     es:[si],ah
           CSIZE   49

LDNEXT:    inc     si
           dec     RDE
           CSIZE   27
           mov     ch,0B2H

LDMARKER:  CSIZE   7
           mov     ah,1

LD8BITS:   CSIZE   17
           call    LDEDGE2
           jnc     RETNC
           cmp     ch,0CBH
           cmc
           rcl     ah,1
           mov     ch,0B0H
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
RETNC:     jmp     END_LOAD

LDEDGE2:   CSIZE   17
           call    LDEDGE1
           CSIZE   11
           jumps
           jnc     RETURN
           nojumps
           CSIZE   -6

LDEDGE1:   CSIZE   7
           mov     al,16H
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
           sub     ax,bp
           neg     ax
           cmp     ax,di
           jb      COMPLOOP
           mov     bp,si
           pop     si
           in      al,dx               ; Ovdje je !
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

           ;
           ; Postavi boju BORDER-a
           ;

           push    dx
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

END_LOAD:  ; Kako je ovo cisto citanje sa trake bez pomoci emulatora, nema
           ; nacina da znamo kada je kraj. Zato se ovdje dolazi jedino nakon
           ; greske u glavnoj rutini.

           mov     al,COPY61
           out     61H,al              ; Obnovi port 61H

           sti

           cmp     si,1                ; Duzina nula?
           ja      SA_BLOCK
           jmp     LEAVEIT             ; Izadji ...

SA_BLOCK:  sub     si,2
           dec     si
           mov     word ptr es:[0],si  ; Duzina bloka u .TAP datoteci
           add     si,2
           cmp     CRCON,0
           jz      OK_2_LD
           xor     al,al
           mov     bx,si
TESTPAR:   xor     al,es:[bx]
           dec     bx
           cmp     bx,1
           ja      testpar
           or      al,al
           jz      OK_2_LD

           lea     dx,TAPE_ERR
           PRTSTR
           jmp     short LEAVEIT

           ;
           ; Otvori datoteku (.TAP)
           ;

OK_2_LD:   mov     ax,3d01H
           lea     dx,FILE
           int     21H
           jnc     FILEOPEN2

           ;
           ; Ako ne postoji, kreiraj ga
           ;

           mov     ah,3CH
           xor     cx,cx
           int     21H
           jc      LEAVE_PRG

FILEOPEN2: mov     bx,ax

           ; Pozicioniraj se na kraj

           mov     ax,4202H
           xor     cx,cx
           xor     dx,dx
           int     21H

           push    ds
           push    es
           pop     ds

           xor     dx,dx               ; Pocetak bloka u DS
           mov     cx,si               ; Duzina bloka (+2)
           mov     ah,40H
           int     21H                 ; Snimi blok

           pop     ds
           mov     ah,3EH              ; Zatvori datoteku
           int     21H

           cmp     si,21               ; Nije blok zaglavlja
           jnz     EXIT
           cmp     byte ptr es:[2],0
           jne     EXIT
           cmp     byte ptr es:[3],0
           jnz     NO_BASIC
           lea     dx,B_PROGRAM
           jmp     short ALL_OK
NO_BASIC:  cmp     byte ptr es:[3],3
           jnz     EXIT
           lea     dx,B_CODE

ALL_OK:    PRTSTR
           xor     bh,bh
           mov     di,4
A_CHAR:    mov     al,es:[di]
           cmp     al,32
           jnc     CHAR_OK
           mov     al,'?'
CHAR_OK:   mov     ah,0EH
           int     10H
           inc     di
           cmp     di,14
           jnz     A_CHAR

EXIT:      lea     dx,MES_GOOD
           PRTSTR

LEAVEIT:   jmp     BEGIN

LEAVE_PRG: mov     al,COPY61
           out     61H,al

           xor     ah,ah
;          call    Border   ; !!!!!!!!!!!!
           sti

           cmp     SB_BASE,0
           jz      NOSBREST

           ;
           ; Obnovi MIXER registre
           ;

           mov     al,INPUT_S
           WRMIX   0CH,al
           mov     al,LINE_VOL
           WRMIX   2EH,al
           mov     al,CD_VOL
           WRMIX   28H,al
           mov     al,MIC_VOL
           WRMIX   0AH,al
           mov     al,MASTER_VOL
           WRMIX   22H,al

NOSBREST:  ;
           ; Obnovi DOS sat
           ;

           mov     ah,2 ; RTC2DOSCLK
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

           mov     ax,4C00H
           int     21H

BCD2DEC:   mov     dl,al
           shr     al,4
           mul     VALUE10
           and     dl,0FH
           add     al,dl
           ret

           if      0

ReadVal		proc	near
;        TESTBRK
                cmp     SB_BASE,0
		jnz	@@use_sb

                in      al,dx           ; read printer-1 status port
;               and     al,10h          ; the value may be used...
                and     al,80h          ; reads busy pin

		jmp	short @@sound

@@use_sb:       ; Read sound-blaster

                DSP_WRITE       020h    ; send read command
		DSP_READ
                cmp     al,0e0h
		ja	val_ok
		xor	al,al
val_ok:		test    al,080h

@@sound:        pushf
		cmp	byte ptr ds:[speaker_on],0
		jz	@@leave

		pushf
		popf

@@noisy:                           ; bleed bit read to beeper
		pushf
;       shr al,3
        shr al,6        ; modified for busy pin
		out	61h,al


		popf

@@leave:	popf
		ret
ReadVal		endp

                endif

;
; Cita argumente iz komandne linije
;

READCMD:   mov     ah,51H
           int     21H
           mov     es,bx               ; DS=PSP
           mov     bx,080H
NEW_CH:    inc     bx
           mov     ah,es:[bx]
           cmp     ah,0DH              ; Kraj linije?
           jz      RDEXIT
           cmp     ah,'d'              ; Da li treba ignorisati SB?
           jnz     NEW_CH
           mov     SB_ON,0
RDEXIT:    ret

;
; Podaci.
;



; colour          db      0

LPTPORT    dw      0379H
B_PROGRAM  db      13,10,'Program : $'
B_CODE     db      13,10,'Bytes   : $'

W_ERROR    db      13,10,'This program cannot be run under Windows'
           db      13,10
           db      'It must run outside a multitasking environment'
           db      13,10,'$'

COPYRIGHT  db      13,10
           db      'Tape2Tap v1.12 --- reads ZX Spectrum`s tapes in Pc'
           db      13,10
           db      'Copyright December 1995, Rui Fernando Ferreira Ribeiro'
           db      ' [aka Star Man]'
           db      13,10,13,10,'$'

INST       db      0dh, 0ah
           db      'You can press at any time:'
           db      0dh, 0ah, 0dh, 0ah
           db      ' - ESC exit program', 0dh, 0ah
           db      ' - S   turn speaker on', 0dh, 0ah
           db      ' - N   turn speaker off', 0dh, 0ah
           db      ' - 1   disable block CRC', 0dh, 0ah
           db      ' - 2   enable  block CRC', 0dh, 0ah
           db      0dh, 0ah
           db      '$'

ERROR_MEM  db      0dh, 0ah
           db      ' Not enough free memory to run the program'
           db      0dh, 0ah, '$'

TAPE_ERR   db      0dh, 0ah
           db      ' Tape loading error! Rewind and try again this block'
           db      0dh, 0ah, '$'

MES_GOOD   db      0dh, 0ah
           db      ' Tape block load suceffuly'
           db      0dh, 0ah, '$'

SB_BASE    dw      0

SB_NO_MSG  db      0dh, 0ah
           db      ' SoundBlaster not found'
           db      0dh, 0ah
           db      ' Using printer port to digitalize data.'
           db      0dh, 0ah, 0dh, 0ah, '$'

sb_found   db      0dh, 0ah
           db      ' SoundBlaster found at port 2'
port_sb    db      '00', 0dh, 0ah
           db      'Using it to digitalize data.'
           db      0dh, 0ah, 0dh, 0ah, '$'

stackseg   segment para STACK 'STACK'    ; Definition of stack segment

           db 512 dup ('S')        ; The stack comprises 256 words

stackseg   ends                          ; End of stack segment

           end     START

