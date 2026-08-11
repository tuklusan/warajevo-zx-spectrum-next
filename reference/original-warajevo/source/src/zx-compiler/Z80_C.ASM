;
;          WARAJEVO Spectrum Simulator    Release 1.6
;                (C) Zeljko Juric & Samir Ribic
;

           .286

;          warn

           extrn   EASYSAVE:near,EASYLOAD:near
           extrn   BLOCKSAVE:near,BLOCKLOAD:near
           extrn   PORT_IN:near,PORT_OUT:near
           extrn   EXIT:near,SLOW_MAIN:near
           extrn   RESTTIMER:far

           public  TABLE1,IFF,GETINST
           public  RET_,I,R7,R,HSTATE,IFF1,IFF2,IMODE
           public  RCLK,PVN,SLOW
           public  NOP_,ALTA,IX
           public  ALTBC,ALTDE,ALTHL,IY,ALTF
           public  ALTPVN,PV,N
           public  ADJUSTTBL

;
; Alias imena za registre Z80.
;

A          equ     al

BC         equ     cx
C          equ     cl
B          equ     ch

DE         equ     dx
E          equ     dl
D          equ     dh

HL         equ     di

AUX        equ     bx
AUX_H      equ     bh
AUX_L      equ     bl

STP        equ     bp                  ; Stek pointer Z80
PC         equ     si                  ; Programski brojac Z80

;
; Alias imena za laksu izvedbu nekih makroa.
;

DE_H       equ     dh
DE_L       equ     dl

;
; Segment sistemskih varijabli i tabela.
;

SSEG       group   SSEG1,SSEG2

SSEG1      segment public

           if      VER_128

           extrn   MMUVAR:word

           endif

;
; Tabele izvrsnih rutina za pojedine instrukcije procesora Z80.
;

           even

TABLE1     dw      NOP_,LDBCNN,LDMBCA,INCBC,INC_B,DECB,LDBN,RLCA
           dw      EXAFAF,ADDHLBC,LDAMBC,DECBC,INC_C,DECC,LDCN,RRCA
           dw      DJNZ,LDDENN,LDMDEA,INCDE,INC_D,DECD,LDDN,RLA
           dw      JR,ADDHLDE,LDAMDE,DECDE,INC_E,DECE,LDEN,RRA
           dw      JRNZ,LDHLNN,LDMNNHL,INCHL,INC_H,DECH,LDHN,DAA_
           dw      JRZ,ADDHLHL,LDHLMNN,DECHL,INC_L,DECL,LDLN,CPL
           dw      JRNC,LDSPNN,LDMNNA,INCSTP,INC_MHL,DECMHL,LDMHLN,SCF
           dw      JRC,ADDHLSTP,LDAMNN,DECSTP,INC_A,DECA,LDAN,CCF
           dw      NOP_,LDBC,LDBD,LDBE,LDBH,LDBL,LDBMHL,LDBA
           dw      LDCB,NOP_,LDCD,LDCE,LDCH,LDCL,LDCMHL,LDCA
           dw      LDDB,LDDC,NOP_,LDDE,LDDH,LDDL,LDDMHL,LDDA
           dw      LDEB,LDEC,LDED,NOP_,LDEH,LDEL,LDEMHL,LDEA
           dw      LDHB,LDHC,LDHD,LDHE,NOP_,LDHL,LDHMHL,LDHA
           dw      LDLB,LDLC,LDLD,LDLE,LDLH,NOP_,LDLMHL,LDLA
           dw      LDMHLB,LDMHLC,LDMHLD,LDMHLE,LDMHLH,LDMHLL,HALT,LDMHLA
           dw      LDAB,LDAC,LDAD,LDAE,LDAH,LDAL,LDAMHL,NOP_
           dw      ADDAB,ADDAC,ADDAD,ADDAE,ADDAH,ADDAL,ADDAMHL,ADDAA
           dw      ADCAB,ADCAC,ADCAD,ADCAE,ADCAH,ADCAL,ADCAMHL,ADCAA
           dw      SUBB,SUBC,SUBD,SUBE,SUBH,SUBL,SUBMHL,SUBA
           dw      SBCAB,SBCAC,SBCAD,SBCAE,SBCAH,SBCAL,SBCAMHL,SBCAA
           dw      ANDB,ANDC,ANDD,ANDE,ANDH,ANDL,ANDMHL,ANDA
           dw      XORB,XORC,XORD,XORE,XORH,XORL,XORMHL,XORA
           dw      ORB,ORC,ORD,ORE,ORH,ORL,ORMHL,ORA
           dw      CP_B,CP_C,CP_D,CP_E,CP_H,CP_L,CP_MHL,CP_A
           dw      RETNZ,POPBC,JPNZ,JP_,CALLNZ,PUSHBC,ADDAN,RST0
           dw      RETZ,RET_,JPZ,PREFIXCB,CALLZ,CALL_,ADCAN,RST8
           dw      RETNC,POPDE,JPNC,OUTNA,CALLNC,PUSHDE,SUBN,RST16
           dw      RETC,EXX,JPC,INAN,CALLC,PREFIXDD,SBCAN,RST24
           dw      RETPO,POPHL,JPPO,EXMSPHL,CALLPO,PUSHHL,ANDN,RST32
           dw      RETPE,JPHL,JPPE,EXDEHL,CALLPE,PREFIXED,XORN,RST40
           dw      RETP,POPAF,JPP,DI_,CALLP,PUSHAF,ORN,RST48
           dw      RETM,LDSPHL,JPM,EI,CALLM,PREFIXFD,CP_N,RST56

TABLE2     dw      RLC_B,RLC_C,RLC_D,RLC_E,RLC_H,RLC_L,RLCMHL,RLC_A
           dw      RRC_B,RRC_C,RRC_D,RRC_E,RRC_H,RRC_L,RRCMHL,RRC_A
           dw      RL_B,RL_C,RL_D,RL_E,RL_H,RL_L,RLMHL,RL_A
           dw      RR_B,RR_C,RR_D,RR_E,RR_H,RR_L,RRMHL,RR_A
           dw      SLAB,SLAC,SLAD,SLAE,SLAH,SLAL,SLAMHL,SLAA
           dw      SRAB,SRAC,SRAD,SRAE,SRAH,SRAL,SRAMHL,SRAA
           dw      SLLB,SLLC,SLLD,SLLE,SLLH,SLLL,SLLMHL,SLLA
           dw      SRLB,SRLC,SRLD,SRLE,SRLH,SRLL,SRLMHL,SRLA
           dw      BIT0B,BIT0C,BIT0D,BIT0E,BIT0H,BIT0L,BIT0MHL,BIT0A
           dw      BIT1B,BIT1C,BIT1D,BIT1E,BIT1H,BIT1L,BIT1MHL,BIT1A
           dw      BIT2B,BIT2C,BIT2D,BIT2E,BIT2H,BIT2L,BIT2MHL,BIT2A
           dw      BIT3B,BIT3C,BIT3D,BIT3E,BIT3H,BIT3L,BIT3MHL,BIT3A
           dw      BIT4B,BIT4C,BIT4D,BIT4E,BIT4H,BIT4L,BIT4MHL,BIT4A
           dw      BIT5B,BIT5C,BIT5D,BIT5E,BIT5H,BIT5L,BIT5MHL,BIT5A
           dw      BIT6B,BIT6C,BIT6D,BIT6E,BIT6H,BIT6L,BIT6MHL,BIT6A
           dw      BIT7B,BIT7C,BIT7D,BIT7E,BIT7H,BIT7L,BIT7MHL,BIT7A
           dw      RES0B,RES0C,RES0D,RES0E,RES0H,RES0L,RES0MHL,RES0A
           dw      RES1B,RES1C,RES1D,RES1E,RES1H,RES1L,RES1MHL,RES1A
           dw      RES2B,RES2C,RES2D,RES2E,RES2H,RES2L,RES2MHL,RES2A
           dw      RES3B,RES3C,RES3D,RES3E,RES3H,RES3L,RES3MHL,RES3A
           dw      RES4B,RES4C,RES4D,RES4E,RES4H,RES4L,RES4MHL,RES4A
           dw      RES5B,RES5C,RES5D,RES5E,RES5H,RES5L,RES5MHL,RES5A
           dw      RES6B,RES6C,RES6D,RES6E,RES6H,RES6L,RES6MHL,RES6A
           dw      RES7B,RES7C,RES7D,RES7E,RES7H,RES7L,RES7MHL,RES7A
           dw      SET0B,SET0C,SET0D,SET0E,SET0H,SET0L,SET0MHL,SET0A
           dw      SET1B,SET1C,SET1D,SET1E,SET1H,SET1L,SET1MHL,SET1A
           dw      SET2B,SET2C,SET2D,SET2E,SET2H,SET2L,SET2MHL,SET2A
           dw      SET3B,SET3C,SET3D,SET3E,SET3H,SET3L,SET3MHL,SET3A
           dw      SET4B,SET4C,SET4D,SET4E,SET4H,SET4L,SET4MHL,SET4A
           dw      SET5B,SET5C,SET5D,SET5E,SET5H,SET5L,SET5MHL,SET5A
           dw      SET6B,SET6C,SET6D,SET6E,SET6H,SET6L,SET6MHL,SET6A
           dw      SET7B,SET7C,SET7D,SET7E,SET7H,SET7L,SET7MHL,SET7A

TABLE3     dw      GO8086,63 dup (NOP_)
           dw      INBC,OUTCB,SBCHLBC,LDMNNBC,NEG_,RETN_,IM0,LDIA
           dw      INCC,OUTCC,ADCHLBC,LDBCMNN,NEG_,RETI,IM0,LDRA
           dw      INDC,OUTCD,SBCHLDE,LDMNNDE,NEG_,RETN_,IM1,LDAI
           dw      INEC,OUTCE,ADCHLDE,LDDEMNN,NEG_,RETI,IM2,LDAR
           dw      INHC,OUTCH,SBCHLHL,LDMNNHL,NEG_,RETN_,IM0,RRD
           dw      INLC,OUTCL,ADCHLHL,LDHLMNN,NEG_,RETI,IM0,RLD
           dw      INFC,OUTC0,SBCHLSTP,LDMNNSTP,NEG_,RETN_,IM1,NOP_
           dw      INAC,OUTCA,ADCHLSTP,LDSPMNN,NEG_,RETI,IM2,NOP_
           dw      32 dup (NOP_)
           dw      LDI,CPI,INI,OUTI,4 dup (NOP_)
           dw      LDD,CPD,IND,OUTD,4 dup (NOP_)
           dw      LDIR,CPIR,INIR,OTIR,4 dup (NOP_)
           dw      LDDR,CPDR,INDR,OTDR,4 dup (NOP_)
           dw      60 dup (NOP_),EASYSAVE,EASYLOAD,BLOCKSAVE,BLOCKLOAD

TABLE4     dw      NOP_,LDBCNN,LDMBCA,INCBC,INC_B,DECB,LDBN,RLCA
           dw      EXAFAF,ADDIXBC,LDAMBC,DECBC,INC_C,DECC,LDCN,RRCA
           dw      DJNZ,LDDENN,LDMDEA,INCDE,INC_D,DECD,LDDN,RLA
           dw      JR,ADDIXDE,LDAMDE,DECDE,INC_E,DECE,LDEN,RRA
           dw      JRNZ,LDIXNN,LDMNNIX,INCIX,INC_XH,DECXH,LDXHN,DAA_
           dw      JRZ,ADDIXIX,LDIXMNN,DECIX,INC_XL,DECXL,LDXLN,CPL
           dw      JRNC,LDSPNN,LDMNNA,INCSTP,INC_MIX,DECMIX,LDMIXN,SCF
           dw      JRC,ADDIXSTP,LDAMNN,DECSTP,INC_A,DECA,LDAN,CCF
           dw      NOP_,LDBC,LDBD,LDBE,LDBXH,LDBXL,LDBMIX,LDBA
           dw      LDCB,NOP_,LDCD,LDCE,LDCXH,LDCXL,LDCMIX,LDCA
           dw      LDDB,LDDC,NOP_,LDDE,LDDXH,LDDXL,LDDMIX,LDDA
           dw      LDEB,LDEC,LDED,NOP_,LDEXH,LDEXL,LDEMIX,LDEA
           dw      LDXHB,LDXHC,LDXHD,LDXHE,NOP_,LDXHXL,LDHMIX,LDXHA
           dw      LDXLB,LDXLC,LDXLD,LDXLE,LDXLXH,NOP_,LDLMIX,LDXLA
           dw      LDMIXB,LDMIXC,LDMIXD,LDMIXE,LDMIXH,LDMIXL,HALT,LDMIXA
           dw      LDAB,LDAC,LDAD,LDAE,LDAXH,LDAXL,LDAMIX,NOP_
           dw      ADDAB,ADDAC,ADDAD,ADDAE,ADDAXH,ADDAXL,ADDAMIX,ADDAA
           dw      ADCAB,ADCAC,ADCAD,ADCAE,ADCAXH,ADCAXL,ADCAMIX,ADCAA
           dw      SUBB,SUBC,SUBD,SUBE,SUBXH,SUBXL,SUBMIX,SUBA
           dw      SBCAB,SBCAC,SBCAD,SBCAE,SBCAXH,SBCAXL,SBCAMIX,SBCAA
           dw      ANDB,ANDC,ANDD,ANDE,ANDXH,ANDXL,ANDMIX,ANDA
           dw      XORB,XORC,XORD,XORE,XORXH,XORXL,XORMIX,XORA
           dw      ORB,ORC,ORD,ORE,ORXH,ORXL,ORMIX,ORA
           dw      CP_B,CP_C,CP_D,CP_E,CP_XH,CP_XL,CP_MIX,CP_A
           dw      RETNZ,POPBC,JPNZ,JP_,CALLNZ,PUSHBC,ADDAN,RST0
           dw      RETZ,RET_,JPZ,PREFDDCB,CALLZ,CALL_,ADCAN,RST8
           dw      RETNC,POPDE,JPNC,OUTNA,CALLNC,PUSHDE,SUBN,RST16
           dw      RETC,EXX,JPC,INAN,CALLC,PREFIXDD,SBCAN,RST24
           dw      RETPO,POPIX,JPPO,EXMSPIX,CALLPO,PUSHIX,ANDN,RST32
           dw      RETPE,JPIX,JPPE,EXDEHL,CALLPE,PREFIXED,XORN,RST40
           dw      RETP,POPAF,JPP,DI_,CALLP,PUSHAF,ORN,RST48
           dw      RETM,LDSPIX,JPM,EI,CALLM,PREFIXFD,CP_N,RST56

