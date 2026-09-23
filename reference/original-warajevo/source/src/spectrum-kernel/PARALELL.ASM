;
;	   WARAJEVO Spectrum Simulator	  Release 1.5
;		 (C) Zeljko Juric & Samir Ribic
;
;   Example external module to add emulation of Kempston S Centronics paralell
;               printer interface into WARAJEVO Spectrum simulator
;

;
; External driver must define handlers for UserOut (via int 249) and UserIn
; (via int 248). COM type of driver is recommended but this is not necessary.
;

CSeg       segment

           assume  cs:CSeg

           org     100h                ; COM must start at 100H

Start:     mov     ax,25F9h            ; Initialization of interrupt vectors
           lea     dx,UserOut
           int     21h
           mov     ax,25F8h
           lea     dx,UserIn
           int     21h
           lea     dx,CodeEnd          ; Terminate and stay resident
           int     27h

;
; Kempston S Centronics paralell printer interface is based on Intel 8255 PPI
; chip. PPI port A is Centronics data port (configured as output port). Bit b0
; in PPI port C is a busy pin, and bit b7 in PPI port C acts as a strobe bit.
; Other bits and whole PPI port B are not used. All ports use MODE 0 (see 8255
; description). Emulation of 8255 in this simple example will not be full
; emulation. Only things which are real need for Centronics emulation will be
; emulated (this is quite logical).
;

;
; Handler for user OUT instructions must be after a signature 'USER'.
;

           db      'USER'              ; Signature

;
; Start of user-OUT handler.
;

UserOut:   test    cl,64               ; Test for b6 in port address
           jz      OutOK               ; (tipically is cl=0BFH for Centronics)
           iret                        ; Exit if it is no Centronics port

;
; Whole port B will be ignored, strobe bit will be ignored (int 17H will do
; it), and whole control register (needs for initialization) will be ignored.
;

OutOK:     cmp     ch,0E0h             ; Is it 8255 port A?
           je      PortA
Done:      stc                         ; If no, do nothing, but signal by CF
           retf    2                   ;   that I/O address is recognized

PortA:     xor     dx,dx               ; Use printer 1
           mov     al,ah               ; Move byte into AL
           xor     ah,ah               ; Send byte command
           int     17h                 ; Call BIOS
           jmp     Done

;
; Handler for user IN instructions must be after a signature 'USER' too.
;

           db      'USER'              ; Signature

;
; Start of user-IN handler.
;

UserIn:    test    cl,64               ; Test for b6 in port address
           jz      InOK
           iret                        ; Exit if it is no Centronics port

;
; Only busy bit will be emulated.
;


InOK:      cmp     ch,0E2h             ; Is it 8255 port C?
           je      PortC
           mov     ah,255              ; If no, return 255 and signal by CF
           jmp     Done                ;   that I/O address is recognized

PortC:     xor     dx,dx               ; Use printer 1
           mov     ah,2                ; Read status command
           int     17h                 ; Call BIOS
           rol     ah,1                ; Move BUSY bit into b0
           mov     al,ah
           mov     cl,3                ; Move OFF-LINE bit into b0
           rol     al,cl
           and     ah,al               ; Make "BUSY" if printer is really BUSY,
           not     ah                  ;   or if it is in OFF-LINE state
           or      ah,254              ; Mask unused bits
           jmp     Done

CodeEnd    label   byte                ; End of code

CSeg       ends

           end     Start

