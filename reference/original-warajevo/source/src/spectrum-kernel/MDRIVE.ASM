;
;          WARAJEVO Spectrum Simulator    Release 2.0
;                (C) Zeljko Juric & Samir Ribic
;

           .186

           warn

           public  MDV_IN,MDV_OUT,ZXI1_STS,ZXI1_CTRL
           public  ZXI1_IN,ZXI1_OUT,NETFNAME
           public  MDVINIT,MDV_TIMER,ERASECNT
           public  MHANDLES,CARTNAMES,MDVUPDT
           public  NETMODE,BLOCKID,NBUFPOS,NETDATA
           public  NETDELAY,BUSYLEN,FREELEN

           extrn   RSI1_OUT:near,RSI1_IN:near
           extrn   USER_IN:near,USER_OUT:near

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   STEKSEG:word
           extrn   RSI1_C:byte,RSI1_T:byte

;
; Stanja za Network konacni automat
;

CLAIM      equ     1
IDLE       equ     2
BUSY       equ     3
FREE       equ     4
COLLREAD   equ     5
COLLWRITE  equ     6

A          equ     al                  ; Procesorski A registar
E          equ     dl                  ; Procesorski E registar

MDRVSEG    dw      LOGOSEG             ; Pokazivac na microdrive cache

BUSYLEN    dw      5                   ; Network konstante
FREELEN    dw      5
NETDELAY   dw      30

NETINTBL   dw      IN_CLAIM,IN_IDLE,IN_BUSY      ; Akcije za Net IN
           dw      IN_FREE,IN_CLR,IN_CLW

MOTORS     db      255                 ; bi=0 => i-ti motor je ukljucen

NETMODE    db      IDLE                ; U pocetku je Network Idle

SSEG1      ends

SSEG2      segment public

           extrn   OPTIONE:byte
           extrn   FLAGSL:word

ZXI1CTREG  db      ?                   ; Upravljacki registar ZX Interface-a 1

MDVHND     dw      ?                   ; Handler tekuceg kertridza (0=off)
CACHEHND   dw      ?                   ; Handler kertridza koji je u cache-u
GAPCNT     db      ?                   ; Brojac za GAP izmedju sektora
WRITPROT   db      ?                   ; 1 ako je kertridz zasticen, inace 0
ERASECNT   db      ?                   ; Brojac za OUT 239,0 emulaciju
SECPTR     dw      ?                   ; Pokazivac na bajt u sektoru
GAPCHECK   db      ?                   ; <> 0 ako je sektor GAP
MDVHDR     db      ?                   ; 0 = blok, 255 = heder
SECTOR     db      ?                   ; Redni broj sektora u kertridzu
CACHESEC   db      ?                   ; Redni broj sektora za cache
CHGFLAG    db      ?                   ; Da li je cache mijenjan
CARTLEN    db      ?                   ; Duzina kertridza u sektorima
CLTRUNC    db      ?                   ; CARTLEN and 248
CLLAST     dw      ?                   ; (CARTLEN mod 8)*543

MDVBUFFER  db      544 dup (?)         ; Bafer za jedan sektor

PROTTAB    db      8 dup (?)           ; Sadrzi WRITPROT za svih 8 kertridza
MHANDLES   dw      8 dup (?)           ; Sadrzi handlere za svih 8 kertridza
CARTLENS   db      8 dup (?)           ; Duzine kertridza u sektorima

CARTNAMES  db      1024 dup (?)        ; Sadrzi imena kertridza

BLOCKID    dw      ?                   ; Redni broj bloka u mrezi   OVO TROJE
NETBLSIZE  dw      ?                   ; Duzina bloka u mrezi          NE
NETDATA    db      256 dup (?)         ; Bafer za mrezu             MRDAJ !!!

CLAIMBUF   db      ?                   ; Pamti zadnji poslani bajt na 247 claim
NETCOUNT   dw      ?                   ; Odbrojava ponavljanje IN instrukcije
BLKLEN     dw      ?                   ; Duzina bloka koji se ucitava s diska
NBUFPOS    dw      ?                   ; Pozicija u baferu
BITCNT     db      ?                   ; Brojac bita
NETBYTE    db      ?                   ; Bajt koji se trenutno Ÿita sa Neta
LASTID     dw      ?                   ; Posljednji procitani ID u Netu
NETFNAME   db      128 dup (?)         ; Ime Network datoteke