TABLE5     dw      NOP_,LDBCNN,LDMBCA,INCBC,INC_B,DECB,LDBN,RLCA
           dw      EXAFAF,ADDIYBC,LDAMBC,DECBC,INC_C,DECC,LDCN,RRCA
           dw      DJNZ,LDDENN,LDMDEA,INCDE,INC_D,DECD,LDDN,RLA
           dw      JR,ADDIYDE,LDAMDE,DECDE,INC_E,DECE,LDEN,RRA
           dw      JRNZ,LDIYNN,LDMNNIY,INCIY,INC_YH,DECYH,LDYHN,DAA_
           dw      JRZ,ADDIYIY,LDIYMNN,DECIY,INC_YL,DECYL,LDYLN,CPL
           dw      JRNC,LDSPNN,LDMNNA,INCSTP,INC_MIY,DECMIY,LDMIYN,SCF
           dw      JRC,ADDIYSTP,LDAMNN,DECSTP,INC_A,DECA,LDAN,CCF
           dw      NOP_,LDBC,LDBD,LDBE,LDBYH,LDBYL,LDBMIY,LDBA
           dw      LDCB,NOP_,LDCD,LDCE,LDCYH,LDCYL,LDCMIY,LDCA
           dw      LDDB,LDDC,NOP_,LDDE,LDDYH,LDDYL,LDDMIY,LDDA
           dw      LDEB,LDEC,LDED,NOP_,LDEYH,LDEYL,LDEMIY,LDEA
           dw      LDYHB,LDYHC,LDYHD,LDYHE,NOP_,LDYHYL,LDHMIY,LDYHA
           dw      LDYLB,LDYLC,LDYLD,LDYLE,LDYLYH,NOP_,LDLMIY,LDYLA
           dw      LDMIYB,LDMIYC,LDMIYD,LDMIYE,LDMIYH,LDMIYL,HALT,LDMIYA
           dw      LDAB,LDAC,LDAD,LDAE,LDAYH,LDAYL,LDAMIY,NOP_
           dw      ADDAB,ADDAC,ADDAD,ADDAE,ADDAYH,ADDAYL,ADDAMIY,ADDAA
           dw      ADCAB,ADCAC,ADCAD,ADCAE,ADCAYH,ADCAYL,ADCAMIY,ADCAA
           dw      SUBB,SUBC,SUBD,SUBE,SUBYH,SUBYL,SUBMIY,SUBA
           dw      SBCAB,SBCAC,SBCAD,SBCAE,SBCAYH,SBCAYL,SBCAMIY,SBCAA
           dw      ANDB,ANDC,ANDD,ANDE,ANDYH,ANDYL,ANDMIY,ANDA
           dw      XORB,XORC,XORD,XORE,XORYH,XORYL,XORMIY,XORA
           dw      ORB,ORC,ORD,ORE,ORYH,ORYL,ORMIY,ORA
           dw      CP_B,CP_C,CP_D,CP_E,CP_YH,CP_YL,CP_MIY,CP_A
           dw      RETNZ,POPBC,JPNZ,JP_,CALLNZ,PUSHBC,ADDAN,RST0
           dw      RETZ,RET_,JPZ,PREFFDCB,CALLZ,CALL_,ADCAN,RST8
           dw      RETNC,POPDE,JPNC,OUTNA,CALLNC,PUSHDE,SUBN,RST16
           dw      RETC,EXX,JPC,INAN,CALLC,PREFIXDD,SBCAN,RST24
           dw      RETPO,POPIY,JPPO,EXMSPIY,CALLPO,PUSHIY,ANDN,RST32
           dw      RETPE,JPIY,JPPE,EXDEHL,CALLPE,PREFIXED,XORN,RST40
           dw      RETP,POPAF,JPP,DI_,CALLP,PUSHAF,ORN,RST48
           dw      RETM,LDSPIY,JPM,EI,CALLM,PREFIXFD,CP_N,RST56

TABLE6     dw      RLCPIB,RLCPIC,RLCPID,RLCPIE,RLCPIH,RLCPIL,RLCI,RLCPIA
           dw      RRCPIB,RRCPIC,RRCPID,RRCPIE,RRCPIH,RRCPIL,RRCI,RRCPIA
           dw      RLPIB,RLPIC,RLPID,RLPIE,RLPIH,RLPIL,RLI,RLPIA
           dw      RRPIB,RRPIC,RRPID,RRPIE,RRPIH,RRPIL,RRI,RRPIA
           dw      SLAPIB,SLAPIC,SLAPID,SLAPIE,SLAPIH,SLAPIL,SLAI,SLAPIA
           dw      SRAPIB,SRAPIC,SRAPID,SRAPIE,SRAPIH,SRAPIL,SRAI,SRAPIA
           dw      SLLPIB,SLLPIC,SLLPID,SLLPIE,SLLPIH,SLLPIL,SLLI,SLLPIA
           dw      SRLPIB,SRLPIC,SRLPID,SRLPIE,SRLPIH,SRLPIL,SRLI,SRLPIA
           dw      8 dup (BIT0I),8 dup (BIT1I)
           dw      8 dup (BIT2I),8 dup (BIT3I)
           dw      8 dup (BIT4I),8 dup (BIT5I)
           dw      8 dup (BIT6I),8 dup (BIT7I)
           dw      RESP0B,RESP0C,RESP0D,RESP0E,RESP0H,RESP0L,RES0I,RESP0A
           dw      RESP1B,RESP1C,RESP1D,RESP1E,RESP1H,RESP1L,RES1I,RESP1A
           dw      RESP2B,RESP2C,RESP2D,RESP2E,RESP2H,RESP2L,RES2I,RESP2A
           dw      RESP3B,RESP3C,RESP3D,RESP3E,RESP3H,RESP3L,RES3I,RESP3A
           dw      RESP4B,RESP4C,RESP4D,RESP4E,RESP4H,RESP4L,RES4I,RESP4A
           dw      RESP5B,RESP5C,RESP5D,RESP5E,RESP5H,RESP5L,RES5I,RESP5A
           dw      RESP6B,RESP6C,RESP6D,RESP6E,RESP6H,RESP6L,RES6I,RESP6A
           dw      RESP7B,RESP7C,RESP7D,RESP7E,RESP7H,RESP7L,RES7I,RESP7A
           dw      SETP0B,SETP0C,SETP0D,SETP0E,SETP0H,SETP0L,SET0I,SETP0A
           dw      SETP1B,SETP1C,SETP1D,SETP1E,SETP1H,SETP1L,SET1I,SETP1A
           dw      SETP2B,SETP2C,SETP2D,SETP2E,SETP2H,SETP2L,SET2I,SETP2A
           dw      SETP3B,SETP3C,SETP3D,SETP3E,SETP3H,SETP3L,SET3I,SETP3A
           dw      SETP4B,SETP4C,SETP4D,SETP4E,SETP4H,SETP4L,SET4I,SETP4A
           dw      SETP5B,SETP5C,SETP5D,SETP5E,SETP5H,SETP5L,SET5I,SETP5A
           dw      SETP6B,SETP6C,SETP6D,SETP6E,SETP6H,SETP6L,SET6I,SETP6A
           dw      SETP7B,SETP7C,SETP7D,SETP7E,SETP7H,SETP7L,SET7I,SETP7A

;
; Neke pomocne varijable.
;

PAGEWORDS  dw      4096                ; 8192 ako je /EX, a mijenja ga i 386

SSEG1      ends

SSEG2      segment public

           extrn   FLAGSL:byte,FLAGSH:byte,FLAGS:word
           extrn   ATTRCHG:word,VIDEOCHG:word

           if      VER_128

           extrn   ACT_HL:word,ACT_AUX:word,ACT_STP:word
           extrn   MMUSTATE:byte

           endif

;
; Neki Z80 registri i interni flegovi.
;

;          evendata

PVN        label   word                ; Cesto se PV i N azuriraju zajedno
N          db      ?                   ; 0 ako je N=0, 2 ako je N=1
PV         db      ?                   ; 0 ako je P/V flag P, 255 ako je V

IX         label   word                ; Sve instrukcije koje eksplicitno
XL         db      ?                   ; operisu sa XH i XL (visom i nizom
XH         db      ?                   ; polovinom registra IX) su sinteticke

IY         label   word                ; Primjedba kao i za IX
YL         db      ?
YH         db      ?

ALTA       db      ?                   ; A'

;          evendata

ALTF       dw      ?                   ; alternativni FLAGS za 8086
ALTPVN     dw      ?                   ; alternativna vrijednost za PVN
ALTBC      dw      ?                   ; B'C'
ALTDE      dw      ?                   ; D'E'
ALTHL      dw      ?                   ; H'L'

IFF        label   word                ; Dva IFF flip-flopa Z80
IFF1       db      ?                   ; 0 ako je DI, 4 ako je EI
IFF2       db      ?                   ; Fleg IFF2 cuva IFF1 u slucaju NMI

IMODE      db      ?                   ; 0 za IM 0, 1 za IM 1, 2 za IM 2
HSTATE     db      ?                   ; 1 ako je HALT stanje, inace 0
           db      ?                   ; Lazni bajt za neke trikove

I          db      ?                   ; Interrupt vektor registar

RCLK       label   word
R          db      ?                   ; Refresh registar bitovi b0-67
CLK        db      ?
R7         db      ?                   ; Refresh registar bit b7

;          evendata

RETADD     dw      ?                   ; Povratna adresa za SLOW potprogram
SAVEBC     dw      ?                   ; Privremeni spremnik za BC

SSEG2      ends

CSEG       segment public

           assume  cs:CSEG,ss:SSEG,ds:ZXDSEG,es:nothing

;
; Makroi koji pribavljaju kod instrukcije. FX se stavlja ako je BH vec 0.
;

F          macro
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           jmp     TABLE1[AUX]
           endm

FX         macro
           mov     AUX_L,[PC]
           add     AUX,AUX
           jmp     TABLE1[AUX]
           endm

;
; Makroi koji racunaju efektivnu adresu kod indeksnog adresiranja u AUX.
;

INDIX      macro
           mov     AUX,ax
           lodsb
           cbw
           xchg    AUX,ax
           add     AUX,IX
           endm

INDIY      macro
           mov     AUX,ax
           lodsb
           cbw
           xchg    AUX,ax
           add     AUX,IY
           endm

;
; Rutina za Real-time rezim.
;

SLOW:      call    SLOW_MAIN
           pop     RETADD
           add     RETADD,9
           jmp     RETADD

;
; Makro za simulaciju osvjezavanja dinamicke memorije, brojanja ciklusa i
; uvecavanje PC registra. RC se poziva ako ne treba mijenjati R registar.
;

RF         macro   CNT
           local   IFSLOW
IFSLOW:    call    SLOW
           add     RCLK,1+256*CNT
           jc      IFSLOW
           inc     PC
           endm

RC         macro   CNT
           local   IFSLOW
IFSLOW:    call    SLOW
           add     RCLK,256*CNT
           jc      IFSLOW
           inc     PC
           endm


;
; Makro za pristup memoriji vodeci racuna da li se smjesta u bit mapu ekrana,
; atribute, obicni RAM ili se pokusava upis u ROM. Takodje vodi racuna o
; tretmanu memorijskih stranica 2 i 5 na 128 verziji.
;

WRITE      macro   ADR,BYTE
           local   RAM,ROM
           ifidn   <ADR>,<AUX>
           cmp     AUX_H,91
           jnb     short RAM
           cmp     AUX_H,64
           jb      short ROM
           cmp     AUX_H,88
           else
           cmp     ADR,23296
           jnb     RAM
           cmp     ADR,16384
           jb      short ROM
           cmp     ADR,22528
           endif
           adc     VIDEOCHG,0
           cmc
           adc     ATTRCHG,0
RAM:       mov     ds:[ADR],BYTE
           if      VER_128
           mov     es:[ADR],BYTE
           cmp     ADR,MMUVAR
           jbe     ROM
           ifdif   <BYTE>,<ah>
           mov     ah,BYTE
           endif
           call    ACT_&ADR
           endif
ROM:
           endm

;
; Makro za setovanje bita u memoriji.
;

SETMEM     macro   ADR,BIT
           local   RAM,ROM
           ifidn   <ADR>,<AUX>
           cmp     AUX_H,91
           jnb     short RAM
           cmp     AUX_H,64
           jb      short ROM
           cmp     AUX_H,88
           else
           cmp     ADR,23296
           jnb     RAM
           cmp     ADR,16384
           jb      short ROM
           cmp     ADR,22528
           endif
           adc     VIDEOCHG,0
           cmc
           adc     ATTRCHG,0
RAM:       or      byte ptr ds:[ADR],1 shl BIT
           if      VER_128
           or      byte ptr es:[ADR],1 shl BIT
           cmp     ADR,MMUVAR
           jbe     ROM
           mov     ah,ds:[ADR]
           call    ACT_&ADR
           endif
ROM:
           endm

;
; Makro za resetovanje bita u memoriji.
;

RESMEM     macro   ADR,BIT
           local   RAM,ROM
           ifidn   <ADR>,<AUX>
           cmp     AUX_H,91
           jnb     short RAM
           cmp     AUX_H,64
           jb      short ROM
           cmp     AUX_H,88
           else
           cmp     ADR,23296
           jnb     RAM
           cmp     ADR,16384
           jb      short ROM
           cmp     ADR,22528
           endif
           adc     VIDEOCHG,0
           cmc
           adc     ATTRCHG,0
