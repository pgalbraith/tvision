/*------------------------------------------------------------*/
/* filename -       tscreen.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TScreen member functions                  */
/*------------------------------------------------------------*/
/*
 *      Turbo Vision - Version 2.0
 *
 *      Copyright (c) 1994 by Borland International
 *      All Rights Reserved.
 *
 */

#define Uses_TEvent
#define Uses_TScreen
#define Uses_THardwareInfo
#include <tvision/tv.h>

#if !defined( __FLAT__ ) && !defined( __DOS_H )
#include <tvision/compat/borland/dos.h>
#endif  // __DOS_H

ushort _NEAR TScreen::startupMode = 0xFFFF;
ushort _NEAR TScreen::startupCursor = 0;
ushort _NEAR TScreen::screenMode = 0;
ushort _NEAR TScreen::screenWidth = 0;
ushort _NEAR TScreen::screenHeight = 0;
Boolean _NEAR TScreen::hiResScreen = False;
Boolean _NEAR TScreen::checkSnow = True;
TScreenCell * _NEAR TScreen::screenBuffer;
ushort _NEAR TScreen::cursorLines = 0;
Boolean _NEAR TScreen::clearOnSuspend = True;

ushort TDisplay::getCursorType() noexcept
{
#if defined( __FLAT__ )
    return THardwareInfo::getCaretSize();
#elif defined( __WATCOMC__ )
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

    if( isEGAorVGA() )
    {
        r.w.ax = 0x1130;
        r.h.bl = 0;
        int86( 0x10, &r, &r );
        base = r.h.cl;
    }

    start = (ushort) start * 100 / base;
    end = (ushort) end * 100 / base;

    result = (start << 8) + end;
    return result;
#else
    uchar start, end, base = 8;
    ushort result;

    _AH = 3;
    _BH = 0;
    videoInt();

    start = _CH;
    end = _CL;

    if( _CX == 0x2000 )
        return 0;

    if( isEGAorVGA() )
    {
        _AX = 0x1130;
        _BL = 0;
        videoInt();
        base = _CL;
    }

    start = (ushort) start * 100 / base;
    end = (ushort) end * 100 / base;

    result = (start << 8) + end;
    return result;
#endif
}

#if !defined( __FLAT__ )
int TDisplay::isEGAorVGA(void)
{
#if defined( __WATCOMC__ )
    union REGS r;
    r.h.bl = 0x10;
    r.h.ah = 0x12;
    int86( 0x10, &r, &r );
    return r.h.bl != 0x10;
#else
    _BL=0x10;
    _AH=0x12;
    videoInt();
    return _BL != 0x10;
#endif
}
#endif