SSEG2      ends

CSEG       segment public

           assume  cs:CSEG,ss:SSEG

;
; Simulacija citanja DATA registra ZX Interface-a 1 (IN 247). Bit b0 je NETWORK
; INPUT, a b7 je RS232 INPUT. Biti b1-b4 su uvijek 1, a b5 i b6 su slucajni.
; RS232 ulaz se ne simulira, tako da ovaj registar uvijek vraca 30 ili 31.
;

ZXI1_IN:   cmp     OPTIONE,1           ; Ako nije /E, ovaj port je korisnicki
           je      NET_IN
           jmp     USER_IN

NET_IN:    mov     bl,NETMODE          ; Network IN emulacija
           xor     bh,bh
           add     bx,bx
           jmp     NETINTBL[bx-2]

IN_CLAIM:  mov     NETMODE,IDLE        ; Claim stanje
           mov     ah,CLAIMBUF
           ret

IN_BUSY:   mov     ah,31               ; Busy stanje
           dec     NETCOUNT
           jz      IN_CLW
           ret

IN_FREE:   mov     ah,30               ; Free stanje
           dec     NETCOUNT
           jz      IN_CLW
           ret

IN_CLR:    mov     ah,NETBYTE          ; Collect Read stanje
           ror     NETBYTE,1
           dec     BITCNT
           jz      IN_CLW
           ret

IN_CLW:    mov     NETMODE,IDLE        ; Postavlja Idle stanje
           ret

IN_IDLE:   mov     ah,30               ; Idle stanje
           call    GETID               ; Uzmi ID blok
           jc      SETBUSY             ; ???
           mov     bx,BLOCKID
           cmp     bx,LASTID
           je      SETFREE
           mov     LASTID,bx
SETBUSY:   mov     NETMODE,BUSY
           mov     bx,BUSYLEN
           mov     NETCOUNT,bx
           ret
SETFREE:   mov     NETMODE,FREE
           mov     bx,FREELEN
           mov     NETCOUNT,bx
           ret

GETID:     mov     BLKLEN,2            ; Uzimanje ID bloka
           jmp     short GETBLOCK1

GETBLOCK:  mov     BLKLEN,260          ; Uzimanje bloka
GETBLOCK1: push    ds
           pusha
           mov     ds,STEKSEG
           call    far ptr RESTTIMER
           lea     dx,NETFNAME
           mov     ax,3D20H            ; Open file (shared mode), deny write,
           int     21H                 ;  read mode
           pushf
           jc      ENDGET
           popf
           mov     bx,ax
           mov     ah,3FH              ; Read file
           mov     cx,BLKLEN
           lea     dx,BLOCKID
           int     21H
           pushf
           mov     ah,3EH              ; Close file
           int     21H
ENDGET:    call    far ptr SETTIMER
           popf
           popa
           pop     ds
           ret

;
; Simulacija upisa u DATA registar ZX Interface-a 1 (OUT 247). Ovaj registar
; ima samo bit b0. Upis u bit b0 salje se na RS232 (u emulatoru na LPT1) ako je
; u upravljackom registru b0=1, a inace se salje na network.
;

ZXI1_OUT:  cmp     OPTIONE,1           ; Ovaj port je korisnicki ako nije /E
           je      ZXI1_OUT1
           jmp     USER_OUT
ZXI1_OUT1: test    ZXI1CTREG,1         ; Testiraj b0 u upravljackom registru
           jz      NET_OUT
           jmp     RSI1_OUT            ; Skoci na RS232 emulaciju preko LPT1

NET_OUT:   cmp     NETMODE,COLLWRITE   ; Network emulacija
           je      DOCOLLWR
           test    FLAGSL,64           ; Sta se tacno desava moze se zakljuciti
           jnz     TESTACC             ;  samo citanjem procesorskih registara
           mov     NETMODE,CLAIM
           mov     CLAIMBUF,ah
           xor     CLAIMBUF,1
           ret
