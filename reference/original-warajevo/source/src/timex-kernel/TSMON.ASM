;
;          WARAJEVO Spectrum Simulator    Release 2.0
;                (C) Zeljko Juric & Samir Ribic
;

           .286

           warn

           extrn   SAVEFORMON:far,LOADFORMON:far
           extrn   RESTTIMER:far,RESTKBD:far
           extrn   SETKBD:far,SETTIMER:far
           extrn   REPAINT:far,PO_CHR:far
           extrn   ULAFAR:far,PO_ONE:far
           extrn   TIMCTLFAR:far,TIMMMUFAR:far

           public  MONITOR,SEGMON,BPADDR,BRALREADY
           public  ENDSS,SSTEPON

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           extrn   TABLE1:word,STEKSEG:word
           extrn   PROGSEG:word,VIDEOSEG:word
           extrn   CHUNKS:byte,DEBUGZONE:byte

;
; Help tekstovi za monitor.
;

HELPTXT    db      'COMMANDS:',0
           db      ' ',0
           db      'A[ramtop][flag]          Abort',0
           db      'B[addr]                  BrkPnt',0
           db      'Cfrom to len             CopyMem',0
           db      'D[from [to [pgl [opt]]]] Disasm',0
           db      'E[addr]                  Execute',0
           db      'F[byt|wrd [byt|wrd ...]] FindMem',0
           db      'H                        ToglHex',0
           db      'I[endaddr]               SnglStp',0
           db      'J[endaddr]               Jump',0
           db      'Kfrom to with            FillMem',0
           db      'L[addr]                  LoadBlk',0
           db      'M[addr [byt [byt ...]]]  MemPtr',0
           db      'N                        IntHndl',0
           db      'O                        OutFile',0
           db      'Pbyte [byte ...]         PutByte',0
           db      'Q                        Quit',0
           db      'R[value]                 RegChg',0
           db      'Sfrom len [name]         SaveBlk',0
           db      'T[from [to [pglen]]]     ListMem',0
           db      'U                        UndoReg',0
           db      'V                        ViewScr',0
           db      'Wwrd [wrd ...]           PutWord',0
           db      'X[code]                  Decrypt',0
           db      'Y[conf]                  MemConf',0
           db      'Z[opt [marg]]            PrntScr',0
           db      '![iff [imode]]           SetIntr',0
           db      '?[num]                   ShowNum',0
           db      '+num1 num2               Add',0
           db      '-num1 num2               Substr',0
           db      '*num1 num2               Mult',0
           db      '/num1 num2               IntDiv',0
           db      '%num1 num2               Modulus',0
           db      '&num1 num2               BtwsAnd',0
           db      '|num1 num2               BtwsOr',0
           db      '^num1 num2               BtwsXor',0
           db      '_                        Refresh',0
           db      ' ',0
           db      'Use TAB key to select active',0
           db      'register (marked with ":")',0
           db      ' ',0
           db      'Arguments may be dec (e.g. 100),',0
           db      'hex (e.g. #F4) or char (e.g. ''a''',0
           db      'or ''xyz'' which is same as ''x'',',0
           db      '''y'',''z''); some commands accept',0
           db      'wildcard ? as argument (which is',0
           db      'coded internally as 65535)',0

;
; Tabele za disasembler.
;

SINGLE     db      255,10,'LD A,(BCb',')'+128
           db      255,26,'LD A,(DEd',')'+128
           db      255,2,'LD (BCb),','A'+128
           db      255,18,'LD (DEd),','A'+128
           db      255,235,'EX DE,H','L'+128
           db      255,8,'EX AF,AF',''''+128
           db      255,217,'EX','X'+128
           db      255,39,'DA','A'+128
           db      255,47,'CP','L'+128
           db      255,63,'CC','F'+128
           db      255,55,'SC','F'+128
           db      255,0,'NO','P'+128
           db      255,118,'HAL','T'+128
           db      255,243,'D','I'+128
           db      255,251,'E','I'+128
           db      255,7,'RLC','A'+128
           db      255,23,'RL','A'+128
           db      255,15,'RRC','A'+128
           db      255,31,'RR','A'+128
           db      255,201,'RE','T'+128
           db      255,233,'JP (@',')'+128
           db      255,249,'LD SP,','@'+128
           db      255,227,'EX (SPs),','@'+128
           db      255,58,'LD A,(>',')'+128
           db      255,50,'LD (>),','A'+128
           db      255,42,'LD @,(>',')'+128
           db      255,34,'LD (>),','@'+128
           db      255,195,'JP ','#'+128
           db      255,24,'JR ','%'+128
           db      255,16,'DJNZ ','%'+128
           db      255,205,'CALL ','#'+128
           db      255,219,'IN A,(*',')'+128
           db      255,211,'OUT (*),','A'+128
           db      255,241,'POP A','F'+128
           db      255,245,'PUSH A','F'+128
           db      255,102,'LD H,','^'+128
           db      255,110,'LD L,','^'+128
           db      255,116,'LD !,','H'+128
           db      255,117,'LD !,','L'+128
           db      255,221,'NOP I','X'+128
           db      255,253,'NOP I','Y'+128
           db      199,196,'CALL ?,','#'+128
           db      199,192,'RET ','?'+128
           db      199,194,'JP ?,','#'+128
           db      192,128,'&','^'+128
           db      199,198,'&','*'+128
           db      199,4,'INC ','!'+128
           db      199,5,'DEC ','!'+128
           db      192,64,'LD !,','^'+128
           db      199,6,'LD !,','*'+128
           db      207,1,'LD |,','#'+128
           db      207,197,'PUSH ','|'+128
           db      207,193,'POP ','|'+128
           db      207,9,'ADD @,','|'+128
           db      207,3,'INC ','|'+128
           db      207,11,'DEC ','|'+128
           db      231,32,'JR .,','%'+128
           db      199,199,'RST ','`'+128

SING_ED    db      255,87,'LD A,','I'+128
           db      255,95,'LD A,','R'+128
           db      255,71,'LD I,','A'+128
           db      255,79,'LD R,','A'+128
           db      255,160,'LD','I'+128
           db      255,176,'LDI','R'+128
           db      255,168,'LD','D'+128
           db      255,184,'LDD','R'+128
           db      255,161,'CP','I'+128
           db      255,177,'CPI','R'+128
           db      255,169,'CP','D'+128
           db      255,185,'CPD','R'+128
           db      255,111,'RL','D'+128
           db      255,103,'RR','D'+128
           db      255,162,'IN','I'+128
           db      255,178,'INI','R'+128
           db      255,170,'IN','D'+128
           db      255,186,'IND','R'+128
           db      255,163,'OUT','I'+128
           db      255,179,'OTI','R'+128
           db      255,171,'OUT','D'+128
           db      255,187,'OTD','R'+128
           db      255,246,'ML','T'+128          ; Multilevel load trap
           db      255,251,'ML','T'+128
           db      255,112,'IN F,(C',')'+128
           db      255,113,'OUT (C),','0'+128
           db      215,70,'IM ','0'+128
           db      223,86,'IM ','1'+128
           db      223,94,'IM ','2'+128
           db      199,68,'NE','G'+128
           db      207,77,'RET','I'+128
           db      207,69,'RET','N'+128
           db      199,64,'IN !,(C',')'+128
           db      199,65,'OUT (C),','!'+128
           db      207,75,'LD |,(>',')'+128
           db      207,67,'LD (>),','|'+128
           db      207,74,'ADC HL,','|'+128
           db      207,66,'SBC HL,','|'+128
           db      0,0,'NOP','D'+128

SING_CB    db      192,0,'/~ ','='+128
           db      192,64,'BIT <,','='+128
           db      192,128,'RES~ <,','='+128
           db      192,192,'SET~ <,','='+128

CONDCODES  db      'N','Z'+128,'Z'+128,'N','C'+128,'C'+128
           db      'P','O'+128,'P','E'+128,'P'+128,'M'+128

MNEMCS     db      'ADD  A',','+128,'ADC  A',','+128,'SUB ',' '+128
           db      'SBC  A',','+128,'AND ',' '+128,'XOR ',' '+128
           db      'OR  ',' '+128,'CP  ',' '+128

ROTSHFTS   db      'RL','C'+128,'RR','C'+128,'R','L'+128
           db      'R','R'+128,'SL','A'+128,'SR','A'+128
           db      'SL','L'+128,'SR','L'+128

DBTEXT     db      'DB * ','\'+128
DWTEXT     db      'DW ','#'+128

ADR16      db      'HLIXIY'
RPARS      db      'BCDEHLSP'
REGS       db      'BCDEHL*A'

SPECIAL    db      '@*#>\dbs%!?&^|.`</~= ',0

SPECADDR   dw      HLIXIY,N8,N16,N16ARG,TOKEN
           dw      ARGDE,ARGBC,ARGSP,DISP,R8_1,C2
           dw      OPC,R8_2,RP,C1,RSTN,BITS
           dw      ROTSHFT,SPUT,INDCB,SPACE

;
; Inicijalizirane sistemske varijable (i baferi) monitora.
;

BASE       dw      10                  ; Baza za ispis
SCBASE     dw      10                  ; Baza za skaniranje parametara
DIGITS     db      '0123456789ABCDEF'  ; Cifre
NUMBUF     db      '00000@'            ; Bafer za ispis broja

CURREG     dw      10                  ; Tekuci registar puta 2

HEADER     db      3,14 dup (32),0,128 ; Header za SAVE komandu

FINDCOUNT  db      255                 ; Broj bajtova za pretragu
FBYTES     dw      1,9,9,4,30 dup (0)  ; Bajtovi za pretragu (?=-1)

OUTNAME    db      'MONITOR.OUT',0     ; Ime za OUTPUT fajl

SEGMON     dw      MONSEG              ; Pointer na MONSEG

;
; Konstante za kopiranje ekrana.
;

CLGRCC     db      3,27,'3',36         ; Kodovi za vracanje stampaca u normal

IGRCC1     db      5,27,'*',5,0,2      ; Kodovi za Z0 i Z1 (Z5 i Z6)
CRLFCC1    db      5,27,'3',24,13,10
MHEADCC1   db      1,13

IGRCC2     db      5,27,'*',5,0,1      ; Kodovi za Z2 (Z7)
CRLFCC2    db      5,27,'3',24,13,10
MHEADCC2   db      1,13

IGRCC3     db      5,27,'*',7,0,1      ; Kodovi za Z3 (Z8)
CRLFCC3    db      5,27,'3',23,13,10
MHEADCC3   db      4,27,'3',1,13

IGRCC4     db      5,27,'*',3,0,1      ; Kodovi za Z4 (Z9)
CRLFCC4    db      5,27,'3',23,13,10
MHEADCC4   db      4,27,'3',1,13

;
; Tabele za ispis u OUTPUT datoteku.
;

FIRSTROW   db      'AF    BC    DE    HL    SP    PC    IX',13,10,'@'
SECONDROW  db      13,10,13,10
           db      'IY    IR    AF''   BC''   DE''   HL''   MP',13,10,'@'
THIRDROW   db      13,10,13,10
           db      'ARG   (ARG) SZXHYPNC SZXHYPNC'' IM IJ CTRL',13,10,'@'

