/*  _defs.h

    Common definitions for pointer size and calling conventions.

    Calling conventions:
    _RTLENTRY       Specifies the calling convention used by the RTL

    _USERENTRY      Specifies the calling convention the RTL expects user
                    compiled functions to use (for callbacks)

    Export (and size for DOS) information:
    _EXPCLASS       Exports class if building DLL version of library
                    For DOS16 also provides size information

    _EXPDATA        Exports data if building DLL version of library

    _EXPFUNC        Exports function if building DLL version of library
                    For DOS16 also provides size information

    _FAR            Promotes data pointers to far in DLLs (DOS16 only)

    Obsolete versions:
    _Cdecl          Use _RTLENTRY
    _CLASSTYPE      Use _EXPCLASS
    _FARFUNC        Use _EXPFUNC
    _FARCALL        Use _EXPFUNC and declare function explicity __far

    Copyright (c) 1991, 1992 by Borland International
    All Rights Reserved.
*/

#ifdef __BORLANDC__
#include <_defs.h>
#else

#ifndef TVISION_COMPAT__DEFS_H
#define TVISION_COMPAT__DEFS_H

#define _RTLENTRY  __cdecl
#define _Cdecl      _RTLENTRY

#if defined( __WATCOMC__ ) && !defined( __FLAT__ ) && !defined( __DLL__ )
// Watcom's own compiler doesn't predefine _FAR/_NEAR the way Borland's
// native <_defs.h> does. In the common (non-DLL) 16-bit large-model case,
// data is already far by default, so these can safely expand to nothing,
// matching what tv.h does for __FLAT__ builds. (_NEAR is already handled
// by ttypes.h before this header is reached.)
#define _FAR
#endif

#endif // TVISION_COMPAT__DEFS_H

#endif // __BORLANDC__