TESTACC:   or      A,A
           jz      STARTCOLL
           jmp     SETFREE
STARTCOLL: mov     NETMODE,COLLWRITE
           mov     BITCNT,9
           mov     NBUFPOS,0
           ret

DOCOLLWR:  or      E,E                 ; Collect Write stanje
           jz      FLSHNTBUF
           cmp     BITCNT,0
           jne     PUSHNTBIT
           mov     BITCNT,9
           cmp     NBUFPOS,256
           adc     NBUFPOS,0
           ret
PUSHNTBIT: xor     ah,1
           rcr     ah,1
           mov     bx,NBUFPOS
           rcr     NETDATA[bx],1
           dec     BITCNT
           ret

FLSHNTBUF: mov     bx,30000            ; Ovim se sprijeŸava blokada ako nije
FLSHNTLP:  call    FLNETMAIN           ;  Network aktivan
           jnc     FLSHNTOK
           dec     bx
           jnz     FLSHNTLP
FLSHNTOK:  ret

FLNETMAIN: mov     NETMODE,IDLE        ; Praznjenje network bafera
           mov     BITCNT,9
           push    ds
           pusha
           call    far ptr RESTTIMER
           mov     ds,STEKSEG
           lea     dx,NETFNAME
           mov     ax,3D02H            ; Open file (compatibility mode),
           int     21H                 ;  read/write
           jc      ENDFLSH
           mov     bx,ax
           mov     ah,3FH              ; Read file
           mov     cx,2
           lea     dx,BLOCKID
           int     21H
           jc      FLSHFAIL
           mov     ah,42H              ; Lseek
           xor     cx,cx
           xor     dx,dx
           mov     al,cl
           int     21H
           inc     BLOCKID
           mov     dx,BLOCKID
           mov     LASTID,dx
           mov     ah,40H              ; Write
           mov     cx,260
           lea     dx,BLOCKID
           int     21H
           jc      FLSHFAIL
           mov     ah,3EH
           int     21H
           xor     ax,ax
           mov     ds,ax
           mov     dx,ds:[46CH]
           add     dx,NETDELAY
FLSHWAIT:  cmp     dx,ds:[46CH]
           ja      FLSHWAIT
           call    far ptr SETTIMER
           clc
           jmp     short FLSHDONE
FLSHFAIL:  mov     ah,3EH
           int     21H
ENDFLSH:   call    far ptr SETTIMER
           stc
FLSHDONE:  mov     NBUFPOS,0
           popa
           pop     ds
           ret

;
; Simulacija statusnog registra (IN 239) ZX Interface-a 1.
;   b0=0 => Kertridz je zasticen ili motor ne radi
;   b1=0 => Sinhronizacioni bit (simulira se stalno nulom radi ubrzanja ROM
;           rutina, osim ako je neformatiran sektor)
;   b2=1 => Detektovan je prostor bez podataka (GAP) - emulator smatra da je
;           GAP svaki neformatirani (tj. los) sektor, a vjestacki se dodaje
;           izvjesni GAP ispred svakog hedera i bloka da se zadovolji rad
;           rutina iz ROM-a
;   b3=1 => RS232 DTR bit (tj. printer je spreman)
;
; Bitovi b4-b7 su uvijek 1.
;

ZXI1_STS:  cmp     OPTIONE,1           ; Ovaj port je korisnicki ako nije /E
           je      ZXI1_STS1
           jmp     USER_IN
ZXI1_STS1: mov     ERASECNT,0
           call    RSI1_IN             ; Ocitaj DTR (u AH ce biti 1111b110), i
           cmp     MDVHND,0            ;  ako motor nije ukljucen, to i vrati
           jz      NOMDV               ;  (GAP, NO SYNC, WRITE PROTECTED)
           mov     bh,GAPCNT
           dec     bh                  ; Umanji GAPCNT i ako je cijeli sektor
           cmp     GAPCHECK,0          ;  GAP, preskoci dalje
           jne     NOSYNC
           and     ah,255-2            ; Deaktiviraj SYNC bit
           and     bh,15
