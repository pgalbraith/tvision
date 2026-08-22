#
#   Open Watcom makefile for Turbo Vision (wmake)
#
#   Builds the library and the example programs for one target:
#
#       wmake -h TARGET=dos16     16-bit real mode DOS, large model   (tvw16.lib)
#       wmake -h TARGET=dos32     32-bit DOS/4G extender, flat model  (tvw32.lib)
#       wmake -h TARGET=win32     32-bit Win32 console, flat model    (tvwnt.lib)
#
#   TARGET defaults to dos16. Run from this directory. Libraries go to
#   ..\..\LIB, objects and executables to .\<TARGET>. Other macros:
#
#       DEBUG=1     full debug info (-d2 / -d1, wlink debug all)
#       EXAMPLES=   override the example list
#                   (default: tvdemo tvedit tvhc tvdir tvforms)
#
#   tvforms needs its two forms data files, which the genparts and genphone
#   programs write when run. They are built for every target, but can only
#   be run where the target's executables run: the win32 build runs them
#   itself (the 'forms' target, part of 'all'), producing parts.f32 and
#   phonenum.f32 in .\win32 - and those files also serve the DOS/4G build,
#   since both are __FLAT__ targets. For dos16, run .\dos16\genparts.exe
#   and genphone.exe under DOS to get the .f16 files.
#
#   Open Watcom's binnt (or binw) directory must be on PATH: wlink only finds
#   its system definitions (wlsystem.lnk) through PATH, not by absolute
#   invocation.
#
#   Notes that are easy to lose:
#
#   - -xr (RTTI) is required for the whole tree: tobjstrm.cpp uses
#     dynamic_cast<void *>, which Watcom rejects without it.
#   - Only hardware.asm, swapst.asm and wcstubs.asm are assembled, and only
#     for the 16-bit target. Every other .asm file has a C++ twin that is
#     compiled instead, so there is no GENINC / TVWRITE.INC step (tv.inc
#     skips the include under __WASM__).
#   - geninc.cpp is the Borland GENINC tool, not library code.
#   - Win32 needs _WIN32_WINNT=0x0501 and psapi.lib (the examples touch
#     GetConsoleWindow / process information).
#

!ifndef TARGET
TARGET = dos16
!endif

TVDIR   = ..\..
TVSRC   = $(TVDIR)\source\tvision
TVINC   = $(TVDIR)\include
EXDIR   = $(TVDIR)\examples
LIBDIR  = $(TVDIR)\LIB
OUTDIR  = $(TARGET)
OBJDIR  = $(OUTDIR)\obj

!ifndef EXAMPLES
EXAMPLES = tvdemo tvedit tvhc tvdir tvforms
!endif

# ------------------------------------------------------------------ targets

!ifeq TARGET dos16
CC      = wpp
ASM     = wasm
SYSTEM  = dos
LIBNAME = tvw16.lib
CMODEL  = -bt=dos -ml
AMODEL  = -0 -ml
LNKOPTS = option stack=8k
AOBJS   = hardware.obj swapst.obj wcstubs.obj
XDEFS   =
XLIBS   =
FORMS   =
!else ifeq TARGET dos32
CC      = wpp386
ASM     = wasm
SYSTEM  = dos4g
LIBNAME = tvw32.lib
CMODEL  = -bt=dos -mf
AMODEL  = -3 -mf
LNKOPTS = option stack=32k
AOBJS   =
XDEFS   =
XLIBS   =
FORMS   =
!else ifeq TARGET win32
CC      = wpp386
ASM     = wasm
SYSTEM  = nt
LIBNAME = tvwnt.lib
CMODEL  = -bt=nt -mf
AMODEL  = -3 -mf
LNKOPTS = option stack=32k
AOBJS   =
XDEFS   = -D_WIN32_WINNT=0x0501
XLIBS   = library psapi.lib
FORMS   = forms
!else
!error TARGET must be one of dos16, dos32, win32
!endif

!ifdef DEBUG
CDEBUG  = -d2
ADEBUG  = -d1
LDEBUG  = debug all
!else
CDEBUG  =
ADEBUG  =
LDEBUG  =
!endif

LIB     = wlib
LINK    = wlink
LIBFILE = $(LIBDIR)\$(LIBNAME)

CFLAGS  = $(CMODEL) -zq -xr -w1 $(CDEBUG) $(XDEFS) -i=$(TVINC)
AFLAGS  = $(AMODEL) -q -zq $(ADEBUG)

# --------------------------------------------------------------- path search

.EXTENSIONS:
.EXTENSIONS: .exe .lib .obj .cpp .asm

