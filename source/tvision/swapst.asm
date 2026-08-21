;/*------------------------------------------------------------*/
;/* filename -           swapst.asm                            */
;/*                                                            */
;/* function(s)                                                */
;/*                      TSystemError swapStatusLine function  */
;/*------------------------------------------------------------*/

;
;       Turbo Vision - Version 2.0
; 
;       Copyright (c) 1994 by Borland International
;       All Rights Reserved.
; 


IFNDEF __WASM__
IFNDEF __FLAT__
        PUBLIC  @TSystemError@swapStatusLine$qm11TDrawBuffer
ELSE
        PUBLIC  @TSystemError@swapStatusLine$qr11TDrawBuffer
ENDIF
        EXTRN   @TScreen@screenWidth :  BYTE
        EXTRN   @TScreen@screenHeight : BYTE


IFNDEF __FLAT__
        EXTRN   @TScreen@screenBuffer : FAR PTR
ELSE
        EXTRN   @TScreen@screenBuffer : FWORD
ENDIF
ENDIF


        INCLUDE TV.INC

IFDEF __WASM__

; Watcom version. Rather than referencing C++ statics through a
; compiler-specific name mangling scheme, everything is received as plain
; extern "C" arguments; TSystemError::swapStatusLine wraps this on the
; C++ side (see syserr.cpp).
;
; void tvSwapStatusLine( void far *bufData, void far *scrBuf,
;                        unsigned width, unsigned height );

        PUBLIC  tvSwapStatusLine

        .CODE

IFNDEF __FLAT__

tvSwapStatusLine PROC FAR bufData:DWORD, scrBuf:DWORD, scrW:WORD, scrH:WORD
        USES    SI, DI, DS

        MOV     CX, [scrW]
        MOV     AX, [scrH]
        DEC     AL
        MUL     CL
        SHL     AX, 1
        LES     DI, [scrBuf]
        ADD     DI, AX
        LDS     SI, [bufData]
@@1:
        MOV     AX, ES:[DI]
        MOVSW
        MOV     DS:[SI-2], AX
        LOOP  @@1
        RET
tvSwapStatusLine ENDP

ELSE         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 32-bit ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tvSwapStatusLine PROC bufData:PTR, scrBuf:PTR, scrW:DWORD, scrH:DWORD
        USES    ESI, EDI

        MOV     ECX, [scrW]
        MOV     EAX, [scrH]
        DEC     AL
        MUL     CL
        MOVZX   EAX, AX
        SHL     EAX, 1
        MOV     EDI, [scrBuf]
        ADD     EDI, EAX
        MOV     ESI, [bufData]
@@1:
        MOV     AX, [EDI]
        MOV     DX, [ESI]
        MOV     [EDI], DX
        MOV     [ESI], AX
        ADD     ESI, 2
        ADD     EDI, 2
        LOOP  @@1
        RET
tvSwapStatusLine ENDP

ENDIF

ELSE ; not __WASM__ (Borland/TASM original)

CODESEG

IFNDEF __FLAT__
@TSystemError@swapStatusLine$qm11TDrawBuffer PROC
ELSE
@TSystemError@swapStatusLine$qr11TDrawBuffer PROC
ENDIF
        ARG     Buffer : PTR
IFNDEF __FLAT__
        USES    SI, DI

        MOV     CL, BYTE PTR [@TScreen@screenWidth]
        XOR     CH, CH
        MOV     AL, [@TScreen@screenHeight]
        DEC     AL
        MUL     CL
        SHL     AX, 1
        LES     DI, [@TScreen@screenBuffer]
        ADD     DI, AX
        PUSH    DS
        LDS     SI, [Buffer]
@@1:
        MOV     AX, ES:[DI]
        MOVSW
        MOV     DS:[SI-2], AX
        LOOP  @@1
        POP     DS
        RET
ELSE         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 32-bit ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        USES ESI, EDI

        MOVZX   ECX, BYTE PTR  [LARGE @TScreen@screenWidth]
        MOV     AL, BYTE PTR [LARGE @TScreen@screenHeight]
        DEC     AL
        MUL     CL
        MOVZX   EAX, AX
        SHL     EAX, 1
        LES     EDI, [LARGE @TScreen@screenBuffer]
        ADD     EDI, EAX
        MOV     ESI, DWORD PTR [Buffer]
        ADD     ESI, TDrawBufferData
        MOV     ESI, [ESI]
@@1:
        MOV     AX, ES:[EDI]
        MOVSW
        MOV     [ESI-2], AX
        LOOP  @@1
        RET
ENDIF

ENDP

ENDIF ; __WASM__

END
