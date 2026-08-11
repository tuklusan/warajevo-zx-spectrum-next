;
;          WARAJEVO Spectrum Simulator    Release 2.0
;                (C) Zeljko Juric & Samir Ribic
;

           .286

           warn

           include VERSION.ASM

A          equ     al

B          equ     ch
C          equ     cl

DE         equ     dx
D          equ     dh
E          equ     dl

HL         equ     di

AUX        equ     bx
AUX_H      equ     bh
AUX_L      equ     bl

STP        equ     bp
PC         equ     si

           extrn   INCD:near,FARSTOPF:far
           extrn   BUZZ:far,KLIK:far,ULAFAR:far
           extrn   CALL_SLOW:near,CALL_:near
           extrn   INAN:near,INAN_FAST:near
           extrn   SETKBD:far,RESTKBD:far
           extrn   SETTIMER:far,RESTTIMER:far
           extrn   REPAINT:far,BORDONLY:near
           extrn   RET_:near,NOP_:near
           extrn   MAXLSON:far,MAXLSOFF:far
           extrn   SBINSETUP:far,DSPRESET:far

           public  H_POCHRL,V_POCHRL,LD_EDGE,LD_BYTES,SA_BYTES,SAVEFORMON
           public  BRKABLE,EDTABLE,LDCNT,EDGE_ADDR,CREATETAPE,INGTAPE
           public  TMANAGER,TMNAME,SEARCHREC,LOADFORMON,CURBLK
           public  KILLEDGE,PO_CHR,VMADDR,NTH,SA_LD_MON,PO_ONE
           public  LOAD_MODE,STOP_MODE,SETLMODE,X_POCHRL,INV_FLG
           public  EARSAMPLE,TSTSMPBLK,EDGE_AFTER,SPROTECT,SMPSTARTER
           public  DISPDIS

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   TABLE1:word,ULAMODE:byte
           extrn   STEKSEG:word,MEMSEG:word
           extrn   SNAPSEG:word,SAMPLEON:byte
           extrn   KBDROWS:byte,VIDSYNC:word

           if      VER_128

           extrn   ALTMEMSEG:word
           extrn   MMUVAR:word

           endif

;
; Karakter set za upotrebu u tape manageru, itd.
;

CHARSET    dw      0,0,0,0,6144,6168,24,24
           dw      13312,52,0,0,13312,13438,32308,52
           dw      2048,26686,2878,2110,25088,2148,9744,70
           dw      12288,12392,25642,58,6144,48,0,0
           dw      3072,6168,6168,12,12288,6168,6168,48
           dw      0,6196,6270,52,0,6168,6270,24
           dw      0,0,6144,12312,0,0,62,0
           dw      0,0,6144,24,0,3078,12312,96
           dw      15360,27238,29290,60,6144,6200,6168,60
           dw      15360,1638,24636,126,15360,3174,26118,60
           dw      3072,11292,32332,12,32256,31840,25090,60
           dw      15360,31840,25186,60,32256,3078,12312,48
           dw      15360,15458,25186,60,15360,25186,574,60
           dw      0,6168,6144,24,0,6168,6144,12312
           dw      0,6156,6192,12,0,15872,15872,0
           dw      0,6192,6156,48,15360,3174,24,24
           dw      15360,22090,16478,60,15360,25186,25214,98
           dw      31744,31842,25186,124,15360,24674,25184,60
           dw      30720,25188,25698,120,32256,31840,24672,126
           dw      32256,31840,24672,96,15360,24674,25198,60
           dw      25088,32354,25186,98,15360,6168,6168,60
           dw      7680,3084,27756,56,25600,28776,25704,98
           dw      24576,24672,24672,126,25088,27254,25186,98
           dw      25088,27250,26218,98,15360,25186,25186,60
           dw      31744,25186,24700,96,15360,25186,27234,1084
           dw      31744,25186,25724,98,15360,15456,25090,60
           dw      32256,6168,6168,24,25088,25186,25186,60
           dw      25088,25186,9314,24,25088,25186,31330,36
           dw      25088,6196,13336,98,16896,6180,6168,24
           dw      32256,6156,24624,126,7680,6168,6168,30
           dw      0,12384,3096,6,30720,6168,6168,120
           dw      6144,23100,6168,24,0,0,0,-256
           dw      7168,30770,12336,126,0,1080,25660,60
           dw      24576,31840,25186,124,0,24636,24672,60
           dw      1024,15364,25700,60,0,25656,24696,60
           dw      7168,14384,12336,48,0,25660,15460,14340
           dw      24576,30816,25700,100,12288,28672,12336,120
           dw      3072,3072,3084,14444,24576,28776,26736,100
           dw      12288,12336,12336,28,0,27252,27242,106
           dw      0,25720,25700,100,0,25656,25700,56
           dw      0,25720,30820,24672,0,25660,15460,1540
           dw      0,24636,24672,96,0,24632,1080,120
           dw      12288,12408,12336,24,0,25700,25700,56
           dw      0,25700,14376,16,0,27234,27242,52
           dw      0,13410,13336,98,0,25700,15460,14340
           dw      0,3196,12312,124,3584,12312,6192,14
           dw      6144,6168,6168,24,28672,3096,6156,112
           dw      30208,220,0,0,16956,41369,39328,15426

;
; Tabela tokena za Tape manager.
;

TOKENS     db      191,'RN','D'+128,'INKEY','$'+128,'P','I'+128,'F','N'+128
           db      'POIN','T'+128,'SCREEN','$'+128,'ATT','R'+128,'A','T'+128
           db      'TA','B'+128,'VAL','$'+128,'COD','E'+128,'VA','L'+128
           db      'LE','N'+128,'SI','N'+128,'CO','S'+128,'TA','N'+128
           db      'AS','N'+128,'AC','S'+128,'AT','N'+128,'L','N'+128
           db      'EX','P'+128,'IN','T'+128,'SQ','R'+128,'SG','N'+128
           db      'AB','S'+128,'PEE','K'+128,'I','N'+128,'US','R'+128
           db      'STR','$'+128,'CHR','$'+128,'NO','T'+128,'BI','N'+128
           db      'O','R'+128,'AN','D'+128,'<','='+128,'>','='+128
           db      '<','>'+128,'LIN','E'+128,'THE','N'+128,'T','O'+128
           db      'STE','P'+128,'DEF F','N'+128,'CA','T'+128
           db      'FORMA','T'+128,'MOV','E'+128,'ERAS','E'+128
           db      'OPEN ','#'+128,'CLOSE ','#'+128,'MERG','E'+128
           db      'VERIF','Y'+128,'BEE','P'+128,'CIRCL','E'+128,'IN','K'+128
           db      'PAPE','R'+128,'FLAS','H'+128,'BRIGH','T'+128
           db      'INVERS','E'+128,'OVE','R'+128,'OU','T'+128,'LPRIN','T'+128
           db      'LLIS','T'+128,'STO','P'+128,'REA','D'+128,'DAT','A'+128
           db      'RESTOR','E'+128,'NE','W'+128,'BORDE','R'+128
           db      'CONTINU','E'+128,'DI','M'+128,'RE','M'+128,'FO','R'+128
           db      'GO T','O'+128,'GO SU','B'+128,'INPU','T'+128,'LOA','D'+128
           db      'LIS','T'+128,'LE','T'+128,'PAUS','E'+128,'NEX','T'+128
           db      'POK','E'+128,'PRIN','T'+128,'PLO','T'+128,'RU','N'+128
           db      'SAV','E'+128,'RANDOMIZ','E'+128,'I','F'+128,'CL','S'+128
           db      'DRA','W'+128,'CLEA','R'+128,'RETUR','N'+128,'COP','Y'+128

FROMGET    db      0                   ; 1 ako je TMANAGER pozvan iz GETTAPE

TMNAME     db      '*.TAP       ',0    ; Maska imena datoteka traka

CURBLK     dw      1                   ; Redni broj tekuceg bloka

BUFPTR     dw      0,ZXDSEG2           ; FAR pokazivac na bafer za ucitavanje
BUFPTR2    dw      0,ZXEXPBUF          ; Isto, ali kad se koristi dekompresor

EDGE_ADDR  dw      ED_PREP             ; Indirektni vektor za EDGE simulaciju

EMPTAP     db      4,7 dup (0)         ; Bajtovi koji cine praznu traku
           db      4 dup (255)


;
; Pomocni kodovi za detokenizaciju imena.
;

CONTS      db      '?????? ?!@??? ??######$$????????'
           db      ' '

;
; Poruke i varijable koje koristi potprogram za izbor trake i tekuce datoteke.
;

MSGWIN     dw      16384
           db      0,20 dup (1),11,9 dup (1),2
           db      3,'Tapefile:',11 dup (' '),3,9 dup (' '),4
           db      3,20 dup (' '),3,9 dup (' '),4
           db      8,20 dup (9),12,9 dup (9),10
           db      4 dup (3,30 dup (' '),4),'@'
           dw      18432
           db      7 dup (3,30 dup (' '),4)
           db      8,30 dup (9),10,'@'
           dw      20480
           db      3,'TAB,F2     Change current tape',4
           db      3,'SPACE      Change loading mode',4
           db      3,'BACKSPACE  Continue/Stop mode ',4
           db      3,'UP/DOWN    Next/previous file ',4
           db      3,'PGUP/PGDN  Next/previous basic',4
           db      3,'HOME/END   Begin/end of tape  ',4
           db      3,'ENTER,ESC  Exit tape manager  ',4
           db      5,30 dup (6),7,'@'
MSG_1      dw      16545
           db      'Filename:  @'
MSG_2      dw      16620
           db      'Basic@'
           dw      18529
           db      'Autorun:   @'
MSG_3      dw      18593
           db      'Prog only: @'
MSG_4      dw      16620
           db      'Number array@'
MSG_4_1    dw      18529
           db      'Arr name:  @'
MSG_5      dw      16620
           db      'Character array@'
MSG_6      dw      16620
           db      'Bytes@'
           dw      18529
           db      'Start adr: @'
MSG_7      dw      16545
           db      'Flag:      @'
MSG_7A     dw      16545
           db      'Frequency: @'
MSG_8      dw      16620
           db      'Headerless@'
MSG_9      dw      16609
           db      'Type:@'
           dw      18465
           db      'Length:@'
MSG_10     dw      18470
           db      '*** END OF TAPE ***@'