THREESPC   db      '   @'
TWELWESPC  db      '            @'
TWOCRLF    db      13,10,13,10,'@'
MCMSG      db      'MC: @'

IMMSG      db      'IM IJ',13,10,'@'
MPMSG      db      'MP',13,10,'@'

;
; Tabele za FRONT PANEL DISPLAY.
;

DBW        macro   WRD,BYT
           dw      WRD
           db      BYT
           endm

PANEL      dw      16448
           db      'AF        AF''          SZXHYPNC '
           db      'BC        BC''       F           '
           db      'DE        DE''       F''          '
           db      'HL        HL''                   '
           db      'SP                  IM I J CTRL '
           db      'PC        MP                @'
           DBW     18432,'IX@'
           dw      18464
           db      'IY        ARG       MC          '
           db      'IR       (ARG)      BP@'
           DBW     18574,'(PC)@'
           DBW     18606,'(SP)@'
           DBW     18638,'(HL)@'
           DBW     18670,'(IX)@'
           DBW     20494,'(IY)@'
           DBW     20526,'(DE)@'
           DBW     20558,'(BC)@'
           DBW     20590,'(MP)@'
           DBW     20662,'F1 - Help@'
           DBW     20672,'Type Q to quit@'

ARGCLR     dw      18478
           db      '     @'
ARGMCLR    dw      18512
           db      '   @'

SYNERMSG   dw      20672
           db      'Syntax error@'
COMERMSG   dw      20672
           db      'Invalid command@'
PARERMSG   dw      20672
           db      'Too few parameters@'
OVRERMSG   dw      20672
           db      'Overflow@'
BIGERMSG   dw      20672
           db      'Parameter too big@'
RTPERMSG   dw      20672
           db      'RAMPTOP too low@'
INTERMSG   dw      20672
           db      'No ZX Interface 1@'
FINERMSG   dw      20672
           db      'String not found@'
OUTERMSG   dw      20672
           db      'File output error@'
BPROMMSG   dw      18525
           db      'ROM@'

REGPOS     dw      16451,16483,16515,16547
           dw      16579,16611,18435,18467
           dw      18499,16462,16494,16526
           dw      16558,16622,18478

MEMBYTES   dw      SSEG:STPC,18561,SSEG:STSP,18593
           dw      SSEG:STHL,18625,SSEG:STIX,18657
           dw      SSEG:STIY,20481,SSEG:STDE,20513
           dw      SSEG:STBC,20545,SSEG:STMP,20577

;
; Tabela komandi.
;

COMS       db      'EDHR?+-*/%TMVIQUNA&|^JXPWFLSKCB!YOZ_@',0

COMADDR    dw      MONEXIT,DISS,HEXMODE,SETREG
           dw      CONVNUM,ADDNUM,SUBNUM,MULNUM
           dw      DIVNUM,MODNUM,TABULATE,MWRITE
           dw      VIEWSCR,ISTEP,MONQUIT,UNDO,IMADDR
           dw      ABORT,ANDNUM,ORNUM,XORNUM,JUMP
           dw      SETXOR,POKE,POKEWORD,FINDMEM,LOAD
           dw      SAVE,KILLMEM,COPYMEM,BREAKP
           dw      SETIFFS,MEMPAGE,SETOUTPUT,DUMPSCR
           dw      REFRESH,DEBUG

SSEG1      ends

SSEG2      segment public

           extrn   LASTB:word,STOP_F:word
           extrn   VMADDR:word,INV_FLG:byte
           extrn   ALTHL:word,ALTDE:word,ALTBC:word
           extrn   R:byte,R7:byte,I:byte,IMODE:word
           extrn   IX:word,IY:word,IFF:word
           extrn   ALTA:byte,TIMCTL:byte,TIMSTAT:byte
           extrn   FLAGS:word,INT_STACK:byte
           extrn   ALTF:word,ALTPVN:word,PVN:word
           extrn   FLAGSL:byte,TSKSWTCH:byte
           extrn   FLASH:word,WINTROUB:byte
           extrn   INKPAP:byte

;
; Neinicijalizirani baferi i sistemske varijable monitora.
;

BUFFER     db      32 dup (?)          ; Bafer za disasembliranje
BUFPOS     dw      ?                   ; Pozicija u baferu

MFLAG      db      ?                   ; Razni flegovi za ispis i disasembler
SSFLAG     db      ?                   ; Flegovi za Single-Step
SSTEPON    db      ?                   ; 1 ako je Single Step
DBFLAG     db      ?                   ; Flegovi za ispis DB i DW

STCODE     dw      ?                   ; Adresa instrukcije za disasembliranje
NEXTINS    dw      ?                   ; Adresa slijedece instrukcije

DIGCNT     dw      ?                   ; Broj cifara koje se ispisuju

EDITBUF    db      30 dup (?)          ; Editorski bafer
QUOTE      db      ?                   ; 255 pri ASCII unosu

COMCODE    db      ?                   ; Komanda
PARCOUNT   db      ?                   ; Broj parametara
PARAMS     dw      30 dup (?)          ; Bafer za parametre

DISEND     dw      ?                   ; Krajnja adresa za disasembliranje
DISCNT     dw      ?                   ; Duzina stranice za disasembliranje

TSTACK     db      1000 dup (?)        ; Privremeni spremnik steka
OLDSP      dw      ?                   ; Spremnik za SP

FSTART     dw      ?                   ; Mjesto pocetka pretrage
FINSTEP    dw      ?                   ; Zavrsna tacka za ISTEP

XORBYTE    db      ?                   ; XOR bajt
ARGFLG     db      ?                   ; 1 ako ima memorijski argument

SAVEBP     db      ?                   ; Sadrzaj adrese gdje ide break point
BPADDR     dw      ?                   ; Adresa breakpointa
BRALREADY  db      ?                   ; Status breakpointa

OUTACT     db      ?                   ; 1 ako je OUTPUT aktivan
OUTFLAG    db      ?                   ; 1 ako treba poslati u datoteku, itd.
OUTHANDLE  dw      ?                   ; Handler za OUTPUT fajl
OUTCHAR    db      ?                   ; Znak koji se salje

INDIRECT   dw      ?                   ; Vektor za indirektne pozive rutina
HELPPTR    dw      ?                   ; Pokazivac na help tekst


;
; Varijable za kopiranje ekrana.
;

DUMPMODE   db      ?                   ; Rezim za kopiranje
DUMPBRD    db      ?                   ; 1 ako ima okvir oko ekrana

INITGR     dw      ?                   ; Adresa tabele za aktiviranje grafike
CRLF       dw      ?                   ; Adresa tabele za pomak u novi red
MOVEHEAD   dw      ?                   ; Adresa tabele za fini pomak glave

YSTEP      db      ?                   ; Inkrement po y osi
PASSNUM    db      ?                   ; Broj prolaza minus 1
ROWEND     db      ?                   ; Zadnja linija u redu
ROWSTEP    db      ?                   ; Razmak izmedju linija

MARGIN     dw      ?                   ; Iznos margine

SAVECTL    db      ?                   ; Cuva stanje kontrolnog registra
SAVEMMU    db      ?                   ; Cuva stanje horizontalnog MMU registra

;
; Spremljeni registri.
;

STAF       dw      ?,?
STBC       dw      ?,?
STDE       dw      ?,?
STHL       dw      ?,?
STSP       dw      ?,?
STPC       dw      ?,?
STIX       dw      ?,?
STIY       dw      ?,?
STIR       dw      ?,?
STALTAF    dw      ?,?
STALTBC    dw      ?,?
STALTDE    dw      ?,?
STALTHL    dw      ?,?

STMP       dw      ?,?
STARG      dw      ?

;
; Oprez: SPECMON mora biti zadnji EXE za linkanje zbog steka!
;

ENDSS      label   byte

SSEG2      ends

CSEG       segment public

           extrn   RSTPIC:near

           assume  cs:CSEG,ss:SSEG

MONITOR:   pusha
           call    RSTPIC
           call    RESTTIMER
           call    RESTKBD
           mov     ah,TIMCTL
           mov     SAVECTL,ah
           and     ah,192
           call    TIMCTLFAR
           mov     ah,TIMSTAT
           mov     SAVEMMU,ah
           push    cs
           call    near ptr COLORMON
           popa
           sti
           call    far ptr MONMAIN
           pusha
           and     TIMCTL,192
           mov     ah,SAVECTL
           and     ah,63
           or      ah,TIMCTL
           call    TIMCTLFAR
           call    REPAINT
           call    SETKBD
           call    SETTIMER
           popa
           ret

;
; Slijedi niz procedura koji su intersegmentni interfejs.
;

COLORMON   proc    far
           xor     ah,ah
           push    LASTB
           call    ULAFAR
           pop     LASTB
           call    STOP_F
           ret
COLORMON   endp

FLSHFAR    proc    far
           call    FLASH
           ret
FLSHFAR    endp

CSEG       ends

MONSEG     segment public

           assume  cs:MONSEG,ss:SSEG

;
; Smjestanje u memoriju koje vodi racuna o Timex specificnostima.
;

MEMPOKE:   push    di
           rol     di,3
           and     di,7
           test    CHUNKS[di],1
           pop     di
           jz      MEMROM
           mov     [di],ah
MEMROM:    ret

;
; Glavna petlja monitora.
;

MONMAIN:   cld                         ; Pokupi sve registre
           call    STOREREG
           cmp     BRALREADY,1         ; Da li je bila BREAK point?
           jne     MONITOR1
           mov     bh,1                ; Obrisi prekidnu tacku
           call    BPSUB
MONITOR1:  mov     BRALREADY,0
           lea     di,STAF             ; Napravi kopiju za UNDO
           mov     cx,14
STRLOOP:   mov     si,ss:[di]
           mov     ss:[di+2],si
           add     di,4
           loop    STRLOOP
           call    PANELOUT            ; Posalji panel u OUT datoteku ako treba
           mov     cx,13               ; Prvi put se ispise poruka vise.
           jmp     short PANENTRY1
PANENTRY:  mov     cx,12
PANENTRY1: call    CLRPANEL            ; Iscrtaj staticni dio panela
           lea     di,PANEL
PANLOOP:   call    PO_MSG
           loop    PANLOOP
           mov     bx,CURREG           ; Ispisi pokazivac tekuceg registra
           mov     ax,REGPOS[bx]
           dec     ax
           mov     VMADDR,ax
           mov     dl,':'
           call    PO_CHR1
REGENTRY:  in      al,61H              ; Zaustavi zvuk (ko zna sta se sve
           and     al,0FCH             ;  desavalo)
           out     61H,al
           mov     DBFLAG,0
           mov     si,STPC             ; Odredi ARG.
           call    DISS_ONE
           lea     di,ARGCLR
           call    PO_MSG
           lea     di,ARGMCLR
           call    PO_MSG
           lea     bx,STAF             ; Ispisi registre, MP i ARG
           lea     si,REGPOS
           mov     cx,15
           mov     MFLAG,16
           cmp     ARGFLG,1
           je      REGLOOP
           dec     cx
