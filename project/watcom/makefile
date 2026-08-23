#
#   Open Watcom makefile for Turbo Vision (wmake)
#
#   Builds the library and the example programs for one target:
#
#       wmake -h TARGET=dos16     16-bit real mode DOS, large model   (tvw16.lib)
#       wmake -h TARGET=dos32     32-bit DOS extender, flat model     (tvw32.lib)
#
#   There is no Win32 target, because none is needed: the CMake build (MSVC,
#   MinGW) already covers the Win32 console.
#
#   TARGET defaults to dos16. Run from this directory. Libraries go to
#   ..\..\LIB, object files and executables to .\<TARGET>. Other macros:
#
#       DEBUG=1     full debug information (-d2 / -d1, wlink debug all) and
#                   no optimization; without it the build is optimized
#       EXAMPLES=   override the list of example programs
#                   (default: tvdemo tvedit tvhc tvdir tvforms)
#       EXTENDER=   dos32 only: which DOS extender the examples are linked
#                   for. dos4g, the default, needs dos4gw.exe from Watcom's
#                   binw directory beside the program or on PATH when it
#                   runs; causeway puts the extender inside the .exe, so the
#                   program runs on its own. Both come with Open Watcom. The
#                   library is the same either way, so this only affects
#                   linking. The CauseWay build has never been seen to run -
#                   read project\watcom\README.md before using it.
#
#   tvforms reads two data files, written by the genparts and genphone
#   programs. Those can only run under the target they were built for, so the
#   32-bit files are checked in as examples\tvforms\parts.f32 and
#   phonenum.f32, and the 'forms' target (part of 'all' for dos32) copies
#   them next to tvforms.exe. There are no checked-in .f16 files: for dos16,
#   run .\dos16\genparts.exe and genphone.exe under DOS to produce them.
#
#   tvdemo opens its help file (F1) from the current directory, so the 'help'
#   target copies the checked-in examples\tvdemo\demohelp.h16 (dos16) or
#   demohelp.h32 (dos32) next to tvdemo.exe. Those files are not regenerated
#   here; they come with the repository, along with demohelp.h, which holds
#   the matching help context numbers.
#
#   Open Watcom's binnt (or binw) directory must be on PATH: wlink looks
#   there, and nowhere else, for the file describing the systems it can link
#   for (wlsystem.lnk). WATCOM must be set to the installation directory,
#   because the run-time library headers are taken from there rather than
#   from INCLUDE. That way the makefile does not depend on how the
#   environment was set up.
#
#   Notes:
#
#   - -xr, which enables run-time type information, is required for every
#     file: tobjstrm.cpp uses dynamic_cast<void *>, which Watcom rejects
#     without it.
#   - tvw16.lib is built for the large memory model (-ml), and applications
#     must be compiled -ml to link against it. The memory model decides
#     pointer sizes and how the library reaches its own data, and wlink
#     reports a mismatch only as unresolved symbols, or as symbols that
#     resolve to the wrong thing. There is no small, medium or compact
#     build: the library uses far pointers for data throughout.
#   - wcstubs.asm is the only file assembled, and only for the 16-bit target.
#     None of Borland's own .asm files is: each one either has a C++
#     equivalent that is compiled instead, or its routines are in wcstubs.asm
#     under extern "C" names, which is what hardware.asm and swapst.asm
#     needed. Borland's .asm files and tv.inc are therefore untouched by this
#     port, and nothing here generates TVWRITE.INC with the GENINC tool.
#   - geninc.cpp is the Borland GENINC tool, not part of the library.
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
EXTSTAMP =
LNKOPTS = option stack=8k
AOBJS   = wcstubs.obj
FORMS   =
HELPEXT = h16
!else ifeq TARGET dos32
!ifndef EXTENDER
EXTENDER = dos4g
!endif
!ifneq EXTENDER dos4g
!ifneq EXTENDER causeway
!error EXTENDER must be dos4g or causeway
!endif
!endif
CC      = wpp386
ASM     = wasm
SYSTEM  = $(EXTENDER)
LIBNAME = tvw32.lib
CMODEL  = -bt=dos -mf
AMODEL  = -3 -mf
EXTSTAMP = $(OBJDIR)\$(EXTENDER).ext
LNKOPTS = option stack=32k
AOBJS   =
FORMS   = forms
HELPEXT = h32
!else
!error TARGET must be dos16 or dos32
!endif

!ifndef %WATCOM
!error WATCOM must be set to the Open Watcom installation directory
!endif

# -obmiler -ot is the usual release setting -ox, without -s so that stack
# overflow checking stays on, and without -oa because TScreenCell reads
# itself through a ushort pointer, which relaxed aliasing would break.
!ifdef DEBUG
CDEBUG  = -d2 -od
ADEBUG  = -d1
LDEBUG  = debug all
!else
CDEBUG  = -obmiler -ot
ADEBUG  =
LDEBUG  =
!endif

