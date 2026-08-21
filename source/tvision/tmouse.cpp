/*------------------------------------------------------------*/
/* filename -       tmouse.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  TMouse and THWMouse member functions      */
/*------------------------------------------------------------*/
/*
 *      Turbo Vision - Version 2.0
 *
 *      Copyright (c) 1994 by Borland International
 *      All Rights Reserved.
 *
 */

#define Uses_TEvent
#define Uses_TEventQueue
#define Uses_THardwareInfo
#include <tvision/tv.h>

#if !defined( __FLAT__ )
#if !defined( __DOS_H )
#include <tvision/compat/borland/dos.h>
#endif  // __DOS_H
#endif  // __FLAT__


uchar _NEAR THWMouse::buttonCount = 0;
Boolean _NEAR THWMouse::handlerInstalled = False;

THWMouse::THWMouse() noexcept
{
    resume();
}

void THWMouse::resume() noexcept
{
#if defined( __FLAT__ )
    buttonCount = THardwareInfo::getButtonCount();
    show();
#elif defined( __WATCOMC__ )
    if( _dos_getvect( 0x33 ) == 0 )
        return;

    union REGS r;
    r.w.ax = 0;
    int86( 0x33, &r, &r );

    if( r.w.ax == 0 )
        return;
    buttonCount = r.h.bl;

    r.w.ax = 4;
    r.w.cx = 0;
    r.w.dx = 0;

    int86( 0x33, &r, &r );
    show();
#else
    if( getvect( 0x33 ) == 0 )
        return;

    _AX = 0;
    _genInt( 0x33 );

    if( _AX == 0 )
        return;
    buttonCount = _BL;

    _AX = 4;
    _CX = 0;
    _DX = 0;

    _genInt( 0x33 );
    show();
#endif
}

THWMouse::~THWMouse()
{
    suspend();
}

void THWMouse::suspend() noexcept
{
#if defined(__FLAT__)
    hide();
    buttonCount = 0;
#else
    if( present() == False )
        return;
    hide();
    if( handlerInstalled == True )
        {
        registerHandler( 0, 0 );
        handlerInstalled = False;
        }
    buttonCount = 0;
#endif
}

#pragma warn -asc

void THWMouse::show() noexcept
{
#if defined( __FLAT__ )
    THardwareInfo::cursorOn();
#elif defined( __WATCOMC__ )
    if( present() )
        {
        union REGS r;
        r.w.ax = 1;
        int86( 0x33, &r, &r );
        }
#else
    asm push ax;
    asm push es;

    if( present() )
        {
        _AX = 1;
        _genInt( 0x33 );
        }

    asm pop es;
    asm pop ax;
#endif
}

void THWMouse::hide() noexcept
{
#if defined( __FLAT__ )
    THardwareInfo::cursorOff();
#elif defined( __WATCOMC__ )
    if( buttonCount != 0 )
        {
        union REGS r;
        r.w.ax = 2;
        int86( 0x33, &r, &r );
        }
#else
    asm push ax;
    asm push es;

    if( buttonCount != 0 )
        {
        _AX = 2;
        _genInt( 0x33 );
        }
    asm pop es;
    asm pop ax;
#endif
}

#pragma warn .asc

#pragma argsused
void THWMouse::setRange( ushort rx, ushort ry ) noexcept
{
    (void) rx;
    (void) ry;
#if defined( __WATCOMC__ ) && !defined( __FLAT__ )
    if( buttonCount != 0 )
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
#elif !defined( __FLAT__ )
    if( buttonCount != 0 )
        {
        _DX = rx;
        _DX <<= 3;
        _CX = 0;
        _AX = 7;
        _genInt( 0x33 );

        _DX = ry;
        _DX <<= 3;
        _CX = 0;
        _AX = 8;
        _genInt( 0x33 );
        }
#endif
}

void THWMouse::getEvent( MouseEventType& me ) noexcept
{
#if defined( __FLAT__ )
    me.buttons = 0;
    me.wheel = 0;
    me.where.x = 0;
    me.where.y = 0;
    me.eventFlags = 0;
#elif defined( __WATCOMC__ )
    union REGS r;
    r.w.ax = 3;
    int86( 0x33, &r, &r );
    me.buttons = r.h.bl;
    me.wheel = r.h.bh == 0 ? 0 : char(r.h.bh) > 0 ? mwDown : mwUp; // CuteMouse
    me.where.x = r.w.cx >> 3;
    me.where.y = r.w.dx >> 3;
    me.eventFlags = 0;
#else
    _AX = 3;
    _genInt( 0x33 );
    _AX = _BX;
    me.buttons = _AL;
    me.wheel = _AH == 0 ? 0 : char(_AH) > 0 ? mwDown : mwUp; // CuteMouse
    me.where.x = _CX >> 3;
    me.where.y = _DX >> 3;
    me.eventFlags = 0;
#endif
}

#if defined( __WATCOMC__ ) && !defined( __FLAT__ )
void THWMouse::registerHandler( unsigned mask, void (_FAR *func)() )
{
    if( !present() )
        return;

    union REGS r;
    struct SREGS s;
    segread( &s );
    r.w.ax = 12;
    r.w.cx = mask;
    r.w.dx = FP_OFF( func );
    s.es = FP_SEG( func );

    int86x( 0x33, &r, &r, &s );
    handlerInstalled = True;
}
#elif !defined( __FLAT__ )
void THWMouse::registerHandler( unsigned mask, void (_FAR *func)() )
{
    if( !present() )
        return;

    _AX = 12;
    _CX = mask;
    _DX = FP_OFF( func );
    _ES = FP_SEG( func );

    _genInt( 0x33 );
    handlerInstalled = True;
}
#endif

TMouse::TMouse() noexcept
{
//    show();
}

TMouse::~TMouse()
{
//    hide();
}