NOSYNC:    cmp     bh,7
           ja      GAP                 ; Prije svakog sektora ima GAP, sve do
           jne     NONEXT              ;  vodeceg signala, kada treba ucitati
           call    far ptr NEXT        ;  slijedeci sektor
NONEXT:    cmp     GAPCHECK,0
           jne     GAP                 ; Ignorisi sektor ako je GAP, inace
           and     ah,255-4            ;  deaktiviraj GAP bit
GAP:       mov     GAPCNT,bh
           mov     bh,WRITPROT         ; Azuriraj WRITE PROTECTED bit
           xor     bh,1
           or      ah,bh
NOMDV:     ret

;
; Simulacija upravljackog registra (OUT 239) ZX Interface-a 1.
;   b0 => COMMS DATA bit: 0 = izlaz na network, 1 - izlaz na RS232; koristi se
;         i za indirektni upis u MOTOR registar
;   b1 => COMMS CLK bit: svaki prelaz 1->0 izaziva prepisivanje stare
;         vrijednosti COMMS DATA bita u MOTOR shift registar, pri cemu u tom
;         registru svaki bit jednak nuli oznacava da pripadni motor radi
;   b2 => R/W' bit: 0 = upis na mikrodrajv, 1 = citanje sa mikrodrajva
;   b3 => ERASE bit: b3=0 znaci da je aktivirano brisanje sadrzaja trake, i
;         koristi se za formiranje GAP-a izmedju sektora; drzanje b3=0 duze
;         vrijeme (uz aktivan motor) bez ikakvog upisa obrisace cijeli sadrzaj
;         kertridza
;   b4 => WAIT bit: koristi se za network
;
; Bitovi b5-b7 nemaju funkciju.
;

ZXI1_CTRL: cmp     OPTIONE,1           ; Ovaj port je korisnicki ako nije /E
           je      ZXI1_CTR1
           jmp     USER_OUT
ZXI1_CTR1: cmp     ah,206              ; WAIT bit
           je      NET_CLCT
           mov     ERASECNT,0
           mov     bh,ZXI1CTREG
           xor     bh,ah               ; Da li su mijenjani MDRV bitovi ?
           and     bh,15
           jz      NOFLUSH
           call    far ptr FLUSH       ; Ako jesu, isprazni cache po potrebi
NOFLUSH:   mov     bh,ZXI1CTREG
           mov     ZXI1CTREG,ah        ; Sacuvaj staru vrijednost upr. registra
           test    ah,1
           jnz     NORSRES             ; Ako je COMMS DATA = 0, treba
           mov     RSI1_C,9            ;  resetovati RS232 interfejs
           mov     RSI1_T,0
NORSRES:   not     ah
           and     ah,bh               ; Da li je COMMS CLOCK : 1->0 ?
           test    ah,2
           jz      NO_CLOCK            ; Kraj ako nije
           shr     bh,1
           rcl     MOTORS,1            ; Pomakni COMMS DATA u MOTORS i pozovi
           call    far ptr MOTOR       ;  potprogram za azuriranje "motora"
NO_CLOCK:  ret

NET_CLCT:  mov     BITCNT,9            ; Kolekcija bita za Network slanje
           cmp     NETMODE,COLLREAD    
           jne     DOLOAD
           cmp     NBUFPOS,256
           adc     NBUFPOS,0
           jmp     short LDPREPC

DOLOAD:    mov     bx,30000            ; Ovdje se ogradjujem od beskonacne
DOLLP:     call    GETBLOCK            ;  petlje ako Net datoteka ne postoji
           jnc     LDOK
           dec     bx
           jnz     DOLLP
           ret
LDOK:      mov     NBUFPOS,0
           mov     NETMODE,COLLREAD
LDPREPC:   mov     bx,NBUFPOS
           mov     bl,NETDATA[bx]
           mov     NETBYTE,bl
           ret


;
; Upis bajta na mikrodrajv (OUT 231). Bajti se baferuju, i izbacuju se na
; kertridz kada se promijeni upravljacki registar, ili kada timer da nalog.
;

MDV_OUT:   cmp     OPTIONE,1           ; Ovaj port je korisnicki ako nije /E
           je      MDV_OUT1
           jmp     USER_OUT