RAM:       and     byte ptr ds:[ADR],255-(1 shl BIT)
           if      VER_128
           and     byte ptr es:[ADR],255-(1 shl BIT)
           cmp     ADR,MMUVAR
           jbe     ROM
           mov     ah,ds:[ADR]
           call    ACT_&ADR
           endif
ROM:
           endm

;
; Potprogram koji sredjuje entry point.
;

ADJUSTTBL: push    cx
           push    si
           xor     si,si
           mov     cx,6*256
ADJLOOP:   add     TABLE1[si],3
           add     si,2
           loop    ADJLOOP
           pop     si
           pop     cx
           ret

;
; Sada slijedi simulacija procesora Z80 uz neke sitnice za simulaciju hardvera
; ZX Spectruma prilikom smjestanja u memoriju i pristupa ulazno-izlaznoj mapi.
;

; имммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE 8-BITNOG PRENOSA ╨
; хмммммммммммммммммммммммммммммм╪

;
; Ove instrukcije ne uticu ni na jedan fleg.
;

; зддддддддддддддддддддддддддддддддддддддд©
; Ё LD  reg1,reg2                         Ё
; Ё reg1,reg2 = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; Ё reg1 = H,L => reg2 <> XH,XL,YH,YL     Ё
; Ё reg1 = XH,XL => reg2 <> H,L,YH,YL     Ё
; Ё reg1 = YH,YL => reg2 <> H,L,XH,XL     Ё
; юддддддддддддддддддддддддддддддддддддддды

           irp     REG,<B,C,D,E>

LDA&REG:   RF      4
           mov     A,REG
           FX

           endm

           irp     REG,<XH,XL,YH,YL>

LDA&REG:   RF      4                   ;; Mozda je 3. Ovo vazi i dolje.
           mov     A,REG
           FX

           endm

           irp     REG,<A,C,D,E>

LDB&REG:   RF      4
           mov     B,REG
           FX

           endm

           irp     REG,<XH,XL,YH,YL>

LDB&REG:   RF      4
           mov     B,REG
           FX

           endm

           irp     REG,<A,B,D,E>

LDC&REG:   RF      4
           mov     C,REG
           FX

           endm

           irp     REG,<XH,XL,YH,YL>

LDC&REG:   RF      4
           mov     C,REG
           FX

           endm

           irp     REG,<A,B,C,E>

LDD&REG:   RF      4
           mov     D,REG
           FX

           endm

           irp     REG,<XH,XL,YH,YL>

LDD&REG:   RF      4
           mov     D,REG
           FX

           endm

           irp     REG,<A,B,C,D>

LDE&REG:   RF      4
           mov     E,REG
           FX

           endm

           irp     REG,<XH,XL,YH,YL>

LDE&REG:   RF      4
           mov     E,REG
           FX

           endm

           irp     REG,<A,B,C,D,E>

LDH&REG:   RF      4
           xchg    AUX,HL
           mov     AUX_H,REG
           xchg    AUX,HL
           FX

           endm

           irp     REG,<A,B,C,D,E>

LDL&REG:   RF      4
           xchg    AUX,HL
           mov     AUX_L,REG
           xchg    AUX,HL
           FX

           endm

           irp     REG,<A,B,C,D,E>

LDXH&REG:  RF      4
           mov     XH,REG
           FX

           endm

LDXHXL:    RF      4
           mov     ah,XL
           mov     XH,ah
           FX

           irp     REG,<A,B,C,D,E>

LDXL&REG:  RF      4
           mov     XL,REG
           FX

           endm

LDXLXH:    RF      4
           mov     ah,XH
           mov     XL,ah
           FX

           irp     REG,<A,B,C,D,E>

LDYH&REG:  RF      4
           mov     YH,REG
           FX

           endm

LDYHYL:    RF      4
           mov     ah,YL
           mov     YH,ah
           FX

           irp     REG,<A,B,C,D,E>

LDYL&REG:  RF      4
           mov     YL,REG
           FX

           endm

LDYLYH:    RF      4
           mov     ah,YH
           mov     YL,ah
           FX

LDAH:      RF      4
           mov     AUX,HL
           mov     A,AUX_H
           F

LDBH:      RF      4
           mov     AUX,HL
           mov     B,AUX_H
           F

LDCH:      RF      4
           mov     AUX,HL
           mov     C,AUX_H
           F

LDDH:      RF      4
           mov     AUX,HL
           mov     D,AUX_H
           F

LDEH:      RF      4
           mov     AUX,HL
           mov     E,AUX_H
           F

LDLH:      RF      4
           xchg    AUX,HL
           mov     AUX_L,AUX_H
           xchg    AUX,HL
           FX

LDAL:      RF      4
           mov     AUX,HL
           mov     A,AUX_L
           F

LDBL:      RF      4
           mov     AUX,HL
           mov     B,AUX_L
           F

LDCL:      RF      4
           mov     AUX,HL
           mov     C,AUX_L
           F

LDDL:      RF      4
           mov     AUX,HL
           mov     D,AUX_L
           F

LDEL:      RF      4
           mov     AUX,HL
           mov     E,AUX_L
           F

LDHL:      RF      4
           xchg    AUX,HL
           mov     AUX_H,AUX_L
           xchg    AUX,HL
           F

; зддддддддддддддддддддддддддддддддд©
; Ё LD  reg,n                       Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

LDAN:      RF      7
           mov     A,[PC]
           inc     PC
           FX

LDBN:      RF      7
           mov     B,[PC]
           inc     PC
           FX

LDCN:      RF      7
           mov     C,[PC]
           inc     PC
           FX

LDDN:      RF      7
           mov     D,[PC]
           inc     PC
           FX

LDEN:      RF      7
           mov     E,[PC]
           inc     PC
           FX

LDHN:      RF      7
           xchg    AUX,HL
           mov     AUX_H,[PC]
           xchg    AUX,HL
           inc     PC
           FX

LDLN:      RF      7
           xchg    AUX,HL
           mov     AUX_L,[PC]
           xchg    AUX,HL
           inc     PC
           FX

LDXHN:     RF      7                   ; ? ili 6
           mov     ah,[PC]
           mov     XH,ah
           inc     PC
           FX

LDXLN:     RF      7
           mov     ah,[PC]
           mov     XL,ah
           inc     PC
           FX

LDYHN:     RF      7
           mov     ah,[PC]
           mov     YH,ah
           inc     PC
           FX

LDYLN:     RF      7
           mov     ah,[PC]
           mov     YL,ah
           inc     PC
           FX

; зддддддддддддддддддддд©
; Ё LD  reg,(HL)        Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

LDAMHL:    RF      7
           mov     A,[HL]
           FX

LDBMHL:    RF      7
           mov     B,[HL]
           FX

LDCMHL:    RF      7
           mov     C,[HL]
           FX

LDDMHL:    RF      7
           mov     D,[HL]
           FX

LDEMHL:    RF      7
           mov     E,[HL]
           FX

LDHMHL:    RF      7
           xchg    AUX,HL
           mov     AUX_H,[AUX]
           xchg    AUX,HL
           FX

LDLMHL:    RF      7
           xchg    AUX,HL
           mov     AUX_L,[AUX]
           xchg    AUX,HL
           FX

; зддддддддддддддддддддд©
; Ё LD  reg,(regpar+d)  Ё
; Ё reg = A,B,C,D,E,H,L Ё
; Ё regpar = IX,IY      Ё
; юддддддддддддддддддддды

           irp     IR,<IX,IY>

LDAM&IR:   RF      15
           IND&IR
           mov     A,[AUX]
           F

LDBM&IR:   RF      15
           IND&IR
           mov     B,[AUX]
           F

LDCM&IR:   RF      15
           IND&IR
           mov     C,[AUX]
           F

LDDM&IR:   RF      15
           IND&IR
           mov     D,[AUX]
           F

LDEM&IR:   RF      15
           IND&IR
           mov     E,[AUX]
           F

LDHM&IR:   RF      15
           IND&IR
           xchg    DE,HL
           mov     D,[AUX]
           xchg    DE,HL
           F

LDLM&IR:   RF      15
           IND&IR
           xchg    DE,HL
           mov     E,[AUX]
           xchg    DE,HL
           F

           endm

; зддддддддддддддддддддд©
; Ё LD  (HL),reg        Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

LDMHL&REG: RF      7
           WRITE   HL,REG
           FX

           endm

           irp     REG,<H,L>

LDMHL&REG: RF      7
           mov     AUX,HL
           WRITE   AUX,AUX_&REG
           F

           endm

; зддддддддддддддддддддд©
; Ё LD  (regpar+d),reg  Ё
; Ё reg = A,B,C,D,E,H,L Ё
; Ё regpar = IX,IY      Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

LDMIX&REG: RF      15
           INDIX
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

LDMIX&REG: RF      15
           INDIX
           xchg    DE,HL
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

           irp     REG,<A,B,C,D,E>

LDMIY&REG: RF      15
           INDIY
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

LDMIY&REG: RF      15
           INDIY
           xchg    DE,HL
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддд©
; Ё LD  (HL),n Ё
; юдддддддддддды

LDMHLN:    RF      10
           mov     ah,[PC]
           WRITE   HL,ah
           inc     PC
           FX

; здддддддддддддддддд©
; Ё LD  (regpar+d),n Ё
; Ё regpar = IX,IY   Ё
; юдддддддддддддддддды

LDMIXN:    RF      15
           INDIX
           mov     ah,[PC]
           WRITE   AUX,ah
           inc     PC
           F

LDMIYN:    RF      15
           INDIY
           mov     ah,[PC]
           WRITE   AUX,ah
           inc     PC
           F

; здддддддддддддддд©
; Ё LD  A,(regpar) Ё
; Ё regpar = BC,DE Ё
; юдддддддддддддддды

           irp     RP,<BC,DE>

LDAM&RP:   RF      7
           mov     AUX,RP
           mov     A,[AUX]
           F

           endm

; здддддддддддд©
; Ё LD  A,(nn) Ё
; юдддддддддддды

LDAMNN:    RF      13
           mov     AUX,[PC]
           mov     A,[AUX]
           add     PC,2
           F

; здддддддддддддддд©
; Ё LD  (regpar),A Ё
; Ё regpar = BC,DE Ё
; юдддддддддддддддды

LDMBCA:    RF      7
           mov     AUX,BC
           WRITE   AUX,A
           F

LDMDEA:    RF      7
           mov     AUX,DE
           WRITE   AUX,A
           F

; здддддддддддд©
; Ё LD  (nn),A Ё
; юдддддддддддды

LDMNNA:    RF      13
           mov     AUX,[PC]
           WRITE   AUX,A
           add     PC,2
           F

; зддддддддддд©
; Ё LD  reg,A Ё
; Ё reg = I,R Ё
; юддддддддддды

LDIA:      RF      5                   ; Niposto 9-4 - zeza makro ekspanzija!
           mov     I,A
           FX

LDRA:      RF      5
           mov     R7,A
           mov     R,A
           FX

;
; Ove instrukcije su iznimka. Uticu na Z i S flag, P/V flag poprima vrijednost
; IFF2 flip-flopa, H i N flegovi se resetuju.
;

; зддддддддддд©
; Ё LD  A,reg Ё
; Ё reg = I,R Ё
; юддддддддддды

LDAI:      RF      5
           mov     PVN,0
           mov     A,I
           or      A,A
           lahf
           and     ah,0EAH
           or      ah,IFF2
           and     FLAGSL,1
           or      FLAGSL,ah
           FX

LDAR:      RF      5
           mov     PVN,0
           mov     al,R
           mov     ah,R7
           and     ax,807FH
           or      A,ah
           lahf
           and     ah,0EAH
           or      ah,IFF2
           and     FLAGSL,1
           or      FLAGSL,ah
           FX

; иммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE 16-BITNOG PRENOSA ╨
; хммммммммммммммммммммммммммммммм╪

;
; Ove instrukcije ne uticu ni na jedan fleg.
;

; здддддддддддддддддддддддддддд©
; Ё LD  regpar,nn              Ё
; Ё regpar = BC,DE,HL,SP,IX,IY Ё
; юдддддддддддддддддддддддддддды

LDBCNN:    RF      10
           mov     BC,[PC]
           add     PC,2
           FX

LDDENN:    RF      10
           mov     DE,[PC]
           add     PC,2
           FX

LDHLNN:    RF      10
           mov     HL,[PC]
           add     PC,2
           FX

LDSPNN:    RF      10
           mov     STP,[PC]
           add     PC,2
           FX

LDIXNN:    RF      10
           mov     AUX,[PC]
           mov     IX,AUX
           add     PC,2
           F

LDIYNN:    RF      10
           mov     AUX,[PC]
           mov     IY,AUX
           add     PC,2
           F

; здддддддддддддддддддддддддддд©
; Ё LD  regpar,(nn)            Ё
; Ё regpar = BC,DE,HL,SP,IX,IY Ё
; юдддддддддддддддддддддддддддды

LDBCMNN:   RF      16
           mov     AUX,[PC]
           mov     BC,[AUX]
           add     PC,2
           F

LDDEMNN:   RF      16
           mov     AUX,[PC]
           mov     DE,[AUX]
           add     PC,2
           F

LDHLMNN:   RF      16
           mov     HL,[PC]
           mov     HL,[HL]
           add     PC,2
           FX

LDSPMNN:   RF      16
           mov     STP,[PC]
           mov     STP,ds:[STP]
           add     PC,2
           FX

LDIXMNN:   RF      16
           mov     AUX,[PC]
           mov     AUX,[AUX]
           mov     IX,AUX
           add     PC,2
           F

LDIYMNN:   RF      16
           mov     AUX,[PC]
           mov     AUX,[AUX]
           mov     IY,AUX
           add     PC,2
           F