REGLOOP:   mov     ax,ss:[si]
           mov     VMADDR,ax
           mov     dx,ss:[bx]
           call    NUM16
           add     si,2
           add     bx,4
           loop    REGLOOP
           mov     VMADDR,16503        ; Ispisi flegove
           mov     ah,byte ptr STAF
           call    FLPRINT
           mov     VMADDR,16535
           mov     ah,byte ptr STALTAF
           call    FLPRINT
           cmp     ARGFLG,1
           jne     DISPBP
           mov     VMADDR,18512        ; Ispisi (ARG)
           mov     bx,STARG
           mov     dl,[bx]
           call    NUM8
DISPBP:    cmp     BRALREADY,0         ; Ispisi BP
           je      DISPIM
           mov     VMADDR,18519
           mov     dx,BPADDR
           call    NUM16
           mov     bx,BPADDR
           cmp     bx,16384
           jae     DISPBPRAM
           lea     di,BPROMMSG         ; Ispisi upozorenje ako je BP u ROM-u
           call    PO_MSG
           mov     VMADDR,18493
           mov     dl,'S'              ; Ispisi stranu u kojoj je BP ???
DISPBPRAM: mov     dl,' '
DISPBPPG:  push    dx
;          call    PO_SPACE
           pop     dx
;          call    PO_CHR1
DISPIM:    mov     VMADDR,16629
           mov     ax,IMODE            ; Ispisi stanje interapta
           add     al,'0'
           mov     dl,al
           call    PO_CHR1
           call    PO_SPACE
           mov     ax,IFF
           shr     ax,2
           add     ax,'00'
           mov     dl,al
           call    PO_CHR1
           call    PO_SPACE
           mov     dl,ah
           call    PO_CHR1
           call    PO_SPACE
           call    PO_SPACE
           mov     dl,TIMCTL           ; Ispisi kontrolni registar
           mov     al,SAVECTL
           and     al,63
           and     dl,192
           or      dl,al
           call    NUM8
           mov     VMADDR,18487
           mov     cx,8                ; Ispisi konfiguraciju memorije
           mov     bl,'X'
           mov     al,TIMSTAT
           test    TIMCTL,128
           jnz     CHNKLOOP
           mov     bl,'D'
CHNKLOOP:  mov     dl,bl
           rol     al,1
           jc      NOHOME
           mov     dl,'H'
NOHOME:    call    PO_CHR1
           loop    CHNKLOOP
           push    BASE                ; Ispisi bajtove adresirane registrima
           mov     BASE,16
           mov     MFLAG,48
           lea     si,MEMBYTES
           mov     cx,8
BYTLOOP:   mov     bx,ss:[si]
           mov     bx,ss:[bx]
           sub     bx,6
           mov     ax,ss:[si+2]
           mov     VMADDR,ax
           push    cx
           mov     cx,6
           call    WRTBYTES
           add     ax,18
           mov     VMADDR,ax
           mov     cx,6
           call    WRTBYTES
           add     si,4
           pop     cx
           loop    BYTLOOP
           pop     BASE
           mov     VMADDR,16392        ; Obrisi dio linije sa tekucom
           mov     cx,24               ; instrukcijom da ne ostane djubre
           mov     dl,' '
EOLLOOP:   call    PO_CHR1
           loop    EOLLOOP
           xor     ah,ah               ; Ispisi disasembliranu liniju
           mov     si,STPC
           mov     DBFLAG,0
           call    DISS_LINE
SINENTRY:  cmp     SSFLAG,0            ; Provjeri da li je aktivan kontinualni
           je      EDENTRY             ;  single-step. Ako jeste, izvrsi ga.
           mov     ax,STPC             ; Da li je doslo do zadnje single-step
           cmp     ax,FINSTEP          ;  instrukcije?
           je      ENDSSTEP
           mov     ah,1
           int     16H
           jz      CONTSSTEP
           xor     ah,ah
           int     16H
           cmp     al,' '
           je      ENDSSTEP
CONTSSTEP: jmp     ISTEP
ENDSSTEP:  call    NEWLOUT
EDENTRY:   call    EDITOR              ; Ocitaj i skaniraj komandu
           call    SCANLINE
           jnc     SYNERR
           mov     ah,COMCODE
           or      ah,ah
           je      EDENTRY
           xor     bx,bx
COMLOOP:   cmp     COMS[bx],0
           je      COMERR
           cmp     ah,COMS[bx]
           je      DOCOMM
           inc     bx
           jmp     COMLOOP
DOCOMM:    add     bx,bx               ; Indirektno izvrsi komandu
           mov     MFLAG,0
           mov     SSFLAG,0
           jmp     COMADDR[bx]

;
; Prijava gresaka.
;

SYNERR:    lea     di,SYNERMSG
           jmp     short ERROR
COMERR:    lea     di,COMERMSG
           jmp     short ERROR
PARERR:    lea     di,PARERMSG
           jmp     short ERROR
OVRERR:    lea     di,OVRERMSG
           jmp     short ERROR
BIGERR:    lea     di,BIGERMSG
           jmp     short ERROR
RTPERR:    lea     di,RTPERMSG
           jmp     short ERROR
INTERR:    lea     di,INTERMSG
           jmp     short ERROR
FINERR:    lea     di,FINERMSG
           jmp     short ERROR
OUTERR:    lea     di,OUTERMSG
ERROR:     call    TESTOUT
           call    PO_MSG
           call    ENDOUT
           call    NEWLOUT
           jmp     EDENTRY

;
; Izlaz iz monitora uz ignorisanje promjena (Q).
;

MONQUIT:   call    UNDOREG
           jmp     short DOMONEXIT

;
; Izlaz iz monitora uz uvazavanje promjena (E).
;

MONEXIT:   cmp     PARCOUNT,0
           je      DOMONEXIT
           mov     si,PARAMS
           mov     STPC,si
DOMONEXIT: cmp     BRALREADY,2
           jne     NOSETBP
           mov     BRALREADY,1         ; Po potrebi postavlja prekidnu tacku
           xor     bh,bh
           call    BPSUB
NOSETBP:   call    BACKREG
           retf

;
; Ignorisanje promjena (U).
;

UNDO:      call    UNDOREG
           call    PANELOUT
           jmp     REGENTRY

UNDOREG:   lea     di,STAF
           mov     cx,13
UNDOLOOP:  mov     si,ss:[di+2]
           mov     ss:[di],si
           add     di,4
           loop    UNDOLOOP
           ret

;
; Izlaz u bejzik (A).
;

ABORT:     xor     ah,ah
           call    TIMMMUFAR
           mov     SAVECTL,0
           mov     TIMCTL,0
           cmp     PARCOUNT,0
           je      ASOFT
           mov     si,PARAMS
           cmp     si,1                ; Provjeri A0 ili A1
           jbe     ASOFTRISC
           cmp     si,23900
           jae     ARTOK
           jmp     RTPERR
ARTOK:     dec     si                  ; Skok na NEW. Takodje vrsi obnovu PIP,
           mov     ds:[23730],si       ;  RASP i P_RAMT.
           mov     IMODE,0
           mov     word ptr ds:[23608],64
           mov     word ptr ds:[23732],65535
           cmp     PARCOUNT,1
           ja      NEWLIKEZX
           mov     si,3357             ; Timex NEW
           retf
NEWLIKEZX: mov     si,4535             ; ZX NEW
           retf
ASOFTRISC: mov     si,ds:[23730]       ; A0 na svoj rizik radi kao kod ZX48
           jmp     short ASOFT1
ASOFT:     mov     word ptr ds:[23734],0
           mov     si,ds:[23744]
ASOFT1:    mov     IY,23610            ; "Vruci" izlazak. Obnavlja registre SP
           mov     IFF,0404H           ;  i IY, IFF, postavlja IM 0, obnavlja
           mov     IMODE,0             ;  varijable PIP, RASP, ERRSP, FLAGS,
           dec     si                  ;  FLAGX, P_RAMT (mozda i ERR_LN), i
           mov     bp,si               ;  nakon toga vrsi skok na MAIN_EXEC.
           sub     si,2
           mov     ds:[23613],si
           mov     byte ptr ds:[23665],0
           mov     byte ptr ds:[23611],0
           mov     word ptr ds:[23608],64
           mov     word ptr ds:[23732],65535
           cmp     PARAMS,1
           je      LIKEZX
           mov     si,3624             ; MAIN_EXEC na Timexu
           retf
LIKEZX:    mov     si,4770             ; MAIN_EXEC na Spectrumu
           retf

;
; Stranicenje memorije (Y).
;

MEMPAGE:   cmp     PARCOUNT,0
           jne     NODEFPAGE
           mov     ah,SAVECTL
           and     ah,192
           call    TIMCTLFAR
           mov     ah,SAVEMMU
           call    TIMMMUFAR
           call    PANELOUT
           jmp     REGENTRY
NODEFPAGE: mov     si,PARAMS
           cmp     si,512
           jb      PAGEOK
           jmp     BIGERR
PAGEOK:    mov     ah,128
           cmp     si,256
           jae     PAGECONT
           xor     ah,ah
PAGECONT:  call    TIMCTLFAR
           mov     ax,si
           xchg    ah,al
           call    TIMMMUFAR
           call    PANELOUT
           jmp     REGENTRY

;
; Spremanje svih registara.
;

STOREREG:  mov     bx,IY
           mov     STIY,bx
           mov     bx,IX
           mov     STIX,bx
           mov     STPC,si
           mov     STSP,bp
           mov     STHL,di
           mov     STDE,dx
           mov     STBC,cx
           mov     bx,FLAGS
           mov     dx,PVN
           call    GETAF
           mov     STAF,ax
           mov     bx,ALTHL
           mov     STALTHL,bx
           mov     bx,ALTDE
           mov     STALTDE,bx
           mov     bx,ALTBC
           mov     STALTBC,bx
           mov     al,ALTA
           mov     bx,ALTF
           mov     dx,ALTPVN
           call    GETAF
           mov     STALTAF,ax
           mov     al,R
           mov     ah,R7
           and     ax,807FH
           or      al,ah
           mov     ah,I
           mov     STIR,ax
           ret

;
; Vracanje svih registara.
;

BACKREG:   mov     ax,STIR
           mov     I,ah
           mov     ah,al
           and     ax,807FH
           mov     ss:R,al
           mov     ss:R7,ah
           mov     ax,STALTAF
           mov     ALTA,ah
           mov     byte ptr ALTF,al
           and     ax,2AH
           mov     ALTPVN,ax
           mov     bx,STALTHL
           mov     ALTHL,bx
           mov     bx,STALTDE
           mov     ALTDE,bx
           mov     bx,STALTBC
           mov     ALTBC,bx
           mov     bx,STIY
           mov     IY,bx
           mov     bx,STIX
           mov     IX,bx
           mov     si,STPC
           mov     bp,STSP
           mov     di,STHL
           mov     dx,STDE
           mov     cx,STBC
           mov     ax,STAF
           mov     FLAGSL,al
           mov     bx,ax
           and     bx,2AH
           mov     PVN,bx
           mov     al,ah
           ret

;
; Potprogram za uzimanje AF ili AF' para.
;

GETAF:     test    dh,255
           jnz     GAF1
           mov     ah,bl
           and     ah,0D5H
           or      ah,dl
           xchg    ah,al
           ret
GAF1:      and     bx,08F1H
           or      bl,dl
           shr     bh,1
           or      bl,bh
           mov     ah,bl
           xchg    ah,al
           ret