MSG_11     dw      16620
           db      'Compressed@'
MSG_11A    dw      16620
           db      'EAR Sample@'
MSG_12     dw      18529
           db      'Packed:    @'
MSG_13     dw      18615 ; 18489
           db      'F: @'
MODES      dw      16438
           db      'AUTO@'
           dw      16438
           db      'SLOW@'
           dw      16438
           db      'FAST@'
           dw      16438
           db      'LPT1@'
           dw      16438
           db      'SBST@'
CSMSG      dw      16443
           db      'CONT@'
           dw      16443
           db      'STOP@'
NONWARMSG  dw      16449
           db      'Non-Warajevo tape!!!@'
           dw      20576
           db      3,'DOWN       Next tape file     ',4
           db      3,'PAGE DOWN  Next basic loader  @'

DISABMSG   dw      18432+99
           db      'Screen refreshing disabled@'

;
; Pomocne vrijednosti za ispis razlomljene duzine kod semplova.
;

FRACTIONS  dw      125,25,375,5,625,75,875

;
; Tabele za ucitavanje iz semplova.
;

SMPFREQS   dw      15000,22050,30303,44100

TSTATES    db      233,159,116,79

;
; Graficki karakteri koje koristi potprogram TMANAGER.
;

GR_CHARS   db      255,128,191,5 dup (160)
           db      255,0,255,0,0,0,0,0
           db      255,1,253,5,5,5,5,5
           db      8 dup (160)
           db      8 dup (5)
           db      5 dup (160),191,128,255
           db      0,0,0,0,0,255,0,255
           db      5,5,5,5,5,253,1,255
           db      160,160,191,128,191,160,160,160
           db      0,0,255,0,255,0,0,0
           db      5,5,253,1,253,5,5,5
           db      255,0,191,160,160,160,160,160
           db      160,160,191,0,255,0,0,0

BLK_CHARS  db      8 dup (0)
           db      15,15,15,15,0,0,0,0
           db      4 dup (240),0,0,0,0
           db      4 dup (255),0,0,0,0
           db      0,0,0,0,15,15,15,15
           db      8 dup (15)
           db      4 dup (240),15,15,15,15
           db      4 dup (255),15,15,15,15
           db      0,0,0,0,4 dup (240)
           db      15,15,15,15,4 dup (240)
           db      8 dup (240)
           db      4 dup (255),4 dup (240)
           db      0,0,0,0,4 dup (255)
           db      15,15,15,15,4 dup (255)
           db      4 dup (240),4 dup (255)
           db      8 dup (255)

SSEG1      ends

SSEG2      segment public

           extrn   TCLFLAG:byte,SSTEPON:byte
           extrn   ALTA:byte,FLAGSL:byte,ALTF:word
           extrn   IX:word,IFF:word,EDGEOLD:word
           extrn   MASKA:word,ADR_DF:word
           extrn   LASTB:word,VIDEOFLG:word
           extrn   VIDEOSEG:word,IRQCNT:word
           extrn   POCHRL:word,CLK:byte
           extrn   FILTER:byte,TSKSWTCH:byte
           extrn   VIDEOCHG:word,ATTRCHG:word
           extrn   BLASTADDR:word

           if      VER_128

           extrn   XVIDEOCHG:word,XATTRCHG:word
           extrn   ACT_AUX:word

           endif

SEARCHREC  db      43 dup (?)          ; Blok koji koriste DOS pozivi 4EH i 4FH

CUR_LINK   dw      ?,?                 ; Pokazivac na slijedecu datoteku
BACK       dw      ?,?                 ; Pokazivac na prethodnu datoteku
NEW_DAT    dw      ?,?                 ; Pokazivac na novu datoteku (kod SAVE)
DUMMYPTR   dw      ?,?                 ; Lazni pokazivac

LOAD_MODE  db      ?                   ; 0-AUTO, 1-SLOW, 2-FAST, 3-LPT1, 4-SBST
STOP_MODE  db      ?                   ; 0-CONT, 1-STOP

CUR_LEN    dw      ?                   ; Duzina datoteke
CUR_FLAG   db      ?                   ; Flegbajt datoteke

ENDFLG     db      ?                   ; 1 ako je dostignut kraj trake
INGTAPE    db      ?                   ; 1 za vrijeme GETTAPE

REQ_LEN    dw      ?                   ; Zeljena duzina za SAVE (DE)     NE
REQ_FLAG   db      ?                   ; Zeljeni flegbajt za SAVE (A)   MRDAJ !

PARBYTE    db      ?                   ; Parity bajt za Non-Warajevo trake

EXP_LEN    dw      ?                   ; Duzina ulaznog bafera           NE
SIG_LEN    dw      ?                   ; Duzina signatura               MRDAJ !

EDTABLE    dw      ?                   ; Pozicija u tabeli gdje je EDGE trap
EDGE_TYP   db      ?                   ; Tip EDGE tranzijenta (0/255)
EDGE_LEN   dw      ?                   ; Duzina bloka ukljucujuci flag i paritet
EDGE_PTR   dw      ?                   ; Pokazivac u baferu za EDGE simulaciju
EDGE_BYT   db      ?                   ; Tekuci bajt za EDGE simulaciju
EDGE_CNT   dw      ?                   ; Brojac tranzijenata
EDGE_AFTER dw      ?                   ; Mjesto odakle se nastavlja EDGE rutina
LDCNT      dw      ?                   ; Dozvoljeno vrijeme prije ponistenja EDGE
BRKABLE    db      ?                   ; 1 ako se LD_EDGE rutina moze prekinuti
SPROTECT   db      ?                   ; Fleg za EDGE zvuk

CURRENT    dw      ?,?                 ; Pozicija tekuce datoteke u traci
TYPEF      db      ?                   ; Tip zaglavlja datoteke
FNAME      db      10 dup (?)          ; Ime iz zaglavlja datoteke
BADCNT     dw      ?                   ; Brojac za detekciju lose trake
WORK       dw      ?                   ; Razni privremeni podaci

PNUMBUF    db      7 dup (?)           ; Bafer za ispis brojeva
VMADDR     dw      ?                   ; Sluzi za pozicioniranje ispisa
INV_FLG    db      ?,?                 ; 255 = inverzni ispis (drugi bajt lazni)

EARLOADED  db      ?                   ; 1 ako je sempl ucitan u bafer

OLDVFLG    dw      ?                   ; Broj prethodne scan linije za sempl
OLDCLK     db      ?                   ; Stari CLK za sempl

SAMPLE     db      ?                   ; Tekuca vrijednost za IN sa trake
REMNDR     db      ?                   ; Ostatak za kompenzaciju

SMPBUFPTR  dw      ?,?                 ; Virtuelni 32-bitni pokazivac za sempl
SMPBITPTR  db      ?                   ; Indeks na bit u sempl bajtu

LASTUSED   db      ?                   ; Broj bita (1-8) u zadnjem sempl bajtu

SAMPLELEN  db      ?                   ; Period semplovanja u T stanjima
SLEN100    dw      ?                   ; SAMPLELEN*100

SMPSTARTER db      ?                   ; "Okidac" za prelazak u "sample" mod

SA_LD_MON  db      ?                   ; 1 ako je SAVE/LOAD pozvan iz monitora
NONWARTAP  db      ?                   ; 1 ako nije Warajevo format

NAME_BUF   db      100 dup (?)         ; Bafer za detokenizaciju imena

SSEG2      ends

CSEG2      segment public

           assume cs:CSEG2,ds:ZXEXPBUF,ss:SSEG,es:nothing

;
; Rutina za otpakivanje kompresovanih datoteka. DS i ES moraju pokazivati na
; ulazni i izlazni bafer respektivno.
;

TREE       macro   L0,L1
           call    NEXT_BIT
           jc      L1
           jmp     short L0
           endm

EXPLODE    proc    far
           pusha
           mov     ds,SNAPSEG
           mov     si,SIG_LEN          ; Bajtovi pocinju iza signatura
           inc     si
           xor     bp,bp               ; Pokazivac na signature
           mov     di,1                ; Pokazivac u izlazni bafer
           mov     ah,ds:[bp]          ; Pripremi prvu signaturu
           mov     dl,8                ; Brojac bitova
           cld
EXP_LOOP:  TREE    E_GET,GOBACK        ; Zavisno od signature prepisi bajt
E_GET:     movsb                       ;  ili se vrati
           cmp     si,EXP_LEN
           jbe     EXP_LOOP
           popa
           ret
GOBACK:    TREE    B0,B1               ; Hafmanovo stablo za broj bajtova koji
B0:        TREE    B00,B01             ;  se prepisuju prilikom vracanja
B1:        TREE    B10,B11
B11:       TREE    B110,B111
B01:       TREE    B010,B011
B10:       TREE    B100,B101
B110:      TREE    B1100,B1101
B111:      TREE    B1110,B1111
B010:      mov     cx,2                ; Duzina 2 moze da ima samo jednobajtni
           lodsb                       ;  doseg, inace nema nikakve ustede
           xor     bh,bh
           mov     bl,al
           jmp     GB_END
B00:       mov     cx,3                ; Duzine bajtova mozemo zakljuciti iz
           jmp     short V             ;  signatura preko Hafmanovog stabla
B100:      mov     cx,4
           jmp     short V
B101:      mov     cx,5
           jmp     short V
B011:      lodsb                       ; Ovo je signatura za vise od 9 bajtova
           xor     ch,ch               ;  ciji se tacan broj dobije posebno
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
V0:        TREE    V00,V01             ;  udaljenosti sekvence
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
GB_LOOP:   mov     al,es:[di+bx+1]     ;  bafera u izlazni
           stosb
           loop    GB_LOOP
           cmp     si,EXP_LEN
           ja      EXP_END
           jmp     EXP_LOOP
EXP_END:   popa
           ret
EXPLODE    endp

NEXT_BIT:  shl     ah,1
           dec     dl
           jz      NEXT_SIG
           ret
NEXT_SIG:  inc     bp
           mov     ah,ds:[bp]
           mov     dl,8
           ret

;
; Potprogram za ispis znaka iz DL na ekran. VMADDR odredjuje poziciju.
;

PO_CHR     proc   far
           push   ds
           pusha
           mov    ds,STEKSEG
           cmp    dl,128
           jae    PO_BLK
           cmp    dl,32
           jae    PO_REG
           lea    si,GR_CHARS
           jmp    short PO_CONT
