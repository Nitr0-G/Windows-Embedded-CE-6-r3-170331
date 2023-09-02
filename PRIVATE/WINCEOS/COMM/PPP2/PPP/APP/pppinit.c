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
//-***********************************************************************
//
//  This file contains code that performs initialization of the PPP module
//  during bootup.
//
//-***********************************************************************

#include "windows.h"
#include "cxport.h"
#include "types.h"

//  PPP Include Files

#include "protocol.h"
#include "ppp.h"
#include "mac.h"
#include "crypt.h"
#include "pppserver.h"
#include "auth.h"
#include "debug.h"

#include <ndistapi.h>

//
// PPP Context List
//

extern  CRITICAL_SECTION v_ListCS;
extern  HANDLE           v_hInstDLL;

#ifdef DEBUG
DBGPARAM dpCurSettings =
{
    TEXT("PPP"),
    {
        TEXT("Init"),TEXT("Mac"),TEXT("Lcp"),TEXT("Auth"),
        TEXT("Ncp"),TEXT("IpCp"),TEXT("Ras"),TEXT("PPP"),
        TEXT("Timing"),TEXT("Trace"),TEXT("Unused"),TEXT("Event"),
        TEXT("Alloc"),TEXT("Function"),TEXT("Warning"),TEXT("Error")
    },
    0x0C000
};
#endif

BOOL            g_bPPPInitialized = FALSE;

HMODULE         g_hCoredllModule;
pfnPostMessageW g_pfnPostMessageW;

BOOL
GetGWEProcAddresses(void)
{
	BOOL bOk = FALSE;

	g_hCoredllModule = LoadLibrary(TEXT("coredll.dll"));
	if (g_hCoredllModule)
	{
		g_pfnPostMessageW = (pfnPostMessageW)GetProcAddress(g_hCoredllModule, TEXT("PostMessageW"));
		if (g_pfnPostMessageW == NULL)
		{
			RETAILMSG(1, (TEXT("PPP: ERROR: Can't find PostMessageW in coredll.dll!\n")));
			FreeLibrary(g_hCoredllModule);
		}
		else
		{
			bOk = TRUE;
		}
	}
	else
	{
		RETAILMSG(1, (TEXT("PPP: ERROR: LoadLibrary of coredll.dll failed!\n")));
	}
	return bOk;
}

BOOL __stdcall
DllEntry( HANDLE  hinstDLL, DWORD   Op, LPVOID  lpvReserved )
{
    switch (Op)
    {
    case DLL_PROCESS_ATTACH :
		DEBUGREGISTER(hinstDLL);
        v_hInstDLL = hinstDLL;
        DisableThreadLibraryCalls ((HMODULE)hinstDLL);
		InitializeCriticalSection( &v_ListCS );
		break;

    case DLL_PROCESS_DETACH:
		// PPP never unloads from device.exe in the current
		// architecture. Properly implementing support for unloading
		// would take some effort...
		ASSERT(FALSE);
        break;

    default :
        break;
    }
    return TRUE;
}

BOOL
Deinit(
    DWORD Context
    )
{
	RETAILMSG(1, (TEXT("PPP:Deinit\r\n")));
    ASSERT(0);
    return TRUE;
}

BOOL
Init(
    IN TCHAR *szRegPath
    )
//
//  Called at boot time by the device.exe driver loader if
//  the Builtin/PPP registry key values are configured like this:
//    [HKEY_LOCAL_MACHINE\Drivers\BuiltIn\PPP]
//      "Dll"="PPP.Dll"
//      ; Must load after NDIS
//      "Order"=dword:3
//
//  This function performs boot time initialization of PPP.
//  This is done here rather than DllEntry because of potential
//  deadlocks that can occur because DllEntry runs with the loader
//  critical section held.
//
//  Return TRUE if successful, FALSE if an error occurs.
//
{
	if (g_bPPPInitialized == FALSE)
	{
		// OK if this fails, code will handle gracefully
		GetGWEProcAddresses();

		// Initialize the NDIS interface
		pppMac_Initialize();

		// Initialize hooks to EAP
		(void)rasEapGetDllFunctionHooks();

		// Initialize PPP Server
		PPPServerInitialize();

		g_bPPPInitialized = TRUE;
	}

	return TRUE;
}