;
; Ispis flegova (AH-flegovi).
;

FLPRINT:   mov     cx,8
FLLOOP:    mov     dl,'0'
           shl     ah,1
           adc     dl,0
           call    PO_CHR1
           loop    FLLOOP
           ret

;
; Ispis niza bajtova.
;

WRTBYTES:  mov     dl,[bx]
           xor     dl,XORBYTE
           call    NUM8
           inc     bx
           loop    WRTBYTES
           ret

;
; Aktiviranje izlaza u eksternu datoteku (O).
;

SETOUTPUT: xor     OUTACT,1
           jz      OUTCLOSE
           push    ds
           mov     ds,STEKSEG
           mov     ax,3D01H
           lea     dx,OUTNAME
           int     21H
           jnc     NOCREATE
           mov     ah,3CH
           xor     cx,cx
           int     21H
           jc      OUTERROR
NOCREATE:  mov     OUTHANDLE,ax
           mov     bx,ax
           xor     dx,dx
           mov     ax,4202H
           int     21H
           jc      OUTERROR
           pop     ds
           call    PANELOUT
           jmp     REGENTRY
OUTCLOSE:  mov     ah,3EH
           mov     bx,OUTHANDLE
           int     21H
           jmp     REGENTRY
OUTERROR:  pop     ds
           cmp     OUTHANDLE,0
           mov     OUTHANDLE,0
           je      NOOUTOPEN
           mov     ah,3EH
           mov     bx,OUTHANDLE
           int     21H
NOOUTOPEN: mov     OUTACT,0
           mov     OUTFLAG,0
           jmp     OUTERR

;
; Hex mod (H).
;

HEXMODE:   xor     BASE,26
           call    PANELOUT
           jmp     REGENTRY

;
; Brisanje panela.
;

CLRPANEL:  pusha
           mov     ax,16384
           mov     dx,768+' '
CLP1:      mov     VMADDR,ax
           mov     cx,256
CLP2:      call    PO_CHR1
           loop    CLP2
           add     ax,2048
           dec     dh
           jnz     CLP1
           popa
           ret

;
; Trazenje adrese jedne linije (u AH broj, u AX adresa).
;

CLADDR:    mov     al,ah
           ror     al,3
           and     al,0E0H
           and     ah,18H
           or      ah,40H
           ret

;
; Prikaz DEBUG zone (@), nedokumentovano.
;

DEBUG:     mov     VMADDR,20672
           mov     al,MFLAG
           push    ax
           mov     MFLAG,0
           lea     bx,DEBUGZONE
           mov     cx,8
DBGLOOP:   mov     dl,ss:[bx]
           call    NUM8
           call    PO_SPACE
           inc     bx
           loop    DBGLOOP
           pop     ax
           mov     MFLAG,al
           jmp     EDENTRY

;
; Konverzija brojeva u razne oblike (?).
;

CONVNUM:   cmp     PARCOUNT,1
           jae     CN1
           mov     bx,CURREG
           add     bx,bx
           mov     bx,STAF[bx]
           jmp     short CNENTRY
CN1:       mov     bx,PARAMS
CNENTRY:   call    TESTOUT
           mov     VMADDR,20672
           push    BASE
           mov     BASE,16
           mov     dx,bx
           call    NUM16
           call    PO_SPACE
           mov     BASE,10
           mov     MFLAG,0
           mov     dx,bx
           or      bh,bh
           jz      CN2
           call    NUM16
           call    PO_SPACE
           or      bx,bx
           jns     CN3
           mov     dl,'-'
           call    PO_CHR1
           neg     bx
           mov     dx,bx
           call    NUM16
           neg     bx
           call    PO_SPACE
           jmp     short CN3
CN2:       call    NUM8
           call    PO_SPACE
           or      bl,bl
           jns     CN3
           mov     dl,'-'
           call    PO_CHR1
           neg     bl
           mov     dl,bl
           call    NUM8
           neg     bl
CN3:       or      bh,bh
           jz      CN4
           mov     dl,bh
           call    NUM8
           mov     dl,','
           call    PO_CHR1
           mov     dl,bl
           call    NUM8
           jmp     short CN5
CN4:       cmp     bl,32
           jb      CN5
           cmp     bl,127
           ja      CN5
           mov     dl,''''
           call    PO_CHR1
           mov     dl,bl
           call    PO_CHR1
           mov     dl,''''
           call    PO_CHR1
CN5:       pop     BASE
           call    ENDOUT
           call    NEWLOUT
           jmp     EDENTRY

;
; Adresa IM2 rutine (N):
;

IMADDR:    cmp     IMODE,2
           jae     IM2ADDR
           mov     bx,56
           jmp     CNENTRY
IM2ADDR:   mov     bh,I
           mov     bl,255
           mov     cl,[bx]
           mov     ch,[bx+1]
           mov     bx,cx
           mov     al,[bx]
           cmp     al,195
           jz      IMJP
           cmp     al,24
           jz      IMJR
           jmp     CNENTRY
IMJP:      inc     bx
           mov     cl,[bx]
           mov     ch,[bx+1]
           mov     bx,cx
           jmp     CNENTRY
IMJR:      mov     al,[bx+1]
           cbw
           add     bx,ax
           add     bx,2
           jmp     CNENTRY

;
; Sabiranje (+):
;

ADDNUM:    call    TWOPARS
           add     bx,cx
           jmp     CNENTRY

TWOPARS:   cmp     PARCOUNT,2
           jb      TPERR
           mov     bx,PARAMS
           mov     cx,PARAMS[2]
           ret
TPERR:     pop     bx                  ; Ignorise povratnu adresu!
           jmp     PARERR

;
; Oduzimanje (-):
;

SUBNUM:    call    TWOPARS
           sub     bx,cx
           jmp     CNENTRY

;
; Mnozenje (*):
;

MULNUM:    call    TWOPARS
           mov     ax,bx
           mul     cx
           jo      MULERR
           mov     bx,ax
           jmp     CNENTRY
MULERR:    jmp     OVRERR

;
; Dijeljenje (/):
;

DIVNUM:    call    TWOPARS
           or      cx,cx
           jz      MULERR
           mov     ax,bx
           xor     dx,dx
           div     cx
           mov     bx,ax
           jmp     CNENTRY

;
; Ostatak dijeljenja (%):
;

MODNUM:    call    TWOPARS
           or      cx,cx
           jz      MULERR
           mov     ax,bx
           xor     dx,dx
           div     cx
           mov     bx,dx
           jmp     CNENTRY

;
; Binarni AND (&):
;

ANDNUM:    call    TWOPARS
           and     bx,cx
           jmp     CNENTRY

;
; Binarni OR (|):
;

ORNUM:     call    TWOPARS
           or      bx,cx
           jmp     CNENTRY

;
; Binarni XOR (^):
;

XORNUM:    call    TWOPARS
           xor     bx,cx
           jmp     CNENTRY

;
; Postavljanje registara (R).
;

SETREG:    mov     ax,PARAMS
           cmp     PARCOUNT,1
           jae     SR1
           mov     ax,STMP
SR1:       mov     bx,CURREG
           add     bx,bx
           mov     STAF[bx],ax
           call    PANELOUT
           jmp     REGENTRY

;
; Postavljanje IFF-a i interupt moda (!).
;

SETIFFS:   cmp     PARCOUNT,0
           jumps
           je      PARERR
           mov     ax,PARAMS
           cmp     ax,-1
           je      short SETIMODE
           cmp     ax,1
           ja      BIGERR
           shl     ax,2
           mov     ah,al
           mov     IFF,ax
SETIMODE:  cmp     PARCOUNT,1
           je      short SETIEND
           mov     ax,PARAMS[2]
           cmp     ax,2
           ja      BIGERR
           mov     IMODE,ax
           nojumps
SETIEND:   call    TESTOUT
           or      OUTFLAG,2
           lea     di,IMMSG
           call    PO_MSGL
           call    OUTINTS
           call    ENDOUT
           call    NEWLOUT
           jmp     REGENTRY

;
; Prikaz stvarnog ekrana (V).
;

VIEWSCR:   mov     ah,SAVECTL
           call    TIMCTLFAR
           call    REPAINT
           call    CLEARKBD
           sti
VSLOOP:    mov     cx,11
VSWAIT:    push    ds
           xor     ax,ax
           mov     ds,ax
           mov     ax,ds:[46CH]
VSHLT:     cmp     ax,ds:[46CH]        ; Ovim se simulira hlt, koji ne radi
           je      VSHLT               ;  dobro pod Windowsom
           pop     ds
           mov     ah,1
           int     16H
           jnz     VSEND
           loop    VSWAIT
           call    FLSHFAR
           jmp     VSLOOP
VSEND:     cli
           mov     ah,TIMCTL
           and     ah,192
           call    TIMCTLFAR
           call    COLORMON
           jmp     PANENTRY

;
; Osvjezavanje panela (_), nedokumentovano (interno potrebno).
;

REFRESH:   call    COLORMON
           jmp     PANENTRY

;
; Postavljanje XOR bajta (X).
;

SETXOR:    cmp     PARCOUNT,1
           jae     SX1
           xor     ax,ax
           jmp     short SX2
SX1:       mov     ax,PARAMS
           or      ah,ah
           jz      SX2
           jmp     BIGERR
SX2:       mov     XORBYTE,al
           call    PANELOUT
           jmp     PANENTRY

;
; Save bloka bez hedera ili sa hederom (S).
;

SAVE:      call    TWOPARS
           cmp     PARCOUNT,2
           je      SAVEBLOCK
           push    cx
           lea     di,HEADER[1]
           mov     cx,10
SAVECLR:   mov     byte ptr ss:[di],' '
           inc     di
           loop    SAVECLR
           lea     di,HEADER[1]
           mov     si,4
           mov     cx,10
           cmp     PARCOUNT,12
           ja      SAVELOOP
           mov     cl,PARCOUNT
           xor     ch,ch
           sub     cx,2
SAVELOOP:  mov     ax,PARAMS[si]
           or      ah,ah
           jnz     SAVEERR
           mov     ss:[di],al
           add     si,2
           inc     di
           loop    SAVELOOP
           pop     cx
           pusha
           mov     word ptr HEADER[11],cx
           mov     word ptr HEADER[13],bx
           push    ds
           mov     ds,STEKSEG
           lea     bx,HEADER
           mov     cx,17
           xor     al,al
           call    SAVEFORMON
           pop     ds
           popa
SAVEBLOCK: mov     al,255
           call    SAVEFORMON
           jmp     PANENTRY
SAVEERR:   pop     cx
           jmp     BIGERR

;
; Load bloka (L).
;

LOAD:      mov     cx,PARAMS
           cmp     PARCOUNT,0
           jne     LD2
           mov     cx,STMP
LD2:       call    LOADFORMON
           call    COLORMON
           jmp     PANENTRY

;
; Postavljanje memorijskog pokazivaca i upis u memoriju (M).
;

MWRITE:    cmp     PARCOUNT,0
           jne     MW1
           mov     di,CURREG
           add     di,di
           mov     di,STAF[di]
           mov     STMP,di
           call    MPOUT
           jmp     REGENTRY
