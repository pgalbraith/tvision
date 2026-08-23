/*------------------------------------------------------------*/
/* filename -       wchw16.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  real mode services for the Open Watcom    */
/*                  16-bit build                              */
/*------------------------------------------------------------*/

// The real mode counterpart to WCHWDOS.CPP, which does the same jobs for the
// DOS/4G target. Everything here would be in one of Borland's .asm files, or
// in THardwareInfo on the 32-bit target. It is kept out of the Turbo Vision
// sources that call it so that those stay as close to upstream as they can;
// see WCDOS16.H.
//
// This file compiles to an empty object for every target other than 16-bit
// Watcom.

#define Uses_TScreen
#define Uses_THardwareInfo
#include <tvision/tv.h>

#if defined( __WATCOMC__ ) && !defined( __FLAT__ )

#include <tvision/internal/wcdos16.h>

#if !defined( __DOS_H )
#include <tvision/compat/borland/dos.h>
#endif  // __DOS_H

#if !defined( __BIOS_H )
#include <bios.h>
#endif  // __BIOS_H

/*------------------------------------------------------------*/
/* Caret                                                      */
/*                                                            */
/* Real mode has no THardwareInfo caret functions: TVCURSOR.ASM*/
/* used INT 10h directly, and so do these.                    */
/*------------------------------------------------------------*/

void dosSetCaretPosition( int x, int y ) noexcept
{
    union REGS r;

    r.h.ah = 2;
    r.h.bh = 0;
    r.h.dh = (uchar) y;
    r.h.dl = (uchar) x;
    int86( 0x10, &r, &r );
}

void dosSetCaretSize( int caretSize, Boolean insertMode ) noexcept
{
    union REGS r;

    if( caretSize == 0 )
        r.w.cx = 0x2000;                // Hide the caret.
    else
        {
        uchar base = 8;                 // Scan lines per character cell.
        uchar start, end;

        if( TDisplay::isEGAorVGA() )
            {
            // BH selects the font pointer, and INT 10h AX=1130h reads it.
            // TVCURSOR.ASM cleared BH earlier in the routine and only BL
            // here; carried across to C++ without it, BH was whatever the
            // stack held, so base came back wrong and the caret was
            // invisible in tvedit on this target.
            r.w.ax = 0x1130;
            r.h.bh = 0;
            r.h.bl = 0;
            int86( 0x10, &r, &r );
            base = r.h.cl;
            }

        start = (uchar) (((ushort) (uchar) (caretSize >> 8) * base + 50) / 100);
        end = (uchar) (((ushort) (uchar) caretSize * base + 50) / 100);

        if( insertMode )
            {
            start = 0;                  // Grow the caret up into a block.
            if( end == 0 )
                end = 7;
            }

        r.h.ch = start;
        r.h.cl = end;
        }
    r.h.ah = 1;
    int86( 0x10, &r, &r );
}

/*------------------------------------------------------------*/
/* Display                                                    */
/*                                                            */
/* What TDisplay does through Borland's pseudo-register       */
/* variables and its own videoInt(), done through int86().    */
/*------------------------------------------------------------*/

int dosIsEGAorVGA() noexcept
{
    union REGS r;

    r.h.bl = 0x10;
    r.h.ah = 0x12;
    int86( 0x10, &r, &r );
    return r.h.bl != 0x10;
}

ushort dosGetCursorType() noexcept
{
    uchar start, end, base = 8;
    ushort result;
    union REGS r;

    r.h.ah = 3;
    r.h.bh = 0;
    int86( 0x10, &r, &r );

    start = r.h.ch;
    end = r.h.cl;

    if( r.w.cx == 0x2000 )
        return 0;

    if( dosIsEGAorVGA() )
    {
        r.w.ax = 0x1130;
        r.h.bh = 0;     // Selects the font; dosGetRows() sets it too.
        r.h.bl = 0;
        int86( 0x10, &r, &r );
        base = r.h.cl;
    }

    start = (ushort) start * 100 / base;
    end = (ushort) end * 100 / base;

    result = (start << 8) + end;
    return result;
}

