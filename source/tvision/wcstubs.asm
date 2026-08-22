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
        PUBLIC  tvCallOnAltStack

; In the large data model Open Watcom reaches DGROUP through SS, not DS, and
; its generated code checks the stack against __STACKLOW. A C function entered
; from an interrupt runs on whatever stack was interrupted, so it has to be
; given one inside DGROUP first. The two stubs below do that. The sizes are
; what the code on each stack needs, with room for an interrupt on top:
;
;   tvIntStack   the INT 33h mouse callback, which only queues an event.
;   tvCritStack  TSystemError::sysErr, which formats a message and puts up a
;                status line prompt. SYSINT.ASM gave the same job 1K.

intStackSize    EQU     512
critStackSize   EQU     2048

        .DATA?

tvIntStack      DB      intStackSize DUP (?)
tvIntStackTop   LABEL   BYTE
tvCritStack     DB      critStackSize DUP (?)
tvCritStackTop  LABEL   BYTE

        .DATA

; The limit Open Watcom's generated stack check (__STK) compares against.
; Written without a leading underscore, because '.MODEL <model>, C' adds one.
        EXTRN   _STACKLOW : WORD

        .CODE

; Called asynchronously by the INT 33h mouse driver with:
;   AX = event flag mask, BX = button state (BH = wheel, CuteMouse),
;   CX = X coordinate, DX = Y coordinate.
; Saves every register, switches to tvIntStack and DGROUP, and forwards the
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

        MOV     SI, SP          ; Remember the driver's stack.
        MOV     DI, SS
        CLI
        MOV     SS, AX
        MOV     SP, OFFSET DGROUP:tvIntStackTop
        STI

        PUSH    DI
        PUSH    SI
        MOV     AX, _STACKLOW
        PUSH    AX
        MOV     WORD PTR _STACKLOW, OFFSET DGROUP:tvIntStack

        PUSH    DX              ; y
        PUSH    CX              ; x
        PUSH    BX              ; buttons
        PUSH    BP              ; flag
        CALL    tvMouseIntBody
        ADD     SP, 8

        POP     AX
        MOV     _STACKLOW, AX
        POP     SI
        POP     DI
        CLI
        MOV     SS, DI
        MOV     SP, SI
        STI

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

; void __cdecl tvCallOnAltStack( void (far *fn)( void ) );
;
; Calls 'fn' on tvCritStack. Used by the INT 24H critical error handler in
; WCSYSINT.CPP: DOS enters that handler on a stack of its own, too small for
; the prompt Turbo Vision puts up there, and possibly in a segment other than
; DGROUP.
tvCallOnAltStack PROC FAR
        PUSH    BP
        MOV     BP, SP
        PUSH    SI
        PUSH    DI

        ; Everything needed after the switch has to be in a register: the
        ; argument is addressed through BP, which belongs to the old stack.
        MOV     CX, [BP+6]      ; fn offset
        MOV     BX, [BP+8]      ; fn segment
        MOV     AX, SEG DGROUP

        MOV     SI, SP
        MOV     DI, SS
        CLI
        MOV     SS, AX
        MOV     SP, OFFSET DGROUP:tvCritStackTop
        STI

        PUSH    DI              ; Caller's SS
        PUSH    SI              ; Caller's SP
        MOV     AX, _STACKLOW
        PUSH    AX
        MOV     WORD PTR _STACKLOW, OFFSET DGROUP:tvCritStack

        PUSH    BX
        PUSH    CX
        MOV     BP, SP
        CALL    DWORD PTR [BP]
        ADD     SP, 4

        POP     AX
        MOV     _STACKLOW, AX
        POP     SI
        POP     DI
        CLI
        MOV     SS, DI
        MOV     SP, SI
        STI

        POP     DI
        POP     SI
        POP     BP
        RET
tvCallOnAltStack ENDP

ENDIF
ENDIF

        END