MW1:       lea     si,PARAMS
           mov     di,STMP
           mov     bx,ss:[si]
           mov     STMP,bx
           mov     di,bx
           add     si,2
           xor     ch,ch
           mov     cl,PARCOUNT
           dec     cx
           jnz     MWLOOP
           call    MPOUT
           jmp     REGENTRY
MWLOOP:    mov     bx,ss:[si]
           cmp     bx,-1
           je      MW4
           or      bh,bh
           jz      MW3
           jmp     BIGERR
MW3:       xor     bl,XORBYTE
           mov     ah,bl
           call    MEMPOKE
MW4:       inc     STMP
           inc     di
           add     si,2
           loop    MWLOOP
           call    MPOUT
           jmp     REGENTRY

;
; Upis bajtova u memoriju na tekuci pokazivac (P).
;

POKE:      cmp     PARCOUNT,0
           jne     PO1
           jmp     PARERR
PO1:       lea     si,PARAMS
           mov     di,STMP
           xor     ch,ch
           mov     cl,PARCOUNT
           jmp     MWLOOP

;
; Upis sesnaestbitnih rijeci u memoriju na tekuci pokazivac (W).
;

POKEWORD:  cmp     PARCOUNT,0
           jne     WR1
           jmp     PARERR
WR1:       lea     si,PARAMS
           mov     di,STMP
           xor     ch,ch
           mov     cl,PARCOUNT
WRLOOP:    mov     bx,ss:[si]
WR3:       xor     bl,XORBYTE
           xor     bh,XORBYTE
           mov     ah,bl
           call    MEMPOKE
           inc     di
           mov     ah,bh
           call    MEMPOKE
           add     STMP,2
           inc     di
           add     si,2
           loop    WRLOOP
           call    MPOUT
           jmp     REGENTRY

;
; Kopiranje memorije (C).
;

COPYMEM:   call    THREEPARS
           mov     si,ax
           mov     di,bx
           cmp     si,di
           ja      COPYLOOP1
           add     si,cx
           add     di,cx
           dec     si
           dec     di
COPYLOOP2: mov     ah,[si]              ; Kopira s nize na visu adresu
           call    MEMPOKE
           dec     si
           dec     di
           loop    COPYLOOP2
           jmp     PANENTRY
COPYLOOP1: mov     ah,[si]             ; Kopira s vise na nizu
           call    MEMPOKE
           inc     si
           inc     di
           loop    COPYLOOP1
           jmp     PANENTRY

THREEPARS: cmp     PARCOUNT,3
           jb      THRPERR
           mov     ax,PARAMS
           mov     bx,PARAMS[2]
           mov     cx,PARAMS[4]
           ret
THRPERR:   pop     bx                  ; Ignorise povratnu adresu!
           jmp     PARERR

;
; Punjenje memorije (K).
;

KILLMEM:   call    THREEPARS
           or      ch,ch
           jz      KILLOK
           jmp     BIGERR
KILLOK:    xor     cl,XORBYTE
           mov     di,ax
           mov     ah,cl
KILLLOOP:  call    MEMPOKE
           cmp     di,bx
           je      KILLEND
           inc     di
           jmp     KILLLOOP
KILLEND:   jmp     PANENTRY

;
; Pretraga memorije (F).
;

FINDMEM:   cmp     PARCOUNT,0
           je      FINDOLD
           mov     FSTART,-1
           mov     FINDCOUNT,0
           mov     cl,PARCOUNT
           xor     ch,ch
           lea     si,PARAMS
           lea     di,FBYTES
FPARLOOP:  lods    word ptr ss:[si]
           or      ah,ah
           jz      FIND1
           cmp     ax,-1
           je      FIND2

           xor     al,XORBYTE
           xor     ah,XORBYTE
           cmp     FSTART,-1
           jne     FINDW
           mov     FSTART,di
FINDW:     add     FINDCOUNT,2
           mov     bx,ax
           xor     bh,bh
           mov     ss:[di],bx
           mov     bl,ah
           mov     ss:[di+2],bx
           add     di,4
           loop    FPARLOOP
           jmp     short FINDOLD
FIND1:     xor     al,XORBYTE
           cmp     FSTART,-1
           jne     FIND2
           mov     FSTART,di
FIND2:     cmp     FSTART,-1
           je      FIND3
           inc     FINDCOUNT
FIND3:     mov     ss:[di],ax
           add     di,2
           loop    FPARLOOP
FINDOLD:   inc     STMP
FINDNEW:   cmp     FSTART,-1
           jne     FINDCONT
           call    MPOUT
           jmp     REGENTRY
FINDCONT:  push    es
           push    ds
           pop     es
           mov     cx,65535
           mov     di,STMP
           mov     bx,FSTART
           sub     bx,offset SSEG:FBYTES
           shr     bx,1
           add     di,bx
           push    bx
FINDLOOP:  mov     si,FSTART
           lods    word ptr ss:[si]
           repne   scasb
           jnz     FINDERR
           push    di
           mov     dl,FINDCOUNT
FBYTLOOP:  dec     dl
           jz      FINDOK
           lods    word ptr ss:[si]
           or      ah,ah
           jnz     FSKIP
           scasb
           je      FBYTLOOP
           pop     di
           jmp     FINDLOOP
FSKIP:     inc     di
           jmp     FBYTLOOP
FINDERR:   pop     bx
           pop     es
           jmp     FINERR
FINDOK:    pop     di
           pop     bx
           pop     es
           dec     di
           sub     di,bx
           mov     STMP,di
           call    MPOUT
           jmp     REGENTRY

;
; Help prikaz (poziva se sa F1 iz editora).
;

SHOWHELP:  mov     INDIRECT,offset HLP_LINE
           lea     di,HELPTXT
           mov     HELPPTR,di
           mov     PARCOUNT,2
           mov     PARAMS,0
           mov     PARAMS[2],47
           xor     si,si
           jmp     DISENTRY

;
; Prikaz jedne help linije.
;

HLP_LINE:  call    CLADDR
           mov     VMADDR,ax
HLP_LOOP:  mov     di,HELPPTR
           mov     dl,ss:[di]
           inc     HELPPTR
           or      dl,dl
           jz      HLP_END
           call    PO_CHR1
           jmp     HLP_LOOP
HLP_END:   inc     si
           ret

;
; Listanje memorije (T).
;

TABULATE:  mov     INDIRECT,offset TAB_LINE
           mov     PARAMS[6],0
           mov     si,STMP
           jmp     short DISENTRY

;
; Listanje jedne linije (AH-broj linije).
;

TAB_LINE:  call    CLADDR
           mov     VMADDR,ax
           mov     MFLAG,16
           mov     dx,si
           call    NUM16
           call    PO_SPACE
           mov     MFLAG,48
           push    BASE
           mov     BASE,16
           mov     bx,8
TLLOOP:    mov     dl,[si]
           xor     dl,XORBYTE
           call    NUM8
           inc     si
           dec     bx
           jnz     TLLOOP
           call    PO_SPACE
           sub     si,8
           mov     bx,8
TALOOP:    mov     dl,[si]
           xor     dl,XORBYTE
           cmp     dl,127
           jbe     TA1
           mov     INV_FLG,255
           sub     dl,128
TA1:       cmp     dl,' '
           jae     TA2
           mov     dl,'.'
TA2:       call    PO_CHR1
           mov     INV_FLG,0
           inc     si
           dec     bx
           jnz     TALOOP
           pop     BASE
           ret

;
; Disasembler (D).
;

DISS:      mov     INDIRECT,offset DISS_LINE
           mov     si,STPC
DISENTRY:  cmp     PARCOUNT,3
           jb      DISSENT2
           jumps
           cmp     PARAMS[4],-1
           je      short DISSENT1
           cmp     PARAMS[4],24
           ja      BIGERR
           cmp     PARAMS[4],0
           jne     short DISSENT1
           mov     PARAMS[4],24
DISSENT1:  cmp     PARCOUNT,3
           je      short DISSENT2
           cmp     PARAMS[6],3
           ja      BIGERR
           nojumps
DISSENT2:  call    CLRPANEL
           mov     DBFLAG,0
           mov     DISEND,-1
           mov     cx,24
           mov     DISCNT,cx
           cmp     PARCOUNT,0
           je      DLOOP
           mov     si,PARAMS
           cmp     PARCOUNT,1
           je      DLOOP
           mov     bx,PARAMS[2]
           mov     DISEND,bx
           cmp     PARCOUNT,2
           je      DLOOP
           cmp     PARAMS[4],-1
           je      DLOOP
           mov     cx,PARAMS[4]
           mov     DISCNT,cx
DLOOP:     cmp     PARCOUNT,4
           jb      DNOOPT
           mov     ah,byte ptr PARAMS[6]
           mov     DBFLAG,ah
DNOOPT:    mov     ah,byte ptr DISCNT
           sub     ah,cl
           call    TESTOUT
           call    INDIRECT
           call    ENDOUT
           cmp     si,DISEND
           ja      DEND
           mov     ah,1
           int     16H
           jz      DTEST
           xor     ah,ah
           int     16H
           cmp     al,' '
           je      DDONE
DTEST:     loop    DLOOP
           call    WAITKEY
           mov     cx,DISCNT
           cmp     al,' '
           je      DDONE
           call    CLRPANEL
           jmp     DLOOP
DEND:      call    WAITKEY
DDONE:     call    NEWLOUT
           jmp     PANENTRY

;
; Disasembliranje jedne linije (AH-broj linije).
;

DISS_LINE: call    CLADDR
           push    ax
           mov     VMADDR,ax
           mov     MFLAG,16
           mov     dx,si
           call    NUM16
           call    PO_SPACE
           call    DISS_ONE
           mov     NEXTINS,si
           mov     dx,si
           mov     si,STCODE
           sub     dx,si
           mov     MFLAG,48
           push    BASE
           mov     BASE,16
DLLOOP:    push    dx
           mov     dl,[si]
           xor     dl,XORBYTE
           call    NUM8
           inc     si
           pop     dx
           dec     dx
           jnz     DLLOOP
           pop     BASE
           pop     dx
           mov     ax,VMADDR
           sub     ax,dx
           mov     dx,15
           sub     dx,ax
DLSPACES:  push    dx
           call    PO_SPACE
           pop     dx
           dec     dx
           jnz     DLSPACES
           lea     di,BUFFER
           jmp     PO_MSGL

;
; Disasembliranje jedne instrukcije.
;

DISS_ONE:  mov     MFLAG,64
           mov     ARGFLG,0
           mov     STARG,0
           mov     STCODE,si
           lodsb
           xor     al,XORBYTE
           lea     di,BUFFER
           cmp     DBFLAG,0
           je      DISS_NORM
           dec     si                  ; Specijalne opcije disasembliranja
           lea     bx,DBTEXT-1
           cmp     DBFLAG,2
           jne     DISS_DB
           lea     bx,DWTEXT-1
DISS_DB:   cmp     DBFLAG,3
           je      DISS_40
DISS_DB1:  mov     DBFLAG,0
           jmp     PLOOP
DISS_DB2:  cmp     PARCOUNT,4
           jb      DISS_DB1
           mov     PARCOUNT,3
           jmp     DISS_DB1
