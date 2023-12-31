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

    kdp.h

Abstract:

    Private include file for the Kernel Debugger subcomponent

Environment:

    WinCE


--*/

// Override kernel's KData.
extern struct KDataStruct *g_pKData;

#define KData (*g_pKData)

extern void* (* g_pPfn2Virt) (unsigned long) ;

#define Pfn2Virt (*g_pPfn2Virt)

#include <winerror.h>

#include "kernel.h"
#include "cpuid.h"
#include "kdstub.h"
#include "hdstub.h"
#include "string.h"
#include "kdpcpu.h"
#include "dbg.h"
#include "KitlProt.h"
#include "osaxs.h"
#include "OsAxsFlexi.h"
#include "kdApi2Structs.h"
#include "osaxsprotocol.h"


// status Constants for Packet waiting
// TODO: remove this since we use KITL
#define KDP_PACKET_RECEIVED    0x0000
#define KDP_PACKET_RESEND      0x0001
#define KDP_PACKET_UNEXPECTED  0x0002
#define KDP_PACKET_NONE        0xFFFF


#ifdef SHx
// for SR_DSP_ENABLED and SR_FPU_DISABLED
#include "shx.h"
#endif

// Useful thing to have.
#define lengthof(x)                     (sizeof(x) / sizeof(*x))


extern DBGPARAM dpCurSettings;

#define KDZONE_INIT             DEBUGZONE(0)    /* 0x0001 */
#define KDZONE_TRAP             DEBUGZONE(1)    /* 0x0002 */
#define KDZONE_API              DEBUGZONE(2)    /* 0x0004 */
#define KDZONE_DBG              DEBUGZONE(3)    /* 0x0008 */
#define KDZONE_SWBP             DEBUGZONE(4)    /* 0x0010 */
#define KDZONE_BREAK            DEBUGZONE(5)    /* 0x0020 */
#define KDZONE_CTRL             DEBUGZONE(6)    /* 0x0040 */
#define KDZONE_MOVE             DEBUGZONE(7)    /* 0x0080 */
#define KDZONE_KERNCTXADDR      DEBUGZONE(8)    /* 0x0100 */
#define KDZONE_PACKET           DEBUGZONE(9)    /* 0x0200 */
#define KDZONE_STACKW           DEBUGZONE(10)   /* 0x0400 */
#define KDZONE_INTELWMMX        DEBUGZONE(11)   /* 0x0800 */
#define KDZONE_VIRTMEM          DEBUGZONE(12)   /* 0x1000 */
#define KDZONE_HANDLEEX         DEBUGZONE(13)   /* 0x2000 */
#define KDZONE_DBGMSG2KDAPI     DEBUGZONE(14)   /* 0x4000 */ // This flag indicate that KD Debug Messages must be re-routed to KDAPI to be displayed in the target debug output instead of the Serial output
#define KDZONE_ALERT            DEBUGZONE(15)   /* 0x8000 */

#define KDZONE_FLEXPTI          KDZONE_DBG


#define KDZONE_DEFAULT          (0x8000) // KDZONE_ALERT

#define _O_RDONLY   0x0000  /* open for reading only */
#define _O_WRONLY   0x0001  /* open for writing only */
#define _O_RDWR     0x0002  /* open for reading and writing */
#define _O_APPEND   0x0008  /* writes done at eof */

#define _O_CREAT    0x0100  /* create and open file */
#define _O_TRUNC    0x0200  /* open and truncate */
#define _O_EXCL     0x0400  /* open only if file doesn't already exist */

extern VOID NKOtherPrintfW(LPWSTR lpszFmt, ...);
#define DEBUGGERPRINTF NKOtherPrintfW
#include "debuggermsg.h"


// version of Kd.dll
#define CUR_KD_VER (600)


// ------------------------------- OS Access specifics --------------------------


#include <pshpack4.h>

//
// structures for HANDLE_MODULE_REFCOUNT_REQUEST protocol
//
// also in:
// /tools/ide/debugger/dmcpp/kdapi.cpp and
// /tools/ide/debugger/odcpu/odlib/datamgr.cpp
typedef struct tagGetModuleRefCountProc
{
    WORD wRefCount;

    // This is not a string. It is an array of characters. It probably won't
    // be null-terminated.
    WCHAR szProcName[15];
} DBGKD_GET_MODULE_REFCNT_PROC;

typedef struct tagGetModuleRefCount
{
    UINT32 nProcs;

    // Array with length = nProcs
    DBGKD_GET_MODULE_REFCNT_PROC pGMRCP[];
} DBGKD_GET_MODULE_REFCNT;

// structures and defines for HANDLE_DESC_HANDLE_DATA


#include <poppack.h>


// ------------------------------- END of OS Access specifics --------------------------



