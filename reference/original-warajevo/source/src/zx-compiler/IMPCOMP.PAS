{$A+,B-,D-,E-,F-,I-,L-,N-,O-,R-,S-,V+}
{$M 16384,0,655360}

unit impcomp;

interface

procedure compress(ulazni_1,izlazni_1:pointer;dekompr: word;
  var kompr,sign:word; prtofs:longint);

implementation

uses dos;

const nije_nadjen=65535;
  andmask:array[0..7] of byte=($7F,$BF,$DF,$EF,$F7,$FB,$FD,$FE);
  ormask:array[0..7] of byte=($80,$40,$20,$10,$08,$04,$02,$01);

type struktura=^struct;
  struct=record
    gdje:word;
    next,last:struktura;
  end;
  podstringtip=array[1..512] of byte;
  bafertip1=array[0..65519] of byte;
  bafertip=^bafertip1;

var poceci,krajevi:array[0..255] of struktura;
  bitmask,nizi,visi,brojac_bitova:byte;
  frekvencija:array[0..255] of word;
  oslobodi:struktura;
  poredbeni:podstringtip;
  izbaceni_p:struktura;
  ulazni,izlazni,signaturni:bafertip;
  timing,duzina_ulaznog_bafera,zadsig,duz_vel_izl,
  lastelement,preporuceni,tren_sig_za_dek,tren_pos_sig_za_dek,
  tren_ul_za_dek,zad_izl_dek,duzina_ul_dek,dek_duz_sign:word;

procedure anuliraj_pointere;

var i:integer;

begin
  for i:=0 to 255 do begin
    poceci[i]:=nil; krajevi[i]:=nil; frekvencija[i]:=0
  end
end;

procedure uzmi_bajt_iz_ulaznog(var b:byte; var kraj:Boolean; prtofs:longint);

var izbaci:byte;

