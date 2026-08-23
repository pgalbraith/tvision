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

; THardwareInfo's state, defined in hardwrvr.cpp. Borland's HARDWARE.ASM
; reaches the same four variables as static members of THardwareInfo, under
; the names Borland's compiler gives them.
        EXTRN   tvDpmiFlag : BYTE
        EXTRN   tvColorSel : WORD
        EXTRN   tvMonoSel : WORD
        EXTRN   tvBiosSel : WORD

        PUBLIC  tvHWInfoCtor
        PUBLIC  tvHWInfoDtor
        PUBLIC  tvGetBiosEquipmentFlag
        PUBLIC  tvGetBiosSelector

        PUBLIC  tvSwapStatusLine

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


; ---------------------------------------------------------------------------
; THardwareInfo, moved here from HARDWARE.ASM
;
; Borland's HARDWARE.ASM defines these as THardwareInfo member functions,
; named the way Borland's compiler names them. WASM cannot write Watcom's
; spelling of a C++ member function, so this build needs its own copies under
; extern "C" names, which hardware.h declares. Keeping them here rather than
; inside HARDWARE.ASM leaves Borland's file exactly as upstream has it; this
; build does not assemble it at all.
; ---------------------------------------------------------------------------

        ASSUME DS:DGROUP

tvHWInfoCtor  PROC    FAR

; The four variables below are near data, written through DS. Borland's large
; model keeps DS on DGROUP, so HARDWARE.ASM does not load it; Watcom's lets DS
; float, so this does, the same way the two routines below do before they read
; tvBiosSel. THardwareInfo is a function-local static in tapplica.cpp, built
; from ordinary C++ code rather than from startup code, so there is nothing to
; say what DS holds on entry.
        PUSH    DS
        MOV     AX, SEG DGROUP
        MOV     DS, AX

; Are we running in protected mode?
        MOV     AX, 352FH   ; Check for a null INT 2F handler first
        INT     21H         ; just in case.
        MOV     AX, ES
        OR      AX, BX
        JZ    @@nodpmi

        MOV     AX, 0FB42H
        MOV     BX, 01H
        INT     2FH
        CMP     AX, 01H
        JNE   @@nodpmi

; Yes, in protected mode, thus we need to allocate selectors...
        MOV     [tvDpmiFlag], 01H

        MOV     AX, 02H
        MOV     BX, 0040H
        INT     31H
        MOV     [tvBiosSel], AX

        MOV     AX, 02H
        MOV     BX, 0B000H
        INT     31H
        MOV     [tvMonoSel], AX

        MOV     AX, 02H
        MOV     BX, 0B800H
        INT     31H
        MOV     [tvColorSel], AX

        POP     DS
        RET

@@nodpmi:
        MOV     [tvDpmiFlag], 00H
        MOV     [tvBiosSel], 00040H
        MOV     [tvMonoSel], 0B000H
        MOV     [tvColorSel], 0B800H

        POP     DS
        RET
tvHWInfoCtor  ENDP

tvHWInfoDtor  PROC    FAR
        RET
tvHWInfoDtor  ENDP

tvGetBiosEquipmentFlag   PROC FAR
        PUSH    DS
        MOV     AX, SEG DGROUP
        MOV     DS, AX

        MOV     BX, 10H
        MOV     ES, WORD PTR DGROUP:[tvBiosSel]
        MOV     AX, ES:[BX]

        POP     DS
        RET
tvGetBiosEquipmentFlag   ENDP

tvGetBiosSelector    PROC FAR
        PUSH    DS
        MOV     AX, SEG DGROUP
        MOV     DS, AX
        MOV     AX, WORD PTR DGROUP:[tvBiosSel]
        POP     DS
        RET
tvGetBiosSelector    ENDP


; ---------------------------------------------------------------------------
; TSystemError::swapStatusLine, moved here from SWAPST.ASM
;
; The same problem, and the same answer: this takes plain arguments instead of
; reaching TScreen's members by their Borland names, and syserr.cpp wraps it.
; Only the 16-bit build needs it. The 32-bit build has no assembly at all.
;
; void tvSwapStatusLine( void far *bufData, void far *scrBuf,
;                        unsigned width, unsigned height );
; ---------------------------------------------------------------------------

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

ENDIF
ENDIF

        END
