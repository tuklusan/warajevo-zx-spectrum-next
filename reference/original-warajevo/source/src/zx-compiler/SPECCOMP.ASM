;
;          WARAJEVO Spectrum Simulator    Release 2.0
;		 (C) Zeljko Juric & Samir Ribic
;

           .186

           warn

A          equ     al

BC         equ     cx
B          equ     ch
C          equ     cl

DE         equ     dx
D          equ     dh
E          equ     dl

HL         equ     di

AUX        equ     bx
AUX_H      equ     bh
AUX_L      equ     bl

PC         equ     si
STP        equ     bp

           extrn   ADJUSTTBL:near,GETINST:near
           extrn   EXPLODE:far

           public  RESTTIMER,SETTIMER
           public  BUZZ,MEMSEG,STEKSEG
           public  PORT_IN,PORT_OUT
           public  FLAGS,FLAGSL,FLAGSH
           public  ATTRCHG,VIDEOCHG
           public  EXIT,SLOW_MAIN

           if      VER_128

           public  ALTMEMSEG,MMU,MMUSTATE,MMUVAR
           public  ACT_HL,ACT_AUX,ACT_STP
           public  XVIDEOCHG,XATTRCHG

           endif

;
; ZXDSEG simulira memoriju ZX Spectruma.
;

ZXDSEG     segment public
ZXDSEG     ends

;
; Alternativna memorijska stranica za Spectrum 128.
;

           if      VER_128

ZXALTDSEG  segment public
ZXALTDSEG  ends

           endif

;
; ZXDSEG2 je bafer za ucitavanje sa kasete, a ZXEXPBUF bafer za dekompresiju.
; ZXDSEG2 se koristi i za preklapanje ROM-a za ZX INTERFACE 1. ZXEXPBUF se
; koristi i za Snapshot, jer i on ima kompresiju.
;

ZXDSEG2    segment public

           db      65535 dup (?)

ZXDSEG2    ends

ZXEXPBUF   segment public

SNAPA      db      ?                  ; Snapshot A registar
SNAPF      db      ?                  ; Snapshot F registar
SNAPBC     dw      ?                  ; Snapshot BC registar
SNAPHL     dw      ?                  ; Snapshot HL registar
SNAPPCOLD  dw      ?                  ; Do verzije 1.45 PC registar, kasnije 0
SNAPSP     dw      ?                  ; Snapshot SP registar
SNAPI      db      ?                  ; Snapshot I registar
SNAPR      db      ?                  ; Snapshot R registar bez bita 7
SNAPSTTS   db      ?                  ; Neki snapshot flegovi
SNAPDE     dw      ?                  ; Snapshot DE registar
SNAPBCP    dw      ?                  ; Snapshot BC' registar
SNAPDEP    dw      ?                  ; Snapshot DE' registar
SNAPHLP    dw      ?                  ; Snapshot HL' registar
SNAPAP     db      ?                  ; Snapshot A' registar
SNAPFP     db      ?                  ; Snapshot F' registar
SNAPIY     dw      ?                  ; Snapshot IY registar
SNAPIX     dw      ?                  ; Snapshot IX registar
SNAPIFF1   db      ?                  ; Snapshot IFF1 (0/1)
SNAPIFF2   db      ?                  ; Snapshot IFF2 (0/1)
SNAPIM     db      ?                  ; Snapshot interapt mod
SNAPADD    dw      ?                  ; Normalno 23, 54 za verziju 3.0
SNAPPC     dw      ?                  ; Snapshot PC registar
SNAPHWMOD  db      ?                  ; Snapshot hardverski mod
SNAP7FFD   db      ?                  ; Snapshot MMU stanje
SNAPIN1ON  db      ?                  ; 255 ako je aktivan Shadow ROM
SNAPRLDIR  db      ?                  ; Flegovi za R i LDIR emulaciju
SNAPFFFD   db      ?                  ; Snapshot aktivni zvucni registar
SNAPSOREG  db      16 dup (?)         ; Snapshot zvucni registri
SNAPDTLEN  dw      ?                  ; Snapshot duzina bloka
SNAPPAGE   db      ?                  ; Snapshot stranica
SNAPDATA   db      65477 dup (?)      ; Snapshot podaci

ZXEXPBUF   ends

;
; Dodatna memorija za slucaj 128 K spectruma.
;

           if      VER_128

ZXEXTRA1   segment public

PAGE_0     db      16384 dup (?)
PAGE_1     db      16384 dup (?)

ZXEXTRA1   ends

ZXEXTRA2   segment public

PAGE_3     db      16384 dup (?)
PAGE_4     db      16384 dup (?)
PAGE_6     db      16384 dup (?)
PAGE_7     db      16383 dup (?)

ZXEXTRA2   ends

ZXDSEG3    segment public

           db      65535 dup (?)

ZXDSEG3    ends

           endif

;
; SSEG sadrzi interni stek, tabele, i sistemske varijable. Podijeljen je u 2
; dijela (da se smanji djubre pri linkovanju): SSEG1 sadrzi varijable koje imaju
; inicijalne vrijednosti, a SSEG2 sadrzi varijable koje se inicijaliziraju nakon
; pokretanja programa.
;

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   TMNAME:byte
           extrn   PAGEWORDS:word
           extrn   TABLE1:word

TTIMER     equ     23864               ; Period za timer

;
; Ovo su sjenke za boje na Herculesu, CGA i EGAMONO karticama.
;

BOJE       label   word

           db      00000000B           ; Crna
           db      00000000B

           db      01010101B           ; Plava
           db      00000000B

           db      10101010B           ; Crvena
           db      10101010B

           db      11111111B           ; Ljubicasta
           db      00000000B

           db      10011001B           ; Zelena
           db      01100110B

           db      01010101B           ; Cijan
           db      10101010B

           db      11111111B           ; Zuta
           db      10101010B

           db      11111111B           ; Bijela
           db      11111111B

;
; Kombinacije boja na CGA ako se koristi Windows.
;

WINBOJE1   label   word                ; Prva paleta

           db      0,0,33H,0CCH,0AAH,0AAH,22H,88H
           db      55H,55H,11H,44H,66H,99H,255,255

WINBOJE2   label   word                ; Druga paleta

           db      0,0,11H,44H,22H,88H,0AAH,0AAH
           db      77H,0DDH,55H,55H,0BBH,0EEH,255,255

;
; Pokazivac na tabelu boja.
;

BOJEPTR    dw      BOJE

;
; Tabela za konverziju atributa na EGA i VGA
;

ATRCONV    label   byte
           rept    2
           db      0,1,4,5,2,3,6,7
           db      16,17,20,21,18,19,22,23
           db      64,65,68,69,66,67,70,71
           db      80,81,84,85,82,83,86,87
           db      32,33,36,37,34,35,38,39
           db      48,49,52,53,50,51,54,55
           db      96,97,100,101,98,99,102,103
           db      112,113,116,117,114,115,118,119
           db      0,9,12,13,10,11,14,15
           db      144,153,156,157,154,155,158,159
           db      192,201,204,205,202,203,206,207
           db      208,217,220,221,218,219,222,223
           db      160,169,172,173,170,171,174,175
           db      176,185,188,189,186,187,190,191
           db      224,233,236,237,234,235,238,239
           db      240,249,252,253,250,251,254,255
           endm

;
; Podaci za Hercules karticu.
;

HGCCARD    dw      0B000H,0
           dw      H_VID_UPD,H_A_UPD
           dw      H_TEKST,H_FLASH,H_STOP_F
           dw      H_GRAPH,H_BORDER

;
; Podaci za Hercules karticu, bez atributa.
;

HGCMCARD   dw      0B000H,0
           dw      H_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY
           dw      H_GRAPH,DUMMY

;
; Podaci za CGA karticu.
;

CGACARD    dw      0B800H,0
           dw      C_VID_UPD,C_A_UPD
           dw      C_TEKST,DUMMY,DUMMY
           dw      C_GRAPH,C_BORDER

;
; Podaci za CGA bez atributa.
;

CGAMCARD   dw      0B800H,0
           dw      H_VM_UPD,DUMMY
           dw      C_TEKST,DUMMY,DUMMY
           dw      C_GRAPH,DUMMY

;
; Podaci za CGA pod Windows-om.
;

WINCARD    dw      0B800H,-6
           dw      C_VID_UPD,C_A_UPD
           dw      C_TEKST,DUMMY,DUMMY
           dw      W_GRAPH,C_BORDER

;
; Podaci za VGA karticu.
;

VGACARD    dw      0A000H,328
           dw      V_VID_UPD,V_A_UPD
           dw      V_TEKST,V_FLASH,V_STOP_F
           dw      V_GRAPH,V_BORDER

;
; Podaci za EGA karticu.
;

EGACARD    dw      0A000H,6328
           dw      V_VID_UPD,V_A_UPD
           dw      E_TEKST,V_FLASH,V_STOP_F
           dw      E_GRAPH,V_BORDER

;
; Podaci za EGAMONO karticu.
;

EMONOCARD  dw      0A000H,6328
           dw      H_VID_UPD,EM_A_UPD
           dw      H_TEKST,EM_FLASH,EM_STOP_F
           dw      EM_GRAPH,EM_BORDER

;
; Podaci za EGAMONO karticu bez atributa.
;

EGAMMCARD  dw      0A000H,6328
           dw      H_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY
           dw      EM_GRAPH,DUMMY

;
; Podaci za Extended Hercules.
;

EXTHCARD   dw      0B000H,0
           dw      X_VID_UPD,X_A_UPD
           dw      H_TEKST,H_FLASH,H_STOP_F
           dw      X_GRAPH,X_BORDER

;
; Podaci za Extended Hercules bez atributa.
;

EXTHMCARD  dw      0B000H,0
           dw      X_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY
           dw      X_GRAPH,DUMMY

;
; Vrijednosti video registara za grafiku na Herculesu (GRAPHCON = obicna,
; EXTCON = prosirena)
;

GRAPHCON   db      35H,2DH,2EH,07H,5BH,02H,58H,58H,02H,03H,00H,00H
EXTCON     db      35H,2AH,2BH,0AH,64H,02H,61H,61H,02H,03H,00H,00H

;
; Rutine za inicijalizaciju konverzionih tabela za kartice.
;

CARDS      dw      BADCARD,INITCGA,BADCARD,INITEGA,BADCARD,INITEM
           dw      INITWIN,INITHERC,INITEXTH,INITVGA,BADCARD

CARDPTR    dw      0,CSEG2             ; Vektor za daleki poziv rutine

;
; Poruke o greskama.
;

ESCAPED    db      'ษออออออออออออออออออออออออออออออออออออออออออออออออ'
           db      'อออออออออออออออออออออออออออออป',13,10
           db      'บ     This program is created using Warajevo Z80 '
           db      'compiler (release 2.0) by    บ',13,10
           db      'บ    Zeljko Juric & Samir Ribic. Thank you for '
           db      'using Warajevo Z80 compiler.   บ',13,10
           db      'ศอออออออออออออออออออออออออออออออออออออออออออออออออ'
           db      'ออออออออออออออออออออออออออออผ$'

UNEXPMSG   db      'Internal error - unexpected wraping$'
BCMESS     db      'Invalid graphic card$'
CPUERR     db      'Cannot work on 8086/8088$'
PSPERMSG   db      'Invalid command line parameter$'
SNAPERMSG  db      'Compiled file is corrupt$'
BACKERROR  db      'CS must not be changed$'

VIRTMSG    db      13,10
           db      'Warning: Procesor is in virtual 8086 mode. Sometimes this '
           db      'may cause problems.',13,10
           db      'If this program not works, it is highly recommended to '
           db      'switch off all virtual',13,10
           db      'mode drivers, like EMM386, etc. Press CTRL + C to quit or '
           db      'any other key for',13,10
           db      'continue anyway.',13,10,13,10,'$'

;
; Pomoc (/?).
;

HLPMSGS    db      '/%<num>        Run program using <num> percent of normal '
           db      'Spectrum speed',13,10
           db      '/%MAX          Run program using maximum possible speed'
           db      13,10
           db      '/JA            Use PC analog joystick instead of Kempston '
           db      'joystick',13,10
           db      '/JB            Use arrow keys instead of Sinclair 2 '
           db      'joystick',13,10
           db      '/JC            Use arrow keys instead of Cursor joystick'
           db      13,10
           db      '/JK            Use arrow keys instead of Kempston joystick'
           db      13,10
           db      '/JM            Use mouse instead of Kempston '
           db      'joystick',13,10
           db      '/JS            Use arrow keys instead of Sinclair 1 '
           db      'joystick',13,10
           if      VER_128
           db      '/S<mask>       Use PC beeper for AY sound:',13,10
           db      '                 S0 = interlace channels A, B and C',13,10
           db      '                 S1 = mask channel A, S2 = mask channel B,'
           db      ' S3 = mask channel C',13,10
           db      '                 Add values to mask more than one channel'
           db      13,10
           db      '/SA            Use AdLib compatibile sound card for AY '
           db      'sound',13,10
           endif
           db      '/VC            Use CGA monochrome graphic mode',13,10
           db      '/VW            Use CGA color graphic mode (good MS Windows'
           db      ' compatibility)',13,10
           db      '/VE            Use EGA color graphic mode',13,10
           db      '/VM            Use EGA monochrome graphic mode',13,10
           db      '/VV            Use VGA color graphic mode',13,10
           db      '/VH            Use standard Hercules graphic mode',13,10
           db      '/VX            Use extended Hercules graphic mode',13,10
           db      13,10
           db      'After running: F1 - Pause  $'
HLP4       db      'F4 - Sound  $'
HLP5       db      'F5 - Video mode  $'
HLP6       db      'F6 - Border mode$'
HLPEND     db      13,10,'               F10 - Exit',13,10,'$'

HLPTAB     dw      HLP4,HLP5,HLP6

;
; Vrijednosti segmentnih registara za program, memoriju, i stek. Na verziji 128
; MEMSEG i ALTMEMSEG nisu staticni, nego ih mijenja MMU.
;

PROGSEG    dw      CSEG
MEMSEG     dw      ZXDSEG
STEKSEG    dw      SSEG
CSEG2SEG   dw      CSEG2
DSEG2SEG   dw      ZXDSEG2
SNAPSEG    dw      ZXEXPBUF

           if      VER_128

ALTMEMSEG  dw      ZXALTDSEG
DSEG3SEG   dw      ZXDSEG3

           endif

;
; Instrukcije koje se motre za INT 0D, pripadni handleri, i duzine pripadnih
; instrukcija.
;

INSTRS     dw      1C8BH,0C8BH,148BH,3C8BH,2C8BH
           dw      0F8BH,178BH,3D8BH,1F8BH,748BH
           dw      8B3EH,348BH,5C8BH

INSTRADDR  dw      GETBX,GETCX,GETDX,GETDI,GETBP
           dw      GETBX,GETDX,GETDI,GETBX,GETSI
           dw      SASTP,GETSI,GETBX

INSTRLEN   dw      2,2,2,2,2,2,2,2,2,3,4,2,3

;
; Instrukcije iza prefiksa SS: koje se motre za INT 0D, i pripadni handleri.
;

SASTVAL    dw      6EH,4EH,56H,5EH,76H,7EH

SASTADDR   dw      GETBP,GETCX,GETDX,GETBX,GETSI,GETDI

;
; Komandne opcije i adrese pripadnih rutina.
;

PSPOPTS    db      'uv%!jmszq? ก',0

PSPCOM     dw      ULASTATE,VIDEOCRD
           dw      SETSPEED,NOWARNING,SETJOY
           dw      SETATMODE
           dw      SETAYMASK,INHKEYS
           dw      SETSYNC,SHOWHLP
           dw      AUTOTAPE,AUTOTAPE2

;
; Tablica za konverziju tastature.
;

;
;
; 0-7     = polured za IN
; 8       = nista
; 9       = funkcijska tipka
; 10-199  = dvojna tipka
; 200-255 = tipke za joystick
;

HALFROW    db      8,17,3,3,3,3,3,4,4,4,4,4,8,8,10,11
           db      2,2,2,2,2,5,5,5,5,5,8,8,6,0,1,1
           db      1,1,1,6,6,6,6,8,8,200,0,8,0,0,0,0
           db      7,7,7,8,8,8,0,8,7,7,12,9,9,9,9,9
           db      9,9,9,9,9,8,8,8,201,8,8,202,8,203,200,8
           db      204,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
           db      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
           db      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8

;
; Pripadni bajt koji vraca IN
;

INBYTE     db      0,0,254,253,251,247,239,239,247,251,253,254,0,0,0,0
           db      254,253,251,247,239,239,247,251,253,254,0,0,254,254,254,253
           db      251,247,239,239,247,251,253,0,0,0,254,0,253,251,247,239
           db      239,247,251,0,0,0,254,0,253,254

;
; Svaka dvojna tipka tretira se kao par tipki iz ove tabele
;

DBLKEYS    db      11,42,42,56,3,42,8,42,6,42,9,42,7,42,57,42

;
; Vektori za funkcijske tipke
;

FKADDR     dw      PAUSE,DUMMY,DUMMY,SETSOUND,HSETUP
           dw      SETBORDR,DUMMY,DUMMY,DUMMY,EXIT

KBDROWS    db      8 dup (255)         ; 8 poluredova tastera, setuje ih INT 9

BORCOL     db      0,1,4,5,2,3,6,7     ; Boje BORDER-a za EGA i VGA karticu

ULA        dw      ULAFULL             ; Pokazivac na rutinu za ULA simulaciju

ULAMODE    db      3                   ; Rezim za Ula simulaciju

FL_CNT     dw      30                  ; Tekuca vrijednost brojaca za FLASH

F5INIT     dw      0,CSEG2             ; F5 far pointer za color inicijalizaciju
F5INITM    dw      0,CSEG2             ; F5 far pointer za mono inicijalizaciju

KBDJOYTAB  db      16,8,2,1,4          ; Tabela za Kempston konverziju

JOYRPLC    db      11,8,6,9,7          ; Konverzija za cursor joystick
           db      6,5,2,3,4           ; Konverzija za Sinclair 1
           db      11,10,7,8,9         ; Konverzija za Sinclair 2

CALDIGS    db      100,10,1            ; Cifre za ocitavanje parametra iza /JA

           evendata

VIDSYNC    dw      150                 ; Tip video sinhronizacije

SPEED      dw      (76+1)*2            ; Brzinska konstanta za Real-Time

           if      VER_128

           evendata

MMUVAR     dw      65535               ; Varijabla za specijalne mem. akcije
MMUWORDS   dw      8192                ; Ovo je 4096 na 386 procesoru

;
; FAR pointeri na memorijske stranice
;

PAGES      dw      PAGE_0,ZXEXTRA1,PAGE_1,ZXEXTRA1
           dw      32768,ZXDSEG,PAGE_3,ZXEXTRA2
           dw      PAGE_4,ZXEXTRA2,16384,ZXDSEG
           dw      PAGE_6,ZXEXTRA2,PAGE_7,ZXEXTRA2

;
; Specijalne akcije za stranice 2, 5 i 7.
;

ACTIONS    dw      SETNORM,SETNORM,SETP2,SETNORM
           dw      SETNORM,SETP5,SETNORM,SETP7

AY_MASK    db      255,15,255,15       ; Neki registri AY cipa nemaju sve bite
           db      255,15,31,255
           db      31,31,31,255
           db      255,15,255,255

ACTMASK    db      248                 ; Maska koju postavlja /S

CURCH      db      1                   ; Tekuci kanal (A=1, B=2, C=4)

;
; Tabela specijalnih akcija za simulaciju zvucnog cipa.
;