MDV_OUT1:  cmp     MDVHND,0
           je      MDV_CRASH
           mov     GAPCHECK,0          ; Oznaci da nije GAP
           test    ZXI1CTREG,12        ; Upis je dozvoljen samo ako je ERASE
           jnz     MDV_IORET           ;  aktivan (0) i R/W' = WRITE
           mov     bx,SECPTR
           mov     MDVBUFFER[bx],ah    ; Smjesti bajt u bafer
           cmp     bx,543
           adc     bx,0                ; Uvecaj SECPTR, ali ne dozvoli da
           mov     SECPTR,bx           ;  predje 543
MDV_IORET: ret

;
; Citanje bajta sa mikrodrajva (IN 231). Bajti se citaju iz bafera, a bafer se
; ucitava iz kertridza nakon GAP-a koji je simuliran izmedju sektora u rutini
; za citanje statusnog registra, ili kada timer da nalog.
;

MDV_IN:    cmp     OPTIONE,1           ; Ovaj port je korisnicki ako nije /E
           je      MDV_IN1
           jmp     USER_IN
MDV_IN1:   cmp     MDVHND,0
           je      MDV_CRASH
           xor     ah,ah
           cmp     SECPTR,0            ; Vrati 0 ako je bafer prazan, ako je
           je      MDV_IORET           ;  sektor GAP, ako je ERASE aktivan ili
           cmp     GAPCHECK,0          ;  ako je R/W' = WRITE
           jne     MDV_IORET
           mov     bl,ZXI1CTREG
           and     bl,12
           cmp     bl,12
           jne     MDV_IORET
           mov     bx,SECPTR
           mov     ah,MDVBUFFER[bx-1]  ; Uzmi bajt iz bafera
           cmp     bx,543
           adc     bx,0                ; Uvecaj SECPTR, ali ne dozvoli da
           mov     SECPTR,bx           ;  predje 543
           ret

;
; Simulacija IN crasha.
;

MDV_CRASH: sub      si,2               ; Smanji PC za 2, pa emulator nece
           ret                         ;  nikada zavrsiti instrukciju

;
; MDV_TIMER se poziva iz timera, za emulaciju OUT 239,0 i slicnih stvari. Ako
; nije ERASE=0, kertridz se samo "premotava", a inace mu se brise sadrzaj.
;

MDV_TIMER: cmp     MDVHND,0            ; Ako motor ne radi, nista
           je      MDV_GRET
           test    ZXI1CTREG,8         ; Ako je ERASE = 0, napravi GAP sektor
           jz      MDV_WGAP
           call    far ptr NEXT        ; U suprotnom, samo premotaj traku i
           in      al,41H              ;  postavi pointer na slucajno mjesto
           mov     ah,al
           in      al,41H
           and     ax,511
           mov     SECPTR,ax
           ret
MDV_WGAP:  mov     MDVBUFFER[26],255   ; 255 je nemoguc ceksum
           mov     ah,ZXI1CTREG
           and     ZXI1CTREG,255-4     ; Forsiraj WRITE
           mov     SECPTR,15
           call    far ptr FLUSH       ; Snimi GAP heder
           mov     SECPTR,530
           call    far ptr FLUSH       ; Snimi GAP blok
           mov     ZXI1CTREG,ah
MDV_GRET:  ret

CSEG       ends

CSEG2      segment public

           assume  cs:CSEG2

           extrn   SETTIMER:far,RESTTIMER:far

;
; Potprogram MOTOR aktivira odnosno gasi odredjene "motore" mikrodrajva.
;

MOTOR:     push    ax
           mov     WRITPROT,1          ; Pretpostavi da je WRITE PROTECTED
           xor     bx,bx
           mov     ah,MOTORS           ; Analiziraj koji "motori" rade
           mov     al,bl
MDVFIND:   ror     ah,1
           jnc     MDVFOUND
           inc     al
           cmp     al,7
           jb      MDVFIND
           jmp     short MOTORERR      ; Greska (svi motori ugaseni)