void dosSetCursorType( ushort ct ) noexcept
{
    uchar start, end, base = 8;
    union REGS r;

    if( ct == 0 )
        r.w.cx = 0x2000;
    else
        {
        start = ct >> 8;
        end = ct & 0xFF;

        if( dosIsEGAorVGA() )
            {
            r.w.ax = 0x1130;
            r.h.bh = 0;     // Selects the font; dosGetRows() sets it too.
            r.h.bl = 0;
            int86( 0x10, &r, &r );
            base = r.h.cl;
            }

        start = ((ushort) start * base + 50) / 100;
        end = ((ushort) end * base + 50) / 100;

        r.h.ch = start;
        r.h.cl = end;
        }
    r.h.ah = 1;
    int86( 0x10, &r, &r );
}

void dosClearScreen( uchar w, uchar h ) noexcept
{
    union REGS r;

    r.h.bh = 0x07;
    r.w.cx = 0;
    r.h.dl = w;
    r.h.dh = h - 1;
    r.w.ax = 0x0600;
    int86( 0x10, &r, &r );
}

ushort dosGetRows() noexcept
{
    union REGS r;

    r.w.ax = 0x1130;
    r.h.bh = 0;
    r.h.dl = 0;
    int86( 0x10, &r, &r );
    if( r.h.dl == 0 )
        r.h.dl = 24;
    return r.h.dl + 1;
}

ushort dosGetCols() noexcept
{
    union REGS r;

    r.h.ah = 0x0F;
    int86( 0x10, &r, &r );
    return r.h.ah;
}

ushort dosGetCrtMode() noexcept
{
    union REGS r;

    r.h.ah = 0x0F;
    int86( 0x10, &r, &r );
    ushort mode = r.h.al;
    if( dosGetRows() > 25 )
        mode |= TDisplay::smFont8x8;
    return mode;
}

void dosSetCrtMode( ushort mode ) noexcept
{
    ushort eflag = THardwareInfo::getBiosEquipmentFlag() & 0xFFCF;
    eflag |= (mode == TDisplay::smMono) ? 0x30 : 0x20;
    THardwareInfo::setBiosEquipmentFlag( eflag );
    THardwareInfo::setBiosVideoInfo( THardwareInfo::getBiosVideoInfo() & 0x00FE );

    union REGS r;
    r.h.ah = 0;
    r.h.al = mode;
    int86( 0x10, &r, &r );

    if( (mode & TDisplay::smFont8x8) != 0 )
        {
        r.w.ax = 0x1112;
        r.h.bl = 0;
        int86( 0x10, &r, &r );

        if( dosGetRows() > 25 )
            {
            THardwareInfo::setBiosVideoInfo( THardwareInfo::getBiosVideoInfo() | 1 );

            r.h.ah = 1;
            r.w.cx = 0x0607;
            int86( 0x10, &r, &r );

            r.h.ah = 0x12;
            r.h.bl = 0x20;
            int86( 0x10, &r, &r );
            }
        }
}

/*------------------------------------------------------------*/
/* Mouse                                                      */
/*                                                            */
/* What THWMouse does directly through INT 33h, done here     */
/* instead so that TMOUSE.CPP does not reach into the BIOS    */
/* itself.                                                    */
/*------------------------------------------------------------*/

int dosMouseReset( uchar &buttonCount ) noexcept
{
    if( _dos_getvect( 0x33 ) == 0 )
        return 0;

    union REGS r;
    r.w.ax = 0;
    int86( 0x33, &r, &r );

    if( r.w.ax == 0 )
        return 0;
    buttonCount = r.h.bl;

    r.w.ax = 4;
    r.w.cx = 0;
    r.w.dx = 0;
    int86( 0x33, &r, &r );
    return 1;
}

void dosMouseShow() noexcept
{
    union REGS r;
    r.w.ax = 1;
    int86( 0x33, &r, &r );
}

void dosMouseHide() noexcept
{
    union REGS r;
    r.w.ax = 2;
    int86( 0x33, &r, &r );
}