;
; Makro za smjestanje 16-bitnih vrijednosti.
;

WRITE16    macro   ADR,LREG,HREG
           WRITE   ADR,LREG
           inc     ADR
           WRITE   ADR,HREG
           ifidn   <ADR>,<STP>
           dec     ADR
           endif
           endm

; здддддддддддддддддддддддддддд©
; Ё LD  (nn),regpar            Ё
; Ё regpar = BC,DE,HL,SP,IX,IY Ё
; юдддддддддддддддддддддддддддды

LDMNNBC:   RF      16
           mov     AUX,[PC]
           WRITE16 AUX,C,B
           add     PC,2
           F

LDMNNDE:   RF      16
           mov     AUX,[PC]
           WRITE16 AUX,E,D
           add     PC,2
           F

           irp     RP,<HL,STP,IX,IY>

LDMNN&RP:  RF      16
           mov     AUX,[PC]
           xchg    DE,RP
           WRITE16 AUX,E,D
           xchg    DE,RP
           add     PC,2
           F

           endm

; зддддддддддддддддддд©
; Ё LD  SP,regpar     Ё
; Ё regpar = HL,IX,IY Ё
; юддддддддддддддддддды

           irp     RP,<HL,IX,IY>

LDSP&RP:   RF      6
           mov     STP,RP
           F

           endm

; имммммммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE ZA MANIPULACIJU STEKOM ╨
; хмммммммммммммммммммммммммммммммммммм╪

;
; Ove instrukcije ne uticu ni na jedan fleg, osim POP AF koja obnavlja ranije
; sacuvani sarzaj flegova.
;

; здддддддддддддддддддддддддддд©
; Ё PUSH  regpar               Ё
; Ё regpar = AF,BC,DE,HL,IX,IY Ё
; юдддддддддддддддддддддддддддды

PUSHAF:    RF      11
           sub     STP,2
           test    PV,0FFH
           jnz     PUSHAF_L1
           mov     AUX_L,FLAGSL
           and     AUX_L,0D5H
           or      AUX_L,N
           if      VER_128             ; na 128 verziji gornji skok ne moze da
           jmp     short PUSHAF_L2     ; dohvati labelu PUSHAF_L1
           else
           WRITE16 STP,AUX_L,A
           F
           endif
PUSHAF_L1: mov     AUX,FLAGS
           and     AUX,09D1H  ; 09F1H
           or      AUX_L,N
           shr     AUX_H,1
           or      AUX_L,AUX_H
           mov     AUX_H,A             ; Parcijalna emulacija bita 3 i 5
           and     AUX,28D7H
           or      AUX_L,AUX_H
PUSHAF_L2: WRITE16 STP,AUX_L,A
           F

PUSHBC:    RF      11
           sub     STP,2
           WRITE16 STP,C,B
           F

PUSHDE:    RF      11
           sub     STP,2
           WRITE16 STP,E,D
           F

           irp     RP,<HL,IX,IY>

PUSH&RP:   RF      11
           sub     STP,2
           mov     AUX,RP
           WRITE16 STP,AUX_L,AUX_H
           F

           endm

; здддддддддддддддддддддддддддд©
; Ё POP  regpar                Ё
; Ё regpar = AF,BC,DE,HL,IX,IY Ё
; юдддддддддддддддддддддддддддды

POPAF:     RF      10
           mov     AUX,ds:[STP]
           add     STP,2
           mov     FLAGSL,AUX_L
           mov     A,AUX_H
           and     AUX,002AH
           mov     PVN,AUX
           F

           irp     RP,<BC,DE,HL>

POP&RP:    RF      10
           mov     RP,ds:[STP]
           add     STP,2
           F

           endm

           irp     IR,<IX,IY>

POP&IR:    RF      10
           mov     AUX,ds:[STP]
           mov     IR,AUX
           add     STP,2
           F

           endm

; иммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE RAZMJENE SADRZAJA ╨
; хммммммммммммммммммммммммммммммм╪

;
; Ove instrukcije ne uticu ni na jedan fleg, osim EX AF,AF' koja obnavlja ranije
; sacuvani sadrzaj flegova.
;

; зддддддддддд©
; Ё EX  DE,HL Ё
; юддддддддддды

EXDEHL:    RF      4
           xchg    DE,HL
           F

; здддддддддддд©
; Ё EX  AF,AF' Ё
; юдддддддддддды

EXAFAF:    RF      4
           pop     AUX
           xchg    ALTF,AUX
           push    AUX
           xchg    ALTA,A
           mov     AUX,PVN
           xchg    ALTPVN,AUX
           mov     PVN,AUX
           F

; зддддд©
; Ё EXX Ё
; юддддды

EXX:       RF      4
           xchg    ALTBC,BC
           xchg    ALTDE,DE
           xchg    ALTHL,HL
           F

; зддддддддддддддддддд©
; Ё EX  (SP),regpar   Ё
; Ё regpar = HL,IX,IY Ё
; юддддддддддддддддддды

EXMSPHL:   RF      19
           mov     AUX,HL
           mov     HL,ds:[STP]
           WRITE16 STP,AUX_L,AUX_H
           F

           irp     IR,<IX,IY>

EXMSP&IR:  RF      19
           push    HL
           mov     AUX,IR
           mov     HL,ds:[STP]
           mov     IR,HL
           WRITE16 STP,AUX_L,AUX_H
           pop     HL
           F

           endm

; иммммммммммммммммммммммммммммммммммммммммм╩
; ╨ 8-BITNE ARITMETICKO LOGICKE INSTRUKCIJE ╨
; хммммммммммммммммммммммммммммммммммммммммм╪

;
; Makroi za izvrsavanje aritmetickih operacija ADD, ADC, SUB, SBC i CP. Sve ove
; instrukcije postavljaju C, Z, S i H flag standardno, a P/V flag signalizira
; prekoracenje (V). ADD i ADC resetuju N flag, a SUB, SBC i CP ga setuju.
;

ARITH      macro   OPER,REG,NEGATIVE
           popf
           OPER    A,REG
           pushf
           ifnb    <NEGATIVE>
           mov     PVN,0FF02H          ;; P/V=V, N=1
           else
           mov     PVN,0FF00H          ;; P/V=V, N=0
           endif
           endm

; зддддддддддддддддддддддддддддддддд©
; Ё ADD  A,reg                      Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

ADDA&REG:  RF      4
           ARITH   add,REG
           F

           endm

           irp     REG,<H,L>

ADDA&REG:  RF      4
           mov     AUX,HL
           ARITH   add,AUX_&REG
           F

           endm

; здддддддддд©
; Ё ADD  A,n Ё
; юдддддддддды

ADDAN:     RF      7
           ARITH   add,[PC]
           inc     PC
           F

; зддддддддддддд©
; Ё ADD  A,(HL) Ё
; юддддддддддддды

ADDAMHL:   RF      7
           ARITH   add,[HL]
           F

; зддддддддддддддддддд©
; Ё ADD  A,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

           irp     IR,<IX,IY>

ADDAM&IR:  RF      15
           IND&IR
           ARITH   add,[AUX]
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё ADC  A,reg                      Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

ADCA&REG:  RF      4
           ARITH   adc,REG
           F

           endm

           irp     REG,<H,L>

ADCA&REG:  RF      4
           mov     AUX,HL
           ARITH   adc,AUX_&REG
           F

           endm

; здддддддддд©
; Ё ADC  A,n Ё
; юдддддддддды

ADCAN:     RF      7
           ARITH   adc,[PC]
           inc     PC
           F

; зддддддддддддд©
; Ё ADC  A,(HL) Ё
; юддддддддддддды

ADCAMHL:   RF      7
           ARITH   adc,[HL]
           F

; зддддддддддддддддддд©
; Ё ADC  A,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

           irp     IR,<IX,IY>

ADCAM&IR:  RF      15
           IND&IR
           ARITH   adc,[AUX]
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё SUB  reg                        Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

SUB&REG:   RF      4
           ARITH   sub,REG,N
           F

           endm

           irp     REG,<H,L>

SUB&REG:   RF      4
           mov     AUX,HL
           ARITH   sub,AUX_&REG,N
           F

           endm

; здддддддд©
; Ё SUB  n Ё
; юдддддддды

SUBN:      RF      7
           ARITH   sub,[PC],N
           inc     PC
           F

; зддддддддддд©
; Ё SUB  (HL) Ё
; юддддддддддды

SUBMHL:    RF      7
           ARITH   sub,[HL],N
           F

; зддддддддддддддддд©
; Ё SUB  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

           irp     IR,<IX,IY>

SUBM&IR:   RF      15
           IND&IR
           ARITH   sub,[AUX],N
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё SBC  A,reg                      Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

SBCA&REG:  RF      4
           ARITH   sbb,REG,N
           F

           endm

           irp     REG,<H,L>

SBCA&REG:  RF      4
           mov     AUX,HL
           ARITH   sbb,AUX_&REG,N
           F

           endm

; здддддддддд©
; Ё SBC  A,n Ё
; юдддддддддды

SBCAN:     RF      7
           ARITH   sbb,[PC],N
           inc     PC
           F

; зддддддддддддд©
; Ё SBC  A,(HL) Ё
; юддддддддддддды

SBCAMHL:   RF      7
           ARITH   sbb,[HL],N
           F

; зддддддддддддддддддд©
; Ё SBC  A,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

           irp     IR,<IX,IY>

SBCAM&IR:  RF      15
           IND&IR
           ARITH   sbb,[AUX],N
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё CP  reg                         Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

CP_&REG:   RF      4
           ARITH   cmp,REG,N
           F

           endm

           irp     REG,<H,L>

CP_&REG:   RF      4
           mov     AUX,HL
           ARITH   cmp,AUX_&REG,N
           F

           endm

; зддддддд©
; Ё CP  n Ё
; юддддддды

CP_N:      RF      7
           ARITH   cmp,[PC],N
           inc     PC
           F

; здддддддддд©
; Ё CP  (HL) Ё
; юдддддддддды

CP_MHL:    RF      7
           ARITH   cmp,[HL],N
           F

; здддддддддддддддд©
; Ё CP  (regpar+d) Ё
; Ё regpar = IX,IY Ё
; юдддддддддддддддды

           irp     IR,<IX,IY>

CP_M&IR:   RF      15
           IND&IR
           ARITH   cmp,[AUX],N
           F

           endm

;
; Makro za izvrsavanje logickih operacija AND, OR, XOR. Ove instrukcije djeluju
; na S i Z fleg standardno, C i N fleg se resetuju, a P/V fleg signalizira
; paritet (P). Instrukcija AND setuje, a OR i XOR resetuju H fleg.
;

LOGIC      macro   OPER,REG            ;; *** Kvari ah ! ***
           OPER    A,REG
           lahf
           ifdif   <OPER>,<and>
           and     ah,0EFH             ;; Ovo resetuje H fleg
           else
           or      ah,10H              ;; Direktno setovanje H flega
           endif
           mov     FLAGSL,ah
           mov     PVN,0
           endm

; зддддддддддддддддддддддддддддддддд©
; Ё AND  reg                        Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

AND&REG:   RF      4
           LOGIC   and,REG
           F

           endm

           irp     REG,<H,L>

AND&REG:   RF      4
           mov     AUX,HL
           LOGIC   and,AUX_&REG
           F

           endm

; здддддддд©
; Ё AND  n Ё
; юдддддддды

ANDN:      RF      7
           LOGIC   and,[PC]
           inc     PC
           F

; зддддддддддд©
; Ё AND  (HL) Ё
; юддддддддддды

ANDMHL:    RF      7
           LOGIC   and,[HL]
           F

; зддддддддддддддддд©
; Ё AND  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

           irp     IR,<IX,IY>

ANDM&IR:   RF      15
           IND&IR
           LOGIC   and,[AUX]
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё OR  reg                         Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

ORA:       RF      4
           add     A,0                 ; LOGIC or,A bi generisalo sporiji i duzi
           lahf                        ;  kod. Ovako se A ne mijenja, samo se
           mov     FLAGSL,ah           ;  azuriraju flegovi.
           mov     PVN,0
           F

           irp     REG,<B,C,D,E,XH,XL,YH,YL>

OR&REG:    RF      4
           LOGIC   or,REG
           F

           endm

           irp     REG,<H,L>

OR&REG:    RF      4
           mov     AUX,HL
           LOGIC   or,AUX_&REG
           F

           endm

; зддддддд©
; Ё OR  n Ё
; юддддддды

ORN:       RF      7
           LOGIC   or,[PC]
           inc     PC
           F

; здддддддддд©
; Ё OR  (HL) Ё
; юдддддддддды

ORMHL:     RF      7
           LOGIC   or,[HL]
           F

; здддддддддддддддд©
; Ё OR  (regpar+d) Ё
; Ё regpar = IX,IY Ё
; юдддддддддддддддды

           irp     IR,<IX,IY>

ORM&IR:    RF      15
           IND&IR
           LOGIC   or,[AUX]
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё XOR  reg                        Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

XORA:      RF      4
           sub     A,A                 ; LOGIC xor,A bi generisalo sporiji i
           lahf                        ;  duzi kod. Ovako, ovdje SUB A,A djeluje
           mov     FLAGSL,ah           ;  na H flag bolje nego XOR A,A koji ga
           mov     PVN,0               ;  ostavlja nedefinisanog.
           F

           irp     REG,<B,C,D,E,XH,XL,YH,YL>

XOR&REG:   RF      4
           LOGIC   xor,REG
           F

           endm

           irp     REG,<H,L>

XOR&REG:   RF      4
           mov     AUX,HL
           LOGIC   xor,AUX_&REG
           F

           endm

; здддддддд©
; Ё XOR  n Ё
; юдддддддды

XORN:      RF      7
           LOGIC   xor,[PC]
           inc     PC
           F

; зддддддддддд©
; Ё XOR  (HL) Ё
; юддддддддддды

