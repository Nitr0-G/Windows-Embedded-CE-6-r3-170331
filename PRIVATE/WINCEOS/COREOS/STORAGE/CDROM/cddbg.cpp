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

#include <windows.h>
#include <cddbg.h>

#ifdef DEBUG

//
// Debug zones
//

DBGPARAM dpCurSettings = {
    TEXT("UDFS"), 
    {
        TEXT("Errors"),
        TEXT("Warnings"),
        TEXT("Functions"),
        TEXT("Initialization"),
        TEXT("Disk I/O"),
        TEXT("Media"),
        TEXT("Alloc"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Undefined"),
        TEXT("Initial DebugBreak()")
    },
    ZONEMASK_ERROR | ZONEMASK_INIT
};

#endif