extern BOOL g_fForceReload;
extern BOOL g_fKdbgRegistered;
extern BOOL g_fHandlingLoadNotification;
extern BOOL g_fWorkaroundPBForVMBreakpoints;


// KdStub State Notification Flags
extern BOOL g_fDbgKdStateMemoryChanged; // Set this signal to TRUE to notify the host that target memory has changed and host-side must refresh


#define PAGE_ALIGN(Va)  ((ULONG)(Va) & ~(VM_PAGE_SIZE - 1))
#define BYTE_OFFSET(Va) ((ULONG)(Va) & (VM_PAGE_SIZE - 1))


//
// Ke stub routines and definitions
//


#if defined(x86)

//
// There is no need to sweep the i386 cache because it is unified (no
// distinction is made between instruction and data entries).
//

#define KeSweepCurrentIcache()

#elif defined(SHx)

//
// There is no need to sweep the SH3 cache because it is unified (no
// distinction is made between instruction and data entries).
//

extern void FlushCache (void);

#define KeSweepCurrentIcache() FlushCache()

#else

extern void FlushICache (void);

#define KeSweepCurrentIcache() FlushICache()

#endif


#define VER_PRODUCTBUILD 0


#define STATUS_SYSTEM_BREAK             ((NTSTATUS)0x80000114L)


//
// TRAPA / BREAK immediate field value for breakpoints
//

#define DEBUGBREAK_STOP_BREAKPOINT         1

#define DEBUG_PROCESS_SWITCH_BREAKPOINT       2
#define DEBUG_THREAD_SWITCH_BREAKPOINT        3
#define DEBUG_BREAK_IN                        4
#define DEBUG_REGISTER_BREAKPOINT             5


#if defined (ARM)

BOOL HasWMMX();

void GetWMMXRegisters(PCONCAN_REGS);
void SetWMMXRegisters(PCONCAN_REGS);

#endif

typedef ULONG KSPIN_LOCK;

//
// Miscellaneous
//

#if DBG