AY_ACTS    dw      DOSOUND,DOSOUND,DOSOUND,DOSOUND
           dw      DOSOUND,DOSOUND,NOTH,DOSOUND
           dw      SETVA,SETVB,SETVC,SETENVL
           dw      SETENVL,STARTENV,NOTH,NOTH

;
; Tabela decibelskih vrijednosti za Adlib
;

DECIBEL    db      255,63,63,63,44,38,34,29,22,18,12,10,7,5,2,1

;
; Inicijalni set Sound Blaster registara za Adlib FM sintezu
;

ADLIBR     db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,33,0,0,0,0,0,0,0,0,0,0,0
           db      24,24,24,63,63,63,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,255,0,0,0,0,0,0,0,0,0,0,0
           db      240,240,240,240,240,240,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,240,0,0,0,0,0,0,0,0,0,0,0
           db      20,20,20,19,19,19,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,40,0,0
           db      10,10,10,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0
           db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

           endif

SSEG1      ends

SSEG2      segment public

           extrn   IFF:word,IFF1:byte,IFF2:byte
           extrn   I:byte,R7:byte,HSTATE:byte,PV:byte,N:byte
           extrn   IMODE:byte,IX:word,IY:word,RCLK:word
           extrn   PVN:word,ALTA:byte,ALTPVN:word,ALTBC:word
           extrn   ALTDE:word,ALTHL:word
           extrn   R:byte,PARAMSBUF:byte
           extrn   PVN:word,ALTF:word
           extrn   ENDSS:byte
           extrn   DEC_LEN:word

           if      VER_128

           extrn   DEC_LEN_2:word

           endif

TEMP_STACK db      100 dup (?)         ; Privremeni stek za INT_9
INT_STACK  db      1000 dup (?)        ; Interni stek

FLAGS      label   word                ; Na ovom mjestu na steku se cuvaju
FLAGSL     db      ?                   ; flegovi, pa je dobro imati alias ime
FLAGSH     db      ?                   ; radi mogucnosti direktnog pristupa

;
; Ovdje se nalaze tablice za konverziju video memorije i za sjencenja (ako je u
; pitanju Hercules kartica), itd. Popunjavaju se za vrijeme inicijalizacije.
;

ADR_DF     dw      6144 dup (?)        ; Konverzija adrese video memorije
ADR_A      dw      6144 dup (?)        ; Konverzija adrese atributa
MASKA      dw      256 dup (?)         ; Maska za sjencenje (Hercules)
ATTR       dw      768 dup (?)         ; Inverzija od ADR_A (Hercules)
INKPAP     dw      512 dup (?)         ; Sjenke za INK i PAPER (Hercules)

;
; Bafer za azuriranje ekrana.
;

SCRBUF     db      6913 dup (?)

;
; Podaci ovisni od graficke kartice.
;

TABLELEN   equ     9

VIDEOSEG   dw      ?                   ; Segment video memorije
VIDEOFFS   dw      ?                   ; Offset u video memoriji
UPDT_VID   dw      ?                   ; Adresa rutine za azuriranje ekrana
UPDT_ATTR  dw      ?                   ; Adresa rutine za azuriranje atributa
TEKST      dw      ?                   ; Adresa rutine za prelazak u tekst mod
FLASH      dw      ?                   ; Adresa rutine za FLASH
STOP_F     dw      ?                   ; Adresa rutine za zaustavljanje FLASH-a
GRAPH      dw      ?                   ; Adresa rutine za akriviranje grafike
BORDER     dw      ?                   ; Adresa rutine za BORDER

BRDCOL     dw      ?                   ; Boja BORDER-a za HiCol BORDER (VGA)
BRDHACK    dw      ?                   ; Mali hakerluk za HiCol BORDER

VIDEOHCK   dw      ?                   ; AdHoc kompenzacione promjenljive za
ATTRHCK    dw      ?                   ; HiCol BORDER konflikte u Low RAM-u

NAMJERNA   db      ?                   ; Kod za slucaj forsiranja kartice

LAST_01    dd      ?                   ; Stara vrijednost vektora za INT 01
LAST_08    dd      ?                   ; Stara vrijednost vektora za INT 08
LAST_09    dd      ?                   ; Stara vrijednost vektora za INT 09
LAST_0D    dd      ?                   ; Stara vrijednost vektora za INT 0D
LAST_1B    dd      ?                   ; Stara vrijednost vektora za INT 1B
LAST_FA    dd      ?                   ; Stara vrijednost vektora za INT FA

           evendata

ATTRCHG    dw      ?                   ; Brojac izmjena atributa
VIDEOCHG   dw      ?                   ; Brojac izmjena video memorije

           if      VER_128

XATTRCHG   dw      ?                   ; Isto za alternativnu video memoriju
XVIDEOCHG  dw      ?

           endif

LASTB      dw      ?                   ; Sluzi da zapamti zadnju boju BORDER-a
F5BORDER   dw      ?                   ; Indirektni poziv BORDER-a za F5

LEFTBND    dw      ?                   ; Lijevi prag za joystick
RIGHTBND   dw      ?                   ; Desni prag za joystick
UPBND      dw      ?                   ; Gornji prag za joystick
DOWNBND    dw      ?                   ; Donji prag za joystick

ACCXLO     dw      ?                   ; X akumulator za kalibraciju, niza rijec
ACCXHI     dw      ?                   ; X akumulator za kalibraciju, visa rijec
ACCYLO     dw      ?                   ; Y akumulator za kalibraciju, niza rijec
ACCYHI     dw      ?                   ; Y akumulator za kalibraciju, visa rijec

MAX        dw      ?                   ; Gornja granica za dekompresiju

CURBYTE    dw      ?                   ; Trenutni bajt koji se otpakuje

           if      VER_128

CURPAGE    db      ?                   ; Trenutni blok koji se otpakuje (0/1)

           endif

INHIBIT    db      ?                   ; 0FFH ako su onemogucene funkc. tipke

;
; Indirektni vektori za specijalne memorijske akcije na 128K Spectrumu
;

           if      VER_128

ACT_HL     dw      ?                   ; LD (HL),<byte>
ACT_AUX    dw      ?                   ; LD (AUX),<byte>
ACT_STP    dw      ?                   ; LD (SP),<byte>

           endif

FKVEC      dw      ?                   ; Vektor za trenutnu funkcijsku tipku
EXTHFLG    db      ?                   ; 1 nakon koda 224 (EXT_KBD)

FL_CREG    db      ?                   ; Upravljacki registar video chipa

ATTRMODE   db      ?                   ; Rezim za simulaciju atributa

KEMPDEV    db      ?                   ; Kempston: 0=nema, 1=mis, 2=tastatura,
                                       ; 3=analogni

KJOYVAL    db      ?                   ; Vrijednost za IN 31

JTYPE      db      ?                   ; 0=klasicno, 1=Kempston, 2=Cursor
                                       ; 7=Sinclair 1, 12=Sinclair 2

LRCOUNT    dw      ?                   ; Brojac lijevo/desno za misa
UDCOUNT    dw      ?                   ; Brojac gore/dolje za misa

VIDEOFLG   dw      ?                   ; Fleg za azuriranje ekrana

VIRTMODE   db      ?                   ; 1 ako je procesor u 'virtual 8086' modu
VIRTWARN   db      ?                   ; 1 ako ne treba ispisati VM upozorenje
MXSPD      db      ?                   ; 1 ako je MaxSpeed mod
VGAOK      db      ?                   ; 1 ako je VGA a nije MaxSpeed
BLASTPRES  db      ?                   ; 1 ako je Blaster ili ADLIB

SNAPERR    db      ?                   ; 1 ako je ostecen overlay dio

           evendata

LASTTIMER  dw      ?                   ; Zadnja vrijednost timera za Real-time

           if      VER_128

MMUSTATE   db      ?                   ; Stanje MMU

ENVCNT     dw      ?                   ; Brojac za generator envelope
ENVLEN     dw      ?                   ; Period envelope
VOLSTEP    db      ?                   ; 1=ATTACK, -1=DECAY, 0=OFF
CURVOL     db      ?                   ; Tekuca glasnoca generatora
FINVOL     db      ?                   ; Krajnja glasnoca nakon jednog ciklusa

STOPAY     db      ?                   ; 1 ako se ne simulira AY zbog F4

MMUTEMP    db      ?                   ; Privremeno cuva MMU za snapshot

AY_REG     db      ?                   ; Oznacava koji se AY registar adresira

AY_REGS    db      16 dup (?)          ; Registri zvucnog cipa

VOLA       db      ?                   ; Aktuelna glasnoca kanala A
VOLB       db      ?                   ; Aktuelna glasnoca kanala B
VOLDUMMY   db      ?                   ; Lazna varijabla
VOLC       db      ?                   ; Aktuelna glasnoca kanala C

CNTSAVE    dw      ?                   ; Stanje tajmera radi detekcije SB-a

           endif

SSEG2      ends

CSEG2      segment public

           assume  cs:CSEG2,ds:ZXDSEG,ss:SSEG,es:nothing

;
; Emulacija Kempston joysticka misem.
;

JOYMOUSE   proc    far
           pusha
           mov     ax,3
           int     33H
           push    bx
           mov     ax,11
           int     33H
           pop     bx
           rol     bx,4
           and     bx,10H
           mov     ah,bl
           add     cx,LRCOUNT
           jz      NOLR
           js      NEGLR
           dec     cx
           jmp     short NOLR
NEGLR:     inc     cx
NOLR:      mov     LRCOUNT,cx
           cmp     cx,-1
           jge     NOLEFT
           or      ah,2
NOLEFT:    cmp     cx,1
           jl      NORIGHT
           or      ah,1
NORIGHT:   add     dx,UDCOUNT
           jz      NOUD
           js      NEGUD
           dec     dx
           jmp     short NOUD
NEGUD:     inc     dx
NOUD:      mov     UDCOUNT,dx
           cmp     dx,1
           jl      NODOWN
           or      ah,4
NODOWN:    cmp     dx,-1
           jge     MEND
           or      ah,8
MEND:      mov     KJOYVAL,ah
           popa
           ret
JOYMOUSE   endp

;
; Emulacija Kempston joysticka analognim PC joystickom.
;

JOYANALOG  proc    far
           pusha
           mov     ah,84H
           mov     dx,1
           int     15H                 ; BIOS Joystick poziv
           mov     KJOYVAL,0           ; Prije mjerenja sve na nulu
           cmp     ax,LEFTBND
           jb      ANLEFT
           cmp     ax,RIGHTBND
           ja      ANRIGHT
ANUPDN:    cmp     bx,UPBND
           jb      ANUP
           cmp     bx,DOWNBND
           ja      ANDOWN
ANSTRIG:   mov     dx,201H             ; Citaj fire taster
           out     dx,al
           in      al,dx
           or      al,0CFH             ; Postavi 11xx1111
           inc     al                  ; Ako su oba xx=11 nije pritisnuto
           jz      NOSHOOT             ; => AL=255
           or      KJOYVAL,16
NOSHOOT:   popa
           ret
ANLEFT:    or      KJOYVAL,2
           jmp     ANUPDN
ANRIGHT:   or      KJOYVAL,1
           jmp     ANUPDN
ANUP:      or      KJOYVAL,8
           jmp     ANSTRIG
ANDOWN:    or      KJOYVAL,4
           jmp     ANSTRIG
JOYANALOG  endp

;
; Kalibracija analognog joysticka.
;

CALIBRATE  proc    far
           pusha
           mov     al,[bx+1]           ; Ako nema nista iza /JA izvrsi
           cmp     al,' '              ; autokalibraciju
           je      AUTOCAL
           cmp     al,13
           je      AUTOCAL
           lea     di,LEFTBND
           mov     ch,4
CALMLP1:   xor     dx,dx
           lea     si,CALDIGS
           mov     cl,3
CALMLP2:   inc     bx
           mov     al,[bx]
           cmp     al,'0'
           jb      CALERR
           cmp     al,'9'
           ja      CALERR
           sub     al,'0'
           mul     byte ptr ss:[si]
           add     dx,ax
           inc     si
           dec     cl
           jnz     CALMLP2
           mov     ss:[di],dx
           add     di,2
           dec     ch
           jnz     CALMLP1
           mov     ACCXLO,bx           ; BX se mora promijeniti
           popa
           mov     bx,ACCXLO
           stc
           ret
CALERR:    popa
           clc
           ret
AUTOCAL:   xor     ax,ax               ; Automatska kalibracija
           mov     ACCXLO,ax
           mov     ACCXHI,ax
           mov     ACCYLO,ax
           mov     ACCYHI,ax
           mov     cx,100
CALLOOP:   mov     ah,84H
           mov     dx,1
           push    cx
           int     15H
           pop     cx
           add     ACCXLO,ax
           adc     ACCXHI,0
           add     ACCYLO,bx
           adc     ACCYHI,0
           loop    CALLOOP
           mov     dx,ACCXHI
           mov     ax,ACCXLO
           mov     bx,100
           div     bx
           push    ax
           shr     ax,1
           mov     LEFTBND,ax
           pop     bx
           add     ax,bx
           mov     RIGHTBND,ax
           mov     dx,ACCYHI
           mov     ax,ACCYLO
           mov     bx,100
           div     bx
           push    ax
           shr     ax,1
           mov     UPBND,ax
           pop     bx
           add     ax,bx
           mov     DOWNBND,ax
           popa
           stc
           ret
CALIBRATE  endp

;
; Emulacija BUS IDLE porta.
;

B_IDL_FAR  proc    far
           push    bx
           push    ax
           mov     bx,VIDEOFLG         ; 255 ako je "mlaz" na gornjem ili
           sub     bx,64               ;  donjem rubu ekrana
           jb      NOATSCR
           cmp     bx,191
           ja      NOATSCR
           mov     ax,RCLK             ; Odredi X (u AH) i Y (u AL) koordinatu
           mov     al,bl               ;  tekuceg bajta na ekranu. Opseg
           sub     ah,32               ;  koordinata ce postati 0<=X<=55
           shr     ah,2                ;  odnosno 0<=Y<=191
           cmp     ah,32
           jae     NOATSCR             ; Ako je X>32 to je lijevi odnosno desni
           cmp     ah,32               ;  rub ekrana, ili retrace
           jae     NOATSCR
           mov     bh,al
	   in	   al,41H	       ; Odredi slucajno da li ce se citati
	   mov	   bl,al	       ;  video mapa ili atribut
	   in	   al,41H
	   mov	   al,bh
	   test    bl,2
	   jnz	   ATATTRS
           rol     ah,3                ; Pretvori koordinate u adresu bajta u
           xor     ah,bh               ;  video memoriji
           and     ah,199
           xor     ah,bh
           rol     ah,2
           mov     bl,ah
           shr     al,3
           or      al,64
           xor     al,bh
           and     al,248
           xor     al,bh
           mov     bh,al
           pop     ax
           mov     ah,[bx]             ; Uzmi bajt
           pop     bx
	   ret
NOATSCR:   pop     ax
           mov     ah,255
           pop     bx
           ret
ATATTRS:   mov	   bl,ah	       ; Pretvori koordinate u adresu atributa
	   xor	   bh,bh
	   and	   ax,248
	   shl	   ax,2
	   add	   bx,ax
	   add	   bx,22528
           pop     ax
	   mov	   ah,[bx]	       ; Uzmi bajt
           pop     bx
           ret
B_IDL_FAR  endp

;
; Obnavljanje stanja iz snapshot fajla.
;

           assume  ds:ZXEXPBUF

RECOVSNAP  proc    far
           push    ds
           mov     ds,SNAPSEG
           mov     al,SNAPF
           mov     FLAGSL,al
           mov     si,ax
           and     si,002AH
           mov     PVN,si
           mov     al,SNAPFP
           mov     byte ptr ALTF,al
           mov     si,ax
           and     si,002AH
           mov     ALTPVN,si
           mov     ax,SNAPBCP
           mov     ALTBC,ax
           mov     ax,SNAPDEP
           mov     ALTDE,ax
           mov     ax,SNAPHLP
           mov     ALTHL,ax
           mov     al,SNAPAP
           mov     ALTA,al
           mov     al,SNAPI
           mov     I,al
           mov     al,SNAPR
           mov     R,al
           mov     al,SNAPSTTS
           mov     R7,al
           ror     R7,1
           mov     ax,SNAPIX
           mov     IX,ax
           mov     ax,SNAPIY
           mov     IY,ax
           mov     IFF1,0
           mov     al,SNAPIFF1
           or      al,al
           je      RECOVIFF2
           mov     IFF1,4
RECOVIFF2: mov     IFF2,0
           mov     al,SNAPIFF2
           or      al,al
           je      RECIMODE
           mov     IFF2,4
RECIMODE:  mov     al,SNAPIM
           and     al,3                ; Ignorisi Lunterove dodatke
           mov     IMODE,al
           mov     A,SNAPA
           mov     BC,SNAPBC
           mov     HL,SNAPHL
           mov     STP,SNAPSP
           mov     DE,SNAPDE
           mov     PC,SNAPPC
           pop     ds
           ret
RECOVSNAP  endp

;
; Ucitavanje stanja memorije iz snapshot fajla.
;

LOADSNAP   proc    far
           pusha
           push    ds
           push    es
           mov     CURBYTE,0
           if      VER_128
           mov     CURPAGE,0
           endif
           mov     cx,30
           lea     dx,SNAPA
           mov     ds,SNAPSEG
           mov     SNAPADD,0
           call    GETBYTES
           cmp     SNAPPCOLD,0
           jne     UNSNAPERR
           mov     cx,25
           lea     dx,SNAPADD
           call    GETBYTES
           cmp     SNAPADD,23
           jne     UNSNAPERR
           if      VER_128             ; Provjera da li se ucitava u ispravnu
           cmp     SNAPHWMOD,3         ; verziju (48 ili 128)
           else
           cmp     SNAPHWMOD,0
           endif
           jne     UNSNAPERR
LSNAPLOOP: mov     cx,3
           lea     dx,SNAPDTLEN
           call    GETBYTES
           or      ax,ax
           jz      LSNAPRET
           mov     cx,SNAPDTLEN
           lea     dx,SNAPDATA
           call    GETBYTES
           lea     bx,SNAPDATA
           mov     MAX,bx
           mov     cx,SNAPDTLEN
           add     MAX,cx
           if      VER_128
           mov     di,49152            ; Na 128 verziji obnoviti strane 3-10
           mov     ah,SNAPPAGE
           cmp     ah,3
           jb      UNSNAPERR
           cmp     ah,10
           ja      UNSNAPERR
           sub     ah,3
           mov     ds,MEMSEG
           mov     es,ALTMEMSEG
           call    far ptr SNAPMMU
           mov     ds,SNAPSEG
           call    SNAPDCOMP
           jmp     LSNAPLOOP
           else
           mov     di,32768            ; Na 48 verziji legalne su samo strane
           cmp     SNAPPAGE,4          ; 4, 5 i 8
           je      LOADPG48
           mov     di,49152
           cmp     SNAPPAGE,5
           je      LOADPG48
           mov     di,16384
           cmp     SNAPPAGE,8
           je      LOADPG48
           jmp     short UNSNAPERR
LOADPG48:  call    SNAPDCOMP
           jmp     LSNAPLOOP
           endif
UNSNAPERR: mov     SNAPERR,1
LSNAPRET:  pop     es
           pop     ds
           popa
           ret
LOADSNAP   endp

;
; Snapshot dekompresija.
;

           assume  ds:ZXEXPBUF,es:ZXDSEG

SNAPDCOMP: push    ds
           push    es
           mov     es,MEMSEG
           mov     ds,SNAPSEG
           mov     al,es:[0]           ; Za svaki slucaj
           push    ax
DCLOOP:    cmp     byte ptr [bx],237
           je      CHECKED
OUTBX:     mov     al,[bx]
           stosb
