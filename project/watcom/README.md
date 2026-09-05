# Turbo Vision with Open Watcom 1.9

This directory holds the `wmake` makefile that builds Turbo Vision and its
example programs with Open Watcom 1.9, for two targets:

| `TARGET` | Environment | Library |
| --- | --- | --- |
| `dos16` | 16-bit real mode DOS, large model | `LIB\tvw16.lib` |
| `dos32` | 32-bit DOS/4G extender, flat model | `LIB\tvw32.lib` |

```
wmake -h TARGET=dos16
```

The [root README](../../README.md#build-watcom) explains how to use these
builds, and the comments at the top of [`makefile`](makefile) explain the
options and the compiler flags each target needs. This file records what has
been tested and what has not.

## Why this build exists

This build lets Turbo Vision be built for DOS from a modern development
environment. Borland C++ can target DOS as well, and this repository still
supports it, but it needs a 1990s toolchain: its installer is a 16-bit program
that will not run on 64-bit Windows, and the build normally wants Turbo
Assembler alongside it. Open Watcom 1.9 installs and runs on current Windows,
and the build here can be run from a script or a CI job.

The 32-bit target was not a recompile of the existing 32-bit code. Turbo
Vision's 32-bit code path talks to the Win32 console API, because the only
32-bit DOS compiler it originally supported was Borland C++ with the DOS
PowerPack, which provided that API on DOS. A plain DOS/4G program has nothing
of the kind, so `wchwdos.cpp` and `wcsysint.cpp` handle the screen, keyboard,
mouse and interrupts instead.

There is no Win32 target, because none is needed. MSVC and MinGW already build
Turbo Vision for the Win32 console through CMake, from the same modern
environment, and with Unicode, UTF-8 console support and 24-bit color as well.
Use that build for Win32.

## What has been tested

Last checked on 2026-09-05. Both targets rebuild from scratch without errors,
producing the library plus `tvdemo`, `tvedit`, `tvhc`, `tvdir` and `tvforms`.

On 2026-09-05 a merge from `master` (commit `292dee7`) brought in three
`tvdemo`/`tvhc` commits: a fix for `TCalendarView`'s leap-year formula and its
next/previous-month mouse arrows (`examples/tvdemo/calendar.cpp`), removal of
two redundant `virtual` qualifiers (`examples/tvdemo/gadgets.h`), and
typo fixes to the help text, including regenerated `demohelp.h16`/`.h32`
binaries that the makefile copies as-is rather than rebuilding. Both targets
were rebuilt afterward and re-run under FreeDOS 1.4 in QEMU. The Calendar's
`+`/`-` keyboard navigation was checked on `dos16`. The arrow icons were
checked on both targets by clicking the up and down triangles with a real
mouse click delivered through QEMU's QMP socket: the up arrow moves to the
previous month and the down arrow to the next month, on both `dos16` and
`dos32`, matching the fixed source rather than the reversed behavior it
replaced. The default PS/2 mouse emulation did not deliver clicks reliably
enough for this; the clicks were driven through a `virtio-mouse-pci` device
with `vio.exe`/`viomouse.exe` from the `virtio-dos` project loaded resident
in the guest instead.

The build is not free of warnings. Three appear, and all three come from code
shared with the other builds rather than from the files written for Open
Watcom:

* W446, from `include/tvision/tspan.h`. That header is included nearly
  everywhere, so the warning appears in almost every source file, on both
  targets.
* W728, from `include/tvision/textview.h`, on both targets.
* W388, in `tvdemo`'s `ascii.cpp`, `calendar.cpp` and `puzzle.cpp`, on their
  `flags &= ~(...)` lines. This one appears on `dos16` only.

Both targets have also been run under FreeDOS 1.4 in QEMU. For `dos32` that
always means the default DOS/4G extender; the CauseWay alternative is covered
under the limitations below. What was exercised:

* In `tvdemo`: menus, shadows, dialogs, the file dialog, the event viewer,
  mouse input, the DOS shell, and the critical-error prompt that INT 24H puts
  up when a drive is not ready.
* In `tvedit`: every key on the status line — F1 help, F2 save, F3 open, F5
  zoom, F6 next window, F10 menu, Ctrl-W and Alt-F3 to close, Alt-X to exit —
  along with the Ctrl and Shift combinations of Insert and Delete used for
  the clipboard, Ctrl-Backspace, Ctrl-Y, and the WordStar Ctrl-K key
  sequences.

`tvforms` is the exception. It builds for both targets, and `dos32` gets its
data files, but nobody has opened a form with it.

## Known limitations

These affect anyone using the builds and are not expected to change soon.

* **`dos16` has no forms data files.** `tvforms` reads `parts.f16` and
  `phonenum.f16`. Only `dos16\genparts.exe` and `dos16\genphone.exe` can write
  them, and running those means running them under DOS. The 32-bit equivalents
  are checked in as `examples\tvforms\parts.f32` and `phonenum.f32`, and the
  `dos32` build copies them next to `tvforms.exe`. The 16-bit files cannot be
  produced on the machine doing the build, so there are no checked-in copies
  of those.

* **No prompt to swap disks on a single-floppy machine.** Borland's
  `SYSINT.ASM` hooked INT 21H so that Turbo Vision could ask for the disk to be
  changed before DOS printed its own "Insert diskette for drive B:" message
  over the display. `wcsysint.cpp` does not do this. It only matters on a
  machine with one floppy drive and no hard disk.

* **`tvdemo`'s heap indicator stays blank.** The function behind it,
  `THeapView::heapSize()` in `examples/tvdemo/gadgets.cpp`, only measures the
  heap when compiled with Borland C++, where it calls `farcoreleft`,
  `heapcheck` and `farheapwalk`. Its other branches are for Glibc and for
  Windows, and neither applies when Open Watcom targets DOS, so both targets
  end up in the fallback branch, which reports nothing. The indicator is still
  displayed; it is simply empty, and never changes. Open Watcom does provide
  equivalent functions — `_heapchk`, `_heapwalk`, `_fheapwalk` and `_memavl`,
  in `<malloc.h>` — so this could be implemented. Nobody has.

* **The keypad Shift+Insert and Shift+Delete keys behave differently on the
  two targets.** With NumLock off, the BIOS reports `'0'` or `'.'` for a
  shifted keypad key. The `dos16` keyboard handler turns those back into
  `kbShiftIns` and `kbShiftDel`; the DOS/4G driver does not, so the character
  is typed instead. Borland's own 16-bit and 32-bit drivers differed in the
  same way, so this is not something this port introduced. The dedicated
  Insert and Delete keys, rather than the keypad ones, work on both targets.

* **The CauseWay extender has never been seen to run.** Building with
  `EXTENDER=causeway` succeeds, and produces a program with the extender
  inside the executable, so it needs no other file at run time — where a
  DOS/4G program needs `dos4gw.exe` beside it or on `PATH`. But CauseWay 4.04,
  the version that comes with Open Watcom 1.9, fails as soon as it starts
  under FreeDOS 1.4 in QEMU, printing `Exception: 0D, Error code: 1284` and
  then `CauseWay error 09 : Unrecoverable exception`.

  This is not a Turbo Vision problem. A five-line program whose whole body is
  `printf("hello")`, compiled with `wcl386 -l=causeway`, fails in the same way
  and at the same address. The failure was reproduced with the JEMMEX memory
  manager loaded, which is how the FreeDOS image boots by default; with
  `CAUSEWAY=NOVCPI` set; with no memory manager at all, using boot menu option
  5, which does change what CauseWay does, since the information flags it
  reports go from `8026` to `8042`; and with QEMU emulating a 486 and then a
  Pentium. Whether the fault lies with QEMU, with FreeDOS 1.4 or with the
  extender itself is not known.

  The option is offered anyway, because it costs nothing to provide and
  CauseWay is free software, whereas DOS/4GW belongs to Tenberry and may only
  be redistributed under the terms set out in Watcom's `binw\dos4gw.doc`. Do
  not ship a CauseWay build without testing it first.

* **`tvw16.lib` works only with the large memory model.** On 16-bit DOS the
  memory model is part of the binary interface between a program and the
  library: it determines how large pointers are and how the library reaches
  its own data. An application must therefore be compiled with `-ml` to link
  against this library. `wlink` does not report a mismatch directly. It shows
  up as unresolved symbols or, worse, as symbols that resolve to the wrong
  thing, so a program built with `-mm` may link and then misbehave. There is
  no small, medium or compact build: the port uses far pointers for data
  throughout, and `wcstubs.asm` assumes the data segment is addressed through
  `SS`. None of this applies to `tvw32.lib`, which uses the flat model.

* **Nothing has been run on real hardware.** Every runtime result above comes
  from QEMU. Timings measured there mean nothing, so it is not known whether
  the C++ replacements for the original assembly routines cost anything
  noticeable in practice.

## Open work

1. **The continuous integration job has never run.** The file
   `.github/workflows/cmake.yml` defines a `build-watcom` job for
   `windows-latest` that installs Open Watcom 1.9 using
   `open-watcom/setup-watcom@v1`, and the branch containing it has
   been pushed. But `pgalbraith/tvision` shows no workflow runs at all, on any
   branch, since it was created: nine pushes, no runs, and no `github-actions`
   entry among the checks on any commit. Other integrations do add their own
   check entries, so the repository is not refusing checks in general.

   The reason is that GitHub disables workflows in a forked repository until
   the owner opens the fork's Actions tab and confirms "I understand my
   workflows, go ahead and enable them". The REST API is misleading here,
   reporting the workflow as enabled either way, because that setting belongs
   to the repository and is separate from the confirmation a fork needs.

   Nothing suggests the job itself is wrong; it has simply never had the
   chance to run. Until it does, the build is only known to work on a machine
   where Open Watcom was installed by hand at `C:\WATCOM`, and the list of
   files the job uploads has never been checked.

2. **`tvforms` has never been run.** It builds for both targets, and `dos32`
   gets its data files, but nobody has opened a form with it. That is the only
   way to find out whether the checked-in `.f32` files are correct.

3. **Some routines could be written in assembly again, but should not be
   without a reason.** Seven of the original `.asm` files have C++ equivalents,
   and those are compiled instead. No file of Borland's is assembled at all:
   `wcstubs.asm` is the only one, and only for `dos16`. This is only worth
   revisiting if a measurement on real hardware shows the C++ versions are
   slower — see the note about real hardware above. The section below records
   what was learned when it was tried.

## Assembling Borland's original .asm files

None of Borland's `.asm` files is assembled by this build, and none of them is
modified by it. `wcstubs.asm` is the only assembly the build uses. Two of the
routines in it, `THardwareInfo`'s constructor group and
`TSystemError::swapStatusLine`, are copies of routines in `hardware.asm` and
`swapst.asm`, under `extern "C"` names instead of the Borland-mangled ones
`wasm` cannot reproduce for a Watcom build. They were kept inside Borland's
files at first, in `IFDEF __WASM__` blocks, and moved out on 2026-08-22: that
removed 190 lines of change from `hardware.asm`, `swapst.asm` and `tv.inc`,
which are now identical to upstream, at the cost of nothing but a longer
`wcstubs.asm`. The generated code was compared instruction by instruction
before and after the move, and is the same.

Borland's `.asm` files can be assembled by `wasm` and linked into `tvw16.lib`,
with their instructions unchanged. This was done for `framelin.asm` on
2026-08-22 and then reverted. It is written down here so that the question does
not have to be answered twice.

The result was correct. `tvdemo` with four overlapping windows rendered
pixel-for-pixel identically to the build using `framelin.cpp`, which exercises
the frame-joint logic — the part of the routine that decides which line
character belongs at each place two framed windows meet.

It was reverted because it made the port diverge further from upstream, not
less. The 16-bit build was compiling `framelin.cpp` exactly as upstream ships
it, with no changes at all. Assembling `framelin.asm` instead cost 102 added
lines across four files upstream already owns — `framelin.asm`, `framelin.cpp`,
`tv.inc` and `tvtext1.cpp` — plus two new files. The `tvtext1.cpp` change was
the worst of them: it moved the storage of `TFrame::initFrame` and
`TFrame::frameChars` into `extern "C"` objects, which left both members
declared in `views.h` but undefined in this build. Every one of those edits
would have to be carried through each future merge from upstream.

Four things stand between a Borland `.asm` file and this build. None of them is
a reason to give up, and the third is the one that costs real time.

* **TASM's `PROLOG`, `ARG` and `USES`.** A compatibility layer in `TV.INC`
  cannot be written: `wasm` 1.9 has neither `:VARARG` macro parameters nor
  `INSTR`/`SUBSTR`, so a macro cannot take `ARG name:type, ...` apart. What
  works is giving each routine its own header under `IFDEF __WASM__`, in the
  form `NAME PROC FAR SYSCALL USES DS SI DI, thisPtr:DWORD, Y:WORD, ...`, and
  wrapping the `ARG` and `USES` lines in `IFNDEF __WASM__`. `wasm` then builds
  the same frame TASM's `PROLOG` builds, with arguments from `[BP+6]` upward.
  `SYSCALL` is the C calling convention without the leading underscore, which
  is what allows the Borland-mangled name on the `PUBLIC` line to stay as
  written. `CODESEG` and `DATASEG` can be defined as macros for `.CODE` and
  `.DATA`. There are 40 routines across the seven files, 20 of which take
  arguments.

* **Names.** `wasm` accepts `@TFrame@frameLine$qm11TDrawBufferssuc` as a symbol
  without complaint, but cannot write Watcom's name for the same member,
  `W?frameLine$:TFrame$f(rf$TDrawBuffer$$ssuc)v`, nor `W?initFrame$:TFrame$n[]xa`
  for a static data member. Nothing in the assembly has to be renamed for this.
  Declare the C++ side `extern "C"` and rename it to Borland's spelling with
  `#pragma aux`:

  ```cpp
  extern "C" void tvFrameLine( TFrame *, TDrawBuffer *, short, short, TColorAttr );
  #pragma aux tvFrameLine "@TFrame@frameLine$qm11TDrawBufferssuc" \
                          parm caller [] modify [ax bx cx dx es];
  ```

  `parm caller []` puts every argument on the stack and makes the caller clean
  up, which is what the assembly expects. Do not write `__cdecl` as well: it
  conflicts with the pragma's naming and Watcom quietly keeps `_tvFrameLine`.
  The same pragma works on a variable, and renames its definition, but it
  cannot be applied to a C++ static data member, which is why the storage has
  to move out of the class.

* **`DS` is not `DGROUP`.** Borland's large model keeps `DS` on `DGROUP`, and
  the assembly relies on it: near data such as `initFrame`, `frameChars` and a
  routine's own scratch buffers are all read through `DS`. Watcom's large model
  lets `DS` float — `-zdf` is its default — so on entry `DS` points somewhere
  else and every one of those reads returns the wrong bytes. Each ported
  routine has to load `DGROUP` itself:

  ```asm
          MOV     AX, SEG DGROUP
          MOV     DS, AX
  ```

  with `DS` added to the `USES` list so the generated epilogue restores it.
  Arguments are unaffected, being reached through `SS:[BP]`.

  The symptom is worth knowing, because it does not look like a segment fault.
  A scratch buffer is written and read back through the same wrong `DS`, so it
  stays self-consistent; only the reads of real data go wrong. In
  `framelin.asm` that produced frames drawn in the correct colour with blank
  characters, and no crash.

  Compiling the library with `-zdp`, which pegs `DS` to `DGROUP`, would remove
  the need for this and leave the assembly completely untouched. It cannot be
  used: Open Watcom 1.9 fails with internal compiler error 40 on
  `tdirlist.cpp` when `-zdp` is combined with `-ol` and `-oe`, both of which
  are in the `-obmiler` release setting. Dropping `-oe` avoids the error.
  Pegging `DS` would also become part of the library's binary interface, in the
  same way the large memory model already is, so every application linking
  against `tvw16.lib` would have to be compiled with `-zdp` too.

* **Class member offsets.** The files that index class members need the
  equates that `GENINC.EXE` produces in `TVWRITE.INC` for the Borland build.
  Watcom lays the classes out differently, so it needs its own copy. This does
  not require running anything under DOS. Put each `offsetof()` into a static
  array inside a function named `genRefs()` — that name matters, because it is
  what the class headers declare as a friend, and several of the members are
  protected — compile it with `wpp -bt=dos -ml`, and read the values back out
  of the object file with `wdis -a`. The offsets should then be re-checked
  against the headers at compile time, so that a class change breaks the build
  instead of silently moving what the assembly reads.

`sysint.asm` is not like the other six. It has 12 routines and no `ARG` at all,
and it overlaps `wcsysint.cpp` rather than having a straight C++ equivalent, so
it would need looking at on its own terms.