XORMHL:    RF      7
           LOGIC   xor,[HL]
           F

; зддддддддддддддддд©
; Ё XOR  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

           irp     IR,<IX,IY>

XORM&IR:   RF      15
           IND&IR
           LOGIC   xor,[AUX]
           F

           endm

;
; Makro za instrukcije brojanja INC i DEC. Uticu na S, Z i H fleg standardno.
; P/V fleg signalizira prekoracenje (V), INC setuje, a DEC resetuje N fleg.
;

CNT        macro   OPER,REG
           popf
           OPER    REG
           pushf
           ifidn   <OPER>,<dec>
           mov     PVN,0FF02H
           else
           mov     PVN,0FF00H
           endif
           endm

; зддддддддддддддддддддддддддддддддд©
; Ё INC  reg                        Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

INC_&REG:  RF      4
           CNT     inc,REG
           FX

           endm

           irp     REG,<H,L>

INC_&REG:  RF      4
           xchg    AUX,HL
           CNT     inc,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё INC  (HL) Ё
; юддддддддддды

INC_MHL:   RF      11
           mov     ah,[HL]
           CNT     inc,ah
           WRITE   HL,ah
           FX

; зддддддддддддддддд©
; Ё INC  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

           irp     IR,<IX,IY>

INC_M&IR:  RF      19
           IND&IR
           mov     ah,[AUX]
           CNT     inc,ah
           WRITE   AUX,ah
           F

           endm

; зддддддддддддддддддддддддддддддддд©
; Ё DEC  reg                        Ё
; Ё reg = A,B,C,D,E,H,L,XH,XL,YH,YL Ё
; юддддддддддддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E,XH,XL,YH,YL>

DEC&REG:   RF      4
           CNT     dec,REG
           FX

           endm

           irp     REG,<H,L>

DEC&REG:   RF      4
           xchg    AUX,HL
           CNT     dec,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё DEC  (HL) Ё
; юддддддддддды

DECMHL:    RF      11
           mov     ah,[HL]
           CNT     dec,ah
           WRITE   HL,ah
           FX

; зддддддддддддддддд©
; Ё DEC  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

           irp     IR,<IX,IY>

DECM&IR:   RF      19
           IND&IR
           mov     ah,[AUX]
           CNT     dec,ah
           WRITE   AUX,ah
           F

           endm

; имммммммммммммммммммммммммммммммммм╩
; ╨ 16-BITNE ARITMETICKE INSTRUKCIJE ╨
; хмммммммммммммммммммммммммммммммммм╪

;
; Makro za izvrsavanje 16-bitne ADD instrukcije. Smisleno utice samo na C flag,
; a N flag se resetuje. H flag je nedefinisan.
;

ADD_16     macro   SRC,DEST            ;; *** Kvari AUX ! ***
           pop     AUX
           add     SRC,DEST
           rcr     AUX_L,1
           rol     AUX_L,1
           push    aux
           mov     N,0
           endm

; здддддддддддддддддддддддддддддддддд©
; Ё ADD  regpar1,regpar2             Ё
; Ё regpar1 = HL,IX,IY               Ё
; Ё regpar2 = BC,DE,HL,SP,IX,IY      Ё
; Ё regpar1 = IX => regpar2 <> HL,IY Ё
; Ё regpar1 = IY => regpar2 <> HL,IX Ё
; юдддддддддддддддддддддддддддддддддды

           irp     RP,<BC,DE,HL,STP>

ADDHL&RP:  RF      11
           ADD_16  HL,RP
           F

           endm

           irp     RP,<BC,DE,STP>

ADDIX&RP:  RF      11
           ADD_16  IX,RP
           F

           endm

ADDIXIX:   RF      11
           pop     AUX                 ; ADD_16 IX,IX nije izvodivo jer je IX
           shl     IX,1                ;  lociran u memoriji a ne u registru
           rcr     AUX_L,1
           rol     AUX_L,1
           push    AUX
           mov     N,0
           F

           irp     RP,<BC,DE,STP>

ADDIY&RP:  RF      11
           ADD_16  IY,RP
           F

           endm

ADDIYIY:   RF      11
           pop     AUX                 ; Primjedba kao za ADDIXIX
           shl     IY,1
           rcr     AUX_L,1
           rol     AUX_L,1
           push    AUX
           mov     N,0
           F

;
; 16-bitne ADC i SBC instrukcije uticu na C, Z i S flegove normalno, a P/V fleg
; signalizira prekoracenje (V). ADC resetuje, a SBC setuje N fleg. H fleg je
; nedefinisan.
;

; здддддддддддддддддддддд©
; Ё ADC  HL,regpar       Ё
; Ё regpar = BC,DE,HL,SP Ё
; юдддддддддддддддддддддды

           irp     RP,<BC,DE,HL,STP>

ADCHL&RP:  RF      11
           popf
           adc     HL,RP
           pushf
           mov     PVN,0FF00H
           FX

           endm

; здддддддддддддддддддддд©
; Ё SBC  HL,regpar       Ё
; Ё regpar = BC,DE,HL,SP Ё
; юдддддддддддддддддддддды

           irp     RP,<BC,DE,HL,STP>

SBCHL&RP:  RF      11
           popf
           sbb     HL,RP
           pushf
           mov     PVN,0FF02H
           FX

           endm

;
; 16-bitne INC i DEC instrukcije ne uticu ni na jedan fleg.
;

; здддддддддддддддддддддддддддд©
; Ё INC  regpar                Ё
; Ё regpar = BC,DE,HL,SP,IX,IY Ё
; юдддддддддддддддддддддддддддды

           irp     RP,<BC,DE,HL,STP,IX,IY>

INC&RP:    RF      6
           inc     RP
           FX

           endm

; здддддддддддддддддддддддддддд©
; Ё DEC  regpar                Ё
; Ё regpar = BC,DE,HL,SP,IX,IY Ё
; юдддддддддддддддддддддддддддды

           irp     RP,<BC,DE,HL,STP,IX,IY>

DEC&RP:    RF      6
           dec     RP
           FX

           endm

; имммммммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE ROTIRANJA I POMJERANJA ╨
; хмммммммммммммммммммммммммммммммммммм╪

;
; Makro za rotiranje akumulatora. Jednobajtne rotacije akumulatora uticu logicno
; samo na C fleg, a usput resetuju H i N fleg.
;

AROT       macro   OPER                ;; *** Kvari AUX ! ***
           RF      4
           pop     AUX                 ;; Moze li bolje ?
           ror     AUX_L,1
           OPER    A,1
           rcl     AUX_L,1
           and     AUX_L,0EFH
           push    AUX
           mov     N,0
           F
           endm

; здддддд©
; Ё RLCA Ё
; юдддддды

RLCA:      AROT    rol

; здддддд©
; Ё RRCA Ё
; юдддддды

RRCA:      AROT    ror

; зддддд©
; Ё RLA Ё
; юддддды

RLA:       AROT    rcl

; зддддд©
; Ё RRA Ё
; юддддды

RRA:       AROT    rcr

;
; Makro za instrukcije rotiranja RLC, RRC i sinteticku instrukciju SLL.
; Instrukcije normalno uticu na S, Z i C fleg. P/V fleg indicira paritet (P).
; Flegovi N i H se resetuju. SLL <arg> je ekvivalentno sa SCF i RL <arg>.
;

ROT        macro   OPER,REG,SLL        ;; *** Kvari ah ! ***
           ifnb    <SLL>
           stc                         ;; Ako je SLL instrukcija, treba setovati
           endif                       ;;  C flag prije operacije.
           OPER    REG,1
           lahf                        ;; Cuva C flag.
           add     REG,0               ;; Postavlja S,Z i P flag, C=0, H=0.
           ror     ah,1                ;; Vraca C flag.
           lahf
           mov     FLAGSL,ah
           mov     PVN,0
           endm

; зддддддддддддддддддддд©
; Ё RLC  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RLC_&REG:  RF      4
           ROT     rol,REG
           FX

           endm

           irp     REG,<H,L>

RLC_&REG:  RF      4
           xchg    AUX,HL
           ROT     rol,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё RLC  (HL) Ё
; юддддддддддды

RLCMHL:    RF      11
           mov     AUX_L,[HL]
           ROT     rol,AUX_L
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё RLC  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

RLCI:      RC      15                  ; Moze li bolje ?
           mov     SAVEBC,BC
           mov     B,[AUX]
           ROT     rol,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

; зддддддддддддддддддддд©
; Ё RRC  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RRC_&REG:  RF      4
           ROT     ror,REG
           FX

           endm

           irp     REG,<H,L>

RRC_&REG:  RF      4
           xchg    AUX,HL
           ROT     ror,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё RRC  (HL) Ё
; юддддддддддды

RRCMHL:    RF      11
           mov     AUX_L,[HL]
           ROT     ror,AUX_L
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё RRC  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

RRCI:      RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           ROT     ror,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

;
; Makro za instrukcije rotiranja RL i RR. Instrukcije normalno uticu na S, Z i C
; fleg. P/V fleg indicira paritet (P). Flegovi N i H se resetuju.
;

ROT_C      macro   OPER,REG            ;; *** Kvari ah ! ***
           popf
           OPER    REG,1
           lahf                        ;; Cuva C flag.
           add     REG,0               ;; Postavlja S,Z i P flag, C=0, H=0.
           ror     ah,1                ;; Vraca C flag.
           pushf
           mov     PVN,0
           endm

; зддддддддддддддддддддд©
; Ё RL  reg             Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RL_&REG:   RF      4
           ROT_C   rcl,REG
           FX

           endm

           irp     REG,<H,L>

RL_&REG:   RF      4
           xchg    AUX,HL
           ROT_C   rcl,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; здддддддддд©
; Ё RL  (HL) Ё
; юдддддддддды

RLMHL:     RF      11
           mov     AUX_L,[HL]
           ROT_C   rcl,AUX_L
           WRITE   HL,AUX_L
           FX

; здддддддддддддддд©
; Ё RL  (regpar+d) Ё
; Ё regpar = IX,IY Ё
; юдддддддддддддддды

RLI:       RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           ROT_C   rcl,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

; зддддддддддддддддддддд©
; Ё RR  reg             Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RR_&REG:   RF      4
           ROT_C   rcr,REG
           FX

           endm

           irp     REG,<H,L>

RR_&REG:   RF      4
           xchg    AUX,HL
           ROT_C   rcr,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; здддддддддд©
; Ё RR  (HL) Ё
; юдддддддддды

RRMHL:     RF      11
           mov     AUX_L,[HL]
           ROT_C   rcr,AUX_L
           WRITE   HL,AUX_L
           FX

; здддддддддддддддд©
; Ё RR  (regpar+d) Ё
; Ё regpar = IX,IY Ё
; юдддддддддддддддды

RRI:       RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           ROT_C   rcr,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

;
; Makro za instrukcije shiftovanja SLA, SRA i SRL. Dejstvo na flegove je isto
; kao i za ROTATE grupu instrukcija (RLC, RRC, RL i RR).
;

SHIFT      macro   OPER,REG            ;; *** Kvari ah ! ***
           OPER    REG,1
           lahf
           and     ah,0EFH
           mov     FLAGSL,ah
           mov     PVN,0
           endm

; зддддддддддддддддддддд©
; Ё SLA  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SLA&REG:   RF      4
           SHIFT   shl,REG
           FX

           endm

           irp     REG,<H,L>

SLA&REG:   RF      4
           xchg    AUX,HL
           SHIFT   shl,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё SLA  (HL) Ё
; юддддддддддды

SLAMHL:    RF      11
           mov     AUX_L,[HL]
           SHIFT   shl,AUX_L
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё SLA  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

SLAI:      RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           SHIFT   shl,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

; зддддддддддддддддддддд©
; Ё SRA  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SRA&REG:   RF      4
           SHIFT   sar,REG
           FX

           endm

           irp     REG,<H,L>

SRA&REG:   RF      4
           xchg    AUX,HL
           SHIFT   sar,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё SRA  (HL) Ё
; юддддддддддды

SRAMHL:    RF      11
           mov     AUX_L,[HL]
           SHIFT   sar,AUX_L
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё SRA  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

SRAI:      RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           SHIFT   sar,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

; зддддддддддддддддддддд©
; Ё SRL  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SRL&REG:   RF      4
           SHIFT   shr,REG
           FX

           endm

           irp     REG,<H,L>

SRL&REG:   RF      4
           xchg    AUX,HL
           SHIFT   shr,AUX_&REG
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё SRL  (HL) Ё
; юддддддддддды

SRLMHL:    RF      11
           mov     AUX_L,[HL]
           SHIFT   shr,AUX_L
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё SRL  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

SRLI:      RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           SHIFT   shr,B
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

;
; Sinteticka instrukcija SLL <arg> ekvivalentna je sekvenci SCF i RL arg.
;

; зддддддддддддддддддддд©
; Ё SLL  reg            Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SLL&REG:   RF      4
           ROT     rcl,REG,SLL
           FX

           endm

           irp     REG,<H,L>

SLL&REG:   RF      4
           xchg    AUX,HL
           ROT     rcl,AUX_&REG,SLL
           xchg    AUX,HL
           FX

           endm

; зддддддддддд©
; Ё SLL  (HL) Ё
; юддддддддддды

SLLMHL:    RF      11
           mov     AUX_L,[HL]
           ROT     rcl,AUX_L,SLL
           WRITE   HL,AUX_L
           FX

; зддддддддддддддддд©
; Ё SLL  (regpar+d) Ё
; Ё regpar = IX,IY  Ё
; юддддддддддддддддды

SLLI:      RC      15
           mov     SAVEBC,BC
           mov     B,[AUX]
           ROT     rcl,B,SLL
           WRITE   AUX,B
           mov     BC,SAVEBC
           F

;
; Instrukcije RLD i RRD normalno uticu na S i Z fleg, P/V fleg signalizira
; paritet (P), a H i N flegovi se resetuju.
;

