/*------------------------------------------------------------*/
/* filename -       wcdos16.h                                 */
/*                                                            */
/* function(s)                                                */
/*                  real mode services for the Open Watcom    */
/*                  16-bit build. Not used by any other       */
/*                  compiler or target.                       */
/*------------------------------------------------------------*/

// Borland's 16-bit build gets these from its .asm files, which this build
// cannot assemble, and the 32-bit build gets the same jobs done through
// THardwareInfo, which real mode does not have. The implementations are in
// WCHW16.CPP, kept out of the files that call them so that those stay as
// close to upstream as they can.

#ifndef TVISION_WCDOS16_H
#define TVISION_WCDOS16_H

#if defined( __WATCOMC__ ) && !defined( __FLAT__ )

#define Uses_TPoint
#define Uses_TEvent
#define Uses_TEventQueue
#define Uses_THardwareInfo
#include <tvision/ttypes.h>
#include <tvision/objects.h>
#include <tvision/hardware.h>
#include <tvision/system.h>

// Caret, for TVCURSOR.CPP. TVCURSOR.ASM drove the caret through INT 10h, and
// so do these. caretSize is a pair of percentages, high byte to low, matching
// what TVCursor::computeCaretSize builds out of TScreen::cursorLines; zero
// hides the caret.

void dosSetCaretPosition( int x, int y ) noexcept;
void dosSetCaretSize( int caretSize, Boolean insertMode ) noexcept;

// Display, for TSCREEN.CPP. Borland reaches the BIOS there through its
// pseudo-register variables and its own videoInt(); these do the same work
// through int86(). The cursor type is a pair of percentages, high byte to
// low, as TDisplay::getCursorType and setCursorType define it.

ushort dosGetCursorType() noexcept;
void dosSetCursorType( ushort ct ) noexcept;
int dosIsEGAorVGA() noexcept;
void dosClearScreen( uchar w, uchar h ) noexcept;
ushort dosGetRows() noexcept;
ushort dosGetCols() noexcept;
ushort dosGetCrtMode() noexcept;
void dosSetCrtMode( ushort mode ) noexcept;

// Mouse, for TMOUSE.CPP. THWMouse keeps buttonCount and handlerInstalled as
// its own protected/private static members, so these take or return the
// values that member functions need rather than reaching into the class
// themselves; getEvent needs neither and takes the caller's MouseEventType
// directly.

int dosMouseReset( uchar &buttonCount ) noexcept;
void dosMouseShow() noexcept;
void dosMouseHide() noexcept;
void dosMouseSetRange( ushort rx, ushort ry ) noexcept;
void dosMouseGetEvent( MouseEventType &me ) noexcept;
void dosMouseRegisterHandler( unsigned mask, void (_FAR *func)() ) noexcept;

// For SYSERR.CPP's checkIDE(), which fires INT 12h with two values in AX and
// BX for TSystemError's Int11trap to see. AX and BX are the same value both
// times it is called, so this takes one.

void dosFireInt12( int ax_bx ) noexcept;

// Event, for TEVENT.CPP.
//
// tvMouseIntBody is declared a friend of TEventQueue in this header's own
// class (system.h), under exactly this name and signature, so it can be
// defined in any file; TEVENT.CPP no longer needs to be the one that does.
// It is what WCSTUBS.ASM's tvMouseIntStub calls once it has saved the
// driver's registers and switched to DGROUP.
//
// dosReadKeyPress reads one key with the BIOS instead of Borland's inline
// INT 16h, and returns whether one was waiting; on a miss it still sets
// ev.what so the caller can tell without a second check.

extern "C" void __cdecl tvMouseIntBody( unsigned flag, unsigned buttons,
                                        unsigned x, unsigned y );
int dosReadKeyPress( TEvent &ev ) noexcept;

#endif // __WATCOMC__ && !__FLAT__

#endif // TVISION_WCDOS16_H