.cpp : $(TVSRC);$(EXDIR)\tvdemo;$(EXDIR)\tvedit;$(EXDIR)\tvhc;$(EXDIR)\tvdir;$(EXDIR)\tvforms
.asm : $(TVSRC)
.obj : $(OBJDIR)
.exe : $(OUTDIR)

# ------------------------------------------------------------------ objects

# Translated assembly (portable C++ twins of the Borland .asm files).
TOBJS1 = edits.obj framelin.obj tgrmv.obj ttprvlns.obj tvcursor.obj &
         tvexposd.obj tvwrite.obj

# Watcom-specific platform files: DOS/4G and Win32 hardware drivers, the
# 16-bit interrupt layer, findfirst/findnext. Each compiles to an empty
# object on the targets it does not apply to.
TOBJS2 = wchwdos.obj wchwnt.obj wcsysint.obj wcdir.obj

TOBJS3 = colorsel.obj drivers.obj  drivers2.obj editstat.obj &
         fmtstr.obj   grp.obj      hardwrvr.obj help.obj     &
         helpbase.obj histlist.obj mapcolor.obj menu.obj     &
         misc.obj     msgbox.obj   new.obj      newstr.obj   &
         palette.obj  popupmnu.obj prntcnst.obj snprintf.obj &
         stddlg.obj   syserr.obj   tapplica.obj tbkgrnd.obj  &
         tbutton.obj  tchdrdlg.obj tcheckbo.obj tclipbrd.obj &
         tcluster.obj tcmdset.obj  tcollect.obj tdesktop.obj &
         tdialog.obj  tdircoll.obj tdirlist.obj teditor1.obj &
         teditor2.obj teditwnd.obj tevent.obj   textview.obj &
         tfildlg.obj  tfilecol.obj tfiledtr.obj tfillist.obj &
         tframe.obj   tgroup.obj   thistory.obj thistwin.obj &
         thstview.obj tindictr.obj tinputli.obj tkey.obj     &
         tlabel.obj   tlistbox.obj tlstview.obj tmemo.obj    &
         tmenubar.obj tmenubox.obj tmenupop.obj tmnuview.obj &
         tmouse.obj   tmulchkb.obj tobject.obj  tobjstrm.obj &
         toutline.obj tparamte.obj tprogram.obj tradiobu.obj &
         trescoll.obj tresfile.obj tscreen.obj  tscrlbar.obj &
         tscrolle.obj tsortcol.obj tstatict.obj tstatusl.obj &
         tstrcoll.obj tstrlist.obj tstrview.obj tsurface.obj &
         ttimerqu.obj tvalidat.obj tview.obj    tvtext1.obj  &
         tvtext2.obj  twindow.obj

NOBJS  = nmbkgrnd.obj nmbutton.obj nmchdrdl.obj nmchkbox.obj &
         nmclrsel.obj nmclustr.obj nmcollct.obj nmdialog.obj &
         nmdirbox.obj nmdircol.obj nmdsktop.obj nmeditor.obj &
         nmfilcol.obj nmfildlg.obj nmfillst.obj nmframe.obj  &
         nmgroup.obj  nmhist.obj   nminptln.obj nmlabel.obj  &
         nmlstbox.obj nmlstvwr.obj nmmnubar.obj nmmnubox.obj &
         nmmnupop.obj nmmnuvw.obj  nmmulchk.obj nmoutlin.obj &
         nmprmtxt.obj nmrbtns.obj  nmrescol.obj nmscoll.obj  &
         nmscrbar.obj nmscroll.obj nmstctxt.obj nmstddlg.obj &
         nmstline.obj nmstrcol.obj nmstrlst.obj nmvalida.obj &
         nmview.obj   nmwindow.obj

SOBJS  = sbkgrnd.obj  sbutton.obj  schdrdlg.obj scheckbo.obj &
         sclrsel.obj  scluster.obj sdesktop.obj sdialog.obj  &
         sdircoll.obj sdirlist.obj seditors.obj sfilcoll.obj &
         sfildlg.obj  sfilelst.obj sfinfpne.obj sfinputl.obj &
         sframe.obj   sgroup.obj   shistory.obj sinputli.obj &
         slabel.obj   slistbox.obj slstview.obj smenubar.obj &
         smenubox.obj smenupop.obj smnuview.obj smulchkb.obj &
         soutline.obj sparamte.obj sradiobu.obj srescoll.obj &
         sscrlbar.obj sscrolle.obj sstatict.obj sstatusl.obj &
         sstrcoll.obj sstrlst.obj  strmstat.obj svalid.obj   &
         sview.obj    swindow.obj

OBJS   = $(AOBJS) $(TOBJS1) $(TOBJS2) $(TOBJS3) $(NOBJS) $(SOBJS)