MDVFOUND:  cmp     ah,127
           jne     MOTORERR            ; Greska (ukljuceno vise motora)
           mov     bl,al
           mov     al,PROTTAB[bx]      ; Odredi je li kertridz zasticen
           mov     WRITPROT,al
           mov     al,CARTLENS[bx]     ; Postavi duzinu kertridza u sektorima
           add     bl,bl
           mov     bx,MHANDLES[bx]     ; Nadji handler kertridza
           or      bx,bx
           jz      MOTORERR            ; Preskoci neke stvari ako je nula
           mov     CARTLEN,al
           mov     CLTRUNC,al          ; Izracunaj neke pomocne stvarcice:
           and     CLTRUNC,248         ;  CLTRUNC = CARTLEN and 248
           push    dx                  ;  CLLAST = (CARTLEN mod 8) * 543
           mov     dx,543
           and     ax,7
           mul     dx
           pop     dx
           mov     CLLAST,ax
MOTORERR:  pop     ax
           mov     MDVHND,bx           ; Handler ce biti 0 za slucaj greske
EMPTYBUF:  mov     SECPTR,0            ; Signaliziraj "prazan bafer"
           retf


;
; Potprogram FLUSH premjesta sadrzaj bafera u cache, prazni sadrzaj cache-a
; kada dodje vrijeme, i u tom slucaju ucitava nove sektore u cache.
;

FLUSH:     cmp     SECPTR,0            ; Izadji ako je bafer prazan
           je      EMPTYBUF
           test    ZXI1CTREG,4         ; Ako nije WRITE, samo resetuj SECPTR
           jnz     EMPTYBUF
           mov     MDVHDR,0            ; Ako u baferu ima vise od 512 bajta, to
           cmp     SECPTR,512          ;  je blok, inace je zaglavlje
           sbb     MDVHDR,0
           mov     SECPTR,0            ; Resetuj SECPTR i oznaci da nije GAP
           mov     GAPCHECK,0
           cmp     WRITPROT,0          ; Ignorisi snimanje ako je kertridz
           jne     FLUSHEND            ;  zasticen
           pusha
           call    MDV_CACHE           ; Dobavi u DI pointer na sektor u
;          jc      CARTERR             ;  cache-u, ali izadji ako je nastupilo
           push    es                  ;  gasenje motora
           cld                         ; Iskopiraj sektor u cache, ali ignorisi
           mov     es,MDRVSEG          ;  prvih 12 bajta (lead-in signal)
           lea     si,MDVBUFFER[12]
           mov     cx,15               ; Heder ima 15 korisnih bajta a blok 528
           cmp     MDVHDR,0
           jnz     MDV_TRANS
           add     di,cx               ; Ne snimaj blok preko hedera!
           mov     cx,528
MDV_TRANS: rep     movs byte ptr es:[di],ss:[si]
           pop     es
           mov     CHGFLAG,255         ; Oznaci da je cache mijenjan
CARTERR:   popa
FLUSHEND:  call    INCSEC              ; Predji na novi sektor
           retf

;
; Potprogram NEXT ucitava slijedeci sektor u bafer.
;

NEXT:      call    INCSEC              ; Predji na novi sektor
           pusha
           call    MDV_CACHE           ; Dobavi u DI pointer na sektor u
           push    es                  ;  cache-u (uz obavljanje cache
           push    ds                  ;  managementa)
           cld                         
           mov     ds,MDRVSEG          ; Iskopiraj sektor iz cache-a u bafer
           mov     es,STEKSEG
           mov     si,di
           lea     di,MDVBUFFER
           mov     cx,15               ; Duzina hedera je 15 bajta, a bloka 528
           cmp     MDVHDR,0            ;  bajta bez lead-in signala
           jne     MDV_MOVE
           add     si,cx               ; U cache-u je blok odmah iza hedera
           mov     cx,528
MDV_MOVE:  rep     movsb
           pop     ds
           pop     es
MDV_TRASH: lea     bx,MDVBUFFER        ; Provjera da li je sektor GAP ili
           mov     cx,14               ;  sadrzi smislene podatke koristenjem
           xor     al,al               ;  ceksuma po modulu 255 prvih 14 bajta
