;
;          WARAJEVO Spectrum Simulator    Release 2.0
;		 (C) Zeljko Juric & Samir Ribic
;

          .286

           warn

           include VERSION.ASM

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

           extrn   ZXP_IN:near,ZXP_OUT:near
           extrn   H_POCHRL:near,V_POCHRL:near,X_POCHRL:near
           extrn   SETRST24:near,SETZXI1:far
           extrn   SETLMODE:far,NTH:far,CREATETAPE:near
           extrn   MONITOR:near,TMANAGER:far
           extrn   KILLEDGE:far,R_DISABLE:near
           extrn   GETINST:near,DOSHADOW:near
           extrn   MDV_OUT:near,MDV_IN:near
           extrn   ZXI1_STS:near,ZXI1_CTRL:near
           extrn   ZXI1_IN:near,ZXI1_OUT:near
           extrn   MDV_TIMER:near,MDVINIT:far
           extrn   MDVUPDT:far,ADJUSTTBL:near
           extrn   EARSAMPLE:far,TSTSMPBLK:far
           extrn   DISPDIS:far

           public  RESTTIMER,SETTIMER,RESTKBD,SETKBD
           public  MASKA,TCLFLAG,FLASH,BUZZ,BORDONLY
           public  ADR_DF,LASTB,ULA,STOP_F,SLOW_MAIN
           public  VIDEOSEG,PROGSEG,MEMSEG,ULAMODE
           public  REPAINT,KLIK,ILL1,ESCSMSG,SCRBUF
           public  POCHRL,ARESC,KBDROWS,FILTER,BLASTADDR
           public  DS2PTR,NOESCAPE,FARSTOPF,RSTPIC
           public  INPORTS,OUTPORTS,MAXLSON,MAXLSOFF
           public  INT_STACK,FLAGS,FLAGSL,FLAGSH
           public  OPTIONE,RSI1_OUT,RSI1_IN,WINTROUB
           public  USER_IN,USER_OUT,RSI1_C,RSI1_T
           public  INKPAP,ATTRCHG,VIDEOCHG,DEBUGZONE
           public  STEKSEG,SNAPSEG,ULAFAR,TSKSWTCH
           public  VIDEOFLG,IRQCNT,VIDSYNC,EXTRADELAY
           public  ATTR,ATRCONV,EGAVGA,SBINSETUP,DSPRESET

           if      VER_128

           public  ALTMEMSEG,MMU,MMUSTATE,MMUVAR
           public  ACT_HL,ACT_AUX,ACT_STP
           public  XVIDEOCHG,XATTRCHG

           endif

;
; ZXDSEG simulira memoriju ZX Spectruma.
;

ZXDSEG     segment public

           db      65535 dup (?)

ZXDSEG     ends

;
; ZXDSEG2 je bafer za ucitavanje sa kasete, a ZXEXPBUF bafer za dekompresiju.
; ZXDSEG2 se koristi i za preklapanje ROM-a za ZX INTERFACE 1. ZXEXPBUF se
; koristi i za Snapshot, jer i on ima kompresiju. U ZXDSEG2 se ucitava i
; konfiguracioni fajl.
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
; ZXI1ROM je prostor u koji se smjesta ROM ZX INTERFACE-a 1.
;

ZXI1ROM    segment public

           db      16384 dup (?)

ZXI1ROM    ends

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

;
; Alternativna memorijska stranica za Spectrum 128.
;

ZXALTDSEG  segment public

           db      65535 dup (?)

ZXALTDSEG  ends

           endif

;
; SSEG sadrzi interni stek, tabele, i sistemske varijable. Podijeljen je u 2
; dijela (da se smanji djubre pri linkovanju): SSEG1 sadrzi varijable koje imaju
; inicijalne vrijednosti, a SSEG2 sadrzi varijable koje se inicijaliziraju nakon
; pokretanja programa.
;

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   TMNAME:byte,CURBLK:word
           extrn   PAGEWORDS:word
           extrn   TABLE1:word,SAMPLEON:byte
           extrn   NETDELAY:word,FREELEN:word,BUSYLEN:word

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
; JUMP tabele za IN i OUT instrukcije.
;

INPORTS    dw      31 dup (BIG_IN),JOYSTICK
           dw      6 dup (31 dup (BIG_IN),BUS_IDLE)
           dw      7 dup (BIG_IN),MDV_IN
           dw      7 dup (BIG_IN),ZXI1_STS
           dw      7 dup (BIG_IN),ZXI1_IN
           dw      3 dup (BIG_IN),ZXP_IN,BIG_IN,IN_128,KEYBOARD,BUS_IDLE

OUTPORTS   dw      7 dup (31 dup (BIG_OUT),USER_OUT)
           dw      7 dup (BIG_OUT),MDV_OUT
           dw      7 dup (BIG_OUT),ZXI1_CTRL
           dw      7 dup (BIG_OUT),ZXI1_OUT
           dw      3 dup (BIG_OUT),ZXP_OUT,BIG_OUT,OUT_128,ULA_OUT,USER_OUT

;
; Podaci za Hercules karticu.
;

HGCCARD    dw      0B000H,0
           dw      H_VID_UPD,H_A_UPD
           dw      H_TEKST,H_FLASH,H_STOP_F,H_POCHRL
           dw      H_GRAPH,H_BORDER

;
; Podaci za Hercules karticu, bez atributa.
;

HGCMCARD   dw      0B000H,0
           dw      H_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY,H_POCHRL
           dw      H_GRAPH,DUMMY

;
; Podaci za CGA karticu.
;

CGACARD    dw      0B800H,0
           dw      C_VID_UPD,C_A_UPD
           dw      C_TEKST,DUMMY,DUMMY,H_POCHRL
           dw      C_GRAPH,C_BORDER

;
; Podaci za CGA bez atributa.
;

CGAMCARD   dw      0B800H,0
           dw      H_VM_UPD,DUMMY
           dw      C_TEKST,DUMMY,DUMMY,H_POCHRL
           dw      C_GRAPH,DUMMY

;
; Podaci za CGA pod Windows-om.
;

WINCARD    dw      0B800H,-6
           dw      C_VID_UPD,C_A_UPD
           dw      C_TEKST,DUMMY,DUMMY,H_POCHRL
           dw      W_GRAPH,C_BORDER

;
; Podaci za VGA karticu.
;

VGACARD    dw      0A000H,328
           dw      V_VID_UPD,V_A_UPD
           dw      V_TEKST,V_FLASH,V_STOP_F,V_POCHRL
           dw      V_GRAPH,V_BORDER

;
; Podaci za EGA karticu.
;

EGACARD    dw      0A000H,6328-15*80
           dw      V_VID_UPD,V_A_UPD
           dw      E_TEKST,V_FLASH,V_STOP_F,V_POCHRL
           dw      E_GRAPH,V_BORDER

;
; Podaci za EGAMONO karticu.
;

EMONOCARD  dw      0A000H,6328
           dw      H_VID_UPD,EM_A_UPD
           dw      H_TEKST,EM_FLASH,EM_STOP_F,H_POCHRL
           dw      EM_GRAPH,EM_BORDER

;
; Podaci za EGAMONO karticu bez atributa.
;

EGAMMCARD  dw      0A000H,6328
           dw      H_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY,H_POCHRL
           dw      EM_GRAPH,DUMMY

;
; Podaci za Extended Hercules.
;

EXTHCARD   dw      0B000H,0
           dw      X_VID_UPD,X_A_UPD
           dw      H_TEKST,H_FLASH,H_STOP_F,X_POCHRL
           dw      X_GRAPH,X_BORDER

;
; Podaci za Extended Hercules bez atributa.
;

EXTHMCARD  dw      0B000H,0
           dw      X_VM_UPD,DUMMY
           dw      H_TEKST,DUMMY,DUMMY,X_POCHRL
           dw      X_GRAPH,DUMMY

;
; Vrijednosti video registara za grafiku na Herculesu (GRAPHCON = obicna,
; EXTCON = prosirena)
;

GRAPHCON   db      35H,2DH,2EH,07H,5BH,02H,58H,58H,02H,03H,00H,00H
EXTCON     db      35H,2AH,2BH,0AH,64H,02H,61H,61H,02H,03H,00H,00H

;
; Tabela za MultiColor hakerluke
; Svaki entry : IM2 adresa ili HALT adresa, EXTRADELAY+(130+EXTRAKUR)*256
;

           ife     VER_128

HCKTABLEN  equ     12

HACKTAB    dw      32512,36+154*256    ; Overscan Demo - brzinski test
           dw      35081,244+154*256   ; Overscan Demo - glavna rutina
           dw      63288,243+130*256   ; Shock Megademo (65535)
           dw      60029,12+128*256    ; Advanced Machine Code Demo
           dw      40185,239+128*256   ; Bright Demo (65535)
           dw      40372,239+128*256
           dw      40574,239+128*256
           dw      40760,239+128*256
           dw      40952,239+128*256
           dw      42522,239+128*256
           dw      51764,8+134*256     ; Pepper 3
           dw      36357,3+128*256     ; Pepper 4

           else

HCKTABLEN  equ     20

HACKTAB    dw      32512,34+128*256    ; Overscan test
           dw      35081,248+160*256   ; Overscan demo
           dw      63288,248+130*256   ; Shock megademo (65535)
           dw      58307,5+152*256     ; AY Demo (65535)
           dw      40185,243+145*256   ; Bright Demo (65535)
           dw      40372,243+145*256
           dw      40574,243+145*256
           dw      40760,243+145*256
           dw      40952,243+145*256
           dw      42522,243+145*256
           dw      33024,250+128*256   ; Emittion Sexual Pleasure
           dw      65506,255+128*256   ; Hypnotic world (65535)
           dw      33281,3+140*256     ; Inner Universe
           dw      23500,34+128*256    ; Echologia test
           dw      25694,234+170*256   ; Echologia MultiColor part
           dw      33513,233+108*256   ; Echologia Sound In Line part
           dw      37613,21+127*256    ; MDA demo intro
           dw      34618,38+165*256    ; MDA demo main
           dw      59110,247+150*256   ; No more inteligence 2
           dw      38481,249+182*256   ; Crystal Kilodemo

           endif

;
; Rutine za inicijalizaciju konverzionih tabela za kartice.
;

CARDS      dw      BADCARD,INITCGA,BADCARD,INITEGA,BADCARD,INITEM
           dw      INITWIN,INITHERC,INITEXTH,INITVGA,BADCARD

CARDPTR    dw      0,CSEG2             ; Vektor za daleki poziv rutine

;
; Poruke o greskama.
;

SHADTXT    db      ' (Shadow ROM)$'

CFERRMS    db      'Error loading CFG file$'
ESCAPED    db      'Program finish at address #$'
UNEXPMSG   db      'Unexpected wraping at address #$'
ESCSMSG    db      'Escape sequence detected at address #$'
BCMESS     db      'Invalid graphic card$'
LRIERROR   db      'File ZXI1.ROM not found$'
CPUERR     db      'Cannot work on 8086/8088$'
PSPERMSG   db      'Invalid command line parameter$'
DIRERMSG   db      'Invalid drive or directory$'
TCLERMSG   db      'Invalid tape name or tape error$'
MDVERRMSG  db      'Cannot open cartridge file$'
MDVLENERR  db      'Cartridge file too long or short$'
NOSNAPERR  db      'Cannot open snapshot file$'
SNPERRMSG  db      'Incorrect snapshot file$'
HRDWERMSG  db      'Incorrect hardware version$'
CHNERMSG   db      'Error loading environment$'
BACKERROR  db      'CS must not be changed$'
OBSERMSG   db      'Obsolete command option$'
NETERMSG   db      'Cannot open net file$'
SBNFND     db      'Sound Blaster card cannot be detected$'

VIRTMSG    db      'ษออ[þ]อออออออออ Warning ออออออออออออออออป'
           db      'บ                                       บ'
           db      'บ    Procesor is in virtual 8086 mode.  บ'
           db      'บ    This may cause interactions with   บ'
           db      'บ    some programs. It is highly        บ'
           db      'บ    recommended to switch off all      บ'
           db      'บ    virtual mode drivers like EMM386   บ'
           db      'บ    etc. Press ''Esc'' to quit or any    บ'
           db      'บ    other key for continue anyway.     บ'
           db      'บ                                       บ'
           db      'ศอออออออออออออออออออออออออออออออออออออออผ'

           if      VER_128

PTEXT      db      ' (RAM page $'
DERBYTXT   db      ' (Derby ROM)$'
STDTEXT    db      ' (Standard ROM)$'

LRERROR    db      'File ZX128_0.ROM or ZX128_1.ROM not found$'
LRPERROR   db      'File ZXP2_0.ROM or ZXP2_1.ROM not found$'

           else

LRERROR    db      'File ZX48.ROM not found$'
TESTMSG    db      'Program use Spectrum 128 ports at address #$'

           endif

;
; Vrijednosti segmentnih registara za program, memoriju, i stek. Na verziji 128
; MEMSEG i ALTMEMSEG nisu staticni, nego ih mijenja MMU.
;

PROGSEG    dw      CSEG
MEMSEG     dw      ZXDSEG
STEKSEG    dw      SSEG
CSEG2SEG   dw      CSEG2
DS2PTR     dw      ZXDSEG2
SNAPSEG    dw      ZXEXPBUF

           if      VER_128

ALTMEMSEG  dw      ZXALTDSEG

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