void dosMouseSetRange( ushort rx, ushort ry ) noexcept
{
    union REGS r;

    r.w.dx = rx << 3;
    r.w.cx = 0;
    r.w.ax = 7;
    int86( 0x33, &r, &r );

    r.w.dx = ry << 3;
    r.w.cx = 0;
    r.w.ax = 8;
    int86( 0x33, &r, &r );
}

void dosMouseGetEvent( MouseEventType &me ) noexcept
{
    union REGS r;
    r.w.ax = 3;
    int86( 0x33, &r, &r );
    me.buttons = r.h.bl;
    me.wheel = r.h.bh == 0 ? 0 : char(r.h.bh) > 0 ? mwDown : mwUp; // CuteMouse
    me.where.x = r.w.cx >> 3;
    me.where.y = r.w.dx >> 3;
    me.eventFlags = 0;
}

void dosMouseRegisterHandler( unsigned mask, void (_FAR *func)() ) noexcept
{
    union REGS r;
    struct SREGS s;
    segread( &s );
    r.w.ax = 12;
    r.w.cx = mask;
    r.w.dx = FP_OFF( func );
    s.es = FP_SEG( func );
    int86x( 0x33, &r, &r, &s );
}

/*------------------------------------------------------------*/
/* System error                                                */
/*                                                            */
/* SYSERR.CPP's checkIDE() fires this to let Int11trap's       */
/* handler see whether Turbo Vision is running under the IDE.  */
/*------------------------------------------------------------*/

void dosFireInt12( int ax_bx ) noexcept
{
    union REGS r;
    r.x.ax = ax_bx;
    r.x.bx = ax_bx;
    int86( 0x12, &r, &r );
}

/*------------------------------------------------------------*/
/* Event                                                       */
/*                                                            */
/* tvMouseIntBody is TEventQueue's mouse callback body, called */
/* by tvMouseIntStub (WCSTUBS.ASM) once it has saved the       */
/* driver's registers and switched to DGROUP. It is declared a */
/* friend of TEventQueue by system.h, under this exact name    */
/* and signature, which is what lets it live here rather than  */
/* in TEVENT.CPP, where Borland's mouseInt() is a member.       */
/*                                                            */
/* dosReadKeyPress reads one key with the BIOS in place of      */
/* Borland's inline INT 16h.                                  */
/*------------------------------------------------------------*/

extern "C" void __cdecl tvMouseIntBody( unsigned flag, unsigned buttons,
                                        unsigned x, unsigned y )
{
    MouseEventType tempMouse;

    tempMouse.buttons = buttons & 0xFF;
    uchar wheel = buttons >> 8;
    tempMouse.wheel = wheel == 0 ? 0 : char(wheel) > 0 ? mwDown : mwUp; // CuteMouse
    tempMouse.eventFlags = 0;
    tempMouse.where.x = x >> 3;
    tempMouse.where.y = y >> 3;
    tempMouse.controlKeyState = THardwareInfo::getShiftState();

    if( (flag & 0x1e) != 0 &&
        TEventQueue::eventCount < eventQSize )
        {
        TEventQueue::eventQTail->what = THardwareInfo::getTickCount();
        TEventQueue::eventQTail->mouse = TEventQueue::curMouse;
        if( ++TEventQueue::eventQTail >= TEventQueue::eventQueue + eventQSize )
            TEventQueue::eventQTail = TEventQueue::eventQueue;
        TEventQueue::eventCount++;
        }

    TEventQueue::curMouse = tempMouse;
    TEventQueue::mouseIntFlag = True;
}

int dosReadKeyPress( TEvent &ev ) noexcept
{
    if( _bios_keybrd( _KEYBRD_READY ) == 0 )
        {
        ev.what = evNothing;
        return 0;
        }
    ev.what = evKeyDown;
    ev.keyDown.keyCode = _bios_keybrd( _KEYBRD_READ );
    ev.keyDown.controlKeyState = THardwareInfo::getShiftState();
    return 1;
}

#endif // __WATCOMC__ && !__FLAT__
