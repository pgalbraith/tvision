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

Last checked on 2026-08-22. Both targets rebuild from scratch without errors,
producing the library plus `tvdemo`, `tvedit`, `tvhc`, `tvdir` and `tvforms`.

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

3. **Some routines could be written in assembly again, but need not be.**
   Seven of the original `.asm` files have C++ equivalents, and those are
   compiled instead. Only `hardware.asm`, `swapst.asm` and `wcstubs.asm` are
   assembled, and only for `dos16`. Going back to assembly would also mean
   restoring the `TVWRITE.INC` include in `tv.inc`, and the step that
   generates that file. This is only worth doing if a measurement on real
   hardware shows the C++ versions are slower — see the note about real
   hardware above.