; зддддд©
; Ё RLD Ё
; юддддды

RLD:       RF      14
           mov     ah,[HL]
           rol     A,4
           rol     ax,4
           WRITE   HL,ah
           popf
           lahf
           add     A,0
           ror     ah,1
           pushf
           mov     PVN,0
           FX

; зддддд©
; Ё RRD Ё
; юддддды

RRD:       RF      14
           mov     ah,[HL]
           ror     ax,4
           ror     A,4
           WRITE   HL,ah
           popf
           lahf
           add     A,0
           ror     ah,1
           pushf
           mov     FLAGSL,ah
           mov     PVN,0
           FX

; имммммммммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE ZA MANIPULACIJU BITOVIMA ╨
; хмммммммммммммммммммммммммммммммммммммм╪

;
; Makroi za testiranje bita. Azurira se Z flag. H flag se setuje, N resetuje, a
; S i P/V flagovi su nedefinisani (u praksi, P/V postaje jednak Z flegu, a S
; fleg je uvijek 0, osim ako se testira sedmi bit koji je jednak jedinici).
;

TST_B      macro   REG,BIT             ;; *** Kvari ah ! ***
           test    REG,1 shl BIT
           lahf
           or      ah,10H
           and     FLAGSL,03BH
           or      FLAGSL,ah
           mov     N,0
           endm

; зддддддддддддддддддддд©
; Ё BIT  b,reg          Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

BIT0&REG:  RF      4
           TST_B   REG,0
           FX

           endm

BIT0H:     RF      4
           TST_B   HL,8
           FX

BIT0L:     RF      4
           TST_B   HL,0
           FX

           irp     REG,<A,B,C,D,E>

BIT1&REG:  RF      4
           TST_B   REG,1
           FX

           endm

BIT1H:     RF      4
           TST_B   HL,9
           FX

BIT1L:     RF      4
           TST_B   HL,1
           FX

           irp     REG,<A,B,C,D,E>

BIT2&REG:  RF      4
           TST_B   REG,2
           FX

           endm

BIT2H:     RF      4
           TST_B   HL,10
           FX

BIT2L:     RF      4
           TST_B   HL,2
           FX

           irp     REG,<A,B,C,D,E>

BIT3&REG:  RF      4
           TST_B   REG,3
           FX

           endm

BIT3H:     RF      4
           TST_B   HL,11
           FX

BIT3L:     RF      4
           TST_B   HL,3
           FX

           irp     REG,<A,B,C,D,E>

BIT4&REG:  RF      4
           TST_B   REG,4
           FX

           endm

BIT4H:     RF      4
           TST_B   HL,12
           FX

BIT4L:     RF      4
           TST_B   HL,4
           FX

           irp     REG,<A,B,C,D,E>

BIT5&REG:  RF      4
           TST_B   REG,5
           FX

           endm

BIT5H:     RF      4
           TST_B   HL,13
           FX

BIT5L:     RF      4
           TST_B   HL,5
           FX

           irp     REG,<A,B,C,D,E>

BIT6&REG:  RF      4
           TST_B   REG,6
           FX

           endm

BIT6H:     RF      4
           TST_B   HL,14
           FX

BIT6L:     RF      4
           TST_B   HL,6
           FX

           irp     REG,<A,B,C,D,E>

BIT7&REG:  RF      4
           TST_B   REG,7
           FX

           endm

BIT7H:     RF      4
           TST_B   HL,15
           FX

BIT7L:     RF      4
           TST_B   HL,7
           FX

; зддддддддддддд©
; Ё BIT  b,(HL) Ё
; юддддддддддддды

BIT0MHL:   RF      8
           TST_B   <byte ptr [HL]>,0
           FX

BIT1MHL:   RF      8
           TST_B   <byte ptr [HL]>,1
           FX

BIT2MHL:   RF      8
           TST_B   <byte ptr [HL]>,2
           FX

BIT3MHL:   RF      8
           TST_B   <byte ptr [HL]>,3
           FX

BIT4MHL:   RF      8
           TST_B   <byte ptr [HL]>,4
           FX

BIT5MHL:   RF      8
           TST_B   <byte ptr [HL]>,5
           FX

BIT6MHL:   RF      8
           TST_B   <byte ptr [HL]>,6
           FX

BIT7MHL:   RF      8
           TST_B   <byte ptr [HL]>,7
           FX

; зддддддддддддддддддд©
; Ё BIT  b,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

BIT0I:     RC      12
           TST_B   <byte ptr [AUX]>,0
           F

BIT1I:     RC      12
           TST_B   <byte ptr [AUX]>,1
           F

BIT2I:     RC      12
           TST_B   <byte ptr [AUX]>,2
           F

BIT3I:     RC      12
           TST_B   <byte ptr [AUX]>,3
           F

BIT4I:     RC      12
           TST_B   <byte ptr [AUX]>,4
           F

BIT5I:     RC      12
           TST_B   <byte ptr [AUX]>,5
           F

BIT6I:     RC      12
           TST_B   <byte ptr [AUX]>,6
           F

BIT7I:     RC      12
           TST_B   <byte ptr [AUX]>,7
           F

;
; Instrukcije RES i SET ne uticu na flegove.
;

; зддддддддддддддддддддд©
; Ё RES  b,reg          Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RES0&REG:  RF      4
           and     REG,0FEH
           F

           endm

RES0H:     RF      4
           and     HL,0FEFFH
           F

RES0L:     RF      4
           and     HL,0FFFEH
           F

           irp     REG,<A,B,C,D,E>

RES1&REG:  RF      4
           and     REG,0FDH
           F

           endm

RES1H:     RF      4
           and     HL,0FDFFH
           F

RES1L:     RF      4
           and     HL,0FFFDH
           F

           irp     REG,<A,B,C,D,E>

RES2&REG:  RF      4
           and     REG,0FBH
           F

           endm

RES2H:     RF      4
           and     HL,0FBFFH
           F

RES2L:     RF      4
           and     HL,0FFFBH
           F

           irp     REG,<A,B,C,D,E>

RES3&REG:  RF      4
           and     REG,0F7H
           F

           endm

RES3H:     RF      4
           and     HL,0F7FFH
           F

RES3L:     RF      4
           and     HL,0FFF7H
           F

           irp     REG,<A,B,C,D,E>

RES4&REG:  RF      4
           and     REG,0EFH
           F

           endm

RES4H:     RF      4
           and     HL,0EFFFH
           F

RES4L:     RF      4
           and     HL,0FFEFH
           F

           irp     REG,<A,B,C,D,E>

RES5&REG:  RF      4
           and     REG,0DFH
           F

           endm

RES5H:     RF      4
           and     HL,0DFFFH
           F

RES5L:     RF      4
           and     HL,0FFDFH
           F

           irp     REG,<A,B,C,D,E>

RES6&REG:  RF      4
           and     REG,0BFH
           F

           endm

RES6H:     RF      4
           and     HL,0BFFFH
           F

RES6L:     RF      4
           and     HL,0FFBFH
           F

           irp     REG,<A,B,C,D,E>

RES7&REG:  RF      4
           and     REG,7FH
           F

           endm

RES7H:     RF      4
           and     HL,07FFFH
           F

RES7L:     RF      4
           and     HL,0FF7FH
           F

; зддддддддддддд©
; Ё RES  b,(HL) Ё
; юддддддддддддды

RES0MHL:   RF      11
           RESMEM  HL,0
           F

RES1MHL:   RF      11
           RESMEM  HL,1
           F

RES2MHL:   RF      11
           RESMEM  HL,2
           F

RES3MHL:   RF      11
           RESMEM  HL,3
           F

RES4MHL:   RF      11
           RESMEM  HL,4
           F

RES5MHL:   RF      11
           RESMEM  HL,5
           F

RES6MHL:   RF      11
           RESMEM  HL,6
           F

RES7MHL:   RF      11
           RESMEM  HL,7
           F

; зддддддддддддддддддд©
; Ё RES  b,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

RES0I:     RC      15
           RESMEM  AUX,0
           F

RES1I:     RC      15
           RESMEM  AUX,1
           F

RES2I:     RC      15
           RESMEM  AUX,2
           F

RES3I:     RC      15
           RESMEM  AUX,3
           F

RES4I:     RC      15
           RESMEM  AUX,4
           F

RES5I:     RC      15
           RESMEM  AUX,5
           F

RES6I:     RC      15
           RESMEM  AUX,6
           F

RES7I:     RC      15
           RESMEM  AUX,7
           F

; зддддддддддддддддддддд©
; Ё SET  b,reg          Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SET0&REG:  RF      4
           or      REG,1
           F

           endm

SET0H:     RF      4
           or      HL,256
           F

SET0L:     RF      4
           or      HL,1
           F

           irp     REG,<A,B,C,D,E>

SET1&REG:  RF      4
           or      REG,2
           F

           endm

SET1H:     RF      4
           or      HL,512
           F

SET1L:     RF      4
           or      HL,2
           F

           irp     REG,<A,B,C,D,E>

SET2&REG:  RF      4
           or      REG,4
           F

           endm

SET2H:     RF      4
           or      HL,1024
           F

SET2L:     RF      4
           or      HL,4
           F

           irp     REG,<A,B,C,D,E>

SET3&REG:  RF      4
           or      REG,8
           F

           endm

SET3H:     RF      4
           or      HL,2048
           F

SET3L:     RF      4
           or      HL,8
           F

           irp     REG,<A,B,C,D,E>

SET4&REG:  RF      4
           or      REG,16
           F

           endm

SET4H:     RF      4
           or      HL,4096
           F

SET4L:     RF      4
           or      HL,16
           F

           irp     REG,<A,B,C,D,E>

SET5&REG:  RF      4
           or      REG,32
           F

           endm

SET5H:     RF      4
           or      HL,8192
           F

SET5L:     RF      4
           or      HL,32
           F

           irp     REG,<A,B,C,D,E>

SET6&REG:  RF      4
           or      REG,64
           F

           endm

SET6H:     RF      4
           or      HL,16384
           F

SET6L:     RF      4
           or      HL,64
           F

           irp     REG,<A,B,C,D,E>

SET7&REG:  RF      4
           or      REG,128
           F

           endm

SET7H:     RF      4
           or      HL,32768
           F

SET7L:     RF      4
           or      HL,128
           F

; зддддддддддддд©
; Ё SET  b,(HL) Ё
; юддддддддддддды

SET0MHL:   RF      11
           SETMEM  HL,0
           F

SET1MHL:   RF      11
           SETMEM  HL,1
           F

SET2MHL:   RF      11
           SETMEM  HL,2
           F

SET3MHL:   RF      11
           SETMEM  HL,3
           F

SET4MHL:   RF      11
           SETMEM  HL,4
           F

SET5MHL:   RF      11
           SETMEM  HL,5
           F

SET6MHL:   RF      11
           SETMEM  HL,6
           F

SET7MHL:   RF      11
           SETMEM  HL,7
           F

; зддддддддддддддддддд©
; Ё SET  b,(regpar+d) Ё
; Ё regpar = IX,IY    Ё
; юддддддддддддддддддды

SET0I:     RC      15
           SETMEM  AUX,0
           F

SET1I:     RC      15
           SETMEM  AUX,1
           F

SET2I:     RC      15
           SETMEM  AUX,2
           F

SET3I:     RC      15
           SETMEM  AUX,3
           F

SET4I:     RC      15
           SETMEM  AUX,4
           F

SET5I:     RC      15
           SETMEM  AUX,5
           F

SET6I:     RC      15
           SETMEM  AUX,6
           F

SET7I:     RC      15
           SETMEM  AUX,7
           F

; имммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммм╩
; ╨ SINTETICKE INSTRUKCIJE ROTIRANJA, POMJERANJA I MANIPULACIJE BITOVIMA ╨
; хмммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммммм╪

;
; Slijedi poveca grupa sintetickih instrukcija koje se od odgovarajucih legalnih
; instrukcija razlikuju po tome sto rezultat naznacene operacije ostavljaju u
; nekom od registara opste namjene. Tako su slijedece konstrukcije ekvivalentne:
;
;          RLCP    (IX-3),D               =>      RLC     (IX-3)
;                                                 LD      D,(IX-3)
;
;          SETP    2,(IX+10),A            =>      SET     2,(IX+10)
;                                                 LD      A,(IX+10)
;
; Mnemonici su plod maste autora, pri cemu sufiks P asocira na "put into reg".
;

; здддддддддддддддддддддд©
; Ё RLCP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RLCPI&REG: RC      15
           mov     REG,[AUX]
           ROT     rol,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RLCPI&REG: RC      15
           xchg    DE,HL
           ROT     rol,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддд©
; Ё RRCP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RRCPI&REG: RC      15
           mov     REG,[AUX]
           ROT     ror,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RRCPI&REG: RC      15
           xchg    DE,HL
           ROT     ror,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; зддддддддддддддддддддд©
; Ё RLP  (regpar+d),reg Ё
; Ё regpar = IX,IY      Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RLPI&REG:  RC      15
           mov     REG,[AUX]
           ROT_C   rcl,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RLPI&REG:  RC      15
           xchg    DE,HL
           ROT_C   rcl,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; зддддддддддддддддддддд©
; Ё RRP  (regpar+d),reg Ё
; Ё regpar = IX,IY      Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RRPI&REG:  RC      15
           mov     REG,[AUX]
           ROT_C   rcr,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RRPI&REG:  RC      15
           xchg    DE,HL
           ROT_C   rcr,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддд©
; Ё SLAP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SLAPI&REG: RC      15
           mov     REG,[AUX]
           SHIFT   shl,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SLAPI&REG: RC      15
           xchg    DE,HL
           SHIFT   shl,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддд©
; Ё SRAP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SRAPI&REG: RC      15
           mov     REG,[AUX]
           SHIFT   sar,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SRAPI&REG: RC      15
           xchg    DE,HL
           SHIFT   sar,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддд©