PSPOPTS    db      'cruvtfdnkabqjem%yls2hopwi#!?&`',0      ; ` je @+32

PSPCOM     dw      PSPCONF,RECOVER,ULASTATE,VIDEOCRD
           dw      PSPTAPE,PSPFILE,PSPDIR,PSPNOSCR
           dw      INVERTCUR,SETARUN,ACCBASIC,SETSYNC
           dw      SETJOY,EXPAND,SETAMODE,SETSPEED
           dw      SETINITA,LOADMODE,SETAYMASK,SETPLUS2
           dw      HIDEBORD,SETFILTER,SETZXP,OBSOLETE,OBSOLETE
           dw      SETMDV,NOWARNING,TESTING,ENVPARS
           dw      TIMEEXIT

;
; Konverzione tablice za snimanje konfiguracione datoteke.
;

CONVCARD   db      '?C?E?MWHXV'
CONVKEMP   db      '?MKA'
CONVJOY    db      '??C????S????B'
CONVPRT    db      '?EBLP'

;
; Tablica za konverziju tastature.
;

;
;
; 0-7     = polured za IN
; 8       = nista
; 9       = funkcijska tipka
; 10-199  = dvojna tipka
; 200-255 = tipke za keypad i joystick
;

HALFROW    db      8,17,3,3,3,3,3,4,4,4,4,4,8,8,10,11
           db      2,2,2,2,2,5,5,5,5,5,8,8,6,0,1,1
           db      1,1,1,6,6,6,6,8,8,200,0,8,0,0,0,0
           db      7,7,7,8,8,205,0,206,7,7,12,9,9,9,9,9
           db      9,9,9,9,9,8,216,208,201,209,207,202,210,203,200,211
           db      204,212,213,214,8,8,8,9,9,8,8,8,8,8,8,8
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

FKADDR     dw      HELPSCR,TMANAGER1,MONITOR,SETSOUND,HSETUP
           dw      SETBORDR,SNAPSH,NMI,RESET,EXIT
           dw      18 dup (DUMMY),INCKUR,DECKUR

KBDROWS    db      8 dup (255)         ; 8 poluredova tastera, setuje ih INT 9

BORCOL     db      0,1,4,5,2,3,6,7     ; Boje BORDER-a za EGA i VGA karticu

ULA        dw      ULAFULL             ; Pokazivac na rutinu za ULA simulaciju

FL_CNT     dw      30                  ; Tekuca vrijednost brojaca za FLASH

F5INIT     dw      0,CSEG2             ; F5 far pointer za color inicijalizaciju
F5INITM    dw      0,CSEG2             ; F5 far pointer za mono inicijalizaciju

KBDJOYTAB  db      16,8,2,1,4          ; Tabela za Kempston konverziju

JOYRPLC    db      11,8,6,9,7          ; Konverzija za cursor joystick
           db      6,5,2,3,4           ; Konverzija za Sinclair 1
           db      11,10,7,8,9         ; Konverzija za Sinclair 2

CALDIGS    db      100,10,1            ; Cifre za ocitavanje parametra iza /JA

SNPFILE    db      'SNAP.Z80',0        ; Ime snapshot datoteke za snimanje
CFFILE     db      'SPECSIM.CFG',0     ; Ime konfiguracione datoteke

IMEI1      db      'ZXI1.ROM',0        ; Ime ZXI1 ROM datoteke

NUMBUF     db      '00000',0           ; Bafer za numericku ASCII konverziju

           if      VER_128

IME1       db      'ZX128_0.ROM',0     ; Ime 128 Derby ROM datoteke
IME2       db      'ZX128_1.ROM',0     ; Ime 128 standardne ROM datoteke
IMEP1      db      'ZXP2_0.ROM',0,0    ; Ime +2 Derby ROM datoteke
IMEP2      db      'ZXP2_1.ROM',0,0    ; Ime +2 standardne ROM datoteke

           else

IME        db      'ZX48.ROM',0        ; Ime 48 ROM datoteke

           endif

CHAINPATH  db      'WARAJEVO.EXE',0    ; Ime okruzenja za CHAIN
CHAINCOM   db      2,'/C',0            ; Parametar koji se prosljedjuje

ULAMODE    db      3                   ; Rezim ULA simulacije
INITATTR   db      7                   ; Inicijalni atribut (/Y)

           evendata

VIDSYNC    dw      150                 ; Tip video sinhronizacije
OLDVIDSYNC dw      150                 ; Stara vrijednost VIDSYNC za MultiColor
OLDVIDS2   dw      150                 ; Jos jedna kopija VIDSYNC

SPEED      dw      (76+1)*2            ; Brzinska konstanta za Real-Time
SPEEDPRCT  dw      100                 ; Procenat brzine
SBSPEED    dw      200                 ; Brzinska konstanta za SB AY emulaciju

SLFLAG     db      1                   ; 0 ako se ne iscrtava slika (/N)

RSI1_B     db      0                   ; Hvata bajt koji se salje na RS232
RSI1_C     db      8                   ; Brojac bitova (9 do pojave start bita)
RSI1_T     db      0                   ; Interni fleg

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

RS128_B    db      0                   ; Kao i RSI1_B
RS128_C    db      8                   ; Kao i RSI1_C
RS128_T    db      0                   ; Kao i RSI1_T
MIDI_B     db      0                   ; analogno i za MIDI
MIDI_C     db      8
MIDI_T     db      0

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
           dw      SETENVL,STARTENV,RS128_OUT,NOTH

AY_ACTS_SB dw      SB_CHASET,SB_CHASET,SB_CHBSET,SB_CHBSET
           dw      SB_CHCSET,SB_CHCSET,SB_NOISET,SB_MIXSET
           dw      SETVA,SETVB,SETVC,SETENVL
           dw      SETENVL,STARTENV,RS128_OUT,NOTH

;
; Tabela decibelskih vrijednosti za Adlib
;

DECIBEL    db      255,63,63,63,44,38,34,29,22,18,12,10,7,5,2,1

OPLCOUNT   dw      1                   ; 35 za stare OPL cipove

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

;
; Tabele i varijable za simulaciju KEYPAD-a
;

KPDTABLE   db      255,223,255,255,4 dup (223,255,255,255)
           db      5 dup (223,223,255,255,4 dup (223,223,255,255))

KPDCONV    db      101,105,81,93,101,117,101,109,41,53
           db      61,77,61,73,81,85,81,97,81,89
           db      101,113,41,57,41,49,21,37,21,29
           db      61,69,61,65,41,45

KPDCONV_E  db      101,105,61,73,81,97,81,89,81,93
           db      61,77,61,73,81,85,41,57,101,109
           db      101,113,41,53,101,105,21,37,61,69
           db      61,69,61,65,41,45

KPDLAST    db      255                 ; Prethodni OUT na KEYPAD
KPDTIMER   db      2                   ; Brojac koliko dugo se KEYPAD ne koristi

           endif

DMACH      db      1                   ; DMA kanal za SoundBlaster
PAGEREG    db      87H,83H,81H,82H     ; Page registri za odgovarajuce kanale

ENVVNAME   db      9,'WARAJEVO='       ; Environment varijabla WARAJEVO

DEBUGZONE  db      0,0,0,0,0,0,0,0     ; Za debagiranje

SSEG1      ends

SSEG2      segment public

           extrn   SEARCHREC:byte,ERASECNT:byte
           extrn   IFF:word,IFF1:byte,IFF2:byte
           extrn   I:byte,R7:byte,HSTATE:byte,PV:byte,N:byte
           extrn   IMODE:byte,IX:word,IY:word,RCLK:word
           extrn   PVN:word,ALTA:byte,ALTPVN:word,ALTBC:word
           extrn   ALTDE:word,ALTHL:word,NETFNAME:byte
           extrn   R:byte,SHADOW:byte,LDCNT:word
           extrn   PVN:word,ALTF:word,BLOCKID:byte
           extrn   LOAD_MODE:byte,STOP_MODE:byte
           extrn   PRTMODE:byte,ENDSS:byte,INGTAPE:byte
           extrn   MHANDLES:word,CARTNAMES:byte
           extrn   SSTEPON:byte,SPROTECT:byte
           extrn   SMPSTARTER:byte,EDGEOLD:word

TEMP_STACK db      100 dup (?)         ; Privremeni stek za INT_9
INT_STACK  db      1000 dup (?)        ; Interni stek

FLAGS      label   word                ; Na ovom mjestu na steku se cuvaju
FLAGSL     db      ?                   ;  flegovi, pa je dobro imati alias ime
FLAGSH     db      ?                   ;  radi mogucnosti direktnog pristupa

PSPSEG     dw      ?                   ; Adresa PSP-a

WARVARBUF  db      256 dup (?)         ; Bafer za WARAJEVO environment varijablu

;
; Ovdje se ucitava heder okruzenja prilikom CHAIN-a.
;

EXEHEADER  dw      ?,?                  ; Pocetak zaglavlja
EXEPAGES   dw      ?,?,?                ; Duzina EXE fajla u stranicama
MINHEAP    dw      ?,?                  ; Minimalna potrebna memorija
CHAINSS    dw      ?                    ; SS za EXE fajl
CHAINSP    dw      ?,?                  ; SP za EXE fajl
CHAINIP    dw      ?                    ; IP za EXE fajl
CHAINCS    dw      ?,?,?                ; CS za EXE fajl

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

TABLELEN   equ     10

VIDEOSEG   dw      ?                   ; Segment video memorije
VIDEOFFS   dw      ?                   ; Offset u video memoriji
UPDT_VID   dw      ?                   ; Adresa rutine za azuriranje ekrana
UPDT_ATTR  dw      ?                   ; Adresa rutine za azuriranje atributa
TEKST      dw      ?                   ; Adresa rutine za prelazak u tekst mod
FLASH      dw      ?                   ; Adresa rutine za FLASH
STOP_F     dw      ?                   ; Adresa rutine za zaustavljanje FLASH-a
POCHRL     dw      ?                   ; Adresa jezgra rutine za ispis znaka
GRAPH      dw      ?                   ; Adresa rutine za aktiviranje grafike
BORDER     dw      ?                   ; Adresa rutine za BORDER

BRDCOL     dw      ?                   ; Boja BORDER-a za HiCol BORDER (VGA)

VIDEOHCK   dw      ?                   ; AdHoc kompenzacione promjenljive za
ATTRHCK    dw      ?                   ;  HiCol BORDER konflikte u Low RAM-u

NAMJERNA   db      ?                   ; Kod za slucaj forsiranja kartice

LAST_01    dd      ?                   ; Stara vrijednost vektora za INT 01
LAST_08    dd      ?                   ; Stara vrijednost vektora za INT 08
LAST_09    dd      ?                   ; Stara vrijednost vektora za INT 09
LAST_0D    dd      ?                   ; Stara vrijednost vektora za INT 0D
LAST_1B    dd      ?                   ; Stara vrijednost vektora za INT 1B
LAST_2F    dd      ?                   ; Stara vrijednost vektora za INT 2F
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

           if      VER_128

JTYPETEMP  db      ?                   ; Privremeni spremnik za JTYPE

           endif

LEFTBND    dw      ?                   ; Lijevi prag za joystick
RIGHTBND   dw      ?                   ; Desni prag za joystick
UPBND      dw      ?                   ; Gornji prag za joystick
DOWNBND    dw      ?                   ; Donji prag za joystick

ACCXLO     dw      ?                   ; X akumulator za kalibraciju, niza rijec
ACCXHI     dw      ?                   ; X akumulator za kalibraciju, visa rijec
ACCYLO     dw      ?                   ; Y akumulator za kalibraciju, niza rijec
ACCYHI     dw      ?                   ; Y akumulator za kalibraciju, visa rijec

RESTFNAME  db      128 dup (?)         ; Ime snapshot datoteke za /R

SNPHANDLE  dw      ?                   ; Handler za snapshot datoteku
MAX        dw      ?                   ; Gornja granica za snapshot kompr./dek.

DIRNAME    db      128 dup (?)         ; Ime diska i direktorija za /D
DEFDIR     db      65 dup (?)          ; Default direktorij
DEFDRIVE   db      ?                   ; Default drive

DIRPTR     dw      ?                   ; Pointer na ime direktorija
SWAPDRV    db      ?                   ; Swap drajv

INHIBIT    db      ?                   ; 0FFH ako su onemogucene funkc. tipke

           if      VER_128

;
; Indirektni vektori za specijalne memorijske akcije na 128K Spectrumu
;

ACT_HL     dw      ?                   ; LD (HL),<byte>
ACT_AUX    dw      ?                   ; LD (AUX),<byte>
ACT_STP    dw      ?                   ; LD (SP),<byte>

           endif

FKVEC      dw      ?                   ; Vektor za trenutnu funkcijsku tipku

EXTHFLG    db      ?                   ; 1 nakon koda 224 (EXT_KBD)

FL_CREG    db      ?                   ; Upravljacki registar video chipa

ATTRMODE   db      ?                   ; Rezim za simulaciju atributa

KEMPDEV    db      ?                   ; Kempston: 0=nema, 1=mis, 2=tastatura,
                                       ;  3=analogni

KJOYVAL    db      ?                   ; Vrijednost za IN 31

JTYPE      db      ?                   ; 0=klasicno, 1=Kempston, 2=Cursor,
                                       ;  7=Sinclair 1, 12=Sinclair 2

LRCOUNT    dw      ?                   ; Brojac lijevo/desno za misa
UDCOUNT    dw      ?                   ; Brojac gore/dolje za misa

LUNTV30    db      ?                   ; 1 ako se ucitava snap iz verzije 3.0.
SNAPERR    db      ?                   ; 1=err_in_snap, 2=hrdwr_err, 3=no_file

DATNUM     dw      ?                   ; Redni broj datoteke u traci

           evendata

VIDEOFLG   dw      ?                   ; Fleg za azuriranje ekrana
IRQCNT     dw      ?                   ; Brojac IRQ zahtijeva

WORKTIME   dw      ?                   ; Vrijeme nakon kojeg se prekida rad

AFLAG      db      ?                   ; 1 ako se trazi autostart (/A)
FBASIC     db      ?                   ; 1 ako je ubrzan BASIC
INVCURS    db      ?                   ; 12 ako treba inverzni kurzor
TCLFLAG    db      ?                   ; 1 ako je zadano ime trake
SNLOAD     db      ?                   ; 1 ako treba ucitati snapshot
PAGED      db      ?                   ; 1 ako je Shadow ROM u snapshotu
CHGDRDIR   db      ?                   ; 1 ako je zadano nesto sa /D
NOESCAPE   db      ?                   ; 1 ako su zabranjene Escape sekvence
FILTER     db      ?                   ; 1 ako je zadano /O
VIRTMODE   db      ?                   ; 1 ako je procesor u 'virtual 8086' modu
VIRTWARN   db      ?                   ; 1 ako ne treba ispisati VM upozorenje
TESTFLG    db      ?                   ; 1 ako je zadano /?128
ZXI1P      db      ?                   ; 1 ako je prisutan ZXI1.ROM
ZXI1OLD    db      ?                   ; 1 ako se koristi stari ZXI1 ROM
OPTIONE    db      ?                   ; 1 ako je aktivna emulacija ZXI1
EXPE       db      ?                   ; 1 ako se preklapa svih 16K
MANCAL     db      ?                   ; 1 ako je manualna kalibracija za /JA
NETPRES    db      ?                   ; 1 ako se koristi Network
MXSPD      db      ?                   ; 1 ako je MaxSpeed mod
MAXLOAD    db      ?                   ; 1 ako je Maximal Loading Speed mod
EXTRABEEP  db      ?                   ; 1 ako je ExtraBeep mod
EGAVGA     db      ?                   ; 1 ako je EGA ili VGA
VGAOK      db      ?                   ; 1 ako je VGA a nije MaxSpeed
MPXINST    db      ?                   ; 1 ako je instaliran Multiplex interapt
TSKSWTCH   db      ?                   ; 1 ako je nastupio Task Switch
CALLENV    db      ?                   ; 1 ako je /@@

USERTEMP   db      ?                   ; Privremena varijabla

OLDSPEED   dw      ?                   ; Stara brzina u Max Loading Speed modu

ENVPBUF    db      1000 dup (?)        ; Bafer za opcije /&
ENVPPTR    dw      ?                   ; Pokazivac u ENVPBUF

           evendata

LASTTIMER  dw      ?                   ; Zadnja vrijednost timera za Real-time

OLDR       db      ?                   ; Stara vrijednost R za MultiColor
EXTRADELAY db      ?                   ; Podesivo kasnjenje za MultiColor
HACKDELAY  db      ?                   ; Automatsko kasnjenje za MultiColor
EXTRAKUR   db      ?                   ; Jos jedan od hakerluka

LASTAL     db      ?                   ; Zadnji AL kod IN-a sa SBlastera
RANDOM     db      ?                   ; Slucajan broj

WINTROUB   db      ?                   ; Windows troubleshooting opcije

           if      VER_128

PLUS2FLG   db      ?                   ; 1 ako se emulira +2
CHGFLAGS   db      ?                   ; Neki flegovi za 128 autostart

           endif

BLASTPRES  db      ?                   ; 1 ako je Blaster ili ADLIB
BLASTADDR  dw      ?                   ; 0 ako je ADLIB, inace SB baznu adresa

           if      VER_128

MMUSTATE   db      ?                   ; Stanje MMU

ENVCNT     dw      ?                   ; Brojac za generator envelope
ENVLEN     dw      ?                   ; Period envelope
VOLSTEP    db      ?                   ; 1=ATTACK, -1=DECAY, 0=OFF
CURVOL     db      ?                   ; Tekuca glasnoca generatora
FINVOL     db      ?                   ; Krajnja glasnoca nakon jednog ciklusa

STOPAY     db      ?                   ; 1 ako se ne simulira AY zbog F4

KPDFLAG    db      ?                   ; 1 ako KEYPAD radi
KPDPOS     dw      ?                   ; Pokazivac u tabeli KPDTABLE

MMUTEMP    db      ?                   ; Privremeno cuva MMU za snapshot

AY_REG     db      ?                   ; Oznacava koji se AY registar adresira

AY_REGS    db      16 dup (?)          ; Registri zvucnog cipa

VOLA       db      ?                   ; Aktuelna glasnoca kanala A
VOLB       db      ?                   ; Aktuelna glasnoca kanala B
VOLDUMMY   db      ?                   ; Lazna varijabla
VOLC       db      ?                   ; Aktuelna glasnoca kanala C

MAXSNDVAL  db      ?                   ; Vsrna vrijednost zvucnog izlaza

DIGBYTE    db      ?                   ; Jednobajtni DMA bafer

           evendata

;
; Varijable za egzaktnu simulaciju AY cipa semplovanjem.
;

CHAMAX     dw      ?                   ; Max. vr. brojaca ABC u T stanjima
CHBMAX     dw      ?
CHCMAX     dw      ?

NOIMAX     dw      ?                   ; Max. vr. brojaca suma

CHACUR     dw      ?                   ; Tekuca vr. brojaca ABC u T stanjima
CHBCUR     dw      ?
CHCCUR     dw      ?

NOICUR     dw      ?                   ; Tekuca vr. brojaca suma u T stanjima

CHAOSC     db      ?                   ; Logicko stanje oscilatora (0/255)
CHBOSC     db      ?
CHCOSC     db      ?

           evendata

NOIOSC     dw      ?                   ; Log. stanje generatora suma (0/65535)

CHAOFF     db      ?                   ; 255 ako kanal ne radi, inace 0
CHBOFF     db      ?
CHCOFF     db      ?

NOIAOFF    db      ?                   ; 255 ako na kanalu nema suma, inace 0
NOIBOFF    db      ?
NOICOFF    db      ?

CHAULT     db      ?                   ; 255 ako nije "ultrazvuk", inace 0
CHBULT     db      ?
CHCULT     db      ?

           endif

           evendata

SSEG2      ends

CSEG2      segment public

           assume  cs:CSEG2,ds:ZXDSEG,ss:SSEG,es:nothing

;
; Univerzalna CLS rutina za tekst nacin.
;

CLS        proc    far
           mov     ah,0FH
           int     10H
           xor     ah,ah
           int     10H
           ret
CLS        endp

;
; CHAIN potprogram za poziv okruzenja.
;

CHAIN:     lea     di,CHAINPATH
           mov     ds,STEKSEG
           mov     bx,di               ; Sacuvaj offset imena u BX
           mov     dx,di
           mov     ax,3D00H            ; Otvori okruzenje, read-only
           int     21H
           jnc     CHFOUND             ; Greska ako nije nadjeno okruzenje
CHERROR:   lea     dx,CHNERMSG
           mov     ah,09H
           int     21H
           jmp     far ptr ILLEND
CHFOUND:   mov     bx,ax               ; Handler u BX
           lea     dx,EXEHEADER
           mov     cx,28               ; Ucitaj EXE heder
           mov     ah,3FH
           int     21H
           jc      CHERROR
           mov     ax,EXEPAGES         ; Odredi broj stranica u EXE datoteci
           xor     dx,dx
           mov     cx,5
NSHIFTL:   shl     ax,1                ; Pomnozi taj broj sa 32
           rcl     dx,1
           loop    NSHIFTL             ; U DX:AX je sada broj paragrafa
           add     ax,MINHEAP          ; Dodaj minimalni HEAP
           adc     dx,0
           mov     cx,ax               ; Sacuvaj to u CX
           mov     ah,3EH
           int     21H                 ; Zatvori datoteku (zasad)
           jc      CHERROR
           or      dx,dx               ; DX mora biti 0, greska ako je >640K
           jnz     CHERROR
           mov     es,PSPSEG           ; U ES adresa PSP
           mov     ah,4AH
           mov     bx,0FFFFH           ; Utvrdi koliko se memorije moze dobiti
           int     21H
           mov     ax,bx               ; BX - koliko je paragrafa slobodno
           sub     ax,10H              ; Predvidi mjesto za PSP
           cmp     ax,cx               ; Provjeri ima li dovoljno mjesta
           jb      CHERROR
           mov     ah,4AH              ; Zauzmi maksimalnu mogucu memoriju
           int     21H
           jc      CHERROR
           mov     ax,es               ; U AX segment programa
           add     ax,bx               ; Dodaj sve raspolozive paragrafe
           sub     ax,1000H            ; Napravi mjesto za jedan segment
           mov     cs:TMPSS,ax         ; Smjesti to u privremeni SS za kasnije
           mov     bx,5
           mov     cx,15
CHCLOSE:   mov     ah,3EH              ; Pozatvaraj sve datoteke sa handlerima
           int     21H                 ;  vecim od 5 (zakljucno sa 20), da se
           inc     bx                  ;  ne zatvore STDIN itd.
           loop    CHCLOSE
           lea     si,CHAINCOM         ; Postavi DS:SI na liniju sa parametrima
           mov     di,80H              ;  a ES:DI na komandnu liniju u PSP-u
           lodsb                       ; Uzmi u AL bajt duzine i prekopiraj ga
           stosb
           mov     bx,di               ; Zapamti pocetak komandne linije u BX
           mov     cl,al
           xor     ch,ch
           rep     movsb               ; Iskopiraj parametre u PSP
           mov     al,13
           stosb                       ; Oznaci kraj sa CR
           mov     ds,PSPSEG
           mov     si,bx               ; Adresa komandne linije u DS:SI
           mov     di,005CH            ; Adresa FCB1 u PSP-u u ES:DI
           mov     ax,2901H            ; Postavi FCB1
           int     21H
           mov     di,006CH            ; Isto za FCB2
           mov     ax,2901H
           int     21H
           mov     bx,es               ; Odredi u BX segment pocetka koda
           add     bx,10H
           mov     ax,CHAINCS          ; Uzmi CS iz EXE hedera, relociraj ga i
           add     ax,bx               ;  smjesti u NEWCS
           mov     cs:NEWCS,ax
           mov     ax,CHAINIP          ; Inicijaliziraj IP
           mov     cs:NEWIP,ax
           mov     ax,CHAINSS          ; Inicijaliziraj SS
           add     ax,bx
           mov     cs:NEWSS,ax
           mov     ax,CHAINSP          ; Inicijaliziraj SP
           mov     cs:NEWSP,ax
           mov     cx,2CH              ; Duzina loadera u bajtima
           mov     di,100H             ; Izracunaj u DI adresu u PSP-u na koju
           sub     di,cx               ;  ce se iskopirati loader
           mov     cs:TMPIP,di
           mov     cs:TMPCS,es         ; Smjesti adresu za skok
           mov     ds,CSEG2SEG
           lea     si,CHAINCODE
           rep     movsb               ; Iskopiraj loader
           mov     ax,bx
           stosw                       ; Inicijaliziraj ECB
           stosw
           mov     ds,STEKSEG
           lea     dx,CHAINPATH        ; U DS:DX staza imena okruzenja, a u
           mov     bx,100H             ;   ES:BX adresa ECB
           mov     ax,4B03H            ; Priprema za Load Overlay DOS poziv
           cli
           mov     ss,cs:TMPSS         ; Smjesti stek na vrh memorije
           mov     sp,0FFFEH
           sti
           db      0EAH                ; JMP FAR na loader u PSP-u
TMPIP      dw      0
TMPCS      dw      0

TMPSS      dw      0                   ; Privremeni stek segment za CHAIN

;
; Loader za CHAIN koji se izvrsava u PSP-u
;

CHAINCODE: int     21H                 ; Pozovi Load Overlay
           jnc     GOODLOAD
           int     20H                 ; Greske su rijetke ali izadji ako ipak
GOODLOAD:  mov     dx,cs               ;  nastupi greska
           mov     ds,dx               ; Inicijaliziraj DS i ES
           mov     es,dx
           mov     ah,4AH              ; Odredi kolicinu slobodne memorije
           mov     bx,0FFFFH
           int     21H
           add     bx,dx               ; Oznaci kraj memorije u PSP
           mov     ds:[0002H],bx
           cli
           mov     ss,ds:[0FEH]        ; Inicijaliziraj stek
           mov     sp,ds:[0FCH]
           sti
           db      0EAH                ; JMP FAR na start programa
NEWIP      dw      100H
NEWCS      dw      0
NEWSP      dw      0FFFEH
NEWSS      dw      0

;
; Citanje environment varijable WARAJEVO.
;

READENV    proc    far
           cld
           mov     es,STEKSEG
           mov     dl,ENVVNAME
           xor     dh,dh
           push    ds
           mov     ds,PSPSEG
           mov     ds,ds:[002CH]       ; pointer na environment
           xor     si,si
RENVLOOP:  cmp     byte ptr [si],0
           je      RENVEND
           lea     di,ENVVNAME[1]
           mov     cx,dx
           repe    cmpsb
           jz      RENVEND
           dec     si
RENVSKP:   lodsb
           or      al,al
           jnz     RENVSKP
           jmp     short RENVLOOP
RENVEND:   mov     di,si
           xor     al,al
           mov     cx,256
           push    ds
           pop     es
           repne   scasb
           not     cl
           mov     WARVARBUF,cl
           lea     di,WARVARBUF[1]
           mov     es,STEKSEG
           rep     movsb
           cmp     byte ptr es:[di-2],' '
           jne     RENVFIN
           mov     byte ptr es:[di-2],0 ; Ignoriraj eventualno " G" na kraju
           sub     WARVARBUF,2          ;  koje moze biti u varijabli
RENVFIN:   pop     ds
           ret
READENV    endp

;
; Ukljucenje i iskljucenje Max Loading Speed moda.
;

MAXLSON    proc    far
           cmp     MAXLOAD,1
           jne     MAXLEND
           cmp     SPEED,1
           je      MAXLEND
           push    ax
           mov     ax,SPEED
           mov     OLDSPEED,ax
           mov     SPEED,1
           pop     ax
           ret
MAXLSON    endp

MAXLSOFF   proc    far
           cmp     MAXLOAD,1
           jne     MAXLEND
           cmp     OLDSPEED,0
           je      MAXLEND
           push    ax
           mov     ax,OLDSPEED
           mov     SPEED,ax
           mov     OLDSPEED,0
           pop     ax
MAXLEND:   ret
MAXLSOFF   endp

;
; Snimanje konfiguracione datoteke SPECSIM.CFG. Ne snima se /C, /N, /O i /@.
;

PUTW       macro   WORD
           mov     ax,WORD
           stosw
           endm

PUTB       macro   BYTE
           mov     al,BYTE
           stosb
           endm

CRLF       equ     13+256*10

CONFSAVE   proc    far
           cld
           mov     ds,STEKSEG
           mov     es,DS2PTR
           xor     di,di
           PUTW    '&/'                ; Tip emulatora
           if      VER_128
           PUTW    '1E'
           else
           PUTW    '0E'
           endif
           PUTB    ' '
           cmp     VIRTWARN,1
           jne     CFS1
           PUTW    '!/'                ; Opcija /!
           mov     al,WINTROUB
           or      al,al
           je      CFS0
           add     al,'0'
           stosb
CFS0:      PUTB    ' '
CFS1:      cmp     TESTFLG,1
           jne     CFS2
           PUTW    '?/'                ; Opcija /?128
           PUTW    '21'
           PUTW    ' 8'
CFS2:      if      VER_128
           cmp     PLUS2FLG,1
           jne     CFS3
           PUTW    '2/'                ; Opcija /2
           PUTB    ' '
           endif
CFS3:      cmp     AFLAG,1
           jne     CFS4
           PUTW    'A/'                ; Opcija /A
           PUTB    ' '
CFS4:      cmp     FBASIC,1
           jne     CFS5
           PUTW    'B/'                ; Opcija /B
           PUTB    ' '
CFS5:      cmp     INVCURS,0
           je      CFS61
           PUTW    'K/'                ; Opcija /K
           PUTB    ' '
CFS61:     cmp     NOESCAPE,0
           je      CFS7
           PUTW    '?/'                ; Opcija /?86
           PUTW    '86'
CFS7:      cmp     OPTIONE,1
           jne     CFS8
           PUTW    'E/'                ; Opcija /E
           cmp     ZXI1OLD,1
           jne     CFS7B
           PUTB    'O'
CFS7B:     cmp     EXPE,1
           jne     CFS7A
           PUTB    'X'
CFS7A:     PUTB    ' '
CFS8:      cmp     NAMJERNA,0
           je      CFS9
           PUTW    'V/'                ; Opcija /V
           mov     al,NAMJERNA
           mov     ah,' '
           lea     bx,CONVCARD
           xlat    CONVCARD
           stosw
CFS9:      cmp     KEMPDEV,0
           je      CFS10
           PUTW    'J/'                ; Opcija /J
           mov     al,KEMPDEV
           mov     ah,' '
           lea     bx,CONVKEMP
           xlat    CONVKEMP
           stosw
           cmp     MANCAL,1
           jne     CFS10
           lea     bx,LEFTBND
           mov     cx,4
CFS9A:     mov     ax,ss:[bx]
           mov     word ptr NUMBUF[2],'00'
           call    PUTNUM
           lea     si,NUMBUF[2]
           dec     di
           call    COPYZ
           add     bx,2
           loop    CFS9A
CFS10:     cmp     JTYPE,2
           jb      CFS11
           PUTW    'J/'
           mov     al,JTYPE
           mov     ah,' '
           lea     bx,CONVJOY
           xlat    CONVJOY
           stosw
CFS11:     cmp     PRTMODE,0
           je      CFS12
           PUTW    'P/'                ; Opcija /P
           mov     al,PRTMODE
           mov     ah,' '
           lea     bx,CONVPRT
           xlat    CONVPRT
           stosw
           nojumps
CFS12:     if      VER_128
           PUTW    'S/'                ; Opcija /S
           cmp     EXTRABEEP,1
           jne     CFS12N
           PUTB    'X'                 ; /SX, Extra Beeper
           PUTB    ' '
           jmp     short CFSATTRIB
CFS12N:    cmp     BLASTPRES,1         ; Ako nije Sound Blaster, postavi biper
           jne     CFSBEEP
           xor     bx,bx
           cmp     BLASTADDR,0
           jne     CFSADDR
           cmp     OPLCOUNT,1
           jne     CFSOLDADL
           PUTB    'A'                 ; /SA, samo ADLIB FM zvuk
           PUTB    ' '
           jmp     short CFSATTRIB
CFSOLDADL: PUTB    'O'                 ; /SO, stari ADLIB
           PUTB    ' '
           jmp     short CFSATTRIB
CFSADDR:   PUTB    'B'                 ; /SB, digitalna sinteza i MIDI
           mov     ax,BLASTADDR
           call    PUTNUM
           call    COPYZ
           dec     di
           PUTB    ';'
           mov     al,DMACH
           add     al,'0'
           mov     ah,' '
           stosw
           jmp     short CFSATTRIB
CFSBEEP:   mov     al,ACTMASK
           and     al,7
           add     al,'0'
           mov     ah,' '
           stosw
           endif
CFSATTRIB: cmp     ATTRMODE,0
           je      CFS12A
           PUTW    'M/'                ; Opcija /M
           mov     al,ATTRMODE
           add     al,'0'
           mov     ah,' '
           stosw
CFS12A:    mov     al,LOAD_MODE
           add     al,STOP_MODE
           or      al,al
           jz      CFS13A
           PUTW    'L/'                ; Opcija /L
           mov     al,LOAD_MODE
           cmp     al,3
           jb      CFS12B
           add     al,3
CFS12B:    add     al,'0'
           cmp     STOP_MODE,1
           jne     CFS13
           add     al,3
CFS13:     mov     ah,' '
           stosw
CFS13A:    cmp     ULAMODE,3
           je      CFS13B
           PUTW    'U/'                ; Opcija /U
           mov     al,ULAMODE
           cmp     al,3
           jbe     CFS13AA
           add     al,3
           shr     al,1
CFS13AA:   add     al,'0'
           mov     ah,' '
           stosw
CFS13B:    PUTW    'R/'                ; Opcija /R
           mov     al,SWAPDRV
           add     al,'A'
           mov     ah,':'
           stosw
           mov     si,DIRPTR
           call    COPYZ
           dec     di
           cmp     byte ptr es:[di-1],'\'
           je      CFS13C
           PUTB    '\'
CFS13C:    lea     si,SNPFILE
           call    COPYZ
           PUTW    CRLF
           cmp     CHGDRDIR,1
           jne     CFS14
           PUTW    'D/'                ; Opcija /D
           lea     si,DIRNAME
           call    COPYZ
CFS14:     PUTW    'T/'                ; Opcija /T
           lea     si,SEARCHREC[1EH]
           call    COPYZ
           PUTW    CRLF
           mov     cx,8
           xor     bx,bx
CFS15:     mov     ax,MHANDLES[bx]
           or      ax,ax
           jz      CFS16
           PUTW    '#/'                ; Opcija /#
           mov     dx,bx
           shr     dx,1
           mov     al,dl
           add     al,'1'
           mov     ah,';'
           stosw
           lea     si,CARTNAMES
           shl     dx,7
           add     si,dx
           call    COPYZ
           PUTW    CRLF
CFS16:     add     bx,2
           loop    CFS15
           cmp     NETPRES,1
           jne     CFS16A
           PUTW    '#/'                ; Opcija /#N
           PUTW    ';N'
           lea     si,NETFNAME
           call    COPYZ
           dec     di
           PUTB    ';'
           mov     ax,NETDELAY
           call    PUTNUM
           call    COPYZ
           dec     di
           PUTB    ';'
           mov     ax,BUSYLEN
           call    PUTNUM
           call    COPYZ
           dec     di
           PUTB    ';'
           mov     ax,FREELEN
           call    PUTNUM
           call    COPYZ
           PUTW    CRLF
CFS16A:    cmp     INITATTR,7
           je      CFS16AA
           PUTW    'Y/'                ; Opcija /Y
           mov     al,INITATTR
           xor     ah,ah
           call    PUTNUM
           call    COPYZ
CFS16AA:   cmp     MXSPD,1
           je      CFS16B
           cmp     SPEEDPRCT,100
           je      CFS17
           PUTW    '%/'                ; Opcija /%
           mov     ax,SPEEDPRCT
           call    PUTNUM
           call    COPYZ
           jmp     short CFS17
CFS16B:    PUTW    '%/'                ; Opcija /%MAX
           PUTW    'AM'
           PUTW    ' X'
CFS17:     cmp     MAXLOAD,1
           jne     CFS17B
           PUTW    '%/'                ; Opcija /%LMAX
           PUTW    'ML'
           PUTW    'XA'
           PUTB    ' '
CFS17B:    cmp     CURBLK,1
           je      CFS17AA
           PUTW    'F/'                ; Opcija /F
           mov     ax,CURBLK
           call    PUTNUM
           call    COPYZ
CFS17AA:   cmp     VIDSYNC,150
           je      CFS17A
           PUTW    'Q/'                ; Opcija /Q
           mov     ax,VIDSYNC
           cmp     ax,-1
           je      CFS17BB
           call    PUTNUM
           call    COPYZ
           jmp     short CFS17A
CFS17BB:   PUTB    'X'                 ; Opcija /QX
           mov     al,EXTRADELAY
           or      al,EXTRAKUR
           jz      CFS17A
           mov     al,EXTRADELAY
           or      al,al
           jns     CFS17C
           push    ax
           PUTB    '-'
           pop     ax
           neg     al
CFS17C:    xor     ah,ah
           call    PUTNUM
           call    COPYZ
           mov     al,EXTRAKUR
           or      al,al
           jz      CFS17A
           dec     di
           push    ax
           PUTB    ';'
           pop     ax
           xor     ah,ah
           call    PUTNUM
           call    COPYZ
CFS17A:    cmp     ENVPPTR,0
           je      CFS18
           PUTW    CRLF                ; Opcije /&
           lea     si,ENVPBUF
           call    COPYZ
           dec     di
CFS18:     PUTW    0A0DH
           mov     ax,3C02H            ; Otvori datoteku
           xor     cx,cx
           lea     dx,CFFILE
           int     21H
           jc      CSAVEERR
           mov     bx,ax
           mov     ds,DS2PTR
           xor     dx,dx
           mov     ah,40H              ; Snimi je
           mov     cx,di
           int     21H
           cmp     ax,cx
           jne     CSAVEERR
           mov     ah,3EH
           int     21H
           jc      CSAVEERR
           ret
CSAVEERR:  ret
CONFSAVE   endp

;
; Potprogram za pretvaranje broja u ASCII format. Koristi se za CONFSAVE.
;

PUTNUM:    lea     si,NUMBUF[5]
           mov     bp,10
PNLOOP:    dec     si
           xor     dx,dx
           div     bp
           add     dl,'0'
           mov     ss:[si],dl
           or      ax,ax
           jnz     PNLOOP
           ret

COPYZ:     lodsb                       ; Kopira ASCIIZ string
           or      al,al
           jz      COPYZEND
           stosb
           jmp     COPYZ
COPYZEND:  PUTB    ' '
           ret

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
           jz      NOSHOOT             ;  => AL=255
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
           cmp     al,' '              ;  autokalibraciju
           je      AUTOCAL
           cmp     al,13
           je      AUTOCAL
           mov     MANCAL,1
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
; Procedura SAVESNAP snima snapshoot u Lunterovom .Z80 formatu
;

           assume  ds:ZXEXPBUF

SAVESNAP   proc    far
           pusha
           push    ds
           mov     ds,SNAPSEG
           mov     SNAPA,A             ; Smjesti sve snapshoot registre
           mov     SNAPBC,BC
           mov     SNAPHL,HL
           mov     SNAPDE,DE
           mov     SNAPPCOLD,0
           mov     SNAPSP,STP
           mov     SNAPPC,PC
           mov     ax,IX
           mov     SNAPIX,ax
           mov     ax,IY
           mov     SNAPIY,ax
           mov     al,I
           mov     SNAPI,al
           mov     al,R
           mov     SNAPR,al
           mov     ax,ALTBC
           mov     SNAPBCP,ax
           mov     ax,ALTDE
           mov     SNAPDEP,ax
           mov     ax,ALTHL
           mov     SNAPHLP,ax
           mov     al,ALTA
           mov     SNAPAP,al
           mov     al,IFF1
           shr     al,2
           mov     SNAPIFF1,al
           mov     al,IFF2
           shr     al,2
           mov     SNAPIFF2,al
           mov     al,IMODE
           mov     SNAPIM,al
           mov     SNAPADD,23
           mov     SNAPRLDIR,3         ; Aktivni i R i LDIR
           mov     ax,LASTB            ; Border
           shl     ah,1
           and     ah,15
           test    R7,80H
           jnz     SETSTTS
           or      ah,1
SETSTTS:   mov     SNAPSTTS,ah
           test    PV,255
           jnz     SNF_L1
           mov     ah,FLAGSL
           and     ah,0D5H
           or      ah,N
           jmp     short SNF
SNF_L1:    mov     bx,FLAGS
           and     bx,08F1H
           or      bl,N
           shr     bh,1
           or      bl,bh
           mov     ah,bl
SNF:       mov     SNAPF,ah
           mov     cx,ALTPVN
           test    ch,255
           jnz     SNFP_L1
           mov     ax,ALTF
           mov     ah,al
           and     ah,0D5H
           or      ah,cl
           jmp     short SNFP
SNFP_L1:   mov     bx,ALTF
           and     bx,08F1H
           or      bl,cl
           shr     bh,1
           or      bl,bh
           mov     ah,bl
SNFP:      mov     SNAPFP,ah
           mov     ah,OPTIONE
           if      VER_128
           add     ah,3
           endif
           mov     SNAPHWMOD,ah
           mov     SNAPIN1ON,0
           cmp     SHADOW,1
           jne     SN128
           mov     SNAPIN1ON,255
SN128:     if      VER_128             ; Za 128 verziju ima dodatnih podataka
           mov     ah,MMUSTATE
           mov     SNAP7FFD,ah
           mov     ah,AY_REG
           mov     SNAPFFFD,ah
           mov     cx,16
           xor     si,si
           xor     di,di
SNSNDLOOP: mov     ah,AY_REGS[si]
           mov     SNAPSOREG[di],ah
           inc     si
           inc     di
           loop    SNSNDLOOP
           endif
           mov     ax,3C02H            ; Otvori snapshot fajl
           xor     cx,cx
           mov     ds,STEKSEG
           lea     dx,SNPFILE
           int     21H
           jc      SNERR               ; U slucaju greske ignorisi
           mov     SNPHANDLE,ax
           mov     bx,ax
           mov     ds,SNAPSEG
           lea     dx,SNAPA
           mov     ah,40H
           mov     cx,55
           int     21H
           if      VER_128             ; Na verziji 128 snimi sve strane od
           mov     cx,8                ;  4 do 10 (tj. RAM 0 - RAM 7)
           mov     ah,MMUSTATE
           mov     MMUTEMP,ah
           and     MMUSTATE,31
           mov     ax,3
SNMEMLOOP: mov     bx,49152
           mov     MAX,0
           pop     ds                  ; Pozvati MMU iz drugog segmenta. Postoje
           call    far ptr SNAPMMU     ;  mali problemi sa prenosom segmentnih
           push    ds                  ;  registara
           mov     ds,SNAPSEG
           pusha
           call    SNAPCOMP
           call    SNSBLK
           popa
           jc      SNERR
           add     ax,257
           loop    SNMEMLOOP
           pop     ds
           mov     ah,MMUTEMP
           call    far ptr SNAPMMU
           push    ds
           else
           mov     bx,32768            ; Snimi strane 4,5 i 8 za 48 rezim
           mov     MAX,49152
           mov     al,4
           call    SNAPCOMP
           call    SNSBLK
           jc      SNERR
           mov     bx,49152
           mov     MAX,0
           mov     al,5
           call    SNAPCOMP
           call    SNSBLK
           jc      SNERR
           mov     bx,16384
           mov     MAX,32768
           mov     al,8
           call    SNAPCOMP
           call    SNSBLK
           jc      SNERR
           endif                       ; Zatvori fajl
           mov     ah,3EH
           mov     bx,SNPHANDLE
           int     21H
SNRET:     pop     ds
           popa
           ret
SNERR:     push    cs
           call    near ptr BUZZ
           pop     ds
           popa
           ret
SAVESNAP   endp

REPEATING: cmp     byte ptr [bx-1],237 ; Ovaj podprogram upisuje ponavljajuce
           je      SOUTMORE            ;  sekvence kao ED ED nn vv ako su
           cmp     cx,5                ;  duzine 5 ili vise bajtova ili ako su
           jae     SOUTMORE            ;  od 2 ili vise ED bajtova
SOUTLOOP:  mov     al,[bx-1]
           stosb
           loop    SOUTLOOP
           ret
SOUTMORE:  mov     al,0EDH
           stosb
           mov     al,0EDH
           stosb
           mov     al,cl
           stosb
           mov     al,[bx-1]
           stosb
           ret

;
; Snapshoot kompresija: BX=pocetna adresa, MAX=maksimalna adresa, AL broj bloka
;

           assume  ds:ZXDSEG,es:ZXEXPBUF

SNAPCOMP:  push    ds
           push    es
           cld
           mov     ds,MEMSEG
           mov     es,SNAPSEG
           mov     SNAPPAGE,al
           lea     di,SNAPDATA
           inc     bx                  ; Krece od prvog, a ne nultog
SMAIN:     mov     al,[bx-1]           ; Poredi trenutni i prethodni
           cmp     al,[bx]
           je      SREPEAT             ; Ako su isti, ovo je ponavljanje
           cmp     byte ptr [bx-1],237 ; Bajt iza ED obavezno upisi
           je      SNORPTED
           mov     al,[bx-1]           ; Upisi bajt
           stosb
           inc     bx
ENDSMAIN:  cmp     bx,MAX
           jne     SMAIN
SLASTBYT:  mov     al,[bx-1]
           stosb
           jmp     short SNAPEND        ; Ovo je za slucaje ED xx
SNORPTED:  mov     al,[bx-1]
           stosb
           inc     bx
           cmp     bx,MAX
           je      SLASTBYT
           mov     al,[bx-1]
           stosb
           inc     bx
           jmp     ENDSMAIN
SREPEAT:   mov     cx,2                ; Zadnji i predzadnji isti
SRPTLOOP:  inc     bx                  ; Startuj brojac
           cmp     bx,MAX
           je      SLASTRPT
           cmp     cx,255              ; Brojac mora biti 8 bitni
           je      SRPTSHORT
           mov     al,[bx-1]
           cmp     al,[bx]
           jne     SRPTSHORT
           inc     cx                  ; Uvecavaj brojac
           jmp     SRPTLOOP
SRPTSHORT: call    REPEATING           ; Upisi sekvencu i novi bajt
           inc     bx
           jmp     ENDSMAIN
SLASTRPT:  call    REPEATING
SNAPEND:   sub     di,offset SNAPDATA
           mov     SNAPDTLEN,di
           pop     es
           pop     ds
           ret

;
; Potprogram koji snima blok snapshot fajla na disk.
;

           assume  ds:ZXEXPBUF,es:nothing

SNSBLK:    push    ds
           mov     bx,SNPHANDLE
           mov     ds,SNAPSEG
           mov     cx,SNAPDTLEN
           add     cx,3
           lea     dx,SNAPDTLEN
           mov     ah,40H
           int     21H
           pushf
           cmp     ax,cx
           jne     SNSBER
           popf
           pop     ds
           ret
SNSBER:    popf
           pop     ds
           stc
           ret

;
; Obnavljanje stanja iz snapshot fajla.
;

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
           mov     ax,3D00H            ; Otvori datoteku
           lea     dx,RESTFNAME
           mov     ds,STEKSEG
           int     21H
           jumps
           jc      NOSNERR
           mov     SNPHANDLE,ax
           mov     bx,ax
           mov     ah,3FH
           mov     cx,30
           lea     dx,SNAPA
           mov     ds,SNAPSEG
           int     21H
           jc      UNSNAPERR
           cmp     cx,ax
           jne     UNSNAPERR
           cmp     SNAPPCOLD,0
           jne     SNAPOLDVER          ; Snapshot stara verzija
           mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,25
           lea     dx,SNAPADD
           int     21H
           jc      UNSNAPERR
           cmp     cx,ax
           jne     UNSNAPERR
           cmp     SNAPADD,23
           je      short SNAPGOOD
           mov     LUNTV30,1
           cmp     SNAPADD,54
           je      short SNAPGOOD
           jmp     UNSNAPERR
SNAPGOOD:  if      VER_128             ; Provjera da li se ucitava u ispravnu
           cmp     SNAPHWMOD,2         ;  verziju hardvera (48 verzija se moze
           je      HRDWERR             ;  ucitati u 128 verziju)
           cmp     LUNTV30,1
           je      short V30TESTS
           cmp     SNAPHWMOD,4
           ja      HRDWERR
           jmp     short LSNAPCONT
V30TESTS:  cmp     SNAPHWMOD,3
           je      HRDWERR
           cmp     SNAPHWMOD,5
           ja      HRDWERR
           else
           cmp     SNAPHWMOD,1
           ja      HRDWERR
           endif
LSNAPCONT: cmp     LUNTV30,1
           jne     short LSNAPLOOP
           mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,31
           lea     dx,SNAPDTLEN
           int     21H
           jc      UNSNAPERR
           cmp     ax,cx
           jne     UNSNAPERR
LSNAPLOOP: mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,3
           lea     dx,SNAPDTLEN
           int     21H
           ife     VER_128
           nojumps
           endif
           jc      UNSNAPERR
           or      ax,ax
           jz      ENDLSNAP
           cmp     cx,ax
           jne     UNSNAPERR
           mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,SNAPDTLEN
           lea     dx,SNAPDATA
           int     21H
           jc      UNSNAPERR
           cmp     cx,ax
           jne     UNSNAPERR
           nojumps
           lea     bx,SNAPDATA
           mov     MAX,bx
           mov     cx,SNAPDTLEN
           add     MAX,cx
           if      VER_128
           cmp     SNAPHWMOD,2         ; 48 snapshot u 128 verziji?
           jb      L48IN128
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
L48IN128:  mov     SNAP7FFD,48         ; Ucitavanje 48 snapshota u 128 emulator
           mov     SNAPSOREG[7],255
           endif
           mov     di,32768            ; Na 48 verziji legalne su samo strane
           cmp     SNAPPAGE,4          ;  4, 5 i 8
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
ENDLSNAP:  mov     ah,3EH
           mov     bx,SNPHANDLE
           int     21H
           jmp     short LSNAPRET
NOSNERR:   mov     SNAPERR,3
           jmp     short LSNAPRET
HRDWERR:   mov     SNAPERR,2
           jmp     short LSNAPRET
UNSNAPERR: mov     SNAPERR,1
LSNAPRET:  pop     es
           pop     ds
           popa
           ret

           assume  ds:ZXDSEG,es:ZXEXPBUF

SNAPOLDVER:push    es
           mov     es,SNAPSEG
           if      VER_128
           mov     SNAP7FFD,48         ; Na 128 verziji blokiraj MMU i zvuk kad
           mov     SNAPSOREG[7],255    ;  se ucitava 48 snapshot
           endif
           mov     SNAPHWMOD,0
           mov     SNAPIN1ON,0
           mov     SNAPRLDIR,0
           mov     ax,SNAPPCOLD
           mov     SNAPPC,ax
           test    SNAPSTTS,20H
           pop     es
           jnz     SNOLDCOMP
           mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,49152
           mov     dx,16384
           mov     ds,MEMSEG
           int     21H
           jc      UNSNAPERR
           cmp     cx,ax
           jne     UNSNAPERR
           jmp     ENDLSNAP
SNOLDCOMP: mov     ah,3FH
           mov     bx,SNPHANDLE
           mov     cx,65000
           lea     dx,SNAPDATA
           mov     ds,SNAPSEG
           int     21H
           jc      UNSNAPERR
           mov     es,SNAPSEG
           mov     MAX,65535
           mov     di,16384
           lea     bx,SNAPDATA
           call    SNAPDCOMP
           jmp     ENDLSNAP
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
           cmp     byte ptr [bx],237
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

;
; SETSNNAME postavlja ime snapshoot fajla.
;

           assume  ds:ZXDSEG,es:nothing

SETSNNAME  proc    far
           push    es
           xor     cx,cx
           mov     es,STEKSEG
           lea     di,RESTFNAME
SETSNNMLP: inc     bx
           mov     al,[bx]
           cmp     al,13
           je      SNCHKEXT
           cmp     al,' '
           je      SNCHKEXT
           cmp     al,'.'
           jne     SNNMPUT
           inc     cl                  ; Ime ima ekstenziju
SNNMPUT:   stosb
           inc     ch                  ; Prisutno je ime
           jmp     SETSNNMLP
SNCHKEXT:  or      cl,cl               ; Ima li ime ekstenziju ?
           jnz     CHECKEMPT
           mov     ax,'Z.'             ; Ako nema, podrazumijevaj .Z80
           stosw
           mov     ax,'08'
           stosw
CHECKEMPT: or      ch,ch               ; Da li je prisutno ime ?
           jne     SETSNEND
           inc     ch                  ; Ako nema, podrazumijevaj SNAP.Z80
           lea     di,RESTFNAME
           mov     ax,'NS'
           stosw
           mov     ax,'PA'
           stosw
           jmp     SNCHKEXT
SETSNEND:  dec     bx
           pop     es
           ret
SETSNNAME  endp

;
; Sada zapocinju rutine za inicijalizaciju konverzionih tabela za razne kartice.
; Prvo slijedi nekoliko opstih potprograma.
;

ZX_COORDS: mov     dx,cx               ; Pronalazi koordinate karaktera u
           dec     dx                  ;  Spectrumovoj video memoriji
           mov     ah,dl
           and     ah,1FH              ; AH <- X Spectruma
           shr     dx,5                ; DL <- ADR shr 5 (DH nebitan)
           mov     dh,dl               ; Kopija u DH i u AL
           mov     al,dl
           and     dx,38C0H            ; DL <- ADR and $C0, DH <- ADR and $38
           shr     dh,3                ; DH <- (ADR and $38) div 8
           or      dl,dh               ; DL <- (ADR and $C0) +
                                       ;  (ADR and $38) div 8
           and     al,07H              ; AL <- ADR and $0007
           shl     al,3                ; AL <- (ADR and $0007)*8
           or      dl,al               ; Y spectruma
           mov     al,dl               ; AL <- Y spectruma
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
           mov     bl,dl               ;  osnovu (X,Y) koordinata karaktera
           and     bl,11111000B
           shl     bx,2
           add     bx,22528
           add     bl,ah
           adc     bh,0                ; Adresa atributa
           mov     ADR_A[di],bx
           sub     di,2
           ret

VE_KERNEL: call    ZX_COORDS           ; Nalazenje adrese u tablici za EGAMONO,
           push    ax                  ;  EGA i VGA kartice (jezgro)
           mov     bl,80
           mul     bl                  ; AX <- 80*Y
           mov     bx,ax               ; Sad je 80*Y u BX
           pop     ax                  ; Vrati AX
           mov     al,ah               ; AL <- X
           shl     al,1                ; AL <- 2*X
           add     bl,al
           adc     bh,0                ; Adresa u EGA i VGA kartici
           add     bx,VIDEOFFS         ; Centriranje
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
           shr     al,2                ; AL <- Y div 4
           push    ax
           mov     bl,90
           mul     bl                  ; AX <- 90*(Y div 4)
           mov     bx,ax               ; Sad je to u BX
           pop     ax                  ; Vrati ax
           mov     al,dl               ; AL <- Y
           and     al,03H              ; AL <- (Y mod 4)
           ror     al,3                ; AL <- visi bajt od 8192*(Y mod 4)
           or      bh,al               ; BX <- adresa bez X
           mov     al,ah               ; AL <- X
           shl     al,1                ; AL <- 2*X
           add     bl,al
           adc     bh,0                ; Adresa u Hercules kartici
           add     bx,1363             ; Centriranje, 13+90*(60/4)=1363
           mov     ADR_DF[di],bx
           call    VMTOATTR
           loop    HLOOP1
INIT_H1:   mov     cx,768              ; Inicijalizacija adresa karaktera za
           lea     di,ATTR+767*2       ;  atribute
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
           shr     al,1                ; AL <- Y div 2, CF <- Y mod 2
           pushf
           push    ax
           mov     bl,80
           mul     bl                  ; AX <- 80*(Y div 2)
           mov     bx,ax               ; Sad je to u BX
           pop     ax                  ; Vrati AX i CF
           popf
           jnc     CCONT
           add     bx,8192             ; Dodaj 8192 ako je CF=1
CCONT:     mov     al,ah               ; AL <- X
           shl     al,1                ; AL <- 2*X
           add     bl,al
           adc     bh,0                ; Adresa u CGA
           add     bx,168              ; Centriranje
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
           lea     di,ATTR+767*2       ;  EGA i VGA karticama
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
           mov     cx,192              ; Inicijalizacija tabela za MultiColor
           lea     di,ADR_A
VLOOP21:   mov     bx,192
           sub     bx,cx
           mov     si,bx
           shl     si,2
           and     si,1111111111100000B
           add     si,22528
           mov     [di],si
           add     si,si
           mov     si,ATTR[si-45056]
           mov     ax,bx
           and     ax,7
           imul    ax,ax,80
           add     ax,si
           mov     [di+384],ax
           mov     al,bl
           and     al,7
           mov     bh,bl
           and     bh,192
           ror     bh,3
           or      bh,al
           or      bh,64
           rol     bl,2
           and     bl,11100000B
           mov     [di+768],bx
           add     di,2
           loop    VLOOP21
           mov     di,255*2            ; Inicijalizacija opisa specijalnih
           mov     cx,256              ; karaktera na EGA i VGA
VLOOP3:    mov     ax,cx
           dec     ax
           mov     ah,al
           mov     MASKA[di],ax
           sub     di,2
           loop    VLOOP3
           jmp     HINKPAP
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
           shr     al,1                ; AL <- Y div 2, CF <- Y mod 2
           pushf
           push    ax
           mov     bl,84
           mul     bl                  ; AX <- 84*(Y div 2)
           mov     bx,ax               ; Sad je to u BX
           pop     ax                  ; Vrati AX i CF
           popf
           jnc     XCONT
           add     bx,16384            ; Dodaj 16384 ako je CF=1
XCONT:     mov     al,ah               ; AL <- X
           shl     al,1                ; AL <- 2*X
           add     bl,al
           adc     bh,0                ; Adresa u Hercules katrici
           add     bx,10               ; Centriranje
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

;
; Aktiviranje/deaktiviranje zvuka, daleki poziv.
;

SETSNDFAR  proc    far
           push    es
           xor     ULAMODE,2
           if      VER_128
           test    ULAMODE,2
           jnz     NOSTOPSB
           push    cs
           call    near ptr STOPADLIB
           jmp     short SKIPADLB
NOSTOPSB:  push    cs
           call    near ptr INITADLIB
SKIPADLB:  xor     STOPAY,1
           endif
           mov     es,PROGSEG
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
           pop     es
           ret
SETSNDFAR  endp

;
; Aktiviranje/deaktiviranje BORDER-a, daleki poziv. Ova rutina je sada jako
; zakomplikovana, jer se na VGA vrsi i prebacivanje na HiCol BORDER.
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

SETBORDRF  proc    far                 ; Shift+F6 blokira generisanje slike
           pusha
           push    cs
           call    near ptr KLIK
           test    KBDROWS[0],1
           jz      BLOCKPIC
           mov     ax,OLDVIDSYNC
           mov     VIDSYNC,ax
           call    SETBRDFAR
SETBEND:   call    far ptr REPAINT
           popa
           ret
BLOCKPIC:  cmp     VIDSYNC,-2
           je      RESTPIC
           mov     ax,VIDSYNC
           mov     OLDVIDS2,ax
           mov     VIDSYNC,-2
           call    DISPDIS
           popa
           ret
RESTPIC:   mov     ax,OLDVIDS2
           mov     VIDSYNC,ax
           jmp     SETBEND
SETBORDRF  endp

;
; Rutina DEFDD vraca default disk i direktorijum.
;

DEFDD      proc    far
           push    ds
           mov     ah,3BH              ; Obnavlja default direktorij
           lea     dx,DEFDIR
           mov     DIRPTR,dx
           mov     ds,STEKSEG
           int     21H
           mov     ah,0EH              ; Obnavlja default drajv
           mov     dl,DEFDRIVE
           mov     SWAPDRV,dl
           int     21H
           pop     ds
           ret
DEFDD      endp

;
; Rutina za postavljanje swap drajva i direktorija.
;

SWAPDD     proc    far
           cmp     WARVARBUF,2         ; Swap direktorij postaje default
           jae     EXISTSWAP           ;  direktorij ako nije zadan u varijabli
           jmp     DEFDD
EXISTSWAP: push    ds
           mov     ds,STEKSEG
           lea     dx,WARVARBUF[1]
SWAPDDBCK: mov     di,dx
           mov     al,[di+1]
           cmp     al,' '
           jne     NOSKIPGRF
           add     dx,2
           jmp     SWAPDDBCK
NOSKIPGRF: cmp     al,':'
           jne     DOSWAPDIR
           mov     ah,0EH
           push    dx
           mov     dl,[di]             ; Postavka drajva
           or      dl,32
           sub     dl,'a'
           mov     SWAPDRV,dl
           int     21H
           pop     dx
           add     dx,2
DOSWAPDIR: mov     DIRPTR,dx
           mov     ah,3BH              ; Postavka direktorija
           int     21H
           pop     ds
           ret
SWAPDD     endp

;
; Ucitavanje konfiguracione datoteke.
;

CONFFILE   proc    far
           call    SWAPDD              ; Prebaci se u swap direktorij
           mov     ds,STEKSEG
           mov     ax,3D00H            ; Open file
           lea     dx,CFFILE
           int     21H
           jc      CFERROR
           mov     bx,ax
           mov     dx,81H
           mov     ds,DS2PTR
CONFLOOP:  mov     ah,3FH              ; Read bytes
           mov     cx,1
           int     21H
           jc      CFERROR
           mov     si,dx
           cmp     byte ptr [si],13
           jne     NOCONFCR
           mov     byte ptr [si],' '   ; Zamijeni CR LF sa ' '
           mov     ah,3FH
           push    dx                  ; Preskoci LF bajt
           xor     dx,dx
           mov     cx,1
           int     21H
           pop     dx
NOCONFCR:  inc     dx
           cmp     dx,60000
           ja      CFERROR
           or      ax,ax
           jnz     CONFLOOP
           mov     si,dx
           mov     byte ptr [si-1],13
           mov     ah,3EH              ; Close file
           int     21H
           jc      CFERROR
           call    DEFDD
           ret
CFERROR:   call    CLS
           call    DEFDD
           lea     dx,CFERRMS
           mov     ds,STEKSEG
           mov     ah,09H
           int     21H
           jmp     far ptr ILLEND
CONFFILE   endp

;
; Detekcija graficke kartice, izlaz u AX (0 ako nije detektovana).
;
; CGA=1, MCGA=2, EGA=3, EGAMONO=5, HERCULES=7, VGA=9, PC_3270=10
; (Forsirano se koriste WINCGA=6, EXTHERC=8 ali se ne detektuju)
;

DETECTCRD  proc    far
           mov     ah,0FH
           int     10H                 ; BIOS Get video state, AL - mode
           cmp     al,7                ; Da li je mono mod?
           je      MONO_CARD
           call    TEST_EGA            ; Provjera EGA BIOS
           jnc     NO_EGA
MAYBE_EGA: cmp     bh,1                ; BIOS detektovao mono
           mov     ax,5
           je      EGA_END             ; Ovo je EGAMONO
           call    TEST_VGA            ; Da nije mozda VGA?
           jc      VGA_CARD
           mov     bx,0C000H           ; Provjeri signaturu BIOS-a kao
           mov     es,bx               ;  alternativu za VGA
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
           int     10H                 ;  PC 3270 BIOS poziv
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
MONO_CARD: call    TEST_EGA            ; Da nije EGAMono?
           jc      MAYBE_EGA
           mov     dx,3BAH             ; Provjeriti Hercules
           xor     bl,bl
           in      al,dx               ; port 3BAH, MDA/EGA video status
           and     al,80H
           mov     ah,al
           mov     cx,8000H
TH_LOOP:   in      al,dx
           and     al,80H              ; Provjeriti da li nastupa
           cmp     al,ah               ;  "vertical scan retrace"
           je      NO_VSRTC
           inc     bl
           cmp     bl,0AH
           jae     HERC_CARD
NO_VSRTC:  loop    TH_LOOP
           jmp     short NO_HERC
HERC_CARD: mov     ax,7                ; Ovo je HERCULES
           ret
NO_HERC:   mov     si,0B800H           ; Posljednja nada: probava se smjestiti
           mov     es,si               ;  broj u video memoriju
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

TEST_EGA:  mov     ax,1200H            ; Testiranje da li EGA BIOS vraca
           mov     bl,10H              ;  pravilne vrijednosti
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
; Glavnina rutine za RS232 simulaciju zajednicka je za Interface 1 i RS 128, a
; prilagodjena je ulazu za 128 varijantu interfejsa.
;

RS_OUT     proc    far
           cmp     byte ptr ss:[AUX+1],9 ; Provjeri da li je vec dosao start
           jnz     RI1_GET               ;  bit
           test    ah,8
           jnz     RI1_END
           dec     byte ptr ss:[AUX+1]   ; Detektovan start bit
           mov     byte ptr ss:[AUX+2],0
           ret
RI1_GET:   test    ah,8
           jz      RI1_ZERO
           stc
RI1_ZERO:  rcr     byte ptr ss:[AUX],1   ; Prikupljaj bitove jedan po jedan
           dec     byte ptr ss:[AUX+1]
           jnz     RI1_END
           mov     byte ptr ss:[AUX+1],9
           push    ax                    ; Kad prikupis 8 bitova, posalji ih
           push    dx
           push    cs
           call    near ptr RESTTIMER
           xor     dx,dx
           mov     ah,dl
           mov     al,ss:[AUX]
           test    WINTROUB,2
           jnz     SKIPRS
           int     17H
SKIPRS:    push    cs
           call    near ptr SETTIMER
           pop     dx
           pop     ax
RI1_END:   ret
RS_OUT     endp

;
; Slicna stvar je i sa citanjem.
;

RS_IN      proc    far
           push    dx
           push    ds
           mov     dx,0040H            ; Segment BIOS sistemskih varijabli
           mov     ds,dx
           mov     dx,ds:[0008H]       ; Adresa baznog porta za LPT1
           or      dx,dx               ; Ako je adresa 0, LPT1 nije instaliran
           jz      RSIE                ;  i izadji sa AH = 0FFH
           inc     dx
           mov     ah,al
           in      al,dx               ; Citaj status printera (LPT1)
           in      al,dx
           xor     al,0D0H             ; Invertuj b7, b6 i b4
           and     al,0F0H
           mov     al,ah
           mov     ah,-1
           jnz     RSIE
           mov     ah,191
RSIE:      pop     ds
           pop     dx
           ret
RS_IN      endp

;
; Simulacija AY-3-8912 zvucnog cipa. On takodje kontrolise RS232, KEYPAD i MIDI.
;

           if      VER_128

AY_OUT     proc    far                 ; Postavljanje AY registara
           mov     AUX,word ptr AY_REG
           and     AUX,15
           push    ax
           and     ah,AY_MASK[AUX]
           mov     AY_REGS[AUX],ah
           add     AUX,AUX
           cmp     BLASTADDR,0
           jne     AY_OUT_SB
           cmp     EXTRABEEP,1
           je      AY_OUT_SB
           call    AY_ACTS[AUX]
           pop     ax
           ret
AY_OUT_SB: call    AY_ACTS_SB[AUX]     ; Za Sound Blaster sa DMA i ExtraBeep su
           pop     ax                  ;  drugacije akcije
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
; Rutina koja zaustavlja AdLib ili SoundBlaster.
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
           cmp     BLASTADDR,0         ; Iskljuci DMA na Sound Blasteru
           je      ENDOFSTB
           call    near ptr DIG_KILL
ENDOFSTB:  ret
STOPADLIB  endp

;
; Rutina koja inicijalizira AdLib ili SoundBlaster
;

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
           cmp     BLASTADDR,0         ; Pripremi DMA na Sound Blasteru
           je      ENDOFSTB
           call    near ptr DIG_SETUP
ENDOFINA:  ret
INITADLIB  endp

;
; SoundBlaster/Adlib podprogram za upis zvucnih registara
; AH=registar, AL=vrijednost
;

SETSBREGS  proc    far
           push    cx
           push    dx
           push    ax
           mov     dx,388H
           xchg    ah,al
           out     dx,al               ; Upisi registar, i sacekaj da bude
           rept    6                   ;  spreman da primi vrijednost
           in      al,dx
           endm
           xchg    ah,al
           inc     dx
           out     dx,al
           dec     dx
           mov     cx,OPLCOUNT
OPLLOOP:   in      al,dx
           loop    OPLLOOP
           pop     ax
           pop     dx
           pop     cx
           ret
SETSBREGS  endp

;
; Specijalne akcije za simulaciju AY cipa.
;

SETVA:     cmp     ah,16               ; Postavljanja glasnoce. 16 znaci da se
           jb      STVA1               ;  glasnoca uzima iz generatora envelope
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
           mov     ah,CURVOL           ;  envelopu
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
SEL1:      cmp     ax,59440            ;  envelopu
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
DSND:      cmp     EXTRABEEP,1         ; Ako je ExtraBeep ovdje ne treba nista
           jne     DSND1
           ret
DSND1:     cmp     BLASTPRES,1         ; Da li ima AdLib ili SB kartice?
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
           cmp     BLASTADDR,0
           jz      DSNDFM
           push    cs
           call    near ptr AY_MIX
           jmp     FINISH
DSNDFM:    mov     ch,AY_REGS[7]       ; FM sinteza ako nije Sound Blaster
           xor     cl,cl
           shr     ch,1
           jnc     PLAYTONA
           call    NOACTIVE
           jmp     short TONEB
PLAYTONA:  call    ADFREQ
           mov     bl,VOLA
           and     bx,000FH
           mov     al,DECIBEL[bx]
           mov     ah,43H
           call    SETSBREGS
TONEB:     inc     cl
           shr     ch,1
           jnc     PLAYTONB
           call    NOACTIVE
           jmp     short TONEC
PLAYTONB:  call    ADFREQ
           mov     bl,VOLB
           and     bx,000FH
           mov     al,DECIBEL[bx]
           mov     ah,44H
           call    SETSBREGS
TONEC:     inc     cl
           shr     ch,1
           jnc     PLAYTONC
           call    NOACTIVE
           jmp     short ADNOISE
PLAYTONC:  call    ADFREQ
           mov     bl,VOLC
           and     bx,000FH
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
SETHIGH:   xor     dx,dx
           push    cx
           div     bx
           pop     cx
           mov     dl,28
           jmp     short ADSETTING

NOACTIVE:  mov     dl,0
           jmp     short ADSETFREQ
ADSETTING: or      dl,ah
           or      dl,20H
ADSETFREQ: mov     ah,0A0H
           add     ah,cl
           call    SETSBREGS
           add     ah,10H
           mov     al,dl
           call    SETSBREGS
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
           add     ENVCNT,2217         ;  glasnoce kanala?
           mov     ax,ENVLEN
           cmp     ENVCNT,ax
           ja      ENVLOOP
           pop     ax
           ret
ENVLOOP:   mov     ax,ENVLEN           ; Nadoknadi "izgubljeno" ako je izmedju
           sub     ENVCNT,ax           ;  2 interapta trebalo izvrsiti promjenu
           jc      ENVEND              ;  za vise od jednog koraka
           call    ENVKERNEL
           jmp     ENVLOOP
ENVEND:    add     ENVCNT,ax           ; Azuriraj glasnoce kanala koji imaju
           test    AY_REGS[8],16       ;  aktivnu envelopu
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
           xor     FINVOL,15
           ret
;
; Simulacija MIDI interfejsa
;

MIDI_OUT   proc    far
           cmp     MIDI_C,9              ; Provjeri da li je vec dosao start
           jnz     MIDI_GET              ;  bit
           test    ah,4
           jnz     MIDI_END
           dec     MIDI_C                ; Detektovan start bit
           mov     MIDI_T,0
           ret
MIDI_GET:  test    ah,4
           jz      MIDI_ZERO
           stc
MIDI_ZERO: rcr     MIDI_B,1              ; Prikupljaj bitove jedan po jedan
           dec     MIDI_C
           jnz     MIDI_END
           mov     MIDI_C,9
           cli
           push    ax                    ; Kad prikupis 8 bitova, posalji ih
           push    dx
           push    cx
           xor     cx,cx
           mov     dx,BLASTADDR
           add     dx,0CH
MIDILP1:   dec     cx                  ; Ovo sprecava blokiranje emulatora
           jz      ENDMIDI             ;  ako MIDI ne radi
           in      al,dx
           and     al,80H
           jnz     MIDILP1
           mov     al,3BH              ; Midi Write pool
           out     dx,al
MIDILP2:   dec     cx
           jz      ENDMIDI
           in      al,dx
           and     al,80H
           jnz     MIDILP2
           mov     al,MIDI_B
           out     dx,al
ENDMIDI:   pop     cx
           pop     dx
           pop     ax
           sti
MIDI_END:  ret
MIDI_OUT   endp

           endif

;
; Priprema registara za simulaciju digitalnog zvuka.
;

DSP_W:    push     ax
          push     cx
          mov      dx,BLASTADDR
          add      dx,0CH
          xor      cx,cx
DSP_BSY:  dec      cx
          jcxz     DSP_BAD
          in       al,dx
          or       al,al
          js       DSP_BSY
          pop      cx
          pop      ax
          out      dx,al
          ret
DSP_BAD:  mov      DEBUGZONE,1
          pop      cx
          pop      ax
          ret

          if       VER_128

DIG_SETUP proc     near
          pusha
          mov      ax,offset DIGBYTE   ; Izracunaj linearnu adresu jednobajtnog
          mov      dx,seg DIGBYTE      ;  bafera u CH;BX
          mov      bl,dh
          shr      bl,4
          shl      dx,4
          add      ax,dx
          adc      bl,0
          mov      ch,bl
          mov      bx,ax
          mov      dx,21H              ; Maskiraj IRQ_5 inace ce biti "belaja"
          mov      al,32
          out      dx,al
          mov      al,0D1H             ; Ukljuci SB zvucnik
          call     DSP_W
          mov      dx,0AH              ; Maskiraj DMA kanal
          mov      al,4
          add      al,DMACH
          out      dx,al
          add      dx,2                ; DMA interni reset
          xor      al,al
          out      dx,al
          dec      dx                  ; DMA "sent to device in auto-init
          mov      al,58H              ;  signal mode"
          add      al,DMACH
          out      dx,al
          mov      dl,DMACH            ; DMA nizih 16 bita linearne adrese
          add      dl,dl               ;  bafera
          mov      al,bl
          out      dx,al
          mov      al,bh
          out      dx,al
          inc      dx                  ; DMA duzina-1 (0 za duzinu 1 bajt)
          xor      al,al
          out      dx,al
          out      dx,al
          mov      dl,DMACH            ; DMA najvisa 4 bita linearne adrese
          mov      di,dx               ;  bafera
          mov      dl,PAGEREG[di]
          mov      al,ch
          out      dx,al
          mov      dx,0AH              ; Aktiviraj DMA kanal
          mov      al,DMACH
          out      dx,al
          mov      al,40H              ; Postavi SB DSP vremensku konstantu
          call     DSP_W
          mov      al,212
          call     DSP_W
          mov      al,48H              ; SB DSP duzina bloka - 1
          call     DSP_W
          xor      al,al
          call     DSP_W
          mov      al,8
          call     DSP_W
          mov      al,1CH              ; SB DSP auto-init DMA mode
          call     DSP_W
          cmp      LOAD_MODE,4
          jne      FINDGSTP
          mov      al,20H              ; Priprema za IN sa SBlastera
          call     DSP_W
FINDGSTP: popa
          ret
DIG_SETUP endp

          endif

;
; Neke SB inicijalizacije.
;

SBINSETUP proc     far
          cmp      BLASTADDR,0
          je       SBIFIN
          mov      al,20H              ; Priprema za IN sa SBlastera
          call     DSP_W
SBIFIN:   ret
SBINSETUP endp

DSPRESET  proc     far                 ; Reset SB kartice
          cmp      BLASTADDR,0
          je       SBIFIN
          pusha
          mov      dx,BLASTADDR
          add      dx,6
          mov      al,1
          out      dx,al               ; Izvrsi reset
          xor      al,al               ; citaj tajmer
          out      43H,al
          in       al,40H              ; Citaj nizi bajt
          mov      ah,al
          in       al,40H              ; Citaj visi bajt
          xchg     ah,al
          mov      cx,ax
BWAIT:    xor      al,al               ; citaj tajmer
          out      43H,al
          in       al,40H              ; Citaj nizi bajt
          mov      ah,al
          in       al,40H              ; Citaj visi bajt
          xchg     ah,al
          sub      ax,cx
          neg      ax
          cmp      ax,30
          jc       BWAIT               ; Cekaj neko vrijeme
          xor      al,al
          out      dx,al
          popa
          ret
DSPRESET  endp

DIG_KILL  proc     near                ; Iskljucivanje DMA
          mov      al,0DAH
          call     DSP_W
          mov      dx,0BH
          mov      al,49H
          ret
DIG_KILL  endp

;
; Rutina za egzaktnu simulaciju AY cipa pomocu Sound Blastera. Poziva se sa
; takvog mjesta da ne treba cuvati AX i CX. Vremenski je kriticna jer se poziva
; svakih 228 T stanja.
;

           if      VER_128

AY_EXACT   proc    far
           mov     cl,byte ptr RCLK+1
           xor     ch,ch
           add     cx,SBSPEED
           add     CHACUR,cx           ; Oscilator A
           mov     ax,CHAMAX
           cmp     CHACUR,ax
           jb      AY_OSCB
           sub     CHACUR,ax
           not     CHAOSC
AY_OSCB:   add     CHBCUR,cx           ; Oscilator B
           mov     ax,CHBMAX
           cmp     CHBCUR,ax
           jb      AY_OSCC
           sub     CHBCUR,ax
           not     CHBOSC
AY_OSCC:   add     CHCCUR,cx           ; Oscilator C
           mov     ax,CHCMAX
           cmp     CHCCUR,ax
           jb      AY_NOI
           sub     CHCCUR,ax
           not     CHCOSC
AY_NOI:    add     NOICUR,cx           ; Generator suma
           mov     ax,NOIMAX
           cmp     NOICUR,ax
           jb      AY_MIX
           sub     NOICUR,ax
           in      al,41H
           mov     ah,al
           in      al,41H
           test    ah,1
           jz      AY_MIX
           not     NOIOSC
AY_MIX:    mov     ax,word ptr CHAOSC  ; Simulacija AY mixera: svaki kanal je
           and     ax,word ptr CHAULT  ;  (OSCILATOR AND NOT TONE_ENABLE) AND
           or      ax,word ptr CHAOFF  ;  (NOISE_GEN OR NOT NOISE_ENABLE), a
           mov     cx,NOIOSC           ;  ova logicka vrijednost se mnozi sa
           or      cx,word ptr NOIAOFF ;  glasnocom i svi kanali se na kraju
           and     cx,ax               ;  saberu; zbog nemogucnosti DMA da
           and     cx,word ptr VOLA    ;  ispravno generise "ultrazvuk",
           mov     al,CHCOSC           ;  oscilatori se blokiraju ako je
           and     al,CHCULT           ;  frekvencija isuvise visoka, sto
           or      al,CHCOFF           ;  omogucava ispravan rad programa kao
           mov     ah,byte ptr NOIOSC  ;  sto je GotToGet
           or      ah,NOICOFF
           and     al,ah
           and     al,VOLC
           add     al,cl
           add     al,ch
           mov     ah,al               ; A+B+C je u opsegu 0-47, pa mnozenje sa
           add     al,al               ;  5 daje opseg 0-235, sto je blisko
           add     al,al               ;  punom opsegu 0-255 D/A konvertora
           add     al,ah
           cmp     EXTRABEEP,1
           je      DOEXTBEEP
           mov     DIGBYTE,al          ; Simulacija preko DMA
           ret
DOEXTBEEP: cmp     al,MAXSNDVAL        ; Extra Beeper adaptivna AY emulacija
           jbe     NOMAXSND            ;  (vrlo zanimljiv eksperiment)
           mov     MAXSNDVAL,al
NOMAXSND:  mov     ah,MAXSNDVAL
           shr     ah,1
           cmp     al,ah
           mov     ah,0
           jb      EXTCONT
           mov     ah,2
EXTCONT:   in      al,61H
           and     al,0FCH
           or      al,ah
           out     61H,al
           ret
AY_EXACT   endp

;
; Pripremne radnje prilikom promjene zvucnih registara (posto je AY_EXACT
; vremenski kriticna, pozeljno je da se sto vise stvari obavi izvan nje).
;

SB_CHASET: push    ax                  ; Postavka kanala A
           push    cx
           push    dx
           mov     ax,word ptr AY_REGS[0]
           cmp     ax,4
           cmc                         ; Odredjivanje da li je ultrazvuk
           sbb     cl,cl
           mov     CHAULT,cl
           shl     ax,3                ; Izgleda da umjesto preciznog mnozenja
           mov     dx,17734            ;  sa 140000/8867 moze priblizno
           mul     dx                  ;  mnozenje sa 16 bez osjetne razlike
           mov     cx,8867
           div     cx
           mov     CHAMAX,ax
           cmp     CHACUR,ax           ; Narednih nekoliko instrukcija emulira
           jb      CHAL1               ;  egzaktno ponasanje AY pri promjeni
           mov     al,28               ;  registra frekvencije
           sub     al,byte ptr RCLK+1
           cbw
           mov     CHACUR,ax
           not     CHAOSC
CHAL1:     call    far ptr AY_MIX
           pop     dx
           pop     cx
           pop     ax
           ret

SB_CHBSET: push    ax                  ; Postavka kanala B
           push    cx
           push    dx
           mov     ax,word ptr AY_REGS[2]
           cmp     ax,4
           cmc
           sbb     cl,cl
           mov     CHBULT,cl
           shl     ax,3
           mov     dx,17734 ;500
           mul     dx
           mov     cx,8867
           div     cx
           mov     CHBMAX,ax
           cmp     CHBCUR,ax
           jb      CHBL1
           mov     al,28
           sub     al,byte ptr RCLK+1
           cbw
           mov     CHBCUR,ax
           not     CHBOSC
CHBL1:     call    far ptr AY_MIX
           pop     dx
           pop     cx
           pop     ax
           ret

SB_CHCSET: push    ax                  ; Postavka kanala C
           push    cx
           push    dx
           mov     ax,word ptr AY_REGS[4]
           cmp     ax,4
           cmc
           sbb     cl,cl
           mov     CHCULT,cl
           shl     ax,3
           mov     dx,17734 ; 500
           mul     dx
           mov     cx,8867
           div     cx
           mov     CHCMAX,ax
           cmp     CHCCUR,ax
           jb      CHCL1
           mov     al,28
           sub     al,byte ptr RCLK+1
           cbw
           mov     CHCCUR,ax
           not     CHCOSC
CHCL1:     call    far ptr AY_MIX
           pop     dx
           pop     cx
           pop     ax
           ret

SB_NOISET: push    ax                  ; Postavka generatora suma
           mov     ax,word ptr AY_REGS[6]
           and     ax,31
           add     ax,8
           shl     ax,5
           mov     NOIMAX,ax
           pop     ax
           ret

SB_MIXSET: push    ax                  ; Postavka miksera
           mov     al,AY_REGS[7]
           shr     al,1
           sbb     ah,ah
           mov     CHAOFF,ah
           shr     al,1
           sbb     ah,ah
           mov     CHBOFF,ah
           shr     al,1
           sbb     ah,ah
           mov     CHCOFF,ah
           shr     al,1
           sbb     ah,ah
           mov     NOIAOFF,ah
           shr     al,1
           sbb     ah,ah
           mov     NOIBOFF,ah
           shr     al,1
           sbb     ah,ah
           mov     NOICOFF,ah
           push    cx
           call    far ptr AY_MIX      ; Izvrsi ucinjene promjene odmah
           pop     cx
           pop     ax
           ret

;
; Simulacija ZX Spectrum 128K RS 232 interfejsa i KEYPAD-a.
;

RS128_OUT: test    AY_REGS[7],64       ; Ako je u R7 b6=0, ignorisi izlaz
           jz      NOOUTPUT
           push    ax                  ; Obrisi eventualno KEYPAD timer
           test    ah,1
           jz      NOCLRTMR
           mov     KPDTIMER,2
NOCLRTMR:  xor     ah,KPDLAST          ; Da li je nastupio KEYPAD clock?
           test    ah,1
           pop     ax
           mov     KPDLAST,ah
           jz      NOKPDOUT
           mov     AUX,KPDPOS          ; Aktiviraj KEYPAD shifter
           inc     AUX
           cmp     AUX,22
           je      KPDFLG
           cmp     AUX,42
           je      KPDFLG
           cmp     AUX,62
           je      KPDFLG
           cmp     AUX,82
           je      KPDFLG
           cmp     AUX,102
           jne     NOKPDFLG
KPDFLG:    cmp     KPDTABLE[AUX-1],255
           mov     KPDTABLE[AUX-1],223
           je      NOKPDFLG
           add     AUX,16
NOKPDFLG:  cmp     AUX,120
           jne     NOKPDLOOP
           mov     AUX,20
NOKPDLOOP: mov     KPDPOS,AUX
NOKPDOUT:  lea     AUX,RS128_B         ; Posalji bit na RS232
           call    RS_OUT
           cmp     BLASTPRES,1
           jne     NOOUTPUT
           cmp     BLASTADDR,0
           je      NOOUTPUT
           call    MIDI_OUT
NOOUTPUT:  ret

RS128_IN:  call    RS_IN               ; Ocitaj RS232
           cmp     KPDFLAG,1
           je      KPDIN
           and     ah,223
           jmp     short TSTMASK
KPDIN:     mov     AUX,KPDPOS          ; Ocitaj KEYPAD
           and     ah,KPDTABLE[AUX]
TSTMASK:   test    AY_REGS[7],64       ; Ako je u R7 b6=1, maskiraj sa R14
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
           mov     SPROTECT,0
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
; Rutine SETKBD i RESTKBD postavljaju odnosno obnavljaju interrupt tastature.
; SETKBD takodje azurira NumLock LED diodu prema stanju KEYPAD-a, i brise
; bitmapu tastera.
;

SETKBD     proc    far
           pushf
           cli
           push    es
           push    di
           if      VER_128
           mov     cl,KPDFLAG
           shl     cl,5
           else
           xor     cl,cl
           endif
           xor     ax,ax
           mov     es,ax
           mov     es:[36],offset INT_9
           mov     ax,CSEG2
           mov     es:[38],ax
           and     byte ptr es:[0417H],143
           or      es:[0417H],cl
           mov     al,0EDH
           out     60H,al
           push    cx
           xor     cx,cx
SETKWAIT:  loop    SETKWAIT
           pop     cx
           shr     cl,4
           mov     al,cl
           out     60H,al
           mov     cx,8
           lea     di,KBDROWS
SETLOOP:   mov     byte ptr ss:[di],0FFH
           inc     di
           loop    SETLOOP
           pop     di
           pop     es
           popf
           mov     al,20H
           out     20H,al
           ret
SETKBD     endp

RESTKBD    proc    far
           cli
           push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ax,word ptr LAST_09
           mov     ds:[36],ax
           mov     ax,word ptr LAST_09+2
           mov     ds:[38],ax
           pop     ds
           mov     al,20H
           out     20H,al
           sti
           ret
RESTKBD    endp

;
; Rutine SETVEC i RESTVEC preusmjeravaju odnosno obnavljaju INT vektore. Takodje
; se obnavlja i DOS sistemski sat prema satu realnog vremena, kao i originalni
; drajv i direktorijum (prije pokretanja simulatora).
;

SETVEC     proc    far
           pushf
           pusha
           cli
           call    SETTIMER
           call    SETKBD
           push    es
           mov     cx,8
           lea     dx,RMOUSEHND        ; Handler za desnu tipku misa
           mov     es,PROGSEG
           mov     ax,12
           int     33H
           xor     ax,ax
           mov     es,ax
           mov     ax,CSEG
           mov     es:[52],offset INT_0D
           mov     es:[54],ax
           mov     es:[108],offset INT_1B
           mov     es:[110],ax
           mov     es:[140],offset INT_1B
           mov     es:[142],ax
           mov     es:[144],offset INT_24
           mov     es:[146],ax
           mov     es:[188],offset INT_2F
           mov     es:[190],ax
           mov     es:[1000],offset INT_FA
           mov     es:[1002],ax
           pop     es
           popa
           popf
           ret
SETVEC     endp

CONVPCK:   irp     REG,<ch,cl,dh,dl>   ; Konvertuje BCD format u binarni
           mov     al,REG
           xor     ah,ah
           shl     ax,4
           rol     al,4
           aad
           mov     REG,al
           endm
           ret

RESTVEC    proc    far
           call    RESTTIMER
           call    RESTKBD
           push    es
           pusha
           xor     cx,cx               ; Deaktiviraj MOUSE interapt
           mov     ax,12
           int     33H
           xor     ax,ax
           mov     es,ax
           mov     ax,word ptr LAST_01
           mov     es:[4],ax
           mov     ax,word ptr LAST_01+2
           mov     es:[6],ax
           mov     ax,word ptr LAST_0D
           mov     es:[52],ax
           mov     ax,word ptr LAST_0D+2
           mov     es:[54],ax
           mov     ax,word ptr LAST_1B
           mov     es:[108],ax
           mov     ax,word ptr LAST_1B+2
           mov     es:[110],ax
           mov     ax,word ptr LAST_2F
           mov     es:[188],ax
           mov     ax,word ptr LAST_2F+2
           mov     es:[190],ax
           mov     ax,word ptr LAST_FA
           mov     es:[1000],ax
           mov     ax,word ptr LAST_FA+2
           mov     es:[1002],ax
           mov     ah,02H              ; Cita sat realnog vremena
           int     1AH
           call    CONVPCK
           mov     ah,2DH              ; Postavlja sistemski sat
           int     21H
           mov     ah,04H              ; Cita datum realnog vremena (ovo je
           int     1AH                 ;  neophodno zbog MidNight problema)
           call    CONVPCK
           mov     al,ch
           xor     ah,ah
           imul    ax,ax,100
           xor     ch,ch
           add     cx,ax
           mov     ah,2BH              ; Postavlja sistemski datum
           int     21H
           call    DEFDD
           popa
           pop     es
           ret
RESTVEC    endp

;
; Obrada interrupta tastature. Neke tipke (npr. strelice) reaguju kao pritisak
; na dvije tipke istovremeno (npr. CAPS SHIFT + 5).
;

STORESS    dw      0                   ; Spremnik za SS
STORESP    dw      0                   ; Spremnik za SP
CSSTEK     dw      SSEG                ; Pointer na SSEG adresibilan preko CS

INT_9:     mov     STORESS,ss          ; Ovo je sve zbog problema koji nastupe
           mov     STORESP,sp          ;  ako INT_9 nastupi u DOS rutini zato
           mov     ss,CSSTEK           ;  sto DOS mijenja SS!
           lea     sp,INT_STACK
           push    ax
           if      VER_128
           mov     al,JTYPE
           mov     JTYPETEMP,al
           endif
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
I9_NORMAL: if      VER_128
           cmp     al,69
           jne     NONUMLCK            ; NumLock pali KEYPAD
           push    cx
           xor     KPDFLAG,1
           mov     al,0EDH             ; Upali NumLock LED diodu
           out     60H,al
           xor     cx,cx
I9_WAIT:   loop    I9_WAIT
           mov     al,KPDFLAG
           add     al,al
           out     60H,al
           xor     ax,ax               ; Obavijesti BIOS
           push    es
           mov     es,ax
           xor     byte ptr es:[0417H],32
           pop     es
           pop     cx
           mov     al,69
           endif
NONUMLCK:  call    I9_MAIN
           mov     EXTHFLG,0
           if      VER_128
           mov     al,JTYPETEMP
           mov     JTYPE,al
           endif
END_I9:    mov     al,20H              ; EOI
           out     20H,al
           pop     ax
           mov     ss,STORESS          ; Obnovi stek
           mov     sp,STORESP
           iret

I9_MAIN:   push    bx                  ; Ova rutina se rekurzivno poziva dva
           push    cx                  ;  puta za slucaj dvojnih tipki
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
           jbe     PRESS               ;  pritiska ili otpustanja tastera?
           not     cl
           or      KBDROWS[bx],cl      ; Signaliziraj otpustanje
           jmp     short END_9
DBLKEY:    push    si                  ; Ako je dvojna tipka, ocitaj dvije
           mov     si,ax               ;  zamjenske tipke iz tablice, i pozovi
           and     si,00FFH            ;  2 puta istu ovu rutinu
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

KBDJOY:    if      VER_128             ; Tretman kurzorskih i sivih tipki
           cmp     KPDFLAG,1           ; Ako radi KEYPAD, razmotri detaljnije
           je      KEYPAD
           endif
NOKPD:     cmp     al,205              ; Bez KEYPAD-a samo su kurzori znacajni
           jae     END_9
           cmp     JTYPE,0
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
           jne     KBDOTH1             ;  shift tipkom
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

           if      VER_128

KEYPAD:    cmp     EXTHFLG,1           ; Obrada kad je aktivan KEYPAD
           jne     KEYPAD1
           mov     JTYPE,0             ; Sivi kurzori uvijek rade kao kurzori
           cmp     al,200
           je      KEYPAD1
           cmp     al,204
           ja      KEYPAD1
           jmp     NOKPD
KEYPAD1:   push    si                  ; Obavi potrebne konverzije
           mov     bl,al
           sub     bl,200
           xor     bh,bh
           add     bx,bx
           lea     si,KPDCONV
           cmp     EXTHFLG,1           ; Konverzije nisu iste za sive i bijele
           jne     KEYPAD2
           lea     si,KPDCONV_E
KEYPAD2:   mov     cx,word ptr ss:[si+bx]
           mov     bl,cl
           mov     KPDTABLE[bx],255
           mov     bl,ch
           mov     KPDTABLE[bx],223
           cmp     ah,128
           jae     KPDRLSE
           mov     KPDTABLE[bx],255
KPDRLSE:   pop     si
           jmp     END_9

           endif

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

;
; Verzija bordera za Hercules karticu formira sloj oko ekrana.
;

H_B_FAR    proc    far
           push    di
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
H_B_FAR    endp

;
; Slicno je i na CGA kartici.
;

C_B_FAR    proc    far
           push    di
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
C_B_FAR    endp

;
; BORDER za Extended Hercules.
;

X_B_FAR    proc    far
           push    di
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
X_B_FAR    endp

;
; I EGAMONO kartica koristi isti princip.
;

EM_B_FAR   proc    far
           push    di
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
EM_B_FAR   endp

;
; Rutina za postavljanje BORDER-a iz monitora, itd. Malo je postala slozenija
; zbog potrebe za iskljucenjem HiCol BORDER-a.
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
ULAF1:     call    far ptr ULAFULLFAR
           pop     ax
           test    al,4
           jz      ULAF2
           call    SETBRDFAR
ULAF2:     pop     ax
           retf
ULAFAR     endp

;
; Aktiviranje grafike za Hercules.
;

HGFAR      proc    far
           lea     si,GRAPHCON
           jmp     short HG1
HGFAR      endp

XGFAR      proc    far
           lea     si,EXTCON
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
XGFAR      endp

;
; Aktiviranje grafike na EGA i VGA.
;

V_G_FAR    proc    far
           mov     ax,1200H
           mov     bl,30H
           int     10H
           mov     al,1
           sub     al,MXSPD
           mov     VGAOK,al            ; Odredi da li je dozvoljen HiCol border
           stc
           pushf
           jmp     short VE_CONT
V_G_FAR    endp

E_G_FAR    proc    far
           clc
           pushf
VE_CONT:   lea     ax,VSETUP
           mov     FKADDR[8],ax        ; Taster F5 preusmjeriti na EGA i VGA
           mov     ax,1
           int     10H
           mov     ax,1100H            ; Inicijaliziraj fontove
           mov     es,STEKSEG
           lea     bp,MASKA
           mov     cx,256
           xor     dx,dx
           mov     bx,512
           int     10H
           mov     dx,03D4H            ; CRT kontroler
           mov     al,9                ; Broj scan linija
           out     dx,al
           inc     dx
           xor     al,al
           popf
           pushf
           rcr     al,1
           out     dx,al               ; Postavljanje broja scan linija
           mov     dx,03DAH            ; Read Status za cekanje
           in      al,dx
           mov     dx,03C0H            ; Mode control
           mov     al,10H
           out     dx,al               ; Color, text, 9. bit background, no
           xor     al,al               ;  flash
           out     dx,al
           mov     al,20H
           out     dx,al
           mov     dx,03CEH            ; Misc Graphic control port
           mov     al,6
           out     dx,al
           inc     dx
           mov     al,2
           out     dx,al               ; ORG A000, text ODD after even
           mov     dx,3DAH             ; Read Status
           in      al,dx
           popf
           jnc     NO_SETYEL
           mov     bx,6                ; Postavi zutu boju da bude ispravna
           mov     dx,39*256+0         ;  (samo za VGA: DH=R, CH=G, CL=DL=B)
           mov     cx,dx
           mov     ax,1010H
           int     10H
NO_SETYEL: mov     es,VIDEOSEG         ; Postavi laznu boju za emulaciju bordera
           mov     cx,32768
           xor     di,di
           mov     ax,08FFH
           cld
           rep     stosw
           cmp     MPXINST,1
           je      VGEND
           test    WINTROUB,1
           jz      VGEND
           mov     ax,4000H            ; Windows podrska za VDD
           int     2FH
           mov     MPXINST,1
VGEND:     mov     EGAVGA,1
           ret
E_G_FAR    endp

CSEG2      ends

CSEG       segment public

           extrn   EDGESOUND:word,SLOW:word
           extrn   M386_4:word,M386_5:word,M386_6:word

           assume  cs:CSEG,ds:ZXDSEG,ss:SSEG,es:nothing

;
; Potprogrami za simulaciju nestandardnih stanja za MMU kod 128K Spectruma, tj.
; stranica 2 i 5, kao i stranice 7 kada je aktivna alternativna video memorija.
;

WRITESP    macro   BYTE                ;; Makro za upis na stek
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

           align   4
P2_&RP:    xor     RP,16384            ; Akcije za stranicu 2
           mov     ds:[RP],ah
           mov     es:[RP],ah
           xor     RP,16384
           ret

           align   4
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

           align   4
P7_&RP:    cmp     RP,56064            ; Akcije za stranicu 7
           jb      P7SPC_&RP
           ret
P7SPC_&RP: cmp     RP,55296
           adc     XVIDEOCHG,0
           cmc
           adc     XATTRCHG,0
           ret

           endm

           endif

;
; BIG_IN se poziva ako IN prozove istovremeno vise periferala, sto se normalno
; ne bi trebalo da radi, ali je moguce.
;

BIG_IN:    call    USER_IN             ; Ispitaj korisnicki handler i izadji
           jc      BI8                 ;  ako je on obavljen
           push    dx
           mov     dl,ah               ; Treba AND-ovati sva citanja prisutnih
           test    C,1                 ;  periferala
           jnz     BI1
           call    KEYBOARD            ; b0=0 => tastatura
           and     dl,ah
BI1:       test    C,2
           jnz     BI2
           call    IN_128              ; b1=0 => 128 specificnosti
           and     dl,ah
BI2:       test    C,4
           jnz     BI3
           call    ZXP_IN              ; b2=0 => ZX printer
           and     dl,ah
BI3:       test    C,24
           jnz     BI4
           call    MDV_IN              ; b3=0 & b4=0 => MDV data port
           and     dl,ah
           jmp     short BI6
BI4:       test    C,8
           jnz     BI5
           call    ZXI1_IN             ; b3=0 & b4=1 => ZXI1 data port
           and     dl,ah
           jmp     short BI6
BI5:       test    C,16
           jnz     BI6
           call    ZXI1_STS            ; b3=1 & b4=0 => ZXI1 status port
           and     dl,ah
BI6:       test    C,224               ; b5=0 & b6=0 & b7=0 => joystick
           jnz     BI7
           call    JOYSTICK
           and     dl,ah
BI7:       mov     ah,dl
           pop     dx
BI8:       ret

;
; BIG_OUT se poziva ako OUT prozove istovremeno vise periferala, sto se normalno
; ne bi trebalo da radi, ali je moguce.
;

BIG_OUT:   call    USER_OUT            ; Ispitaj korisnicki handler i izadji
           jc      DUMMY               ;  ako je on obavljen
           test    C,1
           jnz     BO1
           call    ULA_OUT             ; b0=0 => ULA
BO1:       if      VER_128
           test    C,2
           jnz     BO2
           call    OUT_128             ; b1=0 => 128 specificnosti
           endif
BO2:       test    C,4
           jnz     BO3
           call    ZXP_OUT             ; b2=0 => ZX printer
BO3:       test    C,24
           jnz     BO4
           call    MDV_OUT             ; b3=0 & b4=0 => MDV data port
           jmp     short DUMMY
BO4:       test    C,8
           jnz     BO5
           call    ZXI1_OUT            ; b3=0 & b4=1 => ZXI1 data port
           jmp     short DUMMY
BO5:       test    C,16
           jnz     DUMMY
           call    ZXI1_CTRL           ; b3=1 & b4=0 => ZXI1 control port
DUMMY:     ret

;
; Korisniski IN port. Poziva se INT 248 ako je ispred pripadnog handlera
; signatura 'USER'.
;

USER_IN:   push    es
           push    bx
           xor     bx,bx               ; Test signature
           mov     es,bx
           les     bx,es:[992]
           cmp     bx,4
           jb      NO_U_IN
           cmp     word ptr es:[bx-4],'SU'
           je      MAYBE_IN
NO_U_IN:   pop     bx
           pop     es
           clc
           ret

MAYBE_IN:  cmp     word ptr es:[bx-2],'RE'
           jne     NO_U_IN
           pop     bx
           push    ds
           pusha                       ; Signatura je ispravna
           call    RESTTIMER
           clc
           int     248                 ; Pozovi handler
           call    SETTIMER
           mov     USERTEMP,ah         ; AH registar mora da "prezivi"
           popa
           mov     ah,USERTEMP
           pop     ds
           pop     es
           ret

;
; Korisniski OUT port. Poziva se INT 249 ako je ispred pripadnog handlera
; signatura 'USER'.
;

USER_OUT:  push    es
           push    bx
           xor     bx,bx               ; Test signature
           mov     es,bx
           les     bx,es:[996]
           cmp     bx,4
           jb      NO_U_OUT
           cmp     word ptr es:[bx-4],'SU'
           je      MAYBE_OUT
NO_U_OUT:  pop     bx
           pop     es
           clc
           ret

MAYBE_OUT: cmp     word ptr es:[bx-2],'RE'
           jne     NO_U_OUT
           pop     bx
           push    ds
           pusha                       ; Signatura je ispravna
           call    RESTTIMER
           clc
           int     249                 ; Pozovi handler
           call    SETTIMER
           popa
           pop     ds
           pop     es
           ret

;
; Ako port nije definisan korisnicki, poziva se B_IDL_FAR.
;

BUS_IDLE:  call    USER_IN
           jc      USER_DONE
           call    B_IDL_FAR
USER_DONE: ret

;
; ULA se simulira na 4 razna nacina, pa se koristi indirektni skok.
;

ULA_OUT:   cmp     LOAD_MODE,3
           je      OUTTOBGSV
           cmp     PC,1373             ; Ignorisi OUT na adresi 1372 (unutar
           jne     DOULAOUT            ;  LD_BYTES rutine)
           ret
DOULAOUT:  jmp     ULA
OUTTOBGSV: pusha                       ; Slanje preko LPT1
           push    es
           xor     dx,dx
           mov     es,dx
           mov     dx,es:[0408H]
           shr     ah,3
           and     ah,1
           mov     al,ah
           out     dx,al
           pop     es
           popa
           jmp     DOULAOUT

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
; Potprogram REPAINT iscrtava ponovo ekran visoke rezolucije (npr. nakon F1).
;

REPAINT    proc    far
           pusha
           mov     ax,LASTB            ; Obnovi BORDER
           and     ah,7
           mov     LASTB,-1
           cmp     INGTAPE,1
           je      NORPBRD             ; Ne obnavljaj border u GETTAPE
           call    ULAFAR
NORPBRD:   push    es
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
REPAINT    endp

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
           mov     al,FL_CREG          ;  izmjenu grafickih stranica
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
           cmp     VIDSYNC,-1
           je      VF1
           out     dx,al
           inc     dx
VF1:       mov     al,FL_CREG
           xor     al,40H
           cmp     VIDSYNC,-1
           je      VF2
           out     dx,al
VF2:       mov     FL_CREG,al
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
; izlazu je u AH ocitana vrijednost. Rutina se oslanja na izmjenjeni INT 9.
;

KEYBOARD:  call    EARIN
           or      ah,0BFH
           lea     AUX,KBDROWS         ; Ova rutina ne cuva AUX !
           push    BC
           mov     C,8
KBDLOOP:   ror     B,1
           jc      NOROW
           and     ah,ss:[AUX]
NOROW:     inc     AUX
           dec     C
           jnz     KBDLOOP
           pop     BC
           ret

;
; EAR ulaz (bit b6). Vraca slucajan broj, osim u rezimu semplovanja.
;

EARIN:     cmp     LOAD_MODE,3
           je      INBGSV
           cmp     LOAD_MODE,4
           je      INSBST
           cmp     SAMPLEON,1
           je      INSAMPLE
           cmp     SMPSTARTER,0        ; Ne preduzimaj nista posebno ako nije
           jz      EARRANDOM           ;  sempl blok na redu, ili ako je aktivan
           cmp     EDGEOLD,0           ;  edge recognizer
           jnz     EARRANDOM
           dec     SMPSTARTER          ; Cekaj 100 IN-ova (uz b6=0)
           jnz     EARZERO
           call    TSTSMPBLK           ; Zadnja provjera
           jnz     EARZERO
           mov     SAMPLEON,1          ; Predji u "sample" mod
           call    far ptr MAXLSON
EARZERO:   xor     ah,ah
           ret
EARRANDOM: mov     ah,RANDOM           ; Slucajan broj
           not     RANDOM
           ret
INSAMPLE:  call    EARSAMPLE
           ret

;
; Ucitavanje sa realne trake.
;

INBGSV:    push    dx                  ; Citanje sa BG Services
           push    es
           xor     dx,dx
           mov     es,dx
           mov     dx,es:[0408H]
           inc     dx
           xchg    ah,al
           in      al,dx
           shr     al,1
           xchg    ah,al
           pop     es
           pop     dx
           ret

INSBST:    cmp     BLASTADDR,0         ; Citanje sa SBlastera
           jne     DOINSBST
           ret
DOINSBST:  push    dx
           xchg    ah,al
           mov     dx,BLASTADDR
           add     dx,0eh              ; DSP Data available
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
           shr     al,1
           xchg    ah,al
           pop     dx
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
           jmp     USER_IN             ; Ako nema joysticka, izvrsi USER_IN
JOYA:      call    RESTTIMER
           call    JOYANALOG
           call    SETTIMER
           jmp     short JOYK
JOYM:      mov     INHIBIT,0FFH
           call    JOYMOUSE
           mov     INHIBIT,0
JOYK:      mov     ah,KJOYVAL
           ret

;
; Simulacija RS 232 interfejsa na ZX Interface-u 1.
;

RSI1_OUT:  lea     AUX,RSI1_B          ; Prilagodi pointer na varijable
           push    ax
           rol     ah,3                ; Prilagodi ulazni format
           not     ah
           call    RS_OUT
           pop     ax
           ret

RSI1_IN:   call    RS_IN               ; Pozovi rutinu za RS citanje, i
           ror     ah,3                ;  prilagodi rezultat ZXI1 formatu
           xor     ah,8
           and     ah,254              ; Resetuj b0 (WRITE PROTECTED)
           ret

;
; Simulacija slanja na ULA chip (port 254). Bitovi b0, b1 i b2 kontrolisu boju
; bordera, a bit b3 zvucnik. Bit b4 (MIC izlaz) se ne simulira.
;

ULAFULLFAR proc    far
           call    ULAFULL
           ret
ULAFULLFAR endp

ULAFULL:   call    BORDONLY            ; Normalna ulazna tacka
SPEAKER:   cmp     SPROTECT,0          ; Ulazna tacka ako je BORDER iskljucen
           je      DOSPEAKER           ;  (ovdje su neki trikovi zbog EDGE
           dec     SPROTECT            ;  zvuka)
           jmp     short NOSPBR
DOSPEAKER: push    ax
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
           cmp     SSTEPON,1
           je      NO_BORDER           ; Ne mijenjaj border u single stepu
           call    BORDER
NO_BORDER: pop     ax
           ret

;
; Daleki pozivi, prepun segment...
;

H_BORDER:  call    H_B_FAR
           ret

C_BORDER:  call    C_B_FAR
           ret

X_BORDER:  call    X_B_FAR
           ret

EM_BORDER: call    EM_B_FAR
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
           test    ULAMODE,4           ; U HclBrd modu ne mijenjaj paletu
           jnz     V_BRD_END
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
V_BRD_END: pop     bx
           ret

;
; Simulacija ZX Spectrum 128K specificnih periferala (upravljanje memorijom,
; zvucni cip AY-3-8912, RS 232 i MIDI interfejs, dodatna tastatura).
;

IN_128:    if      VER_128
           test    B,128
           jz      MMU
           test    B,64
           jz      IGNOR_128
           call    AY_IN
           endif
IGNOR_128: ret

OUT_128:   if      VER_128
           test    B,128
           jz      MMU
           test    B,64
           jnz     AY_SETREG
           call    AY_OUT
           ret
AY_SETREG: mov     AY_REG,ah
           and     AY_REG,15
           ret
           else
           cmp     TESTFLG,1           ; Eventualno prekini program
           je      STOP128
           ret
STOP128:   lea     dx,TESTMSG
           jmp     ILL1

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

SETP7:     mov     MMUVAR,49151
           mov     ACT_HL,offset P7_HL
           mov     ACT_AUX,offset P7_AUX
           mov     ACT_STP,offset P7_STP
           ret

           endif

;
; Obnova generisanja slike.
;

RSTPIC:    cmp     VIDSYNC,-2
           je      DORSTPIC
           ret
DORSTPIC:  push    ax
           mov     ax,OLDVIDS2
           mov     VIDSYNC,ax
           pop     ax
           ret

;
; Prikaz HELP ekrana tastature (F1).
;

HELPSCR:   pusha
           call    RSTPIC
           if      VER_128
           call    STOPADLIB
           endif
           call    KLIK
           call    RESTTIMER
           call    RESTKBD
           if      VER_128
           mov     ah,MMUSTATE
           push    ax
           and     ah,247
           and     MMUSTATE,31
           call    MMU
           endif
           push    es
           mov     es,DS2PTR
           mov     cx,3456
           mov     si,16384
           mov     di,58608
           cld
           rep     movsw
           pop     es
HELPAGAIN: push    es
           mov     es,MEMSEG
           mov     ax,LOGOSEG
           mov     ds,ax
           mov     si,6912
           mov     di,16384
           mov     cx,3456
           rep     movsw
           mov     ds,MEMSEG
           pop     es
           push    LASTB
           mov     ax,5*256
           mov     LASTB,ax
           call    REPAINT
           pop     LASTB
           mov     TSKSWTCH,0
HSL3:      mov     ah,1                ; Isprazni bafer za tastaturu
           int     16H
           jz      HSL4
           xor     ah,ah
           int     16H
           jmp     HSL3
HSL4:      cmp     TSKSWTCH,1          ; Ponovi iscrtavanje ako Task Switch
           mov     TSKSWTCH,0          ;  nastupi unutar ove petlje
           je      HELPAGAIN
           mov     ah,1                ; Cekaj na pritisak tipke
           int     16H
           jz      HSL4
           mov     ds,DS2PTR
           push    es
           mov     es,MEMSEG
           mov     cx,3456
           mov     di,16384
           mov     si,58608
           rep     movsw
           mov     ds,MEMSEG
           pop     es
           if      VER_128
           pop     ax
           call    MMU
           endif
           call    REPAINT
           call    SETKBD
           call    SETTIMER
           if      VER_128
           call    INITADLIB
           endif
           popa
           ret

;
; Poziv okruzenja ili izlaz iz programa (F10).
;

EXIT:      pusha
           call    RSTPIC
           call    MDVUPDT             ; Azuriraj MDV cache
           popa
           test    KBDROWS[0],1        ; Testiraj SHIFT
           jnz     DOCHAIN
           add     PC,2                ; Izlaz ako je SHIFT + F10
           lea     dx,ESCAPED
           jmp     ILL1
DOCHAIN:   mov     word ptr SNPFILE[0],'ET'
           mov     word ptr SNPFILE[2],'PM'
           pusha
           in      al,61H              ; Zaustavi zvuk smjesta
           and     al,0FCH
           out     61H,al
           if      VER_128
           call    STOPADLIB
           endif
           call    RESTTIMER
           call    RESTKBD
           call    SWAPDD
           popa
           call    SAVESNAP
           call    CONFSAVE
           call    TERMINATE
           call    DEFDD
           jmp     far ptr CHAIN

;
; Reset taster (F9).
;

RESET:     if      VER_128
           and     MMUSTATE,31
           xor     ah,ah
           call    MMU
           endif
           xor     PC,PC
           mov     IFF,0
           mov     word ptr I,0
           mov     R7,0
           mov     HSTATE,0
           ret

;
; Nemaskirani interrupt NMI (F8).
;

NMI:       sub     STP,2
           add     PC,word ptr HSTATE
           mov     ds:[STP],PC
           mov     PC,102
           mov     ah,IFF1
           mov     IFF2,ah
           mov     IFF1,0
           mov     HSTATE,0
           add     RCLK,11*256
           ret

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
           pop     es
           pop     ds
           popf
           popa
           ret

;
; Multikolor postavka na VGA (F5).
;

VSETUP:    push    ax                  ; Samo F5 uvecava EXTRADELAY, a sa bilo
           test    ULAMODE,4           ;  kojim SHIFT-om ga smanjuje
           jz      VSTPEND
           call    KLIK
           cmp     VIDSYNC,-1
           je      VHCRSET
           push    dx
           call    V_STOP_F
           pop     dx
           mov     VIDSYNC,-1
           jmp     short VSTPEND
VHCRSET:   dec     EXTRADELAY
           test    KBDROWS[0],1        ; Testira CS ili SS
           jz      VSTPEND
           test    KBDROWS[7],2
           jz      VSTPEND
           add     EXTRADELAY,2
VSTPEND:
;;;!!!
;mov al,extradelay
;mov ds:16000,al
           pop     ax
           ret

;
; Aktiviranje i deaktiviranje zvuka (F4). ALT+F4 napusta program (Windows).
;

SETSOUND:  pusha
           test    KBDROWS[7],2
           jz      WINEXIT
           call    KLIK
           call    SETSNDFAR
           popa
           ret

WINEXIT:   call    MDVUPDT
           popa
           add     PC,2
           lea     dx,ESCAPED
           jmp     ILL1

;
; Aktiviranje i deaktiviranje bordera (F6), ili iskljucenje slike (SHIFT+F6).
;

SETBORDR:  call    SETBORDRF           ; Prepun segment!
           ret

;
; Potprogram COUNTERS poziva se iz vremenskog interapta. On azurira neke brojace
; i poduzima odgovarajuce akcije ovisno od njihove vrijednosti.
;

COUNTERS:  dec     FL_CNT              ; Izvrsi FLASH kada dodje vrijeme
           jnz     NOFLASH
           call    FLASH
NOFLASH:   cmp     WORKTIME,0          ; Izadji ako je ograniceni rezim rada, a
           je      NOLIMITED           ;  vrijeme je isteklo
           dec     WORKTIME
           jnz     NOLIMITED
           mov     KBDROWS[0],0
           mov     FKVEC,offset EXIT
           cmp     CALLENV,1
           jne     NOLIMITED
           mov     KBDROWS[0],1
NOLIMITED: if      VER_128
           inc     MIDI_T              ; Resetuj MIDI ako se ne koristi
           cmp     MIDI_T,5
           jne     NORMIDI
           mov     MIDI_C,9
NORMIDI:   inc     RS128_T             ; Resetuj RS 232 (128) ako se ne koristi
           cmp     RS128_T,5
           jne     NOR128DIS
           mov     RS128_C,9
NOR128DIS: test    AY_REGS[14],1
           jz      NOKPDDIS
           dec     KPDTIMER
           jnz     NOKPDDIS
           mov     KPDPOS,0
           endif
NOKPDDIS:  inc     RSI1_T              ; Isto za RS 232 u Interface-u 1
           cmp     RSI1_T,5
           jne     NORFSH
           mov     RSI1_C,9
NORFSH:    cmp     LDCNT,0             ; Ponisti LD_EDGE ako nije koristen
           jz      NOLD                ;  izvjesno vrijeme
           dec     LDCNT
           jnz     NOLD
           call    KILLEDGE
NOLD:      inc     ERASECNT            ; Pomjeri traku na mikrodrajvu ako radi
           cmp     ERASECNT,2          ;  motor, i eventualno ubrljaj sektor ako
           jne     COUNTEND            ;  je ERASE = 0
           call    MDV_TIMER
           mov     ERASECNT,0
COUNTEND:  ret

;
; Rutina za postizanje Real-time rezima.
;

           align   4
SLOW_MAIN: push    ax
           push    cx
           if      VER_128
           sub     RCLK,228*256        ; Spectrum 128 ima drugaciji timing
           else
           sub     RCLK,224*256
           endif
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
           if      VER_128
           cmp     EXTRABEEP,1         ; U slucaju SoundBlaster-a ili ExtraBeep
           je      DOAYEXACT           ;  rezima potrebna je softverska
           cmp     BLASTADDR,0         ;  simulacija oscilatora
           je      NOAYSB
DOAYEXACT: cmp     STOPAY,1
           je      NOAYSB
           call    AY_EXACT
           endif
NOAYSB:    pop     cx
           test    ULAMODE,4
           jz      NOHCRBRD
           call    HICOLBRD            ; Pozovi HiCol BORDER ako radi
NOHCRBRD:  inc     VIDEOFLG
           mov     ax,VIDSYNC
           cmp     VIDEOFLG,ax
           je      DO_VID_L
           if      VER_128
           cmp     VIDEOFLG,311        ; Spectrum 128 ima jednu liniju manje
           else
           cmp     VIDEOFLG,312
           endif
           jae     DO_IRQ
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
           if      VER_128
           sub     RCLK,228*256
           else
           sub     RCLK,224*256
           endif
           and     R,127
           inc     VIDEOFLG
           mov     ax,VIDEOFLG
           cmp     ax,VIDSYNC
           je      DO_VID_F
           if      VER_128
           cmp     ax,311
           else
           cmp     ax,312
           endif
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

DO_IRQ:    inc     IRQCNT
           mov     VIDEOFLG,0
           mov     ax,VIDEOCHG         ; Ovo su neki hackovi za HiRes BORDER
           mov     VIDEOHCK,ax
           mov     ax,ATTRCHG
           mov     ATTRHCK,ax
           cmp     SSTEPON,1           ; Niposto ne izvrsavati interapt u
           je      NOINT               ;  Single-Step rezimu!
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
           mov     VIDEOHCK,0
           mov     ATTRHCK,0
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
UPD_XCONT: cmp     XVIDEOCHG,0
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

           if      VER_128
FIRSTLINE  equ     63                  ; Na Spectrumu 128 border ima jednu
LASTLINE   equ     255                 ;  liniju manje gore
           else
FIRSTLINE  equ     64
LASTLINE   equ     256
           endif

HCBEND:    pop     bx
           ret

           align   4
HICOLBRD:  push    bx
           mov     bx,VIDEOFLG
           cmp     bx,FIRSTLINE-4      ; Testiraj limit ekrana i izadji ako je
           jb      HCBEND              ;  van ekrana
           cmp     bx,LASTLINE+4
           ja      HCBEND
           push    di
           push    es
           mov     es,VIDEOSEG
           mov     di,bx
           sub     di,FIRSTLINE-4
           imul    di,di,80
           cld
           cmp     bx,FIRSTLINE        ; Ne kompenziraj ako mlaz nije na ekranu
           jb      WHOLEBORD           ;  ili u Sample modu
           cmp     bx,LASTLINE
           jae     WHOLEBORD
           cmp     SAMPLEON,1
           je      HACKEND
           mov     ax,VIDEOCHG         ; Ovaj ovdje dio pokusava da izvrsi
           sub     ax,VIDEOHCK         ;  kompenzaciju timinga za slucaj kada
           add     ax,ATTRCHG          ;  se smjesta nesto u Low RAM (sve je
           sub     ax,ATTRHCK          ;  zasnovano heuristicki), ili je PC u
           mov     ah,byte ptr RCLK    ;  Low RAM-u
           sub     ah,OLDR
           and     ah,127
           add     ax,ax
           cmp     PC,16384            ; Testiraj da li je PC u Low Ram-u
           jb      NOLOWRAM
           cmp     PC,32768
           jae     NOLOWRAM
           add     al,ah               ; Kompenzacija za PC u Low Ram-u
NOLOWRAM:  add     al,HACKDELAY
           add     byte ptr RCLK+1,al
HACKEND:   mov     ax,BRDCOL           ; Zapocni iscrtavanje jedne scan linije
           stosw                       ;  sa lijeve i desne strane PAPER-a
           stosw
           stosw
           stosw
           add     di,64
           stosw
           stosw
           stosw
           stosw
           cmp     VIDSYNC,-1          ; Pozovi MultColor ako je ukljucen
           je      DOMCL
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
           jmp     short HCBDONE
DOMCL:     push    ds
           call    MULTCOLOR
           pop     ds
           jmp     short HCBDONE
WHOLEBORD: mov     ax,BRDCOL           ; Crtanje scan linije na gornjem ili
           push    cx                  ;  donjem dijelu BORDER-a
           mov     cx,40
           rep     stosw
           cmp     VIDSYNC,-1
           je      WHBSKIP
           add     di,32768-80
           mov     cx,40
           rep     stosw
WHBSKIP:   pop     cx
HCBDONE:   mov     ax,VIDEOCHG         ; Azuriraj stanja
           mov     VIDEOHCK,ax
           mov     ax,ATTRCHG
           mov     ATTRHCK,ax
           mov     ah,byte ptr RCLK
           mov     OLDR,ah
           pop     es
           pop     di
           pop     bx
           ret

;
; MultiColor emulacija.
;

MULTCOLOR: pusha
           if      VER_128
           test    MMUSTATE,8
           jz      MC_NOALT
           mov     cx,ZXDSEG+2048
           mov     ah,MMUSTATE
           and     ah,7
           cmp     ah,7
           je      MC_NOSHAD
           mov     cx,ZXEXTRA2+2048
MC_NOSHAD: mov     ds,cx
           endif
MC_NOALT:  add     bx,bx
           mov     si,ADR_A[bx-2*FIRSTLINE]
           mov     di,ADR_A[bx+384-2*FIRSTLINE]
           mov     bp,ADR_A[bx+768-2*FIRSTLINE]
           dec     di
           lea     bx,ATRCONV
           mov     cx,32
MC_A_LP:   lodsb
           mov     ah,ds:[bp]
           cmp     al,128
           xlat    ATRCONV
           jb      MC_NOFLSH
           test    FL_CREG,40H
           jz      MC_NOFLSH
           rol     al,4
MC_NOFLSH: xchg    al,ah
           stosw
           inc     bp
           loop    MC_A_LP
           popa
           ret

;
; Obrada vremenskog interrupta. Azuriraju se neki brojaci, i simulira se AY
; zvuk i FLASH.
;

INT_8:     push    ax
           if      VER_128
           call    SNDINT              ; Na 128 verziji simuliraj AY
           shr     MAXSNDVAL,1
           cmp     BLASTADDR,0
           je      NOACKSB
           push    dx                  ; SoundBlaster s vremena na vrijeme
           mov     dx,BLASTADDR        ;  treba "podsjecati" da ne prestane
           add     dx,14               ;  pruzati usluge
           in      al,dx
           pop     dx
           endif
NOACKSB:   in      al,41H              ; Relativno sluajan broj
           rol     al,4
           mov     RANDOM,al
           in      al,41H
           mov     al,20H              ; EOI je potreban prije COUNTERS
           out     20H,al
           call    COUNTERS
           pop ax
           iret

;
; Single-step interrupt. Trazi kraj instrukcije, a kada je pronadje, izvrsava
; Spectrumov interrupt ili funkcijsku tipku.
;

           align   4
SSTRAP:    push    bp                  ; Trazi JMP SS:TABLE1[BX]
           mov     bp,sp
           mov     bp,[bp+2]
           cmp     word ptr cs:[bp+1],0A7FFH
           je      SSCONT
SSFAIL:    pop     bp
           iret

           align   4
SSCONT:    cmp     byte ptr cs:[bp],36H
           jne     SSFAIL
           cmp     word ptr cs:[bp+3],offset SSEG:TABLE1
           jne     SSFAIL
           pop     bp
           and     FLAGSH,0FEH         ; Nadjen je kraj instrukcije
           and     ALTF,0FEFFH
           cmp     FKVEC,0             ; Da li je bila funkcijska tipka
           je      NOFKY
           mov     SPROTECT,0
           call    FKVEC
           mov     FKVEC,0
           jmp     PROCINT
NOFKY:     mov     IFF,0
           add     PC,word ptr HSTATE
           mov     HSTATE,0
           dec     STP
           mov     AUX,PC
           WRITESP AUX_H
           dec     STP
           WRITESP AUX_L
           push    ax
           mov     al,EXTRADELAY
           mov     HACKDELAY,al
           cmp     IMODE,2
           je      IMODE2
           if      VER_128
           cmp     PC,48128            ; Kompenzacija za GotToGet, jedini nama
           jne     NOGTG               ;  poznati MultiColor program u IM 1
           mov     HACKDELAY,225
           endif
NOGTG:     mov     PC,56               ; Simulacija IM 0 ili IM 1
           add     RCLK,12*256
add rclk,128*256 ;;;ovo moze praviti belaj !!! (ne znam zasto mi je ovo???)
           pop     ax
           jmp     short PROCINT
IMODE2:    mov     ah,I                ; Simulacija IM 2
           mov     al,255
           mov     PC,ax
           mov     al,[PC]             ; Ova zaobilazenja u velikom broju
           inc     PC                  ;  slucajeva sprecavaju ruzne interakcije
           mov     ah,[PC]             ;  sa virtualnim drajverima (EMM386)
           mov     PC,ax
           add     RCLK,19*256
           call    IM2HACK
           pop     ax
PROCINT:   mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           sti
           retf    2                   ; Vrati se sa ugasenim T flegom

;
; Hakerluk za multikolor efekte.
;

IM2HACK:   cmp     VIDSYNC,-1          ; Ne radi hakerluke ako nije multikolor
           je      IM2TRY
           test    ULAMODE,4
           jnz     IM2TRY
           ret
IM2TRY:    pusha
           mov     al,[PC]
           cmp     al,195
           jne     IM2NOJP
           mov     PC,[PC+1]
IM2NOJP:   cmp     PC,-1               ; Puno rutina ima rutinu na 65535, ali
           jne     IM2NOFFFF           ;  sve imaju HALT, pa se razlikovanje
           mov     PC,ds:[STP]         ;  vrsi po adresi gdje je HALT
IM2NOFFFF: lea     di,HACKTAB
           mov     cx,HCKTABLEN
IM2HLOOP:  cmp     PC,ss:[di]
           jne     IM2HNEXT
           mov     ax,ss:[di+2]
           add     HACKDELAY,al
           jmp     short IM2HEND
IM2HNEXT:  add     di,4
           loop    IM2HLOOP
           mov     ah,130
IM2HEND:   add     ah,EXTRAKUR
           add     byte ptr RCLK+1,ah
           popa
           ret

;
; Ne znam da li ce ove 2 funkcijske tipke (F11 i F12) biti dokumentovane...
;

INCKUR:    pusha
           call    KLIK
           inc     EXTRAKUR
;mov al,extrakur
;mov ds:16001,al
           popa
           ret

DECKUR:    pusha
           call    KLIK
           dec     EXTRAKUR
;mov al,extrakur
;mov ds:16001,al
           popa
           ret

;
; Interrupt greske prekoracenja segmenta. Javlja se u slucaju instrukcija poput
; LD HL,(65535) ili POP HL ili RET ako je SP=1. Ovaj handler omogucava ispravno
; izvrsavanje i ovakvih specijalnih slucajeva.
;

INT_0D:    push    bp
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
           pop     ax                  ;  (ovo se ne bi trebalo desavati u
           pop     di                  ;  normalnom radu)
           pop     es
           pop     bp
           lea     dx,UNEXPMSG
           jmp     ILL1

;
; DOS interrupt kriticne greske (INT 24H).
;

INT_24:    mov     al,3                ; Signal FAIL
           iret

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

H_GRAPH:   call    HGFAR
           ret

X_GRAPH:   call    XGFAR
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
           lea     ax,WSETUP           ; Preusmjeri tipku F5
           mov     FKADDR[8],ax
           ret

;
; Aktiviranje grafike na EGAMONO (deaktiviranje je kao za Hercules).
;

EM_GRAPH:  mov     ax,15
           int     10H
           ret

;
; Aktiviranje i deaktiviranje grafike na EGA i VGA.
;

V_GRAPH:   call    V_G_FAR
           ret

E_GRAPH:   call    E_G_FAR
           ret

V_TEKST:   mov     ax,1202H
           mov     bl,30H
           int     10H
E_TEKST:   mov     ax,3
           int     10H
           ret

;
; Multiplex handler (INT 2F)
;

LAST_2F_XX dd      ?                   ; Mora biti adresabilna preko CS a ne SS
                                       ;  jer MPX moze nastupiti bilo kad!!!
INT_2F:    pushf
           cmp     ax,4002H
           je      SWITCH
           popf
           jmp     LAST_2F_XX
SWITCH:    pusha                       ; Task switch na emulator
           push    ds
           push    es
           mov     ax,ss               ; Ovo je kontola za nedaj Boze
           cmp     ax,STEKSEG
           jne     HUHU
           call    GRAPH
           call    REPAINT
           mov     TSKSWTCH,1
HUHU:      pop     es
           pop     ds
           popa
           popf
           iret

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
           mov     al,NAMJERNA
           xor     ah,ah
           ret
DETCONT:   call    DETECTCRD           ; Far poziv
           xor     ah,ah
           ret

;
; Nije nadjena graficka kartica.
;

BADCARD:   call    CLS
           lea     dx,BCMESS
           mov     ds,STEKSEG
           mov     ah,09H
           int     21H
           jmp     ILLEND

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
; Snapshot taster (F7).
;

SNAPSH:    pusha
           call    KLIK
           call    RESTTIMER
           call    RESTKBD
           popa
           pusha
           call    SAVESNAP
           call    SETKBD
           call    SETTIMER
           popa
           ret

;
; Tape manager (F2).
;

TMANAGER1: pusha
           call    RSTPIC
           if      VER_128
           call    STOPADLIB
           endif
           call    TMANAGER            ; Far poziv
           if      VER_128
           call    INITADLIB
           endif
           popa
           ret

;
; Rutina za ostvarenje autostarta.
;

AUTORUN:   test    AFLAG,1
           jnz     DOARUN
           ret
DOARUN:    mov     ah,INITATTR
           mov     HALFROW[57],8
           if      VER_128
           mov     byte ptr es:[16],201
           mov     word ptr es:[07B6H],01EDH
           mov     byte ptr es:[22E9H],201
           mov     byte ptr ds:[16],201
           mov     word ptr ds:[602],01EDH
           mov     ds:[520],ah
           and     ah,56
           shr     ah,3
           mov     ds:[535],ah
           mov     ax,02377H
           xor     bl,bl
           cmp     ds:[14307],ax
           jne     ARCH1
           mov     ds:[14307],bl
           or      CHGFLAGS,1
ARCH1:     cmp     ds:[14437],ax
           jne     ARCH2
           mov     ds:[14437],bl
           or      CHGFLAGS,2
ARCH2:     cmp     ds:[14345],ax
           jne     ARCH3
           mov     ds:[14345],bl
           or      CHGFLAGS,4
ARCH3:     cmp     ds:[14470],ax
           jne     ARCH4
           mov     ds:[14470],bl
           or      CHGFLAGS,8
ARCH4:     xor     PC,PC
           else
           xor     B,B
           mov     byte ptr ds:[16],201
           mov     word ptr ds:[07B6H],01EDH
           mov     word ptr ds:[1254H],01EDH
           add     byte ptr ds:[12A1H],11
           mov     ds:[1266H],ah
           mov     ds:[1253H],B
           xor     HL,HL
           mov     PC,11EFH
           mov     I,63
           endif
           ret

;
; Escape kod za autorun (#ED #01).
;

ARESC:     db      3 dup (90H)
           inc     R
           inc     PC
           cmp     PC,07B8H
           je      RESTCHGS
           cmp     PC,1256H
           je      STUFFLOAD
           if      VER_128
           cmp     PC,604
           je      STUFFENT
           endif
           jmp     GETINST
STUFFLOAD: mov     word ptr ds:[23641],HL
           mov     AUX,HL
           mov     byte ptr [AUX],239           ; LOAD
           inc     HL
           mov     word ptr ds:[AUX+1],'""'
           add     HL,2
           jmp     GETINST
           if      VER_128
STUFFENT:  mov     byte ptr ds:[23560],13
           mov     byte ptr es:[23560],13
           or      byte ptr ds:[23611],32
           or      byte ptr es:[23611],32
           mov     A,INITATTR
           jmp     GETINST
           endif
RESTCHGS:  if      VER_128
           mov     byte ptr ds:[16],195
           mov     word ptr ds:[07B6H],0E1D7H
           mov     byte ptr ds:[22E9H],205
           mov     byte ptr es:[16],239
           mov     byte ptr es:[520],56
           mov     word ptr es:[602],383EH
           mov     byte ptr es:[535],7
           push    ax
           push    es
           mov     ax,ZXEXTRA2
           mov     es,ax
           mov     byte ptr es:[60431],56
           mov     byte ptr es:[60433],56
           pop     es
           mov     ax,02377H
           test    CHGFLAGS,1
           jz      WASCH1
           mov     es:[14307],ax
WASCH1:    test    CHGFLAGS,2
           jz      WASCH2
           mov     es:[14437],ax
WASCH2:    test    CHGFLAGS,4
           jz      WASCH3
           mov     es:[14345],ax
WASCH3:    test    CHGFLAGS,8
           jz      WASCH4
           mov     es:[14470],ax
WASCH4:    pop     ax
           else
           mov     byte ptr ds:[16],195
           mov     word ptr ds:[07B6H],0E1D7H
           mov     byte ptr ds:[1253H],34
           mov     word ptr ds:[1254H],23641
           mov     byte ptr ds:[1266H],56
           or      byte ptr ds:[23611],8
           sub     byte ptr ds:[12A1H],11
           endif
           mov     HALFROW[57],7
           mov     A,13
           sub     PC,2
           jmp     GETINST

;
; Prikaz naslovne slike.
;

SHOWLOGO:  test    SLFLAG,1            ; Ne crtaj sliku ako je /N
           jnz     DOSHOW
           cmp     SNLOAD,1
           je      SHOWRET
           call    REPAINT             ; Ne moze jmp jer je REPAINT far ptr
           ret
DOSHOW:    push    es
           cli
           mov     ax,LOGOSEG          ; Prebaci sliku u video memoriju
           mov     ds,ax
           xor     si,si
           mov     di,16384
           mov     cx,3456
           cld
           mov     es,MEMSEG
           rep     movsw
           pop     es
           mov     ds,MEMSEG
           mov     LASTB,0100H
           call    REPAINT             ; Iscrtaj sliku
           sti
SHLOOP:    mov     ah,1                ; Cekaj pritisak na taster
           int     16H
           pushf
           xor     ah,ah
           int     16H
           popf
           jnz     SHLOOP
SHOWRET:   ret

;
; Ucitavanje ROM-a ZX Spectruma.
;

ROMLOAD:   push    ds
           mov     ax,3D00H

           if      VER_128

           lea     dx,IME1
           int     21H                 ; Open a file
           jnc     RLOAD1
           jmp     RERROR
RLOAD1:    mov     cx,16384
           mov     bx,ax
           mov     ds,MEMSEG
           xor     dx,dx
           mov     ah,03FH
           int     21H                 ; Read block
           jumps
           jc      RERROR
           cmp     ax,16384
           jne     RERROR
           nojumps
           mov     ah,3EH
           int     21H                 ; Close a file
           mov     ds,STEKSEG
           lea     dx,IME2
           mov     ax,3D00H
           int     21H
           jc      RERROR
           mov     ds,ALTMEMSEG

           else

           lea     dx,IME
           int     21H                 ; Open a file
           jc      RERROR
           mov     ds,MEMSEG

           endif

           mov     cx,16384
           mov     bx,ax
           xor     dx,dx
           mov     ah,03FH
           int     21H                 ; Read block
           jc      RERROR
           cmp     ax,16384
           jne     RERROR
           mov     ah,3EH
           int     21H                 ; Close a file
           mov     ds,STEKSEG
           lea     dx,IMEI1
           mov     ax,3D00H
           int     21H                 ; Open a file
           jc      RLEXIT
           mov     bx,ax
           cmp     ZXI1OLD,1
           jne     RLNEWI1
           mov     ax,4200H            ; Napravi LongSeek za stari ROM
           xor     cx,cx
           mov     dx,8192
           int     21H
RLNEWI1:   mov     cx,8192
           mov     ax,ZXI1ROM
           mov     ds,ax
           xor     dx,dx
           mov     ah,03FH
           int     21H                 ; Read block
           jc      RLEXIT
           cmp     ax,8192
           jne     RLEXIT
           mov     ah,3EH
           int     21H                 ; Close a file
           mov     ax,ZXI1ROM
           mov     es,ax
           cld
           mov     cx,4096
           xor     si,si
           mov     di,8192
           rep     movsw
           inc     ZXI1P
           pop     ds
           ret
RLEXIT:    pop     ds
           cmp     OPTIONE,1
           je      RIERROR
           ret

RERROR:    lea     dx,LRERROR          ; ROM nije nadjen
           jmp     short RERROR1
RIERROR:   lea     dx,LRIERROR         ; Shadow ROM nije nadjen
RERROR1:   mov     ds,STEKSEG
           push    dx
           call    CLS
           pop     dx
           mov     ah,09H
           int     21H
           jmp     short ILLEND
           endp

;
; Ispis poruke o gresci koja sadrzi i adresu.
;

ILL1:      sub     PC,2                ; Ispis raporta za gresku koji u sebi
           push    PC                  ;  sadrzi adresu (i mozda stranicu)
           push    dx
           call    TERMINATE
           mov     ds,STEKSEG
           mov     ah,9
           pop     dx
           int     21H
           pop     PC
           push    PC
           call    CIFRA
           call    CIFRA
           call    CIFRA
           call    CIFRA
           pop     PC
           cmp     PC,16384            ; Da li je SHADOW ROM?
           if      VER_128
           jae     ILLRAM
           else
           jae     ILLEND
           endif
           cmp     SHADOW,1
           jne     NOSHADOW
           lea     dx,SHADTXT
           mov     ah,09H
           int     21H
           jmp     short ILLEND
NOSHADOW:  if      VER_128             ; Na verziji 128 postoje 2 ROM-a
           mov     dl,MMUSTATE
           test    dl,16
           lea     dx,STDTEXT
           jnz     STDROM
           lea     dx,DERBYTXT
STDROM:    mov     ah,09H
           int     21H
           jmp     short ILLEND
ILLRAM:    cmp     PC,49152            ; Test stranice RAMA za 128 verziju
           jb      ILLEND
           mov     dl,MMUSTATE
           and     dl,7
           push    dx
           lea     dx,PTEXT
           mov     ah,09H
           int     21H
           pop     dx
           add     dl,'0'
           mov     ah,02H
           int     21H
           mov     dl,')'
           int     21H
           endif
ILLEND:    call    CLRKBD
           mov     dl,13               ; Ispisuje CR LF
           mov     ah,2
           int     21H
           mov     dl,10
           int     21H
           if      VER_128
           call    STOPADLIB
           endif
           mov     ax,4C00H            ; Kraj programa
           int     21H

CIFRA:     rol     PC,4                ; Ispisuje jednu cifru
           mov     dx,PC
           and     dl,0FH
           cmp     dl,9
           ja      CIF1
           add     dl,'0'
           jmp     short CIF2
CIF1:      add     dl,'A'-10
CIF2:      mov     ah,02H
           int     21H
           ret

TERMINATE: push    0                   ; Dovodi sistem u normalno stanje
           popf
           mov     al,20H
           out     20H,al
           call    RESTVEC
           call    TEKST
           cmp     MPXINST,1
           jne     TERM1
           mov     ax,4007H
           int     2FH
TERM1:     in      al,61H
           and     al,0FCH
           out     61H,al
           ret

CLRKBD:    mov     ah,1                ; Prazni bafer tastature
           int     16H
           jz      CLRKEND
           xor     ah,ah
           int     16H
           jmp     CLRKBD
CLRKEND:   ret

;
; Handler za desnu tipku misa.
;

RMOUSEHND: mov     FKVEC,offset EXIT   ; Desni klik je isto sto i F10
           retf

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
           lea     cx,ENDSS            ; NE ZNAM ZASTO, ALI ASEMBLER NIJE ZNAO
           sub     cx,di               ;  ISPRAVNO DA OVO ODUZME SA '-' !!!
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
           shr     PAGEWORDS,1
           mov     M386_4,66F3H
           mov     M386_5,66F3H
           mov     M386_6,66F3H
           .286p
           smsw    ax
           .286
           test    ax,1
           jz      CPU286
           mov     VIRTMODE,1
CPU286:    push    ds
           mov     PSPSEG,ds
           mov     ds,STEKSEG
           mov     ah,19H              ; Tekuci drajv
           int     21H
           mov     DEFDRIVE,al
           xor     dl,dl               ; Tekuci direktorij
           mov     ah,47H
           lea     si,DEFDIR[1]
           mov     byte ptr [si-1],'\'
           int     21H
           pop     ds
           mov     FLAGS,512
           mov     ALTF,512
           call    READENV             ; Procitaj WARAJEVO environment varijablu
           call    PSPSCAN             ; Skaniraj komandne parametre
           mov     ds,STEKSEG

           assume  ds:SSEG

           if      VER_128
           cmp     MXSPD,1             ; Zabrani /SB u MaxSpeed modu
           jne     NODISSB
           mov     BLASTADDR,0
           endif
NODISSB:   test    ULAMODE,4           ; Zabrani /QX ako nije HiCol border
           jnz     NODISHCR
           mov     ax,OLDVIDSYNC
           mov     VIDSYNC,ax
NODISHCR:  cmp     VIRTMODE,1          ; Da li je virtuelni mod
           jne     NOVIRTM
           cmp     VIRTWARN,1
           je      NOVIRTM
           lea     si,VIRTMSG          ; Ispisi upozorenje u prozoru
           mov     dh,7
VMLP1:     mov     dl,20
VMLP2:     mov     ah,2
           xor     bh,bh
           int     10H
           mov     cx,1
           mov     bx,70H
           lodsb
           mov     ah,9
           int     10H
           inc     dl
           cmp     dl,60
           jbe     VMLP2
           inc     dh
           cmp     dh,17
           jbe     VMLP1
           mov     dx,1900H            ; Ukloni kurzor
           xor     bh,bh
           mov     ah,2
           int     10H
           call    CLRKBD
           xor     ah,ah               ; Testiraj taster
           int     16H                 
           cmp     al,27
           jne     NOVIRTM
           call    CLS                 ; Prekini ako se pritisne ESC
           mov     ax,4C00H
           int     21H
NOVIRTM:   call    ROMLOAD             ; Ucitaj ROM
           cmp     CHGDRDIR,1          ; Da li je mijenjan drajv ili direktorij?
           jne     NOCHDRDIR
           lea     dx,DIRNAME
           cmp     DIRNAME[1],':'
           jne     ONLYDIR
           mov     al,DIRNAME[0]       ; Postavljanje aktivnog drajva
           or      al,32
           sub     al,'a'
           push    dx
           mov     dl,al
           mov     ah,0EH
           int     21H
           pop     dx
           cmp     DIRNAME[2],0
           je      NOCHDRDIR
ONLYDIR:   mov     ah,3BH              ; Postavljanje aktivnog direktorija
           int     21H
           jnc     NOCHDRDIR
           call    DEFDD               ; Direktorij nije nadjen
           lea     dx,DIRERMSG
           jmp     RERROR1
NOCHDRDIR: mov     cx,10               ; Parametri misa
           mov     dx,10
           mov     ax,15
           int     33H
SETTAPE:   lea     dx,SEARCHREC
           mov     ah,1AH
           int     21H                 ; Set DTA poziv DOS-a
           mov     ah,4EH
           lea     dx,TMNAME           ; Sadrzi '*.TAP' ako nema /T opcije
           xor     cx,cx
           int     21H                 ; Find first poziv DOS-a
           jnc     TFOUND
           cmp     ax,18
           je      NOTAPE
           cmp     ax,2
           je      NOTAPE
           jmp     PSPTERR
NOTAPE:    call    CREATETAPE          ; Kreiraj praznu traku
           jnc     SETTAPE
           jmp     PSPTERR
TFOUND:    mov     ax,3501H            ; Sacuvaj vektore
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
           mov     ax,352FH
           int     21H
           mov     word ptr LAST_2F,bx
           mov     word ptr LAST_2F_XX,bx
           mov     word ptr LAST_2F+2,es
           mov     word ptr LAST_2F_XX+2,es
           mov     ax,35FAH
           int     21H
           mov     word ptr LAST_FA,bx
           mov     word ptr LAST_FA+2,es
           mov     ds,MEMSEG

           assume  ds:ZXDSEG

           call    AUTODET
           mov     es,VIDEOSEG
           call    GRAPH
           cmp     VGAOK,1             ; Sprijeciti HiCol BORDER ako nije
           je      HCBALLOW            ;  dozvoljen
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
NOSETATM:  call    SHOWLOGO
           mov     ds,MEMSEG
           if      VER_128
           mov     ENVLEN,100
           endif
           mov     HSTATE,0
           cmp     SNLOAD,1            ; Da li treba ucitati snapshot ?
           jumps
           jne     SETPC0
           nojumps
           if      VER_128
           mov     es,ALTMEMSEG
           endif
           call    LOADSNAP
           cmp     SNAPERR,0
           je      SETREGS
ERRINSNA:  call    RESTVEC
           call    TEKST
           jumps
           lea     dx,NOSNAPERR
           cmp     SNAPERR,3
           je      RERROR1
           lea     dx,HRDWERMSG
           cmp     SNAPERR,2
           je      RERROR1
           nojumps
           lea     dx,SNPERRMSG
           jmp     RERROR1
SETREGS:   call    RECOVSNAP
           pusha
           push    es
           if      VER_128             ; Na 128 verziji treba popuniti i
           mov     si,16384            ;  paralelnu stranicu memorije
           mov     di,si
           mov     cx,24576
           rep     movsw
           endif
           cli
           call    SETVEC
           mov     es,SNAPSEG

           assume  es:ZXEXPBUF

           cmp     SNAPHWMOD,1
           if      VER_128
           je      TESTZXI1
           mov     ah,SNAPHWMOD
           sub     ah,LUNTV30
           cmp     ah,4
           else
           cmp     SNAPHWMOD,1
           endif
           jne     CHECKSTA
TESTZXI1:  cmp     ZXI1P,0             ; Aktivira ZX INTERFACE 1
           mov     OPTIONE,1
           mov     SNAPERR,2
           jz      ERRINSNA
           call    SETZXI1
           cmp     SNAPIN1ON,255
           jne     CHECKSTA
           inc     PAGED
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
           call    CURANDBOR
           call    MMU
           else
           call    CURANDBOR
           endif
           call    REPAINT
           mov     cx,DATNUM           ; auto pozicioniranje
           jcxz    NOMOVEPOS
           call    NTH
NOMOVEPOS: popa
           sti
           cmp     PAGED,1
           je      STIN1PGD
           jmp     GETINST             ; Skace na FETCH
STIN1PGD:  jmp     DOSHADOW

SETPC0:    cli
           call    SETVEC
           xor     PC,PC               ; Postavlja inicijalne registre
           mov     PVN,0
           mov     IFF,0
           mov     IMODE,0
           mov     I,0
           mov     R,0
           mov     R7,0
           call    ADJUSTTBL
           if      VER_128
           mov     es,ALTMEMSEG
           endif
           call    CURANDBOR
           mov     cx,DATNUM           ; auto pozicioniranje
           jcxz    STANDPOS
           call    NTH
STANDPOS:  call    AUTORUN
           sti
           jmp     GETINST

CURANDBOR: mov     al,INVCURS          ; Obrada opcije /K
           or      al,al
           jz      NOCHCUR
           if      VER_128
           mov     es:[6355],al
           else
           mov     ds:[6355],al
           endif
NOCHCUR:   ret

;
; Analiziranje parametara komandne linije. Parametri su:
;
; /!              Ne ispisuje poruku upozorenja za 'virtual 8086' mod ???
; /#<num>;<name>  Postavlja kertridz datoteku <name> u mikrodrajv <num>
; /#N;<name>      Definise Network datoteku; iza imena se mogu navoditi
;                 parametri <delay>, <busylen> i <freelen> tako da je
;                 najopstiji oblik /#N;<name>;<delay>;<busylen>;<freelen>
; /@<time>        Prekid rada emulatora nakon <time> frame-ova
; /@@<time>       Isto, ali poziva i okruzenje uz snimanje TEMP.Z80
; /&<string>      Razne opcije za okruzenje
; /%<num>         Emulacija na odredjenoj brzini
; /%MAX           Emulacija na maksimalnoj dopustivoj brzini
; /%LMAX          Maksimalna brzina pri ucitavanju nestandardnim rutinama
; /?128           Testira da li program koristi 128 portove
; /?86            Sprijecava 'escape' sekvence
; /2              Simulacija verzije ZX Spectrum +2 (128)
; /A              Autostart
; /A-             Ponistenje autostarta
; /B              Ubrzani BASIC
; /C              Ucitavanje iz konfiguracione datoteke SPECSIM.CFG
; /D<drive:dir>   Postavljanje default diska i direktorija
; /E              Aktiviranje ZX interfacea 1 uz preklapanje 8K
; /EX             Aktiviranje ZX interfacea 1 uz preklapanje svih 16K
; /F<blknum>      Pozicioniranje na blok broj <blknum> na aktivnoj traci
; /JA             Simulacija Kempston joysticka analognim joystickom
; /JB             Simulacija Sinclair joysticka B kurzorima
; /JC             Simulacija cursor joysticka kurzorima
; /JK             Simulacija Kempston joysticka kurzorima
; /JM             Simulacija Kempston joysticka misem
; /JS             Simulacija Sinclair joysticka A kurzorima
; /K              Inverzni kurzor
; /L<mode>        Izbor algoritma za rad sa trakom
; /M<mode>        Izbor nacina za simulaciju atributa
; /N              Bez prikaza naslovne slike
; /O              Postavlja filter za trake
; /PE             ZX printer na Epson stampacu, standardno
; /PB             ZX printer na Epson stampacu, uvecano
; /PL             ZX printer na HP Laser Jet stampacu, standardno
; /PP             ZX printer na HP Laser Jet stampacu, uvecano
; /Q<sync>        Postavljanje video sinhronizacije na Scan liniju 0-311
; /QX[<delay>]    MultiColor mod, <delay> je u rasponu -128 - +127
;                 Ima i /QX<delay>;<extra>
; /R              Obnavljanje stanja iz datoteke SNAP.Z80
; /R<file>        Obnavljanje stanja iz datoteke <file>
; /S<mask>        Maska kanala za AY chip (128)
; /SA             AY emulacija preko AdLib kartice
; /SO             AY emulacija na starim AdLib karticama
; /SB             AY i MIDI emulacija preko Sound Blaster-a
; /SB<addr>       Kao /SB, ali uz zadavanje bazne adrese SB-a
; /T<tapename>    Izbor aktivne trake na <tapename>
; /U<status>      Izbor statusa za ULA chip
; /VC             Forsiranje CGA kartice
; /VE             Forsiranje EGA kartice
; /VH             Forsiranje Hercules kartice
; /VM             Forsiranje EGAMONO kartice
; /VV             Forsiranje VGA kartice
; /VX             Aktiviranje Extended Hercules rezima
; /VW             Aktiviranje CGA rezima pogodnog za izvodjenje pod Windowsom
; /Y<attr>        Postavljanje inicijalnog atributa za /A
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

PSPCONF:   call    CONFFILE            ; Uzima iz komandne linije
           jmp     PSPSCAN

SETPLUS2:  if      VER_128             ; Postavlja verziju +2
           lea     di,IME1             ; Na +2 su drugi ROM-ovi
           lea     si,IMEP1
           mov     cx,IMEP1-IME1
           cld
           mov     es,STEKSEG
           rep     movs byte ptr es:[di],es:[si]
           mov     PLUS2FLG,1
           endif
           jmp     PSPC1

EXPAND:    mov     OPTIONE,1           ; Aktivira ZX INTERFACE 1
           mov     ZXI1OLD,0
           inc     bx
           mov     al,[bx]
           or      al,32
           cmp     al,'o'              ; Da li se koristi stari ROM
           jne     NOOLDROM
           mov     ZXI1OLD,1
           inc     bx
           mov     al,[bx]
           or      al,32
NOOLDROM:  cmp     al,'x'              ; Da li se preklapa svih 16K ?
           jne     EXPPART
           cmp     EXPE,1
           je      DOEXP
           mov     EXPE,1
           shl     PAGEWORDS,1
           jmp     short DOEXP
EXPPART:   dec     bx
DOEXP:     call    SETZXI1
           jmp     PSPC1

OBSOLETE:  lea     dx,OBSERMSG         ; Zastarjele opcije (/W i /I)
           jmp     RERROR1

ACCBASIC:  call    SETRST24            ; Ubrzavanje BASIC-a
           mov     FBASIC,1
           jmp     PSPC1

HIDEBORD:  jmp     PSPC1               ;;; obsolete

SETFILTER: mov     FILTER,1            ; Postavlja filter za trake
           jmp     PSPC1

NOWARNING: mov     VIRTWARN,1          ; Bez upozorenja za 'virtual' mod
           cmp     byte ptr [bx+1],'0'
           jumps
           jb      PSPC1
           cmp     byte ptr [bx+1],'9'
           ja      PSPC1
           nojumps
           call    GETPSPNUM
           and     dl,3
           mov     WINTROUB,dl
           jmp     PSPC1

TESTING:   inc     bx                  ; Provjera koristi li program 128 portove
           cmp     word ptr [bx],'68'  ; Testira da li je /?86
           jumps
           jne     CHECK128
           mov     NOESCAPE,1          ; Sprijecavanje Escape sekvenci
           inc     bx
           jmp     PSPC1

SETSPEED:  mov     ax,[bx+1]
           or      ax,2020H
           cmp     ax,'am'
           je      short MAYBEMAX
           cmp     ax,'ml'
           je      short MAYBELMAX
           call    GETPSPNUM           ; Postavljanje brzine rada
           or      dx,dx
           je      PSPERROR
           mov     SPEEDPRCT,dx
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
           mov     ax,22800            ; Srediti brzinu i za SoundBlaster
           xor     dx,dx
           div     cx
           sub     ax,28
           mov     SBSPEED,ax
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
MAYBELMAX: mov     ax,[bx+3]           ; Superbrzo ucitavanje
           or      ax,2020H
           cmp     ax,'xa'
           jne     PSPERROR
           mov     MAXLOAD,1
           add     bx,4
           jmp     PSPC1

SETSYNC:   mov     al,[bx+1]           ; Video sinhronizacija
           or      al,20H
           cmp     al,'x'
           je      short SETMCL
           call    GETPSPNUM           ; Postavka video sinhronizacije
           cmp     dx,311
           ja      PSPERROR
           mov     VIDSYNC,dx
           mov     OLDVIDSYNC,dx
           jmp     PSPC1
SETMCL:    mov     ax,VIDSYNC          ; Postavka MultiColor moda
           mov     OLDVIDSYNC,ax
           mov     VIDSYNC,-1
           mov     EXTRADELAY,0
           inc     bx
           cmp     byte ptr [bx+1],'-' ; Ispitivanje parametra iza /QX
           pushf
           jne     short SMCLNNEG
           inc     bx
SMCLNNEG:  cmp     byte ptr [bx+1],'0'
           jb      short DEFMCL
           cmp     byte ptr [bx+1],'9'
           ja      short DEFMCL
           call    GETPSPNUM
           cmp     dx,128
           ja      PSPERROR
           mov     EXTRADELAY,dl
DEFMCL:    popf
           jne     short SETMCCONT
           neg     EXTRADELAY
SETMCCONT: inc     bx
           cmp     byte ptr [bx],';'
           jne     PSPC2
           call    GETPSPNUM
           cmp     dx,256
           jae     PSPERROR
           mov     EXTRAKUR,dl
           jmp     PSPC1

CHECK128:  cmp     byte ptr [bx],'1'   ; Test 128 opcija
           jne     PSPERROR
           cmp     word ptr [bx+1],'82'
           jne     PSPERROR
           nojumps
           add     bx,2
           if      VER_128
           jmp     PSPC1
           else
           mov     TESTFLG,1
           jmp     PSPC1
           endif

SETAYMASK: inc     bx                  ; Maskiranje AY zvucnih kanala
           mov     EXTRABEEP,0
           mov     al,[bx]
           cmp     al,'a'
           je      ADLIB
           cmp     al,'A'
           je      ADLIB
           cmp     al,'o'
           je      OLDADLIB
           cmp     al,'O'
           je      OLDADLIB
           cmp     al,'b'
           je      BLASTER
           cmp     al,'B'
           je      BLASTER
           cmp     al,'x'
           je      SETEXTBP
           cmp     al,'X'
           je      SETEXTBP
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

SETEXTBP:  mov     EXTRABEEP,1         ; ExtraBeeper mod
           jmp     PSPC1

OLDADLIB:  mov     OPLCOUNT,35
ADLIB:     mov     BLASTPRES,1         ; AdLib ili SB
           mov     BLASTADDR,0
           jmp     short DOINITADL

           else

SETEXTBP:                              ; Na 48 verziji /SA, /SO, /SB i /SX
OLDADLIB:                              ;  prolaze sintaksno, ali se ignori็u
ADLIB:     jmp     PSPC1

           endif

BLASTER:   cmp     byte ptr [bx+1],'0'
           jb      DFLTBLST
           cmp     byte ptr [bx+1],'9'
           ja      DFLTBLST
           call    GETPSPNUM
           mov     BLASTPRES,1
           mov     BLASTADDR,dx
           call    DSPRESET
           cmp     byte ptr [bx+1],';'
           jne     DOINITADL
           inc     bx
           call    GETPSPNUM
           cmp     dx,3
           jumps
           ja      PSPERROR
           nojumps
           mov     DMACH,dl
DOINITADL: if      VER_128
           call    INITADLIB
           endif
           jmp     PSPC1

DFLTBLST:  mov     al,36H              ; Parametar /SB, trazi baznu adresu
           out     43H,al              ; Inicijalizacija Timer-a
           xor     al,al
           out     40H,al
           out     40H,al
           mov     dx,0200H            ; Pretraga pocinje od ovog baznog porta
SBSCAN:    add     dx,6                ; Offset za RESET port je 6
           mov     al,1
           out     dx,al               ; Izvrsi reset
           xor     al,al               ; citaj tajmer
           out     43H,al
           in      al,40H              ; Citaj nizi bajt
           mov     ah,al
           in      al,40H              ; Citaj visi bajt
           xchg    ah,al
           mov     cx,ax
SBWAIT:    xor     al,al               ; citaj tajmer
           out     43H,al
           in      al,40H              ; Citaj nizi bajt
           mov     ah,al
           in      al,40H              ; Citaj visi bajt
           xchg    ah,al
           sub     ax,cx
           neg     ax
           cmp     ax,30
           jc      SBWAIT              ; Cekaj neko vrijeme
           xor     al,al
           out     dx,al
           mov     cx,10000
           add     dx,10-6             ; Offset za READ port je 10
IS_DSP:    in      al,dx
           cmp     al,0AAH             ; READY signal
           je      FOUND_DSP
           loop    IS_DSP
           add     dx,16-10            ; Probaj slijedecu baznu adresu
           cmp     dx,02A0H
           jnz     SBSCAN
           jmp     short NOTDETSB
FOUND_DSP: sub     dx,10
           mov     BLASTADDR,dx
           mov     BLASTPRES,1
           jmp     DOINITADL

NOTDETSB:  lea     dx,SBNFND
           mov     ds,STEKSEG
           mov     ah,09H
           int     21H
           jmp     far ptr ILLEND

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

ZXPRINT    macro   SLOVO,VR            ; Makro za SETZXP
           local   NIJE
           cmp     al,SLOVO
           jne     NIJE
           mov     PRTMODE,VR
           jmp     PSPC1
NIJE:
           endm

SETZXP:    inc     bx                  ; Izbor emulacije ZX printera
           mov     al,[bx]
           or      al,32
           ZXPRINT 'e',1
           ZXPRINT 'b',2
           ZXPRINT 'l',3
           ZXPRINT 'p',4
           jmp     PSPERROR

INVERTCUR: mov     INVCURS,12          ; Inverzija kurzora
           jmp     PSPC1

JOYTIP     macro   SLOVO,VR1,VR2       ; Makro za JOYMOUSE
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
           JOYTIP  'n',0,0             ; Kempston nicim (lazni)
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

SETAMODE:  inc     bx                  ; Izbor nacina simulacije atributa
           mov     al,[bx]             ;  (ili izbor palete za /VW)
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

LOADMODE:  inc     bx                  ; Izbor nacina rada sa trakom
           mov     STOP_MODE,0
           mov     al,[bx]
           cmp     al,'0'
           jumps
           jb      PSPERROR
           cmp     al,'7'
           ja      PSPERROR
           cmp     al,'5'
           ja      short LREALT
           cmp     al,'3'
           jb      short NOSTOPM
           mov     STOP_MODE,1
LREALT:    sub     al,3
NOSTOPM:   sub     al,'0'
           xor     ch,ch
           mov     cl,al
           jcxz    PSPC1
           nojumps
LMLOOP:    xor     ax,ax
           call    SETLMODE
           loop    LMLOOP
           jmp     PSPC1

RECOVER:   call    SETSNNAME           ; Obnova snapshot fajla
           mov     SNLOAD,1
           jmp     PSPC1

SETMDV:    mov     ax,[bx+1]           ; Mikrodrajv i Network
           or      al,32
           cmp     ax,';n'
           je      SETNET
           call    MDVINIT             ; Inicijalizacija mikrodrajva
           jnc     MDVBAD
           mov     OPTIONE,1
           call    SETZXI1
           jmp     PSPC1
MDVBAD:    jumps                       ; Razne su greske moguce pri tome, a CL
           cmp     cl,3                ;  sadrzi kod greske
           je      PSPERROR
           call    DEFDD
           mov     ds,STEKSEG
           lea     dx,MDVERRMSG
           cmp     cl,2
           je      RERROR1
           nojumps
           lea     dx,MDVLENERR
           jmp     RERROR1

SETNET:    add     bx,2                ; Inicijalizacija Networka
           lea     di,NETFNAME
           mov     NETPRES,1
SETNLP:    inc     bx
           mov     al,[bx]
           cmp     al,13
           je      SETNEND
           cmp     al,' '
           je      SETNEND
           cmp     al,';'
           je      SETNCONT
           mov     ss:[di],al
           inc     di
           jmp     SETNLP
SETNCONT:  call    GETPSPNUM           ; Uzimanje NET parametara
           mov     NETDELAY,dx
           inc     bx
           cmp     byte ptr [bx],';'
           jne     SETNEND
           call    GETPSPNUM
           mov     BUSYLEN,dx
           inc     bx
           cmp     byte ptr [bx],';'
           jne     SETNEND
           call    GETPSPNUM
           mov     FREELEN,dx
           inc     bx
SETNEND:   mov     byte ptr ss:[di],0
           push    ds
           push    bx
           lea     dx,NETFNAME         ; Otvori NET datoteku (shared)
           mov     ds,STEKSEG
           mov     ax,3D40H
           int     21H
           mov     bx,ax
           jnc     NETDONE
           cmp     ax,5
           je      NETDONE             ; Access denied - vec u upotrebi
           xor     cx,cx
           mov     ah,3CH
           int     21H                 ; NET datoteka ne postoji - kreiraj je
           jc      NETERR
           mov     bx,ax
           mov     ah,40H
           lea     dx,BLOCKID
           mov     cx,260
           int     21H
           jc      NETERR
NETDONE:   mov     ah,3EH              ; Zatvori datoteku
           int     21H
           jc      NETERR
           pop     bx
           pop     ds
           mov     OPTIONE,1
           call    SETZXI1
           jmp     PSPC2
NETERR:    pop     bx
           pop     ds
           lea     dx,NETERMSG
           jmp     RERROR1

SETARUN:   mov     AFLAG,1             ; Signalizacija autostarta
           inc     bx
           cmp     byte ptr [bx],'-'
           je      KILLARUN
           jmp     PSPC2
KILLARUN:  mov     AFLAG,0
           jmp     PSPC1

PSPNOSCR:  mov     SLFLAG,0            ; Sprecavanje iscrtavanja naslovne slike
           jmp     PSPC1

PSPDIR:    lea     di,DIRNAME          ; Zadavanje direktorija i drajva
PSPDL:     inc     bx
           mov     al,[bx]
           cmp     al,13
           je      PSPDIR1
           cmp     al,' '
           je      PSPDIR1
           mov     ss:[di],al
           inc     di
           jmp     PSPDL
PSPDIR1:   mov     byte ptr ss:[di],0
           mov     CHGDRDIR,1
           jmp     PSPC2

ENVPARS:   mov     di,ENVPPTR          ; Postavljanje parametara okruzenja
           or      di,di
           jz      EP1
           mov     ENVPBUF[di],' '
           inc     di
EP1:       mov     word ptr ENVPBUF[di],'&/'
           add     di,2
EPLOOP:    inc     bx
           mov     al,[bx]
           cmp     al,13
           je      EP2
           cmp     al,' '
           je      EP2
           mov     ENVPBUF[di],al
           inc     di
           jmp     EPLOOP
EP2:       mov     ENVPBUF[di],0
           mov     ENVPPTR,di
           jmp     PSPC2

PSPTAPE:   mov     TCLFLAG,1           ; Zadavanje trake
           lea     di,TMNAME
PSPTL:     inc     bx
           mov     al,[bx]
           cmp     al,13
           je      PSPT1
           cmp     al,' '
           je      PSPT1
           cmp     al,'\'
           je      PSPTERR
           cmp     al,'.'
           jne     PSPTNEXT
           inc     bx
           mov     al,[bx]
           or      al,20H
           cmp     al,'t'
           jne     PSPTERR
           inc     bx
           mov     ax,[bx]
           or      ax,2020H
           cmp     ax,'pa'
           jne     PSPTERR
           add     bx,2
           jmp     short PSPT1
PSPTNEXT:  mov     ss:[di],al
           inc     di
           cmp     di,offset SSEG:TMNAME[9]
           je      PSPTERR
           jmp     PSPTL
PSPT1:     mov     word ptr ss:[di],'T.'
           mov     word ptr ss:[di+2],'PA'
           mov     byte ptr ss:[di+4],0
PSPTEND:   jmp     PSPC2
PSPTERR:   call    DEFDD
           mov     ds,STEKSEG
           lea     dx,TCLERMSG
           jmp     RERROR1

PSPFILE:   call    GETPSPNUM           ; Zadavanje pozicije u traci
           mov     DATNUM,dx
           jmp     PSPC1

SETINITA:  call    GETPSPNUM           ; Postavljanje inicijalnog atributa
           or      dh,dh
           jnz     PSPERROR
           mov     INITATTR,dl
           jmp     PSPC1

TIMEEXIT:  cmp     byte ptr [bx+1],'@' ; Prekid nakon odredjenog broja frame-ova
           jnz     EXNOENV
           mov     CALLENV,1
           inc     bx
EXNOENV:   call    GETPSPNUM
           mov     WORKTIME,dx
           jmp     PSPC1

;
; Citanje broja iz komandne linije.
;

GETPSPNUM: xor     dx,dx
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

PSPERROR:  call    DEFDD               ; Greska u skaniranju
           lea     dx,PSPERMSG
PSPER1:    mov     ds,STEKSEG
           jmp     RERROR1

CSEG       ends

;
; Segment za naslovnu sliku i HELP ekran.
;

LOGOSEG    segment public
LOGOSEG    ends

;
; Segment za monitor.
;

MONSEG     segment public
MONSEG     ends

;
; Privremeni stek segment, samo za inicijalizaciju.
;

TEMPSSEG   segment stack 'stack'

           db      20 dup (?)

TEMPSSEG   ends

           end     START
