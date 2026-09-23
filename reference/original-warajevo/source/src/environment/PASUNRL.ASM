           .386p

           nosmart                     ; Bez ovoga TASM ne pravi ono sto pise!

CSeg       segment use16

           assume  cs:CSeg

           public  SetUnReal

EntryVec   dw      0,16                ; Ovo ide na labelu Protected

;
; Global descriptor table deskriptor
;

GDTR       dw      24                  ; Duzina 24 bajta
           dd      0                   ; Ovdje ce ici lin. adresa od GDT

;
; Global descriptor table
;

GDT        db      8 dup (0)           ; Null deskriptor

           dw      0ffffh              ; Deskriptor za "razvaljene" segmente:
           db      0,0,0               ;  Lin. adresa nebitna, duzina ffffffff,
           db      10010010b           ;  (GRAN=1), PRESENT, DPL=0, DATA,
           db      10001111b           ;  NOT STACK, WRITABLE
           db      0

           dw      0ffffh              ; CS deskriptor za ulaz u Prot mod:
           db      0,0,0               ;  Lin. adresa ce se izracunati!
           db      10011010b           ;  Duzina ffff, PRESENT, DPL=0, CODE,
           db      00000000b           ;  C=0, READABLE
           db      0

SetUnReal  proc    far
           push    ds
           pushf                       ; Test procesora
           pop     ax
           or      ah,0f0h
           push    ax
           popf
           pushf
           pop     ax
           and     ah,0f0h
           mov     ax,1                ; Error: Not 386!
           jz      short Exit
           smsw    ax                  ; Test za Protected mod
           test    ax,1                
           mov     ax,2                ; Error: Protected mode active!
           jnz     short Exit
           xor     eax,eax             ; Priprema linearnu adresu od GDT da se
           xor     ebx,ebx             ;  smjesti u GDT deskriptor
           mov     ax,cs
           shl     eax,4
           lea     bx,GDT
           add     eax,ebx
           mov     dword ptr GDTR[2],eax
           xor     eax,eax             ; Racuna linearnu adresu labele Prot
           mov     ax,cs
           shl     eax,4
           lea     bx,Protected
           add     eax,ebx
           mov     word ptr GDT[18],ax ; Nazalost 4-bajtna linearna adresa je
           rol     eax,16              ;  ruzno razbacana po deskriptoru
           mov     GDT[20],al
           mov     GDT[23],ah
           cli
           lgdt    pword ptr GDTR
           mov     eax,cr0             ; Ulazak u Protected mod
           or      al,1
           mov     cr0,eax
           jmp     dword ptr EntryVec
Protected: mov     ax,8
           mov     es,ax
           mov     ds,ax
           mov     fs,ax
           mov     gs,ax
           mov     eax,cr0             ; Izlaz iz Protected moda
           and     eax,0fffffffeh
           mov     cr0,eax
           jmp     far ptr RealAgain
RealAgain: sti                         ; Ne bas Real nego UnReal!
           xor     ax,ax
Exit:      pop     ds
           ret
SetUnReal  endp

CSeg       ends

           end