void TDisplay::setCursorType( ushort ct ) noexcept
{
#if defined( __FLAT__ )
    THardwareInfo::setCaretSize( ct & 0xFF );
#elif defined( __WATCOMC__ )
    uchar start, end, base = 8;
    union REGS r;

    if( ct == 0 )
        r.w.cx = 0x2000;
    else
        {
        start = ct >> 8;
        end = ct & 0xFF;

        if( isEGAorVGA() )
            {
            r.w.ax = 0x1130;
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
#else
    uchar start, end, base = 8;

    if( ct == 0 )
        _CX = 0x2000;
    else
        {
        start = ct >> 8;
        end = ct & 0xFF;

        if( isEGAorVGA() )
            {
            _AX = 0x1130;
            _BL = 0;
            videoInt();
            base = _CL;
            }

        start = ((ushort) start * base + 50) / 100;
        end = ((ushort) end * base + 50) / 100;

        _CH = start;
        _CL = end;
        }
    _AH = 1;
    videoInt();
#endif
}

void TDisplay::clearScreen( uchar w, uchar h ) noexcept
{
#if defined( __FLAT__ )
    THardwareInfo::clearScreen( w, h );
#elif defined( __WATCOMC__ )
    union REGS r;
    r.h.bh = 0x07;
    r.w.cx = 0;
    r.h.dl = w;
    r.h.dh = h - 1;
    r.w.ax = 0x0600;
    int86( 0x10, &r, &r );
#else
    _BH = 0x07;
    _CX = 0;
    _DL = w;
    _DH = h - 1;
    _AX = 0x0600;
    videoInt();
#endif
}

#pragma warn -asc

#if !defined( __FLAT__ ) && !defined( __WATCOMC__ )
void TDisplay::videoInt()
{

I   PUSH    BP
I   PUSH    ES
I   INT     10h
I   POP     ES
I   POP     BP

}
#endif

#pragma warn .asc

ushort TDisplay::getRows() noexcept
{
#if defined( __FLAT__ )
    return THardwareInfo::getScreenRows();
#elif defined( __WATCOMC__ )
    union REGS r;
    r.w.ax = 0x1130;
    r.h.bh = 0;
    r.h.dl = 0;
    int86( 0x10, &r, &r );
    if( r.h.dl == 0 )
        r.h.dl = 24;
    return r.h.dl + 1;
#else
    _AX = 0x1130;
    _BH = 0;
    _DL = 0;
    videoInt();
    if( _DL == 0 )
        _DL = 24;
    return _DL + 1;
#endif
}

ushort TDisplay::getCols() noexcept
{
#if defined( __FLAT__ )
    return THardwareInfo::getScreenCols();
#elif defined( __WATCOMC__ )
    union REGS r;
    r.h.ah = 0x0F;
    int86( 0x10, &r, &r );
    return r.h.ah;
#else
    _AH = 0x0F;
    videoInt();
    return _AH;
#endif
}

ushort TDisplay::getCrtMode() noexcept
{
#if defined( __FLAT__ )
    return THardwareInfo::getScreenMode();
#elif defined( __WATCOMC__ )
    union REGS r;
    r.h.ah = 0x0F;
    int86( 0x10, &r, &r );
    ushort mode = r.h.al;
    if( getRows() > 25 )
        mode |= smFont8x8;
    return mode;
#else
    _AH = 0x0F;
    videoInt();
    ushort mode = _AL;
    if( getRows() > 25 )
        mode |= smFont8x8;
    return mode;
#endif
}

#pragma argsused
void TDisplay::setCrtMode( ushort mode ) noexcept
{
#if defined( __FLAT__ )
    THardwareInfo::setScreenMode( mode );
#elif defined( __WATCOMC__ )
    ushort eflag = THardwareInfo::getBiosEquipmentFlag() & 0xFFCF;
    eflag |= (mode == smMono) ? 0x30 : 0x20;
    THardwareInfo::setBiosEquipmentFlag( eflag );
    THardwareInfo::setBiosVideoInfo( THardwareInfo::getBiosVideoInfo() & 0x00FE );

    union REGS r;
    r.h.ah = 0;
    r.h.al = mode;
    int86( 0x10, &r, &r );

    if( (mode & smFont8x8) != 0 )
        {
        r.w.ax = 0x1112;
        r.h.bl = 0;
        int86( 0x10, &r, &r );

        if( getRows() > 25 )
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
#else
    ushort eflag = THardwareInfo::getBiosEquipmentFlag() & 0xFFCF;
    eflag |= (mode == smMono) ? 0x30 : 0x20;
    THardwareInfo::setBiosEquipmentFlag( eflag );
    THardwareInfo::setBiosVideoInfo( THardwareInfo::getBiosVideoInfo() & 0x00FE );

    _AH = 0;
    _AL = mode;
    videoInt();

    if( (mode & smFont8x8) != 0 )
        {
        _AX = 0x1112;
        _BL = 0;
        videoInt();

        if( getRows() > 25 )
            {
            THardwareInfo::setBiosVideoInfo( THardwareInfo::getBiosVideoInfo() | 1 );

            _AH = 1;
            _CX = 0x0607;
            videoInt();

            _AH = 0x12;
            _BL = 0x20;
            videoInt();
            }
        }
#endif
}

TScreen::TScreen() noexcept
{
#if defined(__FLAT__)
    THardwareInfo::setUpConsole();
#endif
    startupMode = getCrtMode();
    startupCursor = getCursorType();

#if defined(__FLAT__)
    screenBuffer = THardwareInfo::allocateScreenBuffer();
#endif

    setCrtData();
}

void TScreen::resume() noexcept
{
#if defined(__FLAT__)
    THardwareInfo::setUpConsole();
#endif
    startupMode = getCrtMode();
    startupCursor = getCursorType();
    if (screenMode != startupMode)
        setCrtMode( screenMode );
    setCrtData();
}

TScreen::~TScreen()
{
    suspend();
#if defined( __FLAT__ )
    THardwareInfo::freeScreenBuffer( screenBuffer );
#endif
}

void TScreen::suspend() noexcept
{
    if( startupMode != screenMode )
        setCrtMode( startupMode );
    if (clearOnSuspend)
      clearScreen();
    setCursorType( startupCursor );
#if defined(__FLAT__)
    THardwareInfo::restoreConsole();
#endif
}

#pragma argsused

ushort TScreen::fixCrtMode( ushort mode ) noexcept
{
#ifdef __BORLANDC__
#if defined( __FLAT__ )
    if( THardwareInfo::getPlatform() != THardwareInfo::plDPMI32 )
        {
        mode = (mode & smFont8x8) ? smCO80 | smFont8x8 : smCO80;
        return mode;
        }
#endif
    if( (mode & 0xFF) == smMono )       // Strip smFont8x8 if necessary.
        return smMono;

    if( (mode & 0xFF) != smCO80 && (mode & 0xFF) != smBW80 )
        mode = (mode & 0xFF00) | smCO80;
#endif
    return mode;
}

void TScreen::setCrtData() noexcept
{
    screenMode = getCrtMode();
    screenWidth = getCols();
    screenHeight = getRows();
    hiResScreen = Boolean(screenHeight > 25);

#if !defined(__FLAT__)
    if( screenMode == smMono )
        {
        screenBuffer = THardwareInfo::getMonoAddr();
        checkSnow = False;
        }
    else
        {
        screenBuffer = THardwareInfo::getColorAddr();
        if( isEGAorVGA() )
            checkSnow = False;
        }
#endif

    cursorLines = getCursorType();
    setCursorType( 0 );
}

void TScreen::clearScreen() noexcept
{
    TDisplay::clearScreen( screenWidth, screenHeight );
}

void TScreen::flushScreen() noexcept
{
#ifdef __FLAT__
    THardwareInfo::flushScreen();
#endif
}

void TScreen::setVideoMode( ushort mode ) noexcept
{
    if ( mode != smUpdate )
        setCrtMode( fixCrtMode( mode ) );
#ifdef __FLAT__
    else
        {
        THardwareInfo::freeScreenBuffer( screenBuffer );
        screenBuffer = THardwareInfo::allocateScreenBuffer();
        }
#endif
    setCrtData();
    if (TMouse::present())
        TMouse::setRange( getCols()-1, getRows()-1 );
}