TVDEMO_OBJS = ascii.obj    backgrnd.obj calc.obj     calendar.obj &
              evntview.obj fileview.obj gadgets.obj  mousedlg.obj &
              puzzle.obj   tvdemo1.obj  tvdemo2.obj  tvdemo3.obj
TVEDIT_OBJS = tvedit1.obj tvedit2.obj tvedit3.obj
TVHC_OBJS   = tvhc.obj
TVDIR_OBJS  = tvdir.obj
FORMS_OBJS  = forms.obj datacoll.obj fields.obj listdlg.obj
TVFORMS_OBJS = tvforms.obj $(FORMS_OBJS)
GENPARTS_OBJS = genparts.obj $(FORMS_OBJS)
GENPHONE_OBJS = genphone.obj $(FORMS_OBJS)

# -------------------------------------------------------------------- rules

all : dirs $(LIBFILE) $(EXAMPLES) $(FORMS) .SYMBOLIC

lib : dirs $(LIBFILE) .SYMBOLIC

dirs : .SYMBOLIC
    @if not exist $(OUTDIR) mkdir $(OUTDIR)
    @if not exist $(OBJDIR) mkdir $(OBJDIR)
    @if not exist $(LIBDIR) mkdir $(LIBDIR)

.cpp.obj : .AUTODEPEND
    $(CC) $(CFLAGS) -fr=$(OBJDIR)\$^&.err -fo=$(OBJDIR)\$^. $[@

.asm.obj : .AUTODEPEND
    $(ASM) $(AFLAGS) -fr=$(OBJDIR)\$^&.err -fo=$(OBJDIR)\$^. $[@

# genform.cpp is compiled twice, once per forms definition header.
genparts.obj : $(EXDIR)\tvforms\genform.cpp .AUTODEPEND
    $(CC) $(CFLAGS) -DPARTS -fr=$(OBJDIR)\$^&.err -fo=$(OBJDIR)\$^. $[@

genphone.obj : $(EXDIR)\tvforms\genform.cpp .AUTODEPEND
    $(CC) $(CFLAGS) -DPHONENUM -fr=$(OBJDIR)\$^&.err -fo=$(OBJDIR)\$^. $[@

$(LIBFILE) : $(OBJS)
    @%create $(OBJDIR)\tv.lbc
    @for %i in ($<) do @%append $(OBJDIR)\tv.lbc +%i
    $(LIB) -q -b -n $^@ @$(OBJDIR)\tv.lbc

# Each example links its own objects against the library. The response file
# lists the objects because they live in $(OBJDIR), and a 'file' line is
# clearer than putting the directory on the wlink command line.

tvdemo : $(OUTDIR)\tvdemo.exe .SYMBOLIC
tvedit : $(OUTDIR)\tvedit.exe .SYMBOLIC
tvhc   : $(OUTDIR)\tvhc.exe   .SYMBOLIC
tvdir  : $(OUTDIR)\tvdir.exe  .SYMBOLIC
tvforms : $(OUTDIR)\tvforms.exe $(OUTDIR)\genparts.exe $(OUTDIR)\genphone.exe .SYMBOLIC

$(OUTDIR)\tvdemo.exe : $(TVDEMO_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVDEMO_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\tvedit.exe : $(TVEDIT_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVEDIT_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\tvhc.exe : $(TVHC_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVHC_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\tvdir.exe : $(TVDIR_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVDIR_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\tvforms.exe : $(TVFORMS_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVFORMS_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\genparts.exe : $(GENPARTS_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(GENPARTS_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

$(OUTDIR)\genphone.exe : $(GENPHONE_OBJS) $(LIBFILE)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(GENPHONE_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE) $(XLIBS)

# The generators write their data file to the current directory, so run
# them from the output directory, next to tvforms.exe.
forms : $(OUTDIR)\parts.f32 $(OUTDIR)\phonenum.f32 .SYMBOLIC

$(OUTDIR)\parts.f32 : $(OUTDIR)\genparts.exe
    cd $(OUTDIR)
    .\genparts.exe
    cd ..

$(OUTDIR)\phonenum.f32 : $(OUTDIR)\genphone.exe
    cd $(OUTDIR)
    .\genphone.exe
    cd ..

clean : .SYMBOLIC
    @if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    @if exist $(OBJDIR)\*.err del $(OBJDIR)\*.err
    @if exist $(OBJDIR)\*.lbc del $(OBJDIR)\*.lbc
    @if exist $(OUTDIR)\*.exe del $(OUTDIR)\*.exe
    @if exist $(OUTDIR)\*.f16 del $(OUTDIR)\*.f16
    @if exist $(OUTDIR)\*.f32 del $(OUTDIR)\*.f32
    @if exist $(OBJDIR)\*.lnk del $(OBJDIR)\*.lnk
    @if exist $(LIBFILE) del $(LIBFILE)
