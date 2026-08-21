/*------------------------------------------------------------*/
/* filename -       wcdir.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*          Borland dir.h functions implemented on top of     */
/*          Open Watcom's RTL. Only part of the Watcom build. */
/*------------------------------------------------------------*/

#if defined( __WATCOMC__ )

#include <tvision/compat/borland/dir.h>

#include <string.h>
#include <direct.h>

// Copies at most size-1 characters from a length-delimited component and
// always null-terminates. Returns whether the component was non-empty.
static int copyComponent( char *dst, const char *start, const char *end,
                          unsigned size )
{
    if( end == start )
        return 0;
    if( dst )
        {
        unsigned n = (unsigned) (end - start);
        if( n > size - 1 )
            n = size - 1;
        memcpy( dst, start, n );
        dst[n] = '\0';
        }
    return 1;
}

int fnsplit( const char *pathP, char *driveP, char *dirP,
             char *nameP, char *extP )
{
    int flags = 0;
    if( driveP )
        memset( driveP, 0, MAXDRIVE );
    if( dirP )
        memset( dirP, 0, MAXDIR );
    if( nameP )
        memset( nameP, 0, MAXFILE );
    if( extP )
        memset( extP, 0, MAXEXT );
    if( pathP && *pathP )
        {
        unsigned len = strlen( pathP );
        const char *pathEnd = pathP + len;
        const char *caretP = 0;
        const char *slashP = 0;     // Rightmost slash.
        const char *lastDotP = 0;   // Last dot in filename.
        const char *firstDotP = 0;  // First dot in filename.
        unsigned i;
        for( i = len - 1; i < len; --i )
            switch( pathP[i] )
                {
                case '?':
                case '*':
                    // Wildcards are only detected in filename or extension.
                    if( !slashP )
                        flags |= WILDCARDS;
                    break;
                case '.':
                    if( !slashP )
                        {
                        if( !lastDotP )
                            lastDotP = pathP + i;
                        firstDotP = pathP + i;
                        }
                    break;
                case '\\':
                case '/':
                    if( !slashP )
                        slashP = pathP + i;
                    break;
                case ':':
                    if( i == 1 )
                        {
                        caretP = pathP + i;
                        i = 0;  // Exit loop; don't check the drive letter.
                        }
                default:
                    ;
                }
        // These variables point after the last character of each component.
        const char *driveEnd = caretP ? caretP + 1 : pathP;
        const char *dirEnd = slashP ? slashP + 1 : driveEnd;
        const char *nameEnd = lastDotP ? lastDotP : pathEnd;
        // Special case: path ends with '.' or '..', thus there's no filename.
        if( lastDotP == pathEnd - 1 && lastDotP - firstDotP < 2 &&
            firstDotP == dirEnd )
            dirEnd = nameEnd = pathEnd;
        // Copy components and set flags.
        if( copyComponent( driveP, pathP, driveEnd, MAXDRIVE ) )
            flags |= DRIVE;
        if( copyComponent( dirP, driveEnd, dirEnd, MAXDIR ) )
            flags |= DIRECTORY;
        if( copyComponent( nameP, dirEnd, nameEnd, MAXFILE ) )
            flags |= FILENAME;
        if( copyComponent( extP, nameEnd, pathEnd, MAXEXT ) )
            flags |= EXTENSION;
        }
    return flags;
}

// Appends a null-terminated string, bounded by the destination's total size.
static unsigned appendStr( char *dst, unsigned n, const char *src,
                           unsigned size )
{
    while( *src && n < size - 1 )
        dst[n++] = *src++;
    dst[n] = '\0';
    return n;
}

void fnmerge( char *pathP, const char *driveP, const char *dirP,
              const char *nameP, const char *extP )
{
    unsigned n = 0;
    pathP[0] = '\0';
    if( driveP && *driveP )
        {
        n = appendStr( pathP, n, driveP, MAXPATH );
        if( pathP[n-1] != ':' )
            n = appendStr( pathP, n, ":", MAXPATH );
        }
    if( dirP && *dirP )
        {
        n = appendStr( pathP, n, dirP, MAXPATH );
        if( pathP[n-1] != '\\' && pathP[n-1] != '/' )
            n = appendStr( pathP, n, "\\", MAXPATH );
        }
    if( nameP && *nameP )
        n = appendStr( pathP, n, nameP, MAXPATH );
    if( extP && *extP )
        {
        if( *extP != '.' )
            n = appendStr( pathP, n, ".", MAXPATH );
        appendStr( pathP, n, extP, MAXPATH );
        }
}

int getcurdir( int drive, char *direc )
{
    // direc is an array of length MAXDIR where the null-terminated directory
    // name is placed, without drive specification nor leading backslash.
    // Drive 0 is the default drive, 1 is drive A, etc.
    char buf[MAXPATH];
    if( _getdcwd( drive, buf, MAXPATH ) )
        {
        // Skip the "X:\" prefix.
        const char *p = buf;
        if( p[0] && p[1] == ':' )
            p += 2;
        if( *p == '\\' || *p == '/' )
            ++p;
        unsigned n = strlen( p );
        if( n > MAXDIR - 1 )
            n = MAXDIR - 1;
        memcpy( direc, p, n );
        direc[n] = '\0';
        return 0;
        }
    return -1;
}

#endif // __WATCOMC__