DCNEW:     inc     bx
           cmp     bx,MAX
           jne     DCLOOP
           jmp     short DCEND
CHECKED:   inc     bx
           cmp     bx,MAX
           je      DCOUTLAST
           cmp     byte ptr [bx],0EDH
           je      DCMORE
           mov     al,[bx-1]
           stosb
           jmp     OUTBX
DCMORE:    inc     bx
           cmp     bx,MAX
           je      UNSNAPER1
           mov     cl,[bx]
           xor     ch,ch
           jcxz    MARKEREND
           inc     bx
           cmp     bx,MAX
           je      UNSNAPER1
MOREBXLP:  mov     al,[bx]
           stosb
           loop    MOREBXLP
           jmp     DCNEW
DCOUTLAST: mov     al,[bx-1]
           stosb
           jmp     short DCEND
MARKEREND: cmp     byte ptr es:[di-1],0
           jne     UNSNAPER1
           dec     di
           jmp     short DCEND
UNSNAPER1: mov     SNAPERR,1
DCEND:     pop     ax
           mov     es:[0],al           ; Za svaki slucaj
           pop     es
           pop     ds
           ret

           assume  ds:ZXDSEG,es:nothing

;
; Uzimanje bajtova iz bafera.
;

GETBYTES:  push    es
           push    si
           push    di
           push    bx
           mov     di,dx
           xor     ax,ax
           mov     es,DSEG2SEG
           mov     si,CURBYTE
           if      VER_128
           cmp     CURPAGE,0
           je      GETLOOP
           mov     es,DSEG3SEG
GETLOOP:   cmp     CURPAGE,0
           je      GETLPG0
           cmp     si,DEC_LEN_2
           je      ENDGET
           jmp     short GETLPCNT
GETNEWPG:  inc     CURPAGE
           xor     si,si
           mov     CURBYTE,si
           mov     es,DSEG3SEG
           jmp     GETLOOP
GETLPG0:   cmp     si,DEC_LEN
           je      GETNEWPG
           else
GETLOOP:   cmp     si,DEC_LEN
           je      ENDGET
           endif
GETLPCNT:  mov     bl,es:[si]
           mov     [di],bl
           inc     di
           inc     si
           inc     ax
           inc     CURBYTE
           loop    GETLOOP
ENDGET:    pop     bx
           pop     di
           pop     si
           pop     es
           ret

;
; Sada zapocinju rutine za inicijalizaciju konverzionih tabela za razne kartice.
; Prvo slijedi nekoliko opstih potprograma.
;

ZX_COORDS: mov     dx,cx               ; Pronalazi koordinate karaktera u
           dec     dx                  ; Spectrumovoj video memoriji
           mov     ah,dl
           and     ah,1FH              ; x spectruma
           shr     dx,5                ; adr shr 5 u dl (dh nebitan)
           mov     dh,dl               ; kopija u dh
           mov     al,dl               ; i u al
           and     dx,38C0H            ; u dl adr and $00C0, u dh adr and $0038
           shr     dh,3                ; u dh (adr and $0038) div 8
           or      dl,dh               ; u dl (adr and $00C0) +
                                       ;    (adr and $0038) div 8
           and     al,07H              ; u al adr and $0007
           shl     al,3                ; u al (adr and $0007)*8
           or      dl,al               ; y spectruma
           mov     al,dl               ; u al y
           ret

PREPARE:   mov     ax,ss               ; Priprema JUMP tabele i registre
           mov     ds,ax
           mov     es,ax
           mov     cx,TABLELEN
           lea     di,VIDEOSEG
           cld
           rep     movsw
           mov     cx,6144
           mov     di,6143*2
           ret

VMTOATTR:  xor     bx,bx               ; Rutina za nalazenje adrese atributa na
           mov     bl,dl               ; osnovu (x,y) koordinata karaktera
           and     bl,11111000B
           shl     bx,2
           add     bx,22528
           add     bl,ah
           adc     bh,0                ; Adresa atributa
           mov     ADR_A[di],bx
           sub     di,2
           ret

VE_KERNEL: call    ZX_COORDS           ; Nalazenje adrese u tablici za EGAMONO,
           push    ax                  ; EGA i VGA kartice (jezgro)
           mov     bl,80
           mul     bl                  ; u ax 80*y
           mov     bx,ax               ; sad u bx
           pop     ax                  ; vrati ax
           mov     al,ah               ; u al x
           shl     al,1                ; u al 2*x
           add     bl,al
           adc     bh,0                ; adresa u EGA i VGA kartici
           add     bx,VIDEOFFS         ; centriranje
           mov     ADR_DF[di],bx
           ret

;
; Rutina za inicijalizaciju tabela za konverziju na Herculesu.
;

INITHERC   proc    far
           push    ds
           push    es
           mov     F5BORDER,offset H_BORDER
           mov     F5INIT,offset INITHERC
           mov     F5INITM,offset INITMHERC
           lea     si,HGCCARD
           call    PREPARE
HLOOP1:    call    ZX_COORDS
           shr     al,2                ; u al y div 4
           push    ax
           mov     bl,90
           mul     bl                  ; u ax 90*(y div 4)
           mov     bx,ax               ; sad u bx
           pop     ax                  ; vrati ax
           mov     al,dl               ; u al y
           and     al,03H              ; u al (y mod 4)
           ror     al,3                ; u al visi bajt od 8192*(y mod 4)
           or      bh,al               ; u bx adresa bez x
           mov     al,ah               ; u al x
           shl     al,1                ; u al 2*x
           add     bl,al
           adc     bh,0                ; adresa u Hercules kartici
           add     bx,1363             ; centriranje, 13+90*(60/4)=1363
           mov     ADR_DF[di],bx
           call    VMTOATTR
           loop    HLOOP1
INIT_H1:   mov     cx,768              ; Inicijalizacija adresa karaktera za
           lea     di,ATTR+767*2       ; atribute
HLOOP2:    mov     bx,cx
           dec     bx
           and     bh,00000011B
           shl     bh,3
           add     bx,16384
           mov     [di],bx
           sub     di,2
           loop    HLOOP2
           lea     di,MASKA+255*2      ; Inicijalizacija maski
           mov     cx,256
HLOOP3:    mov     al,cl
           dec     al
           mov     dl,8
           xor     bx,bx
HLOOP4:    shr     al,1
           pushf
           rcr     bx,1
           popf
           rcr     bx,1
           dec     dl
           jnz     HLOOP4
           xchg    bh,bl
           mov     [di],bx
           sub     di,2
           loop    HLOOP3
HINKPAP:   mov     cx,4                ; Inicijalizacija konverzije za INK
           lea     bx,INKPAP           ;  i PAPER
           mov     bp,BOJEPTR
HLOOP5:    xor     si,si
HLOOP6:    mov     ax,[bp+si]
           xor     di,di
HLOOP7:    mov     dx,[bp+di]
           mov     [bx],dl
           mov     [bx+1],al
           mov     [bx+2],dh
           mov     [bx+3],ah
           add     bx,4
           add     di,2
           cmp     di,16
           jne     HLOOP7
           add     si,2
           cmp     si,16
           jne     HLOOP6
           loop    HLOOP5
           pop     es
           pop     ds
           ret
INITHERC   endp

;
; Rutina za inicijalizaciju tabela za konverziju na CGA kartici.
;

INITCGA    proc    far
           push    ds
           push    es
           mov     F5BORDER,offset C_BORDER
           mov     F5INIT,offset INITCGA
           mov     F5INITM,offset INITMCGA
           lea     si,CGACARD
           call    PREPARE
CLOOP:     call    ZX_COORDS
           shr     al,1                ; u al y div 2, u cf y mod 2
           pushf
           push    ax
           mov     bl,80
           mul     bl                  ; u ax 80*(y div 2)
           mov     bx,ax               ; sad u bx
           pop     ax                  ; vrati ax i cf
           popf
           jnc     CCONT
           add     bx,8192             ; dodaj 8192 ako je cf=1
CCONT:     mov     al,ah               ; u al x
           shl     al,1                ; u al 2*x
           add     bl,al
           adc     bh,0                ; adresa u CGA
           add     bx,168              ; centriranje
           add     bx,VIDEOFFS         ; Dodati eventualno pomak za /VW
           mov     ADR_DF[di],bx
           call    VMTOATTR
           loop    CLOOP
           jmp     INIT_H1
INITCGA    endp

;
; Rutina za inicijalizaciju tabela pod Windows-om.
;

INITWIN    proc    far
           mov     BOJEPTR,offset SSEG:WINBOJE2
INITWIN1:  push    ds
           push    es
           lea     si,WINCARD
           call    PREPARE
           jmp     CLOOP
INITWIN    endp

;
; Rutina za inicijalizaciju tabela za konverziju na EGAMONO kartici.
;

INITEM     proc    far
           push    ds
           push    es
           mov     F5BORDER,offset EM_BORDER
           mov     F5INIT,offset INITEM
           mov     F5INITM,offset INITEGAMM
           lea     si,EMONOCARD
           call    PREPARE
EMLOOP:    call    VE_KERNEL
           call    VMTOATTR
           loop    EMLOOP
           jmp     INIT_H1
INITEM     endp

;
; Rutina za inicijalizaciju tabela za konverziju na EGA i VGA karticama.
;

INITVGA    proc    far
           push    ds
           push    es
           lea     si,VGACARD
           jmp     short VE_I_1
INITVGA    endp

INITEGA    proc    far
           push    ds
           push    es
           lea     si,EGACARD
           mov     BORCOL[6],20
VE_I_1:    call    PREPARE
VLOOP1:    call    VE_KERNEL
           sub     di,2
           loop    VLOOP1
           mov     cx,768              ; Inicijalizacija adresa atributa na
           lea     di,ATTR+767*2       ; EGA i VGA karticama
           mov     bp,23295
VLOOP2:    mov     ax,bp
           and     al,1FH
           add     al,al
           add     al,1
           mov     bl,al
           push    bp
           and     bp,03E0H
           mov     ax,20
           mul     bp
           xor     bh,bh
           add     ax,bx
           add     ax,VIDEOFFS
           mov     [di],ax
           pop     bp
           dec     bp
           sub     di,2
           loop    VLOOP2
           mov     di,255*2            ; Inicijalizacija opisa specijalnih
           mov     cx,256              ; karaktera na EGA i VGA
VLOOP3:    mov     ax,cx
           dec     ax
           mov     ah,al
           mov     MASKA[di],ax
           sub     di,2
           loop    VLOOP3
           jmp     HINKPAP             ; Bilo je SS <= ES
INITEGA    endp

;
; Inicijalizacija za Extended Hercules.
;

INITEXTH   proc    far
           push    ds
           push    es
           mov     F5BORDER,offset X_BORDER
           mov     F5INIT,offset INITEXTH
           mov     F5INITM,offset INITMEXTH
           lea     si,EXTHCARD
           call    PREPARE
XLOOP1:    call    ZX_COORDS
           shr     al,1                ; u al y div 2, u cf y mod 2
           pushf
           push    ax
           mov     bl,84
           mul     bl                  ; u ax 84*(y div 2)
           mov     bx,ax               ; sad u bx
           pop     ax                  ; vrati ax i cf
           popf
           jnc     XCONT
           add     bx,16384            ; dodaj 16384 ako je cf=1
XCONT:     mov     al,ah               ; u al x
           shl     al,1                ; u al 2*x
           add     bl,al
           adc     bh,0                ; adresa u Hercules katrici
           add     bx,10               ; centriranje
           mov     ADR_DF[di],bx
           call    VMTOATTR
           loop    XLOOP1
           jmp     INIT_H1
INITEXTH   endp

;
; Prelazak na mono rezim kod Hercules, CGA i EGAMONO kartice.
;

INITMHERC  proc    far
           push    ds
           push    es
           call    far ptr FARSTOPF
           mov     ax,ss
           mov     ds,ax
           mov     es,ax
           mov     cx,TABLELEN
           lea     si,HGCMCARD
           lea     di,VIDEOSEG
           cld
           rep     movsw
           pop     es
           pop     ds
           ret
INITMHERC  endp

INITMCGA   proc    far
           push    ds
           push    es
           mov     ax,ss
           mov     ds,ax
           mov     es,ax
           mov     cx,TABLELEN
           lea     si,CGAMCARD
           lea     di,VIDEOSEG
           cld
           rep     movsw
           pop     es
           pop     ds
           ret
INITMCGA   endp

INITEGAMM  proc    far
           push    ds
           push    es
           call    far ptr FARSTOPF
           mov     ax,ss
           mov     ds,ax
           mov     es,ax
           mov     cx,TABLELEN
           lea     si,EGAMMCARD
           lea     di,VIDEOSEG
           cld
           rep     movsw
           pop     es
           pop     ds
           ret
INITEGAMM  endp

INITMEXTH  proc    far
           push    ds
           push    es
           call    far ptr FARSTOPF
           mov     ax,ss
           mov     ds,ax
           mov     es,ax
           mov     cx,TABLELEN
           lea     si,EXTHMCARD
           lea     di,VIDEOSEG
           cld
           rep     movsw
           pop     es
           pop     ds
           ret
INITMEXTH  endp

           if      VER_128

;
; Rutina koja zaustavlja AdLib.
;

STOPADLIB  proc    far
           cmp     BLASTPRES,1
           jne     ENDOFSTB
           push    bx
           xor     bx,bx
ADLIBS1:   mov     ah,bl
           xor     al,al
           push    cs
           call    near ptr SETSBREGS
           inc     bl
           jnz     ADLIBS1
           pop     bx
ENDOFSTB:  ret
STOPADLIB  endp

INITADLIB  proc    far
           cmp     BLASTPRES,1
           jne     ENDOFSTB
           push    bx
           xor     bx,bx
ADLIBS2:   mov     ah,bl
           mov     al,ADLIBR[bx]
           push    cs
           call    near ptr SETSBREGS
           inc     bl
           jnz     ADLIBS2
           pop     bx
           ret
INITADLIB  endp

           endif

;
; Aktiviranje/deaktiviranje zvuka, daleki poziv.
;

SETSNDFAR  proc    far
           xor     ULAMODE,2
           if      VER_128
           test    ULAMODE,2
           jnz     NOSTOPSB
           call    STOPADLIB
           jmp     short SKIPADLB
NOSTOPSB:  call    INITADLIB
SKIPADLB:  xor     STOPAY,1
           endif
           mov     ax,ULA
           cmp     ax,offset ULAFULL
           jne     SETSOUND1
           lea     ax,BORDONLY
           jmp     short SETSNDEND
SETSOUND1: cmp     ax,offset BORDONLY
           jne     SETSOUND2
           lea     ax,ULAFULL
           jmp     short SETSNDEND
SETSOUND2: cmp     ax,offset SPEAKER
           jne     SETSOUND3
           lea     ax,NOSPBR
           jmp     short SETSNDEND
SETSOUND3: lea     ax,SPEAKER
SETSNDEND: mov     ULA,ax
           in      al,61H
           and     al,0FCH
           out     61H,al
           ret
SETSNDFAR  endp

;
; Aktiviranje/deaktiviranje BORDER-a, daleki poziv.
;

SETBRDFAR  proc    far
           xor     ULAMODE,1
           test    ULAMODE,1
           jz      MAYBEHCL
SETBRDNRM: mov     ax,ULA
           cmp     ax,offset ULAFULL
           jne     SETBORDR1
           lea     ax,SPEAKER
           jmp     short SETBRDEND
SETBORDR1: cmp     ax,offset SPEAKER
           jne     SETBORDR2
           lea     ax,ULAFULL
           jmp     short SETBRDEND
SETBORDR2: cmp     ax,offset BORDONLY
           jne     SETBORDR3
           lea     ax,NOSPBR
           jmp     short SETBRDEND
SETBORDR3: lea     ax,BORDONLY
SETBRDEND: mov     ULA,ax
           ret
MAYBEHCL:  cmp     VGAOK,1
           jne     SETBRDNRM
           xor     ULAMODE,4
           test    ULAMODE,4
           jnz     SETHCRBRD
           push    es
           pusha
           mov     es,VIDEOSEG
           mov     cx,32768
           xor     di,di
           mov     ax,08FFH
           cld
           rep     stosw
           popa
           pop     es
           jmp     SETBRDNRM
SETHCRBRD: or      ULAMODE,1
           ret
SETBRDFAR  endp

;
; Detekcija graficke kartice, izlaz u AX (0 ako nije detektovana).
;
; CGA=1, MCGA=2, EGA=3, EGAMONO=5, HERCULES=7, VGA=9, PC_3270=10
;

DETECTCRD  proc    far
           mov     ah,0FH
           int     10H                 ; BIOS Get video state, AL - mode
           cmp     al,7                ; da li je mono mod
           je      MONO_CARD
           call    TEST_EGA            ; Provjera EGA BIOS
           jnc     NO_EGA
MAYBE_EGA: cmp     bh,1                ; BIOS detektovao mono
           mov     ax,5
           je      EGA_END             ; Ovo je EGAMONO
           call    TEST_VGA            ; Da nije mozda VGA ?
           jc      VGA_CARD
           mov     bx,0C000H           ; Provjeri signaturu BIOS-a
           mov     es,bx               ; kao alternativu za VGA
           mov     bx,39H
           cmp     word ptr es:[bx],345AH
           mov     ax,3
           jne     EGA_END
           cmp     word ptr es:[bx+2],3934H
           jne     EGA_END
VGA_CARD:  mov     ax,9                ; Ovo je VGA
EGA_END:   ret
NO_EGA:    mov     al,6                ; Ispitivanje PC 3270 kartice
           xor     cx,cx
           xor     dx,dx
           mov     ah,30H              ; Ovo je neki slabo dokumentovani
           int     10H                 ; PC 3270 BIOS poziv
           mov     ax,cx
           or      ax,dx
           jz      NO_3270
           push    ds
           mov     ds,cx
           mov     bx,dx
           mov     al,[bx+2]
           pop     ds
           or      al,al
           jz      TEST_188
           cmp     al,2
           jne     NO_3270
TEST_188:  mov     dx,188H             ; O ovom portu nema podataka
           in      al,dx
           test    al,4
           jz      NO_3270
           mov     ax,10               ; PC 3270
           ret
NO_3270:   call    TEST_VGA            ; Ovo moze biti samo CGA ili MCGA
           jnc     NO_MCGA
           mov     ax,2                ; MCGA
           ret
NO_MCGA:   mov     ax,1                ; CGA
           ret
MONO_CARD: call    TEST_EGA            ; Da nije EGA mono ?
           jc      MAYBE_EGA
           mov     dx,3BAH             ; Provjeriti Hercules
           xor     bl,bl
           in      al,dx               ; port 3BAH, MDA/EGA video status
           and     al,80H
           mov     ah,al
           mov     cx,8000H
TH_LOOP:   in      al,dx
           and     al,80H              ; Provjeriti da li nastupa
           cmp     al,ah               ; "vertical scan retrace"
           je      NO_VSRTC
           inc     bl
           cmp     bl,0AH
           jae     HERC_CARD
NO_VSRTC:  loop    TH_LOOP
           jmp     short NO_HERC
HERC_CARD: mov     ax,7                ; Ovo je HERCULES
           ret
NO_HERC:   mov     si,0B800H           ; Posljednja nada: probava se smjestiti
           mov     es,si               ; broj u video memoriju
           xor     si,si
           mov     ax,es:[si]
           not     ax
           not     word ptr es:[si]
           nop
           nop
           cmp     ax,es:[si]
           jne     NO_CARD
           mov     ax,1                ; CGA ili nesto slicno zajedno sa MDA
           ret
NO_CARD:   xor     ax,ax               ; Nije detektovana nikakva kartica
           ret
DETECTCRD  endp

