
unit setstate;
{$O+,V-}
interface
uses objects;
procedure setmenu(r:TRect);
procedure setstatline(r:trect);
implementation
uses app,menus,menucons,helpctx,drivers,views;
procedure setmenu(r:TRect);
begin
  MenuBar := New(PMenuBar, Init(R, NewMenu(
    NewSubMenu('~T~apeFiles',hctfiles,NewMenu(
      NewSubMenu('~T~apes',hcttapes,NewMenu(
        NewItem('~S~elect','F2',kbF2,cm_tselect,hctselect,
        NewItem('~U~nselect','',0,cm_tunselect,hctunselect,
        NewItem('~P~arameters','Shift+F2',kbShiftF2,cm_ttapepar,hctparameters,
        NewItem('~V~iew','Alt+F2',kbAltF2,cm_tview,hcview,
        NewSubMenu ('~P~rint',hcprint,NewMenu(
          NewItem('~S~creen','',0,cm_tprscr,hcprscr,
          NewItem('~P~rinter','',0,cm_tprpri,hcprpri,
          NewItem('~F~ile','',0,cm_tprfile,hcprfile,
          nil)))),
        NewItem('Copy to ~N~ew','',0,cm_tcopynew,hccopynew,
        nil))))))
      ),
      NewSubMenu('~B~locks',hcblocksmenu,NewMenu(
        NewItem('~R~eorder','',0,cm_treorder,hcreorder,
        NewItem('E~x~tract','',0,cm_textract,hcextract,
        NewItem('~D~elete','',0,cm_tdelete,hcdelete,
        NewItem('~A~dd','',0,cm_tadd,hcadd,
        NewItem('~E~dit','',0,cm_tedit,hcedit,
        NewItem('Ex~c~lude','',0,cm_texclude,hcexclude,
        NewItem('Change ~P~ossition','',0,cm_tchgpos,hcchgpos,
        nil)))))))
      ),
      NewSubMenu('~I~mplode',hcimplode,NewMenu(
        NewSubMenu('~C~ompress',hccompress,NewMenu(
          NewItem('~A~ll blocks','',0,cm_tcompall,hccompall,
          NewItem('~L~inearize only','',0,cm_tlinonly,hclinonly,
          NewItem('Compress ~S~elected','',0,cm_tcompsel,hccompsel,
        nil)))),
        NewItem('~D~ecompress','',0,cm_tdecomp,hcdecomp,
        NewItem('~E~fficiency','',0,cm_teff,hceff,
        nil)))
      ),
      NewSubMenu('~C~ommunications',hccomm,NewMenu(
        NewSubMenu('~R~S232',hccommrs232,NewMenu(
          NewItem('~S~end to Spectrum','',0,cm_tsendtsp,hcsendtsp,
          NewItem('~R~eceive from Spectrum','',0,cm_trecsp,hcrecsp,
          NewItem('Send communication ~P~rogram','',0,cm_sendcprg,hcsendcprg,
          NewItem('~C~onfigure RS232','',0,cm_cfgrs232,hccfgrs232,
        nil))))),
        NewSubMenu('~C~assette',hccommcinf,NewMenu(
          NewItem('~R~eceive from cassette','',0,cm_tcasstransfer,hccasstransfer,
          NewItem('~S~ample from cassette','',0,cm_tcasssample,hccasssample,
          NewItem('~A~dvanced setup','',0,cm_tadvcassetting,hcadvcassetting,
        nil)))),
        nil))

      ),
      NewSubMenu('C~o~nvert',hctconvert,NewMenu(
        NewSubMenu('ASCII<->~B~asic',hcascbasm,NewMenu(
          NewItem('~A~SCII->Basic','',0,cm_tascbas,hcascbas,
          NewItem('~B~asic->Ascii','',0,cm_tbasasc,hcbasasc,
          nil))
        ),
        NewSubMenu('ASCII<->~H~isoft',hcaschis,NewMenu(
          NewItem('~A~SCII->Gens','',0,cm_tascgens,hcascgens,
          NewItem('A~S~CII->C','',0,cm_tascc,hcascc,
          NewItem('ASC~I~I->Pascal','',0,cm_tascpas,hcascpas,
          NewItem('~G~ens->ASCII','',0,cm_tgensasc,hcgensasc,
          NewItem('~C~->ASCII','',0,cm_tcasc,hccasc,
          NewItem('~P~ascal->Ascii','',0,cm_tpasasc,hcpasasc,
          nil))))))
        ),
        NewSubMenu('ASCII<->~T~assword',hcasctasw,NewMenu(
          NewItem('~A~SCII->Tassword 2','',0,cm_tasct2,hctasct2,
          NewItem('A~S~CII->Tassword 3','',0,cm_tasct3,hctasct3,
          NewItem('Tassword ~2~->ASCII','',0,cm_tt2asc,hctt2asc,
          NewItem('Tassword ~3~->ASCII','',0,cm_tt3asc,hctt3asc,
          nil))))
        ),
        NewSubMenu('ASCII<->The Last ~W~ord',hcasctlwv,NewMenu(
          NewItem('~A~SCII->The Last Word','',0,cm_tasctlw,hcasctlw,
          NewItem('The Last ~W~ord->ASCII','',0,cm_tlwasc,hctlwasc,
          nil))
        ),
        NewSubMenu('ASCII<->~M~achine Lightning',hcascmlv,NewMenu(
          NewItem('~A~SCII->Machine Lightning','',0,cm_tascml,hcascml,
          NewItem('~M~achine Lightning->ASCII','',0,cm_tmlasc,hcmlasc,
          nil))
        ),
        NewSubMenu('ASCII<->~F~orth',hcascfortfv,NewMenu(
          NewItem('~A~SCII->Abersoft Forth','',0,cm_tascforth,hcascforth,
          NewItem('Abersoft ~F~orth->ASCII','',0,cm_tforthasc,hcforthasc,
          nil))
        ),
        NewSubMenu('ASCII<->~L~ogo',hcasclogov,NewMenu(
          NewItem('~A~SCII->Sinclair Logo','',0,cm_tasclogo,hcasclogo,
          NewItem('Sinclair ~L~ogo->ASCII','',0,cm_tlogoasc,hclogoasc,
          nil))
        ),
        NewLine(
        NewSubMenu('~S~CREEN$->Tiff',hcscrtif,NewMenu(
          NewItem('~C~olor TIFF picture','',0,cm_tscrcoltif,hcscrcoltif,
          NewItem('~B~lack and white TIFF','',0,cm_tscrbwtif,hcscrbwtif,
          nil))
        ),
        NewLine(
        NewSubMenu('~O~ther emulators',hcother,NewMenu(
          NewSubMenu('Warajevo<->~R~oman & Easy',hcRomWara,NewMenu(
            NewItem('~R~oman&Easy Inc->Warajevo','',0,cm_tromwara,hcromwara,
            NewItem('~W~arajevo->Roman&Easy Inc','',0,cm_twararoman,hcwararoman,nil))
          ),
          NewSubMenu('Warajevo<->~L~unter Z80',hcLunWara,NewMenu(
            NewItem('~L~unter TAP->Warajevo TAP','',0,cm_tlunwara,hclunwara,
            NewItem('~W~arajevo TAP->Lunter TAP ','',0,cm_twaralun,hcwaralun,nil))
          ),
          NewSubMenu('Warajevo<->~I~rish Specem',hcIrishWara,NewMenu(
            NewItem('~I~rish Specem ->Warajevo TAP','',0,cm_tirishwara,hcirishwara,
            NewItem('~W~arajevo TAP->Irish Specem','',0,cm_twarairish,hcwarairish,nil))
          ),
          NewSubMenu('Warajevo<->~P~olish SP',hcPolishWara,NewMenu(
            NewItem('~P~olish SP SPC->Warajevo TAP','',0,cm_tpolishwara,hcpolishwara,
            NewItem('~W~arajevo TAP->Polish SP SPC','',0,cm_twarapolish,hcwarapolish,nil))
          ),
          NewSubMenu('Warajevo<->Spectrum ~2~.00',hcSpec200Wara,NewMenu(
            NewItem('~B~LK->Warajevo TAP','',0,cm_tspec20wara,hcSpec200wara,
            NewItem('~W~arajevo TAP->BLK','',0,cm_twaraspec20,hcWaraSpec200,nil))
          ),
          NewSubMenu('Warajevo<->ZX ~G~arabik',hcGarabikWara,NewMenu(
            NewItem('~L~TP->Warajevo TAP','',0,cm_tgarabikwara,hcGarabikWara,
            NewItem('~W~arajevo TAP->LTP','',0,cm_twaragarabik,hcWaraGarabik,nil))
          ),
          NewSubMenu('Warajevo<->ZX ~B~rukner',hcBruknerWara,NewMenu(
            NewItem('ZX ~B~rukner->Warajevo TAP','',0,cm_tbruknerwara,hcBruknerWara,
            NewItem('~W~arajevo TAP->ZX Brukner','',0,cm_twarabrukner,hcWaraBrukner,nil))
          ),
          NewSubMenu('Warajevo<->~T~ZX',hcTZXWara,Newmenu(
            NewItem('~T~ZX->Warajevo TAP','',0,cm_ttzxwara,hcTZXwara,
            NewItem('~W~arajevo TAP->TZX','',0,cm_twaratzx,hcWaraTZX,
            NewItem('TZX conversion ~S~etup','',0,cm_tzxsetup,hcTZXSetup,nil)))

          ),
          NewSubMenu('Warajevo<->ZX ~M~useum',hcZXSWara,NewMenu(
            NewItem('~M~useum ZXS->Warajevo','',0,cm_tmuseumwara,hcZXSWara,
            NewItem('~W~arajevo TAP->Museum ZXS','',0,cm_twaramuseum,hcWaraZXS,nil))
          ),
          NewSubMenu('Warajevo<->Spectrum+TR~D~OS',hcTRDWara,NewMenu(
            NewItem('~T~RD->Warajevo TAP','',0,cm_tshalaevwara,hcTrdWara,
            NewItem('~W~arajevo TAP->TRD','',0,cm_twarashalaev,hcWaraTRD,nil))
          ),
          NewSubMenu('Warajevo<->ZX-~3~2',hcZX32Wara,NewMenu(
            NewItem('~Z~X32 ZXT->Warajevo TAP','',0,cm_tzx32wara,hcZX32Wara,
            NewItem('~W~arajevo TAP->ZX32 ZXT','',0,cm_twarazx32,hcWaraZX32,nil))
          ),
          NewSubMenu('Warajevo<->~J~PP',hcVocWara,NewMenu(
            NewItem('VOC->Warajevo TAP','',0,cm_tvocwara,hcvocWara,
            NewItem('~W~arajevo TAP->VOC','',0,cm_twaravoc,hcWaravoc,nil))
          ),

          nil))))))))))))),

        nil)))))))))))
      ),
    nil)))))),
    NewSubMenu('~Z~80Snaps', hcsnapshots ,NewMenu(
      NewSubMenu('~S~naps',hcsnaps,NewMenu(
        NewItem('~S~elect','Shift+F7',kbShiftF7,cm_sselect,hcselect,
        NewItem('~U~nselect','F9',kbF9,cm_sunselect,hcsunselect,
        NewItem('~R~ename','F7',kbF7,cm_srename,hcsrename,
        NewItem('~I~nfo','',0,cm_sinfo,hcsinfo,
        nil))))
      ),
      NewSubMenu('~E~dit',hcsedit,NewMenu(
        NewItem('~P~rocesor registers','',0,cm_sreg,hcseditreg,
        NewItem('~H~ardware devices','',0,cm_shard,hcsedithard,
        NewItem('~M~emory','F3',kbF3,cm_smem,hceditmeme,
        nil)))
      ),
     NewSubMenu('~C~onvert',hcconvert,NewMenu(
        NewSubMenu('~S~panish Spectrum <-> Warajevo',hcspwaram,NewMenu(
          NewItem('~S~P->Z80','',0,cm_sspwara,hcsspwara,
          NewItem('~Z~80->SP','',0,cm_swarasp,hcswarasp,
          nil))
        ),

        NewSubMenu('~V~GASPEC <-> Warajevo',hcsvgawaram,NewMenu(
          NewItem('Old ~S~P->Z80','',0,cm_svgawara,hcsvgawara,
          NewItem('~Z~80->Old SP','',0,cm_swaravga,hcswaravga,
          nil))
        ),
        NewSubMenu('~I~rish Specem <-> Warajevo',hcsirishwaram,NewMenu(
          NewItem('~P~RG->Z80','',0,cm_sirishwara,hcsirishwara,
          NewItem('~Z~80->PRG','',0,cm_swarairish,hcswarairish,
          nil))
        ),
        NewSubMenu('~J~PP <-> Warajevo',hcwarajppm,NewMenu(
          NewItem('~S~NA->Z80','',0,cm_sjppwara,hcjppwara,
          NewItem('~Z~80->SNA','',0,cm_swarajpp,hcwarajpp,
          nil))
        ),
        NewSubMenu('Specemu ~G~ <-> Warajevo',hcWaraGermanM,NewMenu(
          NewItem('~S~EM->Z80','',0,cm_sgermanwara,hcGermanWara,
          NewItem('~Z~80->SEM','',0,cm_swaragerman,hcWaraGerman,
          nil))
        ),
        NewSubMenu('SP ~U~KV <-> Warajevo',hcwaraspukvm,NewMenu(
          NewItem('~S~NA 128->Z80','',0,cm_ssna128wara,hcspukvwara,
          NewItem('~Z~80->SNA 128','',0,cm_swarasna128,hcwaraspukv,
          nil))
        ),
        NewSubMenu('Nuclear ZX <->Warajevo',hcwarasnpm,NewMenu(
          NewItem('~S~NP->Z80','',0,cm_ssnpwara,hcsnpwara,
          NewItem('~Z~80->SNP','',0,cm_swarasnp,hcWaraSnp,
          nil))
        ),
        NewSubMenu('~X~128 <-> Warajevo',hcWaraX128m,NewMenu(
          NewItem('~S~LT->Z80+TAP','',0,cm_ssltwara,hcx128Wara,
          NewItem('~Z~80+TAP->SLT','',0,cm_swaraslt,hcwaraX128,
          nil))
        ),
        NewSubMenu('Spectrum ~2~.00 <-> Warajevo',hcWaraSitmM,NewMenu(
          NewItem('~S~IT->Z80','',0,cm_ssitwara,hcsitwara,
          NewItem('~Z~80->SIT','',0,cm_swarasit,hcwarasit,
          nil))
        ),
        NewSubMenu('ZX~3~2<->Warajevo',hcwaravaggm,NewMenu(
          NewItem('ZX~S~ RIFF->Z80','',0,cm_szx32wara,hcvaggwara,
          NewItem('~Z~80->ZXS RIFF','',0,cm_swarazx32,hcwaravagg,nil))
        ),

        NewSubMenu('~Z~80 (48K) <-> Z80 (128K)',hcz80z80,NewMenu(
        NewItem('Any Z80-> ~4~8K without Int. 1','',0,cm_sz8048,hcz8048,
        NewItem('Any Z80-> ~1~28K without Int. 1','',0,cm_sz80128,hcz80128,
          nil))
        ),

        NewItem('Z80 snapshot -> ~T~AP','',0,cm_ssntap,hcsnaptap,
          nil)
        )))))))))))),
        nil)))),
    NewSubMenu('~M~driveFiles',hcMicrodrive,NewMenu(
      NewSubMenu('~M~icrodrive',hcmicrosub,NewMenu(
        NewItem('~S~elect','F8',kbF8,cm_mdvsel,hcmdvsel,
        NewItem('~U~nselect','',0,cm_munselect,hcmunselect,
        NewItem('~D~efault','Shift+F8',kbShiftF8,cm_mdvdef,hcmdvdef,
        NewItem('~C~atalog','',0,cm_mdvcat,hcmdvcat,
        NewItem('~F~ormat','',0,cm_mdvformat,hcmdvformat,
        NewItem('~O~ptimize','',0,cm_mdvunfrag,hcmdvunfrag,
        NewItem('~V~iew','',0,cm_mdvview,hcmdvview,
        NewItem('~R~ename','',0,cm_mdvrename,hcmdvrename,
        NewItem('~W~rite protect','',0,cm_mdvwrprot,hcmdvwrprot,
        NewItem('Write ~u~nprotect','',0,cm_mdvwrunpr,hcmdvwrunpr,
        nil))))))))))),
      NewSubMenu('~S~ectors',hcmdvsectors,NewMenu(
        NewItem('~V~erify','',0,cm_mdvsverify,hcmdvsverify,
        NewItem('~E~dit','',0,cm_mdvsedit,hcmdvsedit,
        nil))),
      NewSubMenu('~F~iles',hcmdvfiles,NewMenu(
        NewItem('~D~elete','',0,cm_mdvfdelete,hcmdvfdelete,
        NewItem('~R~ename','',0,cm_mdvfrename,hcmdvfrename,
        NewItem('~H~ide','',0,cm_mdvfhide,hcmdvfhide,
        NewItem('~U~nhide','',0,cm_mdvfunhide,hcmdvfunhide,
        NewItem('~C~opy','',0,cm_mdvfcopy,hcmdvfcopy,

        nil)))))),
      NewSubMenu('~C~onvert',hcMConvert,NewMenu(
        NewItem('Enlarge MDR to 254 sectors','',0,cm_mdvcwaralunter,hcMConvert,
        nil))

      ,nil))))),

    NewSubMenu('Do~c~kFiles',hcDockFilesMenu,NewMenu(
      NewSubMenu('~D~ocks',hcdocks,NewMenu(
        NewItem('~S~elect','',0,cm_dockselect,hcdockselect,
        NewItem('~U~nselect','',0,cm_dockunselect,hcdockunselect,
        NewItem('~V~iew','',0,cm_dockview,hcdockview,
        NewItem('~M~erge','',0,cm_dockmerge,hcdockmerge,
        nil))))),
      NewSubMenu('~C~onvert',hcdockConvert,NewMenu(
        NewItem('~R~OM to DCK','',0,cm_romtodock,hcromtodck,
        NewItem('Binary to ~L~ROS','',0,cm_binlros,hcbinlros,
        NewItem('Binary to ~A~ROS','',0,cm_binaros,hcbinaros,
        nil))))
      ,nil))),

{
    NewSubMenu('Dis~k~Files',hcMicrodrive,NewMenu(
      NewSubMenu('This menu is reserved for version 3.0',hcmicrosub,NewMenu(
        NewItem('~S~elect','F8',kbF8,cm_mdvsel,hcmdvsel,
        NewItem('~U~nselect','',0,cm_munselect,hcmunselect,
        NewItem('~V~iew','',0,cm_mdvview,hcmdvview,
        nil)))),
      NewSubMenu('~C~onvert',hcMConvert,NewMenu(
        NewItem('Enlarge MDR to 254 sectors','',0,cm_mdvcwaralunter,hcMConvert,
        nil))
      ,nil))),
}

     NewSubMenu('Data~B~ase',hcDatabase,NewMenu(
       NewSubmenu('DBF ~D~irectory',hcdbfdir,NewMenu(
         NewItem('~S~elect directory','',0,cm_dbselect,hcdbselect,
         NewItem('~U~nselect','',0,cm_dbunselect,hcdbunselect,nil))),
       NewItem('~R~un','Shift+F10',kbShiftF10,cm_dbrun,hcdbrun,
       NewItem('~E~dit','F6',kbF6,cm_editbase,hceditbase,
       NewItem('So~r~t','',0,cm_sortbase,hcsortbase,
       NewSubMenu('~M~ark sort priority',hcmarkbase,NewMenu(
         NewItem('~C~onditional','',0,cm_bmarkcond,hcbmarkcond,
         NewItem('~S~wap marker','',0,cm_bswapmark,hcbswapmark,
         NewItem('~U~nmark all','',0,cm_bunmark,hcbunmark,
         nil)))),
       NewItem('~R~eport','',0,cm_izvjbase,hcizvjbase,
       NewSubMenu('~C~onvert',hcDBConvert,NewMenu(
         NewItem('Warajevo->Spec~P~ic','',0,cm_cwaraspecpic,hcWaraSpecPic,
         NewItem('Warajevo->~S~GD','',0,cm_cwarasgd,hcWaraSGD,
         NewItem('Warajevo->ZX ~R~ainbow','',0,cm_cwararainbow,hcWaraRainbow,
         NewItem('Warajevo->Spec~B~ase','',0,cm_cwaraspecbase,hcWaraSpecBase,
         NewItem('S~G~D->Warajevo','',0,cm_sgdwara,hcSGDWara,
         NewItem('Specpic->Warajevo','',0,cm_cspecpicwara,hcSpecpicWara,
         NewItem('ZX Rainbow->Warajevo','',0,cm_crainbowwara,hcRainbowWara,
         NewItem('SpecBase->Warajevo','',0,cm_cspecbasewara,hcSpecBaseWara,

         nil))))))))),nil

     )))))))),

     NewSubMenu('Set~u~p',hcsetup,NewMenu(
       NewItem('~E~mulator','',0,cm_emulsel,hcemulsel,

       NewItem('~V~ideo','F5',kbF5,cm_video,hcvideo,
       NewItem('~S~peed','',0,cm_speed,hcspeed,
       NewItem('~T~est','',0,cm_test,hctest,
       NewItem('~N~etwork','',0,cm_network,hcnetwork,
       NewItem('S~o~und','F4',kbF4,cm_sound,hcsound,
       NewItem('Interface ~1~','',0,cm_inter1,hcinter1,
       NewItem('~J~oystick','',0,cm_joystick,hcjoystick,
       NewItem('~P~rinter','',0,cm_printer,hcprinter,
       NewItem('~R~OM','',0,cm_ROM,hcrom,
       NewItem('MS ~W~indows','',0,cm_mswindows,hcmswindows,nil)

     ))))))))))),

    NewSubMenu('~D~os',hcDos,NewMenu(
      NewItem('~A~bout','',0,cm_about,hcAbout,
      NewItem('~D~irectory','',0,cm_dir,hcDir,
      NewItem('~C~hange directory','',0,cm_cd,hccd,
      NewItem('~O~S Shell','',0,cm_osshell,hcosshell,
      NewItem('E~x~it','Alt+X',kbAltX,cmQuit,hcquit,nil)
    ))))),nil))))))))));
end;
procedure setstatline(r:trect);
begin
  StatusLine := New(PStatusLine, Init(R,
    NewStatusDef(0, $FFFF,
      NewStatusKey('', kbCtrlF10, cmMenu,
      NewStatusKey('~S~tart³', kbAltS,cm_Start,
      NewStatusKey('~F1~ Help', kbF1, cmHelp,
      NewStatusKey('~Alt+F4~ Exit', kbAltF4, cmQuit,
      NewStatusKey('~Ctl+F4~ Close', kbCtrlF4, cmClose,
      NewStatusKey('~F10~ Emulator', kbF10, cm_Emulator,
      NewStatusKey('~Alt+F5~ Zoom', kbAltF5, cmZoom,
      NewStatusKey('', kbCtrlF5, cmResize,

      nil)))))))),
    nil)
  ));
end;

end.
