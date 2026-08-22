/*------------------------------------------------------------*/
/* filename -       tvcursor.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TView resetCursor member function         */
/*------------------------------------------------------------*/
/*
 *      Turbo Vision - Version 2.0
 *
 *      Copyright (c) 1994 by Borland International
 *      All Rights Reserved.
 *
 */

#define Uses_TView
#define Uses_TGroup
#define Uses_TScreen
#define Uses_THardwareInfo
#include <tvision/tv.h>

#if !defined( __FLAT__ ) && !defined( __WATCOMC__ )
#error The 16-bit version of this file is in TVCURSOR.ASM
#else

#if defined( __WATCOMC__ ) && !defined( __FLAT__ )
#if !defined( __DOS_H )
#include <tvision/compat/borland/dos.h>
#endif  // __DOS_H

// Real mode has no THardwareInfo caret API: TVCURSOR.ASM drove INT 10h
// directly, and so does this. The caret size travelling through
// TVCursor::computeCaretSize is therefore the same start/end percentage pair
// that TScreen::cursorLines holds, not the single percentage the 32-bit
// targets pass to THardwareInfo::setCaretSize.

static void dosSetCaretPosition( int x, int y )
{
    union REGS r;

    r.h.ah = 2;
    r.h.bh = 0;
    r.h.dh = (uchar) y;
    r.h.dl = (uchar) x;
    int86( 0x10, &r, &r );
}

static void dosSetCaretSize( int caretSize, Boolean insertMode )
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
            r.w.ax = 0x1130;
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
#endif // __WATCOMC__ && !__FLAT__

struct TVCursor {

    TView *self;
    int x, y;

    void resetCursor(TView *);
    int computeCaretSize();
    Boolean caretCovered(TView *) const;
    int decideCaretSize() const;

};

void TView::resetCursor()
{
    TVCursor().resetCursor(this);
}

void TVCursor::resetCursor(TView *p)
{
    self = p;
    x = self->cursor.x;
    y = self->cursor.y;
    int caretSize = computeCaretSize();
#if defined( __WATCOMC__ ) && !defined( __FLAT__ )
    if (caretSize)
        dosSetCaretPosition(x, y);
    dosSetCaretSize(caretSize, Boolean(self->state & sfCursorIns));
#else
    if (caretSize)
        THardwareInfo::setCaretPosition(x, y);
    THardwareInfo::setCaretSize(caretSize);
#endif
}

int TVCursor::computeCaretSize()
{
    if (!(~self->state & (sfVisible | sfCursorVis | sfFocused)))
    {
        TView *v = self;
        while (0 <= y && y < v->size.y && 0 <= x && x < v->size.x)
        {
            y += v->origin.y;
            x += v->origin.x;
            if (v->owner)
            {
                if (v->owner->state & sfVisible)
                {
                    if (caretCovered(v))
                        break;
                    v = v->owner;
                }
                else break;
            }
            else return decideCaretSize();
        }
    }
    return 0;
}

Boolean TVCursor::caretCovered(TView *v) const
{
    TView *u = v->owner->last->next;
    for (; u != v; u = u->next)
    {
        if ( (u->state & sfVisible)
             && (u->origin.y <= y && y < u->origin.y + u->size.y)
             && (u->origin.x <= x && x < u->origin.x + u->size.x) )
            return True;
    }
    return False;
}

int TVCursor::decideCaretSize() const
{
#if defined( __WATCOMC__ ) && !defined( __FLAT__ )
    // sfCursorIns is applied by dosSetCaretSize, which works in scan lines.
    return TScreen::cursorLines;
#else
    if (self->state & sfCursorIns)
        return 100;
    return TScreen::cursorLines & 0x0F;
#endif
}

#endif // __FLAT__
