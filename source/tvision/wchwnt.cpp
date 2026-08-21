/*------------------------------------------------------------*/
/* filename -       wchwnt.cpp                                */
/*                                                            */
/* function(s)                                                */
/*          THardwareInfo for Open Watcom's Win32 target.     */
/*          Implements the "flat" driver interface of         */
/*          hardware.h on top of the Win32 Console API, as    */
/*          hardwrvr.cpp does for Borland, but against the    */
/*          non-Borland member set (TScreenCell screen        */
/*          buffers, 64-bit tick counter). Only part of the   */
/*          Watcom build.                                     */
/*------------------------------------------------------------*/

#if defined( __WATCOMC__ ) && defined( __FLAT__ ) && defined( __NT__ )

#define Uses_TKeys
#define Uses_TEvent
#define Uses_TScreen
#define Uses_TSystemError
#define Uses_THardwareInfo
#include <tvision/tv.h>
#include <tvision/compat/borland/iostream.h>

#include <string.h>

// Unlike Borland's DPMI32 target, which shares hardwrvr.cpp's Win32 code path
// and has to reach part of the Console API through GetProcAddress, Watcom's NT
// target is a native Win32 program: everything below links statically.

// Restored when the application exits.
static UINT startupCpInput;
static UINT startupCpOutput;

THardwareInfo::THardwareInfo() noexcept
{
    platform = plWinNT;
    insertState = True;

    consoleHandle[cnInput] = GetStdHandle( STD_INPUT_HANDLE );
    consoleHandle[cnOutput] = GetStdHandle( STD_OUTPUT_HANDLE );
    if( !GetConsoleMode( consoleHandle[cnInput], &consoleMode ) )
        {
        cerr << "Error: standard input is being redirected or is not a "
                "Win32 console." << endl;
        ExitProcess( 1 );
        }
    GetConsoleCursorInfo( consoleHandle[cnOutput], &crInfo );
    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );

    // Turbo Vision draws single-byte 'extended ASCII' text (frames, shadows,
    // the desktop pattern), so the console has to interpret it in an OEM
    // codepage. When Borland's build was written that was the default; a
    // modern console may well be using UTF-8 instead, which turns every
    // character above 0x7F into a replacement glyph.
    startupCpInput = GetConsoleCP();
    startupCpOutput = GetConsoleOutputCP();
    SetConsoleCP( GetOEMCP() );
    SetConsoleOutputCP( GetOEMCP() );

    consoleHandle[cnStartup] = consoleHandle[cnOutput];
    consoleHandle[cnOutput] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        CONSOLE_TEXTMODE_BUFFER,
        0);
    // Force the screen buffer size to match the window size.
    // The Console API guarantees this, but some implementations
    // are not compliant (e.g. Wine).
    sbInfo.dwSize.X = sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1;
    sbInfo.dwSize.Y = sbInfo.srWindow.Bottom - sbInfo.srWindow.Top + 1;
    SetConsoleScreenBufferSize( consoleHandle[cnOutput], sbInfo.dwSize );

    consoleMode |= ENABLE_WINDOW_INPUT; // Report changes in buffer size
    consoleMode &= ~ENABLE_PROCESSED_INPUT; // Report CTRL+C and SHIFT+Arrow events.
    consoleMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT); // Report Ctrl+S.
    SetConsoleMode( consoleHandle[cnInput], consoleMode );
    // The following flags were introduced in later Windows versions, so use a
    // separate call to SetConsoleMode, just in case it fails.
    consoleMode |= ENABLE_EXTENDED_FLAGS;   /* Disable the Quick Edit mode, */
    consoleMode &= ~ENABLE_QUICK_EDIT_MODE; /* which inhibits the mouse.    */
    SetConsoleMode( consoleHandle[cnInput], consoleMode );
}

THardwareInfo::~THardwareInfo()
{
    restoreConsole();
    CloseHandle( consoleHandle[cnOutput] );
    SetConsoleCP( startupCpInput );
    SetConsoleOutputCP( startupCpOutput );
}

uint64_t THardwareInfo::getTickCountMs() noexcept
{
    return GetTickCount();
}

// Caret functions.

