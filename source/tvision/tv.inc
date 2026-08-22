;/*------------------------------------------------------------*/
;/* filename -       tv.inc                                    */
;/*                                                            */
;/*                  Header for Turbo Vision .asm files        */
;/*------------------------------------------------------------*/

;
;       Turbo Vision - Version 2.0
; 
;       Copyright (c) 1994 by Borland International
;       All Rights Reserved.
; 

; TVWRITE.INC / TVWRIT32.INC hold the class member offsets that the .asm
; translations of TVWRITE.ASM, TVCURSOR.ASM and friends index with. They are
; produced by running GENINC.EXE, which the Borland makefile builds for the
; target first. The Open Watcom build has no equivalent step, and needs none:
; the only .asm files it assembles (HARDWARE.ASM, SWAPST.ASM, WCSTUBS.ASM)
; use no member offsets, since every file that does is compiled from its C++
; twin instead.

ifdef __FLAT__ ;;;;;;;;;;;;;;;;;;;;;;;; 32-bit ;;;;;;;;;;;;;;;;;;;;;;;

ifdef __WASM__
    .MODEL FLAT, C
else
    P386
    MODEL  FLAT, C
endif

        ARGINT   equ DWORD
    Dnear    equ DD
    LocalRet equ db 0C3h
    _FAR   equ


ifndef __WASM__
    INCLUDE TVWRIT32.INC
endif


else    ;;;;;;;;;;;;;;;;;;;;;;;; 16-bit ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifdef __WASM__
    ; Open Watcom's assembler doesn't support TASM's PROLOG auto-generation
    ; extension, so procedures in the 16-bit .asm files spell out their own
    ; prologue/epilogue explicitly under __WASM__ (see USES/RET usage below).
    .MODEL LARGE, C
else
    MODEL  LARGE, PROLOG
endif

    ARGINT equ WORD
    Dnear  equ DW
    _FAR   equ FAR

ifndef __WASM__
    INCLUDE TVWRITE.INC
endif

endif
; WASM treats '@@' labels as scoped/local automatically; TASM needs LOCALS.
ifndef __WASM__
    LOCALS @@
endif