DISS_40:   cmp     al,38H              ; Tretman RST 40 u Standard ROM-u
           je      DISS_DB2
           push    ax
           mov     al,[si-1]
           xor     al,XORBYTE
           cmp     al,33H
           pop     ax
           je      DISS_DB2
           jmp     short PLOOP

DISS_NORM: cmp     al,221              ; Normalno disasembliranje
           jne     NOIX
           or      MFLAG,1
           jmp     short INDREG
NOIX:      cmp     al,253
           jne     DCONT
           or      MFLAG,2
INDREG:    lodsb
           xor     al,XORBYTE
DCONT:     lea     bx,SINGLE
           cmp     al,237
           jne     NOED
           lodsb
           xor     al,XORBYTE
           lea     bx,SING_ED
           or      MFLAG,8
           jmp     short MLOOP
NOED:      cmp     al,203
           jne     MLOOP
           lea     bx,SING_CB
           lodsb
           xor     al,XORBYTE
           test    MFLAG,3
           jz      MLOOP
           or      MFLAG,4
           mov     al,[si]
           xor     al,XORBYTE
           dec     si

MLOOP:     mov     ah,ss:[bx]
           and     ah,al
           inc     bx
           cmp     ah,ss:[bx]
           je      PLOOP
MSKLOOP:   inc     bx
           test    byte ptr ss:[bx],128
           jz      MSKLOOP
           inc     bx
           jmp     MLOOP

PLOOP:     inc     bx
           mov     ah,ss:[bx]
           and     ah,127
           mov     ss:[di],ah
           push    bx
           xor     bx,bx
SPCLOOP:   cmp     SPECIAL[bx],0
           je      PNORM
           cmp     ah,SPECIAL[bx]
           je      DOSPEC
           inc     bx
           jmp     SPCLOOP
DOSPEC:    add     bx,bx
           call    SPECADDR[bx]
PNORM:     inc     di
           pop     bx
           test    byte ptr ss:[bx],128
           jz      PLOOP
           test    MFLAG,3
           jz      PDONE
           test    MFLAG,4
           jz      BADIND
           test    MFLAG,8
           jz      PDONE
BADIND:    mov     si,STCODE
           lea     di,BUFFER
           mov     MFLAG,64
           jmp     INDREG

PDONE:     mov     byte ptr ss:[di],'@'
           ret

;
; Specijalne akcije za potrebe disasemblera.
;

R8_1:      mov     bl,al
           and     bl,56
           ror     bl,3
R8CONT:    cmp     bl,6
           je      MEMIND
           cmp     bl,7
           je      R8NORM
           cmp     bl,4
           jb      R8NORM
           test    MFLAG,3
           jz      R8NORM
           or      MFLAG,4
           mov     byte ptr ss:[di],'X'
           inc     di
           test    MFLAG,2
           jz      R8NORM
           mov     byte ptr ss:[di-1],'Y'
R8NORM:    xor     bh,bh
           mov     bl,REGS[bx]
           mov     ss:[di],bl
           ret
MEMIND:    mov     byte ptr ss:[di],'('
           inc     di
           call    HLIXIY
           mov     bx,STHL
           mov     STARG,bx
           mov     ARGFLG,1
           inc     di
           test    MFLAG,3
           jz      NOMIND
           mov     bx,STIX
           test    MFLAG,2
           jz      MEMIX
           mov     bx,STIY
MEMIX:     mov     dl,[si]
           xor     dl,XORBYTE
           push    ax
           mov     al,dl
           cbw
           add     bx,ax
           mov     STARG,bx
           mov     ARGFLG,1
           pop     ax
           inc     si
           call    NUM8SIGN
           inc     di
NOMIND:    mov     byte ptr ss:[di],')'
           ret

R8_2:      mov     bl,al
           and     bl,7
           jmp     R8CONT

INDCB:     test    MFLAG,3
           jz      R8_2
           call    MEMIND
           inc     si
           mov     dl,al
           and     dl,192
           cmp     dl,64
           je      INDEND
           mov     bl,al
           and     bl,7
           cmp     bl,6
           je      INDEND
           inc     di
           mov     byte ptr ss:[di],','
           inc     di
           jmp     R8NORM
INDEND:    ret

OPC:       lea     bx,MNEMCS
           jmp     short CCONT1

ROTSHFT:   lea     bx,ROTSHFTS
           jmp     short CCONT1

C1:        lea     bx,CONDCODES
           mov     dl,al
           and     dl,24
           jmp     short CCONT2

C2:        lea     bx,CONDCODES
CCONT1:    mov     dl,al
           and     dl,56
CCONT2:    shr     dl,3
           jz      C2LOOP2
C2LOOP1:   inc     bx
           test    byte ptr ss:[bx-1],128
           jz      C2LOOP1
           dec     dl
           jnz     C2LOOP1
C2LOOP2:   mov     dl,ss:[bx]
           and     dl,127
           mov     ss:[di],dl
           inc     di
           inc     bx
           test    byte ptr ss:[bx-1],128
           jz      C2LOOP2
           dec     di
           ret

BITS:      mov     dl,al
           and     dl,56
           ror     dl,3
           push    BASE
           mov     BASE,10
           call    NUM8
           pop     BASE
           ret

RSTN:      mov     dl,al
           and     dl,56
           cmp     dl,8
           jne     NORST8
           mov     DBFLAG,1
           jmp     NUM8
NORST8:    cmp     dl,40
           jumps
           jne     NUM8
           nojumps
           mov     DBFLAG,3
           jmp     NUM8

DISP:      push    ax
           lodsb
           xor     al,XORBYTE
           mov     dx,si
           cbw
           add     dx,ax
           pop     ax
           jmp     NUM16

RP:        mov     bl,al
           and     bl,48
           cmp     bl,32
           je      HLIXIY
           ror     bl,3
           xor     bh,bh
           mov     bx,word ptr RPARS[bx]
           jmp     short HLIRET

HLIXIY:    xor     bh,bh
           mov     bl,MFLAG
           and     bl,3
           add     bx,bx
           mov     bx,word ptr ADR16[bx]
           test    MFLAG,3
           jz      HLIRET
           or      MFLAG,4
HLIRET:    mov     ss:[di],bx
           inc     di
           ret

TOKEN:     cmp     PARCOUNT,4
           jb      TOKEND
           cmp     PARAMS[6],1
           jne     TOKEND
           mov     dl,[si-1]
           xor     dl,XORBYTE
           call    PO_ONE
TOKEND:    dec     di
           ret

N8:        mov     dl,[si]
           xor     dl,XORBYTE
           inc     si
           jmp     NUM8

ARGDE:     mov     dx,STDE
           mov     STARG,dx
           mov     ARGFLG,1
           dec     di
           ret

ARGBC:     mov     dx,STBC
           mov     STARG,dx
           mov     ARGFLG,1
           dec     di
           ret

ARGSP:     mov     dx,STSP
           mov     STARG,dx
           mov     ARGFLG,1
           dec     di
           ret

N16:       mov     dx,[si]
           xor     dl,XORBYTE
           xor     dh,XORBYTE
           add     si,2
           jmp     NUM16

N16ARG:    mov     dx,[si]
           xor     dl,XORBYTE
           xor     dh,XORBYTE
           add     si,2
           mov     STARG,dx
           mov     ARGFLG,1
           jmp     NUM16

SPUT:      test    MFLAG,3
           jz      CBREG
           mov     dl,[si+1]
           xor     dl,XORBYTE
           and     dl,7
           cmp     dl,6
           je      CBREG
           mov     byte ptr ss:[di],'P'
           ret
CBREG:     mov     byte ptr ss:[di],' '
           dec     di
           ret

SPACE:     mov     byte ptr ss:[di],' '
           inc     di
           cmp     di,offset SSEG:BUFFER[4]
           je      SPC_MAYBE
           cmp     di,offset SSEG:BUFFER[5]
           je      SPC_END
           cmp     di,offset SSEG:BUFFER[8]
           je      SPC_END
           cmp     di,offset SSEG:BUFFER[9]
           je      SPC_END
           jmp     SPACE
SPC_MAYBE: cmp     PARCOUNT,4
           jb      SPACE
           cmp     PARAMS[6],1
           jne     SPACE
SPC_END:   dec     di
           ret

;
; Break point (B) - upisuje RST 0.
;

BREAKP:    cmp     PARCOUNT,0
           jne     BREAK1
           mov     BRALREADY,0
           jmp     PANENTRY
BREAK1:    mov     si,PARAMS
           mov     BPADDR,si
           mov     BRALREADY,2
           jmp     PANENTRY

BPSUB:        ;;; ???                  ; Pomocni potprogram za postavljanje
                                       ;  (BH=0) ili brisanje (BH=1) prekidne
                                       ;  tacke u proizvoljnu stranicu memorije

           mov     di,BPADDR
           or      bh,bh
           jz      BPSUB2
           mov     ah,SAVEBP
           cmp     byte ptr [di],199
           jne     BPSUB3
           mov     [di],ah ;;;mempoke1
           ret
BPSUB2:    mov     ah,[di]
           mov     SAVEBP,ah
           mov     ah,199
           mov     [di],ah ;;;mempoke1
BPSUB3:    ret

;
; Izvrsavanje potprograma (J). Vodi se racuna o nekim INLINE parametrima.
;

JUMP:      cmp     PARCOUNT,0
           jne     JUMP1
           mov     si,NEXTINS
           xor     ah,ah
           mov     al,DBFLAG
           add     si,ax
           cmp     al,3
           jne     JUMP2
           sub     si,2
JMPLOOP:   cmp     byte ptr [si-1],38H
           je      JUMP2
           cmp     byte ptr [si-2],33H
           je      JUMP2
           inc     si
           jmp     JMPLOOP
JUMP1:     mov     si,PARAMS
JUMP2:     mov     BPADDR,si
           mov     BRALREADY,2
           jmp     DOMONEXIT

;
; Instruction single-step (I).
;

ISTEP:     cmp     PARCOUNT,0
           je      ISTEP1
           mov     SSFLAG,1
           mov     si,PARAMS
           mov     FINSTEP,si
ISTEP1:    call    TESTOUT
           or      OUTFLAG,2
           xor     ah,ah
           mov     si,STPC
           call    DISS_LINE
           push    es
           push    VIDEOSEG
           mov     VIDEOSEG,0F000H
           mov     OLDSP,sp
           xor     si,si
           mov     es,si
           mov     es:[4],offset SSTRAP
           mov     es:[6],cs
           mov     es,STEKSEG
           lea     di,TSTACK
           lea     si,INT_STACK
           mov     cx,500
           cld
           rep     movs word ptr es:[di],ss:[si]
           call    BACKREG
           mov     bl,[si]
           xor     bh,bh
           add     bx,bx
           lea     sp,FLAGS
           or      FLAGS,256
           or      ALTF,256
           mov     SSTEPON,1
           push    256
           push    PROGSEG
           push    TABLE1[bx]
           iret

;
; Trap za INT 1.
;

SSTRAP:    push    bp                  ; Trazi JMP SS:TABLE1[BX]
           mov     bp,sp
           push    es
           les     bp,[bp+2]
           cmp     word ptr es:[bp+1],0A7FFH
           je      SSCONT