ushort THardwareInfo::getCaretSize() noexcept
{
    return (ushort) crInfo.dwSize;
}

void THardwareInfo::setCaretSize( ushort size ) noexcept
{
    if( size == 0 )
        {
        crInfo.bVisible = FALSE;
        crInfo.dwSize = 1;
        }
    else
        {
        crInfo.bVisible = TRUE;
        crInfo.dwSize = size;
        }

    SetConsoleCursorInfo( consoleHandle[cnOutput], &crInfo );
}

void THardwareInfo::setCaretPosition( ushort x, ushort y ) noexcept
{
    COORD coord;
    coord.X = (SHORT) x;
    coord.Y = (SHORT) y;
    SetConsoleCursorPosition( consoleHandle[cnOutput], coord );
}

BOOL THardwareInfo::isCaretVisible() noexcept
{
    return crInfo.bVisible;
}

// Screen functions.

ushort THardwareInfo::getScreenRows() noexcept
{
    return sbInfo.dwSize.Y;
}

ushort THardwareInfo::getScreenCols() noexcept
{
    return sbInfo.dwSize.X;
}

ushort THardwareInfo::getScreenMode() noexcept
{
    // B/W and monochrome are not supported on NT; that would mean going
    // through the registry.
    ushort mode = TDisplay::smCO80;

    if( getScreenRows() > 25 )
        mode |= TDisplay::smFont8x8;
    return mode;
}

void THardwareInfo::setScreenMode( ushort mode ) noexcept
{
    COORD newSize = { 80, 25 };
    SMALL_RECT rect = { 0, 0, 79, 24 };

    if( mode & TDisplay::smFont8x8 )
        {
        newSize.Y = 50;
        rect.Bottom = 49;
        }

    COORD maxSize = GetLargestConsoleWindowSize( consoleHandle[cnOutput] );
    if( newSize.Y > maxSize.Y )
        {
        newSize.Y = maxSize.Y;
        rect.Bottom = newSize.Y - 1;
        }

    if( mode & TDisplay::smFont8x8 )
        {
        SetConsoleScreenBufferSize( consoleHandle[cnOutput], newSize );
        SetConsoleWindowInfo( consoleHandle[cnOutput], TRUE, &rect );
        }
    else
        {
        SetConsoleWindowInfo( consoleHandle[cnOutput], TRUE, &rect );
        SetConsoleScreenBufferSize( consoleHandle[cnOutput], newSize );
        }

    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );
}

void THardwareInfo::clearScreen( ushort w, ushort h ) noexcept
{
    COORD coord = { 0, 0 };
    DWORD read;

    FillConsoleOutputAttribute( consoleHandle[cnOutput], 0x07, (DWORD) w*h, coord, &read );
    FillConsoleOutputCharacterA( consoleHandle[cnOutput], ' ', (DWORD) w*h, coord, &read );
}

void THardwareInfo::flushScreen() noexcept
{
}

void THardwareInfo::screenWrite( ushort x, ushort y, TScreenCell *buf,
                                 DWORD len ) noexcept
{
    // Turbo Vision's screen buffer holds two-byte DOS-style cells, so every
    // run has to be widened into CHAR_INFO before the Console API can take
    // it. A fixed-size chunk avoids an allocation per write.
    enum { chunkLen = 256 };
    CHAR_INFO cells[chunkLen];

    while( len > 0 )
        {
        DWORD count = len < (DWORD) chunkLen ? len : (DWORD) chunkLen;
        for( DWORD i = 0; i < count; ++i )
            {
            cells[i].Char.AsciiChar = buf[i].character;
            cells[i].Attributes = buf[i].attribute;
            }

        COORD size, from;
        SMALL_RECT to;
        size.X = (SHORT) count;
        size.Y = 1;
        from.X = 0;
        from.Y = 0;
        to.Left = (SHORT) x;
        to.Top = (SHORT) y;
        to.Right = (SHORT) ( x + count - 1 );
        to.Bottom = (SHORT) y;
        WriteConsoleOutput( consoleHandle[cnOutput], cells, size, from, &to );

        buf += count;
        x = (ushort) ( x + count );
        len -= count;
        }
}

