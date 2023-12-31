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
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
@doc LIBRARY


Module Name:
    NetInit.h

Abstract:
    Definition of a helper functions to initialize the system

Revision History:
     2-Feb-2000		Created

-------------------------------------------------------------------*/
#ifndef _NET_INIT_H_
#define _NET_INIT_H_

#include <windows.h>
#include <tchar.h>
#include "netmain.h"  // Include NetMain.h to get the flag definitions.


#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI NetInitStartLogWrap(HANDLE h);
BOOL WINAPI NetInit(DWORD param);

#ifdef __cplusplus
}
#endif

#endif
