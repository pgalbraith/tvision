/*------------------------------------------------------------*/
/* filename -       wchwdos.cpp                               */
/*                                                            */
/* function(s)                                                */
/*          THardwareInfo for Open Watcom's 32-bit DOS        */
/*          extender target (DOS/4GW etc.). Implements the    */
/*          "flat" driver interface of hardware.h with        */
/*          direct BIOS/DOS access: linear-mapped video       */
/*          memory, INT 10h/16h via int386() and the INT 33h  */
/*          mouse driver. Only part of the Watcom build.      */
/*------------------------------------------------------------*/

#if defined( __WATCOMC__ ) && defined( __FLAT__ ) && defined( __DOS__ )

#define Uses_TKeys
#define Uses_TEvent
#define Uses_TScreen
#define Uses_TSystemError
#define Uses_THardwareInfo
#include <tvision/tv.h>

#include <i86.h>
#include <dos.h>
#include <string.h>

// Under 32-bit DOS extenders the first megabyte is mapped at its physical
// address, so the BIOS data area and video memory can be accessed directly.
#define BIOS_TICKS      (*(volatile uint32_t *) 0x46C)
#define BIOS_SHIFTSTATE (*(volatile uchar *)    0x417)
#define BIOS_EQUIPMENT  (*(volatile ushort *)   0x410)
#define BIOS_VIDEOINFO  (*(volatile uchar *)    0x487)
#define BIOS_MODE       (*(volatile uchar *)    0x449)
#define BIOS_COLS       (*(volatile ushort *)   0x44A)
#define BIOS_ROWS       (*(volatile uchar *)    0x484)
#define BIOS_KBDHEAD    (*(volatile ushort *)   0x41A)
#define BIOS_KBDTAIL    (*(volatile ushort *)   0x41C)

static int keyWaiting()
{
    return BIOS_KBDHEAD != BIOS_KBDTAIL;
}

// The BIOS keeps the shift state at 0040:0017 in its own bit layout, while
// the __FLAT__ build reads controlKeyState with Win32's (tkeys.h). They are
// not the same: BIOS bit 3 is Alt, but 0x0008 is LEFT_CTRL_PRESSED, so an
// untranslated byte turns Alt-X into Ctrl-X once TKey normalizes it and no
// menu or status line hotkey matches. The byte cannot tell left from right
// Ctrl and Alt (0040:0018 can), and TKey does not care.
static ushort controlKeyState()
{
    uchar bios = BIOS_SHIFTSTATE;
    ushort state = 0;
    if( bios & 0x03 ) state |= kbShift;
    if( bios & 0x04 ) state |= kbLeftCtrl;
    if( bios & 0x08 ) state |= kbLeftAlt;
    if( bios & 0x10 ) state |= kbScrollState;
    if( bios & 0x20 ) state |= kbNumState;
    if( bios & 0x40 ) state |= kbCapsState;
    if( bios & 0x80 ) state |= kbInsState;
    return state;
}

static TScreenCell *videoMem()
{
    return (TScreenCell *) ( BIOS_MODE == 7 ? 0xB0000 : 0xB8000 );
}

// Static member definitions not provided by hardwrvr.cpp's common section
// are already there (insertState, platform, consoleHandle, ...). This file
// only implements the functions the Borland build gets from its RTL.

static void installDosHooks();
static void removeDosHooks();

THardwareInfo::THardwareInfo() noexcept
{
    platform = plDPMI32;
    insertState = True;
    installDosHooks();
}

THardwareInfo::~THardwareInfo()
{
    removeDosHooks();
}

uint64_t THardwareInfo::getTickCountMs() noexcept
{
    return (uint64_t) BIOS_TICKS * 55;
}

// Caret functions.

ushort THardwareInfo::getCaretSize() noexcept
{
    union REGS r;
    r.h.ah = 3;
    r.h.bh = 0;
    int386( 0x10, &r, &r );
    return r.w.cx;
}

void THardwareInfo::setCaretSize( ushort size ) noexcept
{
    union REGS r;
    if( size == 0 )
        r.w.cx = 0x2000;
    else
        r.w.cx = size;
    r.h.ah = 1;
    int386( 0x10, &r, &r );
}

void THardwareInfo::setCaretPosition( ushort x, ushort y ) noexcept
{
    union REGS r;
    r.h.ah = 2;
    r.h.bh = 0;
    r.h.dl = x;
    r.h.dh = y;
    int386( 0x10, &r, &r );
}