; Ё SRLP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SRLPI&REG: RC      15
           mov     REG,[AUX]
           SHIFT   shr,REG
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SRLPI&REG: RC      15
           xchg    DE,HL
           SHIFT   shr,DE_&REG
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддд©
; Ё SLLP  (regpar+d),reg Ё
; Ё regpar = IX,IY       Ё
; Ё reg = A,B,C,D,E,H,L  Ё
; юдддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SLLPI&REG: RC      15
           mov     REG,[AUX]
           ROT     rcl,REG,SLL
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SLLPI&REG: RC      15
           xchg    DE,HL
           ROT     rcl,DE_&REG,SLL
           WRITE   AUX,DE_&REG
           xchg    DE,HL
           F

           endm

; здддддддддддддддддддддддд©
; Ё RESP  b,(regpar+d),reg Ё
; Ё regpar = IX,IY         Ё
; Ё reg = A,B,C,D,E,H,L    Ё
; юдддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

RESP0&REG: RC      15
           mov     REG,[AUX]
           and     REG,0FEH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP0&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0FEH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP1&REG: RC      15
           mov     REG,[AUX]
           and     REG,0FDH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP1&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0FDH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP2&REG: RC      15
           mov     REG,[AUX]
           and     REG,0FBH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP2&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0FBH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP3&REG: RC      15
           mov     REG,[AUX]
           and     REG,0F7H
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP3&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0F7H
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP4&REG: RC      15
           mov     REG,[AUX]
           and     REG,0EFH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP4&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0EFH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP5&REG: RC      15
           mov     REG,[AUX]
           and     REG,0DFH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP5&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0DFH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP6&REG: RC      15
           mov     REG,[AUX]
           and     REG,0BFH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP6&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,0BFH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

RESP7&REG: RC      15
           mov     REG,[AUX]
           and     REG,07FH
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

RESP7&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           and     DE_&REG,07FH
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

; здддддддддддддддддддддддд©
; Ё SETP  b,(regpar+d),reg Ё
; Ё regpar = IX,IY         Ё
; Ё reg = A,B,C,D,E,H,L    Ё
; юдддддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

SETP0&REG: RC      15
           mov     REG,[AUX]
           or      REG,1
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP0&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,1
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP1&REG: RC      15
           mov     REG,[AUX]
           or      REG,2
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP1&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,2
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP2&REG: RC      15
           mov     REG,[AUX]
           or      REG,4
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP2&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,4
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP3&REG: RC      15
           mov     REG,[AUX]
           or      REG,8
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP3&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,8
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP4&REG: RC      15
           mov     REG,[AUX]
           or      REG,16
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP4&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,16
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP5&REG: RC      15
           mov     REG,[AUX]
           or      REG,32
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP5&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,32
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP6&REG: RC      15
           mov     REG,[AUX]
           or      REG,64
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP6&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,64
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

           irp     REG,<A,B,C,D,E>

SETP7&REG: RC      15
           mov     REG,[AUX]
           or      REG,128
           WRITE   AUX,REG
           F

           endm

           irp     REG,<H,L>

SETP7&REG: RC      15
           xchg    HL,DE
           mov     DE_&REG,[AUX]
           or      DE_&REG,128
           WRITE   AUX,DE_&REG
           xchg    HL,DE
           F

           endm

; иммммммммммммммммммм╩
; ╨ INSTRUKCIJE SKOKA ╨
; хммммммммммммммммммм╪

;
; Instrukcije skoka ne uticu na flegove.
;

; здддддддд©
; Ё JP  nn Ё
; юдддддддды

JP_:       call    SLOW
           add     RCLK,10*256+1
           jc      JP_
           mov     PC,[PC+1]
           F

; зддддддддддддддддддддддддддддд©
; Ё JP  uslov,nn                Ё
; Ё uslov = Z,NZ,C,NC,PO,PE,P,M Ё
; юддддддддддддддддддддддддддддды

JPZ:       RF      10                ; *PC
           test    FLAGSL,64
           jnz     JPZ_L1
           add     PC,2
           F
JPZ_L1:    mov     PC,[PC]
           F

JPNZ:      RF      10
           test    FLAGSL,64
           jz      JPZ_L1
           add     PC,2
           F

JPC:       RF      10
           test    FLAGSL,1
           jnz     JPZ_L1
           add     PC,2
           F

JPNC:      RF      10
           test    FLAGSL,1
           jz      JPZ_L1
           add     PC,2
           F

JPPO:      RF      10
           test    PV,0FFH
           jz      JPPO_L1
           test    FLAGSH,08H
           jz      JPPO_L2
           add     PC,2
           F
JPPO_L1:   popf
           pushf
           jnp     JPPO_L2
           add     PC,2
           F
JPPO_L2:   mov     PC,[PC]
           F

JPPE:      RF      10
           test    PV,0FFH
           jz      JPPE_L1
           test    FLAGSH,08H
           jnz     JPPO_L2
           add     PC,2
           F
JPPE_L1:   popf
           pushf
           jp      JPPO_L2
           add     PC,2
           F

JPP:       RF      10
           test    FLAGSL,128
           jz      JPPO_L2
           add     PC,2
           F

JPM:       RF      10
           test    FLAGSL,128
           jnz     JPM_L2
           add     PC,2
           F
JPM_L2:    mov     PC,[PC]
           F

;
; Makro za relativni skok.
;

REL_JUMPC  macro
           local   RLJSLOW,RLJCONT
           add     RCLK,256*5
           jc      RLJSLOW
RLJCONT:   mov     AUX_L,A
           mov     al,[PC-1]
           cbw
           add     PC,ax
           mov     A,AUX_L
           FX
RLJSLOW:   call    SLOW_MAIN
           jmp     RLJCONT
           endm

; зддддддддд©
; Ё JR  dis Ё
; юддддддддды


JR:        RF      12
           inc     PC
           mov     AUX_L,A
           mov     al,[PC-1]
           cbw
           add     PC,ax
           mov     A,AUX_L
           FX

; зддддддддддддддддддд©
; Ё JR  uslov,dis     Ё
; Ё uslov = Z,NZ,C,NC Ё
; юддддддддддддддддддды

JRZ:       RF      7
           inc     PC
           test    FLAGSL,40H
           jz      JRZ_L1
           REL_JUMPC
JRZ_L1:    FX

JRNZ:      RF      7
           inc     PC
           test    FLAGSL,40H
           jnz     JRC_L1
           REL_JUMPC

JRC:       RF      7
           inc     PC
           test    FLAGSL,01H
           jz      JRC_L1
           REL_JUMPC
JRC_L1:    FX

JRNC:      RF      7
           inc     PC
           test    FLAGSL,01H
           jnz     JRC_L1
           REL_JUMPC

; зддддддддддд©
; Ё DJNZ  dis Ё
; юддддддддддды

DJNZ:      RF      8
           inc     PC
           dec     B
           jz      JRC_L1
           REL_JUMPC

; зддддддддддддддддддд©
; Ё JP  (regpar)      Ё
; Ё regpar = HL,IX,IY Ё
; юддддддддддддддддддды

           irp     RP,<HL,IX,IY>

JP&RP:     call    SLOW
           add     RCLK,4*256+1
           jc      JP&RP
           mov     PC,RP
           F

           endm

; имммммммммммммммммммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE ZA POZIV I POVRATAK IZ POTPROGRAMA ╨
; хмммммммммммммммммммммммммммммммммммммммммммммммм╪

;
; Makro za poziv potprograma.
;

DO_CALL    macro
           sub     STP,2
           mov     AUX,PC
           WRITE16 STP,AUX_L,AUX_H
           mov     PC,[PC-2]
           endm

; здддддддддд©
; Ё CALL  nn Ё
; юдддддддддды

CALL_:     call    SLOW
           add     RCLK,17*256+1
           jc      CALL_
           add     PC,3
           DO_CALL
           F

;
; Makro za uslovni poziv potprograma.
;

DO_CC      macro
           local   CCSLOW,CCCONT
           add     RCLK,7*256
           jc      CCSLOW
CCCONT:    DO_CALL
           F
CCSLOW:    call    SLOW_MAIN
           jmp     CCCONT
           endm

; зддддддддддддддддддддддддддддд©
; Ё CALL  uslov,nn              Ё
; Ё uslov = Z,NZ,C,NC,PO,PE,P,M Ё
; юддддддддддддддддддддддддддддды

CALLZ:     call    SLOW
           add     RCLK,256*10+1
           jc      CALLZ
           add     PC,3
           test    FLAGSL,40H
           if      VER_128
           jnz     CALLZ_L2
           jmp     CALLZ_L1
           else
           jz      CALLZ_L1
           endif
CALLZ_L2:  DO_CC
CALLZ_L1:  F

CALLNZ:    call    SLOW
           add     RCLK,256*10+1
           jc      CALLNZ
           add     PC,3
           test    FLAGSL,40H
           jnz     CALLZ_L1
           DO_CC

CALLC:     call    SLOW
           add     RCLK,256*10+1
           jc      CALLC
           add     PC,3
           test    FLAGSL,01H
           if      VER_128
           jnz     CALLC_L2
           jmp     CALLC_L1
           else
           jz      CALLC_L1
           endif
CALLC_L2:  DO_CC
CALLC_L1:  F

CALLNC:    call    SLOW
           add     RCLK,256*10+1
           jc      CALLNC
           add     PC,3
           test    FLAGSL,01H
           jnz     CALLC_L1
           DO_CC

CALLPO:    call    SLOW
           add     RCLK,256*10+1
           jc      CALLPO
           add     PC,3
           test    PV,0FFH
           jz      CALLPO_L1
           test    FLAGSH,08H
           jnz     CALLPO_L2
           jmp     short CALLPO_L3
CALLPO_L1: popf
           pushf
           jnp     CALLPO_L3
           F
CALLPO_L2: F
CALLPO_L3: DO_CC

CALLPE:    call    SLOW
           add     RCLK,256*10+1
           jc      CALLPE
           add     PC,3
           test    PV,0FFH
           jz      CALLPE_L1
           test    FLAGSH,08H
           jz      CALLPE_L2
           jmp     short CALLPE_L3
CALLPE_L1: popf
           pushf
           jp      CALLPE_L3
           F
CALLPE_L2: F
CALLPE_L3: DO_CC

CALLP:     call    SLOW
           add     RCLK,256*10+1
           jc      CALLP
           add     PC,3
           test    FLAGSL,80H
           if      VER_128
           jz      CALLP_L2
           jmp     CALLP_L1
           else
           jnz     CALLP_L1
           endif
CALLP_L2:  DO_CC
CALLP_L1:  F

CALLM:     call    SLOW
           add     RCLK,256*10+1
           jc      CALLM
           add     PC,3
           test    FLAGSL,80H
           jz      CALLP_L1
           DO_CC

; здддддд©
; Ё RETN Ё
; юдддддды

RETN_:     call    SLOW
           add     RCLK,(14-4)*256+1
           jc      RETN_
           mov     PC,ds:[STP]
           mov     ah,IFF2             ; Iskopiraj IFF2 u IFF1
           mov     IFF1,ah
           add     STP,2
           FX

; здддддд©
; Ё RETI Ё
; юдддддды

           ; RETI u principu radi isto sto i RET

; зддддд©
; Ё RET Ё
; юддддды


RETI:
RET_:      call    SLOW
           add     RCLK,10*256+1
           jc      RET_
           mov     PC,ds:[STP]
           add     STP,2
           F

; зддддддддддддддддддддддддддддд©
; Ё RET  uslov                  Ё
; Ё uslov = Z,NZ,C,NC,PO,PE,P,M Ё
; юддддддддддддддддддддддддддддды

;
; Makro za uslovni povratak iz potprograma.
;

DOCRET     macro
           local   CRSLOW,CRCONT
           add     RCLK,6*256
           jc      CRSLOW
CRCONT:    mov     PC,ds:[STP]
           add     STP,2
           F
CRSLOW:    call    SLOW_MAIN
           jmp     CRCONT
           endm

RETZ:      call    SLOW
           add     RCLK,256*5+1
           jc      RETZ
           popf
           pushf
           jnz     RETZ_L1
           DOCRET
RETZ_L1:   inc     PC
           F

RETNZ:     call    SLOW
           add     RCLK,256*5+1
           jc      RETNZ
           popf
           pushf
           jz      RETC_L1
           DOCRET

RETC:      call    SLOW
           add     RCLK,256*5+1
           jc      RETC
           popf
           pushf
           jnc     RETC_L1
           DOCRET
RETC_L1:   inc     PC
           F

RETNC:     call    SLOW
           add     RCLK,256*5+1
           jc      RETNC
           popf
           pushf
           jc      RETC_L1
           DOCRET

RETPO:     RF      5
           test    PV,0FFH
           jz      RETPO_L1
           test    FLAGSH,08H
           jz      RETPO_L2
           F
RETPO_L1:  popf
           pushf
           jnp     RETPO_L2
           F
RETPO_L2:  DOCRET

RETPE:     RF      5
           test    PV,0FFH
           jz      RETPE_L1
           test    FLAGSH,08H
           jnz     RETPO_L2
           F
RETPE_L1:  popf
           pushf
           jp      RETPO_L2
           F

RETP:      call    SLOW
           add     RCLK,256*5+1
           jc      RETP
           popf
           pushf
           js      RETP_L1
           DOCRET
RETP_L1:   inc     PC
           F

RETM:      call    SLOW
           add     RCLK,256*5+1
           jc      RETM
           popf
           pushf
           jns     RETP_L1
           DOCRET

;
; Makro za ostvarivanje restarta.
;

DO_RST     macro   RESNUM
           sub     STP,2
           mov     AUX,PC
           WRITE16 STP,AUX_L,AUX_H
           mov     PC,RESNUM
           F
           endm

; зддддддддддддддддддддддддддддд©
; Ё RST rut                     Ё
; Ё rut = 0,8,16,24,32,40,48,56 Ё
; юддддддддддддддддддддддддддддды