MDV_CHKS:  add     al,ss:[bx]
           adc     al,1
           jz      MDV_255
           dec     al                  ; Ovaj ceksum ne moze nikada dati 255
MDV_255:   inc     bx
           loop    MDV_CHKS
           sub     al,ss:[bx]          ; Oduzmi izracunati i upisani ceksum
           mov     GAPCHECK,al         ; Rezultat je <>0 ako je GAP
           popa
           mov     SECPTR,1            ; Signal: bafer je napunjen
           retf

;
; Prelazak na novi sektor.
;

INCSEC:    mov     SECPTR,0            ; Resetiraj SECPTR
           xor     MDVHDR,255          ; Ako je bio heder sada je blok itd.
           jz      INCEND
           inc     SECTOR              ; U slucaju da je bio blok, predji na
           push    ax                  ;  novi sektor
           mov     al,CARTLEN
           cmp     SECTOR,al           ; Da li se doslo do kraja kertridza?
           pop     ax
           jb      INCEND
           mov     SECTOR,0            ; Ako jeste, vrati se na pocetak
INCEND:    ret


;
; Long seek potprogram.
;

LSEEK:     mov     al,CACHESEC         ; U AX broj sektora od kojeg pocinje
           xor     ah,ah               ;  cache od 8 sektora
           mov     dx,543
           mul     dx                  ; Sektori su po 543 bajta
           mov     cx,dx
           mov     dx,ax               ; Pointer u datoteci je u CX;DX
           mov     ax,4200H
           int     21H                 ; Pozovi LongSeek
           ret

;
; Rutina MDV_CACHE vrsi cache management: ako je sektor u cache-u, vraca u DI
; pointer na njega, a ako nije, po potrebi prazni cache, ucitava trazeni sektor
; u cache, i takodje vraca pointer na njega u DI.
;

MDV_CACHE: mov     bx,MDVHND           ; Ako se motor ugasio, isprazni cache
           test    bx,bx               ;  preko MDV_CLEAR (uz C=1 na izlazu)
           jz      MDV_CLEAR
           cmp     bx,CACHEHND         ; Ako u cache-u nisu podaci o tekucem
           jne     MDV_LOAD            ;  kertridzu, idi na ucitavanje
           mov     al,SECTOR
           sub     al,CACHESEC
           jc      MDV_LOAD            ; Ako sektor nije u cache-u, idi na
           cmp     al,7                ;  ucitavanje
           ja      MDV_LOAD
           xor     ah,ah               ; U suprotnom, sektor je u cache-u i
           imul    di,ax,543           ;  vrati u DI pointer na njega (uz C=0)
           ret
MDV_LOAD:  push    bx
           call    MDV_CLEAR           ; Prije ucitavanja novog bloka u cache
           pop     bx                  ;  snimi stare podatke ako je potrebno i
           mov     CACHEHND,bx         ;  oznaci 'cache-ira se tekuci kertridz'
           mov     al,SECTOR
           and     al,248              ; Oznaci kojih 8 sektora ide u cache
           mov     CACHESEC,al
           call    RESTTIMER           ; Predji na stari timer
           call    LSEEK               ; Pozicioniraj se u datoteci
           mov     ah,3FH
           mov     cx,8*543            ; Zadnji blok u kertridzu mozda ima
           xor     dx,dx               ;  manje od 8 sektora, ali to ne smeta
           push    ds                  ;  rutini
           mov     ds,MDRVSEG
           int     21H                 ; Ucitaj blok u cache i obnovi timer
           call    SETTIMER
           pop     ds
           mov     al,SECTOR           ; Vrati u DI pointer na sektor (uz C=0)
           and     ax,7
           imul    di,ax,543
           ret

;
; MDV_CLEAR snima cache od 8 sektora na disk ako je potrebno, i brise ga.
;

MDV_CLEAR: mov     bx,CACHEHND         ; Izadji ako je cache prazan
           test    bx,bx
           jz      MDV_CLEND
           cmp     CHGFLAG,0           ; Izadji ako cache nije mijenjan
           je      MDV_CLEND
           push    ds
           call    RESTTIMER           ; Predji na stari timer
           call    LSEEK               ; Pozicioniraj se u datoteci
           mov     ds,MDRVSEG
           xor     dx,dx
           mov     cx,8*543            ; Cache ima 8 sektora
           mov     ah,CLTRUNC
           cmp     CACHESEC,ah         ; Da li je zadnji blok sektora?
           jnz     MDV_NOLB
           mov     cx,CLLAST           ; Zadnji blok moze biti kraci
