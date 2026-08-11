{$A+,B-,D+,E+,F+,I-,L-,N-,O+,R-,S-,V+}
{$M 16384,0,655360}

unit imprek;
interface
uses hexedito;
procedure compress(ulazni_1,izlazni_1:pointer;dekompr:word;
 var kompr,sign:word;vezproc:PProcenter);
procedure decompress(ulazni_1,izlazni_1:pointer;
 var dekompr:word;kompr,sign:word);
implementation
uses dos,crt;
{$B-}
const
  nije_nadjen=65535;
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
var
  poceci,krajevi:array[0..255] of struktura;
  bitmask,nizi,visi,brojac_bitova:byte;
  frekvencija:array[0..255] of word;
  oslobodi:struktura;
  poredbeni:podstringtip;
  izbaceni_p:struktura;
  ulazni,izlazni:bafertip;
  timing,xtekst,ytekst,
  duzina_ulaznog_bafera,zadsig,duz_vel_izl,
  lastelement,preporuceni,tren_sig_za_dek,tren_pos_sig_za_dek,
  tren_ul_za_dek,zad_izl_dek,duzina_ul_dek,dek_duz_sign:word;
  dokle:PProcenter;

procedure anuliraj_pointere;
var
  i:integer;
begin
  for i:=0 to 255 do begin
    poceci[i]:=nil;
    krajevi[i]:=nil;
    frekvencija[i]:=0;
  end;
end;

procedure uzmi_bajt_iz_ulaznog(var b:byte; var kraj:Boolean);
var
  izbaci:byte;
  st,st2:string;
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
    str(lastelement:6,st);
    str(memavail:6,st2);

    st:='Address    '+st;
    if dokle<>nil then
      dokle^.update(lastelement div 2,st);
    gotoxy(74,25);
    write(st2);
  end;
  if lastelement >= duzina_ulaznog_bafera then kraj:=true else kraj:=false;
end;

function gdje_je_podstring(var podniz:podstringtip;koji_trazimo,duzina:word):word;

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
  izlazni^[zadsig]:=izlazni^[zadsig] or ormask [bitmask];
  inc(bitmask);
  if bitmask=8 then begin
    dec(zadsig);
    bitmask:=0;
  end
end;

procedure nb0;
begin
  izlazni^[zadsig]:=izlazni^[zadsig] and andmask [bitmask];
  inc(bitmask);
  if bitmask=8 then begin
    dec(zadsig);
    bitmask:=0;
  end
end;

procedure compress_b;
var
  kraj,uslov,uslov2: Boolean;
  b:byte;
  duzina_poredbenog,nadji,udaljenost,koji_trazimo,i,minfreq,pomfreq:word;

begin
  anuliraj_pointere;
  zadsig:=65519;
  bitmask:=0;
  lastelement:=0;
  duz_vel_izl:=0;
  preporuceni:=0;
  if duzina_ulaznog_bafera>0 then kraj:=false else kraj:=true;
  repeat
    if kraj then begin
      exit;
    end;
    uzmi_bajt_iz_ulaznog(b,kraj);
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
          if not(uslov2) then uzmi_bajt_iz_ulaznog(b,kraj);
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
            uzmi_bajt_iz_ulaznog(b,kraj);
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
          else writeln('Internal error in compresor');
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
          else writeln('Internal error');
       end {Case}
     end {else}
  until false
end;

procedure compress(ulazni_1,izlazni_1:pointer;
 dekompr:word;var kompr,sign:word; vezproc:Pprocenter);
var i,j:word;
begin
  timing:=mem[$46:$c];
  dokle:=vezproc;
  if vezproc<>nil then begin
    dokle^.scrollbar^.setrange(0,dekompr div 2);
    dokle^.scrollbar^.setvalue(1);
  end;
  ulazni:=ulazni_1;
  izlazni:=izlazni_1;
  duzina_ulaznog_bafera:=dekompr;
  mark(oslobodi);
  compress_b;
  j:=0;
  for i:=65519 downto zadsig do begin
    ulazni^[j]:=izlazni^[i];
    j:=j+1;
  end;
  move(izlazni^,ulazni^[j],duz_vel_izl);
  izlazni^:=ulazni^;
  release(oslobodi);
  sign:=j-1;
  kompr:=j-1+duz_vel_izl
end;

procedure uzmi_iz_ulaznog_za_dek(var b:byte;var kraj:Boolean);
begin
  b:=ulazni^[tren_ul_za_dek];
  inc(tren_ul_za_dek);
  if tren_ul_za_dek>duzina_ul_dek then kraj:=true else kraj:=false
end;

procedure stavi_u_izlazni_za_dek(b:byte);
begin
  izlazni^[zad_izl_dek]:=b;
  inc(zad_izl_dek);
end;

function nextbit:byte;
var
  rezultat:byte;
