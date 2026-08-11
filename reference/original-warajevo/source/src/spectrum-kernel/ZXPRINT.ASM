;
;          WARAJEVO Spectrum Simulator    Release 2.0
;                (C) Zeljko Juric & Samir Ribic
;

           warn

;
; Simulacija ZX printera je u drugom segmentu. Prvo se definise intersegmentni
; interface.
;

SSEG       group   SSEG1,SSEG2

;
; Varijable za simulaciju ispisa na ZX printer.
;

SSEG1      segment public

ZXP_CNT    dw      256                 ; Brojac karaktera u baferu

XCOORD     dw      25                  ; X koordinata za Laser Jet
YCOORD     dw      100                 ; Y koordinata za Laser Jet

BCOUNT     db      0                   ; Brojac bajtova za Laser Jet

;
; Kontrolni kodovi za stampac.
;

EPSCODES   db      27,51,6,27,42,5,0,2           ; Epson

HPCODES    db      27,'*t150R',27,'*p010'        ; HP Laser Jet

LASTASCX   db      '0X',27,'*r1A',27,'*p010'
LASTASCY   db      '0Y',27,'*b01W'

SSEG1      ends

SSEG2      segment public

           extrn   WINTROUB:byte

LAST_OUT   db      ?                   ; Zadnja poslana vrijednost na port 251
ZXP_BUF    db      256 dup (?)         ; Interni bafer
ZXP_MOTOR  db      ?                   ; FF ako "motor" radi, inace 0
ZXP_ROW    db      ?                   ; Odstampani red (0-3) puta 2

;
; Rezim simulacije: 0 - nema printera, 1 - Epson standardni, 2 - Epson uvecani,
; 3 - HP Laser Jet standardni, 4 - HP Laser Jet uvecani
;

PRTMODE    db      ?

SSEG2      ends

CSEG       segment public

           assume  cs:CSEG,ss:SSEG

           public  ZXP_IN,ZXP_OUT,PRTMODE

ZXP_IN:    call    far ptr Z_IN_FAR
           ret

ZXP_OUT:   push    ax
           call    far ptr Z_OUT_FAR
           pop     ax
           ret

CSEG       ends

CSEG2      segment public

           assume  cs:CSEG2

           extrn   SETTIMER:far,RESTTIMER:far

INT17:     test    WINTROUB,2
           jz      DOINT17
           ret
DOINT17:   int     17H
           ret

;
; Simulacija ZX printera (port 251).
;

;
; Citanje porta 251:
;   b7=0 => "pisaljka" se nalazi izvan papira;
;   b6=1 => ZX printer nije prisutan;
;   b0=1 => ZX printer spreman da prihvati piksel.
;
; Bitovi b0 i b7 se latchuju, medjutim to nije potrebno simulirati. Bit b7 ce
; biti setovan ako je bio setovan bit b7 prilikom zadnjeg slanja na port b7 bez
; obzira na stvarnu poziciju pisaljke (hardversko ogranicenje ZX printera).
;

Z_IN_FAR   proc    far
           push    dx
           push    ds
           cmp     PRTMODE,0           ; Izadji odmah ako se ne emulira printer
           je      ZXP_IN1
           mov     dx,0040H            ; Segment BIOS sistemskih varijabli
           mov     ds,dx
           mov     dx,ds:[0008H]       ; Adresa baznog porta za LPT1
           or      dx,dx               ; Ako je adresa 0, LPT1 nije instaliran
           jz      ZXP_IN1             ;  i izadji sa AH = 0FFH
           inc     dx
           mov     ah,al
           in      al,dx               ; Citaj status printera (LPT1)
           in      al,dx
           mov     dh,ah
           xor     al,60H              ; Invertuj b5 (Paper) i b6 (Ack)
           mov     ah,al
           and     ah,40H              ; Izdvoji b6 jer je on tamo gdje treba
           rol     al,1                ; Bit b7 (Ready) ide u C flag
           adc     ah,0                ; U rezultatu on je na poziciji b0
           rol     al,1                ; Bit b5 (Paper) sada je na poziciji b7
           and     al,80H              ; Izdvoji ga i smjesti u AH
           or      ah,al
           mov     al,LAST_OUT         ; Ako je b7 bio visok u zadnjem ispisu
           and     al,80H              ;  na port 251, bit b7 ce biti 1
           or      ah,al
           or      ah,3EH              ; Bitovi b1-b5 nam nisu interesantni
           mov     al,dh