RST0:      RF      11
           xor     PC,PC               ; Za ZXCOMP moze i ovako!
           F

RST8:      RF      11
           DO_RST  8

RST16:     RF      11
           DO_RST  16

RST24:     RF      11
           DO_RST  24

RST32:     RF      11
           DO_RST  32

RST40:     RF      11
           DO_RST  40

RST48:     RF      11
           DO_RST  48

RST56:     RF      11
           DO_RST  56

; имммммммммммммммммммммммммммм╩
; ╨ ULAZNO IZLAZNE INSTRUKCIJE ╨
; хмммммммммммммммммммммммммммм╪

;
; Instrukcija IN A,(n) ne utice na flegove.
;

; зддддддддддд©
; Ё IN  A,(n) Ё
; юддддддддддды
INAN:      RF      10
           push    BC
           mov     B,A
           mov     C,[PC]
           call    PORT_IN
           pop     BC
           mov     A,ah
           inc     PC
           F

;
; Makro za IN reg,(C) instrukciju. Ova instrukcija postavlja Z i S fleg, P/V
; fleg signalizira paritet (P), a N i H flegovi se resetuju.
;

INREG      macro   REG
           call    PORT_IN
           mov     REG,ah
           popf
           lahf
           add     REG,0
           ror     ah,1
           pushf
           mov     PVN,0
           endm

; зддддддддддддддддддддд©
; Ё IN  reg,(C)         Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

INAC:      RF      7
           INREG   A
           F

INBC:      RF      7
           INREG   B
           F

INCC:      RF      7
           INREG   C
           F

INDC:      RF      7
           INREG   D
           F

INEC:      RF      7
           INREG   E
           F

INHC:      RF      7
           xchg    AUX,HL
           INREG   AUX_H
           xchg    AUX,HL
           F

INLC:      RF      7
           xchg    AUX,HL
           INREG   AUX_L
           xchg    AUX,HL
           F

;
; Instrukcija IN F,(C) samo postavlja flegove bez kvarenja registara.
;

; зддддддддддд©
; Ё IN  F,(C) Ё
; юддддддддддды

INFC:      RF      7
           call    PORT_IN
           popf
           rcl     AUX_L,1
           add     ah,0
           ror     AUX_L,1
           pushf
           mov     PVN,0
           F

;
; OUT instrukcije ne uticu na flegove.
;

; здддддддддддд©
; Ё OUT  (n),A Ё
; юдддддддддддды

OUTNA:     RF      11
           push    BC
           mov     B,A
           mov     C,[PC]
           mov     ah,A
           call    PORT_OUT
           pop     BC
           inc     PC
           F

; зддддддддддддддддддддд©
; Ё OUT  (C),reg        Ё
; Ё reg = A,B,C,D,E,H,L Ё
; юддддддддддддддддддддды

           irp     REG,<A,B,C,D,E>

OUTC&REG:  RF      8
           mov     ah,REG
           call    PORT_OUT
           F

           endm

           irp     REG,<H,L>

OUTC&REG:  RF      8
           mov     AUX,HL
           mov     ah,AUX_&REG
           call    PORT_OUT
           F

           endm

;
; Sinteticka instrukcija OUT (C),0 jasna je iz mnemonike.
;

; здддддддддддд©
; Ё OUT  (C),0 Ё
; юдддддддддддды

OUTC0:     RF      8
           xor     ah,ah
           call    PORT_OUT
           F

; иммммммммммммммммммммммммммммммммммммммм╩
; ╨ INSTRUKCIJE ZA MANIPULACIJU BLOKOVIMA ╨
; хммммммммммммммммммммммммммммммммммммммм╪

;
; Instrukcije LDI, LDIR, LDD i LDDR resetuju N i H fleg. Kod LDI i LDD fleg P/V
; se resetuje ako je na zavrsetku operacije BC=0, inace se setuje. LDIR i LDDR
; bezuslovno resetuju P/V fleg. LDIR i LDDR se mogu sammomodifikovati!
;

; зддддд©
; Ё LDI Ё
; юддддды

LDI:       RF      12
           mov     AUX,DE
           mov     ah,[HL]
           mov     PVN,0
           WRITE   AUX,ah
           inc     HL
           inc     DE
           loop    LDI_L1
           and     FLAGSL,0EBH
           F
LDI_L1:    and     FLAGSL,0EFH
           or      FLAGSL,04H
           F

; зддддд©
; Ё LDD Ё
; юддддды

LDD:       RF      12
           mov     AUX,DE
           mov     ah,[HL]
           mov     PVN,0
           WRITE   AUX,ah
           dec     HL
           dec     DE
           loop    LDI_L1
           and     FLAGSL,0EBH
           F

; здддддд©
; Ё LDIR Ё
; юдддддды

LDIR:      RF      12
           mov     AUX,DE
           mov     ah,[HL]
           mov     PVN,0
           WRITE   AUX,ah
           inc     HL
           inc     DE
           loop    LDIR_L1
           and     FLAGSL,0EBH
           F
LDIR_SLW:  call    SLOW
LDIR_L1:   add     RCLK,5*256
           jc      LDIR_SLW
           and     FLAGSL,0EFH
           or      FLAGSL,04H
           sub     PC,2
           F

; здддддд©
; Ё LDDR Ё
; юдддддды

LDDR:      RF      12
           mov     AUX,DE
           mov     ah,[HL]
           mov     PVN,0
           WRITE   AUX,ah
           dec     HL
           dec     DE
           loop    LDIR_L1
           and     FLAGSL,0EBH
           F

;
; Instrukcije CPI, CPD, CPIR i CPDR setuju N fleg, a S, Z i H fleg se azuriraju
; u skladu sa operacijom. P/V fleg se resetuje ako je na kraju operacije BC=0,
; inace se setuje.
;

; зддддд©
; Ё CPI Ё
; юддддды

CPI:       RF      12
           popf
           lahf
           cmp     A,[HL]
           ror     ah,1
           pushf
           mov     PVN,0002H
           inc     HL
           loop    CPI_L1
           and     FLAGSL,0FBH
           F
CPI_L1:    or      FLAGSL,04H
           F

; зддддд©
; Ё CPD Ё
; юддддды

CPD:       RF      12
           popf
           lahf                        ;; Bio bag !
           rcl     ah,1
           cmp     A,[HL]
           ror     ah,1
           pushf
           mov     PVN,0002H
           dec     HL
           loop    CPI_L1
           and     FLAGSL,0FBH
           F

; здддддд©
; Ё CPIR Ё
; юдддддды

CPIR:      RF      12
           popf
           lahf
           inc     HL
           mov     PVN,0002H
           cmp     A,[HL-1]
           je      CPIR_L2
           ror     ah,1
           pushf
           loop    CPIR_L1
           and     FLAGSL,0FBH
           F
CPIR_SLW:  call    SLOW
CPIR_L1:   add     RCLK,5*256
           jc      CPIR_SLW
           or      FLAGSL,04H
           sub     PC,2
           F
CPIR_L2:   ror     ah,1
           pushf
           loop    CPIR_L3
           and     FLAGSL,0FBH
           F
CPIR_L3:   or      FLAGSL,04H
           F

; здддддд©
; Ё CPDR Ё
; юдддддды

CPDR:      RF      12
           popf
           lahf
           dec     HL
           mov     PVN,0002H
           cmp     A,[HL+1]
           je      CPIR_L2
           ror     ah,1
           pushf
           loop    CPIR_L1
           and     FLAGSL,0FBH
           F

;
; Instrukcije INI, INIR, IND, INDR, OUTI, OTIR, OUTD i OTDR setuju N fleg, a Z
; fleg se setuje ako je na kraju operacije BC=0, inace se resetuje (INIR, INDR,
; OTIR i OTDR ga bezuslovno resetuju). Flegovi S, H i P/V su nedefinisani (u
; praksi za INI, IND, OUTI i OUTD onakvi kao kad se izvrsi DEC B).

; зддддд©
; Ё INI Ё
; юддддды

INI:       RF      11
           call    PORT_IN
           WRITE   HL,ah
           mov     N,02H
           inc     HL
           popf
           dec     B
           pushf
           F

; зддддд©
; Ё IND Ё
; юддддды

IND:       RF      11
           call    PORT_IN
           WRITE   HL,ah
           mov     N,02H
           dec     HL
           popf
           dec     B
           pushf
           F

; здддддд©
; Ё INIR Ё
; юдддддды

INIR:      RF      11
           call    PORT_IN
           WRITE   HL,ah
           mov     N,02H
           inc     HL
           popf
           dec     B
           pushf
           jnz     INIR_L1
           F
INIR_SLW:  call    SLOW
INIR_L1:   add     RCLK,5*256
           jc      INIR_SLW
           sub     PC,2
           F

; здддддд©
; Ё INDR Ё
; юдддддды

INDR:      RF      11
           call    PORT_IN
           WRITE   HL,ah
           mov     N,02H
           inc     HL
           popf
           dec     B
           pushf
           jnz     INIR_L1
           F

; здддддд©
; Ё OUTI Ё
; юдддддды

OUTI:      RF      11
           mov     ah,[HL]
           popf
           dec     B
           pushf
           call    PORT_OUT
           mov     N,02H
           inc     HL
           F

; здддддд©
; Ё OUTD Ё
; юдддддды

OUTD:      RF      11
           mov     AUX,HL
           mov     ah,[AUX]
           popf
           dec     B
           pushf
           call    PORT_OUT
           mov     N,02H
           dec     HL
           F

; здддддд©
; Ё OTIR Ё
; юдддддды

OTIR:      RF      11
           mov     ah,[HL]
           inc     HL
           mov     N,02H
           popf
           dec     B
           pushf
           pushf
           call    PORT_OUT
           popf
           jnz     OTIR_L1
           F
OTIR_SLW:  call    SLOW
OTIR_L1:   add     RCLK,5*256
           jc      OTIR_SLW
           sub     PC,2
           F

; здддддд©
; Ё OTDR Ё
; юдддддды

OTDR:      RF      11
           mov     ah,[HL]
           dec     HL
           mov     N,02H
           popf
           dec     B
           pushf
           pushf
           call    PORT_OUT
           popf
           jnz     OTIR_L1
           F

; имммммммммммммммммммм╩
; ╨ OSTALE INSTRUKCIJE ╨
; хмммммммммммммммммммм╪

; зддддд©
; Ё DAA Ё
; юддддды

DAA_:      RF      4
           mov     PV,0                ; Utice na C, Z, S, H i P/V (P) flegove.
           test    N,2
           jz      DAA_1
           popf
           das
           pushf
           FX
DAA_1:     popf
           daa
           pushf
           FX

; зддддд©
; Ё CPL Ё
; юддддды

CPL:       RF      4
           not     A                   ; Setuju se flegovi N i H.
           or      FLAGSL,10H
           mov     N,02H
           FX

; зддддд©
; Ё NEG Ё
; юддддды

NEG_:      RF      4
           popf                        ; Flegovi kao kod SUB.
           neg     A
           pushf
           mov     PVN,0FF02H
           FX

; зддддд©
; Ё SCF Ё
; юддддды

SCF:       RF      4
           mov     N,0                 ; C fleg se setuje, a N i H resetuju.
           and     FLAGSL,0EFH
           or      FLAGSL,1
           FX

; зддддд©
; Ё CCF Ё
; юддддды

CCF:       RF      4
           mov     N,0                 ; C fleg se komplementira, a N resetuje.
           xor     FLAGSL,1            ; H fleg je nedefinisan.
           FX

; здддд©
; Ё DI Ё
; юдддды

DI_:       RF      4
           cmp     PC,1                ; Trap na adresi 0
           je      ENDEMUL
           mov     IFF,0
           F
ENDEMUL:   jmp     EXIT

; здддд©
; Ё EI Ё
; юдддды

EI:        RF      4
           mov     IFF,0404H
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           push    TABLE1[AUX]         ; Ovako SSTRAP nece prepoznati kraj
           ret                         ;  instrukcije, pa IRQ nece nastupiti

; здддддддддддддд©
; Ё IM  mode     Ё
; Ё mode = 0,1,2 Ё
; юдддддддддддддды

           irp     MODE,<0,1,2>

IM&MODE:   RF      4
           mov     IMODE,MODE
           FX

           endm

; здддддд©
; Ё HALT Ё
; юдддддды

HALT:      call    SLOW                ; Jedini izlaz odavde je pomocu timera
           add     RCLK,4*256+1
           jc      HALT
           mov     HSTATE,1
           FX

; зддддд©
; Ё NOP Ё
; юддддды

NOP_:      RF      4
GETINST:   F

;
; Prefiksi za druge grupe instrukcija.

PREFIXCB:  RF      4
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           jmp     TABLE2[AUX]

PREFIXDD:  RF      4
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           jmp     TABLE4[AUX]

PREFIXED:  RF      4
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           jmp     TABLE3[AUX]

PREFIXFD:  RF      4
           mov     AUX_L,[PC]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           jmp     TABLE5[AUX]

PREFDDCB:  RF      4
           mov     AUX_L,[PC+1]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           push    TABLE6[AUX]
           INDIX
           ret

PREFFDCB:  RF      4
           mov     AUX_L,[PC+1]
           xor     AUX_H,AUX_H
           add     AUX,AUX
           push    TABLE6[AUX]
           INDIY
           ret

;
; Prelazak u 8086 rezim. Povratak u Z80 rezim ostvaruje se sa INT 255.
;

GO8086:    RF      4
           cmp     word ptr [PC],'08'
           jne     NOSIG
           cmp     word ptr [PC+2],'68'
           jne     NOSIG
           call    RESTTIMER
           add     PC,4
           push    ds
           push    PC
           retf
NOSIG:     F

CSEG       ends

ZXDSEG     segment public
ZXDSEG     ends

ZXDSEG2    segment public
ZXDSEG2    ends

ZXI1ROM    segment public
ZXI1ROM    ends

           end