TEST_EGA:  mov     ax,1200H            ; Testiranje da li EGA BIOS vraca pravilne
           mov     bl,10H              ; vrijednosti
           mov     bh,0FFH
           mov     cl,0FH
           int     10H
           cmp     cl,0CH
           jge     NO_E_BIOS
           cmp     bh,1
           jg      NO_E_BIOS
           cmp     bl,3
           jg      NO_E_BIOS
           stc
           ret
NO_E_BIOS: clc
           ret

TEST_VGA:  mov     ax,1A00H            ; Vraca CARRY ako je VGA ili MCGA
           int     10H                 ; Display type BIOS poziv (PS/2)
           cmp     al,1AH
           jne     NO_VGA
           cmp     bl,7                ; VGA na analog mono monitoru
           je      YES_VGA
           cmp     bl,8                ; VGA na analog kolor monitoru
           je      YES_VGA
           cmp     bl,0BH
           jb      NO_VGA
           cmp     bl,0CH
           ja      NO_VGA
YES_VGA:   stc
           ret
NO_VGA:    clc
           ret

;
; Simulacija AY-3-8912 zvucnog cipa.
;

           if      VER_128

AY_OUT     proc    far                 ; Postavljanje AY registara
           mov     AUX,word ptr AY_REG
           and     AUX,15
           push    ax
           and     ah,AY_MASK[AUX]
           mov     AY_REGS[AUX],ah
           add     AUX,AUX
           call    AY_ACTS[AUX]
           pop     ax
           ret
AY_OUT     endp

AY_IN      proc    far                 ; Citanje AY registara
           cmp     AY_REG,14
           jumps
           je      RS128_IN
           nojumps
           mov     AUX,word ptr AY_REG
           and     AUX,15
           mov     ah,AY_REGS[AUX]
           ret
AY_IN      endp

;
; Soundblaster/Adlib podprogram za upis zvucnih registara
; AH=registar, AL=vrijednost
;

SETSBREGS  proc    far
           push    cx
           push    dx
           push    ax
           mov     dx,388H
           xchg    ah,al
           out     dx,al               ; Upisi registar
           in      al,dx               ; sacekaj da bude spreman da primi
           in      al,dx               ;  registar (dokumentacija kaze da treba
           in      al,dx               ;  pet puta ocitati port)
           in      al,dx
           in      al,dx
           xchg    ah,al
           inc     dx
           out     dx,al
           pop     ax
           pop     dx
           pop     cx
           ret
SETSBREGS  endp

;
; Specijalne akcije za simulaciju AY cipa.
;

SETVA:     cmp     ah,16               ; Postavljanja glasnoce. 16 znaci da se
           jb      STVA1               ; glasnoca uzima iz generatora envelope
           mov     ah,CURVOL
STVA1:     mov     VOLA,ah
           jmp     DOSOUND

SETVB:     cmp     ah,16
           jb      STVB1
           mov     ah,CURVOL
STVB1:     mov     VOLB,ah
           jmp     DOSOUND

SETVC:     cmp     ah,16
           jb      STVC1
           mov     ah,CURVOL
STVC1:     mov     VOLC,ah
           jmp     short DOSOUND

STARTENV:  mov     CURVOL,15           ; Postavljanja envelope
           mov     VOLSTEP,-1
           mov     FINVOL,0
           mov     ENVCNT,0
           test    ah,4                ; Da li je ATTACK ili DECAY?
           jz      DECAY
           mov     CURVOL,0
           mov     VOLSTEP,1
           mov     FINVOL,15
DECAY:     test    AY_REGS[8],16       ; Aktiviranje kanala koji ima aktivnu
           mov     ah,CURVOL           ; envelopu
           jz      NOSTA
           mov     VOLA,ah
NOSTA:     test    AY_REGS[9],16
           jz      NOSTB
           mov     VOLB,ah
NOSTB:     test    AY_REGS[10],16
           jz      DOSOUND
           mov     VOLC,ah
           jmp     short DOSOUND

SETENVL:   mov     ax,word ptr AY_REGS[11]
           cmp     ax,100
           jae     SEL1
           mov     ax,100              ; Ne dozvoli prebrzu ni presporu
SEL1:      cmp     ax,59440            ; envelopu
           jbe     SEL2
           mov     ax,59440
SEL2:      mov     ENVLEN,ax
           ret

NOTH:      ret                         ; Nista

;
; Glavna rutina za proizvodjenje zvuka. Poziva se iz vremenskog interapta i
; nakon svake izmjene bitnijih zvucnih registara. Aktivni kanal se mijenja
; ciklicno cime se kanali preplicu i stice se utisak troglasja.
;

DOSOUND:   cmp     STOPAY,1            ; Ako je SOUND OFF ne radi nista
           jne     DSND
           ret
DSND:      cmp     BLASTPRES,1         ; Da li ima sound Blastera
           jumps
           je      DSNDADLIB
           nojumps
           push    ax                  ; Ima li aktivnih kanala?
           mov     ah,AY_REGS[7]
           or      ah,ACTMASK
           test    ah,1
           jnz     NOCHA
           test    VOLA,8
           jnz     DSNDOK
NOCHA:     test    ah,2
           jnz     NOCHB
           test    VOLB,8
           jnz     DSNDOK
NOCHB:     test    ah,4
           jnz     SNDOFF
           test    VOLC,8
           jz      SNDOFF
DSNDOK:    push    bx                  ; Pronadji prvi slijedeci aktivni kanal
           xor     bh,bh
DNSDLOOP:  mov     bl,CURCH
           mov     al,bl
           add     bl,bl
           cmp     bl,8
           jne     NOBACKA
           mov     bl,1
NOBACKA:   mov     CURCH,bl
           test    ah,al
           jnz     DNSDLOOP
           mov     bl,al
           test    VOLA[bx-1],8
           jz      DNSDLOOP
           add     bl,bl               ; Izracunaj mu frekvenciju
           cmp     bx,8
           jne     NOCHC
           mov     bx,6
NOCHC:     mov     bx,word ptr AY_REGS[bx-2]
           imul    bx,bx,11
           in      al,61H              ; Aktiviraj zvuk
           test    al,3
           pushf
           or      al,3
           out     61H,al
           popf
           jne     DSND2
           mov     al,0B6H
           out     43H,al
DSND2:     mov     al,bl
           out     42H,al
           mov     al,bh
           out     42H,al
           pop     bx
           pop     ax
           ret
SNDOFF:    in      al,61H              ; Iskljuci zvuk ako nema aktivnih kanala
           and     al,0FEH
           out     61H,al
           pop     ax
           ret

;
; Rutina za Adlib emulaciju AY-8910 chipa
;

DSNDADLIB: push    ax
           push    bx
           push    cx
           push    dx
           mov     ch,AY_REGS[7]
           mov     cl,0
           shr     ch,1
           jnc     PLAYTONA
           call    NOACTIVE
           jmp     short TONEB
PLAYTONA:  call    ADFREQ
           mov     bh,0
           mov     bl,VOLA
           and     bl,0FH
           mov     al,DECIBEL[bx]
           mov     ah,43H
           call    SETSBREGS
TONEB:     inc     cl
           shr     ch,1
           jnc     PLAYTONB
           call    NOACTIVE
           jmp     short TONEC
PLAYTONB:  call    ADFREQ
           mov     bh,0
           mov     bl,VOLB
           and     bl,0FH
           mov     al,DECIBEL[bx]
           mov     ah,44H
           call    SETSBREGS
TONEC:     inc     cl
           shr     ch,1
           jnc     PLAYTONC
           call    NOACTIVE
           jmp     short ADNOISE
           mov     bh,0
           mov     bl,VOLC
           and     bl,0FH
           mov     al,DECIBEL[bx]
           mov     ah,45H
           call    SETSBREGS
PLAYTONC:  call    ADFREQ
           mov     bh,0
           mov     bl,VOLC
           and     bl,0FH
           mov     al,DECIBEL[bx]
           mov     ah,45H
           call    SETSBREGS
ADNOISE:   mov     al,0
CHNOISEA:  shr     ch,1
           jc      CHNOISEB
           or      al,VOLA
CHNOISEB:  shr     ch,1
           jc      CHNOISEC
           or      al,VOLB
CHNOISEC:  shr     ch,1
           jc      UPDNOISE
           or      al,VOLC
UPDNOISE:  mov     bh,0
           mov     bl,al
           mov     al,DECIBEL[bx]
           mov     ah,54H
           call    SETSBREGS
FINISH:    pop     dx
           pop     cx
           pop     bx
           pop     ax
           ret

ADFREQ:    mov     bl,cl
           mov     bh,0
           add     bx,bx
           mov     bx,word ptr ss:AY_REGS[bx]
           and     bx,4095
           cmp     bx,35
           jbe     HIGHFREQ
           mov     dx,23H
           mov     ax,7731H
           push    cx
           div     bx
           pop     cx
           mov     dl,0
ADOCTAVE:  cmp     ax,400H
           jb      ADSETTING
           shr     ax,1
           add     dl,4
           jmp     ADOCTAVE
HIGHFREQ:  mov     ax,46EEH
           cmp     bx,18
           ja      SETHIGH
           mov     bx,18
SETHIGH:   xor      dx,dx
           push     cx
           div      bx
           pop      cx
           mov      dl,28
           jmp      short ADSETTING

NOACTIVE:  mov      dl,0
           jmp      short ADSETFREQ
ADSETTING: or       dl,ah
           or       dl,20H
ADSETFREQ: mov      ah,0A0H
           add      ah,cl
           call     SETSBREGS
           add      ah,10H
           mov      al,dl
           call     SETSBREGS
           ret

SNDINT     proc    far                 ; Intersegmentni poziv iz interapta
           call    DOSOUND
           call    ENVGEN
           ret
SNDINT     endp

;
; Generator "envelope". Poziva se iz vremenskog interapta.
;

ENVGEN:    cmp     VOLSTEP,0           ; Da li radi generator envelope?
           jnz     DOENV
           ret
DOENV:     push    ax                  ; Da li je doslo vrijeme za izmjenu
           add     ENVCNT,2217         ; glasnoce kanala?
           mov     ax,ENVLEN
           cmp     ENVCNT,ax
           ja      ENVLOOP
           pop     ax
           ret
ENVLOOP:   mov     ax,ENVLEN           ; Nadoknadi "izgubljeno" ako je izmedju
           sub     ENVCNT,ax           ; 2 interapta trebalo izvrsiti promjenu
           jc      ENVEND              ; za vise od jednog koraka
           call    ENVKERNEL
           jmp     ENVLOOP
ENVEND:    add     ENVCNT,ax           ; Azuriraj glasnoce kanala koji imaju
           test    AY_REGS[8],16       ; aktivnu envelopu
           mov     ah,CURVOL
           jz      NOENVA
           mov     VOLA,ah
NOENVA:    test    AY_REGS[9],16
           jz      NOENVB
           mov     VOLB,ah
NOENVB:    test    AY_REGS[10],16
           jz      NOENVC
           mov     VOLC,ah
NOENVC:    pop     ax
           ret

ENVKERNEL: mov     ah,VOLSTEP          ; Azuriraj novu glasnocu
           add     CURVOL,ah
           mov     ah,CURVOL           ; Da li je doslo do kraja jednog ciklusa?
           cmp     ah,FINVOL
           je      ENVNEXT
           ret
ENVNEXT:   test    AY_REGS[13],8       ; Da li je CONT bit?
           jnz     ENVCONT
           mov     CURVOL,0            ; Resetuj glasnocu i zavrsi
           mov     VOLSTEP,0
           ret
ENVCONT:   test    AY_REGS[13],1       ; Da li je HOLD bit?
           jz      ENVNOHOLD
           test    AY_REGS[13],2       ; Da li je ALT bit?
           jz      ENVHNOA
           xor     CURVOL,15           ; Invertuj glasnocu i zavrsi
ENVHNOA:   mov     VOLSTEP,0
           ret
ENVNOHOLD: test    AY_REGS[13],2       ; Da li je ALT bit?
           jnz     ENVALT
           xor     CURVOL,15           ; Invertuj glasnoce i nastavi
           ret
ENVALT:    neg     VOLSTEP             ; Izmjeni smjer i nastavi
           ret

RS128_IN:  mov     ah,223
           test    AY_REGS[7],64       ; Ako je u R7 b6=1, maskiraj sa R14
           jz      NOMASK
           and     ah,AY_REGS[14]
NOMASK:    retf                        ; Daleki RET!

           endif

;
; Rutine SETTIMER i RESTTIMER preusmjeravaju odnosno obnavljaju timer interrupt,
; mijenjaju njegov period i onemogucuju i omogucuju funkcijske tipke u INT 9.
;

SETTIMER   proc    far
           pushf
           cli
           push    ds
           push    dx
           push    ax
           mov     al,36H
           out     43H,al
           mov     ax,TTIMER
           out     40H,al
           mov     al,ah
           out     40H,al
           xor     ax,ax
           mov     ds,ax
           lea     ax,INT_8
           mov     ds:[32],ax
           mov     ax,CSEG
           mov     ds:[34],ax
           and     byte ptr ds:[043FH],0F0H
           mov     dx,03F2H
           mov     al,0CH
           out     dx,al
           pop     ax
           pop     dx
           pop     ds
           popf
           ret
SETTIMER   endp

RESTTIMER  proc    far
           cli
           push    ds
           push    ax
           mov     al,36H
           out     43H,al
           xor     ax,ax
           out     40H,al
           out     40H,al
           mov     ds,ax
           mov     ax,word ptr LAST_08
           mov     ds:[32],ax
           mov     ax,word ptr LAST_08+2
           mov     ds:[34],ax
           pop     ax
           pop     ds
           sti
           ret
RESTTIMER  endp

;
; Potprogrami koji generisu "klik" ton (KLIK) i ton upozorenja (BUZZ).
;

KLIK       proc    far
           pusha
           mov     dx,750
           mov     cx,10
           jmp     short KMAIN
KLIK       endp

BUZZ       proc    far
           pusha
           mov     dx,10000
           mov     cx,200
KMAIN:     mov     al,0B6H
           out     43H,al
           mov     al,dl
           out     42H,al
           mov     al,dh
           out     42H,al
           in      al,61H
           or      al,3
           out     61H,al
           push    ax
KLOOP1:    in      al,40H              ; Vremensko kasnjenje neovisno od brzine
           mov     bl,al               ;  racunara
           in      al,40H
           mov     bh,al
KLOOP2:    in      al,40H
           mov     ah,al
           in      al,40H
           xchg    ah,al
           sub     ax,bx
           jc      KNOADJ
           sub     ax,TTIMER
KNOADJ:    cmp     ax,-10000
           ja      KLOOP2
           loop    KLOOP1
           pop     ax
           and     al,0FCH
           out     61H,al
           popa
           ret
BUZZ       endp

CSEG2      ends

CSEG       segment public

           extrn   SLOW:word

           assume  cs:CSEG,ds:ZXDSEG,ss:SSEG,es:nothing

;
; Potprogrami za simulaciju nestandardnih stanja za MMU kod 128K Spectruma, tj.
; stranica 2 i 5, kao i stranice 7 kada je aktivna alternativna video memorija.
;

WRITESP    macro   BYTE
           local   RAM,ROM
           cmp     STP,23296
           jnb     RAM
           cmp     STP,16384
           jb      short ROM
           cmp     STP,22528
           adc     VIDEOCHG,0
           cmc
           adc     ATTRCHG,0
RAM:       mov     ds:[STP],BYTE
           if      VER_128
           mov     es:[STP],BYTE
           cmp     STP,MMUVAR
           jbe     ROM
           mov     ah,BYTE
           call    ACT_STP
           endif
ROM:
           endm

           if      VER_128

           irp     RP,<AUX,HL,STP>

P2_&RP:    xor     RP,16384            ; Akcije za stranicu 2
           mov     ds:[RP],ah
           mov     es:[RP],ah
           xor     RP,16384
           ret

P5_&RP:    push    RP                  ; Akcije za stranicu 5
           xor     RP,49152
           add     RP,16384
           cmp     RP,23296
           jnb     P5RAM_&RP
           cmp     RP,22528
           adc     VIDEOCHG,0
           cmc
           adc     ATTRCHG,0
P5RAM_&RP: mov     ds:[RP],ah
           mov     es:[RP],ah
           pop     RP
           ret

P7_&RP:    cmp     RP,56064            ; Akcije za stranicu 7 kada je
           jb      P7SPC_&RP           ; alternativna video memorija aktivna
           ret
P7SPC_&RP: cmp     RP,55296
           adc     XVIDEOCHG,0
           cmc
           adc     XATTRCHG,0
           ret

           endm

           endif

;
; Simulacija IN portova.
;

PORT_IN:   push    dx
           mov     dl,255
           mov     ah,dl
           mov     dh,C
           and     dh,35
           cmp     dh,35
           jne     BI0
           call    B_IDL_FAR
           jmp     short BI4
BI0:       test    C,1
           jnz     BI1
           call    KEYBOARD
           and     dl,ah
BI1:       if      VER_128
           test    C,2
           jnz     BI2
           call    IN_128
           and     dl,ah
           endif
BI2:       test    C,224
           jnz     BI3
           call    JOYSTICK
           and     dl,ah
BI3:       mov     ah,dl
BI4:       pop     dx
           ret

;
; Simulacija OUT portova.
;

PORT_OUT:  test    C,1
           jnz     BO1
           call    ULA
BO1:       if      VER_128
           test    C,2
           jnz     DUMMY
           call    OUT_128
           endif
DUMMY:     ret

;
; Opsta rutina za postavljanje BORDER-a. Malo je postala slozenija zbog potrebe
; za iskljucenjem HiCol BORDER-a.
;

ULAFAR     proc    far
           push    ax
           mov     LASTB,-1
           mov     al,ULAMODE
           push    ax
           test    al,4
           jz      ULAF1
           push    ax
           call    SETBRDFAR
           call    SETBRDFAR
           pop     ax
ULAF1:     call    ULAFULL
           pop     ax
           test    al,4
           jz      ULAF2
           call    SETBRDFAR
ULAF2:     pop     ax
           retf
ULAFAR     endp

;
; Ovo je samo interfejs za ono sto se ne moze direktno dohvatiti.
;

BUS_IDLE:  call    B_IDL_FAR
           ret

;
; Rutine za azuriranje ekrana ZX Spectruma.
;

;
; Rutina za azuriranje obicne video memorije (16384-22527). Rutina je za
; Hercules ili EGAMono karticu.
;

H_VID_RET: ret
H_VID_UPD: mov     es,STEKSEG
           repe    cmpsb
           jcxz    H_VID_RET
           mov     es,VIDEOSEG
           push    cx
           mov     bx,si               ; Pribavi u CX adresu pripadnog bajta u
           add     bx,bx               ;  video memoriji kartice, a u BP adresu
           mov     bp,ADR_A[bx-32770]  ;  korespodentnog atributa
           mov     cx,ADR_DF[bx-32770]
           mov     bl,[si-1]           ; Azuriraj bafer
           mov     ss:[di-1],bl
           xor     bh,bh               ; Ocitaj odgovarajucu masku
           add     bx,bx
           mov     dx,MASKA[bx]
           lea     ax,[si-1]           ; Adresu u video memoriji sacuvaj u AX
           mov     bl,ds:[bp]          ; Uzmi atribut u BX
           xor     bh,bh
           ror     ah,1                ; CF=1 ako je neparni red
           adc     bl,bl               ; Pomnozi atribut sa 2+CF
           jc      H_FLSH              ; Skoci naprijed ako je FLASH atribut
           add     bx,bx               ; Pripremi BX za indeksno adresiranje
           mov     ax,INKPAP[bx]       ; Uzmi sare za INK (AL) i PAPER (AH)
           mov     bx,cx               ; U BX adresa u kartici, a u CX maska
           mov     cx,dx
           and     dh,al               ; Formiraj saru za INK
           and     dl,al
           not     cx                  ; Formiraj saru za PAPER
           and     ch,ah
           and     cl,ah
           or      dx,cx               ; Formiraj kompletnu saru
           mov     es:[bx],dx          ; Smjesti je na ekran
           mov     es:[bx+32768],dx
           pop     cx
           jmp     H_VID_UPD