ZXP_IN1:   pop     ds
           pop     dx
           ret
Z_IN_FAR   endp

;
; Upis na port 251:
;   b2=0 => "motor" radi;
;   b1=1 => motor radi "sporo";
;   b7=1 => "pisaljka" radi.
;

Z_OUT_FAR  proc    far
           mov     al,PRTMODE          ; Ako je uvecani ispis, izmijeni neke
           mov     EPSCODES[7],al      ;  kontrolne kodove
           mov     LAST_OUT,ah
           test    ah,04H              ; Bit b2=0 => "motor" radi
           jz      MOTON
           cmp     ZXP_MOTOR,255       ; Da li je motor radio ?
           jz      MOTOFF              ; Ako jeste, "iskljuci" ga
           ret
MOTOFF:    mov     ZXP_MOTOR,0
           cmp     PRTMODE,3           ; Na Laser Jet-u ima nekih razlika
           jae     HPOFF
           cmp     ZXP_CNT,256         ; Da li je bafer prazan ?
           jz      ZXPEND              ; Ako jeste, kraj
           push    ax                  ; Inace, forsirano ga "isprazni"
           push    si
           mov     ZXP_CNT,256
           jmp     FLUSH
HPOFF:     cmp     ZXP_CNT,8           ; Na laserskom stampacu se baferuje samo
           jz      ZXPEND              ;  jedan bajt, a ne cijeli red
           push    ax
           push    si
           mov     ZXP_CNT,8
           jmp     HPFLUSH
ZXPEND:    ret
MOTON:     cmp     ZXP_MOTOR,0         ; Da li je motor bio iskljucen ?
           mov     ZXP_MOTOR,255       ; Signaliziraj da sad motor radi, i ako
           jne     DOPRINT             ;  nije radio motor, ignorisi bit b7
           mov     ah,255
           call    Z_IN_FAR            ; Testiraj status printera
           test    ah,40H
           jnz     NOPRINTER
           ret
NOPRINTER: mov     ZXP_MOTOR,0         ; Ako nema printera, odbij zahtjev za
           ret                         ;  ukljucenje motora
DOPRINT:   push    ax
           push    si
           mov     LAST_OUT,ah
           and     ah,80H              ; Bit b7=1 => aktivna je "pisaljka"
           mov     al,ah               
           cmp     PRTMODE,3           ; Ako je Laser Jet, idi dalje
           jumps
           jae     HPLASER
           nojumps
           cmp     PRTMODE,2           ; Po potrebi uduplaj piksel
           jne     NOPIXDUP
           shr     al,1
           or      al,ah
NOPIXDUP:  mov     si,ZXP_CNT          ; Smanji brojac primljenih piksela
           dec     si
           cmp     ZXP_ROW,0           ; Da li je to prvi red piksela?
           jnz     NOROW0
           mov     ZXP_BUF[si],al      ; Ako jeste, smjesti piksel u bafer
           jmp     short ZXCONT
NOROW0:    mov     ah,cl               ; Ako nije, smjesti takodje piksel u
           mov     cl,ZXP_ROW          ;  bafer, ali na pravo mjesto
           ror     al,cl
           or      ZXP_BUF[si],al
           mov     cl,ah
ZXCONT:    mov     ZXP_CNT,si          ; Azuriraj brojac
           or      si,si               ; Je li prikupljen cijeli red piksela ?
           jumps
           jnz     NOFLUSH             ; Ako nije, kraj
           nojumps
           mov     ah,ZXP_ROW          ; Predji na novi red piksela
           add     ah,PRTMODE
           mov     ZXP_ROW,ah
           mov     ZXP_CNT,256         ; Resetiraj brojac primljenih piksela
           mov     ZXP_MOTOR,0         ; Piksel u slijedecem bajtu se "guta"
           test    LAST_OUT,02H        ; Da li "motor" radi "sporo" ?
           jnz     FLUSH               ; Ako radi, odmah posalji bafer na LPT1
           cmp     ah,8                ; Inace cekaj da se prikupe 4 reda
           jne     NOFLUSH
