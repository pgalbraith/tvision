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

; TVWRITE.INC and TVWRIT32.INC hold the class member offsets that TVWRITE.ASM,
; TVCURSOR.ASM and the rest index with, and GENINC.EXE produces them. The Open
; Watcom build needs neither: the three files it assembles (HARDWARE.ASM,
; SWAPST.ASM, WCSTUBS.ASM) use no member offsets, because every file that does
; is compiled from C++ instead.

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
    ; WASM has no equivalent of TASM's PROLOG directive, so the 16-bit .asm
    ; files write their own prologue and epilogue under __WASM__.
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