H_FLSH:    add     bx,bx               ; Ako je FLASH atribut, uradi isto, samo
           mov     ax,INKPAP[bx]       ;  pripremi i inverznu masku, pa u drugu
           push    dx                  ;  stranicu smjesti inverznu masku
           mov     bx,cx
           mov     cx,dx
           and     dh,al
           and     dl,al
           not     cx
           and     ch,ah
           and     cl,ah
           or      dx,cx               ; Ovo je obicna sara
           mov     es:[bx],dx
           pop     dx
           mov     cx,dx
           and     dh,ah
           and     dl,ah
           not     cx
           and     ch,al
           and     cl,al
           or      dx,cx               ; A ovo je komplementarna
           mov     es:[bx+32768],dx
           pop     cx
           jmp     H_VID_UPD

;
; Isto, za CGA karticu, samo je FLASH zamjenjen sa INVERSE.
;

C_VID_RET: ret
C_VID_UPD: mov     es,STEKSEG
           repe    cmpsb
           jcxz    C_VID_RET
           mov     es,VIDEOSEG
           push    cx
           mov     bx,si
           add     bx,bx
           mov     bp,ADR_A[bx-32770]
           mov     cx,ADR_DF[bx-32770]
           mov     bl,[si-1]
           mov     ss:[di-1],bl
           xor     bh,bh
           add     bx,bx
           mov     dx,MASKA[bx]
           lea     ax,[si-1]
           mov     bl,ds:[bp]
           xor     bh,bh
           ror     ah,1
           adc     bl,bl
           jc      C_FLSH
           add     bx,bx
           mov     ax,INKPAP[bx]
           mov     bx,cx
           mov     cx,dx
           and     dh,al
           and     dl,al
           not     cx
           and     ch,ah
           and     cl,ah
           or      dx,cx
           mov     es:[bx],dx
           pop     cx
           jmp     C_VID_UPD
C_FLSH:    add     bx,bx
           mov     ax,INKPAP[bx]
           mov     bx,cx
           mov     cx,dx
           and     dh,ah
           and     dl,ah
           not     cx
           and     ch,al
           and     cl,al
           or      dx,cx               ; A ovo je komplementarna
           mov     es:[bx],dx
           pop     cx
           jmp     C_VID_UPD

;
; Monohromatska varijanta (bez atributa) za Hercules, CGA i EGAMono.
;

H_VM_UPD:  mov     es,STEKSEG
           repe    cmpsb
           jcxz    H_VM_RET
           mov     es,VIDEOSEG
           mov     al,[si-1]
           mov     ss:[di-1],al
           mov     bp,si
           mov     bl,al               ; Prebaci bajt (u AL) u BX
           xor     bh,bh
           add     bx,bx               ; Pomnozi bajt sa 2 i ocitaj
           mov     bx,MASKA[bx]        ;  odgovarajucu masku iz tabele
           add     bp,bp
           mov     bp,ADR_DF[bp-32770]
           mov     es:[bp],bx          ; Smjesti je na ekran
           jmp     H_VM_UPD
H_VM_RET:  ret

;
; Slijedi isti potprogram (znatno prostiji) za EGA i VGA kartice.
;

V_VID_UPD: mov     es,STEKSEG
           repe    cmpsb
           jcxz    H_VM_RET
           mov     es,VIDEOSEG
           mov     al,[si-1]
           mov     ss:[di-1],al
           mov     bp,si
           add     bp,bp
           mov     bp,ADR_DF[bp-32770]
           mov     es:[bp],al
           mov     es:[bp+32768],al
           jmp     V_VID_UPD

;
; Graficke rutine za Extended Hercules. Za ovaj rezim obe teksture se smjestaju
; za svaki red piksela, a ne prva tekstura za neparne, a druga za parne redove,
; cime je citljivost znatno poboljsana, ali su rutine znatno produzene. Rutine
; su srodne rutinama za obicni Hercules rezim, pa su date bez komentara.
;

X_VID_RET: ret
X_VID_UPD: mov     es,STEKSEG
           repe    cmpsb
           jcxz    X_VID_RET
           mov     es,VIDEOSEG
           push    cx
           push    di
           mov     bx,si
           add     bx,bx
           mov     bp,ADR_A[bx-32770]
           mov     cx,ADR_DF[bx-32770]
           mov     bl,[si-1]
           mov     ss:[di-1],bl
           xor     bh,bh
           add     bx,bx
           mov     dx,MASKA[bx]
           mov     bl,ds:[bp]
           xor     bh,bh
           add     bl,bl
           jc      X_FLSH
           add     bx,bx
           mov     ax,INKPAP[bx]
           mov     bx,INKPAP[bx+2]
           mov     di,cx
           push    dx
           mov     cx,dx
           and     dh,al
           and     dl,al
           not     cx
           and     ch,ah
           and     cl,ah
           or      dx,cx
           mov     es:[di],dx
           mov     es:[di+32768],dx
           pop     dx
           mov     cx,dx
           and     dh,bl
           and     dl,bl
           not     cx
           and     ch,bh
           and     cl,bh
           or      dx,cx
           mov     es:[di+8192],dx
           mov     es:[di+40960],dx
           pop     di
           pop     cx
           jmp     X_VID_UPD
X_FLSH:    add     bx,bx
           mov     ax,INKPAP[bx]
           mov     bx,INKPAP[bx+2]
           mov     di,cx
           push    bp
           mov     bp,dx
           mov     cx,dx
           and     dh,al
           and     dl,al
           not     cx
           and     ch,ah
           and     cl,ah
           or      dx,cx
           mov     es:[di],dx
           mov     dx,bp
           mov     cx,dx
           and     dh,bl
           and     dl,bl
           not     cx
           and     ch,bh
           and     cl,bh
           or      dx,cx
           mov     es:[di+8192],dx
           mov     dx,bp
           mov     cx,dx
           and     dh,ah
           and     dl,ah
           not     cx
           and     ch,al
           and     cl,al
           or      dx,cx
           mov     es:[di+32768],dx
           mov     dx,bp
           pop     bp
           mov     cx,dx
           and     dh,bh
           and     dl,bh
           not     cx
           and     ch,bl
           and     cl,bl
           or      dx,cx
           mov     es:[di+40960],dx
           pop     di
           pop     cx
           jmp     X_VID_UPD

;
; Extended Hercules, bez atributa.
;

X_VM_UPD:  mov     es,STEKSEG
           repe    cmpsb
           jcxz    X_VM_RET
           mov     es,VIDEOSEG
           mov     al,[si-1]
           mov     ss:[di-1],al
           mov     bp,si
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           add     bp,bp
           mov     bp,ADR_DF[bp-32770]
           mov     es:[bp],bx
           mov     es:[bp+8192],bx
           jmp     X_VM_UPD
X_VM_RET:  mov     ATTRCHG,0
           ret

;
; Slijede rutine za simulaciju atributa. Prvo slijedi verzija sa "sjencenjem",
; za Hercules karticu (takodje za CGA i EGAMONO).
;

;
; Atributi za Hercules.
;

H_A_RET:   ret
H_A_UPD:   mov     es,STEKSEG
           repe    cmpsb
           jcxz    H_A_RET
R_CNT      =       0                   ; Brojac redova
           mov     es,VIDEOSEG
           push    si
           push    di
           push    cx
           mov     bx,si               ; Pribavi u BP adresu pripadnog bajta u
           add     bx,bx               ;  video memoriji kartice, a u CX adresu
           mov     cx,ATTR[bx-45058]   ;  korespodentnog karaktera
           mov     bx,cx
           add     bx,bx
           mov     bp,ADR_DF[bx-32768]
           mov     bl,[si-1]
           mov     ss:[di-1],bl        ; Azuriraj bafer
           xor     bh,bh
           add     bx,bx
           add     bx,bx
           mov     ax,INKPAP[bx]       ; Uzmi sare za INK (AL) i PAPER (AH)
           mov     dx,INKPAP[bx+2]     ; Isto, za neparne redove, u DL i DH
           test    bh,2
           mov     bh,al
           mov     bl,al
           mov     si,bx               ; INK za parne redove u SI
           mov     bh,ah
           mov     bl,ah
           mov     di,bx               ; PAPER za parne redove u DI
           jz      H_A_NOFSH
           jmp     H_A_FLASH
H_A_NOFSH: rept    8                   ; Ponovi 8 puta sekvencu
           mov     bx,cx               ; U BX adresa karaktera
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al ; Oznaci bajtove pripadnog karaktera
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]        ; Ocitaj u BX masku
           mov     ax,bx
           not     ax                  ; Inverzna maska u AX
           if      R_CNT mod 2
           and     bh,dl               ; Za neparni red koristi AL i AH
           and     bl,dl
           and     ah,dh
           and     al,dh
           else
           and     bx,si               ; a za parni red SI i DI
           and     ax,di
           endif
           or      bx,ax               ; Formiraj saru
           mov     es:[bp],bx
           mov     es:[bp+32768],bx
           if      (R_CNT mod 4) eq 3  ; Predji u novi red
           sub     bp,24486
           else
           add     bp,8192
           endif
R_CNT      =       R_CNT+1
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     H_A_UPD
H_A_FLASH: rept    8                    ; Za FLASH sve je slicno, samo sa 2 sare
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           push    bx                  ; Maska se cuva
           mov     ax,bx
           not     ax
           if      R_CNT mod 2
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           else
           and     bx,si
           and     ax,di
           endif
           or      bx,ax
           mov     es:[bp],bx          ; Ovo je prava sara
           pop     bx
           mov     ax,bx               ; Obnovi masku
           not     ax
           if      R_CNT mod 2
           and     ah,dl
           and     al,dl
           and     bh,dh
           and     bl,dh
           else
           and     ax,si
           and     bx,di
           endif
           or      bx,ax
           mov     es:[bp+32768],bx    ; Ovo je inverzna sara
           if      (R_CNT mod 4) eq 3
           sub     bp,24486
           else
           add     bp,8192
           endif
R_CNT      =       R_CNT+1
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     H_A_UPD

;
; Atributi za EGAMono. Razlikuje se samo prelazak u novi red.
;

EM_A_RET:  ret
EM_A_UPD:  mov     es,STEKSEG
           repe    cmpsb
           jcxz    EM_A_RET
ODD        =       0
           mov     es,VIDEOSEG
           push    si
           push    di
           push    cx
           mov     bx,si
           add     bx,bx
           mov     cx,ATTR[bx-45058]
           mov     bx,cx
           add     bx,bx
           mov     bp,ADR_DF[bx-32768]
           mov     bl,[si-1]
           mov     ss:[di-1],bl
           xor     bh,bh
           add     bx,bx
           add     bx,bx
           mov     ax,INKPAP[bx]
           mov     dx,INKPAP[bx+2]
           test    bh,2
           mov     bh,al
           mov     bl,al
           mov     si,bx
           mov     bh,ah
           mov     bl,ah
           mov     di,bx
           jz      EM_A_NOF
           jmp     EM_A_FLSH
EM_A_NOF:  rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           mov     ax,bx
           not     ax
           if      ODD
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           else
           and     bx,si
           and     ax,di
           endif
           or      bx,ax
           mov     es:[bp],bx
           mov     es:[bp+32768],bx
           add     bp,80               ; Na EGAMono je linearna video memorija
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     EM_A_UPD
EM_A_FLSH: rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           push    bx
           mov     ax,bx
           not     ax
           if      ODD
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           else
           and     bx,si
           and     ax,di
           endif
           or      bx,ax
           mov     es:[bp],bx
           pop     bx
           mov     ax,bx
           not     ax
           if      ODD
           and     ah,dl
           and     al,dl
           and     bh,dh
           and     bl,dh
           else
           and     ax,si
           and     bx,di
           endif
           or      bx,ax
           mov     es:[bp+32768],bx
           add     bp,80
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     EM_A_UPD

;
; Atributi za CGA. Umjesto FLASH je INVERSE.
;

C_A_RET:   ret
C_A_UPD:   mov     es,STEKSEG
           repe    cmpsb
           jcxz    C_A_RET
ODD        =       0                   ; 1 ako je neparni red
           mov     es,VIDEOSEG
           push    si
           push    di
           push    cx
           mov     bx,si
           add     bx,bx
           mov     cx,ATTR[bx-45058]
           mov     bx,cx
           add     bx,bx
           mov     bp,ADR_DF[bx-32768]
           mov     bl,[si-1]
           mov     ss:[di-1],bl
           xor     bh,bh
           add     bx,bx
           add     bx,bx
           mov     ax,INKPAP[bx]
           mov     dx,INKPAP[bx+2]
           test    bh,2
           mov     bh,al
           mov     bl,al
           mov     si,bx
           mov     bh,ah
           mov     bl,ah
           mov     di,bx
           jz      C_A_NOFSH
           jmp     C_A_FLASH
C_A_NOFSH: rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           mov     ax,bx
           not     ax
           if      ODD
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           else
           and     bx,si
           and     ax,di
           endif
           or      bx,ax
           mov     es:[bp],bx
           if      ODD
           sub     bp,8112
           else
           add     bp,8192
           endif
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     C_A_UPD
C_A_FLASH: rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           mov     ax,bx
           not     ax
           if      ODD
           and     ah,dl
           and     al,dl
           and     bh,dh
           and     bl,dh
           else
           and     ax,si
           and     bx,di
           endif
           or      bx,ax
           mov     es:[bp],bx
           if      ODD
           sub     bp,8112
           else
           add     bp,8192
           endif
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     C_A_UPD

;
; Atributi za Extended Hercules.
;

X_A_RET:   ret
X_A_UPD:   mov     es,STEKSEG
           repe    cmpsb
           jcxz    X_A_RET
ODD        =       0
           mov     es,VIDEOSEG
           push    si
           push    di
           push    cx
           mov     bx,si
           add     bx,bx
           mov     cx,ATTR[bx-45058]
           mov     bx,cx
           add     bx,bx
           mov     bp,ADR_DF[bx-32768]
           mov     bl,[si-1]
           mov     ss:[di-1],bl
           xor     bh,bh
           add     bx,bx
           add     bx,bx
           mov     ax,INKPAP[bx]
           mov     dx,INKPAP[bx+2]
           test    bh,2
           mov     bh,al
           mov     bl,al
           mov     si,bx
           mov     bh,ah
           mov     bl,ah
           mov     di,bx
           jz      X_A_NOFSH
           jmp     X_A_FLASH
X_A_NOFSH: rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           push    bx
           mov     ax,bx
           not     ax
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           or      bx,ax
           mov     es:[bp+8192],bx
           mov     es:[bp+40960],bx
           pop     bx
           mov     ax,bx
           not     ax
           and     bx,si
           and     ax,di
           or      bx,ax
           mov     es:[bp],bx
           mov     es:[bp+32768],bx
           if      ODD
           sub     bp,16300
           else
           add     bp,16384
           endif
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     X_A_UPD
X_A_FLASH: rept    8
           mov     bx,cx
           mov     al,[bx]
           mov     SCRBUF[bx-16384],al
           mov     bl,al
           xor     bh,bh
           add     bx,bx
           mov     bx,MASKA[bx]
           push    cx
           mov     cx,bx
           mov     ax,bx
           not     ax
           and     bh,dl
           and     bl,dl
           and     ah,dh
           and     al,dh
           or      bx,ax
           mov     es:[bp+8192],bx
           mov     bx,cx
           mov     ax,bx
           not     ax
           and     bx,si
           and     ax,di
           or      bx,ax
           mov     es:[bp],bx
           mov     bx,cx
           mov     ax,bx
           not     ax
           and     ah,dl
           and     al,dl
           and     bh,dh
           and     bl,dh
           or      bx,ax
           mov     es:[bp+40960],bx
           mov     bx,cx
           pop     cx
           mov     ax,bx
           not     ax
           and     ax,si
           and     bx,di
           or      bx,ax
           mov     es:[bp+32768],bx
           if      ODD
           sub     bp,16300
           else
           add     bp,16384
           endif
ODD        =       1-ODD
           inc     ch
           endm
           pop     cx
           pop     di
           pop     si
           jmp     X_A_UPD

;
; Sada slijedi simulacija atributa za EGA i VGA kartice.
;

V_A_RET:   ret
V_A_UPD:   mov     es,STEKSEG
           repe    cmpsb
           jcxz    V_A_RET
           mov     es,VIDEOSEG
           mov     al,[si-1]
           mov     ss:[di-1],al
           mov     bp,si
           add     bp,bp
           mov     bp,ATTR[bp-45058]
           lea     bx,ATRCONV
           cmp     al,128
           xlat    ATRCONV
           mov     es:[bp],al
           mov     es:[bp+80],al
           mov     es:[bp+160],al
           mov     es:[bp+240],al
           mov     es:[bp+320],al
           mov     es:[bp+400],al
           mov     es:[bp+480],al
           mov     es:[bp+560],al
           jb      NOFVGA
           rol     al,1
           rol     al,1
           rol     al,1
           rol     al,1
NOFVGA:    mov     es:[bp+32768],al
           mov     es:[bp+32848],al
           mov     es:[bp+32928],al
           mov     es:[bp+33008],al
           mov     es:[bp+33088],al
           mov     es:[bp+33168],al
           mov     es:[bp+33248],al
           mov     es:[bp+33328],al
           jmp     V_A_UPD

;
; Jedan intersegmentni poziv.
;

FARSTOPF:  call    STOP_F
           retf

;
; Simulacija FLASH na Herculesu. Svakih 0.6 sekundi izmjenjuje se graficka
; stranica. Poziva se iz vremenskog interrupta (INT_8).
;

H_FLASH:   push    dx
           mov     ax,30
           mov     FL_CNT,ax
           mov     dx,03B8H
           mov     al,FL_CREG
           xor     al,80H
           out     dx,al
           mov     FL_CREG,al
           pop     dx
           ret

H_STOP_F:  mov     dx,03B8H            ; Poziva se iz TMANAGER da obustavi
           mov     al,FL_CREG          ; izmjenu grafickih stranica
           and     al,7FH
           out     dx,al
           mov     FL_CREG,al
           ret

;
; Simulacija FLASH na EGA i VGA je slicna, samo mijenja adresu pocetka video
; memorije, a ne video stranicu.
;

V_FLASH:   push    dx
           mov     dx,03D4H
           mov     ax,30
           mov     FL_CNT,ax
           mov     al,12
           out     dx,al
           inc     dx
           mov     al,FL_CREG
           xor     al,40H
           out     dx,al
           mov     FL_CREG,al
           pop     dx
           ret

V_STOP_F:  mov     dx,03D4H
           mov     al,12
           out     dx,al
           inc     dx
           mov     al,FL_CREG
           and     al,0BFH
           out     dx,al
           mov     FL_CREG,al
           ret

;
; Na EGAMONO je potpuno isto, ali je drugi hardverski registar, i drugi XOR.
;

EM_FLASH:  push    dx
           mov     dx,03B4H
           mov     ax,30
           mov     FL_CNT,ax
           mov     al,12
           out     dx,al
           inc     dx
           mov     al,FL_CREG
           xor     al,80H
           out     dx,al
           mov     FL_CREG,al
           pop     dx
           ret

EM_STOP_F: mov     dx,03B4H
           mov     al,12
           out     dx,al
           inc     dx
           xor     al,al
           out     dx,al
           mov     FL_CREG,al
           ret