BOOL THardwareInfo::isCaretVisible() noexcept
{
    return ( getCaretSize() & 0x2000 ) == 0;
}

// Screen functions.

ushort THardwareInfo::getScreenRows() noexcept
{
    uchar rows = BIOS_ROWS;
    return rows ? rows + 1 : 25;
}

ushort THardwareInfo::getScreenCols() noexcept
{
    return BIOS_COLS;
}

ushort THardwareInfo::getScreenMode() noexcept
{
    ushort mode = BIOS_MODE;
    if( getScreenRows() > 25 )
        mode |= TDisplay::smFont8x8;
    return mode;
}

void THardwareInfo::setScreenMode( ushort mode ) noexcept
{
    union REGS r;

    ushort eflag = BIOS_EQUIPMENT & 0xFFCF;
    eflag |= (mode == TDisplay::smMono) ? 0x30 : 0x20;
    BIOS_EQUIPMENT = eflag;
    BIOS_VIDEOINFO &= 0x00FE;

    r.h.ah = 0;
    r.h.al = mode;
    int386( 0x10, &r, &r );

    if( (mode & TDisplay::smFont8x8) != 0 )
        {
        r.w.ax = 0x1112;
        r.h.bl = 0;
        int386( 0x10, &r, &r );

        if( getScreenRows() > 25 )
            {
            BIOS_VIDEOINFO |= 1;

            r.h.ah = 1;
            r.w.cx = 0x0607;
            int386( 0x10, &r, &r );

            r.h.ah = 0x12;
            r.h.bl = 0x20;
            int386( 0x10, &r, &r );
            }
        }
}

void THardwareInfo::clearScreen( ushort w, ushort h ) noexcept
{
    union REGS r;
    r.h.bh = 0x07;
    r.w.cx = 0;
    r.h.dl = w;
    r.h.dh = h - 1;
    r.w.ax = 0x0600;
    int386( 0x10, &r, &r );
}

void THardwareInfo::flushScreen() noexcept
{
}

void THardwareInfo::screenWrite( ushort x, ushort y, TScreenCell *buf,
                                 DWORD len ) noexcept
{
    TScreenCell *dst = videoMem() + (size_t) y * getScreenCols() + x;
    memcpy( dst, buf, len * sizeof(TScreenCell) );
}

TScreenCell *THardwareInfo::allocateScreenBuffer() noexcept
{
    int x = getScreenCols(), y = getScreenRows();

    if( x < 80 )        // Make sure we allocate at least enough for
        x = 80;         //   an 80x50 screen.
    if( y < 50 )
        y = 50;

    return new TScreenCell[ x * y ];
}

void THardwareInfo::freeScreenBuffer( TScreenCell *buffer ) noexcept
{
    delete[] buffer;
}

void THardwareInfo::setUpConsole() noexcept
{
}

void THardwareInfo::restoreConsole() noexcept
{
}

// Mouse functions.

static Boolean mousePresent = False;

DWORD THardwareInfo::getButtonCount() noexcept
{
    if( _dos_getvect( 0x33 ) == 0 )
        return 0;
    union REGS r;
    r.w.ax = 0;
    int386( 0x33, &r, &r );
    if( r.w.ax == 0 )
        return 0;
    mousePresent = True;
    return r.h.bl;
}

void THardwareInfo::cursorOn() noexcept
{
    if( mousePresent )
        {
        union REGS r;
        r.w.ax = 1;
        int386( 0x33, &r, &r );
        }
}

void THardwareInfo::cursorOff() noexcept
{
    if( mousePresent )
        {
        union REGS r;
        r.w.ax = 2;
        int386( 0x33, &r, &r );
        }
}

// Event functions.

static MouseEventType lastMouseState;
static volatile Boolean eventWaitInterrupted = False;

BOOL THardwareInfo::getMouseEvent( MouseEventType& event ) noexcept
{
    if( !mousePresent )
        return False;

    union REGS r;
    r.w.ax = 3;
    int386( 0x33, &r, &r );

    event.buttons = r.h.bl;
    event.wheel = r.h.bh == 0 ? 0 : char(r.h.bh) > 0 ? mwDown : mwUp; // CuteMouse
    event.where.x = r.w.cx >> 3;
    event.where.y = r.w.dx >> 3;
    event.eventFlags = 0;
    event.controlKeyState = controlKeyState();

    if( event.buttons != lastMouseState.buttons ||
        event.wheel != 0 ||
        event.where.x != lastMouseState.where.x ||
        event.where.y != lastMouseState.where.y )
        {
        lastMouseState = event;
        return True;
        }
    return False;
}

