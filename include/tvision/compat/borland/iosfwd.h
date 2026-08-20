#ifndef TVISION_IOSFWD_H
#define TVISION_IOSFWD_H

#ifdef __BORLANDC__

#include <_defs.h>

class _EXPCLASS ostream;
class _EXPCLASS streambuf;
typedef long streampos;
typedef long streamoff;

#elif defined( __WATCOMC__ )

// Watcom's own <iosfwd> doesn't declare streampos/streamoff (those live in
// <ios>), and forward-declaring ostream/streambuf by hand here conflicts
// with Watcom's real (std-namespaced) classes once something else pulls in
// <iostream>/<fstream>. So just use the real headers, same as the portable
// branch below.
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