;
; Simulacija ocitavanja tastature (port 256*n+254). CX sadrzi port (bitan je CH
; jer je CL 254 ili neka druga vrijednost sa b0=0). Na ulazu je AH = 0FFH. Na
; izlazu je u AH ocitana vrijednost. Rutina se oslanja na izmjenjeni INT 9. Bit
; b6 (EAR ulaz) se ne simulira, vec se vraca kao slucajna vrijednost.
;

KEYBOARD:  push    BC                  ; AUX se ne cuva !
           lea     AUX,KBDROWS
           mov     C,8
           xchg    ah,al
           in      al,40H
           push    ax
           in      al,40H
           pop     ax
           or      al,0BFH
           xchg    ah,al
KBDLOOP:   ror     B,1
           jc      NOROW
           and     ah,ss:[AUX]
NOROW:     inc     AUX
           dec     C
           jnz     KBDLOOP
           pop     BC
           ret

;
; Simulacija Kempston dzojstika (port 31) .
;

JOYSTICK:  cmp     KEMPDEV,2
           je      JOYK
           cmp     KEMPDEV,1
           je      JOYM
           cmp     KEMPDEV,3
           je      JOYA
           ret
JOYA:      call    RESTTIMER
           call    JOYANALOG
           call    SETTIMER
           jmp     short JOYK
JOYM:      push    IFF
           mov     IFF1,0
           mov     INHIBIT,0FFH
           call    JOYMOUSE
           mov     INHIBIT,0
           pop     IFF
JOYK:      mov     ah,KJOYVAL
           ret

;
; Simulacija slanja na ULA chip (port 254). Bitovi b0, b1 i b2 kontrolisu boju
; bordera, a bit b3 zvucnik. Bit b4 (MIC izlaz) se ne simulira.
;

ULAFULL:   call    BORDONLY            ; Normalna ulazna tacka
SPEAKER:   push    ax                  ; Ulazna tacka ako je BORDER iskljucen
           ror     ah,3
           in      al,61H
           and     al,0FCH
           and     ah,02H
           or      al,ah
           out     61H,al
           pop     ax
NOSPBR:    push    ax
           and     ax,0700H
           mov     LASTB,ax
           pop     ax
           ret

BORDONLY:  push    ax                  ; Ulazna tacka ako je zvuk iskljucen
           and     ax,0700H
           cmp     ax,LASTB            ; Da li je mijenjan border ?
           mov     LASTB,ax
           je      NO_BORDER           ; Ako jeste promijeni ga, inace izadji
           call    BORDER
NO_BORDER: pop     ax
           ret

;
; Verzija bordera za Hercules karticu formira sloj oko ekrana.
;

H_BORDER:  push    di
           push    bx
           push    cx
           push    dx
           push    es
           mov     es,VIDEOSEG
           xchg    ah,al
           mov     di,ax
           add     di,di
           add     di,di
           mov     ax,INKPAP[di]
           mov     ah,al
           mov     bx,INKPAP[di+2]
           mov     bh,bl
           mov     di,1181             ; 11+90*(52/4)=1181
           mov     dh,8
           cld
HBLOOP:    mov     dl,2
HBLOOP1:   mov     cx,34
           rep     stosw
           add     di,22
           dec     dl
           jnz     HBLOOP1
           mov     dl,48
HBLOOP2:   mov     es:[di],ax
           add     di,66
           mov     es:[di],ax
           add     di,24
           dec     dl
           jnz     HBLOOP2
           mov     dl,2
HBLOOP3:   mov     cx,34
           rep     stosw
           add     di,22
           dec     dl
           jnz     HBLOOP3
           and     di,0E000H
           add     di,9373             ; 8192+1181
           xchg    ax,bx
           dec     dh
           jnz     HBLOOP
           pop     es
           pop     dx
           pop     cx
           pop     bx
           pop     di
           ret

;
; Slicno je i na CGA kartici.
;

C_BORDER:  push    di
           push    bx
           push    cx
           push    dx
           push    es
           mov     es,VIDEOSEG
           xchg    ah,al
           mov     di,ax
           add     di,di
           add     di,di
           mov     ax,INKPAP[di]
           mov     ah,al
           mov     bx,INKPAP[di+2]
           mov     bh,bl
           mov     di,6
           add     di,VIDEOFFS
           mov     dh,2
           cld
CBLOOP:    mov     dl,2
CBLOOP1:   mov     cx,34
           rep     stosw
           add     di,12
           dec     dl
           jnz     CBLOOP1
           mov     dl,96
CBLOOP2:   mov     es:[di],ax
           add     di,66
           mov     es:[di],ax
           add     di,14
           dec     dl
           jnz     CBLOOP2
           mov     dl,2
CBLOOP3:   mov     cx,34
           rep     stosw
           add     di,12
           dec     dl
           jnz     CBLOOP3
           and     di,0E000H
           add     di,8192+6
           add     di,VIDEOFFS
           xchg    ax,bx
           dec     dh
           jnz     CBLOOP
           pop     es
           pop     dx
           pop     cx
           pop     bx
           pop     di
           ret

;
; BORDER za Extended Hercules.
;

X_BORDER:  push    di
           push    bx
           push    cx
           push    dx
           push    es
           mov     es,VIDEOSEG
           xchg    ah,al
           mov     di,ax
           add     di,di
           add     di,di
           mov     ax,INKPAP[di]
           mov     ah,al
           mov     bx,INKPAP[di+2]
           mov     bh,bl
           xor     di,di
           mov     dh,8
           cld
XBLOOP:    mov     dl,96
XBLOOP2:   mov     cx,5
           rep     stosw
           add     di,64
           mov     cx,5
           rep     stosw
           dec     dl
           jnz     XBLOOP2
           and     di,0E000H
           add     di,8192
           xchg    ax,bx
           dec     dh
           jnz     XBLOOP
           pop     es
           pop     dx
           pop     cx
           pop     bx
           pop     di
           ret

;
; I EGAMONO kartica koristi isti princip.
;

EM_BORDER: push    di
           push    bx
           push    cx
           push    dx
           push    es
           mov     es,VIDEOSEG
           xchg    ah,al
           mov     di,ax
           add     di,di
           add     di,di
           mov     ax,INKPAP[di]
           mov     ah,al
           mov     bx,INKPAP[di+2]
           mov     bh,bl
           mov     di,5686
           cld
           mov     dh,2
EMBLOOP:   mov     dl,8
EMBLOOP1:  mov     cx,34
           rep     stosw
           add     di,12
           dec     dl
           xchg    ax,bx
           jnz     EMBLOOP1
           mov     dl,192
EMBLOOP2:  mov     es:[di],ax
           add     di,66
           mov     es:[di],ax
           add     di,14
           dec     dl
           xchg    ax,bx
           jnz     EMBLOOP2
           mov     dl,8
EMBLOOP3:  mov     cx,34
           rep     stosw
           add     di,12
           dec     dl
           xchg    ax,bx
           jnz     EMBLOOP3
           and     di,08000H
           add     di,38454
           dec     dh
           jnz     EMBLOOP
           pop     es
           pop     dx
           pop     cx
           pop     bx
           pop     di
           ret

;
; Na EGA i VGA kartici postoji hardverski border.
;

V_BORDER:  push    bx
           mov     al,ah
           lea     bx,BORCOL
           xlat    BORCOL
           mov     ah,al
           mov     al,255
           mov     BRDCOL,ax
           test    ULAMODE,4
           jnz     V_BRD_HCL
           push    dx
           mov     dx,3DAH             ; Read Status
           in      al,dx
           mov     al,8                ; Boja 8 (light black)
           mov     dx,3C0H
           out     dx,al               ; Set color
           mov     al,ah
           out     dx,al
           mov     al,20H
           out     dx,al
           pop     dx
           pop     bx
           ret
V_BRD_HCL: mov     BRDHACK,0           ; Ovo je zbog timinga jedne linije
           cmp     RCLK,(256-96)*256
           cmc
           adc     BRDHACK,0
           pop     bx
           ret

;
; Simulacija ZX Spectrum 128K specificnih periferala (upravljanje memorijom,
; zvucni cip AY-3-8912).
;

           if      VER_128

IN_128:    test    B,128
           jz      MMU
           test    B,64
           jz      IGNOR_128
           call    AY_IN
IGNOR_128: ret

OUT_128:   test    B,128
           jz      MMU
           test    B,64
           jnz     AY_SETREG
           call    AY_OUT
           ret
AY_SETREG: mov     AY_REG,ah
           cmp     ah,15
           jb      AYROK
           mov     AY_REG,15
AYROK:     ret

           else

IN_128:
OUT_128:   ret

           endif

;
; Upravljanje memorijom za ZX Spectrum 128K (port 32765). Labele M386 se
; modifikuju na IAPX 80386 procesoru.
;
; b0,b1,b2 -> Banka RAM-a (0-7) koja se mapira na adrese 49152-65535
; b3       -> 0 = obicni ekran, 1 = alternativni ekran
; b4       -> 0 = ROM 0 aktivan, 1 = ROM 1 aktivan
; b5       -> 48K lock bit (blokira MMU)
;

           if      VER_128

MMU:       test    MMUSTATE,32         ; Ako je LOCK, ignorisi MMU
           jz      NOLOCK
           ret
NOLOCK:    push    ax
           mov     al,ah
           xor     ah,MMUSTATE
           test    ah,16               ; Da li je mijenjan b4?
           jz      NOROMCHG
           push    MEMSEG              ; Zamijeni MEMSEG i ALTMEMSEG
           push    ALTMEMSEG
           pop     MEMSEG
           pop     ALTMEMSEG
           mov     ds,MEMSEG
           mov     es,ALTMEMSEG
NOROMCHG:  test    ah,7                ; Da li su mijenjani b0-b2?
           jz      NORAMCHG
           cld
           push    ds                  ; Razmijeni stranice RAM-a
           push    es
           push    cx
           push    di
           push    si
           mov     di,word ptr MMUSTATE
           and     di,7
           shl     di,2
           les     di,dword ptr PAGES[di]
           mov     si,49152
           mov     cx,MMUWORDS
M386_1:    nop                         ; Ako je 80386 ovo postaje REP MOVSD
           rep     movsw
           mov     si,ax
           and     si,7
           add     si,si
           call    ACTIONS[si]
           add     si,si
           lds     si,dword ptr PAGES[si]
           mov     es,MEMSEG
           mov     di,49152
           mov     cx,MMUWORDS
           cli                         ; Na ovom mjestu timer interapt se ne
           mov     MMUSTATE,al         ;  smije dogoditi ni po koju cijenu
M386_2:    nop
           rep     movsw
           mov     es,ALTMEMSEG
           mov     di,49152
           mov     cx,MMUWORDS
           sub     si,16384
M386_3:    nop
           rep     movsw
           sti
           pop     si
           pop     di
           pop     cx
           pop     es
           pop     ds
NORAMCHG:  mov     MMUSTATE,al
           test    ah,8
           jz      MMUEND
           mov     VIDEOCHG,1          ; Forsiraj azuriranje ekrana
           mov     ATTRCHG,1
           mov     XVIDEOCHG,1
           mov     XATTRCHG,1
MMUEND:    pop     ax
           ret

SNAPMMU:   call    MMU                 ; FAR poziv iz snapshota
           retf

;
; Potprogrami za aktiviranje specijalnih slucajeva za MMU.
;

SETNORM:   mov     MMUVAR,65535
           ret

SETP2:     mov     MMUVAR,32767
           mov     ACT_HL,offset P2_HL
           mov     ACT_AUX,offset P2_AUX
           mov     ACT_STP,offset P2_STP
           ret

SETP5:     mov     MMUVAR,16383
           mov     ACT_HL,offset P5_HL
           mov     ACT_AUX,offset P5_AUX
           mov     ACT_STP,offset P5_STP
           ret

SETP7:     test    al,8
           jnz     SETALTVM
           mov     MMUVAR,65535
           ret
SETALTVM:  mov     MMUVAR,49151
           mov     ACT_HL,offset P7_HL
           mov     ACT_AUX,offset P7_AUX
           mov     ACT_STP,offset P7_STP
           ret

           endif

;
; Potprogram REPAINT iscrtava ponovo ekran visoke rezolucije (npr. nakon F1).
;

REPAINT:   pusha
           mov     ax,LASTB            ; Obnovi BORDER
           and     ah,7
           mov     LASTB,-1
           call    ULAFAR
           push    es
           mov     es,STEKSEG
           cld
           mov     cx,3456             ; "Ubrljaj" bafer
           mov     si,16384
           lea     di,SCRBUF
           push    ds
           if      VER_128
           mov     ah,MMUSTATE
           test    ah,8
           jz      REPLOOP
           and     ah,7
           cmp     ah,7
           jne     REPSHADOW
           mov     ax,ZXDSEG+2048
           mov     ds,ax
           jmp     short REPLOOP
REPSHADOW: mov     ax,ZXEXTRA2+2048
           mov     ds,ax
           endif
REPLOOP:   lodsw
           not     ax
           stosw
           loop    REPLOOP
           mov     cx,6145             ; Pozovi azuriranje
           mov     si,16384
           lea     di,SCRBUF
           call    UPDT_VID
           dec     si
           dec     di
           mov     cx,769
           call    UPDT_ATTR
           pop     ds
           pop     es
           popa
           ret

;
; Rutine SETVEC i RESTVEC preusmjeravaju odnosno obnavljaju INT vektore. Takodje
; se obnavlja i DOS sistemski sat prema satu realnog vremena.
;

SETVEC:    pushf
           cli
           call    SETTIMER
           call    SETKBD
           push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ds:[52],offset INT_0D
           mov     ds:[54],cs
           mov     ds:[108],offset INT_1B
           mov     ds:[110],cs
           mov     ds:[140],offset INT_1B
           mov     ds:[142],cs
           mov     ds:[144],offset INT_24
           mov     ds:[146],cs
           mov     ds:[1000],offset INT_FA
           mov     ds:[1002],cs
           pop     ds
           popf
           ret

CONVBCD    macro   REG                 ; Konvertuje BCD format u binarni
           mov     al,REG
           xor     ah,ah
           shl     ax,4
           rol     al,4
           aad
           mov     REG,al
           endm

RESTVEC:   call    RESTTIMER
           call    RESTKBD
           push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ax,word ptr LAST_01
           mov     ds:[4],ax
           mov     ax,word ptr LAST_01+2
           mov     ds:[6],ax
           mov     ax,word ptr LAST_0D
           mov     ds:[52],ax
           mov     ax,word ptr LAST_0D+2
           mov     ds:[54],ax
           mov     ax,word ptr LAST_1B
           mov     ds:[108],ax
           mov     ax,word ptr LAST_1B+2
           mov     ds:[110],ax
           mov     ax,word ptr LAST_FA
           mov     ds:[1000],ax
           mov     ax,word ptr LAST_FA+2
           mov     ds:[1002],ax
           mov     ah,02H              ; Cita sat realnog vremena
           int     1AH
           CONVBCD ch
           CONVBCD cl
           CONVBCD dh
           mov     ah,2DH              ; Postavlja sistemski sat
           int     21H
           pop     ds
           ret

;
; Rutine SETKBD i RESTKBD postavljaju odnosno obnavljaju interrupt tastature.
; SETKBD takodje brise bitmapu tastera.
;

SETKBD:    pushf
           cli
           push    es
           push    di
           xor     ax,ax
           mov     es,ax
           mov     es:[36],offset INT_9
           mov     es:[38],cs
           mov     cx,8
           lea     di,KBDROWS
SETLOOP:   mov     byte ptr ss:[di],0FFH
           inc     di
           loop    SETLOOP
           mov     al,20H
           out     20H,al
           pop     di
           pop     es
           popf
           ret

RESTKBD:   cli
           push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ax,word ptr LAST_09
           mov     ds:[36],ax
           mov     ax,word ptr LAST_09+2
           mov     ds:[38],ax
           mov     al,20H
           out     20H,al
           pop     ds
           sti
           ret

;
; Pauza (F1).
;

PAUSE:     pusha
           if      VER_128
           call    STOPADLIB
           endif
           call    KLIK
           call    RESTTIMER
           call    RESTKBD
           sti
HSL3:      mov     ah,1                ; Cekaj pritisak tipke
           int     16H
           pushf
           xor     ah,ah
           int     16H
           popf
           jnz     HSL3
           call    SETKBD
           call    SETTIMER
           call    KLIK
           if      VER_128
           call    INITADLIB
           endif
           popa
           ret

;
; Izlaz iz programa (F10).
;

EXIT:      lea     dx,ESCAPED
           jmp     ILL1

;
; Rezim atributa na Herculesu, CGA i EGAMono (F5).
;

HSETUP:    pusha
           mov     ax,30
           mov     FL_CNT,ax
           test    ATTRMODE,128
           jnz     HNOKLIK
           call    KLIK
HNOKLIK:   inc     ATTRMODE
           mov     ah,ATTRMODE
           and     ah,127
           cmp     ah,1
           jne     NOMONO
           call    dword ptr F5INITM
           xor     ax,ax
           call    F5BORDER
           jmp     short ENDHSET
NOMONO:    cmp     ah,2
           jne     NOWHITE
           call    INVERTH
           mov     ax,0700H
           call    F5BORDER
           jmp     short ENDHSET
NOWHITE:   mov     ATTRMODE,0
           call    dword ptr F5INIT
ENDHSET:   call    REPAINT
           popa
           ret

;
; Promjena palete u Windowed rezimu (F5).
;

WSETUP:    pusha
           pushf
           push    ds
           push    es
           test    ATTRMODE,128
           jnz     WNOKLIK
           call    KLIK
WNOKLIK:   inc     ATTRMODE
           push    IFF
           mov     IFF,0
           mov     BOJEPTR,offset SSEG1:WINBOJE1
           mov     ah,0BH
           xor     bx,bx               ; Crna pozadina
           mov     ch,ATTRMODE
           and     ch,127
           cmp     ch,1
           je      PAL1
           cmp     ch,2
           jne     PAL0
           inc     bx                  ; Plava pozadina
PAL1:      int     10H
           mov     bx,256              ; Paleta Green/Red/Brown
           jmp     short ENDWSET
PAL0:      mov     BOJEPTR,offset SSEG1:WINBOJE2
           mov     ATTRMODE,0
           int     10H
           mov     bx,257              ; Paleta Magenta/Cyan/White
ENDWSET:   mov     ah,0BH
           int     10H
           call    far ptr INITWIN1
           call    REPAINT
           pop     IFF
           pop     es
           pop     ds
           popf
           popa
           ret

;
; Aktiviranje i deaktiviranje zvuka (F4).
;

SETSOUND:  pusha
           call    KLIK
           call    SETSNDFAR
           popa
           ret

;
; Aktiviranje i deaktiviranje bordera (F6).
;

SETBORDR:  pusha
           call    KLIK
           call    SETBRDFAR
           call    REPAINT
           popa
           ret

;
; Potprogram COUNTERS poziva se iz vremenskog interapta. On azurira neke brojace
; i poduzima odgovarajuce akcije ovisno od njihove vrijednosti.
;

COUNTERS:  dec     FL_CNT              ; Izvrsi FLASH kada dodje vrijeme
           jnz     NOFLASH
           call    FLASH
NOFLASH:   ret

;
; Rutina za postizanje Real-time rezima.
;

           align   4
SLOW_MAIN: push    ax
           push    cx
           sub     RCLK,224*256
           and     R,127
SLOW_LP:   mov     cx,LASTTIMER
           xor     al,al
           out     43H,al
           in      al,40H
           mov     ah,al
           in      al,40H
           xchg    ah,al
           sub     cx,ax
           jnc     NOADJUST
           add     cx,TTIMER