TScreenCell *THardwareInfo::allocateScreenBuffer() noexcept
{
    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );
    int x = sbInfo.dwSize.X, y = sbInfo.dwSize.Y;

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
    SetConsoleActiveScreenBuffer( consoleHandle[cnOutput] );
    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );
}

void THardwareInfo::restoreConsole() noexcept
{
    SetConsoleActiveScreenBuffer( consoleHandle[cnStartup] );
}

// Mouse functions.

DWORD THardwareInfo::getButtonCount() noexcept
{
    DWORD num = 0;
    GetNumberOfConsoleMouseButtons( &num );
    return num;
}

void THardwareInfo::cursorOn() noexcept
{
    SetConsoleMode( consoleHandle[cnInput], consoleMode | ENABLE_MOUSE_INPUT );
}

void THardwareInfo::cursorOff() noexcept
{
    SetConsoleMode( consoleHandle[cnInput], consoleMode & ~ENABLE_MOUSE_INPUT );
}

// Event functions.

BOOL THardwareInfo::getMouseEvent( MouseEventType& event ) noexcept
{
    if( !pendingEvent )
        {
        GetNumberOfConsoleInputEvents( consoleHandle[cnInput], &pendingEvent );
        if( pendingEvent )
            ReadConsoleInput( consoleHandle[cnInput], &irBuffer, 1, &pendingEvent );
        }

    if( pendingEvent && irBuffer.EventType == MOUSE_EVENT )
        {
        event.where.x = irBuffer.Event.MouseEvent.dwMousePosition.X;
        event.where.y = irBuffer.Event.MouseEvent.dwMousePosition.Y;
        event.buttons = irBuffer.Event.MouseEvent.dwButtonState;
        event.eventFlags = irBuffer.Event.MouseEvent.dwEventFlags;
        event.controlKeyState = irBuffer.Event.MouseEvent.dwControlKeyState;

        // Rotation sense is represented by the sign of dwButtonState's high word
        int positive = !(irBuffer.Event.MouseEvent.dwButtonState & 0x80000000);
        if( irBuffer.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED )
            event.wheel = positive ? mwUp : mwDown;
        else if( irBuffer.Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED )
            event.wheel = positive ? mwRight : mwLeft;
        else
            event.wheel = 0;

        pendingEvent = 0;
        return True;
        }
    return False;
}

