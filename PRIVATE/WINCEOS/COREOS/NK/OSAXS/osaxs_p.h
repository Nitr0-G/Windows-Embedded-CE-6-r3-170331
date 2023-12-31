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
/*++

Module Name:

    Osaxs_p.h

Module Description:

    Private header for osaxs

--*/
#pragma once
#ifndef _OSAXS_P_H
#define _OSAXS_P_H

#ifdef TARGET_BUILD

// Override kernel's KData.
extern struct KDataStruct *g_pKData;
#define KData (*g_pKData)


#include "kernel.h"
#include "kdstub.h"
#include "hdstub.h"
#include "osaxs.h"

#define g_pprcNK  (g_OsaxsData.pprcNK)

#endif

#include "OsAxsFlexi.h"
#include "OsAxsProtocol.h"


/* FLEXI structures */
typedef struct _FLEXI_FIELD_INFO
{
    ULONG32 ul32Id;
    ULONG32 ul32Size;
    WCHAR *szLabel;
    WCHAR *szFormat;
} FLEXI_FIELD_INFO;

// Macros used to generate FLEXI_FIELD_INFO list.  
#define BEGIN_FIELD_INFO_LIST(name)  FLEXI_FIELD_INFO name [] = {
#define FIELD_INFO(id, size, label, format) {(id), (size), (label), (format)},
#define END_FIELD_INFO_LIST() };

// Hard coded size of the FLEXI Info header.
#define CB_FLEXIPTM_HEADER 16

/*
 * Number of Bytes in the Flexi Field Info structure when stored to memory:
 * Offset    Data
 * ------    ----
 * 0         Field Identify
 * 4         Field Stored Size
 * 8         Offset to the Field Label String (which follows all of the field stuctures)
 * 12        Offset to the Field Format String (which follows all of the field structures)
 * 16        End of Field Info Structure.
 */
#define CB_FIELDINFO 16

// Number of fields for Process Data.
#define C_PROCESSFIELDS (sizeof (ProcessDesc) / sizeof (ProcessDesc[0]))

// Number of fields for Thread data.
#define C_THREADFIELDS (sizeof (ThreadDesc) / sizeof (ThreadDesc[0]))

// Size of module ref counts.
#define CB_MODREFCNT (sizeof (WORD) * BUGBUG_MAX_PROCESS)

// Number of Module fields.
#define C_MODULEFIELDS (sizeof (ModuleDesc) / sizeof (ModuleDesc[0]))

#define C_MODPROCFIELDS (sizeof(ModProcDesc)/sizeof(ModProcDesc[0]))

/* Compatibility layer - TODO: Clean this up.  Kind of messy now. */
#if defined (TARGET_BUILD)

#define InitializeCriticalSection (g_OsaxsData.pfnInitializeCriticalSection)
#define DeleteCriticalSection   (g_OsaxsData.pfnDeleteCriticalSection)
#define EnterCriticalSection    (g_OsaxsData.pfnEnterCriticalSection)
#define LeaveCriticalSection    (g_OsaxsData.pfnLeaveCriticalSection)

#define FetchThreadStruct(pthd) ((PTHREAD)(pthd))
#define FetchProcStruct(pproc)  ((PROCESS*)(pproc))
#define FetchKData()            (g_OsaxsData.pKData)
#define FetchDeviceAddr(x)      ((void *)(x))
#define FetchDword(base, off)   ( ((DWORD*)(base))[off] )
#define FetchModuleStruct(ptr)  ( (MODULE *)(ptr) )
#define FetchTOC()              (g_OsaxsData.pRomHdr)
#define FetchLogPtr()           (g_OsaxsData.pLogPtr)
#define FetchMemoryInfo()       ((MEMORYINFO *)(g_OsaxsData.pKData ? g_OsaxsData.pKData->aInfo[KINX_MEMINFO] : NULL))
#define FetchCopyEntry(ptr)     ((COPYentry *)(ptr))
#define IsCurrentThread(pth)    ((pth) == pCurThread)
#define pfnOEMKDIoControl       (g_OsaxsData.pfnOEMKDIoControl)
#define pfnNKKernelLibIoControl (g_OsaxsData.pfnNKKernelLibIoControl)
#define KCall                   (g_OsaxsData.pKCall)
#define ppCaptureDumpFileOnDevice (g_OsaxsData.ppCaptureDumpFileOnDevice)
#define pfnDoThreadGetContext   (g_OsaxsData.pfnDoThreadGetContext)
#define pfnNKGetThreadCallStack (g_OsaxsData.pfnNKGetThreadCallStack)
#define OsAxsIsCurThread(pth)   (pCurThread == (pth))
#define hCoreDll                (*(g_OsaxsData.phCoreDll))
#define Entry2PTBL              (g_OsaxsData.pfnEntry2PTBL)
#if defined(MIPS) 
#define InterlockedDecrement    (g_OsaxsData.pInterlockedDecrement)
#define InterlockedIncrement    (g_OsaxsData.pInterlockedIncrement)
#endif

#endif /* Compatibility layer */


DWORD GetThreadProgramCounter (const THREAD *pth);
DWORD GetThreadRetAddr (const THREAD *pth);