LIB     = wlib
LINK    = wlink
LIBFILE = $(LIBDIR)\$(LIBNAME)

CFLAGS  = $(CMODEL) -zq -xr -w1 $(CDEBUG) -i=$(TVINC) -i=$(%WATCOM)\h
AFLAGS  = $(AMODEL) -q -zq $(ADEBUG)

# --------------------------------------------------------------- path search

.EXTENSIONS:
.EXTENSIONS: .exe .lib .obj .cpp .asm

.cpp : $(TVSRC);$(EXDIR)\tvdemo;$(EXDIR)\tvedit;$(EXDIR)\tvhc;$(EXDIR)\tvdir;$(EXDIR)\tvforms
.asm : $(TVSRC)
.obj : $(OBJDIR)
.exe : $(OUTDIR)

# ------------------------------------------------------------------ objects

# C++ versions of the Borland .asm files.
TOBJS1 = edits.obj framelin.obj tgrmv.obj ttprvlns.obj tvcursor.obj &
         tvexposd.obj tvwrite.obj

# Watcom-specific platform files: the DOS/4G hardware driver, the real mode
# one, the 16-bit interrupt layer, findfirst/findnext. Each compiles to an
# empty object on the target it does not apply to. Code belongs here, rather
# than in the Turbo Vision source that calls it, so that those files stay as
# close to upstream as they can.
TOBJS2 = wchwdos.obj wchw16.obj wcsysint.obj wcdir.obj

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

all : dirs $(LIBFILE) $(EXAMPLES) help $(FORMS) .SYMBOLIC

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

$(OUTDIR)\tvdemo.exe : $(TVDEMO_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVDEMO_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\tvedit.exe : $(TVEDIT_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVEDIT_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\tvhc.exe : $(TVHC_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVHC_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\tvdir.exe : $(TVDIR_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVDIR_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\tvforms.exe : $(TVFORMS_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(TVFORMS_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\genparts.exe : $(GENPARTS_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(GENPARTS_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

$(OUTDIR)\genphone.exe : $(GENPHONE_OBJS) $(LIBFILE) $(EXTSTAMP)
    @%create $(OBJDIR)\$^&.lnk
    @for %i in ($(GENPHONE_OBJS)) do @%append $(OBJDIR)\$^&.lnk file $(OBJDIR)\%i
    $(LINK) system $(SYSTEM) option quiet $(LNKOPTS) $(LDEBUG) name $^@ @$(OBJDIR)\$^&.lnk library $(LIBFILE)

!ifeq TARGET dos32
# Switching EXTENDER changes no object file, so wmake would otherwise think
# the existing executables were up to date. The examples depend on a marker
# file named after the chosen extender, so switching asks for a file that
# does not exist yet; creating it deletes the previous one.
$(EXTSTAMP) :
    @if exist $(OBJDIR)\*.ext del $(OBJDIR)\*.ext
    @%create $^@
!endif

# tvforms reads its data files from the current directory. They are checked in
# rather than generated here, because only genparts and genphone can write
# them and those run under DOS. dos16 has no checked-in copies.
forms : $(OUTDIR)\parts.f32 $(OUTDIR)\phonenum.f32 .SYMBOLIC

$(OUTDIR)\parts.f32 : $(EXDIR)\tvforms\parts.f32
    copy $[@ $^@ >nul

$(OUTDIR)\phonenum.f32 : $(EXDIR)\tvforms\phonenum.f32
    copy $[@ $^@ >nul

# tvdemo looks for its help file in the current directory.
help : $(OUTDIR)\demohelp.$(HELPEXT) .SYMBOLIC

$(OUTDIR)\demohelp.$(HELPEXT) : $(EXDIR)\tvdemo\demohelp.$(HELPEXT)
    copy $[@ $^@ >nul

clean : .SYMBOLIC
    @if exist $(OBJDIR)\*.obj del $(OBJDIR)\*.obj
    @if exist $(OBJDIR)\*.err del $(OBJDIR)\*.err
    @if exist $(OBJDIR)\*.lbc del $(OBJDIR)\*.lbc
    @if exist $(OUTDIR)\*.exe del $(OUTDIR)\*.exe
    @if exist $(OUTDIR)\*.f16 del $(OUTDIR)\*.f16
    @if exist $(OUTDIR)\*.f32 del $(OUTDIR)\*.f32
    @if exist $(OUTDIR)\demohelp.* del $(OUTDIR)\demohelp.*
    @if exist $(OBJDIR)\*.lnk del $(OBJDIR)\*.lnk
    @if exist $(OBJDIR)\*.ext del $(OBJDIR)\*.ext
    @if exist $(LIBFILE) del $(LIBFILE)
