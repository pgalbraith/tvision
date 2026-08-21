;/*------------------------------------------------------------*/
;/* filename -       wcstubs.asm                               */
;/*                                                            */
;/* function(s)                                                */
;/*                  Assembly entry stubs for the Open Watcom  */
;/*                  build. Not used by the Borland build.     */
;/*------------------------------------------------------------*/

IFDEF __WASM__
IFNDEF __FLAT__

        .MODEL LARGE, C

        EXTRN   tvMouseIntBody : FAR

        PUBLIC  tvMouseIntStub

        .CODE

; Called asynchronously by the INT 33h mouse driver with:
;   AX = event flag mask, BX = button state (BH = wheel, CuteMouse),
;   CX = X coordinate, DX = Y coordinate.
; Saves every register, loads DS with our data segment and forwards the
; register values to the C function
;   void __cdecl tvMouseIntBody( unsigned flag, unsigned buttons,
;                                unsigned x, unsigned y );
tvMouseIntStub PROC FAR
        PUSH    DS
        PUSH    ES
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    BP

        MOV     BP, AX          ; Preserve the driver's AX across the DS load.
        MOV     AX, SEG DGROUP
        MOV     DS, AX
        MOV     AX, BP

        PUSH    DX              ; y
        PUSH    CX              ; x
        PUSH    BX              ; buttons
        PUSH    AX              ; flag
        CALL    tvMouseIntBody
        ADD     SP, 8

        POP     BP
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        POP     ES
        POP     DS
        RET
tvMouseIntStub ENDP

ENDIF
ENDIF

        END