#ifdef TARGET_BUILD

/********* Debug Message Macro */
#define OXZONE_INIT      DEBUGZONE(0)   // 0x0001 
#define OXZONE_PSL       DEBUGZONE(1)   // 0x0002
#define OXZONE_FLEXI     DEBUGZONE(2)   // 0x0004
#define OXZONE_MEM       DEBUGZONE(3)   // 0x0008
#define OXZONE_PACKDATA  DEBUGZONE(4)   // 0x0010
#define OXZONE_CV_INFO   DEBUGZONE(5)   // 0x0020
#define OXZONE_IOCTL     DEBUGZONE(6)   // 0x0040
#define OXZONE_VM        DEBUGZONE(7)   // 0x0080
#define OXZONE_HANDLE    DEBUGZONE(8)   // 0x0100
#define OXZONE_ZONE9     DEBUGZONE(9)
#define OXZONE_ZONEa     DEBUGZONE(10)
#define OXZONE_ZONEb     DEBUGZONE(11)
#define OXZONE_ZONEc     DEBUGZONE(12)
#define OXZONE_ZONEd     DEBUGZONE(13)
#define OXZONE_DWDMPGEN  DEBUGZONE(14)   // 0x4000
#define OXZONE_ALERT     DEBUGZONE(15)   // 0x8000

#define OXZONE_DEFAULT   (0x8000) // OXZONE_ALERT

#define DEBUGGERPRINTF DbgPrintf
#include "debuggermsg.h"

/********** targexcept.c */
#if defined(x86)
extern "C" EXCEPTION_DISPOSITION __cdecl _except_handler3 (EXCEPTION_RECORD *, void *,
    CONTEXT *, DISPATCHER_CONTEXT *);
extern "C" BOOL __abnormal_termination(void);
#else
extern "C" EXCEPTION_DISPOSITION __C_specific_handler (EXCEPTION_RECORD *, void *, 
    CONTEXT *, DISPATCHER_CONTEXT *);
#endif

#if DEBUG
#define KD_ASSERT(exp) if (!(exp)) DEBUGGERMSG (OXZONE_ALERT, (L"**** KD_ASSERT **** " L#exp L"\r\n"))
#else
#define KD_ASSERT(exp)
#endif

/********** initt0.c / initt1.c */
extern HDSTUB_DATA Hdstub;
extern OSAXS_DATA g_OsaxsData;

/********** targdbg.cpp */
extern void (*g_pfnOutputDebugString) (const char *, ...);
extern DBGPARAM dpCurSettings;
extern void DbgPrintf (LPCWSTR, ...);

/********** mem.cpp */
extern DWORD SafeMoveMemory (PROCESS *pProcess, void *pvDestination, void *pvSource, DWORD dwLength);
extern DWORD OsAxsReadMemory (void *pDestination, PROCESS *pVM, void *pSource, DWORD Length);
extern void FlushCache();

/********** except.cpp */

#endif /* TARGET_BUILD */

/********** memcommon.c */
struct ModuleSignature
{
    DWORD dwType;
    GUID guid;
    DWORD dwAge;
    char szPdbName [CCH_PDBNAME];
};

HRESULT GetVersionInfo(void* pModOrProc, BOOL fMod, VS_FIXEDFILEINFO *pvsFixedInfo);
HRESULT GetDebugInfo(void* pModOrProc, BOOL fMod, ModuleSignature *pms);

#if 0
/********** pslret.c */
extern HRESULT TranslateRAForPSL (PSLRETSTATE *, PTHREAD, PSLRETINOUT *);
extern BOOL ThreadExists (DWORD);
#endif


/********** flexptmi.c */
extern HRESULT WriteFieldInfo (FLEXI_FIELD_INFO *pFields, const DWORD cFields, 
    DWORD &riOut, const DWORD cbOut, BYTE *pbOut);
extern HRESULT WritePTMResponseHeader(DWORD, DWORD, DWORD,
    DWORD &, const DWORD, BYTE *);
extern HRESULT GetFPTMI (FLEXI_REQUEST *, DWORD *, BYTE *);

/********** DwDmpGen.cpp */
extern SAVED_THREAD_STATE * g_psvdThread;
extern EXCEPTION_INFO * g_pexceptionInfo;
extern BOOL CaptureDumpFile(PEXCEPTION_RECORD pExceptionRecord, CONTEXT *pContextRecord, BOOLEAN fSecondChance);
extern HRESULT CaptureDumpFileOnTheFly (OSAXS_API_WATSON_DUMP_OTF *pWatsonOtfRequest, BYTE *pbOutBuffer, DWORD *pdwBufferSize, OSAXS_KDBG_RESPONSE_FUNC pKdpSendOsAxsResponse);


/********** Inline utility functions to write data into a buffer with a specified
            size at a specified offset */