PO_BLK:    sub    dl,128
           lea    si,BLK_CHARS
           jmp    short PO_CONT
PO_REG:    lea    si,CHARSET-256
PO_CONT:   xor    dh,dh
           shl    dx,3
           add    si,dx
           mov    bp,VMADDR
           add    bp,bp
           mov    cx,8
           call   POCHRL
           inc    VMADDR
           popa
           pop    ds
           ret
PO_CHR     endp

;
; Jezgro rutine za ispis razlikuje se na raznim karticama.
;

H_POCHRL:  if     VER_128              ; Hercules, CGA i EGA Mono
           push   es
           mov    es,VIDEOSEG
           endif
H_POC1:    mov    di,ADR_DF[bp-32768]
           mov    bl,[si]
           xor    bl,INV_FLG
           xor    bh,bh
           add    bx,bx
           mov    bx,MASKA[bx]
           mov    es:[di],bx
           inc    si
           add    bp,512
           loop   H_POC1
           if     VER_128
           pop    es
           endif
           ret

X_POCHRL:  if     VER_128              ; Extended Hercules
           push   es
           mov    es,VIDEOSEG
           endif
X_POC1:    mov    di,ADR_DF[bp-32768]
           mov    bl,[si]
           xor    bl,INV_FLG
           xor    bh,bh
           add    bx,bx
           mov    bx,MASKA[bx]
           mov    es:[di],bx
           mov    es:[di+8192],bx
           inc    si
           add    bp,512
           loop   X_POC1
           if     VER_128
           pop    es
           endif
           ret

V_POCHRL:  if     VER_128              ; EGA i VGA
           push   es
           mov    es,VIDEOSEG
           endif
V_POC1:    mov    di,ADR_DF[bp-32768]
           mov    bl,[si]
           xor    bl,INV_FLG
           mov    bh,7 ; 71
           mov    es:[di],bx
           inc    si
           add    bp,512
           loop   V_POC1
           if     VER_128
           pop    es
           endif
           ret

;
; Ispis poruke, DI cuva adresu poruke, a prva 2 bajta poruke su adresa u video
; memoriji. Poruka je u SS. Kraj poruke je '@'.
;

PO_MSG     proc    far
           mov     ax,ss:[di]
           mov     VMADDR,ax
           add     di,2
PO_MSGL:   mov     dl,ss:[di]
           inc     di
           cmp     dl,'@'
           je      PO_MSGE
           call    PO_CHR
           jmp     PO_MSGL
PO_MSGE:   ret
PO_MSG     endp

;
; Ispis imena (detokenizovanog).
;

PO_NAME:   push    bx
           mov     cx,10
           lea     si,NAME_BUF
PO_DLOOP:  mov     word ptr ss:[si],'  '
           add     si,2
           loop    PO_DLOOP
           mov     cx,10
           lea     si,NAME_BUF
PO_NAMEL:  mov     dl,ss:[di]
           cmp     dl,165
           jae     PO_TOKENS
           cmp     dl,144
           jb      NO_UDG
           sub     dl,79
NO_UDG:    cmp     dl,' '
           jb      NO_UDG1
           jmp     short PO_NCONT
NO_UDG1:   lea     bx,CONTS
           mov     al,dl
           xlat    CONTS
           mov     dl,al
           cmp     al,'@'
           je      PN_NEXT
           cmp     al,'!'
           jne     NO_BCK
           cmp     si,offset SSEG:NAME_BUF
           je      PN_NEXT
           dec     si
           jmp     short PN_NEXT
NO_BCK:    cmp     al,'#'
           jne     NO_C1
           inc     di
           dec     cx
           jmp     short PN_NEXT
NO_C1:     cmp     al,'$'
           jne     PO_NCONT
           add     di,2
           sub     cx,2
           mov     dl,' '
           jmp     short PO_NCONT
PO_TOKENS: lea     bx,TOKENS
           sub     dl,165
           mov     al,dl
           inc     al
PO_STEP:   inc     bx
           test    byte ptr ss:[bx-1],128
           jz      PO_STEP
           dec     al
           jnz     PO_STEP
           cmp     dl,32
           jb      PO_EACH
           cmp     byte ptr ss:[bx],'A'
           jb      PO_EACH
           cmp     byte ptr ss:[si-1],' '
           je      PO_EACH
           mov     byte ptr ss:[si],' '
           inc     si
PO_EACH:   mov     al,ss:[bx]
           and     al,7FH
           mov     ss:[si],al
           inc     si
           mov     al,ss:[bx]
           inc     bx
           add     al,al
           jnc     PO_EACH
           cmp     al,48H
           jz      PO_TRSP
           cmp     al,82H
           jb      PN_NEXT
PO_TRSP:   cmp     dl,3
           jc      PN_NEXT
           mov     dl,' '
PO_NCONT:  mov     ss:[si],dl
           inc     si
PN_NEXT:   inc     di
           dec     cx
           js      PN_END
           jz      PN_END
           jmp     PO_NAMEL
PN_END:    mov     cx,19
           lea     di,NAME_BUF
PN_ELOOP:  mov     dl,ss:[di]
           call    PO_CHR
           inc     di
           loop    PN_ELOOP
           pop     bx
           ret

;
; Ispis znaka iz DL, koji moze biti i token, u bafer adresiran sa SS:DI.
;

PO_ONE     proc    far
           push    bx
           cmp     dl,32
           jb      PO_O_END
           cmp     dl,165
           jae     PO_O_TOK
           cmp     dl,144
           jb      PO_O_CONT
           sub     dl,79
PO_O_CONT: mov     ss:[di],dl
           inc     di
           jmp     short PO_O_END
PO_O_TOK:  lea     bx,TOKENS
           sub     dl,165
           mov     al,dl
           inc     al
PO_O_STEP: inc     bx
           test    byte ptr ss:[bx-1],128
           jz      PO_O_STEP
           dec     al
           jnz     PO_O_STEP
PO_O_EACH: mov     al,ss:[bx]
           mov     dl,al
           and     dl,7FH
           mov     ss:[di],dl
           inc     di
           inc     bx
           add     al,al
           jnc     PO_O_EACH
PO_O_END:  pop     bx
           ret
PO_ONE     endp

;
; Ispis broja u DX.
;

PO_NUM     proc    far
           push    bx
           lea     di,PNUMBUF[6]
           mov     byte ptr ss:[di],'@'
           mov     ax,dx
           mov     bx,10
PO_NUML:   xor     dx,dx
           div     bx
           dec     di
           add     dl,'0'
           mov     ss:[di],dl
           or      ax,ax
           jnz     PO_NUML
           pop     bx
           jmp     PO_MSGL
PO_NUM     endp

;
; Potprogram koji ispisuje da je obustavljena grafika.
;

DISPDIS    proc    far
           pusha
           xor     ah,ah
           push    LASTB
           call    ULAFAR
           pop     LASTB
           mov     dl,' '
           mov     VMADDR,16384
           mov     cx,256
DDL1:      call    PO_CHR
           loop    DDL1
           mov     VMADDR,18432
           mov     cx,256
DDL2:      call    PO_CHR
           loop    DDL2
           mov     VMADDR,20480
           mov     cx,256
DDL3:      call    PO_CHR
           loop    DDL3
           lea     di,DISABMSG
           call    PO_MSG
           push    es
           cld
           xor     si,si
           mov     di,32768
           mov     cx,16384
           mov     es,VIDEOSEG
           rep     movs word ptr es:[di],es:[si]
           pop     es
           popa
           ret
DISPDIS    endp

;
; Potprogram za izbor trake i tekuce datoteke na traci. Ovaj potprogram se
; poziva iz INT 9 pritiskom na F2.
;

LSEEK      macro   POS                 ; Long seek poziv iz DOS-a
           mov     ax,4200H
           mov     dx,POS
           mov     cx,POS+2
           int     21H
           endm

READBLK    macro   ADR,LEN             ; Read block poziv iz DOS-a
           mov     ah,3FH
           mov     cx,LEN
           lea     dx,ADR
           int     21H
           endm

TMANAGER   proc    far
           pusha
           call    RESTTIMER
           call    RESTKBD
           call    FARSTOPF
           mov     ds,STEKSEG
TAPELOOP:  mov     DUMMYPTR,0          ; Ovo rjesava bag sa Non-Warajevo trakom
           mov     ax,3D00H            ;  kracom od 12 bajta
           lea     dx,SEARCHREC[1EH]
           int     21H                 ; Otvori "traku"
           jnc     TAPEPASS
           jmp     DOSELECT
TAPEPASS:  mov     bx,ax               ; Handler u BX
           mov     ax,CUR_LINK         ; Ako je pocetak trake, treba pronaci
           or      ax,CUR_LINK+2       ;  prvu datoteku
           jnz     TMNOF
TAPESTART: READBLK CUR_LINK,4
           READBLK DUMMYPTR,4          ; Provjeri da li je Warajevo format
           READBLK DUMMYPTR,4
           mov     ax,DUMMYPTR
           and     ax,DUMMYPTR+2
           inc     ax
           jz      TMNOF
           mov     NONWARTAP,1         ; Rad sa Non-Warajevo formatom
           mov     CUR_LINK,0
           mov     CUR_LINK+2,0
           mov     BACK,0
           mov     BACK+2,0
TMNOF:     LSEEK   CUR_LINK            ; Pozicioniraj se na tekucu datoteku
TMAINLP:   xor     ah,ah
           push    LASTB               ; Postavi BORDER
           call    ULAFAR
           pop     LASTB
           mov     SAMPLEON,0          ; Nije sempl dok se ne dokaze drugacije
           call    MAXLSOFF
           lea     di,MSGWIN
           call    PO_MSG              ; Iscrtaj okvire itd.
           call    PO_MSG
           call    PO_MSG
           cmp     NONWARTAP,0         ; Warajevo format?
           je      WARTOK
           lea     di,NONWARMSG        ; Ispisi da traka nije Warajevo format
           call    PO_MSG
           call    PO_MSG
WARTOK:    xor     ah,ah               ; Ispisi modove rada
           mov     al,LOAD_MODE
           imul    ax,ax,7
           lea     di,MODES
           add     di,ax
           not     INV_FLG
           call    PO_MSG
           mov     al,STOP_MODE
           xor     ah,ah
           imul    ax,ax,7
           lea     di,CSMSG
           add     di,ax
           call    PO_MSG
           not     INV_FLG
           mov     VMADDR,16428        ; Ispisi ime trake
           lea     di,SEARCHREC[1EH]