BOOL THardwareInfo::getKeyEvent( TEvent& event ) noexcept
{
    do  {
        if( !pendingEvent )
            {
            GetNumberOfConsoleInputEvents( consoleHandle[cnInput], &pendingEvent );
            if( pendingEvent )
                ReadConsoleInput( consoleHandle[cnInput], &irBuffer, 1, &pendingEvent );
            else
                return False;
            }

        // Pending mouse events will be read on the next polling loop.
        if( pendingEvent && irBuffer.EventType != MOUSE_EVENT )
            {
            pendingEvent = 0;

            if( irBuffer.EventType == KEY_EVENT && irBuffer.Event.KeyEvent.bKeyDown )
                {
                event.what = evKeyDown;
                event.keyDown.charScan.scanCode = irBuffer.Event.KeyEvent.wVirtualScanCode;
                event.keyDown.charScan.charCode = irBuffer.Event.KeyEvent.uChar.AsciiChar;
                event.keyDown.controlKeyState = irBuffer.Event.KeyEvent.dwControlKeyState;

                if( event.keyDown.keyCode == 0x2A00 || event.keyDown.keyCode == 0x1D00 ||
                    event.keyDown.keyCode == 0x3600 || event.keyDown.keyCode == 0x3800 ||
                    event.keyDown.keyCode == 0x3A00 || event.keyDown.keyCode == 0x5B00 ||
                    event.keyDown.keyCode == 0x5C00 )
                    // Discard standalone Shift, Ctrl, Alt, Caps Lock, Windows keys.
                    event.keyDown.keyCode = kbNoKey;
                else if( (event.keyDown.controlKeyState & kbLeftCtrl) &&
                         (event.keyDown.controlKeyState & kbRightAlt) &&
                         event.keyDown.charScan.charCode == '\0' )
                    // We cannot tell for sure if the right Alt key is AltGr, since
                    // that depends on the keyboard layout, but it is certain that
                    // AltGr automatically adds the left Ctrl flag.
                    // If both of these are set but no text is produced, discard the
                    // whole event since we don't want AltGr to be handled as Ctrl+Alt.
                    event.keyDown.keyCode = kbNoKey;
                else if( (event.keyDown.controlKeyState & kbCtrlShift) &&
                         (event.keyDown.controlKeyState & kbAltShift) &&
                         event.keyDown.charScan.charCode != '\0' )
                    // If Ctrl+Alt produces text, we are dealing with AltGr. In this case,
                    // discard the Ctrl and Alt modifiers.
                    event.keyDown.controlKeyState &= ~(kbCtrlShift | kbAltShift);
                else if( irBuffer.Event.KeyEvent.wVirtualScanCode < 89 )
                    {
                    // Convert NT style virtual scan codes to PC BIOS codes.
                    uchar index = (uchar) irBuffer.Event.KeyEvent.wVirtualScanCode;
                    if ((event.keyDown.controlKeyState & kbAltShift) && AltCvt[index] != 0)
                        event.keyDown.keyCode = AltCvt[index];
                    else if ((event.keyDown.controlKeyState & kbCtrlShift) && CtrlCvt[index] != 0)
                        event.keyDown.keyCode = CtrlCvt[index];
                    else if ((event.keyDown.controlKeyState & kbShift) && ShiftCvt[index] != 0)
                        event.keyDown.keyCode = ShiftCvt[index];
                    else if ( !(event.keyDown.controlKeyState & (kbShift | kbCtrlShift | kbAltShift)) &&
                              NormalCvt[index] != 0 )
                        event.keyDown.keyCode = NormalCvt[index];
                    }

                /* Set/Reset insert flag.
                 */
                if( event.keyDown.keyCode == kbIns )
                    insertState = !insertState;

                if( insertState )
                    event.keyDown.controlKeyState |= kbInsState;

                if( event.keyDown.keyCode != kbNoKey )
                    return True;
                }
            else if( irBuffer.EventType == WINDOW_BUFFER_SIZE_EVENT )
                {
                event.what = evCommand;
                event.message.command = cmScreenChanged;
                event.message.infoPtr = 0;
                return True;
                }
            }
        } while( !pendingEvent );

    return False;
}

void THardwareInfo::waitForEvents( int timeoutMs ) noexcept
{
    if( !pendingEvent )
        WaitForSingleObject( consoleHandle[cnInput], timeoutMs < 0 ? INFINITE : timeoutMs );
}

void THardwareInfo::interruptEventWait() noexcept
{
    // Not implemented. This would only be necessary in a multi-threaded
    // application, and nothing in this build creates threads.
}

BOOL THardwareInfo::setClipboardText( TStringView text ) noexcept
{
    BOOL result = False;
    if( OpenClipboard( 0 ) )
        {
        HGLOBAL hData = NULL;
        char *pData;
        if( EmptyClipboard() &&
            (result = text.empty()) == 0 &&
            (hData = GlobalAlloc( GMEM_MOVEABLE, text.size() + 1 )) != 0 &&
            (pData = (char *) GlobalLock( hData )) != 0
          )
            {
            memcpy( pData, text.data(), text.size() );
            pData[text.size()] = '\0';
            GlobalUnlock( hData );
            result = SetClipboardData( CF_OEMTEXT, hData ) != 0;
            }
        CloseClipboard();
        if( hData && !result )
            GlobalFree( hData );
        }
    return result;
}

BOOL THardwareInfo::requestClipboardText( void (&accept)( TStringView ) ) noexcept
{
    BOOL result = False;
    if( OpenClipboard( 0 ) )
        {
        HGLOBAL hData;
        char *pData;
        if( (hData = GetClipboardData( CF_OEMTEXT )) != 0 &&
            (result = ((pData = (char *) GlobalLock( hData )) != 0)) == True
          )
            {
            accept( pData );
            GlobalUnlock( hData );
            }
        CloseClipboard();
        }
    return result;
}

#endif // __WATCOMC__ && __FLAT__ && __NT__
