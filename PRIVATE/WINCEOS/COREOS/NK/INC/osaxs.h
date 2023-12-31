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
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*++

Module Name:

    Osaxs.h

Module Description:

    OsaxsT0, OsaxsT1 header / kernel interface

--*/

#pragma once
#ifndef _OSAXS_H
#define _OSAXS_H

#include "osaxs_common.h"

/* Official client names for osaccess stubs */
#define OSAXST0_NAME "OsAxsT0"
#define OSAXST1_NAME "OsAxsT1"

/* OsAccess IOCTL Numbers */
enum OSAXST0_IOCTL
{
    OSAXST0_IOCTL_GET_FLEXIPTMINFO       = 1,
    OSAXST0_IOCTL_SAVE_EXCEPTION_STATE   = 2,
    OSAXST0_IOCTL_GET_WATSON_DUMP = 3,
    OSAXST0_IOCTL_MAP_TO_KVA_IF_ACC = 4,
};

enum OSAXST1_IOCTL
{
    OSAXST1_IOCTL_SAVE_EXCEPTION_STATE  = 2,
    OSAXST1_IOCTL_GET_EXCEPTION_REGISTRATION = 8,
    OSAXST1_IOCTL_TRANSLATE_ADDRESS     = 9,
    OSAXST1_IOCTL_TRANSLATE_HPCI = 10,
    OSAXST1_IOCTL_PROCESS_FROM_HANDLE = 11,
    OSAXST1_IOCTL_CALLSTACK           = 12,
    OSAXST1_IOCTL_GET_HDATA           = 13,
};


// Arguments for OSAXST0_IOCTL_GET_FLEXIPTMINFO
typedef struct _OSAXSFN_GETFLEXIPTMINFO
{
    FLEXI_REQUEST *pRequest;
    DWORD dwBufferSize;
    BYTE *pbBuffer;
} OSAXSFN_GETFLEXIPTMINFO, *POSAXSFN_GETFLEXIPTMINFO;


// Arguments for OSAXST0_IOCTL_GET_WATSON_DUMP
typedef struct _OSAXSFN_GETWATSONDUMP
{
    struct OSAXS_API_WATSON_DUMP_OTF *pWatsonOtfRequest;
    BYTE *pbOutBuffer;
    DWORD dwBufferSize;
    OSAXS_KDBG_RESPONSE_FUNC pfnResponse;
} OSAXSFN_GETWATSONDUMP, *POSAXSFN_GETWATSONDUMP;


// Arguments for OSAXST0_IOCTL_SAVE_EXCEPTION_STATE
// Arguments for OSAXST1_IOCTL_SAVE_EXCEPTION_STATE
typedef struct _OSAXSFN_SAVEEXCEPTIONSTATE
{
    EXCEPTION_INFO *pNewExceptionInfo;
    SAVED_THREAD_STATE *pNewSavedThreadState;
    EXCEPTION_INFO **ppSavedExceptionInfo;
    SAVED_THREAD_STATE **ppSavedThreadState;
} OSAXSFN_SAVEEXCEPTIONSTATE, *POSAXSFN_SAVEEXCEPTIONSTATE;


// Arguments for OSAXST0_IOCTL_MAP_TO_KVA_IF_ACC
typedef struct _OSAXSFN_MAPTOKVA
{
    PPROCESS pVM;
    void *pvAddr;
    BOOL fProbeOnly;

    // return value.
    void *pvKVA;
} OSAXSFN_MAPTOKVA, *POSAXSFN_MAPTOKVA;


// Arguments for OSAXST1_IOCTL_GET_MODULE_O32_DATA
typedef struct _OSAXSFN_GETO32DATA
{
    DWORD dwModuleHandle;
    DWORD dwNbSections;
    void *pvBuffer;
    DWORD dwBufferSize;
} OSAXSFN_GETO32DATA, *POSAXSFN_GETO32DATA;


// Arguments for OSAXST1_IOCTL_TRANSLATE_ADDRESS
typedef struct _OSAXSFN_TRANSLATEADDRESS
{
    HANDLE hProcess;
    void *pvVirtual;
    BOOL fReturnKVA;
    void *pvTranslated;
} OSAXSFN_TRANSLATEADDRESS, *POSAXSFN_TRANSLATEADDRESS;


// Arguments for OSAXST1_IOCTL_TRANSLATE_HPCI
typedef struct _OSAXSFN_TRANSLATEHPCI
{
    HANDLE handle;           // Handle to look up
    DWORD dwProcessHandle;   // Process hosting handle table to use.
    PCCINFO pcinfo;
} OSAXSFN_TRANSLATEHPCI, *POSAXSFN_TRANSLATEHPCI;


typedef struct _OSAXSFN_GETHDATA
{
    HANDLE handle;
    DWORD dwProcessHandle;
    PHDATA phdata;
} OSAXSFN_GETHDATA, *POSAXSFN_GETHDATA;


// Arguments for OSAXST1_IOCTL_PROCESS_FROM_HANDLE
typedef struct _OSAXSFN_PROCESSFROMHANDLE
{
    HANDLE hProc;
    PPROCESS pProc;
} OSAXSFN_PROCESSFROMHANDLE, *POSAXSFN_PROCESSFROMHANDLE;


typedef struct _OSAXSFN_CALLSTACK
{
    HANDLE hThread;
    DWORD FrameStart;
    DWORD FramesToRead;
    DWORD FramesReturned;
    void *FrameBuffer;
    DWORD FrameBufferLength;
} OSAXSFN_CALLSTACK;