NLOOP:     mov     dl,ss:[di]
           cmp     dl,'.'
           je      NEND
           call    PO_CHR
           inc     di
           jmp     NLOOP
NEND:      mov     ax,CUR_LINK         ; Zapamti poziciju u datoteci
           mov     CURRENT,ax
           mov     ax,CUR_LINK+2
           mov     CURRENT+2,ax
           cmp     NONWARTAP,0
           je      DOWARTAP
           READBLK CUR_LEN,2           ; Za Non-Warajevo traku pokazivac na
           mov     ax,CURRENT          ;  slijedecu datoteku treba izracunati
           mov     dx,CURRENT+2
           add     ax,CUR_LEN
           adc     dx,0
           add     ax,2
           adc     dx,0
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           mov     ax,4202H            ; Test kraja za Non-Warajevo TAP
           xor     cx,cx
           xor     dx,dx
           int     21H
           sub     ax,CURRENT
           sbb     dx,CURRENT+2
           or      ax,dx
           jz      NWTEND
           LSEEK   CURRENT
           jmp     short TMNOLAST
NWTEND:    mov     CUR_LINK,-1
           mov     CUR_LINK+2,-1
           LSEEK   CURRENT
           jmp     short ONTAPEEND
DOWARTAP:  READBLK BACK,4              ; Ocitaj pokazivac na prethodnu i na
           READBLK CUR_LINK,4          ;  slijedecu datoteku, i izadji ako
           jumps                       ;  to ne uspijeva
           jc      TMREST
           nojumps
           mov     ax,CUR_LINK         ; Provjeri da li je kraj trake
           and     ax,CUR_LINK+2
           inc     ax
           jnz     TMNOLAST
ONTAPEEND: lea     di,MSG_10           ; Ispisi da je kraj trake
           call    PO_MSG
           jmp     TMCONT
TMNOLAST:  lea     di,MSG_9            ; Ispisi neke tekstove
           call    PO_MSG
           call    PO_MSG
           lea     di,MSG_13           ; Ispisi poziciju bloka
           call    PO_MSG
           mov     dx,CURBLK
           call    PO_NUM
           READBLK CUR_LEN,2           ; Ocitaj duzinu datoteke i prilagodi je
           mov     al,NONWARTAP        ;  za slucaj Non-Warajevo formata
           add     al,al
           xor     ah,ah
           sub     CUR_LEN,ax
           READBLK CUR_FLAG,1          ; Ocitaj flegbajt
           jumps
           cmp     CUR_LEN,17          ; Ako duzina nije 17, to nije header
           jne     NOHEADER
           cmp     CUR_FLAG,0          ; Ako flegbajt nije 0, to nije heder
           jne     NOHEADER
           READBLK TYPEF,1             ; Ucitaj prvi bajt headera
           cmp     TYPEF,3             ; Ako je veci od 3, to nije header
           ja      NOHEADER
           nojumps
           READBLK FNAME,10            ; Ucitaj ime iz headera
           lea     di,MSG_1            ; Ispisi ga
           call    PO_MSG
           lea     di,FNAME
           call    PO_NAME
           READBLK WORK,2              ; Ucitaj duzinu bloka iz headera
           mov     VMADDR,18476        ; Ispisi je
           mov     dx,WORK
           call    PO_NUM
           cmp     TYPEF,0             ; Ako nije tip "Basic" idi dalje
           jnz     TYPE1
           lea     di,MSG_2            ; Ispisi tip
           call    PO_MSG
           call    PO_MSG
           READBLK WORK,2              ; Ucitaj iz headera liniju autostarta
           mov     dx,WORK
           cmp     dx,32768            ; Ako je >= 32768, nema autostarta
           jae     NOARUN
           call    PO_NUM              ; Ispisi autostart
           jmp     short PONLY
NOARUN:    mov     dl,'-'              ; Ispisi crticu
           call    PO_CHR
PONLY:     lea     di,MSG_3
           call    PO_MSG
ONUM:      READBLK WORK,2              ; Ucitaj iz headera duzinu samog programa
           mov     dx,WORK             ; Ispisi je
           call    PO_NUM
           jmp     TMCONT
TYPE1:     cmp     TYPEF,1             ; Ako nije tip "Numeric array" idi dalje
           jne     TYPE2
           lea     di,MSG_4            ; Ispisi tip
           call    PO_MSG
           call    PO_MSG
ARR:       READBLK WORK,2              ; Ucitaj iz headera ime niza
           mov     dl,byte ptr WORK+1  ; Ispisi ga
           and     dl,1FH
           add     dl,64
           call    PO_CHR
           jmp     TMCONT
TYPE2:     cmp     TYPEF,2             ; Ako nije tip "Character array", idi
           jne     TYPE3               ;  dalje (tada je sigurno tip "Bytes")
           lea     di,MSG_5            ; Ispisi tip
           call    PO_MSG
           lea     di,MSG_4_1
           call    PO_MSG
           jmp     ARR                 ; Nastavi kao sa tipom 1
TYPE3:     lea     di,MSG_6            ; Ispisi tip "Bytes"
           call    PO_MSG
           call    PO_MSG
           jmp     ONUM                ; Ucitaj duzinu i ispisi je
NOHEADER:  cmp     CUR_LEN,-2          ; Da li je sempl blok?
           je      SHWSAMPLE
           lea     di,MSG_7            ; Ispisi flegbajt
           call    PO_MSG
           mov     dl,CUR_FLAG
           xor     dh,dh
           call    PO_NUM
           cmp     CUR_LEN,-1          ; Da li je kompresovan blok?
           je      IMPLODED
           lea     di,MSG_8            ; Ispisi tip "Headerless"
           call    PO_MSG
           mov     VMADDR,18476        ; Ispisi duzinu bloka
           mov     dx,CUR_LEN
           call    PO_NUM
           jmp     TMCONT
SHWSAMPLE: lea     di,MSG_7A           ; Ispisi frekvenciju semplovanja
           call    PO_MSG
           mov     al,CUR_FLAG
           xor     ah,ah
           shr     ax,2
           and     al,00000110B
           mov     di,ax
           mov     dx,SMPFREQS[di]
           call    PO_NUM
           lea     di,MSG_11A          ; Ispisi tip "EAR Sample"
           mov     SAMPLEON,1
           call    far ptr KILLEDGE    ; Predji u "sample" nacin rada
           call    MAXLSON
           jmp     short IMPLODED1
IMPLODED:  lea     di,MSG_11           ; Ispisi tip "Compressed"
IMPLODED1: call    PO_MSG
           READBLK CUR_LEN,2           ; Ustanovi stvarnu duzinu
           mov     VMADDR,18476
           mov     dx,CUR_LEN
           push    dx
           cmp     SAMPLEON,1
           jne     IMPLODED2
           mov     al,CUR_FLAG         ; Duzina semplova moze biti razlomljena
           and     al,7
           cmp     al,7
           je      IMPLODED2
           xor     ah,ah
           push    ax
           dec     dx                  ; Ispisi razlomljenu duzinu
           call    PO_NUM
           mov     dl,'.'
           call    PO_CHR
           pop     di
           add     di,di
           mov     dx,FRACTIONS[di]
           call    PO_NUM
           jmp     short IMPLODED3
IMPLODED2: call    PO_NUM              ; Ispisi duzinu
IMPLODED3: lea     di,MSG_12
           call    PO_MSG
           READBLK CUR_LEN,2           ; Ispisi pakovanu duzinu
           mov     dx,CUR_LEN
           call    PO_NUM
           mov     ax,100              ; Izracunaj stepen pakovanja
           mul     CUR_LEN
           pop     cx
           div     cx
           push    ax
           mov     dl,' '
           call    PO_CHR
           mov     dl,'('
           call    PO_CHR
           pop     dx
           call    PO_NUM              ; Ispisi ga
           mov     dl,'%'
           call    PO_CHR
           mov     dl,')'
           call    PO_CHR
           mov     TSKSWTCH,0
TMCONT:    mov     ah,1                ; Isprazni bafer za tastaturu
           int     16H
           jz      TMCONT1
           xor     ah,ah
           int     16H
           jmp     TMCONT
TMCONT1:   cmp     TSKSWTCH,1          ; Ponovi iscrtavanje ako Task Switch
           mov     TSKSWTCH,0          ;  nastupi unutar ove petlje
           jne     TMCONT2
           mov     ax,CURRENT
           mov     CUR_LINK,ax
           mov     ax,CURRENT+2
           mov     CUR_LINK+2,ax
           jmp     TMNOF
TMCONT2:   mov     ah,1                ; Cekaj na pritisak tipke
           int     16H
           jz      TMCONT1
           xor     ah,ah               ; Ocitaj tastaturu
           int     16H
           cmp     ax,5000H            ; Strelica dolje
           jne     LASTFILE
           mov     ax,CUR_LINK         ; Ne moze se pomjerati iza kraja
           and     ax,CUR_LINK+2
           inc     ax
           jz      TMCONT
           call    KLIK
           inc     CURBLK
           jmp     TMNOF
LASTFILE:  cmp     ax,4800H            ; Strelica gore
           jne     SELECT
           mov     ax,BACK             ; Ne moze se pomjerati ispred pocetka
           or      ax,BACK+2
           jz      TMCONT
           call    KLIK
           mov     ax,BACK             ; Predji na prethodnu datoteku
           mov     CUR_LINK,ax
           mov     ax,BACK+2
           mov     CUR_LINK+2,ax
           dec     CURBLK
           jmp     TMNOF
SELECT:    cmp     al,9                ; Taster TAB
           jne     NEXTBAS
SELECT1:   call    KLIK
DOSELECT:  mov     CURBLK,1
           cmp     FILTER,1
           je      TAPFILT
           mov     word ptr TMNAME[0],'.*'
           mov     word ptr TMNAME[2],'AT'
           mov     word ptr TMNAME[4],' P'
           mov     word ptr TMNAME[6],'  '
           mov     word ptr TMNAME[8],'  '
           mov     word ptr TMNAME[10],'  '
