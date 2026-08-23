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
// Real mode has no THardwareInfo caret functions. WCHW16.CPP drives the
// caret through INT 10h, as TVCURSOR.ASM did, so the size computed by
// TVCursor::computeCaretSize is the start/end pair held in
// TScreen::cursorLines, not the single percentage the 32-bit target uses.
#include <tvision/internal/wcdos16.h>
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