MDV_NOLB:  mov     ah,40H
           int     21H                 ; Snimi blok i obnovi timer
           call    SETTIMER
           mov     CHGFLAG,0           ; Signal "cache nije mijenjan"
           mov     CACHEHND,0          ; Signal "cache je prazan"
           pop     ds
MDV_CLEND: stc                         ; C=1 je korisno za neke stvarcice
           ret

;
; MDVUPDT azurira cache po izlasku iz programa.
;

MDVUPDT:   call    MDV_CLEAR
           retf

;
; MDVINIT postavlja ime kertridza za mikrodrajv, odredjuje mu parametre, i
; otvara njegov handler.
;

MDVINIT:   push    es
           mov     ZXI1CTREG,238       ; Postavi upr. registar za ZXI1
           inc     bx
           mov     cx,3                ; Razni indikatori su u CX
           mov     al,[bx]
           jumps
           cmp     al,'1'              ; Prvo provjeri broj kertridza (1-8)
           jb      MDVBAD
           cmp     al,'8'
           ja      MDVBAD
           xor     ah,ah               ; Pripremi u SI indeks u tabeli handlera
           sub     al,'1'
           mov     si,ax
           inc     bx
           cmp     byte ptr [bx],';'   ; Dalje mora biti ';'
           jne     MDVBAD
           nojumps
           mov     es,STEKSEG          ; Pripremi se za skaniranje imena
           mov     dx,si
           shl     dx,7
           lea     di,CARTNAMES
           add     dx,di
           mov     di,dx
MDVNAMLP:  inc     bx
           mov     al,[bx]
           cmp     al,13
           je      MDVEXT
           cmp     al,' '
           je      MDVEXT
           cmp     al,'.'
           jne     MDVNAMPUT
           inc     ch                  ; Ime ima ekstenziju
MDVNAMPUT: stosb
           xor     cl,cl               ; Prisutno je ime
           jmp     MDVNAMLP
MDVEXT:    or      ch,ch               ; Ima li ime ekstenziju?
           jnz     MDVEMPTY
           mov     ax,'M.'             ; Ako nema, podrazumijevaj .MDR
           stosw
           mov     ax,'RD'
           stosw
MDVEMPTY:  or      cl,cl               ; Da li je prisutno ime?
           jne     MDVEND              ; Kraj ako nema uz CF=0
           push    ds
           push    bx
           mov     ds,STEKSEG
           mov     ax,4300H            ; Odredi da li je kertridz read-only
           int     21H
           and     cl,1
           mov     PROTTAB[si],cl      ; Obiljezi da li je WRITE PROTECTED
           mov     al,cl
           rol     al,1                ; Otvori kertridz fajl, ali samo za
           xor     al,2                ;  citanje ako je zasticen
           mov     ah,3DH
           int     21H
           pop     bx
           pop     ds
           mov     cl,2
           jc      MDVBAD
           add     si,si
           mov     MHANDLES[si],ax     ; Zapamti njegov handler
           push    bx                  
           mov     bx,ax
           mov     ax,4202H            ; Odredi mu duzinu
           xor     cx,cx
           xor     dx,dx
           int     21H
           mov     bx,543
           div     bx
           pop     bx
           mov     cl,1
           cmp     ax,254
           ja      MDVBAD              ; Predugacak kertridz
           cmp     ax,9
           jb      MDVBAD              ; Prekratak kertridz
           shr     si,1
           mov     CARTLENS[si],al     ; Obiljezi duzinu kertridza
           stc                         ; Signal "OK"
           jmp     short MDVEND
MDVBAD:    clc                         ; Signal "greska"
MDVEND:    dec     bx
           pop     es
           retf

CSEG2      ends

;
; Preko slike "cele" ide cache za microdrive.
;

LOGOSEG    segment public
LOGOSEG    ends

           end