SSFAIL:    pop     es
           pop     bp
           iret

SSCONT:    cmp     byte ptr es:[bp],36H
           jne     SSFAIL
           cmp     word ptr es:[bp+3],offset SSEG:TABLE1
           jne     SSFAIL
           pop     es                  ; Instrukcija je zavrsena
           pop     bp
           mov     SSTEPON,0
           and     FLAGS,0FEFFH
           and     ALTF,0FEFFH
           call    STOREREG
           call    ENDOUT
           cmp     SSFLAG,1
           je      SSTOP1
           call    NEWLOUT
SSTOP1:    cmp     BRALREADY,0
           je      SSTOP2
           cmp     di,BPADDR
           jne     SSTOP2
           mov     SSFLAG,0
           call    NEWLOUT
SSTOP2:    mov     es,STEKSEG
           lea     di,INT_STACK
           lea     si,TSTACK
           mov     cx,500
           cld
           rep     movs word ptr es:[di],ss:[si]
           mov     sp,OLDSP
           pop     VIDEOSEG
           mov     cl,TIMCTL
           mov     SAVECTL,cl
           pop     es
           call    RESTTIMER
           sti
           jmp     REGENTRY

;
; Kopiranje ekrana na stampac u raznim rezolucijama (Z).
;

DUMPSCR:   xor     ax,ax               ; Setovanje parametara
           mov     DUMPBRD,0
           cmp     PARCOUNT,0
           je      DUMPDEF
           mov     ax,PARAMS
DUMPDEF:   cmp     ax,4
           jbe     DUMPNOBRD
           sub     ax,5
           mov     DUMPBRD,1
DUMPNOBRD: mov     DUMPMODE,al
           cmp     ax,1
           ja      DUMPSCR2
           mov     INITGR,offset SSEG:IGRCC1
           mov     CRLF,offset SSEG:CRLFCC1
           mov     MOVEHEAD,offset SSEG:MHEADCC1
           mov     YSTEP,4
           mov     PASSNUM,0
           mov     ROWEND,3
           mov     ROWSTEP,1
           shl     PARAMS[2],1
           jmp     DODUMP
DUMPSCR2:  cmp     ax,2
           jne     DUMPSCR3
           mov     INITGR,offset SSEG:IGRCC2
           mov     CRLF,offset SSEG:CRLFCC2
           mov     MOVEHEAD,offset SSEG:MHEADCC2
           mov     YSTEP,8
           mov     PASSNUM,0
           mov     ROWEND,7
           mov     ROWSTEP,1
           jmp     DODUMP
DUMPSCR3:  cmp     ax,3
           jne     DUMPSCR4
           mov     INITGR,offset SSEG:IGRCC3
           mov     CRLF,offset SSEG:CRLFCC3
           mov     MOVEHEAD,offset SSEG:MHEADCC3
           mov     YSTEP,16
           mov     PASSNUM,1
           mov     ROWEND,14
           mov     ROWSTEP,2
           jmp     DODUMP
DUMPSCR4:  cmp     ax,4
           jumps
           jne     BIGERR
           nojumps
           mov     INITGR,offset SSEG:IGRCC4
           mov     CRLF,offset SSEG:CRLFCC4
           mov     MOVEHEAD,offset SSEG:MHEADCC4
           mov     YSTEP,24
           mov     PASSNUM,2
           mov     ROWEND,21
           mov     ROWSTEP,3
           jmp     short DODUMP

CCODES:    pusha                       ; Ispis kontrolnih kodova
           mov     cl,ss:[di]
           xor     ch,ch
CCODESL:   inc     di
           mov     bh,ss:[di]
           call    PRTOUT
           loop    CCODESL
           popa
           ret

PRTOUT:    pusha                       ; Ispis znaka na stampac
           xor     dx,dx
           mov     ah,dl
           mov     al,bh
           test    WINTROUB,2
           jnz     SKIPPRT
           int     17H
SKIPPRT:   popa
           ret

POINT:     mov     ah,bl               ; Odredjuje status tacke.
           shr     ah,3                ; Ulaz: BL=Y, CH=X
           or      ah,01000000B        ; Izlaz: CF=0 ako je tacka PAPER ili je
           xor     ah,bl               ;  skrivena, AL=atribut
           and     ah,0F8H
           xor     ah,bl
           mov     al,ch
           rol     al,3
           xor     al,bl
           and     al,0C7H
           xor     al,bl
           rol     al,2
           mov     si,ax
           push    cx
           mov     cl,ch
           and     cl,07H
           inc     cl
           mov     al,[si]
           rol     al,cl
           pushf
           mov     ax,si
           ror     ah,3
           and     ah,3
           or      ah,58H
           mov     si,ax
           mov     al,[si]
           mov     ah,al
           shr     ah,3
           xor     ah,al
           and     ah,7
           jz      HIDDEN
           popf
           pop     cx
           ret
HIDDEN:    popf
           pop     cx
           clc
           ret

DODUMP:    xor     ax,ax               ; Glavnina rutine
           cmp     PARCOUNT,1
           jbe     NOMARGIN
           mov     ax,PARAMS[2]
           cmp     ax,3200
           jb      NOMARGIN
           mov     ax,3200
NOMARGIN:  mov     MARGIN,ax
           mov     di,INITGR
           add     word ptr ss:[di+4],ax
           xor     dh,dh
YLOOP:     xor     dl,dl
SLOOP:     xor     ch,ch
           mov     di,INITGR
           call    CCODES
           push    cx
           mov     cx,MARGIN
           jcxz    NOMARG
           xor     bh,bh
MARGLOOP:  call    PRTOUT
           loop    MARGLOOP
NOMARG:    pop     cx
XLOOP:     xor     bh,bh
           xor     cl,cl
RLOOP:     mov     bl,dh
           add     bl,cl
           add     bl,dl
           call    POINT
           pushf
           rcl     bh,1
           cmp     DUMPMODE,1
           ja      NOTWO1
           popf
           rcl     bh,1
           pushf
NOTWO1:    popf
           add     cl,ROWSTEP
           cmp     cl,ROWEND
           jbe     RLOOP
           cmp     DUMPMODE,0
           jne     NOSHADE1
           push    bx
           xor     ah,ah
           shl     ax,2
           mov     si,ax
           mov     ah,bh
           and     bh,INKPAP[si]
           not     ah
           and     ah,INKPAP[si+1]
           or      bh,ah
           not     bh
NOSHADE1:  push    bx
           call    BORSUB1
           call    PRTOUT
           pop     bx
           cmp     DUMPMODE,1
           ja      NOTWO2
           je      NOSHADE2
           pop     bx
           mov     ah,bh
           and     bh,INKPAP[si+2]
           not     ah
           and     ah,INKPAP[si+3]
           or      bh,ah
           not     bh
NOSHADE2:  call    BORSUB2
           call    PRTOUT
NOTWO2:    inc     ch
           jnz     XLOOP
           mov     di,MOVEHEAD
           call    CCODES
           inc     dl
           cmp     dl,PASSNUM
           jumps
           jbe     SLOOP
           mov     di,CRLF
           call    CCODES
           add     dh,YSTEP
           cmp     dh,191
           jbe     YLOOP
           nojumps
           lea     di,CLGRCC
           call    CCODES
           mov     ax,MARGIN
           mov     di,INITGR
           sub     word ptr [di+4],ax
           jmp     REGENTRY

BORSUB1:   cmp     DUMPBRD,1           ; 2 potprograma za okvir oko slike
           je      MAKEBOR1
           ret
MAKEBOR1:  or      ch,ch
           jz      VERTL
           cmp     ch,255
           jnz     NOVERTL
           cmp     DUMPMODE,1
           ja      VERTL
NOVERTL:   or      dh,dh
           jz      TOPL
           push    dx
           add     dh,YSTEP
           cmp     dh,192
           pop     dx
           jz      BOTL
           ret
VERTL:     mov     bh,255
           ret
TOPL:      or      dl,dl
           jz      TOPL1
           ret
TOPL1:     or      bh,128
           ret
BOTL:      cmp     dl,PASSNUM
           je      BOTL1
           ret
BOTL1:     or      bh,1
           ret

BORSUB2:   cmp     DUMPBRD,1
           je      MAKEBOR2
           ret
MAKEBOR2:  cmp     ch,255
           je      VERTL
           jmp     NOVERTL

;
; Ispis brojeva. Vazan je MFLAG. Za b4=1 ispisuju se vodece nule ispred dekadnog
; broja. Za b5=1 se ne ispisuje # ispred hex brojeva. Za b6=1 ispis se vrsi u
; bafer za disasembleriranje.
;

NUM8SIGN:  mov     byte ptr ss:[di],'+'
           inc     di
           cmp     dl,128
           jb      NUM8
           mov     byte ptr ss:[di-1],'-'
           neg     dl
NUM8:      xor     dh,dh
           mov     DIGCNT,3
           jmp     short NUM16_1
NUM16:     mov     DIGCNT,5
NUM16_1:   push    ax
           push    bx
           push    cx
           push    bp
           xor     cx,cx
           lea     bx,NUMBUF[5]
           cmp     BASE,10
           je      NUMC
           or      MFLAG,16
           dec     DIGCNT
           test    MFLAG,32
           jnz     NUMC
           test    MFLAG,64
           jnz     NHASH
           mov     al,dl
           mov     dl,'#'
           call    PO_CHR1
           mov     dl,al
           jmp     short NUMC
NHASH:     mov     byte ptr ss:[di],'#'
           inc     di
NUMC:      mov     ax,dx
NUMLOOP:   dec     bx
           inc     cx
           xor     dx,dx
           div     BASE
           mov     bp,dx
           mov     dl,DIGITS[bp]
           mov     ss:[bx],dl
           test    MFLAG,16
           jnz     NUMFULL
           or      ax,ax
           jnz     NUMLOOP
           jmp     short NUMCONT
NUMFULL:   dec     DIGCNT
           jnz     NUMLOOP
NUMCONT:   test    MFLAG,64
           jnz     NPLOOP
           mov     di,bx
           call    PO_MSGL
           jmp     short NUMEND
NPLOOP:    mov     al,ss:[bx]
           mov     ss:[di],al
           inc     di
           inc     bx
           loop    NPLOOP
           dec     di
NUMEND:    pop     bp
           pop     cx
           pop     bx
           pop     ax
           ret

;
; Mini editor.
;

EDITOR:    mov     VMADDR,20704
           mov     QUOTE,0
           mov     cx,32
           mov     dl,cl
ECL:       call    PO_CHR1
           loop    ECL
           mov     VMADDR,20704
           mov     dl,'>'
           call    PO_CHR1
           cmp     OUTACT,1
           jne     NODUPPRPT
           call    PO_CHR1
NODUPPRPT: call    CLEARKBD
           lea     di,EDITBUF
           mov     cx,30
           sub     cl,OUTACT
ELOOP:     mov     dl,140
           call    PO_CHR1
           dec     VMADDR
           mov     TSKSWTCH,0
ELP1:      mov     ah,1                ; Isprazni bafer za tastaturu
           int     16H
           jz      ELPCONT
           xor     ah,ah
           int     16H
           jmp     ELP1
