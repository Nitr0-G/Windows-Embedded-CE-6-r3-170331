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
#ifndef __NDP_LIB_H
#define __NDP_LIB_H

//------------------------------------------------------------------------------

#define ETH_MAX_FRAME_SIZE    1514
#define ETH_ADDR_SIZE         6

//------------------------------------------------------------------------------

#ifdef UNDER_CE
DWORD LoadAdapter(LPCTSTR szAdapter,LPCTSTR szDriver);
DWORD UnloadAdapter(LPCTSTR szAdapter);
DWORD QueryAdapters(LPTSTR mszAdapters, DWORD dwSize);
DWORD QueryProtocols(LPTSTR mszProtocols, DWORD dwSize);
DWORD QueryBindings(LPCTSTR szAdapter, LPTSTR mszProtocols, DWORD dwSize);
DWORD BindProtocol(LPCTSTR szAdapterName, LPCTSTR szProtocol);
DWORD UnbindProtocol(LPCTSTR szAdapter, LPCTSTR szProtocol);
DWORD ReBindProtocol(LPCTSTR szAdapterName, LPCTSTR szProtocol);

#if 0
	HANDLE EnableWakeUp(LPCTSTR szEventName, DWORD dwSeconds, HANDLE * pWaitEvent);
	void DisableWakeUp(HANDLE hNotify, HANDLE hWaitEvent);
#endif 

HANDLE SetDeviceWakeUp(DWORD dwWakeSrc, DWORD dwSeconds);
BOOL ClearDeviceWakeUp(HANDLE hAdapter, DWORD dwWakeSrc);

#else

STDAPI StringCchCopy(LPTSTR pszDest,
                     size_t cchDest,
                     LPCTSTR pszSrc);

STDAPI StringCchCat(LPTSTR pszDest,
                    size_t cchDest,
                    LPCTSTR pszSrc);

STDAPI StringCchVPrintf(LPTSTR pszDest,
                        size_t cchDest,
                        LPCTSTR pszFormat,
                        va_list argList);

typedef struct {
   WCHAR szwDisplayName[256];
   WCHAR szwBindName[256];
   WCHAR szwHelpText[256];
   WCHAR szwId[256];
} tsNtAda, * PtsNtAda;

#define MAX_ENUM_NT_ADAPTERS (25)

typedef struct {
   DWORD dwNosOfAda;
   tsNtAda sNtAdaArray[MAX_ENUM_NT_ADAPTERS];
} tsNtEnumAda, * PtsNtEnumAda;

HRESULT NtEnumAdapters(PtsNtEnumAda psNtEnumAd, DWORD dwMaxNosOfAda);

#endif
//------------------------------------------------------------------------------

HANDLE OpenProtocol();
VOID CloseProtocol(HANDLE hAdapter);

BOOL OpenAdapter(HANDLE hAdapter, LPCTSTR szAdapter);
BOOL CloseAdapter(HANDLE hAdapter);

BOOL SendPacket(
   HANDLE hAdapter, UCHAR dstAddr[], DWORD packetType, UCHAR *pData, 
   DWORD dataSize
);
BOOL Listen(HANDLE hAdapter, DWORD poolSize, BOOL bDirect, BOOL bBroadcast);
BOOL ReceivePacket(
   HANDLE hAdapter, DWORD timeout, UCHAR srcAddr[], DWORD *pPacketType, 
   UCHAR *pData, DWORD *pDataSize
);
BOOL StressSend(
   HANDLE hAdapter, BOOL bSend, UCHAR dstAddr[], DWORD packetType, 
   DWORD poolSize, DWORD packetSize, DWORD packetsSend,
   DWORD dwFlagStressControlled,DWORD dwDelayInABurst, DWORD dwPacketsInABurst,
   DWORD *pTime, DWORD *pIdleTime, DWORD *pPacketsSent, DWORD *pBytesSent
);
BOOL StressReceive(
   HANDLE hAdapter, DWORD poolSize, DWORD packetType, UCHAR srcAddr[], 
   DWORD *pPacketType, UCHAR *pData, DWORD *pDataSize, DWORD* pTime, 
   DWORD* pIdleTime, DWORD *pPacketReceived, DWORD *pBytesReceived
);

#include <ntddndis.h>

BOOL QueryMPOID(HANDLE hAdapter,NDIS_OID oid, PBYTE puBuff, PDWORD pdwBuffSize, NDIS_STATUS * status);
BOOL SetMPOID(HANDLE hAdapter,NDIS_OID oid, PBYTE puBuff, PDWORD pdwBuffSize, NDIS_STATUS * status);

//------------------------------------------------------------------------------

#endif