#define KD_ASSERT(exp)
    DEBUGGERMSG (KDZONE_ALERT, (L"**** KD_ASSERT ****" L##exp "\r\n"))

#else

#define KD_ASSERT(exp)

#endif


#define BREAKPOINT_TABLE_SIZE (256) // TODO: move this in HDSTUB.h


//
// Define breakpoint table entry structure.
//

// FLAGS
#define KD_BREAKPOINT_SUSPENDED             (0x01) // original instruction of SW BP is temporary restored (typically to prevent KD hitting that BP)
#define KD_BREAKPOINT_16BIT                 (0x02)
#define KD_BREAKPOINT_INROM                 (0x04) // Indicate that the BP is in ROM (this is useful only to detect duplicates using both current Address and KAddress)
#define KD_BREAKPOINT_WRITTEN               (0x08) // Indicate that the BP was written. (useful for delayed assembly breakpoints.)


typedef struct _BREAKPOINT_ENTRY {
    PPROCESS pVM; // VM Associated with breakpoint.
    PVOID Address; // Address that the user specified for bp
    PVOID KAddr; // Address that the breakpoint was written to.  We need to keep this around
                 // for cases in which the virtual mapping for a module's memory is lost before
                 // the unload notification.
    WORD wRefCount;
    BYTE Flags;
    KDP_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;


// Breakpoint special Handles for error passing

#define KD_BPHND_ROMBP_SUCCESS (1)
#define KD_BPHND_INVALID_GEN_ERR (0)
#define KD_BPHND_ROMBP_ERROR_INSUFFICIENT_PAGES (-1)
#define KD_BPHND_ERROR_COPY_FAILED (-2)
#define KD_BPHND_ROMBP_ERROR_REMAP_FAILED (-3)


// ROM Breakpoints structures

#define NB_ROM2RAM_PAGES (16)

typedef struct _ROM2RAM_PAGE_ENTRY
{
    void* pvROMAddr;
    BYTE* pbRAMAddr;
    DWORD ROMAddrOrgPTEntry;
    int nBPCount;
} ROM2RAM_PAGE_ENTRY;

extern ROM2RAM_PAGE_ENTRY g_aRom2RamPageTable [NB_ROM2RAM_PAGES];
extern BYTE g_abRom2RamDataPool [((NB_ROM2RAM_PAGES + 1) * VM_PAGE_SIZE) - 1];

// Exception Filtering
#define MAX_FILTER_EXCEPTION_PROC 10
#define MAX_FILTER_EXCEPTION_CODE 10

extern BOOL g_fFilterExceptions;
extern CHAR g_szFilterExceptionProc[MAX_FILTER_EXCEPTION_PROC][CCH_PROCNAME];
extern DWORD g_dwFilterExceptionCode[MAX_FILTER_EXCEPTION_CODE];

#if defined(SHx)
void LoadDebugSymbols(void);

//
// User Break Controller memory-mapped addresses
//
#if SH4
#define UBCBarA  0xFF200000        // 32 bit Break Address A
#define UBCBamrA 0xFF200004        // 8 bit  Break Address Mask A
#define UBCBbrA  0xFF200008        // 16 bit Break Bus Cycle A
#define UBCBasrA 0xFF000014        // 8 bit  Break ASID A
#define UBCBarB  0xFF20000C       // 32 bit Break Address B
#define UBCBamrB 0xFF200010       // 8 bit  Break Address Mask B
#define UBCBbrB  0xFF200014       // 16 bit Break Bus Cycle A
#define UBCBasrB 0xFF000018       // 8 bit  Break ASID B
#define UBCBdrB  0xFF200018       // 32 bit Break Data B
#define UBCBdmrB 0xFF20001C       // 32 bit Break Data Mask B
#define UBCBrcr  0xFF200020       // 16 bit Break Control Register
#else
#define UBCBarA    0xffffffb0
#define UBCBamrA   0xffffffb4
#define UBCBbrA    0xffffffb8
#define UBCBasrA   0xffffffe4
#define UBCBarB    0xffffffa0
#define UBCBamrB   0xffffffa4
#define UBCBbrB    0xffffffa8
#define UBCBasrB   0xffffffe8
#define UBCBdrB    0xffffff90
#define UBCBdmrB   0xffffff94
#define UBCBrcr    0xffffff98
#endif
#endif

#define READ_REGISTER_UCHAR(addr) (*(volatile unsigned char *)(addr))
#define READ_REGISTER_USHORT(addr) (*(volatile unsigned short *)(addr))
#define READ_REGISTER_ULONG(addr) (*(volatile unsigned long *)(addr))

#define WRITE_REGISTER_UCHAR(addr,val) (*(volatile unsigned char *)(addr) = (val))
#define WRITE_REGISTER_USHORT(addr,val) (*(volatile unsigned short *)(addr) = (val))
#define WRITE_REGISTER_ULONG(addr,val) (*(volatile unsigned long *)(addr) = (val))

//
// Define Kd function prototypes.
//
#if defined(MIPS_HAS_FPU) || defined(SH4) || defined(x86) || defined (ARM)
VOID FPUFlushContext (VOID);
#endif

#if defined(SHx) && !defined(SH3e) && !defined(SH4)
VOID DSPFlushContext (VOID);
#endif

void KdpResetBps (void);

VOID
KdpReboot (
    IN BOOL fReboot
    );

ULONG
KdpAddBreakpoint (
    PPROCESS pVM,
    IN PVOID Address
    );

BOOLEAN
KdpDeleteBreakpoint (
    IN ULONG Handle
    );

VOID
KdpDeleteAllBreakpoints (
    VOID
    );

DWORD
KdpFindBreakpoint(void* pvOffset);

ULONG
KdpMoveMemory (
    IN PVOID Destination,
    IN PVOID Source,
    IN ULONG Length
    );

ULONG
KdpMoveMemory2 (
    PPROCESS pVMDest,
    IN PVOID Destination,
    PPROCESS pVMSource,
    IN PVOID Source,
    IN ULONG Length
    );

USHORT
KdpReceiveCmdPacket (
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    OUT GUID *pguidClient
    );

VOID
KdpSendPacket (
    IN WORD dwPacketType,
    IN GUID guidClient,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

VOID
KdpSendKdApiCmdPacket (
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

VOID 
KdpSendDbgMessage(
    IN CHAR*  pszMessage,
    IN DWORD  dwMessageSize
    );

ULONG
KdpTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN CONTEXT * ContextRecord,
    IN BOOLEAN SecondChance
    );

BOOL KdpModLoad (DWORD);
BOOL KdpModUnload (DWORD);

BOOL
KdpSanitize(
    BYTE* pbClean,
    VOID* pvMem,
    ULONG nSize,
    BOOL fAlwaysCopy
    );

BOOLEAN
KdpReportExceptionNotif (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN SecondChance
    );


BOOLEAN
KdpSendNotifAndDoCmdLoop(
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

VOID
KdpReadVirtualMemory(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpWriteVirtualMemory(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpReadPhysicalMemory(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpWritePhysicalMemory(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpAdvancedCmd(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpSetContext(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpWriteBreakpoint(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpRestoreBreakpoint(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpReadIoSpace(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData,
    IN BOOL fSendPacket
    );

VOID
KdpWriteIoSpace(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData,
    IN BOOL fSendPacket
    );

NTSTATUS
KdpWriteBreakPointEx(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpRestoreBreakPointEx(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    );

VOID
KdpManipulateBreakPoint(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
);

VOID KdpSuspendAllBreakpoints(
    VOID
);

VOID
KdpReinstateSuspendedBreakpoints(
    VOID
);

BOOLEAN
KdpSuspendBreakpointIfHitByKd(
    IN VOID* Address
);

BOOL
KdpHandlePageIn(
    IN ULONG ulAddress,
    IN ULONG ulNumPages,
    IN BOOL bWrite
);


VOID
KdpHandlePageInBreakpoints(
    ULONG ulAddress,
    ULONG ulNumPages
);

extern DWORD *g_pdwModLoadNotifDbg;

void EnableHDNotifs (BOOL fEnable);

void KdpFlushBreakpointKva(void);

// Define external references.

extern int g_nTotalNumDistinctSwCodeBps;
extern UCHAR g_abMessageBuffer[KDP_MESSAGE_BUFFER_SIZE];
extern BOOL g_fDbgConnected;
extern CRITICAL_SECTION csDbg;
extern EXCEPTION_INFO g_exceptionInfo;

// primary interface between nk and kd
extern KDSTUB_INIT g_kdKernData;
extern void (*g_pfnOutputDebugString)(char*, ...);

extern HDSTUB_DATA Hdstub;
extern HDSTUB_CLIENT g_KdstubClient;
extern SAVED_THREAD_STATE g_svdThread;

#define INTERRUPTS_ENABLE           (g_kdKernData.pfnINTERRUPTS_ENABLE)
#define InitializeCriticalSection   (g_kdKernData.pfnInitializeCriticalSection)
#define DeleteCriticalSection       (g_kdKernData.pfnDeleteCriticalSection)
#define EnterCriticalSection        (g_kdKernData.pfnEnterCriticalSection)
#define LeaveCriticalSection        (g_kdKernData.pfnLeaveCriticalSection)

#define pTOC                        (g_kdKernData.pTOC)
#define kdpKData                    (g_kdKernData.pKData)
#define g_pprcNK                    (g_kdKernData.pprcNK)
#define KCall                       (g_kdKernData.pKCall)
#define kdpInvalidateRange          (g_kdKernData.pInvalidateRange)
#define kdpIsROM                    (g_kdKernData.pkdpIsROM)
#define KdCleanup                   (g_kdKernData.pKdCleanup)
#define KDEnableInt                 (g_kdKernData.pKDEnableInt)
#define pfnIsDesktopDbgrExist       (g_kdKernData.pfnIsDesktopDbgrExist)
#define NKwvsprintfW                (g_kdKernData.pNKwvsprintfW)
#define NKDbgPrintfW                (g_kdKernData.pNKDbgPrintfW)
#define pulHDEventFilter            (g_kdKernData.pulHDEventFilter)
#define KdpIsProcessorFeaturePresent (g_kdKernData.pfnIsProcessorFeaturePresent)
#define kdpHandleToHDATA            (g_kdKernData.pfnHandleToHDATA)
#define Entry2PTBL                  (g_kdKernData.pfnEntry2PTBL)
#if defined(MIPS)
#define InterlockedDecrement        (g_kdKernData.pInterlockedDecrement)
#define InterlockedIncrement        (g_kdKernData.pInterlockedIncrement)
#endif
#if defined(ARM)
#define InSysCall                   (g_kdKernData.pInSysCall)
#endif


extern BOOL KDIoControl (DWORD dwIoControlCode, LPVOID lpBuf, DWORD nBufSize);


typedef struct {
    ULONG Addr;                 // pc address of breakpoint
    ULONG Flags;                // Flags bits
    ULONG Calls;                // # of times traced routine called
    ULONG CallsLastCheck;       // # of calls at last periodic (1s) check
    ULONG MaxCallsPerPeriod;
    ULONG MinInstructions;      // largest number of instructions for 1 call
    ULONG MaxInstructions;      // smallest # of instructions for 1 call
    ULONG TotalInstructions;    // total instructions for all calls
    ULONG Handle;               // handle in (regular) bpt table
    PVOID Thread;               // Thread that's skipping this BP
    ULONG ReturnAddress;        // return address (if not COUNTONLY)
} DBGKD_INTERNAL_BREAKPOINT, *PDBGKD_INTERNAL_BREAKPOINT;

void CpuContextToContext(CONTEXT *pCtx, CPUCONTEXT *pCpuCtx);

#ifdef MIPSII
#define Is16BitSupported         (kdpKData->dwArchFlags & MIPS_FLAG_MIPS16)
#elif defined (THUMBSUPPORT)
#define Is16BitSupported         (1)
#else
#define Is16BitSupported         (0)
#endif