TAPFILT:   mov     ah,3EH              ; Zatvori "traku"
           int     21H
           mov     CUR_LINK,0          ; Pozicioniraj se na pocetak trake
           mov     CUR_LINK+2,0
           mov     NONWARTAP,0
           mov     ah,4FH              ; Predji na slijedecu traku
           int     21H
           jnc     SELECT2             ; Ako nema vise traka, idi dalje
           mov     ah,4EH              ; Vrati se na prvu traku
           lea     dx,TMNAME
           xor     cx,cx
           int     21H
           jnc     SELECT2
           jmp     CHOICE2
SELECT2:   jmp     TAPELOOP
NEXTBAS:   cmp     ax,3C00H            ; Taster F2 je ekvivalentan sa TAB
           je      SELECT1
           cmp     ax,5100H            ; Taster PgDn
           je      NB1
           jmp     LASTBAS
NB1:       mov     BADCNT,1000
           mov     ax,CUR_LINK
           and     ax,CUR_LINK+2
           inc     ax
           jnz     NB2
           jmp     TMCONT
NB2:       call    KLIK
           cmp     NONWARTAP,0
           je      NBLOOP
           jmp     NONWPGDN
NBLOOP:    LSEEK   CUR_LINK            ; Pronadji slijedecu BASIC datoteku
           inc     CURBLK
           READBLK BACK,4
           READBLK CUR_LINK,4
           jumps                       ; Obezbijedi da postoji izlaz u slucaju
           jc      TMREST              ;  greske, ili ako se nakon 1000 citanja
           dec     BADCNT              ;  ne nadje blok (losa traka)
           jz      TMAINLP
           nojumps
           mov     ax,CUR_LINK
           and     ax,CUR_LINK+2
           inc     ax
           mov     dx,-8
           jz      NBACK
           READBLK CUR_LEN,2
           READBLK CUR_FLAG,1
           READBLK TYPEF,1
           cmp     CUR_LEN,17
           jne     NBLOOP
           cmp     CUR_FLAG,0
           jne     NBLOOP
           cmp     TYPEF,0
           jne     NBLOOP
NBEND:     mov     dx,-12
NBACK:     mov     cx,-1               ; Vrati se nazad |CX:DX| bajtova
           mov     ax,4201H
           int     21H
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           jmp     TMAINLP
NONWPGDN:  LSEEK   CUR_LINK            ; Pronalazenje slijedece BASIC datoteke
           inc     CURBLK              ;  na Non-Warajevo trakama
           dec     BADCNT
           jumps
           jz      TMAINLP
           READBLK CUR_LEN,2
           cmp     ax,cx               ; Ad-hoc nacin pronalazenja kraja (!)
           jne     NONWEND
           nojumps
           mov     ax,CUR_LINK
           add     ax,CUR_LEN
           mov     dx,CUR_LINK+2
           adc     dx,0
           add     ax,2
           adc     dx,0
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           READBLK CUR_FLAG,1
           READBLK TYPEF,1
           cmp     CUR_LEN,19
           jne     NONWPGDN
           cmp     CUR_FLAG,0
           jne     NONWPGDN
           cmp     TYPEF,0
           jne     NONWPGDN
           mov     dx,-4
           jmp     NBACK
LASTBAS:   cmp     ax,4900H            ; Taster PgUp
           jumps
           jne     ENDTAPE
           mov     BADCNT,1000
           mov     ax,BACK
           or      ax,BACK+2
           jz      TMCONT
           call    KLIK
LBLOOP:    LSEEK   BACK                ; Pronadji prethodnu BASIC datoteku
           dec     CURBLK
           mov     ax,BACK
           or      ax,BACK+2
           jz      STTAPE1 ; TMAINLP
           READBLK BACK,4
           READBLK CUR_LINK,4
           READBLK CUR_LEN,2
           jc      TMREST              ; Obezbijedi da postoji izlaz u slucaju
           dec     BADCNT              ;  greske, ili ako se nakon 1000 citanja
           jz      TMAINLP             ;  ne nadje blok (losa traka)
           cmp     CUR_LEN,17
           jne     LBLOOP
           READBLK CUR_FLAG,1
           cmp     CUR_FLAG,0
           jne     LBLOOP
           READBLK TYPEF,1
           cmp     TYPEF,0
           jne     LBLOOP
           jmp     NBEND
ENDTAPE:   cmp     ax,4F00H            ; Taster End
           jnz     short STARTTAPE
           call    KLIK
           cmp     NONWARTAP,0
           je      short ENDWARTAP
NONWEND:   mov     ax,4202H            ; Odlazak na kraj Non-Warajevo trake
           xor     cx,cx
           xor     dx,dx
           int     21H
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           jmp     TMAINLP
ENDWARTAP: mov     BADCNT,1000         ; Odlazak na kraj Warajevo trake je
ENDTLOOP:  mov     ax,CUR_LINK         ;  nesto slozeniji
           and     ax,CUR_LINK+2
           inc     ax
           mov     dx,-8
           jz      NBACK
           jumps
           LSEEK   CUR_LINK
           inc     CURBLK
           READBLK BACK,4
           READBLK CUR_LINK,4
           jc      TMREST              ; Obezbijedi izlaz u slucaju greske
           dec     BADCNT
           jnz     ENDTLOOP
           jmp     TMAINLP
STARTTAPE: cmp     ax,4700H            ; Taster Home
           jnz     short CHGALG
           nojumps
           call    KLIK
STTAPE1:   mov     ax,4200H            ; Pozicioniraj se na pocetak trake
           xor     cx,cx
           xor     dx,dx
           int     21H
           mov     CURBLK,1
           jmp     TAPESTART
CHGALG:    cmp     al,32               ; Taster SPACE
           jnz     CHOICE
           call    far ptr KILLEDGE
           mov     ax,3
           call    far ptr SETLMODE
CHMEND:    mov     ax,CURRENT
           mov     CUR_LINK,ax
           mov     ax,CURRENT+2
           mov     CUR_LINK+2,ax
           call    KLIK
           jmp     TMNOF
CHOICE:    cmp     al,13               ; Taster ENTER ili ESC
           je      CHOICE1
           cmp     al,27
           je      CHOICE1
           cmp     al,8                ; Taster BACKSPACE
           je      CHCOST
           jmp     TMCONT
CHCOST:    cmp     LOAD_MODE,2
           ja      CHMEND
           xor     STOP_MODE,1
           jmp     CHMEND
CHOICE1:   call    KLIK
CHOICE2:   mov     ax,CURRENT          ; Oznaci izabranu datoteku
           mov     CUR_LINK,ax
           mov     ax,CURRENT+2
           mov     CUR_LINK+2,ax
TMREST:    mov     ah,3EH              ; Zatvori "traku"
           int     21H
           mov     ds,MEMSEG
           cmp     SAMPLEON,1
           je      TMREST1
           mov     EARLOADED,0
TMREST1:   call    REPAINT
           call    SETKBD
           cmp     FROMGET,0
           jne     TMEXIT
           call    SETTIMER
TMEXIT:    popa
           ret
TMANAGER   endp

;
; Pozicioniranje na n-tu datoteku (za parametar /F).
;

NTH        proc    far
           pusha
           mov     CURBLK,1
           call    RESTTIMER
           mov     ds,STEKSEG
           lea     dx,SEARCHREC[1EH]   ; Otvori "traku" (ovdje joj je ime)
           mov     ax,3D00H
           int     21H
           jumps
           jc      NTHEOT
           nojumps
           mov     bx,ax
           mov     WORK,ax
           mov     ax,4200H            ; Pozicioniraj se na pocetak trake
           xor     cx,cx
           xor     dx,dx
           int     21H
           jumps
           jc      NTHEOT
           READBLK CUR_LINK,4          ; Provjeri da li je Warajevo traka
           READBLK DUMMYPTR,4
           READBLK DUMMYPTR,4
           popa
           pusha
           mov     ax,DUMMYPTR
           and     ax,DUMMYPTR+2
           inc     ax
           jnz     short NTHNONW
           dec     cx
           jcxz    NTHEXIT
NTHLOOP:   push    cx
           mov     bx,WORK
           LSEEK   CUR_LINK
           READBLK BACK,4
           READBLK CUR_LINK,4
           mov     ax,CUR_LINK
           and     ax,CUR_LINK+2      ; Kraj trake, ignorisi
           inc     ax
           jz      NTHEOT
           nojumps
           mov     ax,CUR_LINK
           mov     CURRENT,ax
           mov     ax,CUR_LINK+2
           mov     CURRENT+2,ax
           LSEEK   CUR_LINK
           inc     CURBLK
           pop     cx
           loop    NTHLOOP
           jmp     NTHEXIT
NTHNONW:   pusha                       ; Pozicioniranje u Non-Warajevo traci
           mov     bx,WORK
           mov     ax,4200H
           xor     cx,cx
           xor     dx,dx
           int     21H
           popa
           mov     NONWARTAP,1
           mov     CUR_LINK,0
           mov     CUR_LINK+2,0
           mov     BACK,0
           mov     BACK+2,0
           dec     cx
           jcxz    NTHEXIT
NTHNWLOOP: push    cx
           mov     bx,WORK
           READBLK CUR_LEN,2
           cmp     ax,cx               ; Ne idi dalje od kraja
           pop     cx
           jne     NTHEXIT
           push    cx
           mov     ax,CUR_LINK
           add     ax,CUR_LEN
           mov     dx,CUR_LINK+2
           adc     dx,0
           add     ax,2
           adc     dx,0
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           LSEEK   CUR_LINK
           inc     CURBLK
           pop     cx
           loop    NTHNWLOOP
NTHEXIT:   mov     ds,MEMSEG
           mov     bx,WORK
           mov     ah,3EH              ; Zatvori "traku"
           int     21H
           call    SETTIMER
           popa
           ret
NTHEOT:    pop     cx
           mov     dx,4
           xor     cx,cx
           mov     ax,4200H
           int     21H
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           jmp     NTHEXIT
NTH        endp

;
; Rutina za uzimanje bloka sa trake.
;

GETTAPE    proc    far
           mov     INGTAPE,1
           cmp     SSTEPON,1           ; Preskoci azuriranje ako je single step
           je      SKIPRPNT            ;  ili ako je citanje semplova
           cmp     SAMPLEON,1
           je      SKIPRPNT
           cmp     VIDSYNC,-2
           je      SKIPRPNT
           call    REPAINT             ; Azuriraj sliku
