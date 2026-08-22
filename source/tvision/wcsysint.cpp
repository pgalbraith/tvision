/*------------------------------------------------------------*/
/* filename -       wcsysint.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TSystemError suspend/resume and the       */
/*                  system interrupt handlers, for the Open   */
/*                  Watcom 16-bit DOS build only.             */
/*------------------------------------------------------------*/

/*
 *  The Borland 16-bit build takes these entry points from SYSINT.ASM, which
 *  is written in TASM's dialect against Borland's name mangling and their
 *  DPMI16 host.  This is a fresh implementation on top of Open Watcom's own
 *  RTL - __interrupt functions with an INTPACK register image, plus
 *  _dos_getvect/_dos_setvect - for real mode, the only 16-bit DOS target
 *  Open Watcom has.
 *
 *  Two things SYSINT.ASM does are deliberately not reproduced here:
 *
 *    - It hooks INT 21H so that a critical error can be answered by
 *      re-issuing the entire DOS call.  Answering DOS's own INT 24H with
 *      "retry" gets the same result without putting a hand-written handler
 *      in front of every DOS call the program makes.
 *
 *    - Its single-floppy INT 21H handler prompts for a disk swap before
 *      DOS's own "Insert diskette for drive B:" message can scribble over
 *      the screen.  That only applies to a machine with exactly one floppy
 *      drive and no hard disk.
 */

#if defined( __WATCOMC__ ) && !defined( __FLAT__ )

#define Uses_TKeys
#define Uses_TEvent
#define Uses_TScreen
#define Uses_TSystemError
#define Uses_THardwareInfo
#include <tvision/tv.h>

#if !defined( __DOS_H )
#include <tvision/compat/borland/dos.h>
#endif  // __DOS_H

#if !defined( __CONIO_H )
#include <conio.h>              // inp()
#endif  // __CONIO_H

/*------------------------------------------------------------*/
/* ROM BIOS workspace                                         */
/*------------------------------------------------------------*/

const unsigned
    biosKeyFlags    = 0x17,     // Shift key state.
    biosKeyBufHead  = 0x1A,
    biosKeyBufTail  = 0x1C,
    biosBreakFlag   = 0x71,     // Bit 7 set by the BIOS on Ctrl+Break.
    biosKeyFlags3   = 0x96,     // Bit 1: the last scan code was an E0 prefix.
    biosKeyBufOrg   = 0x80,
    biosKeyBufEnd   = 0x82;

inline uchar _FAR *biosByte( unsigned offset )
{
    return (uchar _FAR *) MAKELONG( tvBiosSel, offset );
}

inline ushort _FAR *biosWord( unsigned offset )
{
    return (ushort _FAR *) MAKELONG( tvBiosSel, offset );
}

/*------------------------------------------------------------*/
/* Keyboard                                                   */
/*------------------------------------------------------------*/

const uchar
    scSpaceKey  = 0x39,
    scInsKey    = 0x52,
    scDelKey    = 0x53;

const uchar
    kbShiftKey  = 0x03,
    kbCtrlKey   = 0x04,
    kbAltKey    = 0x08;

// Key combinations the BIOS either drops or reports as something else.

static const struct
{
    uchar scanCode;
    uchar shiftMask;
    ushort keyCode;
} keyConvertTab[] =
{
    { scSpaceKey, kbAltKey,   kbAltSpace },
    { scInsKey,   kbCtrlKey,  kbCtrlIns  },
    { scInsKey,   kbShiftKey, kbShiftIns },
    { scDelKey,   kbCtrlKey,  kbCtrlDel  },
    { scDelKey,   kbShiftKey, kbShiftDel }
};

const int keyConvertCnt = sizeof( keyConvertTab ) / sizeof( keyConvertTab[0] );

static void (__interrupt _FAR *oldInt09)();
static void (__interrupt _FAR *oldInt1B)();
static void (__interrupt _FAR *oldInt23)();
static void (__interrupt _FAR *oldInt24)();

static Boolean handlersInstalled = False;

static void __interrupt _FAR int09Handler()
{
    // Remember the buffer tail before the BIOS gets the key: if it queues a
    // keystroke we want to replace, the tail is how we find the slot it used.
    ushort tail = *biosWord( biosKeyBufTail );
    uchar scan;
    uchar flags;
    int i;

    oldInt09();

    // Borland's handler sampled port 60H *before* chaining, trusting the
    // keyboard controller to hand the BIOS the same byte again. A controller
    // that pops its output buffer on every read (QEMU's i8042, for one)
    // instead serves the BIOS the *next* byte, so the E0 prefix of a grey
    // cursor key vanished and the key that followed was delivered twice.
    // So the BIOS goes first, and the scan code comes from the keystroke it
    // queued; port 60H is only read when it queued nothing - for the keys
    // the conversion table exists for, the ones old BIOSes drop - and never
    // on the E0 prefix itself, when the next byte may already be waiting.
    flags = *biosByte( biosKeyFlags );
    if( tail != *biosWord( biosKeyBufTail ) )
        {
        scan = (uchar) (*biosWord( tail ) >> 8);
        if( scan == 0x92 || scan == 0x93 )  // Enhanced BIOS Ctrl+Ins/Del.
            scan -= 0x40;
        }
    else if( (*biosByte( biosKeyFlags3 ) & 0x02) != 0 )
        scan = 0x80;                    // E0 prefix: nothing to convert.
    else
        scan = (uchar) inp( 0x60 );     // Already consumed: the byte stays.

    if( (scan & 0x80) == 0 )            // Key presses only, not releases.
        for( i = 0; i < keyConvertCnt; ++i )
            if( keyConvertTab[i].scanCode == scan &&
                (flags & keyConvertTab[i].shiftMask) != 0 )
                {
                if( tail == *biosWord( biosKeyBufTail ) )
                    {
                    // The BIOS queued nothing, so make room ourselves.
                    ushort next = tail + 2;
                    if( next == *biosWord( biosKeyBufEnd ) )
                        next = *biosWord( biosKeyBufOrg );
                    if( next == *biosWord( biosKeyBufHead ) )
                        break;          // Buffer is full.
                    *biosWord( biosKeyBufTail ) = next;
                    }
                *biosWord( tail ) = keyConvertTab[i].keyCode;
                break;
                }

    if( (*biosByte( biosBreakFlag ) & 0x80) != 0 )
        {
        *biosByte( biosBreakFlag ) &= 0x7F;
        TSystemError::ctrlBreakHit = True;
        }
}