NOADJUST:  sub     cx,SPEED
           jb      SLOW_LP
           add     ax,cx
           mov     LASTTIMER,ax
           pop     cx
           test    ULAMODE,4
           jz      NOHCRBRD
           call    HICOLBRD            ; Pozovi HiCol BORDER ako radi
NOHCRBRD:  inc     VIDEOFLG
           mov     ax,VIDSYNC
           cmp     VIDEOFLG,ax
           je      DO_VID_L
           cmp     VIDEOFLG,312
           je      DO_IRQ
           pop     ax
           ret
DO_VID_L:  jmp     DO_VIDEO

;
; Analogna rutina za postizanje maksimalne raspolozive brzine. Ona se kopira
; preko rutine SLOW_MAIN.
;

DO_VID_PTR dw      DO_VIDEO
DO_IRQ_PTR dw      DO_IRQ

MAXSPEED:  push    ax
           sub     RCLK,224*256
           and     R,127
           inc     VIDEOFLG
           mov     ax,VIDEOFLG
           cmp     ax,VIDSYNC
           je      DO_VID_F
           cmp     ax,312
           je      DO_IRQ_F
           pop     ax
           ret
                                       ; Ove grdobe su neophodne da se forsira
DO_VID_F:  jmp     DO_VID_PTR          ;  apsolutno adresiranje, jer nakon
DO_IRQ_F:  jmp     DO_IRQ_PTR          ;  kopiranja ove rutine preko SLOW_MAIN
                                       ;  relativno adresiranje nece raditi!
MAXSPDEND  label   byte

;
; Simulacija Spectrumovog IRQ. Ovdje se izvrsavaju i funkcijske tipke. Ako je
; dozvoljen Z80 prekid, aktivira se SINGLE-STEP da se pronadje kraj
; instrukcije.
;

DO_IRQ:    mov     VIDEOFLG,0
           mov     ax,VIDEOCHG         ; Ovo su neki hackovi za HiRes BORDER
           mov     VIDEOHCK,ax
           mov     ax,ATTRCHG
           mov     ATTRHCK,ax
           cmp     FKVEC,0
           jne     DO_SSTEP
           test    IFF1,4              ; Ako je DI, nema interapta
           jz      NOINT
DO_SSTEP:  push    es                  ; Postavi vektor za INT 1
           xor     ax,ax
           mov     es,ax
           mov     es:[4],offset SSTRAP
           mov     es:[6],cs
           pop     es
           or      FLAGSH,1
           or      ALTF,256
           pushf
           pop     ax                  ; Aktivira Single-Step
           or      ax,256
           push    ax
           popf
NOINT:     pop     ax                  ; Oprez: stek!
           ret

;
; Rutina za azuriranje slike.
;

DO_VIDEO:  pusha                       ; Azuriranje sadrzaja ekrana
           push    es
           push    ds
           cld
           if      VER_128
           test    MMUSTATE,8
           jnz     DO_XVIDEO
           endif
           mov     ds,MEMSEG
           cmp     ATTRCHG,0
           je      UPD_CONT
           mov     cx,769
           mov     si,22528
           lea     di,SCRBUF[6144]
           call    UPDT_ATTR
UPD_CONT:  cmp     VIDEOCHG,0
           je      UPD_END
           mov     cx,6145
           mov     si,16384
           lea     di,SCRBUF
           call    UPDT_VID
UPD_END:   pop     ds
           pop     es
           popa
           mov     VIDEOCHG,0
           mov     ATTRCHG,0
           pop     ax                  ; Oprez: stek!
           ret

           if      VER_128

DO_XVIDEO: mov     cx,ZXDSEG+2048      ; Azuriranje alternativnog ekrana
           mov     ah,MMUSTATE
           and     ah,7
           cmp     ah,7
           je      DO_NOSHAD
           mov     cx,ZXEXTRA2+2048
DO_NOSHAD: mov     ds,cx
           cmp     XATTRCHG,0
           je      UPD_XCONT
           mov     cx,769
           mov     si,22528
           lea     di,SCRBUF[6144]
           call    UPDT_ATTR
UPD_XCONT: cmp     VIDEOCHG,0
           je      UPD_XEND
           mov     cx,6145
           mov     si,16384
           lea     di,SCRBUF
           call    UPDT_VID
UPD_XEND:  pop     ds
           pop     es
           popa
           mov     XVIDEOCHG,0
           mov     XATTRCHG,0
           pop     ax                  ; Oprez: stek!
           ret

           endif

;
; Implementacija visokorezolucijskih BORDER efekata na VGA kartici.
;

           align   4
HICOLBRD:  push    bx
           mov     bx,VIDEOFLG
           add     bx,BRDHACK
           mov     ax,VIDEOCHG         ; Ovaj ovdje dio pokusava da izvrsi
           sub     ax,VIDEOHCK         ;  kompenzaciju timinga za slucaj kada
           add     ax,ATTRCHG          ;  se smjesta nesto u Low RAM (sve je
           sub     ax,ATTRHCK          ;  zasnovano heuristicki, i prilagodjeno
           jz      HACKEND             ;  Aquaplane-u)
           push    ax
           mov     ax,VIDEOCHG
           mov     VIDEOHCK,ax
           mov     ax,ATTRCHG
           mov     ATTRHCK,ax
           pop     ax
           cmp     bx,60               ; Po mojoj logici trebalo bi 63
           jb      hackend
           cmp     bx,256
           jae     hackend
           shl     al,3
           add     byte ptr RCLK+1,al
HACKEND:   cmp     bx,60               ; Testiraj limit ekrana
           jb      VGBEND
           cmp     bx,260
           ja      VGBEND
           push    di
           push    es
           mov     es,VIDEOSEG
           mov     ax,BRDCOL
           mov     di,bx
           sub     di,60
           imul    di,di,80
           cld
           cmp     bx,64
           jb      WHOLEBORD
           cmp     bx,256
           jae     WHOLEBORD
           stosw
           stosw
           stosw
           stosw
           add     di,64
           stosw
           stosw
           stosw
           stosw
           add     di,32768-80
           stosw
           stosw
           stosw
           stosw
           add     di,64
           stosw
           stosw
           stosw
           stosw
           pop     es
           pop     di
           pop     bx
           ret
WHOLEBORD: push    cx
           mov     cx,40
           rep     stosw
           add     di,32768-80
           mov     cx,40
           rep     stosw
           pop     cx
           pop     es
           pop     di
VGBEND:    pop     bx
           ret

;
; Obrada vremenskog interrupta. Azuriraju se neki brojaci, i simulira se AY
; zvuk i FLASH.
;

INT_8:     push    ax
           if      VER_128
           call    SNDINT              ; Na 128 verziji simuliraj AY
           endif
           mov     al,20H              ; EOI je potreban prije COUNTERS
           out     20H,al
           call    COUNTERS
           pop ax
           iret

;
; Single-step interrupt. Trazi kraj instrukcije, a kada je pronadje, izvrsava
; Spectrumov interrupt ili funkcijsku tipku.
;

           even
SSTRAP:    push    bp                  ; Trazi JMP SS:TABLE1[BX]
           mov     bp,sp
           mov     bp,[bp+2]
           cmp     word ptr cs:[bp+1],0A7FFH
           je      SSCONT
SSFAIL:    pop     bp
           iret

           even
SSCONT:    cmp     byte ptr cs:[bp],36H
           jne     SSFAIL
           cmp     word ptr cs:[bp+3],offset SSEG:TABLE1
           jne     SSFAIL
           pop     bp
           and     FLAGSH,0FEH         ; Nadjen je kraj instrukcije
           and     ALTF,0FEFFH
           cmp     FKVEC,0             ; Da li je funkcijska tipka?
           je      SSNOFK
           call    FKVEC               ; Izvrsi funkcijsku tipku
           mov     FKVEC,0
           jmp     PROCINT
SSNOFK:    mov     IFF,0
           add     PC,word ptr HSTATE
           mov     HSTATE,0
           dec     STP
           mov     AUX,PC
           WRITESP AUX_H
           dec     STP
           WRITESP AUX_L
           cmp     IMODE,2
           je      IMODE2
           mov     PC,56               ; Simulacija IM 0 ili IM 1
           add     RCLK,12*256
           jmp     short PROCINT
IMODE2:    push    ax
           mov     ah,I                ; Simulacija IM 2
           mov     al,255
           mov     PC,ax
           mov     al,[PC]             ; Ova zaobilazenja u velikom broju
           inc     PC                  ; slucajeva sprecavaju ruzne interakcije
           mov     ah,[PC]             ; sa virtualnim drajverima (EMM386).
           mov     PC,ax
           add     RCLK,19*256
           pop     ax
PROCINT:   mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           sti
           retf    2                   ; Vrati se sa ugasenim T flegom

;
; Obrada interrupta tastature. Neke tipke (npr. strelice) reaguju kao pritisak
; na dvije tipke istovremeno (npr. CAPS SHIFT + 5).
;

STORESS    dw      0                   ; Spremnik za SS
STORESP    dw      0                   ; Spremnik za SP
CSSTEK     dw      SSEG                ; Pointer na SSEG adresibilan preko CS

INT_9:     mov     STORESS,ss          ; Ovo je sve zbog problema koji nastupe
           mov     STORESP,sp          ; ako INT_9 nastupi u DOS rutini zato sto
           mov     ss,CSSTEK           ; DOS mijenja SS !
           lea     sp,INT_STACK
           push    ax
           in      al,60H              ; Ocitaj scan kod
           push    ax
           in      al,61H              ; Potvrda kontroleru tastature
           push    ax
           or      al,128
           out     61H,al
           pop     ax
           out     61H,al
           pop     ax
           cmp     al,224              ; Da li je prosirena tipka
           jne     I9_NORMAL
           mov     EXTHFLG,1           ; Signal "prosirena tipka"
           jmp     short END_I9
I9_NORMAL: call    I9_MAIN
           mov     EXTHFLG,0
END_I9:    mov     al,20H              ; EOI
           out     20H,al
           pop     ax
           mov     ss,STORESS          ; Obnovi stek
           mov     sp,STORESP
           iret

I9_MAIN:   push    bx                  ; Ova rutina se rekurzivno poziva dva
           push    cx                  ; puta za slucaj dvojnih tipki
           mov     ah,al
           and     al,7FH
I9_ROW:    mov     cl,al
           lea     bx,HALFROW          ; Odredi polured iz tablice
           xlat    HALFROW
           cmp     al,200
           jb      I9_DOUBL
           jmp     short KBDJOY
I9_DOUBL:  cmp     al,10               ; 10 ili vise znaci dvojnu tipku
           jae     DBLKEY
           cmp     al,8                ; 8 nije nista, a 9 je funkcijska tipka
           jae     END_9
           mov     ch,al
           xchg    al,cl
           lea     bx,INBYTE           ; Odredi masku tastera iz tablice
           xlat    INBYTE
           xchg    cl,al
           mov     bl,ch
           xor     bh,bh
           cmp     ah,80H              ; Da li je interrupt nastupio zbog
           jbe     PRESS               ; pritiska ili otpustanja tastera ?
           not     cl
           or      KBDROWS[bx],cl      ; Signaliziraj otpustanje
           jmp     short END_9
DBLKEY:    push    si                  ; Ako je dvojna tipka, ocitaj dvije
           mov     si,ax               ; zamjenske tipke iz tablice, i pozovi
           and     si,00FFH            ; 2 puta istu ovu rutinu za ta 2 tastera
           add     si,si
           mov     cl,ah
           and     cl,80H
           push    ax
           mov     al,DBLKEYS[si-20]
           or      al,cl
           call    I9_MAIN
           mov     al,DBLKEYS[si-19]
           or      al,cl
           call    I9_MAIN
           pop     ax
           pop     si
           jmp     short END_9
PRESS:     and     KBDROWS[bx],cl      ; Signaliziraj pritisak
END_9:     cmp     al,9                ; 9 znaci funkcijska tipka
           jne     NOFKEYS
           cmp     INHIBIT,0FFH        ; Ako treba, ignorisi funkcijske tipke
           je      NOFKEYS
           cmp     ah,80H              ; Otpustanje funkcijskih tipki ignorisi
           jae     NOFKEYS
           mov     bl,ah
           xor     bh,bh
           add     bx,bx
           mov     bx,FKADDR[bx-118]
           mov     FKVEC,bx
NOFKEYS:   pop     cx
           pop     bx
           ret

KBDJOY:    cmp     JTYPE,0
           jne     KBDKEMP
           sub     al,188              ; Obicni tretman kurzora
           jmp     I9_DOUBL
KBDKEMP:   cmp     JTYPE,1
           jne     KBDOTH
           sub     al,200              ; Kempston kurzorima
           lea     bx,KBDJOYTAB
           xlat    KBDJOYTAB
           cmp     ah,80H
           jbe     KJOYPRESS
           not     al
           and     KJOYVAL,al          ; Otpusten Kempston taster
           xor     al,al
           jmp     END_9
KJOYPRESS: or      KJOYVAL,al          ; Pritisnut Kempston taster
           xor     al,al
           jmp     END_9
KBDOTH:    cmp     JTYPE,2
           jne     KBDOTH1
           cmp     EXTHFLG,1           ; Za kurzor joystick sivi kurzori su sa
           jne     KBDOTH1             ; shift tipkom
           and     KBDROWS,254
           cmp     ah,80H
           jb      KBDOTH1
           or      KBDROWS,1
KBDOTH1:   lea     bx,JOYRPLC-2        ; 2=kurzor, 7=sinclair1, 12=sinclair2
           add     bl,JTYPE
           adc     bh,0
           sub     al,200
           xlat    JOYRPLC
           jmp     I9_ROW

;
; Interrupt greske prekoracenja segmenta. Javlja se u slucaju instrukcija poput
; LD HL,(65535) ili POP HL ili RET ako je SP=1. Ovaj handler omogucava ispravno
; izvrsavanje i ovakvih specijalnih slucajeva.
;

INT_0D:    push    bp                  ; Normalni INT_0D handler
           mov     bp,sp
           push    es
           push    di
           mov     di,[bp+2]
           push    ax
           mov     ax,cs:[di]          ; Pribavi spornu instrukciju
           lea     di,INSTRS
           push    cx
           mov     es,STEKSEG          ; Ispitaj spornu instrukciju
           mov     cx,15
           cld
           repne   scasw               
           jne     UNEXPECT            ; Izlaz ako je nema u tabeli
           pop     cx
           sub     di,offset SSEG:INSTRS+2
           mov     ax,INSTRLEN[di]
           add     [bp+2],ax           ; Azuriraj povratnu adresu
           mov     al,ds:[65535]
           mov     ah,ds:[0]
           call    INSTRADDR[di]       ; Obavi spornu instrukciju
           pop     ax
           pop     di
           pop     es
           pop     bp
           iret

GETBX:     mov     bx,ax               ; Uzimanje BX sa 65535
           ret

GETCX:     mov     cx,ax               ; Uzimanje CX sa 65535
           ret

GETDX:     mov     dx,ax               ; Uzimanje DX sa 65535
           ret

GETSI:     mov     si,ax               ; Uzimanje SI sa 65535
           ret

GETDI:     mov     [bp-4],ax           ; Uzimanje DI sa 65535
           ret

GETBP:     mov     [bp],ax             ; Uzimanje BP sa 65535
           ret

SASTP:     push    ax                  ; Uzimanje adresirano preko BP
           push    cx
           mov     di,[bp+2]
           mov     ax,cs:[di-2]
           lea     di,SASTVAL
           mov     cx,6
           repne   scasw
           jne     UNEXPECT
           pop     cx
           pop     ax
           sub     di,offset SSEG:SASTVAL+2
           jmp     SASTADDR[di]

UNEXPECT:  pop     cx                  ; Nije prepoznata sporna instrukcija
           pop     ax                  ; (ovo se ne bi trebalo desavati u
           pop     di                  ; normalnom radu)
           pop     es
           pop     bp
           lea     dx,UNEXPMSG
           jmp     ILL1

;
; DOS interrupt kriticne greske (INT 24H).
;

INT_24:    mov     al,3

;
; Lazni CTRL-BREAK interrupt (INT 1BH za BIOS i INT 23H za DOS).
;

INT_1B:    iret                        ; isto je i INT 23H

;
; Interrupt 250 za povratak u Z80 rezim nakon GO8086.
;

INT_FA:    pop     PC
           pop     AUX
           popf
           mov     ds,MEMSEG
           if      VER_128
           mov     es,ALTMEMSEG
           endif
           mov     ss,STEKSEG
           lea     sp,FLAGS
           cmp     AUX,MEMSEG
           jne     BACKERR
           call    SETTIMER
           popf
           pushf
           jmp     GETINST
BACKERR:   call    TEKST
           call    RESTVEC
           mov     ds,STEKSEG
           lea     dx,BACKERROR
           mov     ah,09H
           int     21H
           jmp     ILLEND

;
; Aktiviranje i deaktiviranje grafike na Herculesu, i na Extended Herculesu.
; Rutine pretpostavljaju da je ES na 0B000H.
;

H_GRAPH:   lea     si,GRAPHCON
           jmp     short HG1

X_GRAPH:   lea     si,EXTCON
HG1:       push    si
           mov     dx,03BFH
           mov     al,3
           out     dx,al
           mov     cx,65535
           xor     di,di
           xor     al,al
           cld
           rep     stosb
           mov     dx,03B8H
           mov     al,2
           out     dx,al
           xor     bx,bx
           mov     cx,12
           mov     dx,03B4H
           pop     si
GRAPHL:    mov     al,bl
           out     dx,al
           inc     dx
           mov     al,ss:[bx+si]
           out     dx,al
           dec     dx
           inc     bx
           loop    GRAPHL
           mov     dx,03B8H
           mov     al,10
           out     dx,al
           mov     FL_CREG,al
           ret

H_TEKST:   mov     ax,7
           int     10H
           ret

;
; Aktiviranje i deaktiviranje grafike na CGA.
;

C_GRAPH:   mov     ax,6
           int     10H
           ret

C_TEKST:   mov     ax,3
           int     10H
           ret

;
; Aktiviranje Windows rezima za CGA.
;

W_GRAPH:   mov     ax,4
           int     10H
           mov     ah,0BH
           xor     bx,bx               ; Boja pozadine (crna)
           int     10H
           mov     ah,0BH
           mov     bx,257              ; Paleta Magenta/Cyan/White
           int     10H
           cmp     FKADDR[8],offset DUMMY
           lea     ax,WSETUP
           je      WGEND
           mov     FKADDR[8],ax        ; Preusmjeri tipku F5, ako nije zabrana
WGEND:     ret

;
; Aktiviranje grafike na EGAMONO (deaktiviranje je kao za Hercules).
;

EM_GRAPH:  mov     ax,15
           int     10H
           ret

;
; Aktiviranje i deaktiviranje grafike na EGA i VGA.
;

V_GRAPH:   mov     ax,1200H
           mov     bl,30H
           int     10H
           mov     al,1
           sub     al,MXSPD
           mov     VGAOK,al            ; Odredi da li je dozvoljen HiCol border
           stc
           pushf
           jmp     short VE_CONT

E_GRAPH:   clc
           pushf
VE_CONT:   lea     ax,DUMMY
           mov     FKADDR[8],ax        ; Taster F5 iskljuciti na EGA i VGA
           mov     ax,1
           int     10H
           mov     ax,1100H
           mov     es,STEKSEG
           lea     bp,MASKA
           mov     cx,256
           xor     dx,dx
           mov     bx,512
           int     10H
           mov     dx,03D4H
           mov     al,9
           out     dx,al
           inc     dx
           xor     al,al
           popf
           pushf
           rcr     al,1                ; ??? Zasto je bilo adc al,0 ???
           out     dx,al
           mov     dx,03DAH
           in      al,dx
           mov     dx,03C0H
           mov     al,10H
           out     dx,al
           xor     al,al
           out     dx,al
           mov     al,20H
           out     dx,al
           mov     dx,03CEH
           mov     al,6
           out     dx,al
           inc     dx
           mov     al,2
           out     dx,al
           mov     dx,3DAH             ; Read Status
           in      al,dx
           popf
           jnc     NO_SETYEL
           mov     bx,6                ; Postavi zutu boju da bude ispravna
           mov     dx,39*256+0         ;  (samo za VGA: DH=R, CH=G, CL=DL=B)
           mov     cx,dx
           mov     ax,1010H
           int     10H