SKIPRPNT:  mov     VIDEOCHG,1
           mov     ATTRCHG,1
           if      VER_128
           mov     XVIDEOCHG,1
           mov     XATTRCHG,1
           endif
           call    RESTTIMER           ; Preusmjeri na normalne vektore timer
           cmp     STOP_MODE,0         ; Ako je STOP mod, pozovi prvo
           je      GETTAPE1            ;  Tape Manager (nakon male pauze)
           push    ax
           push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ax,ds:[46CH]
           add     ax,20
GTHLOOP:   cmp     ax,ds:[46CH]
           ja      GTHLOOP
           pop     ds
           pop     ax
           inc     FROMGET
           call    TMANAGER
           dec     FROMGET
GETTAPE1:  mov     EDGE_ADDR,offset ED_PREP
           pusha
           mov     ENDFLG,0            ; Pretpostavi (zasad) da nije kraj trake
           mov     ds,STEKSEG
           lea     dx,SEARCHREC[1EH]   ; Otvori "traku" (ovdje joj je ime)
           mov     ax,3D00H
           int     21H
           jnc     GETPROC
           jmp     LOADERR1
GETPROC:   mov     bx,ax               ; Handler u BX
           mov     ax,CUR_LINK         ; Ako smo na samom pocetku trake, treba
           or      ax,CUR_LINK+2       ;  naci prvu datoteku
           jnz     LNOFIRST
LFIRST:    READBLK CUR_LINK,4
           READBLK DUMMYPTR,4          ; Provjeri da li je Warajevo format
           READBLK DUMMYPTR,4
           mov     ax,DUMMYPTR
           and     ax,DUMMYPTR+2
           inc     ax
           jz      LNOFIRST
           mov     NONWARTAP,1         ; Rad sa Non-Warajevo formatom
           mov     CUR_LINK,0
           mov     CUR_LINK+2,0
           mov     BACK,0
           mov     BACK+2,0
LNOFIRST:  LSEEK   CUR_LINK            ; Pozicioniraj se na trazenu datoteku
           inc     CURBLK
           cmp     NONWARTAP,0         ; Idi naprijed ako je Warajevo TAP
           je      GETWARTAP
           mov     ax,4202H            ; Provjeri da li smo na kraju
           xor     cx,cx               ;  Non-Warajevo trake
           xor     dx,dx
           int     21H
           sub     ax,CUR_LINK
           sbb     dx,CUR_LINK+2
           or      ax,dx
           jz      GETNWEND
           LSEEK   CUR_LINK
           READBLK EDGE_LEN,2          ; Postupak za Non-Warajevo trake
           mov     ax,CUR_LINK         ;  (pokazivac na slijedecu datoteku se
           add     ax,EDGE_LEN         ;  izracunava)
           mov     dx,CUR_LINK+2
           adc     dx,0
           add     ax,2
           adc     dx,0
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           mov     cx,EDGE_LEN
           mov     EXP_LEN,0
           lds     dx,dword ptr BUFPTR
           jmp     LOADL1
GETWARTAP: READBLK BACK,4              ; Ucitaj pokazivac na prethodnu datoteku
           READBLK CUR_LINK,4          ; Ucitaj pokazivac na slijedecu datoteku
           mov     ax,CUR_LINK         ; Ako smo dosli do kraja trake, treba se
           and     ax,CUR_LINK+2       ;  pozicionirati ponovo na pocetak
           inc     ax
           jnz     LNOLAST
GETNWEND:  mov     CURBLK,1
           cmp     ENDFLG,1            ; Ovim se izbjegava mrtva petlja u
           je      LOADERR             ;  slucaju da je traka potpuno prazna
           mov     ENDFLG,1
           xor     cx,cx
           xor     dx,dx
           mov     ax,4200H
           int     21H
           jmp     LFIRST
LNOLAST:   READBLK EDGE_LEN,2          ; Ucitaj duzinu datoteke
           mov     cx,EDGE_LEN
           cmp     cx,-1               ; Specijalan postupak ako je pakovana
           je      PACKED              ;  ili ako sadrzi EAR sample
           cmp     cx,-2
           je      SMPLBLK
           inc     cx
           mov     EXP_LEN,0
           lds     dx,dword ptr BUFPTR
           jmp     short LOADL
SMPLBLK:   READBLK CUR_FLAG,1          ; Procitaj sempl blokove
           READBLK EDGE_LEN,2
           READBLK EXP_LEN,4
           mov     cx,EDGE_LEN         ; Ako su kompresovana i nekompresovana
           cmp     cx,EXP_LEN          ;  duzina jednake, sempl je kompresovan
           jne     PACKED1
           mov     EXP_LEN,0           ; Signal "nema kompresije"
           lds     dx,dword ptr BUFPTR
           inc     dx
           inc     cx
           jmp     short LOADL
LOADERR:   mov     ah,3EH              ; Zatvori "traku"
           int     21H
LOADERR1:  mov     ds,MEMSEG
           mov     INGTAPE,0
           call    SETTIMER
           cli
           popa
           clc
           ret
PACKED:    READBLK CUR_FLAG,1          ; Procitaj flegbajt, otpakovanu duzinu
           READBLK EDGE_LEN,2          ;  i tehnicke podatke
           READBLK EXP_LEN,4
PACKED1:   lds     dx,dword ptr BUFPTR2
           mov     cx,EXP_LEN
LOADL:     add     EDGE_LEN,2          ; Dodaj flegbajt i paritet
LOADL1:    mov     ah,3FH              ; Ucitaj cijelu datoteku kao jedan blok
           int     21H
           jc      LOADERR
           cmp     ax,cx
           jne     LOADERR
           cmp     EXP_LEN,0           ; Ako je pakovana, treba je otpakovati
           je      NOTPACK
           push    es
           mov     es,BUFPTR+2
           mov     al,CUR_FLAG         ; Prebaci fleg
           mov     es:[0],al
           call    EXPLODE             ; Otpakuj datoteku
           pop     es
           mov     ds,BUFPTR+2
           mov     cx,EDGE_LEN
           dec     cx
NOTPACK:   cmp     NONWARTAP,0         ; Non-Warajevo TAP ima u bloku bajt
           jne     LEN0                ;  pariteta, a za Warajevo TAP bajt
           xor     si,si               ;  pariteta se konstruise
           xor     ah,ah
LOADLP:    xor     ah,[si]
           inc     si
           loop    LOADLP
           mov     [si],ah
LEN0:      mov     ah,3EH              ; Zatvori "traku"
           int     21H
L_S_END:   mov     ds,MEMSEG
           mov     INGTAPE,0
           call    SETTIMER            ; Vrati timer za simulator
           cli
           popa
           mov     EDGE_CNT,8064       ; Header ima 8064 tranzijenta u leaderu,
           test    CUR_FLAG,128        ;  a obicni blokovi 3220
           jz      L_HEADER
           mov     EDGE_CNT,3220
L_HEADER:  mov     B,255
           stc
           ret
GETTAPE    endp

;
; Potprogram koji vraca Z=1 ako je tekuci blok sempl blok.
;

TSTSMPBLK  proc    far
           cmp     NONWARTAP,0
           jne     TSTSMPEND
           pusha
           push    CUR_LINK
           push    CUR_LINK+2
           call    RESTTIMER
           push    ds
           mov     ds,STEKSEG
           lea     dx,SEARCHREC[1EH]   ; Otvori "traku" (ovdje joj je ime)
           mov     ax,3D00H
           int     21H
           mov     bx,ax               ; Handler u BX
           mov     ax,CUR_LINK
           or      ax,CUR_LINK+2
           jnz     TSTSMPNOF
           READBLK CUR_LINK,4
TSTSMPNOF: LSEEK   CUR_LINK
           READBLK DUMMYPTR,4
           READBLK DUMMYPTR,4
           READBLK DUMMYPTR,2
           mov     ah,3EH
           int     21H
           pop     ds
           call    SETTIMER
           pop     CUR_LINK+2
           pop     CUR_LINK
           popa
           cmp     DUMMYPTR,-2
TSTSMPEND: ret
TSTSMPBLK  endp

;
; Potprogram za ucitavanje sempl bloka.
;

GETSMPBLK: call    GETTAPE             ; Ucitaj sempl blok
           sub     EDGE_LEN,3          ; Zaobidji fleg bajt i paritet
           mov     EARLOADED,1
           pusha
           mov     bl,CUR_FLAG
           mov     LASTUSED,bl
           and     LASTUSED,7
           inc     LASTUSED
           xor     bh,bh
           shr     bx,3
           and     bl,3
           mov     al,TSTATES[bx]
           mov     SAMPLELEN,al
           mov     bl,100
           mul     bl
           mov     SLEN100,ax
           popa
           ret

;
; Potprogram za IN sa kasetofona u "sample" modu.
;

EARSAMPLE  proc    far
           pusha
           cmp     EARLOADED,1         ; Da li je sempl ucitan u bafer
           je      EARMAIN
           call    GETSMPBLK
           mov     IRQCNT,0            ; Zapamti tekuci timing
           mov     ax,VIDEOFLG
           mov     OLDVFLG,ax
           mov     al,CLK
           mov     OLDCLK,al
           mov     REMNDR,0
           mov     SMPBUFPTR,0         ; Inicijaliziraj pokazivace
           mov     SMPBUFPTR+2,0
           mov     SMPBITPTR,0