ELPCONT:   cmp     TSKSWTCH,1          ; Forsiraj "_" komandu ako Task switch
           mov     TSKSWTCH,0          ;  nastupi unutar ove petlje
           jne     ELPCONT2
           lea     di,EDITBUF
           mov     byte ptr ss:[di],'_'
           inc     di
           mov     al,13
           mov     ch,1
           jmp     short EDENT
ELPCONT2:  mov     ah,1                ; Cekaj na pritisak tipke
           int     16H
           jz      ELPCONT
           xor     ah,ah
           int     16H
           cmp     ax,3B00H
           je      F1KEY
           cmp     al,' '
           jae     LETTER
           cmp     al,13
           je      EDENT
           cmp     al,9
           jumps
           je      INCREG
           nojumps
           cmp     al,8
           jne     ELOOP
           or      ch,ch
           jz      ELOOP
           dec     di
           cmp     byte ptr ss:[di],''''
           jnz     NODELQ
           not     QUOTE
NODELQ:    dec     ch
           dec     VMADDR
           call    PO_SPACE
           call    PO_CHR1
           sub     VMADDR,2
           jmp     ELOOP
F1KEY:     pop     ax
           jmp     SHOWHELP
LETTER:    cmp     al,126
           jumps
           ja      ELOOP
           nojumps
           cmp     al,''''
           jne     LCONT
           not     QUOTE
LCONT:     cmp     al,'a'-1
           jb      LCASE
           cmp     QUOTE,255
           je      LCASE
           cmp     al,'z'
           ja      LCASE
           sub     al,32
LCASE:     cmp     ch,cl
           jumps
           je      ELOOP
           nojumps
           inc     ch
EDENT:     mov     ss:[di],al
           inc     di
           cmp     al,13
           je      EDEND
           mov     dl,al
           call    PO_CHR1
           jmp     ELOOP
EDEND:     or      ch,ch
           jne     EDEND1
           dec     di
           jmp     ELOOP
EDEND1:    mov     VMADDR,20672
           mov     cx,32
           mov     dl,cl
EENDL:     call    PO_CHR1
           loop    EENDL
           call    TESTOUT
           mov     dl,'>'
           call    CHAROUT
           lea     di,EDITBUF
EOUTL:     mov     dl,ss:[di]
           cmp     dl,13
           je      EDRET
           call    CHAROUT
           inc     di
           jmp     EOUTL
EDRET:     call    ENDOUT
           call    NEWLOUT
           ret

;
; Pomjeranje pokazivaca registara (poziva se iz editora).
;

INCREG:    push    VMADDR
           mov     bx,CURREG
           mov     ax,REGPOS[bx]
           dec     ax
           mov     VMADDR,ax
           call    PO_SPACE
           add     CURREG,2
           cmp     CURREG,26
           jne     IR1
           mov     CURREG,0
IR1:       mov     bx,CURREG
           mov     ax,REGPOS[bx]
           dec     ax
           mov     VMADDR,ax
           mov     dl,':'
           call    PO_CHR1
           pop     VMADDR
           jmp     ELOOP

;
; Brisanje bafera tastature i razne pomocne rutine za tastaturu.
;

CLEARKBD:  mov     ah,1
           int     16H
           jz      CLREND
           xor     ah,ah
           int     16H
           jmp     CLEARKBD
CLREND:    ret

WAITKEY:   call    CLEARKBD
           xor     ah,ah
           int     16H
           ret

;
; Skaniranje parametara.
;

SCANLINE:  lea     si,EDITBUF
           lea     di,PARAMS
           mov     PARCOUNT,0
           mov     COMCODE,0
           call    SKIPSPC
           cmp     al,13
           je      SCEND
           mov     COMCODE,al
SCLOOP:    call    SKIPSPC
           cmp     al,13
           je      SCEND
           xor     dx,dx
           xor     bh,bh
           mov     SCBASE,10
           cmp     al,'#'
           jne     SCNOHEX
           mov     SCBASE,16
           lods    byte ptr ss:[si]
           jmp     short NLOOP
SCNOHEX:   cmp     al,''''
           jne     QUEST
ASCLOOP:   lods    byte ptr ss:[si]
           cmp     al,13
           je      SCERROR
           cmp     al,''''
           je      ENDASC
           inc     PARCOUNT
           xor     ah,ah
           mov     ss:[di],ax
           add     di,2
           jmp     ASCLOOP
QUEST:     cmp     al,'?'
           jne     MEMPT
           mov     ax,-1
QDONE:     mov     word ptr ss:[di],ax
           inc     PARCOUNT
           add     di,2
ENDASC:    lods    byte ptr ss:[si]
           cmp     al,' '
           je      SCLOOP
           cmp     al,13
           je      SCEND
           jmp     short SCERROR
SCEND:     stc
           ret
MEMPT:     cmp     al,'.'
           jne     CREG
           mov     ax,STMP
           jmp     QDONE
CREG:      cmp     al,'$'
           jne     NLOOP
           mov     bx,CURREG
           add     bx,bx
           mov     ax,STAF[bx]
           xor     bh,bh
           jmp     QDONE
SCERROR:   clc
           ret
NLOOP:     cmp     al,'0'
           jb      SCERROR
           cmp     al,'9'
           jbe     NUMOK
           cmp     SCBASE,10
           je      SCERROR
           cmp     al,'A'
           jb      SCERROR
           cmp     al,'F'
           ja      SCERROR
           sub     al,7
NUMOK:     sub     al,'0'
           mov     bl,al
           mov     ax,dx
           mul     SCBASE
           mov     dx,ax
           add     dx,bx
           lods    byte ptr ss:[si]
           cmp     al,13
           je      SCPAR
           cmp     al,' '
           jne     NLOOP
SCPAR:     inc     PARCOUNT
           mov     ss:[di],dx
           add     di,2
           cmp     al,13
           je      SCEND
           jmp     SCLOOP

SKIPSPC:   lods    byte ptr ss:[si]
           cmp     al,32
           jz      SKIPSPC
           ret

;
; Ispis poruke i slicno.
;

PO_MSG:    mov     ax,ss:[di]          ; Ispis poruke uz postavljanje pozicije
           mov     VMADDR,ax
           add     di,2
PO_MSGL:   mov     dl,ss:[di]          ; Ispis poruke bez postavljanja pozicije
           inc     di
           cmp     dl,'@'
           je      PO_MSGE
           call    PO_CHR1
           jmp     PO_MSGL
PO_MSGE:   ret

PO_SPACE:  mov     dl,' '              ; Ispis razmaka

PO_CHR1:   test    OUTFLAG,2           ; Ispis znaka
           jnz     PO_CHR2
           call    PO_CHR
           jmp     short CHAROUT
PO_CHR2:   inc     VMADDR

;
; Neke sitne rutine za OUTPUT datoteku.
;

CHAROUT:   test    OUTFLAG,1           ; Salje znak u datoteku
           jnz     CHOUT1
           ret
CHOUT1:    pusha
           mov     OUTCHAR,dl
           mov     bx,OUTHANDLE
           mov     cx,1
           push    ds
           push    ss
           pop     ds
           lea     dx,OUTCHAR
           mov     ah,40H
           int     21H
           jc      CHOUTERR
           cmp     ax,cx
           jne     CHOUTERR
           pop     ds
           popa
           ret
CHOUTERR:  int     24H

TESTOUT:   cmp     OUTACT,1            ; Aktivira ispis u datoteku
           je      TOUT1
           ret
TOUT1:     mov     OUTFLAG,1
           ret

ENDOUT:    push    dx                  ; Zavrsava ispis u datoteku
           mov     dl,13
           call    CHAROUT
           mov     dl,10
           call    CHAROUT
           pop     dx
           mov     OUTFLAG,0
           ret

NEWLOUT:   call    TESTOUT             ; Salje CR LF u datoteku
           jmp     ENDOUT

;
; Ispisivanje panela u OUT datoteku, ako to treba.
;

PANELOUT:  mov     DBFLAG,0
           mov     si,STPC
           call    DISS_ONE
           call    TESTOUT
           or      OUTFLAG,2
           lea     di,FIRSTROW
           call    PO_MSGL
           lea     bx,STAF
           mov     cx,7
           mov     MFLAG,16
OUTRGLP1:  mov     dx,ss:[bx]          ; Ispisi registre
           call    NUM16
           add     bx,4
           call    PO_SPACE
           loop    OUTRGLP1
           lea     di,SECONDROW
           call    PO_MSGL
           mov     cx,7
OUTRGLP2:  mov     dx,ss:[bx]
           call    NUM16
           add     bx,4
           call    PO_SPACE
           loop    OUTRGLP2
           lea     di,THIRDROW
           call    PO_MSGL
           cmp     ARGFLG,1
           jne     NOOUTARG
           mov     dx,STARG            ; Ispisi ARG i (ARG)
           call    NUM16
           lea     di,THREESPC
           call    PO_MSGL
           mov     bx,STARG
           mov     dl,[bx]
           call    NUM8
           call    PO_SPACE
           jmp     short OUTFLAGS
NOOUTARG:  lea     di,TWELWESPC
           call    PO_MSGL
OUTFLAGS:  mov     ah,byte ptr STAF    ; Ispisi flegove
           call    FLPRINT
           call    PO_SPACE
           mov     ah,byte ptr STALTAF
           call    FLPRINT
           call    PO_SPACE
           call    PO_SPACE
           call    OUTINTS             ; Ispisi stanje interapta
           call    PO_SPACE
           call    PO_SPACE
           mov     dl,TIMCTL           ; Ispisi kontrolni registar
           mov     al,SAVECTL
           and     al,63
           and     dl,192
           or      dl,al
           call    NUM8
           lea     di,TWOCRLF
           call    PO_MSGL
           lea     di,MCMSG
           call    PO_MSGL
           mov     cx,8                ; Ispisi konfiguraciju memorije
           mov     bl,'X'
           mov     al,TIMSTAT
           test    TIMCTL,128
           jnz     CHNKOLP
           mov     bl,'D'
CHNKOLP:   mov     dl,bl
           rol     al,1
           jc      NOHOUT
           mov     dl,'H'
NOHOUT:    call    PO_CHR1
           loop    CHNKOLP
           lea     di,TWOCRLF
           call    PO_MSGL
           mov     DBFLAG,0            ; Ispisi tekucu liniju
           xor     ah,ah
           mov     si,STPC
           call    DISS_LINE
           call    ENDOUT
           jmp     NEWLOUT

OUTINTS:   call    PO_SPACE            ; Ispisuje stanje interapta
           mov     ax,IMODE
           add     al,'0'
           mov     dl,al
           call    PO_CHR1
           call    PO_SPACE
           mov     ax,IFF
           shr     ax,2
           add     ax,'00'
           mov     dl,al
           call    PO_CHR1
           mov     dl,ah
           jmp     PO_CHR1

MPOUT:     call    TESTOUT             ; Ispisuje memorijski pokazivac
           or      OUTFLAG,2
           lea     di,MPMSG
           call    PO_MSGL
           push    word ptr MFLAG
           mov     MFLAG,16
           mov     dx,STMP
           call    NUM16
           pop     word ptr MFLAG
           call    ENDOUT
           jmp     NEWLOUT

MONSEG     ends

ZXDSEG     segment public
ZXDSEG     ends

           end
