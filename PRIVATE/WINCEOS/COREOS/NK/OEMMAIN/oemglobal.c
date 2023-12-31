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
/*
 *
 *
 * Module Name:
 *
 *              oemglb.c
 *
 * Abstract:
 *
 *              This file define the OEM globals.
 *
 */

#include "kernel.h"
#include <bldver.h>

#define DEFAULT_WATCHDOG_PRIORITY   100
#define DEFAULT_THREAD_QUANTUM      100

static BOOL ReturnFalse (void)
{
    return FALSE;
}

static DWORD ReturnSelf (DWORD id)
{
    return id;
}

static DWORD FakeCalcPage (DWORD dwTotal, DWORD dwDefault)
{
    return dwDefault;
}

// compiler /GS runtime
extern DWORD_PTR __security_cookie;
extern DWORD_PTR __security_cookie_complement;

extern POEMGLOBAL g_pOemGlobal;
extern PNKGLOBAL  g_pNKGlobal;

#ifdef KITLOEM
extern BOOL WINAPI KitlDllMain (HINSTANCE DllInstance, DWORD dwReason, LPVOID Reserved);
#endif

static OEMGLOBAL OemGlobal = {
    MAKELONG (CE_MINOR_VER, CE_MAJOR_VER),  // DWORD                   dwVersion;

    // initialization
    OEMInitDebugSerial,                     // PFN_InitDebugSerial     pfnInitDebugSerial;
    OEMInit,                                // PFN_InitPlatform        pfnInitPlatform;

    // debug functions
    OEMWriteDebugByte,                      // PFN_WriteDebugByte      pfnWriteDebugByte;
    (PFN_WriteDebugString)OEMWriteDebugString,  // PFN_WriteDebugString    pfnWriteDebugString;
    OEMReadDebugByte,                       // PFN_ReadDebugByte       pfnReadDebugByte;
    (PFN_WriteDebugLED)ReturnFalse,         // PFN_WriteDebugLED       pfnWriteDebugLED;

    // cache fuctions
    OEMCacheRangeFlush,                     // PFN_CacheRangeFlush     pfnCacheRangeFlush;

    // time related funcitons
    (PFN_InitClock)ReturnFalse,             // PFN_InitClock           pfnInitClock;
    OEMGetRealTime,                         // PFN_GetRealTime         pfnGetRealTime;
    (PFN_SetRealTime)OEMSetRealTime,        // PFN_SetRealTime         pfnSetRealTime;
    (PFN_SetAlarmTime)OEMSetAlarmTime,      // FPN_SetAlarmTime        pfnSetAlarmTime;
    NULL,                                   // PFN_QueryPerfCounter    pfnQueryPerfCounter;
    NULL,                                   // PFN_QueryPerfFreq       pfnQueryPerfFreq;
    OEMGetTickCount,                        // PFN_GetTickCount        pfnGetTickCount;

    // scheduler related functions
    OEMIdle,                                // PFN_Idle                pfnIdle;
    (PFN_NotifyThreadExit)ReturnFalse,      // PFN_NotifyThreadExit    pfnNotifyThreadExit;
    (PFN_NotifyReschedule)ReturnFalse,      // PFN_NotifyReschedule    pfnNotifyReschedule;
    ReturnSelf,                             // PFN_NotifyIntrOccurs    pfnNotifyIntrOccurs;
    NULL,                                   // PFN_UpdateReschedTime   pfnUpdateReschedTime;
    DEFAULT_THREAD_QUANTUM,                 // dwDefaultTheadQuantum

    // power related functions
    OEMPowerOff,                            // PFN_PowerOff            pfnPowerOff;

    // DRAM detection functions
    OEMGetExtensionDRAM,                    // PFN_GetExtensionDRAM    pfnGetExtensionDRAM;
    NULL,                                   // PFN_EnumExtensionDRAM   pfnEnumExtensionDRAM;
    FakeCalcPage,                           // PFN_CalcFSPages         pfnCalcFSPages;
    0,                                      // DWORD                   dwMainMemoryEndAddress;

    // interrupt handling functions
    OEMInterruptEnable,                     // PFN_InterruptEnable     pfnInterruptEnable;
    OEMInterruptDisable,                    // PFN_InterruptDisable    pfnInterruptDisable;
    OEMInterruptDone,                       // PFN_InterruptDone       pfnInterruptDone;
    OEMInterruptMask,                       // PFN_InterruptMask       pfnInterruptMask;

    // co-proc support
    (PFN_InitCoProcRegs)ReturnFalse,        // PFN_InitCoProcRegs      pfnInitCoProcRegs;
    (PFN_SaveCoProcRegs)ReturnFalse,        // PFN_SaveCoProcRegs      pfnSaveCoProcRegs;
    (PFN_RestoreCoProcRegs)ReturnFalse,     // PFN_RestoreCoProcRegs   pfnRestoreCoProcRegs;
    0,                                      // DWORD                   cbCoProcRegSize;
    FALSE,                                  // DWORD                   fSaveCoProcReg;

    // persistent registry support
    NULL,                                   // PFN_ReadRegistry        pfnReadRegistry;
    NULL,                                   // PFN_WriteRegistry       pfnWriteRegistry;

    // watchdog support
    (PFN_RefreshWatchDog)ReturnFalse,       // PFN_RefreshWatchDog     pfnRefreshWatchDog;
    0,                                      // DWORD                   dwWatchDogPeriod;
    DEFAULT_WATCHDOG_PRIORITY,              // DWORD                   dwWatchDogThreadPriority;

    // profiler support
    NULL,                                   // PFN_ProfileTimerEnable  pfnProfileTimerEnable;
    NULL,                                   // PFN_ProfileTimerDisable pfnProfileTimerDisable;

    // Simple RAM based Error Reporting support
    0,                                      // DWORD                   cbErrReportSize;    // was cbNKDrWatsonSize
    
    // others
    OEMIoControl,                           // PFN_Ioctl               pfnOEMIoctl;
    (PFN_KDIoctl)ReturnFalse,               // PFN_KDIoctl             pfnKDIoctl;
    (PFN_IsRom)ReturnFalse,                 // PFN_IsRom               pfnIsRom;
    NULL,                                   // PFN_MapW32Priority      pfnMapW32Priority;
    (PFN_SetMemoryAttributes)ReturnFalse,   // PFN_SetMemoryAttributes pfnSetMemoryAttributes;
    (PFN_IsProcessorFeaturePresent)ReturnFalse, // PFN_IsProcessorFeaturePresent   pfnIsProcessorFeaturePresent;
    (PFN_HaltSystem) ReturnFalse,           // PFN_HaltSystem          pfnHaltSystem;
    (PFN_NotifyForceCleanBoot) ReturnFalse, // PFN_NotifyForceCleanBoot pfnNotifyForceCleanBoot;
    
    NULL,                                   // PROMChain_t             pROMChain;
    
    // Platform specific information passed from OAL to KITL.
    NULL,                                   // LPVOID                  pKitlInfo
#ifdef KITLOEM
    KitlDllMain,                            // KITL entry point (KITL is part of OEM)        
#else
    NULL,                                   // KITL entry point (KITL is in a separate DLL)  
#endif

#ifdef DEBUG
    &dpCurSettings,                         // DBGPARAM                 *pdpCurSettings
#else
    NULL,
#endif

    &__security_cookie,                     // DWORD_PTR *             p__security_cookie
    &__security_cookie_complement,          // DWORD_PTR *             p__security_cookie_complement

    DEFAULT_ALARMRESOLUTION_MSEC,           // DWORD                   dwAlarmResolution; 
    0,                                      // DWORD                   dwYearsRTCRollover; 

#if defined(x86)
    0, 0, 0, 0, 0, 0, 0, 0,                 // DWORD                   _rsvd_[8]

    // CPU dependent functions
    OEMNMIHandler,           //PFN_NMIHandler          pfnNMIHandler;
#elif defined (ARM)
    0, 0, 0, 0, 0, 0,                       // DWORD                   _rsvd_[6]

    NULL,                                   // PFN_PTEUpdateBarrier    pfnPTEUpdateBarrier         // optional
    0,                                      // DWORD                   dwTTBRCacheBits;    

    OEMInterruptHandler,                    // PFN_InterruptHandler        pfnInterruptHandler;        // interrupt handler
    OEMInterruptHandlerFIQ,                 // PFN_InterruptHandler        pfnFIQHandler;              // FIQ handler
    
    0,                                      // DWORD                       dwARM1stLvlBits;            // extra bits to be set in 1st level page table
    0,                                      // DWORD                       dwARMCacheMode;             // C and B bits
    FALSE,                                  // DWORD                       f_V6_VIVT_ICache;           // v6 specific - if I-Cache is VIVT ASID-tagged

    // VFP funcitons
    (PFN_SaveRestoreVFPCtrlRegs)ReturnFalse, // PFN_SaveRestoreVFPCtrlRegs pfnSaveVFPCtrlRegs;
    (PFN_SaveRestoreVFPCtrlRegs)ReturnFalse, // PFN_SaveRestoreVFPCtrlRegs pfnRestoreVFPCtrlRegs;
    (PFN_HandleVFPException)ReturnFalse,    // PFN_HandleVFPException  pfnHandleVFPExcp;
#elif defined (MIPS)
    0, 0, 0, 0, 0, 0, 0, 0,                 // DWORD                   _rsvd_[8]

#define DEFAULT_FPU_ENABLE                  0x20000000    
    DEFAULT_FPU_ENABLE,                     // DWORD                   dwCoProcBits;               // platform specific co-proc enable bits (default 0x20000000)
    0,                                      // DWORD                   dwOEMTLBLastIdx;            // size of TLB - 1
    MIPS_FLAG_NO_OVERRIDE,                  // DWORD                   dwArchFlagOverride;         // architecture flag override
    NULL,                                   // const BYTE *pIntrPrio;                  // interrupt prority (ptr to 64 bytes)
    NULL,                                   // const BYTE *pIntrMask;                  // interrupt mask (ptr to 8 bytes)
#elif defined (SHx)
    0, 0, 0, 0, 0, 0, 0, 0,                 // DWORD                   _rsvd_[8]

    OEMNMI,                                 // PFN_SHxNMIHandler          pfnNMIHandler;
    SH4_INTEVT_LENGTH,                      // platform specific interrupt event code length
#endif
};

POEMGLOBAL g_pOemGlobal = &OemGlobal;

#ifdef MIPS
extern const DWORD dwOEMArchOverride;
extern DWORD OEMTLBSize;
extern BYTE IntrPriority [];
extern BYTE IntrMask [];
#endif

#ifdef SHx
extern const DWORD dwSHxIntEventCodeLength;
#endif

POEMGLOBAL OEMInitGlobals (PNKGLOBAL pNKGlobal)
{
    g_pNKGlobal  = pNKGlobal;

#if defined (ARM)
    g_pOemGlobal->dwARMCacheMode = OEMARMCacheMode ();
#elif defined (MIPS)
    g_pOemGlobal->dwOEMTLBLastIdx = OEMTLBSize;   // OEMTLBSize should probably be renamed...
    g_pOemGlobal->pIntrPrio = IntrPriority;
    g_pOemGlobal->pIntrMask = IntrMask;
    g_pOemGlobal->dwArchFlagOverride = dwOEMArchOverride;
#elif defined (SHx)
    g_pOemGlobal->dwSHxIntEventCodeLength = dwSHxIntEventCodeLength;
#endif
    return g_pOemGlobal;
}