NO_SETYEL: mov     es,VIDEOSEG
           mov     cx,32768
           xor     di,di
           mov     ax,08FFH
           cld
           rep     stosw
           ret

V_TEKST:   mov     ax,1202H
           mov     bl,30H
           int     10H
E_TEKST:   mov     ax,3
           int     10H
           ret
;
; Automatsko odredjivanje kartice i odgovarajuca inicijalizacija
;

AUTODET:   push    es
           call    DETECT
           pop     es
           mov     si,ax
           add     si,si
           mov     si,CARDS[si]
           mov     CARDPTR,si
           call    dword ptr CARDPTR
           ret

DETECT:    cmp     NAMJERNA,0          ; Ovim se preskace detekcija
           je      DETCONT
           cmp     NAMJERNA,8
           je      DETCONT
           mov     al,NAMJERNA
           xor     ah,ah
           ret
DETCONT:   call    DETECTCRD           ; Far poziv
           xor     ah,ah
           cmp     al,7
           je      DETHERC
           ret
DETHERC:   cmp     NAMJERNA,8
           je      EXTHERC
           ret
EXTHERC:   inc     al
           ret

;
; Nije nadjena graficka kartica.
;

BADCARD:   lea     dx,BCMESS
           mov     ds,STEKSEG
           mov     ah,09H
           int     21H
           jmp     short ILLEND

;
; Inverzija slike na Herculesu, i srodnim.
;

INVERTH:   xor     si,si
           mov     cx,256
INVLOOP:   not     MASKA[si]
           add     si,2
           loop    INVLOOP
           ret

;
; Ispis poruke o gresci.
;

ILL1:      push    0
           popf
           mov     al,20H
           out     20H,al
           push    dx
           call    RESTVEC
           call    TEKST
           pop     dx
ILL2:      mov     ds,STEKSEG
           mov     ah,9
           int     21H
ILLEND:    call    CLRKBD
           mov     dl,13               ; Ispisuje CR LF
           mov     ah,2
           int     21H
           mov     dl,10
           int     21H
           in      al,61H              ; Gasi zvuk
           and     al,0FCH
           out     61H,al
           if      VER_128
           call    STOPADLIB
           endif
           mov     ax,4C00H            ; Kraj programa
           int     21H

CLRKBD:    mov     ah,1                ; Prazni bafer tastature
           int     16H
           jz      CLRKEND
           xor     ah,ah
           int     16H
           jmp     CLRKBD
CLRKEND:   ret

;
; *** ULAZNA TACKA U PROGRAM ***
;

START:     cli                         ; Inicijaliziraj varijable
           mov     ax,SSEG
           mov     ss,ax
           lea     sp,FLAGS
           mov     es,ax
           cld
           lea     di,TEMP_STACK
           lea     cx,ENDSS
           sub     cx,di
           xor     ax,ax
           rep     stosb
           mov     es,MEMSEG           ; Obrisi memoriju
           mov     di,16384
           mov     cx,49152
           rep     stosb
           if      VER_128
           mov     es,ALTMEMSEG
           mov     di,16384
           mov     cx,49152
           rep     stosb
           mov     cx,ZXEXTRA1
           mov     es,cx
           xor     di,di
           mov     cx,32768
           rep     stosb
           mov     cx,ZXEXTRA2
           mov     es,cx
           xor     di,di
           mov     cx,65535
           rep     stosb
           endif
           sti
           mov     ax,sp               ; Provjeri procesor
           pusha
           even
           lea     dx,CPUERR
           cmp     ax,sp
           jne     CPUOK
           jmp     PSPER1
CPUOK:     mov     sp,ax
           mov     bx,7000H            ; Na 386 se nesto modifikuje
           push    bx
           popf
           pushf
           pop     bx
           and     bx,7000H
           jz      CPU286
           if      VER_128
           mov     MMUWORDS,4096
           mov     word ptr M386_1,66F3H
           mov     word ptr M386_2,66F3H
           mov     word ptr M386_3,66F3H
           endif
           .286p
           smsw    ax
           .286
           test    ax,1
           jz      CPU286
           mov     VIRTMODE,1
CPU286:    mov     FLAGS,512
           mov     ALTF,512
           push    ds
           call    EXPLODE
           mov     ds,STEKSEG
           lea     bx,PARAMSBUF
           call    PSPLOOP1
           pop     ds
           mov     PSPOPTS[0],255      ; Sprijeci neke komandne parametre
           mov     PSPOPTS[3],255
           mov     PSPOPTS[5],255
           mov     PSPOPTS[7],255
           mov     PSPOPTS[8],255
           call    PSPSCAN             ; Skaniraj komandne parametre
           mov     ds,STEKSEG

           assume  ds:SSEG

           cmp     VIRTMODE,1          ; Da li je virtuelni mod
           jne     NOVIRTM
           cmp     VIRTWARN,1
           je      NOVIRTM
           lea     dx,VIRTMSG          ; Ispisi upozorenje
           mov     ah,09H
           int     21H
           call    CLRKBD
           mov     ah,08H
           int     21H
NOVIRTM:   mov     cx,10               ; Parametri misa
           mov     dx,10
           mov     ax,15
           int     33H
           mov     ax,3501H            ; Sacuvaj vektore
           int     21H
           mov     word ptr LAST_01,bx
           mov     word ptr LAST_01+2,es
           mov     ax,3508H
           int     21H
           mov     word ptr LAST_08,bx
           mov     word ptr LAST_08+2,es
           mov     ax,3509H
           int     21H
           mov     word ptr LAST_09,bx
           mov     word ptr LAST_09+2,es
           mov     ax,350DH
           int     21H
           mov     word ptr LAST_0D,bx
           mov     word ptr LAST_0D+2,es
           mov     ax,351BH
           int     21H
           mov     word ptr LAST_1B,bx
           mov     word ptr LAST_1B+2,es
           mov     ax,35FAH
           int     21H
           mov     word ptr LAST_FA,bx
           mov     word ptr LAST_FA+2,es
           mov     ds,MEMSEG

           assume  ds:ZXDSEG

           call    AUTODET
           mov     es,VIDEOSEG
           call    GRAPH
           cmp     VGAOK,1
           je      HCBALLOW
           test    ULAMODE,4
           jz      HCBALLOW
           and     ULAMODE,255-4
           call    SETBRDFAR
HCBALLOW:  test    ATTRMODE,128        ; Izvrsiti opciju /M
           jz      NOSETATM
           xor     ch,ch
           mov     cl,ATTRMODE
           sub     cl,127
           mov     ATTRMODE,128
SAMLP:     call    FKADDR[8]
           loop    SAMLP
           and     ATTRMODE,127
NOSETATM:  mov     HSTATE,0
           mov     ds,MEMSEG
           if      VER_128
           mov     es,ALTMEMSEG
           endif
           call    LOADSNAP
           cmp     SNAPERR,0
           je      SETREGS
ERRINSNA:  call    RESTVEC
           call    TEKST
           lea     dx,SNAPERMSG
           jmp     ILL2
SETREGS:   call    RECOVSNAP
           pusha
           push    es
           if      VER_128             ; Na 128 verziji treba popuniti i
           mov     si,16384            ; paralelnu stranicu memorije
           mov     di,si
           mov     cx,24576
           rep     movsw
           endif
           cli
           call    SETVEC
           mov     es,SNAPSEG

           assume  es:ZXEXPBUF

           if      VER_128
           cmp     SNAPHWMOD,4
           else
           cmp     SNAPHWMOD,1
           endif
           jne     CHECKSTA
           mov     SNAPERR,1
           jmp     ERRINSNA
CHECKSTA:  mov     ah,SNAPSTTS
           shr     ah,1
           and     ax,0700H
           mov     LASTB,ax
           call    ADJUSTTBL
           if      VER_128             ; Obnova 128 stanja
           xor     bx,bx
AYRESTLP:  mov     ah,SNAPSOREG[bx]
           mov     AY_REG,bl
           push    bx
           call    AY_OUT
           pop     bx
           inc     bx
           cmp     bl,15
           jne     AYRESTLP
           mov     ah,SNAP7FFD
           mov     al,SNAPFFFD
           mov     AY_REG,al
           endif
           pop     es

           assume  es:nothing

           if      VER_128
           mov     es,ALTMEMSEG
           call    MMU
           endif
           call    REPAINT
           popa
           sti
           jmp     GETINST             ; Skace na FETCH

;
; Analiziranje parametara komandne linije. Parametri su:
;
; /!              Ne ispisuje poruku upozorenja za 'virtual 8086' mod
; /%<num>         Emulacija na odredjenoj brzini
; /%MAX           Emulacija na maksimalnoj dopustivoj brzini
; /JA             Simulacija Kempston joysticka analognim joystickom
; /JB             Simulacija Sinclair joysticka B kurzorima
; /JC             Simulacija cursor joysticka kurzorima
; /JK             Simulacija Kempston joysticka kurzorima
; /JM             Simulacija Kempston joysticka misem
; /JS             Simulacija Sinclair joysticka A kurzorima
; /M<mode>        Izbor nacina za simulaciju atributa
; /Q<sync>        Postavljanje video sinhronizacije na Scan liniju 0-311
; /S<mask>        Maska kanala za AY chip (128)
; /SA             AY emulacija preko Adlib kartice
; /U<status>      Izbor statusa za ULA chip
; /VC             Forsiranje CGA kartice
; /VE             Forsiranje EGA kartice
; /VH             Forsiranje Hercules kartice
; /VM             Forsiranje EGAMONO kartice
; /VV             Forsiranje VGA kartice
; /VX             Aktiviranje Extended Hercules rezima
; /W<cycles>      Usporavanje primjenom prazne petlje <cycles> puta
; /Z<mask>        Zabranjuje neke funkcijske tipke
; /€<filename>    Za internu upotrebu
; /              Za internu upotrebu
;
; Opcije /V, /%, /J, /S i /Q su dozvoljene i nakon prevodjenja, dok se ostale
; opcije moraju zadati prije prevodjenja.
;

PSPSCAN:   mov     bx,80H
PSPLOOP:   inc     bx
PSPLOOP1:  mov     al,[bx]
           cmp     al,13
           jnz     PSPCONT
           ret
PSPCONT:   cmp     al,' '
           jz      PSPLOOP
           cmp     al,'/'
           jumps
           jne     PSPERROR
           inc     bx
           mov     al,[bx]
           or      al,32
           xor     si,si
COMLOOP:   cmp     PSPOPTS[si],0
           je      PSPERROR
           nojumps
           cmp     al,PSPOPTS[si]
           je      DOCOMM
           inc     si
           jmp     COMLOOP
DOCOMM:    add     si,si
           jmp     PSPCOM[si]
PSPC1:     inc     bx
PSPC2:     mov     al,[bx]
           cmp     al,13
           je      PSPLOOP1
           cmp     al,' '
           je      PSPLOOP1
           jmp     PSPERROR

NOWARNING: mov     VIRTWARN,1          ; Bez upozorenja za 'virtual' mod
           jmp     PSPC1

SETSPEED:  mov     ax,[bx+1]
           or      ax,2020H
           cmp     ax,'am'
           je      short MAYBEMAX
           call    GETPSPNUM           ; Postavljanje brzine rada
           or      dx,dx
           jumps
           je      PSPERROR
           mov     cx,dx
           mov     ax,7636
           xor     dx,dx
           div     cx
           cmp     dx,3818
           cmc
           adc     ax,0
           inc     ax
           add     ax,ax
           mov     SPEED,ax
           jmp     PSPC1
MAYBEMAX:  mov     al,[bx+3]           ; Maksimalna brzina se postize uz
           or      al,20H              ;  nesto samomodifikacije
           cmp     al,'x'
           jne     PSPERROR
           mov     MXSPD,1
           push    ds
           push    es
           mov     cx,MAXSPDEND+5-MAXSPEED
           lea     si,MAXSPEED
           lea     di,SLOW_MAIN
           mov     ds,PROGSEG
           mov     es,PROGSEG
           cld
           rep     movsb
           pop     es
           pop     ds
           add     bx,3
           jmp     PSPC1

SETSYNC:   call    GETPSPNUM
           cmp     dx,311
           ja      PSPERROR
           nojumps
           mov     VIDSYNC,dx
           jmp     PSPC1

SETAYMASK: inc     bx                  ; Maskiranje AY zvucnih kanala
           mov     al,[bx]
           if      VER_128
           cmp     al,'a'
           je      ADLIB
           cmp     al,'A'
           je      ADLIB
           endif
           mov     BLASTPRES,0
           cmp     al,'0'
           jumps
           jb      PSPERROR
           cmp     al,'8'
           ja      PSPERROR
           nojumps
           if      VER_128
           sub     al,'0'
           or      ACTMASK,al
           endif
           jmp     PSPC1

           if      VER_128

ADLIB:     mov     BLASTPRES,1
           call    INITADLIB
           jmp     PSPC1

           endif

ULASTATE:  inc     bx                  ; Da li treba border i zvuk ?
           mov     al,[bx]
           jumps
           cmp     al,'0'
           jb      PSPERROR
           cmp     al,'5'
           ja      PSPERROR
           nojumps
           cmp     al,'3'
           jbe     ULANORM
           xor     ULAMODE,4
           add     al,al
           sub     al,3
ULANORM:   test    al,1
           jnz     ULABORD
           push    ax
           call    SETBRDFAR
           pop     ax
ULABORD:   test    al,2
           jnz     ULASOUND
           push    bx
           call    SETSNDFAR
           pop     bx
ULASOUND:  jmp     PSPC1

KARTICA    macro   SLOVO,VR            ; Makro za VIDEOCRD
           local   NIJE
           cmp     al,SLOVO
           jne     NIJE
           mov     NAMJERNA,VR
           jmp     PSPC1
NIJE:
           endm

VIDEOCRD:  inc     bx                  ; Forsiranje graficke kartice.
           mov     al,[bx]
           or      al,32
           KARTICA 'c',1
           KARTICA 'e',3
           KARTICA 'm',5
           KARTICA 'w',6
           KARTICA 'h',7
           KARTICA 'x',8
           KARTICA 'v',9
           jmp     PSPERROR

JOYTIP     macro   SLOVO,VR1,VR2       ; Makro za SETJOY
           local   NIJE
           cmp     al,SLOVO
           jne     NIJE
           ifdif   <VR1>,<0>
           mov     KEMPDEV,VR1
           endif
           ifdif   <VR2>,<0>
           mov     JTYPE,VR2
           endif
           jmp     PSPC1
NIJE:
           endm

SETJOY:    inc     bx                  ; Izbor joysticka
           mov     al,[bx]
           or      al,32
           JOYTIP  'm',1,0             ; Kempston misom
           JOYTIP  'k',2,1             ; Kempston kurzorima
           JOYTIP  'c',0,2             ; Cursor kurzorima
           JOYTIP  's',0,7             ; Sinclair joystick A kurzorima
           JOYTIP  'b',0,12            ; Sinclair joystick B kurzorima
           cmp     al,'a'              ; Kempston analognim
           jne     NOANALOG
           mov     KEMPDEV,3
           call    CALIBRATE
           jnc     NOANALOG
           jmp     PSPC1
NOANALOG:  jmp     PSPERROR

SETATMODE: inc     bx                  ; Izbor nacina simulacije atributa
           mov     al,[bx]
           cmp     al,'0'              ; Sjenke (default)
           jumps
           je      PSPC1
           jb      PSPERROR
           cmp     al,'2'
           ja      PSPERROR
           nojumps
           add     al,79               ; Monohromatski (BW ili WB)
           mov     ATTRMODE,al
           jmp     PSPC1

AUTOTAPE:  push    es                  ; Automatski SAVE/LOAD nacin 1
           if      VER_128
           mov     es,ALTMEMSEG
           else
           mov     es,MEMSEG
           endif
           mov     word ptr es:[1388],0FFEDH
           mov     word ptr es:[1232],0FEEDH
           mov     word ptr es:[1372],0
           mov     si,1390
           mov     di,1234
           cld
ATPLOOP:   inc     bx
           mov     al,[bx]
           cmp     al,13
           je      ATPEND
           cmp     al,' '
           je      ATPEND
           mov     es:[si],al
           stosb
           inc     si
           jmp     ATPLOOP
ATPEND:    mov     ax,201*256
           mov     word ptr es:[si],ax
           stosw
           pop     es
           jmp     PSPC2

AUTOTAPE2: push    es                  ; Automatski SAVE/LOAD nacin 2
           if      VER_128
           mov     es,ALTMEMSEG
           else
           mov     es,MEMSEG
           endif
           mov     word ptr es:[1388],0FDEDH
           mov     word ptr es:[1390],201*256
           mov     word ptr es:[1232],0FCEDH
           mov     word ptr es:[1234],201*256
           mov     word ptr es:[1372],0
           pop     es
           jmp     PSPC1

INHKEYS:   inc     bx                  ; Zabrana pojedinih funkcijskih tipki
           mov     al,[bx]
           cmp     al,'0'
           jumps
           jb      PSPERROR
           cmp     al,'8'
           ja      PSPERROR
           nojumps
           test    al,1
           jz      NOINHF4
           mov     FKADDR[6],offset DUMMY
NOINHF4:   test    al,2
           jz      NOINHF5
           mov     FKADDR[8],offset DUMMY
NOINHF5:   test    al,4
           jz      NOINHF6
           mov     FKADDR[10],offset DUMMY
NOINHF6:   jmp     PSPC1

SHOWHLP:   lea     dx,HLPMSGS          ; Help ekran
           mov     ds,STEKSEG
           mov     ah,9
           int     21H
           lea     bx,HLPTAB
           lea     di,FKADDR[6]
           mov     cx,3
HLPLOOP:   mov     dx,ss:[bx]
           cmp     word ptr ss:[di],offset DUMMY
           je      HLPSKIP
           mov     ah,9
           int     21H
HLPSKIP:   add     bx,2
           add     di,2
           loop    HLPLOOP
           lea     dx,HLPEND
           mov     ah,9
           int     21H
           call    CLRKBD
           mov     ax,4C00H            ; Kraj programa
           int     21H

GETPSPNUM: xor     dx,dx               ; Citanje broja iz komandne linije
           inc     bx
           mov     al,[bx]
PSPNLOOP:  cmp     al,'0'
           jb      PSPERROR
           cmp     al,'9'
           ja      PSPERROR
NUMOK:     sub     al,'0'
           push    bx
           mov     bl,al
           xor     bh,bh
           mov     ax,dx
           imul    ax,ax,10
           mov     dx,ax
           add     dx,bx
           pop     bx
           inc     bx
           mov     al,[bx]
           cmp     al,13
           je      ENDPAR
           cmp     al,'/'
           je      ENDPAR
           cmp     al,';'
           je      ENDPAR
           cmp     al,' '
           jne     PSPNLOOP
ENDPAR:    dec     bx
           ret

PSPERROR:  lea     dx,PSPERMSG         ; Greska u skaniranju
PSPER1:    mov     ds,STEKSEG
           jmp     ILL2

CSEG       ends

;
; Privremeni stek segment, samo za inicijalizaciju.
;

TEMPSSEG   segment stack 'stack'

           db      20 dup (?)

TEMPSSEG   ends

           end     START