FLUSH:     push    cx
           mov     cl,ah
           add     ah,ah
           add     ah,cl
           mov     EPSCODES[2],ah      ; Razmak je ZXP_ROW/72 incha
           mov     ZXP_ROW,0           ; Resetuj brojac redova
           push    dx
           call    RESTTIMER           ; Obnovi standardni timer interrupt
           mov     cx,8                ; Posalji kontrolne kodove (line spacing
           lea     si,EPSCODES         ;  ZXP_ROW/72 inch, graphics mode 576
           xor     dx,dx               ;  columns per line, accept 512 pixels)
ZXP_L1:    mov     al,ss:[si]
           mov     ah,dh
           call    INT17
           inc     si
           loop    ZXP_L1
           mov     cx,256
           lea     si,ZXP_BUF+256      ; Posalji sadrzaj bafera
ZXP_L2:    dec     si
           mov     al,ss:[si]          ; Svaki bajt saljemo 1 ili 2 puta,
           mov     ah,dh               ;  zavisno od PRTMODE
           call    INT17
           cmp     PRTMODE,2
           jne     NOTWOBYT
           mov     ah,dh
           call    INT17
NOTWOBYT:  loop    ZXP_L2
           mov     ax,000DH            ; Posalji <CR><LF>
           call    INT17
           mov     ax,000AH
           call    INT17
           mov     ax,001BH            ; Posalji MASTER RESET
           call    INT17
           mov     ax,0040H
           call    INT17
           call    SETTIMER            ; Postavi opet timer za simulator
           pop     dx
           pop     cx
NOFLUSH:   pop     si
           pop     ax
           ret
HPLASER:   shl     al,1                ; Na laserskom stampacu skupljaj piksele
           rcl     ZXP_BUF[0],1        ;  dok ne skupis 8 piksela
           dec     ZXP_CNT
           jnz     NOFLUSH
           mov     ZXP_CNT,8
HPFLUSH:   push    cx
           push    bx
           push    dx
           lea     bx,LASTASCX         ; Postavi X
           mov     ax,XCOORD
           call    SETASCII
           lea     bx,LASTASCY         ; Postavi Y
           mov     ax,YCOORD
           call    SETASCII
           call    RESTTIMER
           cmp     PRTMODE,4           ; Za vecu sliku su potrebne neke izmjene
           jne     NOBIGHP
           mov     word ptr HPCODES[3],'70'
           mov     byte ptr HPCODES[5],'5'
NOBIGHP:   mov     cx,34               
           lea     si,HPCODES          ; Posalji kontrolne kodove
           xor     dx,dx
HPLOOP:    mov     al,ss:[si]
           mov     ah,dh
           call    INT17
           inc     si
           loop    HPLOOP
           xor     ah,ah               ; Posalji sam bajt
           mov     al,ZXP_BUF
           call    INT17
           mov     ax,27               ; Iskljuci graficki rezim stampaca
           call    INT17
           mov     al,'*'
           call    INT17
           mov     al,'r'
           call    INT17
           mov     al,'B'
           call    INT17
           add     XCOORD,8            ; Azuriraj X koordinatu i brojac bajtova
           inc     BCOUNT
           cmp     BCOUNT,32           ; Da li je novi red?
           jb      HPCONT
           mov     BCOUNT,0            ; Resetuj X koordinatu i brojac bajtova
           mov     XCOORD,25
           mov     ZXP_MOTOR,0         ; Piksel u slijedecem bajtu se "guta"
           inc     YCOORD
           mov     cl,5                ; Izracunaj gdje je kraj strane
           sub     cl,PRTMODE
           mov     ax,264
           shl     ax,cl
           add     ax,100
           cmp     YCOORD,ax           ; Da li je kraj strane?
           jb      HPCONT
           mov     YCOORD,100          ; Resetiraj Y koordinatu i posalji
           mov     ax,12               ;  "form feed"
           call    INT17
HPCONT:    call    SETTIMER            ; Postavi opet timer za simulator
           pop     dx
           pop     bx
           pop     cx
           pop     si
           pop     ax
           ret
Z_OUT_FAR  endp

;
; Procedura SETASCII vrsi ASCII konverziju za HP Laser Jet
;

SETASCII:  mov     cl,PRTMODE          ; Izvrsi uvecanje po potrebi
           sub     cl,2
           shl     ax,cl
           mov     cx,4
           mov     dl,10
ASCLOOP:   div     dl                  ; Pretvori bajt u ASCII znakove
           add     ah,'0'
           mov     ss:[bx],ah
           xor     ah,ah
           dec     bx
           loop    ASCLOOP
           ret

CSEG2      ends

           end