begin
  b:=ulazni^[lastelement];
  inc(frekvencija[b]);
  if poceci[b]=nil then begin
    new(poceci[b]);
    krajevi[b]:=poceci[b];
    poceci[b]^.last:=nil;
  end
  else begin
    new(krajevi[b]^.next);
    krajevi[b]^.next^.last:=krajevi[b];
    krajevi[b]:=krajevi[b]^.next;
  end;
  krajevi[b]^.gdje:=lastelement;
  krajevi[b]^.next:=nil;
  if lastelement >=5880 then begin
    izbaci:=ulazni^[lastelement-5880];
    dec(frekvencija[izbaci]);
    izbaceni_p:=poceci[izbaci];
    poceci[izbaci]:=poceci[izbaci]^.next;
    poceci[izbaci]^.last:=nil;
    if poceci[izbaci]=nil then krajevi[izbaci]:=nil;
    dispose(izbaceni_p);
  end;
  lastelement:=lastelement+1;
  if memw[$46:$c]-timing>18 then begin
    timing:=memw[$46:$c];
    write(#8,#8,#8,#8,#8,#8,prtofs+lastelement:6);
  end;
  if lastelement>=duzina_ulaznog_bafera then kraj:=true else kraj:=false
end;

function gdje_je_podstring(var podniz:podstringtip;koji_trazimo,duzina:word)
  :word;

{ Ovom funkcijom se pretrazuje cijeli podniz, a varijabla koji_trazimo
  oznacava koji element podniza ce prvi biti trazen /preporucljivo najrjedji/
}

var
  i:integer;nadjen:Boolean;
  b:byte;
  trenutni:struktura;
  redni_u_baferu:word;

begin
  if preporuceni< lastelement-2*duzina then
    if ulazni^[preporuceni+duzina-1]=
     poredbeni[duzina] then begin
       gdje_je_podstring:=preporuceni;
       exit
     end;
  b:=podniz[koji_trazimo];
  if krajevi[b]=nil then gdje_je_podstring:=nije_nadjen
   else begin
     trenutni:=krajevi[b]^.last;
     nadjen:=false;
     while(trenutni<>nil) and not(nadjen) do begin
       redni_u_baferu:=trenutni^.gdje-koji_trazimo+1;
       if redni_u_baferu>=0 then begin
         inline(                    {INT 3 ZA DEBUG}
           $C4/$3e/ulazni/         {LES DI,ULAZNI}
           $03/$7e/<redni_u_baferu/ {ADD DI,BP[redni_u_baferu]}
           $BE/poredbeni/          {MOV SI,offset poredbeni}
           $8B/$4E/<duzina/         {MOV CX,BP[DUZINA]}
           $F3/$A6/                 {REP CMPSB}
           $C6/$46/<nadjen/$1/     { MOV bp[NADJEN],1}
           $74/$04/                 {JZ preskoci}
           $C6/$46/<nadjen/$00     { MOV bp[NADJEN],0}
         );
       end;
       if nadjen and (redni_u_baferu>=lastelement-2*duzina) then
         nadjen:=false;
       if nadjen then begin
         gdje_je_podstring:=redni_u_baferu;
       end
        else trenutni:=trenutni^.last;
     end;
     if not(nadjen) or (redni_u_baferu>=lastelement-2*duzina) then
       gdje_je_podstring:=nije_nadjen;
   end;  {else (krajevi <> nil)}
end;

procedure smjesti_u_izlazni(b:byte);
begin
  izlazni^[duz_vel_izl]:=b;
  inc(duz_vel_izl);
end;

procedure nb1;
begin
  signaturni^[zadsig]:=signaturni^[zadsig] or ormask [bitmask];
  inc(bitmask);
  if bitmask=8 then begin
    inc(zadsig);
    bitmask:=0;
  end
end;

procedure nb0;
begin
  signaturni^[zadsig]:=signaturni^[zadsig] and andmask [bitmask];
  inc(bitmask);
  if bitmask=8 then begin
    inc(zadsig);
    bitmask:=0;
  end
end;

procedure compress_b(prtofs:longint);
var
  kraj,uslov,uslov2: Boolean;
  b:byte;
  duzina_poredbenog,nadji,udaljenost,koji_trazimo,i,minfreq,pomfreq:word;

begin
  anuliraj_pointere;
  zadsig:=0;
  bitmask:=0;
  lastelement:=0;
  duz_vel_izl:=0;
  preporuceni:=0;
  if duzina_ulaznog_bafera>0 then kraj:=false else kraj:=true;
  repeat
    if kraj then begin
      exit;
    end;
    uzmi_bajt_iz_ulaznog(b,kraj,prtofs);
    uslov:=poceci[b]^.gdje<>lastelement-1;
    if not(uslov) then begin
      nb0;
      smjesti_u_izlazni(b);
    end;
  until uslov;
  repeat
    repeat
      if kraj then begin
        nb0;
        smjesti_u_izlazni(b);
        exit;
      end;
      duzina_poredbenog:=0;
      minfreq:=65535;
      koji_trazimo:=0;
      nadji:=nije_nadjen;
      preporuceni:=nije_nadjen;
      repeat
        inc(duzina_poredbenog);
        poredbeni[duzina_poredbenog]:=b;
        if frekvencija[b]<minfreq then begin
          koji_trazimo:=duzina_poredbenog;
          minfreq:=frekvencija[b]
        end;
        nadji:=gdje_je_podstring(poredbeni,koji_trazimo,duzina_poredbenog);
        preporuceni:=nadji;
        if lastelement-nadji>=5877 then nadji:=nije_nadjen;
        uslov:=nadji=nije_nadjen;
        uslov2:=false;
        if not(uslov) then begin
          uslov2:=kraj or (duzina_poredbenog>=265);
          if not(uslov2) then uzmi_bajt_iz_ulaznog(b,kraj,prtofs);
        end;
      until uslov or uslov2;
      dec(duzina_poredbenog);
      if koji_trazimo>duzina_poredbenog then begin
        minfreq:=65535;
        for i:=1 to duzina_poredbenog do begin
          if pomfreq<minfreq then begin
            minfreq:=pomfreq;
            koji_trazimo:=i
          end  {if}
        end   {for}
      end; {if}
      uslov:=duzina_poredbenog<3;
      if duzina_poredbenog=2 then begin
        nadji:=gdje_je_podstring(poredbeni,koji_trazimo,duzina_poredbenog);
        udaljenost:=lastelement-nadji-duzina_poredbenog;
        if udaljenost>255 then uslov:=true else uslov:=false;
      end;
      if uslov then
        case duzina_poredbenog of
          0: begin
            nb0;
            smjesti_u_izlazni(b);
            uzmi_bajt_iz_ulaznog(b,kraj,prtofs);
            minfreq:=65535;
            koji_trazimo:=0;
          end;
          1: begin
            nb0;
            smjesti_u_izlazni(poredbeni[1]);
            duzina_poredbenog:=0;
            minfreq:=65535;
            koji_trazimo:=0;
          end;
          2: begin
            nb0;
            smjesti_u_izlazni(poredbeni[1]);
            nb0;
            smjesti_u_izlazni(poredbeni[2]);
            duzina_poredbenog:=0;
            minfreq:=65535;
            koji_trazimo:=0;
          end
        end;
    until not(uslov);
    preporuceni:=nije_nadjen;
    nadji:=gdje_je_podstring(poredbeni,koji_trazimo,duzina_poredbenog);
    udaljenost:=lastelement-nadji-duzina_poredbenog;
    nb1;
    nizi:=udaljenost mod 256;
    visi:=udaljenost div 256;
    if duzina_poredbenog=2 then begin
      nb0;nb1;nb0;
      smjesti_u_izlazni(nizi);
    end
     else begin
       case duzina_poredbenog of
         3: begin
           nb0;nb0;
         end;
         4: begin
           nb1;nb0;nb0;
         end;
         5: begin
           nb1;nb0;nb1;
         end;
         6: begin
           nb1;nb1;nb0;nb0;
         end;
         7: begin
           nb1;nb1;nb0;nb1;
         end;
         8: begin
           nb1;nb1;nb1;nb0;
         end;
         9: begin
           nb1;nb1;nb1;nb1;
         end;
         10..265:begin
           nb0;nb1;nb1;
           smjesti_u_izlazni(duzina_poredbenog-10);
         end;
          else begin
            writeln;
            writeln('Internal error');
            halt(1)
          end
       end; {case}
       smjesti_u_izlazni(nizi);
       case visi of
         0: nb1;
         1: begin
           nb0;nb0;nb0;nb0;
         end ;
         2: begin
           nb0;nb0;nb0;nb1;
         end ;
         3: begin
           nb0;nb0;nb1;nb0;nb0;
         end ;
         4: begin
           nb0;nb0;nb1;nb0;nb1;
         end ;
         5: begin
           nb0;nb0;nb1;nb1;nb0;
         end ;
         6: begin
           nb0;nb0;nb1;nb1;nb1;
         end ;
         7: begin
           nb0;nb1;nb0;nb0;nb0;nb0;
         end ;
         8: begin
           nb0;nb1;nb0;nb0;nb0;nb1;
         end ;
         9: begin
           nb0;nb1;nb0;nb0;nb1;nb0;
         end ;
         10: begin
           nb0;nb1;nb0;nb0;nb1;nb1;
         end ;
         11: begin
           nb0;nb1;nb0;nb1;nb0;nb0;
         end ;
         12: begin
           nb0;nb1;nb0;nb1;nb0;nb1;
         end ;
         13: begin
           nb0;nb1;nb0;nb1;nb1;nb0;
         end ;
         14: begin
           nb0;nb1;nb0;nb1;nb1;nb1;
         end ;
         15: begin
           nb0;nb1;nb1;nb0;nb0;nb0;
         end ;
         16: begin
           nb0;nb1;nb1;nb0;nb0;nb1;
         end ;
         17: begin
           nb0;nb1;nb1;nb0;nb1;nb0;
         end ;
         18: begin
            nb0;nb1;nb1;nb0;nb1;nb1;
         end ;
         19: begin
            nb0;nb1;nb1;nb1;nb0;nb0;
         end ;
         20: begin
            nb0;nb1;nb1;nb1;nb0;nb1;
         end ;
         21: begin
            nb0;nb1;nb1;nb1;nb1;nb0;
         end ;
         22: begin
            nb0;nb1;nb1;nb1;nb1;nb1;
         end ;
          else begin
            writeln;
            writeln('Internal error');
            halt
          end
       end {Case}
     end {else}
  until false
end;

procedure compress(ulazni_1,izlazni_1:pointer;
 dekompr:word;var kompr,sign:word;prtofs:longint);
begin
  timing:=mem[$46:$c];
  write(0:6);
  ulazni:=ulazni_1;
  izlazni:=izlazni_1;
  duzina_ulaznog_bafera:=dekompr;
  mark(oslobodi);
  new(signaturni);
  compress_b(prtofs);
  write(#8,#8,#8,#8,#8,#8,prtofs+lastelement:6);
  move(izlazni^,signaturni^[zadsig+1],duz_vel_izl);
  izlazni^:=signaturni^;
  release(oslobodi);
  sign:=zadsig;
  kompr:=zadsig+duz_vel_izl
end;

end.