BOOL THardwareInfo::getKeyEvent( TEvent& event ) noexcept
{
    if( !keyWaiting() )
        return False;

    union REGS r;
    r.h.ah = 0;
    int386( 0x16, &r, &r );

    ushort keyCode = r.w.ax;
    ushort shiftState = controlKeyState();

    // Views compare keyCode against the kb* constants, which for these
    // combinations are Borland's own values rather than what an enhanced
    // BIOS reports: Ctrl+Ins/Del come back as 0x9200/0x9300, and with
    // NumLock off a shifted keypad Ins/Del is the character the shift
    // un-inverts to. Borland's drivers delivered the kb* values - the real
    // mode INT 09H hook rewrites the BIOS buffer, the console driver has its
    // own key tables - so do the same here, or TEditor's key map (a raw
    // comparison, unlike TKey) never sees a Ctrl+Del.
    switch( keyCode )
        {
        case 0x9200: keyCode = kbCtrlIns; break;
        case 0x9300: keyCode = kbCtrlDel; break;
        case 0x5230: if( shiftState & kbShift ) keyCode = kbShiftIns; break;
        case 0x532E: if( shiftState & kbShift ) keyCode = kbShiftDel; break;
        }

    event.what = evKeyDown;
    event.keyDown.keyCode = keyCode;
    event.keyDown.controlKeyState = shiftState;
    return True;
}

void THardwareInfo::waitForEvents( int timeoutMs ) noexcept
{
    TScreen::flushScreen();

    uint64_t deadline = timeoutMs < 0 ? (uint64_t) -1
                                      : getTickCountMs() + timeoutMs;
    eventWaitInterrupted = False;
    while( !eventWaitInterrupted && getTickCountMs() < deadline )
        {
        union REGS r;
        // Keyboard waiting?
        if( keyWaiting() )
            break;
        // Mouse state changed?
        if( mousePresent )
            {
            r.w.ax = 3;
            int386( 0x33, &r, &r );
            if( (r.h.bl != lastMouseState.buttons) || r.h.bh != 0 ||
                (r.w.cx >> 3) != lastMouseState.where.x ||
                (r.w.dx >> 3) != lastMouseState.where.y )
                break;
            }
        // Give up the rest of the time slice (DPMI yield).
        r.w.ax = 0x1680;
        int386( 0x2F, &r, &r );
        }
}

void THardwareInfo::interruptEventWait() noexcept
{
    eventWaitInterrupted = True;
}

BOOL THardwareInfo::setClipboardText( TStringView ) noexcept
{
    return False;
}

BOOL THardwareInfo::requestClipboardText( void (&)( TStringView ) ) noexcept
{
    return False;
}

// System functions.

static void (__interrupt far *oldInt1B)() = 0;
static void (__interrupt far *oldInt23)() = 0;

static void __interrupt far ctrlBreakInt()
{
    TSystemError::ctrlBreakHit = True;
}

static int __far critErrorHandler( unsigned deverror, unsigned errcode,
                                   unsigned __far *devhdr )
{
    (void) deverror; (void) errcode; (void) devhdr;
    return _HARDERR_FAIL;
}

// setCtrlBrkHandler/setCritErrorHandler are inline no-ops in hardware.h for
// this configuration, so the hooks are managed from the constructor and
// destructor instead.
static void installDosHooks()
{
    if( oldInt1B == 0 )
        {
        oldInt1B = _dos_getvect( 0x1B );
        oldInt23 = _dos_getvect( 0x23 );
        _dos_setvect( 0x1B, ctrlBreakInt );
        _dos_setvect( 0x23, ctrlBreakInt );
        _harderr( critErrorHandler );
        }
}

static void removeDosHooks()
{
    if( oldInt1B != 0 )
        {
        _dos_setvect( 0x1B, oldInt1B );
        _dos_setvect( 0x23, oldInt23 );
        oldInt1B = 0;
        oldInt23 = 0;
        }
}

#endif // __WATCOMC__ && __FLAT__ && __DOS__
