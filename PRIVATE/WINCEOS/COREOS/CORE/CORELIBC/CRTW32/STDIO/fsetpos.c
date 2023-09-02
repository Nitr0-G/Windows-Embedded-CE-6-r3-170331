//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft shared
// source or premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license agreement,
// you are not authorized to use this source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the SOURCE.RTF on your install media or the root of your tools installation.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/***
*fsetpos.c - Contains fsetpos runtime
*
*
*Purpose:
*       Fsetpos sets the file position using an internal value returned by an
*       earlier fgetpos call.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <internal.h>

/***
*int fsetpos(stream,pos) - Set file positioning
*
*Purpose:
*       Fsetpos sets the file position for the file indicated by [stream] to
*       the position indicated by [pos].  The [pos] value is defined to be in
*       an internal format (not to be interpreted by the user) and has been
*       generated by an earlier fgetpos call.
*
*Entry:
*       FILEX *stream = pointer to a file stream value
*       fpos_t *pos = pointer to a file positioning value
*
*Exit:
*       Successful call returns 0.
*       Unsuccessful call returns non-zero (!0).
*
*Exceptions:
*       None.
*******************************************************************************/

int __cdecl fsetpos (
    FILEX *stream,
    const fpos_t *pos
    )
{
#if _INTEGRAL_MAX_BITS >= 64
    return(_fseeki64(stream, *pos, SEEK_SET));
#else
    return(fseek(stream, *pos, SEEK_SET));
#endif
}