static void __interrupt _FAR int1BHandler()
{
    *biosByte( biosBreakFlag ) &= 0x7F;
    TSystemError::ctrlBreakHit = True;
}

// Also stands in for INT 10H while resume() flushes a pending Ctrl+C, so
// that DOS's '^C' echo cannot reach the screen.

static void __interrupt _FAR iretHandler()
{
}

/*------------------------------------------------------------*/
/* Critical errors                                            */
/*------------------------------------------------------------*/

/*
 *  DOS enters the INT 24H handler on a stack of its own, which has nowhere
 *  near the room TSystemError::sysErr needs to format a message and paint a
 *  status line, and which is not in DGROUP - where Open Watcom's large data
 *  model expects SS to point. tvCallOnAltStack (WCSTUBS.ASM) puts both right
 *  before any ordinary C++ runs; the handler itself is __interrupt, which
 *  Watcom compiles to reach its statics through DS instead.
 */

extern "C" void __cdecl tvCallOnAltStack( void (_FAR *fn)() );

// INT 24H action codes.
const int critIgnore = 0, critRetry = 1, critAbort = 2, critFail = 3;

static unsigned critDeviceError;
static unsigned critErrorCode;
static ushort _FAR *critDeviceHeader;
static int critResult;
static Boolean inCritErr = False;

static void _FAR critErrBody()
{
    // The low byte of the code INT 24H passes in DI indexes
    // TSystemError::errorString directly.
    short code = (short) (critErrorCode & 0xFF);
    uchar drive = (uchar) (critDeviceError & 0xFF);

    // 0xFE is the drive code SYSINT.ASM used for an error that names no
    // drive: the printer running out of paper, or any other character
    // device (bit 15 of the device header's attribute word).
    if( code == 9 ||
        ( (critDeviceError & 0x8000) != 0 &&
          (critDeviceHeader[2] & 0x8000) != 0 ) )
        drive = 0xFE;

    critResult = TSystemError::sysErrorFunc( code, drive ) == 0 ?
                 critRetry : critFail;
}

static void __interrupt _FAR int24Handler( union INTPACK r )
{
    critResult = critFail;
    if( !inCritErr )
        {
        inCritErr = True;
        critDeviceError = r.w.ax;
        critErrorCode = r.w.di;
        critDeviceHeader = (ushort _FAR *) MAKELONG( r.w.bp, r.w.si );
        tvCallOnAltStack( critErrBody );
        inCritErr = False;
        }
    r.w.ax = (unsigned short) critResult;
}

/*------------------------------------------------------------*/
/* Install and remove                                         */
/*------------------------------------------------------------*/

void TSystemError::resume() noexcept
{
    union REGS r;
    void (__interrupt _FAR *oldInt10)();

    if( handlersInstalled )
        return;

    // Save the state of DOS's break checking flag and clear it.
    r.x.ax = 0x3300;
    int86( 0x21, &r, &r );
    saveCtrlBreak = Boolean( r.h.dl );
    r.x.ax = 0x3301;
    r.h.dl = 0;
    int86( 0x21, &r, &r );

    oldInt09 = _dos_getvect( 0x09 );
    if( !inIDE )                        // The DOS IDE handles INT 09H itself.
        _dos_setvect( 0x09, int09Handler );

    oldInt1B = _dos_getvect( 0x1B );
    _dos_setvect( 0x1B, int1BHandler );
    *biosByte( biosBreakFlag ) &= 0x7F; // Ctrl+Break is off to begin with.

    oldInt23 = _dos_getvect( 0x23 );
    _dos_setvect( 0x23, iretHandler );

    oldInt24 = _dos_getvect( 0x24 );
    _dos_setvect( 0x24, (void (__interrupt _FAR *)()) int24Handler );

    handlersInstalled = True;

    // Make DOS act on a Ctrl+C that is already in the keyboard buffer, with
    // console output going nowhere while it does.
    oldInt10 = _dos_getvect( 0x10 );
    _dos_setvect( 0x10, iretHandler );
    r.h.ah = 0x0B;
    int86( 0x21, &r, &r );
    _dos_setvect( 0x10, oldInt10 );
}

void TSystemError::suspend() noexcept
{
    union REGS r;

    if( !handlersInstalled )
        return;

    _dos_setvect( 0x24, oldInt24 );
    _dos_setvect( 0x23, oldInt23 );
    _dos_setvect( 0x1B, oldInt1B );
    _dos_setvect( 0x09, oldInt09 );

    r.x.ax = 0x3301;
    r.h.dl = (uchar) saveCtrlBreak;
    int86( 0x21, &r, &r );

    handlersInstalled = False;
}

#endif // __WATCOMC__ && !__FLAT__