begin
  tren_sig_za_dek:=tren_sig_za_dek*2;
  if tren_sig_za_dek >=256 then rezultat:=1 else rezultat:=0;
  tren_sig_za_dek:=tren_sig_za_dek and 255;
  inc(brojac_bitova);
  if brojac_bitova=8 then begin
    inc(tren_pos_sig_za_dek);
    tren_sig_za_dek:=ulazni^[tren_pos_sig_za_dek];
    brojac_bitova:=0
  end;
  nextbit:=rezultat;
end;

procedure decompress_b;
label
  lb,b0,b1,b11,b01,b10,b110,b111,b010,b00,b100,b101,b011,b1100,
  b1101,b1110,b1111,v,v0,v1,v00,v01,v000,v001,v0000,v0001,v00100,v00101,
  v00110,v00111,v0010,v0011,izlaz;

var
  b,smjesti:byte;
  bajtova,vrati_se_za,i,pomocni:word;
  kraj:Boolean;
begin
  zad_izl_dek:=0;
  tren_ul_za_dek:=dek_duz_sign+1;
  tren_pos_sig_za_dek:=0;
  tren_sig_za_dek:=ulazni^[tren_pos_sig_za_dek];
  brojac_bitova:=0;
  if duzina_ul_dek=0 then kraj:=true else kraj:=false;
  while not kraj do begin
    if nextbit=0 then begin
      uzmi_iz_ulaznog_za_dek(b,kraj);
      stavi_u_izlazni_za_dek(b);
    end
     else begin
       {Znam, jezis se zbog goto, ali ovo je preglednije
        od ugnjezdenih if then else sekvenci}
        lb: if nextbit=0 then goto b0 else goto b1;
        b0: if nextbit=0 then goto b00 else goto b01;
        b1: if nextbit=0 then goto b10 else goto b11;
        b11: if nextbit=0 then goto b110 else goto b111;
        b01: if nextbit=0 then goto b010 else goto b011;
        b10: if nextbit=0 then goto b100 else goto b101;
        b110: if nextbit=0 then goto b1100 else goto b1101;
        b111: if nextbit=0 then goto b1110 else goto b1111;
        b010: bajtova:=2;
          uzmi_iz_ulaznog_za_dek(b,kraj);
          vrati_se_za:=b;
          goto izlaz;
        b00: bajtova:=3;goto v;
        b100: bajtova:=4;goto v;
        b101: bajtova:=5;goto v;
        b011: uzmi_iz_ulaznog_za_dek(b,kraj);
          bajtova:=b+10;goto v;
        b1100: bajtova:=6;goto v;
        b1101: bajtova:=7;goto v;
        b1110: bajtova:=8;goto v;
        b1111: bajtova:=9;goto v;
        v: uzmi_iz_ulaznog_za_dek(b,kraj);
        vrati_se_za:=b;
        if nextbit=0 then goto v0 else goto v1;
        v0: if nextbit=0 then goto v00 else goto v01;
        v1:goto izlaz;
        v00: if nextbit=0 then goto v000 else goto v001;
        v01: pomocni:=7;
          if nextbit=1 then pomocni:=pomocni+8;
          if nextbit=1 then pomocni:=pomocni+4;
          if nextbit=1 then pomocni:=pomocni+2;
          if nextbit=1 then pomocni:=pomocni+1;
          vrati_se_za:=vrati_se_za+256*pomocni;
          goto izlaz;
        v000: if nextbit=0 then goto v0000 else goto v0001;
        v001: if nextbit=0 then goto v0010 else goto v0011;
        v0010: if nextbit=0 then goto v00100 else goto v00101;
        v0011: if nextbit=0 then goto v00110 else goto v00111;
        v0000: vrati_se_za:=vrati_se_za+1*256;goto izlaz;
        v0001: vrati_se_za:=vrati_se_za+2*256;goto izlaz;
        v00100: vrati_se_za:=vrati_se_za+3*256;goto izlaz;
        v00101: vrati_se_za:=vrati_se_za+4*256;goto izlaz;
        v00110: vrati_se_za:=vrati_se_za+5*256;goto izlaz;
        v00111: vrati_se_za:=vrati_se_za+6*256;goto izlaz;
        izlaz:
        for i:=1 to bajtova do begin
          smjesti:=izlazni^[zad_izl_dek-vrati_se_za+1];
          stavi_u_izlazni_za_dek(smjesti)
        end;
     end {else}
  end {while}
end; {decompress_b}

procedure decompress(ulazni_1,izlazni_1:pointer;var dekompr:word;
 kompr,sign:word);
begin
  ulazni:=ulazni_1;
  izlazni:=izlazni_1;
  duzina_ul_dek:=kompr;
  dek_duz_sign:=sign;
  decompress_b;
  dekompr:=zad_izl_dek;
end;

end.