EARMAIN:   mov     ax,VIDEOFLG         ; Izracunaj broj T stanja od posljednjeg
           sub     ax,OLDVFLG          ;  IN-a (224 * broj linija + 69888 * broj
           if      VER_128             ;  IRQ-ova + broj preostalih T stanja u
           mov     bx,228              ;  liniji + ostatak od posljednjeg
           else                        ;  racunanja); na Spectrumu 128 zbog
           mov     bx,224              ;  drugacijeg timinga 224 se mijenja sa
           endif                       ;  228 a 69888 sa 70908
           imul    bx                  
           mov     bx,ax               
           mov     cx,dx
           mov     al,CLK
           xor     ah,ah
           mov     dl,OLDCLK
           xor     dh,dh
           sub     ax,dx
           cwd
           add     bx,ax
           adc     cx,dx
           mov     ax,IRQCNT
           if      VER_128
           mov     dx,5372             ; 70908 = 5372 + 65536   (228 * 311)
           else
           mov     dx,4352             ; 69888 = 4352 + 65536   (224 * 312)
           endif
           mul     dx
           add     dx,IRQCNT
           add     ax,bx
           adc     dx,cx
           mov     bl,REMNDR
           xor     bh,bh
           add     ax,bx
           adc     dx,0
           mov     IRQCNT,0            ; Zapamti novo stanje
           mov     bx,VIDEOFLG
           mov     OLDVFLG,bx
           mov     bl,CLK
           mov     OLDCLK,bl
           cmp     dx,SLEN100          ; Prije dijeljenja se osiguraj od moguceg
           jumps                       ;  prekoracenja
           jae     NOMORESMP
           nojumps
           mov     bx,SLEN100          ; 32-bitno dijeljenje DX;AX <- DX;AX/...
           div     bx
           mov     bx,dx
           mov     dx,100
           mul     dx
           xchg    ax,bx
           mov     cl,SAMPLELEN
           div     cl
           mov     REMNDR,ah
           xor     ah,ah
           add     ax,bx
           adc     dx,0
           mov     cl,al               ; Izdvoji pomak bita
           and     cl,7
           shr     dx,1                ; Izdvoji pomak bajta
           rcr     ax,1
           shr     dx,1
           rcr     ax,1
           shr     dx,1
           rcr     ax,1
           add     cl,SMPBITPTR        ; Izracunaj novu poziciju bita u baferu
           cmp     cl,8                ;  na osnovu stare pozicije i pomaka
           jb      NONEXTBYT
           sub     cl,8
           add     ax,1
           adc     dx,0
NONEXTBYT: add     ax,SMPBUFPTR
           adc     dx,SMPBUFPTR+2
UPDATEPTR: mov     SMPBUFPTR,ax        ; Oznaci novu poziciju
           mov     SMPBUFPTR+2,dx
           mov     SMPBITPTR,cl
           or      dx,dx               ; Provjeri da li je doslo vrijeme za
           jnz     SMPOVRFLW           ;  citanje novog bloka
           cmp     ax,EDGE_LEN
           ja      SMPOVRFLW
           jne     GETSMPBYT
           cmp     cl,LASTUSED
           jae     SMPOVRFLW
GETSMPBYT: mov     bx,ax               ; Uzmi sempl iz bafera
           mov     ds,BUFPTR+2
           mov     al,[bx+1]
           rol     al,cl
           shr     al,1
           mov     SAMPLE,al
           test    ULAMODE,2           ; Generisi zvuk, ako treba
           jz      NOEARSND
           mov     ah,al
           rol     ah,3
           in      al,61H
           and     al,0FCH
           and     ah,02H
           or      al,ah
           out     61H,al
NOEARSND:  mov     ds,MEMSEG
           popa
           mov     ah,SAMPLE
           ret
SMPOVRFLW: call    TSTSMPBLK           ; Ispitaj ima li iza jos sempl blokova
           jnz     NOMORESMP
           sub     ax,EDGE_LEN         ; Izracunaj poziciju u novom bloku
           sbb     dx,0
           sub     cl,LASTUSED
           jnc     NOADJLAST
           add     cl,8
           sub     ax,1
           sbb     dx,0
NOADJLAST: call    GETSMPBLK           ; Ucitaj novi blok u bafer
           jmp     UPDATEPTR
NOMORESMP: mov     SAMPLEON,0          ; Izlaz iz "sample" moda
           call    MAXLSOFF
           mov     EARLOADED,0
           popa
           mov     ah,255
           ret
EARSAMPLE  endp

CSEG2      ends

CSEG       segment public

           assume  cs:CSEG,ss:SSEG,ds:ZXDSEG,es:nothing

;
; Simulacija rada sa kasetofonom. Kasetofon se ne simulira preko ulazno/izlazne
; mape, nego preko trapa ubacenog na adrese 1378 i 1222 u Spectrumov ROM, koji
; na sebe preuzima ulogu rutina LD_BYTES i SA_BYTES respektivno.
;

;
; Rutina LD_BYTES. U IX je startna adresa, u DE duzina, a u A fleg. Ako je CF=0
; vrsi se VERIFY, a ako je CF=1 vrsi se LOAD. Ako nije ucitavanje korektno, CF
; je na izlazu resetovan, inace je setovan. Simuliran je i "bag" ako je D=0FFH,
; jer se neki programi zasnivaju na ovom bagu.
;


LD_BYTES:  pushf                       ; Ovo je zbog SSTEP
           push    0
           popf
           mov     A,ALTA
           call    GETTAPE
           pushf
           call    TSTSMPBLK           ; Ako je slijedeci blok sempl, pripremi
           jnz     NONEXTSMP           ;  teren za prelazak u "sample" mod
           mov     SMPSTARTER,100
NONEXTSMP: popf
           xchg    HL,DE
           mov     DE,0
           mov     FLAGSL,0
           push    PC
           jumps
           jnc     LDERR
           nojumps
           cmp     SA_LD_MON,1         ; Ne smije se parity ucitati u monitor
           jne     NOSKIPPAR
           dec     EDGE_LEN
           cmp     EDGE_LEN,1
           jne     NOSKIPPAR
           jmp     LDERR
NOSKIPPAR: xor     PC,PC
           cmp     HL,0FF00H
           jae     LOADPREP
           mov     ds,BUFPTR+2
           mov     E,ds:[0]
           mov     ds,MEMSEG
           cmp     SA_LD_MON,1         ; Nebitan je flegbajt za monitor L
           je      LDPUTMEM
           mov     D,E
           cmp     A,E
           je      LDPUTMEM
           jmp     short LDERR1
LDPUTMEM:  dec     EDGE_LEN            ; Sad smjesti ucitanu datoteku u
           inc     PC                  ;  Spectrumovu memoriju vodeci racuna o
LOADPREP:  mov     AUX,IX              ;  video memoriji, itd.
LOADLOOP:  push    es
           mov     es,BUFPTR+2
           mov     ah,es:[PC]
           xor     D,ah
           pop     es
           test    ALTF,1
           jz      VERIFY
           cmp     AUX_H,high 16384
           jb      NOVERIFY
           mov     [AUX],ah
           if      VER_128
           mov     es:[AUX],ah
           cmp     AUX,MMUVAR
           jbe     NOVERIFY
           call    ACT_AUX
           endif
           jmp     short NOVERIFY
VERIFY:    mov     A,[AUX]
           mov     E,ah
           cmp     A,ah
           jne     LDERR1
NOVERIFY:  inc     AUX
           inc     PC
           dec     HL
           mov     IX,AUX
           mov     E,1
           dec     EDGE_LEN
           jz      LDERR
           or      HL,HL
           jnz     LOADLOOP
           push    es
           mov     es,BUFPTR+2
           mov     E,es:[PC]
           xor     D,E
           pop     es
           or      D,D
           jne     LDERR1
           mov     FLAGSL,1
           jmp     short LDERR1
LDERR:     mov     FLAGSL,50H
LDERR1:    pop     PC
           xchg    HL,DE
           jmp     RETORMON            ; Povratak na RET ili u monitor

;
; Rutina LD_EDGE.
;

LD_EDGE:   pushf                       ; Iskljuci trap fleg
           push    0
           popf
           jmp     EDGE_ADDR           ; Indirektni skok

LEADER:    mov     B,255               ; Vodeci signal
           mov     LDCNT,30 ; 60
           dec     EDGE_CNT
           jnz     LEADEND
           mov     EDGE_CNT,2
           mov     EDGE_ADDR,offset SYNC
LEADEND:   jmp     short EDGE_OK

SYNC:      mov     LDCNT,10 ; 20        ; Sinhro bajt
           dec     EDGE_CNT
           jnz     EDGE_OK
           mov     EDGE_ADDR,offset BITS
           mov     EDGE_TYP,0
           mov     EDGE_PTR,0
           jmp     short EDGE_OK

BITS:      mov     LDCNT,10 ; 20        ; Korisni dio informacija
           xor     EDGE_TYP,255
           jnz     EDGE_OK
           cmp     EDGE_CNT,0
           jnz     EDGE_NEXT
           dec     EDGE_LEN
           cmp     EDGE_LEN,-1
           jumps
           je      EDGE_BAD
           nojumps
           mov     ds,BUFPTR+2         ; Pripremi novi bajt
           mov     AUX,EDGE_PTR
           mov     ah,[AUX]
           mov     ds,MEMSEG
           inc     EDGE_PTR
           mov     EDGE_BYT,ah
           mov     EDGE_CNT,8
EDGE_NEXT: dec     EDGE_CNT            ; Dobavi korespodentni bit
           shl     EDGE_BYT,1
           jnc     EDGE_OK
           mov     B,255

EDGE_OK:   test    ULAMODE,2           ; Preskoci generisanje zvuka ako ne treba
           jz      EDGE_CONT
           mov     SPROTECT,3          ; Ovo sprecava da OUT pokvari zvuk
           in      al,61H
           or      al,3
           out     61H,al
           cmp     B,255
           jnz     BIT0
           mov     al,250              ; Zvuk za bit 1
           out     42H,al
           mov     al,4
           out     42H,al
           jmp     short EDGE_CONT
BIT0:      mov     al,169              ; Zvuk za bit 0
           out     42H,al
           mov     al,2
           out     42H,al
EDGE_CONT: cmp     EDGE_LEN,0          ; Ugasi zvuk ako je kraj
           jnz     EDGE_C1
           in      al,61H
           and     al,0FCH
           out     61H,al
EDGE_C1:   cmp     B,255               ; Male svinjarije sa brojacem T stanja
           je      EDGE_HCK1           ;  da se postugnu sto vjerniji BORDER
           mov     CLK,130             ;  efekti
           jmp     short EDGE_HCK2
EDGE_HCK1: add     CLK,27
           jnc     EDGE_HCK2
           mov     CLK,255
EDGE_HCK2: cmp     BRKABLE,1           ; Testiraj BREAK ako je to dozvoljeno
           je      EDGE_BRK
EDGE_EXIT: popf
           mov     PC,EDGE_AFTER       ; Nastavi EDGE potprogram iza kriticnog
           dec     PC                  ;  dijela
           jmp     NOP_
EDGE_BRK:  test    KBDROWS[7],1        ; Test BREAK tipke
           jnz     EDGE_EXIT
           push    cs
           call    near ptr KILLEDGE   ; BREAK je pritisnut
           mov     FLAGSL,0
           mov     A,KBDROWS[7]        ; Prekini cijeli EDGE potprogram
           rcr     A,1
           popf
           jmp     RET_