#define CAPTUREDUMPFILEFNLENGTH 0x200
// RaiseException(STATUS_CRASH_DUMP, ...) is used to programmatically capture dump files.
// This macro makes sure this exception was raised by CaptureDumpFileOnDevice.
// Make sure the extent of the ExceptionAddress includes the whole functin for worst
// case condition (MIPSIV Debug), i.e. (DWORD)pCaptureDumpFileOnDevice + 0x200
static BOOL __forceinline CAPTUREDUMPFILEONDEVICE_CALLED (EXCEPTION_RECORD *pExceptionRecord, PVOID pCaptureDumpFileOnDevice,
                                                          PVOID pKCaptureDumpFileOnDevice)
{
    if (pExceptionRecord->ExceptionCode != STATUS_CRASH_DUMP)
        return FALSE;

    if (pExceptionRecord->NumberParameters != 5)
        return FALSE;

    if ((DWORD)pExceptionRecord->ExceptionInformation[4] != (DWORD)pCaptureDumpFileOnDevice &&
        (DWORD)pExceptionRecord->ExceptionInformation[4] != (DWORD)pKCaptureDumpFileOnDevice)
        return FALSE;

    if (((DWORD)pCaptureDumpFileOnDevice <= (DWORD)pExceptionRecord->ExceptionAddress) &&
        ((DWORD)pExceptionRecord->ExceptionAddress <= ((DWORD)pCaptureDumpFileOnDevice + CAPTUREDUMPFILEFNLENGTH)))
        return TRUE;

    if (((DWORD)pKCaptureDumpFileOnDevice <= (DWORD)pExceptionRecord->ExceptionAddress) &&
        ((DWORD)pExceptionRecord->ExceptionAddress <= ((DWORD)pKCaptureDumpFileOnDevice + CAPTUREDUMPFILEFNLENGTH)))
        return TRUE;

    return FALSE;
}


typedef struct _OSAXS_DATA
{
    /* in */
    DWORD cbSize;
    BOOL   (*pfnINTERRUPTS_ENABLE)(BOOL);
    VOID (*pfnInitializeCriticalSection)(LPCRITICAL_SECTION);
    VOID (*pfnDeleteCriticalSection)(LPCRITICAL_SECTION);
    VOID (*pfnEnterCriticalSection)(LPCRITICAL_SECTION);
    VOID (*pfnLeaveCriticalSection)(LPCRITICAL_SECTION);
    struct KDataStruct *pKData;
    HANDLE*  phCoreDll;
    PPVOID ppCaptureDumpFileOnDevice;
    PPVOID ppKCaptureDumpFileOnDevice;
    const CINFO **pSystemAPISets;
    PROCESS *pprcNK;
    const ROMHDR *pRomHdr;
    fslog_t *pLogPtr;
    PFN_OEMKDIoControl pfnOEMKDIoControl;
    BOOL (*pfnNKKernelLibIoControl) (HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD);
    void (*pfnFlushCacheRange) (LPVOID, DWORD, DWORD);
    BOOL (*pfnIsRom) (LPVOID, DWORD);
    void *(*pfnDbgVerify) (PROCESS *, void *, BOOL, BOOL *);
    void *(*pfnGetKAddrOfProcess)(PROCESS *, void const *);
    DWORD (*pfnGetPFNOfProcess)(PROCESS *, void const *);
    BOOL (*pfnDoThreadGetContext) (PTHREAD, LPCONTEXT);
    ULONG (*pfnNKGetThreadCallStack) (PTHREAD, ULONG, LPVOID, DWORD, DWORD, PCONTEXT);
    BOOL (WINAPI* pEventModify)(HANDLE, DWORD);
    DWORD  dwCEInstructionSet;
#if defined(MIPS)
    LONG  (* pInterlockedDecrement)(LONG volatile *Target);
    LONG  (* pInterlockedIncrement)(LONG volatile *Target);
#endif
#ifdef x86
    EXCEPTION_DISPOSITION (__cdecl *p_except_handler3) (EXCEPTION_RECORD *,
        void *, CONTEXT *, DISPATCHER_CONTEXT *);
    BOOL (*p__abnormal_termination) (void);
#else
    EXCEPTION_DISPOSITION (*p__C_specific_handler) (EXCEPTION_RECORD *,
        void *, CONTEXT *, DISPATCHER_CONTEXT *);
#endif

    struct _REGISTER_DESC * (*pfnOEMGetRegDesc) (DWORD *);
    HRESULT (*pfnOEMReadRegs) (BYTE *, DWORD *, const DWORD);
    HRESULT (*pfnOEMWriteRegs) (BYTE *, const DWORD);

    int (* pKCall)(PKFN, ...);

    void (*DSPFlushContext) (void);
    void (*FPUFlushContext) (void);

    PTHREAD *ppCurFPUOwner;  
    DWORD *  pdwProcessorFeatures;
    
    DWORD (*pKITLIoCtl) (DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD);
    int (*pNKvsprintfW) (LPWSTR, LPCWSTR, va_list, int);
    PHDATA (*pfnHandleToHDATA)(HANDLE, PHNDLTABLE);
    PROCESS* (*pfnGetProcPtr)(PHDATA);
    PVOID (*pfnPfn2Virt)(DWORD);
    PROCESS *(*pfnSwitchActiveProcess) (PROCESS *);
    PROCESS *(*pfnSwitchVM) (PROCESS*);
    PPAGETABLE (*pfnEntry2PTBL)(DWORD);
} OSAXS_DATA;

#endif