// Write an arbitrary buffer to output buffer
static __forceinline HRESULT Write (const void * const pv, const DWORD cb, DWORD &riOut, const DWORD cbOut, BYTE *pbOut)
{
    HRESULT hr = S_OK;
    if (!pbOut || ((riOut < (riOut + cb)) && ((riOut + cb) <= cbOut)) )
    {
        if (pbOut)
            memcpy (&pbOut[riOut], pv, cb);
        riOut += cb;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Write a byte to output buffer
static __forceinline HRESULT Write (const BYTE b, DWORD &riOut, const DWORD cbOut, BYTE *pbOut)
{
    HRESULT hr = S_OK;

    if (!pbOut || (riOut < cbOut))
    {
        if (pbOut)
            pbOut[riOut] = b;
        riOut++; 
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Write a word to the output buffer
static __forceinline HRESULT Write (const WORD w, DWORD &riOut, const DWORD cbOut, BYTE *pbOut)
{
    HRESULT hr = S_OK;
    if (!pbOut || ((riOut < (riOut + sizeof (w))) && ((riOut + sizeof (w)) <= cbOut)) )
    {
        if (pbOut)
            memcpy (&pbOut[riOut], &w, sizeof (w));
        riOut += sizeof (w);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Write a DWORD to the output buffer
static __forceinline HRESULT Write (const DWORD dw, DWORD &riOut, const DWORD cbOut, BYTE *pbOut)
{
    HRESULT hr = S_OK;
    if (!pbOut || ((riOut < (riOut + sizeof (dw))) && ((riOut + sizeof (dw)) <= cbOut)) )
    {
        if (pbOut)
            memcpy (&pbOut[riOut], &dw, sizeof (dw));
        riOut += sizeof (dw);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// Write a 64-bit Ulong to the output buffer
static __forceinline HRESULT Write (const ULONGLONG ul64, DWORD &riOut, const DWORD cbOut, BYTE *pbOut)
{
    HRESULT hr = S_OK;
    if (!pbOut || ((riOut < (riOut + sizeof (ul64))) && ((riOut + sizeof (ul64)) <= cbOut)) )
    {
        if (pbOut)
            memcpy (&pbOut[riOut], &ul64, sizeof (ul64));
        riOut += sizeof (ul64);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}


/********** strings.c */
extern void WideToAnsi (const WCHAR *, char *, DWORD);

/********** except.cpp */
enum _CTXFLAGS
{
    GET_CTX_ALL = 0x7,
    GET_CTX_HDR = 0x1,
    GET_CTX_DESC = 0x2,
    GET_CTX = 0x4,
};

ULONG GetCallStack(PTHREAD pth, ULONG dwMaxFrames, LPVOID lpFrames, DWORD dwFlags, DWORD dwSkip);
HRESULT OSAXSGetCallstack(HANDLE hThread, DWORD FrameStart, DWORD FramesToRead, DWORD *pFramesReturned, void *FrameBuffer, 
    DWORD *pFrameBufferLength);

BOOL KdpFlushExtendedContext(CONTEXT* pCtx);

HRESULT SaveExceptionState (EXCEPTION_INFO     *  pexceptionInfo,
                            SAVED_THREAD_STATE *  psvdThread,
                            EXCEPTION_INFO     ** ppexceptionInfoSave,
                            SAVED_THREAD_STATE ** ppsvdThreadSave);

extern HRESULT GetExceptionContext (DWORD dwFlags, DWORD *pcbBuf, BYTE *pbBuf);
extern HRESULT WriteExceptionContext (DWORD &riOut, const DWORD cbOut, BYTE *pbOut, BOOL fIntegerOnly);

//
// mem.cpp
//
void * MapToDebuggeeCtxKernEquivIfAcc (PROCESS *pFocusProc, void *pvAddr, BOOL fProbeOnly);
PPROCESS GetProperVM(PPROCESS, void *);
void *OSAXSMapPtrWithSize(PROCESS *, void *, size_t, BOOL);

template<typename T> T *OSAXSMapTypePtr (PROCESS *Process, T *Address, BOOL fProbeOnly)
{
    return reinterpret_cast<T*> (OSAXSMapPtrWithSize (Process, Address, sizeof (T), fProbeOnly));
}


/********** packdata.cpp */
HRESULT GetModuleO32Data (DWORD dwAddrModule, DWORD *pdwNumO32Data, void *pvResult, DWORD *pcbResult);

/********** vm.cpp */
HRESULT OSAXSTranslateAddress(HANDLE hProc, void* pvAddr, BOOL fReturnKVA, void** ppvPhysOrKVA);

#if defined(x86)
HRESULT GetExceptionRegistration(DWORD* pdwBuff);
#endif

//
// handle.cpp
//
PPROCESS OSAXSHandleToProcess(HANDLE h);
PTHREAD OSAXSHandleToThread(HANDLE h);
PCCINFO OSAXSHandleToPCInfo(HANDLE h, DWORD dwProcessHandle);
PHDATA OSAXSHandleToPHData(HANDLE, DWORD);


// Test whether a process is valid for debugging
bool __inline OSAXSIsProcessValid(PPROCESS pProc)
{
    return pProc->bState <= PROCESS_STATE_START_EXITING;
}

// Test whether a given module is valid for debugging.
bool __inline OSAXSIsModuleValid(PMODULE pmod)
{
    return (pmod == pmod->lpSelf) && (pmod->wRefCnt > 0);
}

#endif
