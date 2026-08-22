#ifndef TVISION_IOSFWD_H
#define TVISION_IOSFWD_H

#ifdef __BORLANDC__

#include <_defs.h>

class _EXPCLASS ostream;
class _EXPCLASS streambuf;
typedef long streampos;
typedef long streamoff;

#elif defined( __WATCOMC__ )

// Watcom's <iosfwd> has no streampos or streamoff; they are in <ios>. And
// declaring ostream and streambuf by hand clashes with the real std::
// classes as soon as <iostream> is included. Use the real headers, as the
// branch below does.
#include <iosfwd>
#include <ios>

using std::ostream;
using std::streambuf;
using std::streampos;
using std::streamoff;

#else

#include <iosfwd>

using std::ostream;
using std::streambuf;
using std::streampos;
using std::streamoff;

#endif // __BORLANDC__

#endif // TVISION_IOSFWD_H