EDGE_BAD:  push    cs
           call    near ptr KILLEDGE    ; Los tranzijent
           xor     B,B
           mov     A,B
           mov     FLAGSL,50H           ; Prekini cijeli EDGE potprogram
           popf
           jmp     RET_

;
; Priprema za EDGE.
;

ED_PREP:   mov     LDCNT,0
           call    GETTAPE
           jnc     EDGE_BAD
           call    TSTSMPBLK           ; Pripremi teren za "sample" mod ako je
           jnz     ED_NONXSP           ;  slijedeci blok sempl
           mov     SMPSTARTER,100
ED_NONXSP: mov     al,0B6H             ; Priprema za zvuk
           out     43H,al
           mov     EDGE_ADDR,offset LEADER
           jmp     EDGE_OK

;
; Ponistavanje EDGE rutine.
;

KILLEDGE   proc    far
           push    ax
           mov     SPROTECT,0
           in      al,61H
           and     al,0FCH
           out     61H,al
           pop     ax
           mov     EDGE_ADDR,offset ED_PREP
           cmp     EDGEOLD,0
           jne     KE2
           ret
KE2:       push    si
           call    MAXLSOFF
           mov     si,EDGEOLD
           push    bx
           mov     bx,EDTABLE
           mov     ss:[bx],si
           pop     bx
           pop     si
           mov     EDGEOLD,0
           ret
KILLEDGE   endp

;
; Rutina SA_BYTES. U IX je startna adresa, u DE duzina, a u A fleg.
;

WRBLK      macro   ADR,LEN             ; Write block poziv iz DOS-a
           mov     ah,40H
           mov     cx,LEN
           lea     dx,ADR
           int     21H
           endm

SA_BYTES:  pushf                       ; Ovo je zbog SSTEP
           push    0
           popf
           push    STP
           mov     REQ_LEN,DE          ; Duzina
           mov     REQ_FLAG,A          ; Fleg (0=header)
           mov     IFF,0
           or      FLAGSL,1
           call    RESTTIMER           ; Preusmjeri na normalne vektore timer
           push    ds
           mov     ds,STEKSEG
           lea     dx,SEARCHREC[1EH]   ; Otvori "traku" (ovdje joj je ime), i
           mov     ax,3D02H            ;  za citanje i za pisanje
           int     21H
           mov     bx,-1
           jnc     SAVEPROC
           pop     ds
           jmp     SAERROR
SAVEPROC:  mov     bx,ax               ; Handler u BX
           mov     ax,CUR_LINK         ; Ako smo na samom pocetku trake, treba
           or      ax,CUR_LINK+2       ;  naci prvu datoteku
           jnz     SNOFIRST
           READBLK CUR_LINK,4
           READBLK DUMMYPTR,4          ; Provjeri da li je Warajevo format
           READBLK DUMMYPTR,4
           mov     ax,DUMMYPTR
           and     ax,DUMMYPTR+2
           inc     ax
           jz      SNOFIRST
           mov     NONWARTAP,1         ; Rad sa Non-Warajevo formatom
           mov     CUR_LINK,0
           mov     CUR_LINK+2,0
           mov     BACK,0
           mov     BACK+2,0
SNOFIRST:  LSEEK   CUR_LINK            ; Pozicioniraj se na trazenu datoteku
           inc     CURBLK
           READBLK BACK,4              ; Ucitaj pokazivac na prethodnu datoteku
           mov     ax,4202H            ; Fizicki se pozicioniraj na kraj
           xor     cx,cx
           xor     dx,dx
           int     21H
           mov     NEW_DAT,ax          ; Zapamti gdje je fizicki kraj
           mov     NEW_DAT+2,dx
           cmp     NONWARTAP,0
           jumps
           jnz     SANONWAR
           nojumps
           LSEEK   CUR_LINK            ; Povezi pokazivace (Warajevo TAP)
           WRBLK   NEW_DAT,4
           LSEEK   BACK
           mov     ax,BACK
           or      ax,BACK+2
           jz      SFIRST
           READBLK DUMMYPTR,4
SFIRST:    WRBLK   NEW_DAT,4
           LSEEK   NEW_DAT
           WRBLK   BACK,4
           WRBLK   CUR_LINK,4
           WRBLK   REQ_LEN,3           ; Zapisi duzinu i flegbajt
           pop     ds
SAMAINBLK: mov     cx,REQ_LEN
           jcxz    SLEN0               ; Ako je blok duzine 0, ne snimaj nista
           mov     dx,IX
           mov     ah,40H              ; Sad snimi datoteku (blok)
           int     21H
           jc      SAERROR
           cmp     ax,cx               ; Provjeri da li je sve snimljeno
           je      SLEN0
           jmp     short SAERROR
SANONWAR:  add     REQ_LEN,2           ; Snimanje na Non-Warajevo traku
           WRBLK   REQ_LEN,3
           sub     REQ_LEN,2
           pop     ds
           mov     cx,REQ_LEN
           mov     si,IX               ; Konstruisi bajt pariteta
           mov     ah,REQ_FLAG
SAPARLP:   xor     ah,[si]
           inc     si
           loop    SAPARLP
           mov     PARBYTE,ah
           jmp     SAMAINBLK
SAERROR:   call    BUZZ
           mov     FLAGSL,0
           mov     IFF,0404H
           jmp     short SACLOSE
SLEN0:     cmp     NONWARTAP,0
           je      SACLOSE
           push    ds                  ; Za Non-Warajevo TAP snimi parity bajt
           mov     ds,STEKSEG          ;  i ostavi link na kraj trake
           WRBLK   PARBYTE,1
           pop     ds
           jc      SAERROR
           mov     ax,4202H
           xor     cx,cx
           xor     dx,dx
           int     21H
           mov     CUR_LINK,ax
           mov     CUR_LINK+2,dx
           mov     BACK,0
           mov     BACK+2,0
SACLOSE:   mov     ah,3EH              ; Zatvori "traku"
           int     21H
           mov     ds,MEMSEG
           call    SETTIMER            ; Vrati timer za simulator
           pop     STP
           mov     AUX,REQ_LEN
           add     IX,AUX
           inc     IX
           xor     DE,DE
RETORMON:  popf                        ; Skok na SA/LD-RET potprogram u ROM-u
           cmp     SA_LD_MON,1         ; Da li je pozvano iz monitora?
           jz      TOMON
           jmp     RET_
TOMON:     mov     SA_LD_MON,0
           ret

;
; SAVE i LOAD iz monitora.
;

SAVEFORMON proc    far
           pusha
           push    IFF
           push    ds
           push    ax
           if      VER_128
           mov     es,ALTMEMSEG
           else
           mov     es,VIDEOSEG
           endif
           pop     ax
           pop     ds
           mov     SA_LD_MON,1
           mov     IX,bx
           mov     DE,cx
           call    SA_BYTES
           call    RESTTIMER
           mov     SA_LD_MON,0
           pop     IFF
           popa
           ret
SAVEFORMON endp

LOADFORMON proc    far
           pusha
           if      VER_128
           mov     es,ALTMEMSEG
           else
           mov     es,VIDEOSEG
           endif
           mov     ds,MEMSEG
           mov     SA_LD_MON,1
           mov     IX,cx
           mov     DE,65535-256        ; Da ne ucitam flegbajt
           mov     ALTA,255
           mov     ALTF,1
           call    LD_BYTES
           call    RESTTIMER
           mov     SA_LD_MON,0
           popa
           ret
LOADFORMON endp


;
; Potprogram za izbor nacina ucitavanja (na ulazu AX=0 ili AX=3 u zavisnosti
; kakav je entry point u tabeli instrukcija u tom trenutku).
;

SETLMODE   proc    far
           push    bx
           inc     LOAD_MODE           ; Prilagodi simulator novom algoritmu
           cmp     LOAD_MODE,2         ;  za rad sa trakom
           je      LMODE2
           cmp     LOAD_MODE,3
           je      LMODE3
           cmp     LOAD_MODE,4
           je      LMODE4
           cmp     LOAD_MODE,5
           je      LMODE0
           lea     bx,CALL_SLOW        ; SLOW: INAN i CALL_SLOW
           add     bx,ax
           mov     TABLE1[410],bx
           pop     bx
           ret
LMODE2:    lea     bx,CALL_            ; FAST: INAN_FAST i CALL_
           add     bx,ax
           mov     TABLE1[410],bx
           lea     bx,INAN_FAST
           add     bx,ax
           mov     TABLE1[438],bx
           pop     bx
           ret
LMODE3:    lea     bx,CALL_SLOW        ; LPT1: INAN_FAST i CALL_SLOW
           add     bx,ax
           mov     TABLE1[410],bx
           mov     STOP_MODE,0
           pop     bx
           ret
LMODE4:    cmp     ax,3
           jne     LMODE4A
           cmp     BLASTADDR,0
           je      LMODE0
LMODE4A:   mov     STOP_MODE,0         ; SBST: isto
           call    SBINSETUP
           pop     bx
           ret
LMODE0:    call    DSPRESET
           mov     LOAD_MODE,0         ; AUTO: INAN i CALL_
           lea     bx,INAN
           add     bx,ax
           mov     TABLE1[438],bx
           lea     bx,CALL_
           add     bx,ax
           mov     TABLE1[410],bx
           pop     bx
           ret
SETLMODE   endp

;
; Kreira E.TAP ako nije navedeno ime.
;

CREATETAPE:cmp     TCLFLAG,1
           jz      WITHNAME
           mov     TMNAME,'E'
WITHNAME:  push    ds
           mov     ds,STEKSEG
           xor     cx,cx
           lea     dx,TMNAME           ; Otvori traku
           mov     ah,3CH
           int     21H
           jc      CREATEXIT
           mov     bx,ax               ; Snimi blok
           WRBLK   EMPTAP,12
           jc      CREATEXIT
           cmp     ax,cx
           stc
           jne     CREATEXIT
           mov     ah,3EH              ; Zatvori "traku"
           int     21H
           clc
CREATEXIT: pop     ds
           ret

CSEG       ends

ZXDSEG     segment public
ZXDSEG     ends

ZXDSEG2    segment public
ZXDSEG2    ends

;
; Ovaj segment se koristi za dekompresiju i snapshoot.
;

ZXEXPBUF   segment public
ZXEXPBUF   ends

           end
