# Turbo Vision with Open Watcom 1.9

This directory holds the `wmake` makefile that builds Turbo Vision and its
example programs with Open Watcom 1.9, for three targets:

| `TARGET` | Environment | Library |
| --- | --- | --- |
| `dos16` | 16-bit real mode DOS, large model | `LIB\tvw16.lib` |
| `dos32` | 32-bit DOS/4G extender, flat model | `LIB\tvw32.lib` |
| `win32` | 32-bit Win32 console, flat model | `LIB\tvwnt.lib` |

```
wmake -h TARGET=dos16
```

The [root README](../../README.md#build-watcom) describes how to use these
builds; the head of [`makefile`](makefile) documents the options, the flags each
target needs and the constraints behind them. This file records what has been
seen to work, and what has not.

## Verification status

Last checked 2026-08-22 at commit `a379099`. All three targets rebuild from
clean with no errors (one pre-existing warning, W446 in
`include/tvision/tspan.h`), producing the library plus `tvdemo`, `tvedit`,
`tvhc`, `tvdir` and `tvforms`.

Runtime behaviour has been exercised on the two DOS targets under FreeDOS 1.4
in QEMU — `dos32` always with its default DOS/4G extender, never with
`EXTENDER=causeway`, which is discussed under the limitations below: `tvdemo`'s menus, shadows, dialogs, file dialog, event viewer, mouse
input, DOS shell and INT 24H critical-error prompt; and every `tvedit`
status-line key — F1 help, F2 save, F3 open, F5 zoom, F6 next, F10 menu,
Ctrl-W and Alt-F3 close, Alt-X exit, the Ctrl/Shift Ins and Del clipboard
keys, Ctrl-Backspace, Ctrl-Y and the WordStar Ctrl-K chords.

On Win32 only `tvhc` has been checked, by comparing its output against the
checked-in `demohelp.h32` byte for byte.

## Known limitations

These affect anyone using the builds, and are not expected to change soon.

* **No `.f16` forms data for `dos16`.** `tvforms` needs `parts.f16` and
  `phonenum.f16`, which only `dos16\genparts.exe` and `dos16\genphone.exe` can
  write, and those have to be run under DOS. The `win32` build runs its own
  copies to produce `parts.f32` and `phonenum.f32`, which serve `dos32` as
  well, since both are `__FLAT__` targets.
* **No single-floppy drive-swap prompt.** Borland's `SYSINT.ASM` hooked INT 21H
  to prompt for a disk swap before DOS's own "Insert diskette for drive B:"
  message could scribble over the screen. `wcsysint.cpp` does not reimplement
  it; it only applies to a machine with exactly one floppy drive and no hard
  disk.
* **Keypad `Shift`+Ins/Del differ between the DOS targets.** With NumLock off
  the BIOS queues `'0'` or `'.'` for a shifted keypad key. The `dos16` INT 09H
  hook converts those back to `kbShiftIns` / `kbShiftDel`; the DOS/4G driver
  does not, and the character is typed instead. Borland's 16-bit and 32-bit
  drivers behaved the same way — this is not a port artifact. The grey Insert
  and Delete keys work on both.
* **The CauseWay build has never been seen to run.** `EXTENDER=causeway`
  links cleanly and produces a genuinely self-contained executable — the
  extender is bound into the file by `cwstub.exe`, where the DOS/4G build
  needs `dos4gw.exe` beside it or on `PATH`. But CauseWay 4.04, the version
  bundled with Open Watcom 1.9, faults at startup under FreeDOS 1.4 in QEMU:
  `Exception: 0D, Error code: 1284`, followed by `CauseWay error 09 :
  Unrecoverable exception`. This is not a Turbo Vision problem — a five-line
  program whose whole body is `printf("hello")`, compiled with `wcl386
  -l=causeway`, fails in exactly the same way and at the same address. It was
  reproduced with JEMMEX loaded (the image's default boot), with
  `CAUSEWAY=NOVCPI` set, with no memory manager at all (boot menu option 5,
  which changes CauseWay's info flags from `8026` to `8042` and so does take a
  different path), and under `-cpu 486` and `-cpu pentium`. Whether it is QEMU,
  FreeDOS 1.4 or the extender itself is unknown; the option is offered because
  it costs nothing and CauseWay is free software, whereas DOS/4GW is
  Tenberry's and redistributable only under the terms in Watcom's
  `binw\dos4gw.doc`. Do not ship a CauseWay build without testing it.
* **`tvw16.lib` is large-model only.** The 16-bit memory model is part of the
  ABI — it decides pointer sizes and how the library reaches its own data —
  so an application must be compiled `-ml` to link against it. `wlink`
  diagnoses a mismatch only indirectly, as unresolved symbols or, worse, as
  symbols that resolve to the wrong thing, so a program built `-mm` may link
  and then misbehave. There is no small, medium or compact build; the port
  addresses far data throughout, and `wcstubs.asm` assumes `SS`-relative
  `DGROUP` addressing. (`tvw32.lib` and `tvwnt.lib` are flat-model `-mf`,
  where the question does not arise.)
* **Nothing has run on real hardware.** Every runtime observation above comes
  from QEMU. In particular, no timing measurement is meaningful yet, so
  whether the C++ replacements for the original assembly cost anything
  noticeable is unknown.

## Open work

1. **The CI job has never run**, because Actions is dormant on the fork.
   `.github/workflows/cmake.yml` defines a `build-watcom` job on
   `windows-latest` using `open-watcom/setup-watcom@v1`, and the branch
   carrying it is pushed, but `pgalbraith/tvision` has *zero* workflow runs on
   any branch since the fork was created — nine pushes, no runs, and no
   `github-actions` check suite on any commit (other apps do create theirs).
   GitHub disables workflows in a forked repository until the owner opens the
   fork's Actions tab once and confirms "I understand my workflows, go ahead
   and enable them"; the REST API reports `"enabled": true` regardless, since
   that flag is the repository setting rather than the fork acknowledgement.
   Nothing suggests the job itself is wrong — it has simply never been given a
   chance to fail. Until it runs, the build is only known to work on a machine
   with a hand-installed `C:\WATCOM`, and the artifact list is unproven.
2. **The Win32 build has never been driven interactively.** `wchwnt.cpp` is
   the least exercised file in the port: it compiles and links, and `tvhc`
   (which does not open a console UI) is correct, but no one has watched
   `tvdemo` or `tvedit` run in a Windows console.
3. **Hand-written WASM (phase 05) remains optional.** Seven `.asm` files have
   portable C++ twins that are compiled instead; only `hardware.asm`,
   `swapst.asm` and `wcstubs.asm` are assembled, and only for `dos16`.
   Restoring the assembly would also mean restoring the `TVWRITE.INC` include
   in `tv.inc` and the GENINC step that generates it. This is worth doing only
   if a measurement on real hardware shows a regression — see the last
   limitation above.
