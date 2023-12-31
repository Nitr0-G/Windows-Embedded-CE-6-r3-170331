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

#include <kernel.h>
#include <psapi.h>

#ifndef _PREFAST_
#pragma warning(disable: 4068) // Disable pragma warnings
#endif
// memory usage watermark, borrowed from physmem.c
#define PAGEOUT_LOW         (( 68*1024)/VM_PAGE_SIZE)  // do pageout once below this mark, reset when above high
#define PAGEOUT_HIGH        ((132*1024)/VM_PAGE_SIZE)

#undef THSNAP_RESERVE
#define THSNAP_RESERVE_MIN (4*1024*1024)
#define THSNAP_RESERVE_MAX (128*1024*1024)
static DWORD s_THSnapReserve = THSNAP_RESERVE_MIN;

extern CRITICAL_SECTION rtccs, NameCS;
extern long PageOutTrigger;    // Threshold level to start page out.
extern long PageOutLevel;      // Threshold to stop page out.
extern PNAME pDebugger;
PNAME pSearchPath;

DWORD g_fSysFileReadable;

// Default registry values, may be overwritten with actual desktop zone settings
DWORD g_dwKeys[HOST_TRANSCFG_NUM_REGKEYS] = {
    (DWORD)-1,          // nk
    (DWORD)-1,          // filesys
    (DWORD)-1,          // fsdmgr
    (DWORD)-1,          // relfsd
    (DWORD)-1,          // device
    CELOG_CE_MASK,      // CeLogZoneCE
    CELOG_USER_MASK,    // CeLogZoneUser
    CELOG_PROCESS_MASK, // CeLogZoneProcess
};
WCHAR* g_keyNames[] = {L"Nk", L"FileSys", L"FSDMGR", L"ReleaseFSD", L"Device", L"CeLogZoneCE", L"CeLogZoneUser", L"CeLogZoneProcess"};

void UpdateCallerInfo (PTHREAD pth, BOOL fInKMode);

/* xref ref regset
; NK references two registry settings:
;   HKLM\Debug\JITDebugger - for just in time debugging
; and
;   HKLM\Loader\SystemPath - for alternative loader path

[HKEY_LOCAL_MACHINE\Debug]
   ; update the JITDebugger field to be the JIT debugger of your platform
   ;"JITDebugger"="My Debugger Name"

[HKEY_LOCAL_MACHINE\Loader]
   ; specifies all the alternative paths for loader to search when loading an app/module
   ; Note that every string must end with "\\" as the loader uses direct concantenation
   ; to obtain the path of the exe/dll.
   ;"SystemPath"=multi_sz:"AltPath1\\","AltPath2\\"
[HKEY_LOCAL_MACHINE\KERNEL]
   ; specifies all DLLs file name to be injected into process
   ;"InjectDLL"=multi_sz:"INJECT1.DLL","INJECT2.DLL"
[HKEY_LOCAL_MACHINE\KERNEL\TOOLHELP]
   ; specifies toolhelp reserve size between THSNAP_RESERVE_MIN and THSNAP_RESERVE_MAX. 
   ;"THSnapReserve"=dword:00400000
*/


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
VOID
NKGetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    DEBUGMSG(ZONE_ENTRY,(L"NKGetSystemInfo entry: %8.8lx\r\n",lpSystemInfo));
    lpSystemInfo->wProcessorArchitecture = PROCESSOR_ARCHITECTURE;
    lpSystemInfo->wReserved = 0;
    lpSystemInfo->dwPageSize = VM_PAGE_SIZE;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)0x10000;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)0x7fffffff;
    lpSystemInfo->dwActiveProcessorMask = 1;
    lpSystemInfo->dwNumberOfProcessors = 1;
    lpSystemInfo->dwProcessorType = CEProcessorType;
    lpSystemInfo->wProcessorLevel = CEProcessorLevel;
    lpSystemInfo->wProcessorRevision = CEProcessorRevision;
    lpSystemInfo->dwAllocationGranularity = 0x10000;
    DEBUGMSG(ZONE_ENTRY,(L"NKGetSystemInfo exit\r\n"));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL NKQueryInstructionSet(DWORD dwInstSet, LPDWORD lpdwOSInstSet)
{
    BOOL fRet = TRUE;

    DEBUGMSG(ZONE_ENTRY,(L"NKQueryInstructionSet entry: %8.8lx %8.8lx\r\n",dwInstSet, lpdwOSInstSet));

    if (dwInstSet != CEInstructionSet) {

        switch (dwInstSet) {
        case PROCESSOR_QUERY_INSTRUCTION:
            // query always okay
            break;
        case PROCESSOR_MIPS_MIPSII_INSTRUCTION:
            // MIPSII can run on MIPS16
            fRet = (PROCESSOR_MIPS_MIPS16_INSTRUCTION == CEInstructionSet);
            break;

        // OS never build THUMB anymore
        case PROCESSOR_ARM_V4IFP_INSTRUCTION:
        case PROCESSOR_ARM_V4TFP_INSTRUCTION:
            fRet = (PROCESSOR_ARM_V4IFP_INSTRUCTION == CEInstructionSet);
            break;
        case PROCESSOR_ARM_V4T_INSTRUCTION:
        case PROCESSOR_ARM_V4I_INSTRUCTION:
        case PROCESSOR_ARM_V4_INSTRUCTION:
            fRet = (PROCESSOR_ARM_V4I_INSTRUCTION == CEInstructionSet)
                || (PROCESSOR_ARM_V4IFP_INSTRUCTION == CEInstructionSet);
            break;

        default:
            // MIPS use different calling convention between HW FP and FP emulator
            // so don't support FP emulator binary on HW FP OS.
            fRet = ((CEInstructionSet >> 24) != PROCESSOR_ARCHITECTURE_MIPS)
                && ((dwInstSet | PROCESSOR_FEATURE_FP) == CEInstructionSet);
            break;
        }
    }

    __try {
        *lpdwOSInstSet = CEInstructionSet;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        KSetLastError(pCurThread,ERROR_INVALID_PARAMETER);
        fRet = FALSE;
    }

    DEBUGMSG(ZONE_ENTRY,(L"NKQueryInstructionSet exit %d\r\n", fRet));
    return fRet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKIsProcessorFeaturePresent(
    DWORD dwProcessorFeature
    )
{
    BOOL fRet = FALSE;

    DEBUGMSG(ZONE_ENTRY,(L"NKIsProcessorFeaturePresent entry: %8.8lx\r\n",dwProcessorFeature));
    switch( dwProcessorFeature )
    {
#ifdef _X86_
//
// x86 Processor Features
//
    case PF_FLOATING_POINT_PRECISION_ERRATA:
        fRet = ProcessorFeatures;
        break;
    case PF_MMX_INSTRUCTIONS_AVAILABLE:
        fRet = ProcessorFeatures & CPUID_MMX;
        break;
    case PF_3DNOW_INSTRUCTIONS_AVAILABLE:
        fRet = ProcessorFeaturesEx & CPUIDEX_3DNOW;
        break;
    case PF_XMMI_INSTRUCTIONS_AVAILABLE:
        fRet = ProcessorFeatures & CPUID_SSE;
        break;
    case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
        fRet = ProcessorFeatures & CPUID_SSE2;
        break;
    case PF_RDTSC_INSTRUCTION_AVAILABLE:
        fRet = ProcessorFeatures & CPUID_TSC;
        break;
    case PF_FLOATING_POINT_EMULATED:
        fRet = (dwFPType == FPTYPE_SOFTWARE);
        break;
    case PF_COMPARE_EXCHANGE_DOUBLE:
        fRet = ProcessorFeatures & CPUID_CX8;
        break;

#elif defined (ARM)
//
// ARM Processor Features
//
    case PF_ARM_VFP10:
        fRet = vfpStat & VFP_EXIST;
        break;

    case PF_ARM_PHYSICALLY_TAGGED_CACHE:
    case PF_ARM_V6:
    case PF_ARM_UNALIGNED_ACCESS:
        fRet = IsV6OrLater ();
        break;

    case PF_ARM_V4:
#if defined(ARMV4I) || defined(ARMV4T)
    case PF_ARM_THUMB:
#endif
        fRet = TRUE;
        break;

#elif defined (MIPS)
//
// MIPS Processor Features
//
    case PF_MIPS_MIPS16:
        fRet = IsMIPS16Supported;
        break;
    case PF_MIPS_MIPSII:
#ifdef MIPSIV
    case PF_MIPS_MIPSIV:
#endif
#ifdef MIPS_HAS_FPU
    case PF_MIPS_FPU:
#endif
        fRet = TRUE;
        break;


#elif defined (SHx)
//
// SHx Processor Features
//

#ifdef SH3
    case PF_SHX_SH3:
#elif defined (SH4)
    case PF_SHX_SH4:
    case PF_SHX_FPU:
#else
    #error Unknown CPU
#endif
        fRet = TRUE;
        break;

#endif


    default:
        // call down to OEM for everything we don't know
        fRet = g_pOemGlobal->pfnIsProcessorFeaturePresent (dwProcessorFeature);
        break;
    }
    DEBUGMSG(ZONE_ENTRY,(L"NKIsProcessorFeaturePresent exit %d\r\n", fRet));
    return fRet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKIsBadPtr(
    DWORD flag,
    LPBYTE lpv,
    DWORD len
    )
{
    BOOL  fRet;

    DEBUGMSG(ZONE_ENTRY,(L"NKIsBadPtr entry: %8.8lx %8.8lx %8.8lx\r\n",flag,lpv,len));

    if ((int) len <= 0) {
        fRet = (len != 0);
        
    } else if ((g_pprcNK != pActvProc) 
        && !IsValidUsrPtr (lpv, len, (VERIFY_WRITE_FLAG == flag))) {
        fRet = TRUE;
    } else {
        DWORD lFlags = ((VERIFY_WRITE_FLAG == flag)? VM_LF_WRITE : VM_LF_READ) | VM_LF_QUERY_ONLY;
    
        fRet = !VMLockPages (pVMProc, lpv, len, NULL, lFlags);
    }

    DEBUGMSG(ZONE_ENTRY,(L"NKIsBadPtr returns: %d\r\n", fRet));
    
    return fRet;
}



//------------------------------------------------------------------------------
// @func DWORD | GetFSHeapInfo | Gets info on the physical space reserved for the file system
// @comm Retrieves the start of the physical memory reserved for the file system
//------------------------------------------------------------------------------
DWORD NKGetFSHeapInfo (void)
{
    DEBUGMSG(ZONE_ENTRY,(L"NKGetFSHeapInfo entry\r\n"));
    DEBUGMSG(ZONE_ENTRY,(L"NKGetFSHeapInfo exit: %8.8lx\r\n",PAGEALIGN_UP(pTOC->ulRAMFree + MemForPT)));
    return PAGEALIGN_UP(pTOC->ulRAMFree+MemForPT);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
NKUpdateNLSInfo(
    DWORD ocp,
    DWORD acp,
    DWORD sysloc,
    DWORD userloc
    )
{

    KInfoTable[KINX_NLS_CP] = (DWORD)(((WORD)ocp << 16) + (WORD)acp);
    KInfoTable[KINX_NLS_SYSLOC] = sysloc;
    KInfoTable[KINX_NLS_USERLOC] = userloc;
}

DWORD randdw1, randdw2;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
__int64 NKGetRandomSeed (void)
{
    return (((__int64)randdw1)<<32) | (__int64)randdw2;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD
NKGetIdleTime(void)
{
    DWORD result;

    DEBUGMSG(ZONE_ENTRY,(L"NKGetIdleTime entry\r\n"));
    if (g_pNKGlobal->dwIdleConv) {
        result = (DWORD)(g_pNKGlobal->liIdle.QuadPart/g_pNKGlobal->dwIdleConv);
    } else {
        result = 0xffffffff;
    }
    DEBUGMSG(ZONE_ENTRY,(L"NKGetIdleTime exit: %8.8lx\r\n",result));
    return result;
}

static DWORD QueryReg (LPCWSTR pszValName, LPCWSTR pszReserved, DWORD dwType, LPVOID pBuffer, DWORD cbBufSize)
{
    DWORD dwrtype;
    return ((NKRegQueryValueExW (HKEY_LOCAL_MACHINE, pszValName, (LPDWORD) pszReserved, &dwrtype, (LPBYTE)pBuffer, &cbBufSize) == ERROR_SUCCESS)
            && (dwrtype == dwType))
        ? cbBufSize
        : 0;
}

static void InitSystemSettings (void)
{
    BYTE  pBuffer[MAX_PATH*sizeof(WCHAR)];
    DWORD cbSize;
    DWORD cPages = 0, cBytes = 0;
    DWORD cSnapReserve;

    // query JIT debugger path
    if ((cbSize = QueryReg (L"JITDebugger", L"Debug", REG_SZ, pBuffer, sizeof (pBuffer)))
        && (pDebugger = AllocName (cbSize))) {
        memcpy (pDebugger->name, pBuffer, cbSize);
    }

    // query system search path
    if ((cbSize = QueryReg (L"SystemPath", L"Loader", REG_MULTI_SZ, pBuffer, sizeof (pBuffer)))
        && (pSearchPath = AllocName (cbSize)))
        memcpy (pSearchPath->name, pBuffer, cbSize);

    // set default value for PageOutTrigger and PageOutLevel
    QueryReg (L"cbLow", L"SYSTEM\\OOM", REG_DWORD, &cBytes, sizeof (DWORD));
    QueryReg (L"cpLow", L"SYSTEM\\OOM", REG_DWORD, &cPages, sizeof (DWORD));
    
    if (cBytes || cPages) {
        cBytes = (cBytes + VM_PAGE_SIZE -1) / VM_PAGE_SIZE;   // convert to pages
        cPages = (cBytes>cPages)? cBytes : cPages;
    }
    PageOutTrigger = cPages + PAGEOUT_LOW;
    PageOutLevel = cPages + PAGEOUT_HIGH;

    // All read access on system file for all processes?
    QueryReg (L"AllowSystemAccess", L"SYSTEM\\ObjectStore", REG_DWORD, &g_fSysFileReadable, sizeof(DWORD));

    if (QueryReg (L"THSnapReserve", L"KERNEL\\TOOLHELP", REG_DWORD, &cSnapReserve, sizeof (DWORD)))
    {
        if (cSnapReserve > THSNAP_RESERVE_MAX) s_THSnapReserve = THSNAP_RESERVE_MAX;
        else if (cSnapReserve < THSNAP_RESERVE_MIN) s_THSnapReserve = THSNAP_RESERVE_MIN;
        else s_THSnapReserve = cSnapReserve;
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD WINAPI
RunApps(
    LPVOID param
    )
{
    HMODULE hFilesys;
    DEBUGMSG (ZONE_ENTRY, (L"RunApps started\r\n"));

    CELOG_LaunchingFilesys();

    hFilesys = (HMODULE) NKLoadLibraryEx (L"filesys.dll", MAKELONG (LOAD_LIBRARY_IN_KERNEL, LLIB_NO_PAGING), NULL);

    if (hFilesys) {
        FARPROC pfnMain = GetProcAddressA (hFilesys, (LPCSTR) 2);   // WinMain of filesys
        HANDLE hFSReady, hTh;

        DEBUGCHK (pfnMain);

        hFSReady = NKCreateEvent (NULL, TRUE, FALSE, TEXT("SYSTEM/FSReady"));
        hTh = CreateKernelThread ((LPTHREAD_START_ROUTINE)pfnMain, hFilesys, THREAD_RT_PRIORITY_NORMAL, 0);

        DEBUGCHK (hTh && hFSReady);
        HNDLCloseHandle (g_pprcNK, hTh);

        // If pSignalStarted is NULL, we don't have filesys (tinykern). Don't bother waiting for it.
        if (pSignalStarted) {
            NKWaitForSingleObject (hFSReady, INFINITE);

            DEBUGCHK (SystemAPISets[SH_FILESYS_APIS]);

            // Initialize MUI-Resource loader (requires registry)
            InitMUILanguages();

            // Read system settings from registry
            InitSystemSettings ();

            // signal filesys that we're done
            (* pSignalStarted) (0);
        }
        HNDLCloseHandle (g_pprcNK, hFSReady);
   
    } else {
        RETAILMSG (1, (L"Filesys doesn't exist, no app started\r\n"));
    }

    // instead of exiting, we're make this thread cleaning dirty pages in the background.
    CleanPagesInTheBackground ();

    // should've never returned
    DEBUGCHK (0);
    NKExitThread (0);

    return 0;
}

#define INVALID_DBG_ZONES       0xffffffff

typedef struct {
    LPDBGPARAM pdbgSrc;
    LPDBGPARAM pdbgTgt;
    DWORD dwNewZone;
} DbgZoneEnumStruct, *PDbgZoneEnumStruct;

static BOOL UpdateDbgZone (PPROCESS pprc, LPDBGPARAM pdbgSrc, LPDBGPARAM pdbgTgt, DWORD dwNewZone)
{
    BOOL fRet = FALSE;

    if ((DWORD) pdbgSrc & 0x80000000) {
        // kernel module or NK itself
        if (INVALID_DBG_ZONES != dwNewZone) {
            pdbgSrc->ulZoneMask = dwNewZone;
        }
        
        if (pdbgTgt) {
            fRet = CeSafeCopyMemory (pdbgTgt, pdbgSrc, sizeof (DBGPARAM));
        }

    } else {

        if (INVALID_DBG_ZONES != dwNewZone) {
            PROCWriteMemory (pprc, &pdbgSrc->ulZoneMask, &dwNewZone, sizeof (DWORD));
        }

        if (pdbgTgt) {
            fRet = PROCReadMemory (pprc, pdbgSrc, pdbgTgt, sizeof (DBGPARAM));
        }
    }
    return fRet;
}

static BOOL EnumWriteDbgZone (PDLIST pItem, LPVOID pEnumData)
{
    PPROCESS pprc = (PPROCESS) pItem;
    PDbgZoneEnumStruct pdze = (PDbgZoneEnumStruct) pEnumData;

    if (UpdateDbgZone (pprc, pdze->pdbgSrc, pdze->pdbgTgt, pdze->dwNewZone)) {
        pdze->pdbgTgt = NULL;   // updated, don't need to read again
    }

    return (INVALID_DBG_ZONES == pdze->dwNewZone)
        && !pdze->pdbgTgt;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKSetDbgZone(
    DWORD dwProcId,
    PMODULE pMod,
    LPVOID BasePtr, // unused
    DWORD dwZone,
    LPDBGPARAM pdbgTgt
    )
{
    DWORD dwErr = 0;

    


    if (dwProcId) {

        // set debug zones of process

        PHDATA phd = LockHandleData ((HANDLE) dwProcId, g_pprcNK);
        PPROCESS pprc = GetProcPtr (phd);

        if (!pprc || pMod || !pprc->ZonePtr) {
            dwErr = ERROR_INVALID_PARAMETER;
            
        } else {

            UpdateDbgZone (pprc, pprc->ZonePtr, pdbgTgt, dwZone);
        }

        UnlockHandleData (phd);
        
    } else {

        // set debugzones of module

        PMODULELIST pEntry;

        LockModuleList ();
        pEntry = FindModInProc (g_pprcNK, pMod);
        if (!pEntry || !pMod->ZonePtr) {
            dwErr = ERROR_INVALID_PARAMETER;

        } else {

            // user mode module, update all process
            DbgZoneEnumStruct dze = { pMod->ZonePtr, pdbgTgt, dwZone };

            if (INVALID_DBG_ZONES != dwZone) {
                pMod->dwZone = dwZone;
            }
            
            if (IsDListEmpty (&g_pprcNK->prclist)) {
                // setting debug zones before any process is started
                UpdateDbgZone (g_pprcNK, pMod->ZonePtr, pdbgTgt, dwZone);
            } else
            {
                EnumerateDList (&g_pprcNK->prclist, EnumWriteDbgZone, &dze);
            }
        }
        UnlockModuleList ();
    }

    if (dwErr) {
        KSetLastError (pCurThread, dwErr);
    }

    return !dwErr;
}


void ReadZoneFromRegistry (DWORD dwProcId, PMODULE pMod, LPDBGPARAM pdbgparam)
{

    if (SystemAPISets[SH_FILESYS_APIS]) {
        
        DWORD dwKeyReg = 0;
        DWORD dwKeyZones = 0;
        DWORD dwType, dwSize = sizeof(DWORD), dwZone = INVALID_DBG_ZONES;
        WCHAR szUnicode[MAX_PATH];

        // make a local copy of name for security
        NKwcsncpy (szUnicode, pdbgparam->lpszName, MAX_PATH);

        //
        // See if the debug zone mask is specified in the desktop PC's registry
        //
        rRegOpen((DWORD)HKEY_CURRENT_USER, "Pegasus", &dwKeyReg);
        if (dwKeyReg) {
            // remote registry name is ASCII, not UNICODE
            CHAR szName[MAX_PATH];
            rRegOpen(dwKeyReg, "Zones", &dwKeyZones);
            if (dwKeyZones) {
                NKUnicodeToAscii(szName, szUnicode, sizeof(szName));
                rRegGet(dwKeyZones, szName, &dwType, (LPBYTE)&dwZone, &dwSize);
                rRegClose(dwKeyZones);
                if (REG_DWORD != dwType) {
                    dwZone = INVALID_DBG_ZONES;
                }
            }
            rRegClose(dwKeyReg);
        } else {
            int i;
            for (i = 0; i < HOST_TRANSCFG_NUM_REGKEYS; i++) {
                if (NKwcscmp (szUnicode, g_keyNames[i]) == 0) {
                    dwZone = g_dwKeys[i];
                    break;
                }
            }
        }

        //
        // Check the local registry if the filesys API is registered.
        //
        if (INVALID_DBG_ZONES == dwZone) {
            HKEY hKeyZones = NULL;

            //
            // See if the debug zone mask is specified in the Windows CE registry
            // (in HLM\DebugZones:<module name>)
            //
            if (NKRegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("DebugZones"), 0, 0,
                            &hKeyZones) == ERROR_SUCCESS) {
                dwSize = sizeof(DWORD);
                NKRegQueryValueEx(hKeyZones, szUnicode, 0, &dwType,
                            (LPBYTE)&dwZone, &dwSize);
                NKRegCloseKey(hKeyZones);
                if (REG_DWORD != dwType) {
                    dwZone = INVALID_DBG_ZONES;
                }
            }
        }
        
        if (INVALID_DBG_ZONES != dwZone) {
            NKSetDbgZone (dwProcId, pMod, NULL, dwZone, NULL);
        }
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKRegisterDbgZones (PMODULE pMod,
    LPDBGPARAM pdbgparam
    )
{
    o32_lite *optr = NULL;
    e32_lite *eptr = NULL;
    LPDBGPARAM *ppdbg = NULL;
    PPROCESS pprc = pActvProc;
    LPDWORD pdwZone = NULL;
    DWORD dwProcId = 0;

    BOOL fRet = FALSE;
    DEBUGMSG (ZONE_LOADER1, (L"NKRegisterDbgZones %8.8lx %8.8lx\r\n", pMod, pdbgparam));
    if (pMod) {
        // registering debugzone for module
        LockModuleList ();
        if (FindModInProc (pprc, pMod)) {
            eptr = &pMod->e32;
            optr = pMod->o32_ptr;
            ppdbg = &pMod->ZonePtr;
            pdwZone = &pMod->dwZone;
        }
        UnlockModuleList ();
    } else if (pprc != g_pprcNK) {
        // registering debugzone for process
        dwProcId = pprc->dwId;
        eptr = &pprc->e32;
        optr = pprc->o32_ptr;
        ppdbg = &pprc->ZonePtr;
    } else {
        DEBUGMSG (1, (L"!!!! Calling RegisterDbgZones on NK, ignored\r\n"));
        DEBUGMSG (1, (L"!!!! Most likely coming from Dlls that use to be an EXE\r\n"));
    }

    if (eptr
        && (optr = FindOptr (optr, eptr->e32_objcnt, (DWORD) pdbgparam))
        && (optr->o32_flags & IMAGE_SCN_MEM_WRITE)) {
        // valid
        *ppdbg = pdbgparam;
        if (pdwZone) {
            *pdwZone = pdbgparam->ulZoneMask;
        }
        ReadZoneFromRegistry (dwProcId, pMod, pdbgparam);
        
        fRet = TRUE;
    }
    DEBUGMSG (ZONE_LOADER1, (L"NKRegisterDbgZones returns %8.8lx\r\n", fRet));

    return fRet;
}

static DWORD DoTlsAlloc (PPROCESS pprc)
{
    DWORD   slotbase, bitidx;
    LPDWORD pdwUseMask = NULL;
    DWORD   mask;

    DEBUGCHK (OwnLoaderLock (pprc));
    DEBUGCHK ((TLSSLOT_RESERVE | pprc->tlsLowUsed) == pprc->tlsLowUsed);
    
    if (0xffffffff != pprc->tlsLowUsed) {
        // at least 1 unused slot in slot TLSSLOT_NUMRES-31
        pdwUseMask = &pprc->tlsLowUsed;
        slotbase   = 0;
        bitidx     = TLSSLOT_NUMRES;
        
    } else if (0xffffffff != pprc->tlsHighUsed){
        // at least 1 unused slot in slot 32-63
        // 32 == # of bits in a DWORD
        pdwUseMask = &pprc->tlsHighUsed;
        slotbase   = 32;
        bitidx     = 0;
        
    } else {
        // all used up
        return TLS_OUT_OF_INDEXES;
    }

    // search for the empty slot
    for (mask = (1 << bitidx); (mask & *pdwUseMask); bitidx ++, mask <<= 1) {
        // empty body
    }

    DEBUGCHK (bitidx < 32);

    // mark the slot in-use
    *pdwUseMask |= mask;

    return slotbase + bitidx;
}

static BOOL EnumZeroTLS (PDLIST pItem, LPVOID pEnumData)
{
    PTHREAD pth = ((PTHREAD)pItem);
    LPDWORD tlsPtr = TLSPTR (pth->dwOrigBase, pth->dwOrigStkSize);
    tlsPtr[(DWORD)pEnumData] = 0;
    return FALSE;   // keep search
}

static BOOL DoTlsFree (PPROCESS pprc, DWORD slot)
{
    BOOL fRet = FALSE;
    DEBUGCHK (OwnLoaderLock (pprc));
    if (IsInRange (slot, TLSSLOT_NUMRES, TLS_MINIMUM_AVAILABLE)) {
        LPDWORD pdwUseMask;
        DWORD   mask;
        // 32 == # of bits in a DWORD
        if (slot < 32) {
            pdwUseMask = &pprc->tlsLowUsed;
            mask       = (1 << slot);
        } else {
            pdwUseMask = &pprc->tlsHighUsed;
            mask       = (1 << (slot - 32));
        }

        if (*pdwUseMask & mask) {
            // this is a slot in use.
            __try {
                // clear the TLS slot for all threads in the process
                EnumerateDList (&pprc->thrdList, EnumZeroTLS, (LPVOID) slot);
                *pdwUseMask &= ~mask;
                fRet = TRUE;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                DEBUGMSG (1, (L"ERROR!! Exception in ZeroAllThreadTLS\r\n"));
            }
        }
    }
    return fRet;
}

//------------------------------------------------------------------------------
/* Thread local storage APIs */
//------------------------------------------------------------------------------
DWORD
NKTlsCall(
    DWORD type,
    DWORD slot
    )
{
    PTHREAD   pth = pCurThread;
    PPROCESS pprc = pActvProc;
    DWORD  retval = (TLS_FUNCALLOC == type)? TLS_OUT_OF_INDEXES : 0;

    DEBUGMSG(ZONE_ENTRY,(L"NKTlsCall entry: %8.8lx %8.8lx, pCurActiveProc = %8.8lx\r\n",type,slot, pprc));

    // We need to remove the debugchk for most DLL does TlsAlloc/TlsFree at PROCESS_ATTACH/PROCESS_DETACH.
    // It'll be okay as long as the thread doesn't perform TlsGet/TlsSet in other process. The DEBUGCHK
    // is now done in TlsGet/TlsSet.
#if 0
    // PSL should NOT call TlsAlloc/TlsFree on caller thread's stack. Fail the call if it does.
    if ((pth->pprcVM? pth->pprcVM: g_pprcNK) != pprc) {
        RETAILMSG (1, (L"Calling TlsAlloc/TlsFree in a Kernel PSL call, not a good idea!\r\n"));
        DEBUGCHK (0);
    } else 
#endif
    {
        LockLoader (pprc);
        
        switch (type) {
        case TLS_FUNCALLOC:
            retval = DoTlsAlloc (pprc);
            break;
        case TLS_FUNCFREE:
            retval = DoTlsFree (pprc, slot);
            break;
        default:
            DEBUGCHK(0);
            break;
        }        
        
        UnlockLoader (pprc);
    }

    // NOTE: NKTlsAlloc never return 0 on succees
    if (!retval || (TLS_OUT_OF_INDEXES == retval)) {
        KSetLastError (pth, ERROR_INVALID_PARAMETER);
    }

    DEBUGMSG(ZONE_ENTRY,(L"NKTlsCall exit: %8.8lx\r\n", retval));
    
    return retval;

}

/* Set thread last error */



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void
NKSetLastError(
    DWORD dwError
    )
{
    DEBUGMSG(ZONE_ENTRY,(L"NKSetLastError entry: %8.8lx\r\n",dwError));
    pCurThread->dwLastError = dwError;
    DEBUGMSG(ZONE_ENTRY,(L"NKSetLastError exit\r\n"));
}

/* Get thread last error */



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD
NKGetLastError(void)
{
    DEBUGMSG(ZONE_ENTRY,(L"NKGetLastError entry\r\n"));
    DEBUGMSG(ZONE_ENTRY,(L"NKGetLastError exit: %8.8lx\r\n",pCurThread->dwLastError));
    return pCurThread->dwLastError;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD NKGetCallerVMProcessId (void)
{
    return pCurThread->pprcVM
        ? pVMProc->dwId
        : g_pprcNK->dwId;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD NKGetDirectCallerProcessId (void)
{
    return IsInDirectCall ()? g_pprcNK->dwId : NKGetCallerVMProcessId ();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static DWORD DoEXTGetCallerProcessId (BOOL fOldBehavior)
{
    PTHREAD    pth   = pCurThread;
    PCALLSTACK pcstk = pth->pcstkTop;

    PREFAST_DEBUGCHK (pcstk);
    pcstk = pcstk->pcstkNext;   // skip the 1st call stack, which has the information
                                // about the caller of "GetCallerProcessId"

    if (pcstk) {
        // old behavior: as long as we crossed PSL boundary, we shouldn't return NULL.
        fOldBehavior = FALSE;
        
        // skip kernel process
        while (pcstk && (pcstk->pprcLast == g_pprcNK)) {
            pcstk = pcstk->pcstkNext;
        }
    }
    return pcstk
        ? pcstk->pprcLast->dwId
        : (fOldBehavior? 0 : pth->pprcOwner->dwId);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD EXTGetCallerProcessId (void)
{
    return DoEXTGetCallerProcessId (FALSE);
}

DWORD EXTOldGetCallerProcess (void)
{
    return DoEXTGetCallerProcessId (TRUE);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD NKGetOwnerProcessId (void)
{
    return pCurThread->pprcOwner->dwId;
}

// THGrow is separated into its own file, so the coredll side can have the same implementation
#include "thgrow.c"

typedef struct {
    PTHSNAP pSnap;
    DWORD   dwProcId;
} ENUM_TH32, *PENUM_TH32;


extern LPDWORD pKrnMainHeap;
extern LPDWORD pUsrMainHeap;

//------------------------------------------------------------------------------
// function to fill PPROCESSENTRY32 with process pprc
static void FillProcInfo (PPROCESS pprc, PPROCESSENTRY32 pprc32)
{
    // we don't have to do this unless snapshot ever shrink
    // because VirtualAlloc guarantees that the every page is
    // zero-initialized.
    //memset (pprc32, 0, sizeof (PROCESSENTRY32));
    
    pprc32->dwSize = sizeof(PROCESSENTRY32);
    pprc32->cntUsage = 1;
    pprc32->th32ProcessID = pprc->dwId;
    pprc32->cntThreads = pprc->wThrdCnt;
    pprc32->pcPriClassBase = THREAD_PRIORITY_NORMAL;
    if (pprc == g_pprcNK) {
        pprc32->th32DefaultHeapID = *pKrnMainHeap;
    } else {
        PROCReadMemory (pprc, pUsrMainHeap, &pprc32->th32DefaultHeapID, sizeof (DWORD));
    }
    if (pprc->ZonePtr) {
        PROCReadMemory (pprc, &pprc->ZonePtr->ulZoneMask, &pprc32->dwFlags, sizeof (DWORD));
    }
    pprc32->th32MemoryBase = (DWORD) pprc->BasePtr;
    NKwcsncpy (pprc32->szExeFile, pprc->lpszProcName, MAX_PATH);
}

//------------------------------------------------------------------------------
// toolhelp enumeration function for process information
//
static BOOL THProcEnumFunc (PDLIST pItem, LPVOID pEnumData)
{
    PENUM_TH32      penum  = (PENUM_TH32) pEnumData;
    PTHSNAP         pSnap  = penum->pSnap;
    PPROCESSENTRY32 pprc32 = (PPROCESSENTRY32) THGrow (pSnap, sizeof(PROCESSENTRY32));

    if (pprc32) {
       // fill the TH32PROC
        FillProcInfo ((PPROCESS) pItem, pprc32);
        pSnap->dwProcCnt ++;
    }

    return !pprc32;  // return TRUE to stop, FALSE to continue
}

DWORD DoGetFullPath (const openexe_t *oeptr, LPWSTR pBuf, DWORD cchMax);

//------------------------------------------------------------------------------
// toolhelp enumeration function for module information
//
static BOOL THModEnumFunc (PDLIST pItem, LPVOID pEnumData)
{
    PENUM_TH32 penum = (PENUM_TH32) pEnumData;
    PTHSNAP    pSnap = penum->pSnap;
    MODULEENTRY32 *pmod32 = (MODULEENTRY32 *) THGrow (pSnap,sizeof(MODULEENTRY32));

    if (pmod32) {
        // fill the TH32MOD
        PMODULELIST pModEntry = (PMODULELIST) pItem;
        PMODULE pMod = pModEntry->pMod;

        // we don't have to do this unless snapshot ever shrink
        // because VirtualAlloc guarantees that the every page is
        // zero-initialized.
        //memset (pmod32, 0, sizeof (MODULEENTRY32));

        pmod32->dwSize = sizeof (MODULEENTRY32);
        pmod32->th32ModuleID = (DWORD) pMod;
        pmod32->th32ProcessID = penum->dwProcId;

        pmod32->GlblcntUsage = pMod->wRefCnt;
        pmod32->ProccntUsage = pModEntry->wRefCnt;
        pmod32->modBaseAddr  = pMod->BasePtr;
        pmod32->modBaseSize  = pMod->e32.e32_vsize;
        pmod32->hModule      = (HMODULE) pMod;
        pmod32->dwFlags      = pMod->dwZone;

        NKwcsncpy (pmod32->szModule, pMod->lpszModName, MAX_PATH);
        DoGetFullPath (&pMod->oe, pmod32->szExePath, MAX_PATH);

        // insert the new item at the end of the list
        pSnap->dwModCnt ++;
    }

    return !pmod32;  // return TRUE to stop, FALSE to continue
}

//------------------------------------------------------------------------------
// toolhelp enumeration function for thread information
//
static BOOL THThreadEnumFunc (PDLIST pItem, LPVOID pEnumData)
{
    PENUM_TH32 penum = (PENUM_TH32) pEnumData;
    PTHSNAP    pSnap = penum->pSnap;
    THREADENTRY32 *pth32 = (THREADENTRY32 *) THGrow (pSnap, sizeof(THREADENTRY32));

    if (pth32) {
        PTHREAD pTh = (PTHREAD) pItem;

        // we don't have to do this unless snapshot ever shrink
        // because VirtualAlloc guarantees that the every page is
        // zero-initialized.
        //memset (pth32, 0, sizeof (THREADENTRY32));
        
        pth32->dwSize = sizeof(THREADENTRY32);
        pth32->cntUsage = 1;
        pth32->th32ThreadID = pTh->dwId;
        pth32->th32OwnerProcessID = penum->dwProcId;
        pth32->tpBasePri = GET_BPRIO(pTh);
        pth32->tpDeltaPri = GET_BPRIO(pTh) - GET_CPRIO(pTh);
        pth32->dwFlags = GET_SLEEPING(pTh) ? 4 + (pTh->lpProxy != 0) + (pTh->bSuspendCnt != 0) * 2 : GET_RUNSTATE(pTh);
        pth32->th32CurrentProcessID = pTh->pprcActv->dwId;

        pSnap->dwThrdCnt ++;
    }
    return !pth32;
}


//------------------------------------------------------------------------------
// THGetProcs - get the process information for all process in the system
//------------------------------------------------------------------------------
static DWORD THGetProcs (
    PTHSNAP pSnap
    )
{
    DWORD dwErr;
    ENUM_TH32 th32 = { pSnap, 0 };

    // add NK, no need to lock NK for it never gets unloaded
    PPROCESSENTRY32 pprc32 = (PPROCESSENTRY32) THGrow (pSnap, sizeof(PROCESSENTRY32));
    if (!pprc32) {
        dwErr = ERROR_OUTOFMEMORY;
    } else {

        // NK is always the 1st process
        FillProcInfo (g_pprcNK, pprc32);
        pSnap->dwProcCnt ++;

        // now do the other process (NK is not on the process list)
        LockModuleList ();
        
        dwErr = EnumerateDList (&g_pprcNK->prclist, THProcEnumFunc, &th32)
                ? ERROR_OUTOFMEMORY
                : 0;
        
        UnlockModuleList ();
    }

    return dwErr;
}


//------------------------------------------------------------------------------
// THGetMods - get module list of a process
//------------------------------------------------------------------------------
static DWORD THGetMods (
    PTHSNAP  pSnap,
    PPROCESS pprc
    )
{
    DWORD dwErr;
    ENUM_TH32 th32 = { pSnap, pprc->dwId };
    
    LockModuleList ();

    dwErr = EnumerateDList (&pprc->modList, THModEnumFunc, &th32)
            ? ERROR_OUTOFMEMORY
            : 0;
    
    UnlockModuleList ();

    return dwErr;
}

//------------------------------------------------------------------------------
// THGetThreads - get all threads of a process
// NOTE: we can only do it one process at a time. The reason: we cannot hold the 
//       Loader lock of NK and try to hold the loader lock of a process. It will 
//       violate CS ordering.
//------------------------------------------------------------------------------
DWORD THGetThreads(
    PTHSNAP  pSnap,
    PPROCESS pprc
    ) 
{
    DWORD     dwErr;
    ENUM_TH32 th32 = { pSnap, pprc->dwId };

    LockLoader (pprc);

    dwErr = EnumerateDList (&pprc->thrdList, THThreadEnumFunc, &th32)
            ? ERROR_OUTOFMEMORY
            : 0;

    UnlockLoader (pprc);

    return dwErr;
}

#define TOOLHEL_TIMEOUT 60000       // 1 minute
#define MAX_HEAP_COUNT  30000       // unlikely to exceed this # of heaps per process

DWORD THGetHeapFromOtherProcess (PTHSNAP pSnap, PPROCESS pprc)
{
    PTHSNAP  pUsrSnap = PROCVMAlloc (pprc, NULL, s_THSnapReserve, MEM_RESERVE, PAGE_NOACCESS);
    DWORD    dwErr = ERROR_INVALID_PARAMETER;

    if (pUsrSnap && VMAlloc (pprc, pUsrSnap, VM_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
    
        PPROCESS pprcActv = SwitchActiveProcess (pprc);
        PPROCESS pprcVM   = SwitchVM (pprc);
        HANDLE   hTh      = NULL;
        DWORD    dwId;
        DWORD    cbOldInuse = pSnap->cbInuse;

        DEBUGCHK (IsKModeAddr ((DWORD) pSnap));

        __try {

            pUsrSnap->cbCommit   = VM_PAGE_SIZE;
            pUsrSnap->cbReserved = s_THSnapReserve;
            pUsrSnap->cbSize     = sizeof (THSNAP);
            pUsrSnap->cbInuse    = sizeof (THSNAP);
            pUsrSnap->dwFlags    = pSnap->dwFlags;

            if ((hTh = NKCreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)pfnUsrGetHeapSnapshot, pUsrSnap, 0, &dwId))
                && (WAIT_OBJECT_0 == NKWaitForSingleObject (hTh, TOOLHEL_TIMEOUT))) {
                // don't trust anything in the pUsrSnap. It's filled by user

                // make a local copy of sizes
                DWORD cbUsrSnap = pUsrSnap->cbInuse;
                DWORD cbCommit  = pUsrSnap->cbCommit;
                DWORD dwHeapListCnt = pUsrSnap->dwHeapListCnt;

                // we only do minimal validation to guarantee memory sizes are valid. 
                // toolhelp API will verify if the heap snapshots are valid.
                if (   (cbUsrSnap >= sizeof (THSNAP))
                    && (cbUsrSnap <= cbCommit)
                    && (cbCommit  <= s_THSnapReserve)
                    && (cbUsrSnap + pSnap->cbInuse - sizeof(THSNAP) <= pSnap->cbReserved)
                    && (dwHeapListCnt < MAX_HEAP_COUNT)) {

                    // discard snapshot header
                    cbUsrSnap -= sizeof (THSNAP);
                    if (!THGrow (pSnap, cbUsrSnap)) {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    } else {
                        memcpy ((LPBYTE)pSnap+cbOldInuse, (pUsrSnap+1), cbUsrSnap);
                        pSnap->dwHeapListCnt += dwHeapListCnt;
                        dwErr = 0;
                    }
                }
            } else {
                DEBUGMSG (1, (L"Unable to create heap snapshot of process id=%8.8lx", pprc->dwId));
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DEBUGMSG (1, (L"Exception while getting heap snapshot for process id=%8.8lx\r\n", pprc->dwId));
            // restore inuse 
            pSnap->cbInuse = cbOldInuse;
        }

        if (hTh) {
            HNDLCloseHandle (pprc, hTh);
        }

        SwitchVM (pprcVM);
        SwitchActiveProcess (pprcActv);
    }

    if (pUsrSnap) {
        VERIFY (PROCVMFree (pprc, pUsrSnap, 0, MEM_RELEASE));
    }

    return dwErr;
}


//
// Get Heap snapshot.
// NOTE: we never get heap snapshot of current process, for it can be done in-proc. It'll
//       be done in toolhelp API.
//
DWORD THGetHeaps (PTHSNAP pSnap, PPROCESS pprc)
{
    DWORD dwErr = 0;
    if (pprc == g_pprcNK) {
        pprc = SwitchActiveProcess (g_pprcNK);
        // getting kernel heap informaion
        dwErr = (*pfnKrnGetHeapSnapshot) (pSnap)? 0 : ERROR_NOT_ENOUGH_MEMORY;
        SwitchActiveProcess (pprc);
    } else if ((PROCESS_STATE_NORMAL == pprc->bState) && (pprc != pActvProc)) {
        // getting heap snapshot of other processes, create a thread to get the information
        dwErr = THGetHeapFromOtherProcess (pSnap, pprc);
    }

    return dwErr;
}


//------------------------------------------------------------------------------
// Worker function to create toolhelp snapshot
//------------------------------------------------------------------------------
DWORD DoCreateSnapshot (PTHSNAP pSnap, PPROCESS pprc, DWORD dwFlags)
{
    DWORD dwErr = 0;
    PPROCESSENTRY32 pprc32;
    PPROCESS  pprcTrav;
    PHDATA    phd;
    DWORD     dwIdx;

    // we need to create process snapshot if thread is requested. We'll need to 
    // enumerate the processes we snapped for the threads
    if (dwFlags & (TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD)) {
        dwErr = THGetProcs (pSnap);
    }
    if (!dwErr && (dwFlags & TH32CS_SNAPMODULE)) {
        dwErr = THGetMods (pSnap, (dwFlags & TH32CS_GETALLMODS)? g_pprcNK : pprc);
    }
    if (!dwErr && (dwFlags & TH32CS_SNAPTHREAD)) {

        // enumerate the process snapshot we just got, and get the threads 
        // into the snapshot
        pprc32 = TH32FIRSTPROC (pSnap);
        for (dwIdx = 0; !dwErr && (dwIdx < pSnap->dwProcCnt); dwIdx ++, pprc32 ++) {

            phd = LockHandleData ((HANDLE) pprc32->th32ProcessID, g_pprcNK);

            if (pprcTrav = GetProcPtr (phd)) {
                dwErr = THGetThreads (pSnap, pprcTrav);
            }
            UnlockHandleData (phd);
        }
    }

    // get heap-list information if requested. 
    if (!dwErr && (TH32CS_SNAPHEAPLIST & dwFlags) && !(TH32CS_SNAPPROCESS & dwFlags)) {
        // get all heaps of a process
        dwErr = THGetHeaps (pSnap, pprc);
        
    } else if ((TH32CS_SNAPPROCESS & dwFlags) && !(TH32CS_SNAPNOHEAPS & dwFlags)) {
        // enumerate the process snapshot we just got, and snap main process heap information
        pprc32 = TH32FIRSTPROC (pSnap);
        for (dwIdx = 0; !dwErr && (dwIdx < pSnap->dwProcCnt); dwIdx ++, pprc32 ++) {

            phd = LockHandleData ((HANDLE) pprc32->th32ProcessID, g_pprcNK);

            if (pprcTrav = GetProcPtr (phd)) {
                dwErr = THGetHeaps (pSnap, pprcTrav);
            }
            UnlockHandleData (phd);
        }
    }

    return dwErr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PTHSNAP
NKTHCreateSnapshot(
    DWORD dwFlags,
    DWORD dwProcID
    )
{

    PTHSNAP pSnap = NULL;
    PHDATA phd = LockHandleData ((HANDLE) dwProcID, g_pprcNK);
    PPROCESS pprc = GetProcPtr (phd);
    DWORD dwErr = 0;
    
    if (!phd) {
        // see if the passed in dwProcID is actually a handle from OpenProcess call (for BC)
        phd = LockHandleData ((HANDLE) dwProcID, pActvProc);   
        pprc = GetProcPtr (phd);
    }
    
    if (!dwProcID) {
        pprc = pActvProc;
    }

    DEBUGMSG (ZONE_LOADER2, (L"NKTHCreateSnapshot: Creating snapshot, dwProcId = %8.8lx, dwFlags = %8.8lx\r\n", dwProcID, dwFlags));

    // use kernel memory while creating snapshop, VirtualCopy to user address space later.
    if (pprc
        && (pSnap = VMReserve (g_pprcNK, s_THSnapReserve, 0, 0))) {
        // commit the 1st page
        if (VMAlloc (g_pprcNK, pSnap, VM_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
            // initialize the SnapShot
            pSnap->cbSize     = sizeof (THSNAP);
            pSnap->cbInuse    = sizeof (THSNAP);
            pSnap->cbCommit   = VM_PAGE_SIZE;
            pSnap->cbReserved = s_THSnapReserve;
            pSnap->dwFlags    = dwFlags;

            // all other fields are zero. VMAlloc guarantees that the newly committed
            // page is zero'd
            dwErr = DoCreateSnapshot (pSnap, pprc, dwFlags);
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }
    } else {
        dwErr = pprc? ERROR_INVALID_PARAMETER : ERROR_OUTOFMEMORY;
    }

    UnlockHandleData (phd);

    DEBUGMSG (dwErr, (L"NKTHCreateSnapshot: dwErr = %8.8lx\r\n", dwErr));
    pprc = pActvProc;

    if (!dwErr) {
        if (pprc != g_pprcNK) {
            // virtual copy to user VM
            PTHSNAP pUsrSnap = VMReserve (pprc, s_THSnapReserve, 0, 0);

            // Don't need to flush - the VMFreeAndRelease call below will write-back and discard
            // OEMCacheRangeFlush (pSnap, pSnap->cbCommit, CACHE_SYNC_WRITEBACK);

            if (pUsrSnap
                && VMFastCopy (pprc, (DWORD) pUsrSnap, g_pprcNK, (DWORD) pSnap, pSnap->cbCommit, PAGE_READWRITE)) {
                // succeed, release the kernel version, and update return value
                VMFreeAndRelease (g_pprcNK, pSnap, s_THSnapReserve);
                pSnap = pUsrSnap;

            } else {
                dwErr = ERROR_OUTOFMEMORY;
                if (pUsrSnap) {
                    VMFreeAndRelease (pprc, pUsrSnap, s_THSnapReserve);
                }
            }
        }

        if (!dwErr) {
            // add to "external allocated" VM list such that the caller can free it.
            VMAddAllocToList (pprc, pSnap, s_THSnapReserve, NULL);
        }
    }

    // DO NOT derefernce pSnap from here on. pSnap can be pointing to user space, or NULL
    if (dwErr) {
        if (pSnap) {
            VMFreeAndRelease (g_pprcNK, pSnap, s_THSnapReserve);
        }
        pSnap = NULL;
        KSetLastError (pCurThread, dwErr);
    }
    DEBUGMSG (ZONE_LOADER2, (L"NKTHCreateSnapshot: returns = %8.8lx\r\n", pSnap));
    return pSnap;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKSetStdioPathW(
    DWORD id,
    LPCWSTR pwszPath
    )
{
    PNAME pName = NULL, pName2;

    if ((id >= 3) || (pwszPath && !(pName = DupStrToPNAME (pwszPath)))) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
        
    }

    EnterCriticalSection(&NameCS);
    pName2 = pActvProc->pStdNames[id];
    pActvProc->pStdNames[id] = pName;
    LeaveCriticalSection(&NameCS);
    if (pName2) {
        FreeName(pName2);
    }
    
    return TRUE;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKGetStdioPathW(
    DWORD id,
    PWSTR pwszBuf,
    LPDWORD lpdwLen
    )
{
    DWORD len, inlen;
    DWORD dwErr = 0;
    
    if ((id >= 3)
        || !CeSafeCopyMemory (&inlen, lpdwLen, sizeof (DWORD))
        || !IsValidUsrPtr (pwszBuf, inlen, TRUE)) {
        dwErr = ERROR_INVALID_PARAMETER;
        
    } else {
        PNAME pName;
        EnterCriticalSection(&NameCS);
        
        pName = pActvProc->pStdNames[id];
        len = pActvProc->pStdNames[id]? (NKwcslen (pName->name)+1) : 0;
        
        if (inlen < len) {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        } else if (!CeSafeCopyMemory (pwszBuf, pName->name, sizeof(WCHAR)*len)
            || !CeSafeCopyMemory (lpdwLen, &len, sizeof (DWORD))) {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        LeaveCriticalSection(&NameCS);
    }

    if (dwErr) {
        SetLastError (dwErr);
    }

    return !dwErr;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKQueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount
    )
{
    if (g_pOemGlobal->pfnQueryPerfCounter)
        return g_pOemGlobal->pfnQueryPerfCounter(lpPerformanceCount);

    // get the perf counter from tick count
    do {
        // loop in case we get interrupted in the middle of this update
        lpPerformanceCount->HighPart = g_PerfCounterHigh;
        lpPerformanceCount->LowPart = CurMSec;
    }while(lpPerformanceCount->HighPart != g_PerfCounterHigh);

    // add the incremental time passed from the last computation of the low part in the do loop
    lpPerformanceCount->QuadPart += GETCURRTICK() - lpPerformanceCount->LowPart;
    
    return TRUE;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKQueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency
    )
{
    if (g_pOemGlobal->pfnQueryPerfFreq)
        return g_pOemGlobal->pfnQueryPerfFreq (lpFrequency);
    lpFrequency->HighPart = 0;
    lpFrequency->LowPart = 1000;
    return TRUE;
}

DWORD (*pReadRegistryFromOEM)(DWORD dwFlags, LPBYTE pBuf, DWORD len);
BOOL (*pWriteRegistryToOEM)(DWORD dwFlags, LPBYTE pBuf, DWORD len);



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD
NKReadRegistryFromOEM(
    DWORD dwFlags,
    LPBYTE pBuf,
    DWORD len
    )
{
    TRUSTED_API (L"NKReadRegistryFromOEM", 0);
    return g_pOemGlobal->pfnReadRegistry? g_pOemGlobal->pfnReadRegistry (dwFlags, pBuf, len) : 0;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
NKWriteRegistryToOEM(
    DWORD dwFlags,
    LPBYTE pBuf,
    DWORD len
    )
{
    TRUSTED_API (L"NKWriteRegistryToOEM", FALSE);

    return g_pOemGlobal->pfnWriteRegistry
            ? ((dwFlags == REG_WRITE_BYTES_PROBE)? TRUE : g_pOemGlobal->pfnWriteRegistry (dwFlags, pBuf, len))
            : FALSE;
    
}

void NKCacheRangeFlush (LPVOID pAddr, DWORD dwLength, DWORD dwFlags)
{
#ifdef x86
    // flushing TLB is a ring 0 operation on x86
    if (dwFlags & CACHE_SYNC_FLUSH_TLB)
        KCall ((PKFN) OEMCacheRangeFlush, pAddr, dwLength, dwFlags);
    else
#endif
        OEMCacheRangeFlush (pAddr, dwLength, dwFlags);
}

static DWORD g_dwNLSSize;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LPBYTE
NKCreateLocaleView(
    LPDWORD pdwViewSize
    )
{
    HANDLE hNLS, hNLSMap;
    LPBYTE lpNLSView;

    DEBUGCHK (pActvProc == g_pprcNK);

    // create NLS file
    hNLS = FSCreateFile (TEXT("\\windows\\wince.nls"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    DEBUGCHK (hNLS != INVALID_HANDLE_VALUE);

    // get NLS file size
    g_dwNLSSize = FSSetFilePointer (hNLS, 0, 0, FILE_END);

    // create NLS mapping
    hNLSMap = NKCreateFileMapping (pActvProc, hNLS, 0, PAGE_READONLY, 0, 0, TEXT("NLSFILE"));
    DEBUGCHK(hNLSMap);

    // map NLS view
    lpNLSView = MAPMapView (pActvProc, hNLSMap, FILE_MAP_READ, 0, 0, 0);
    DEBUGCHK(lpNLSView);

    *pdwViewSize = g_dwNLSSize;

    return lpNLSView;
}

LPBYTE
EXTCreateLocaleView(
    LPDWORD pdwViewSize
    )
{
    HANDLE hNLSMap;
    LPBYTE lpNLSView;

    hNLSMap = NKCreateFileMapping (pActvProc, INVALID_HANDLE_VALUE, 0, PAGE_READONLY, 0, 0,
                                    TEXT("NLSFILE"));
    DEBUGCHK(hNLSMap);
    lpNLSView = MAPMapView (pActvProc, hNLSMap, FILE_MAP_READ, 0, 0, 0);
    DEBUGCHK(lpNLSView);

    CeSafeCopyMemory (pdwViewSize, &g_dwNLSSize, sizeof (DWORD));

    return lpNLSView;
}

//------------------------------------------------------------
//
// utility functions
//
//------------------------------------------------------------


// EnumerateDList - enumerate DList, call pfnEnum on every item. Stop when pfnEnum returns TRUE, 
// or the list is exhausted. Return the item when pfnEnum returns TRUE, or NULL
// if list is exhausted.
PDLIST EnumerateDList (PDLIST phead, PFN_DLISTENUM pfnEnum, LPVOID pEnumData)
{
    PDLIST ptrav;
    for (ptrav = phead->pFwd; ptrav != phead; ptrav = ptrav->pFwd) {
        if (pfnEnum (ptrav, pEnumData)) {
            return ptrav;
        }
    }
    return NULL;
}

//
// DestroyDList - Remove all items in a DLIST, call pfnEnum on every item destroyed, 
//                return value of pfnEnum is ignored.
//
void DestroyDList (PDLIST phead, PFN_DLISTENUM pfnEnum, LPVOID pEnumData)
{
    PDLIST ptrav;
    while ((ptrav = phead->pFwd) != phead) {
        RemoveDList (ptrav);
        pfnEnum (ptrav, pEnumData);
    }
}

//
// MergeDList - merge two DList, the source DList will be emptied
// 
void MergeDList (PDLIST pdlDst, PDLIST pdlSrc)
{
    if (!IsDListEmpty (pdlSrc)) {
        PDLIST pFwd  = pdlSrc->pFwd;    // 1st item in pdlSrc
        PDLIST pBack = pdlSrc->pBack;   // last item in pdlSrc

        pFwd->pBack = pdlDst->pBack;    // back ptr of 1st element set to the last element of dst DList
        pBack->pFwd = pdlDst;           // fwd ptr of last element set to the head of dst DList
        pdlDst->pBack->pFwd = pFwd;     // fwd ptr of the last element of dst DList set to 1st element of src DList
        pdlDst->pBack = pBack;          // back ptr of the head of dst DList set to last element of src DList

        InitDList (pdlSrc);             // empy the source DList
    }
}


WCHAR NKtowlower(WCHAR ch) {
    return ((ch >= 'A') && (ch <= 'Z')) ? (ch - 'A' + 'a') : ch;
}

int NKstrcmpiAandW(LPCSTR lpa, LPCWSTR lpu) {
    while (*lpa && (NKtowlower((WCHAR)*lpa) == NKtowlower(*lpu))) {
        lpa++;
        lpu++;
    }
    return ((*lpa || *lpu) ? 1 : 0);
}

int NKwcsnicmp(LPCWSTR str1, LPCWSTR str2, int count) {
    wchar_t f,l;
    if (!count)
        return 0;
    do {
        f = NKtowlower(*str1++);
        l = NKtowlower(*str2++);
    } while (--count && f && (f == l));
    return (int)(f - l);
}

int NKwcsicmp(LPCWSTR str1, LPCWSTR str2) {
    wchar_t f,l;
    do  {
        f = NKtowlower(*str1++);
        l = NKtowlower(*str2++);
    } while (f && (f == l));
    return (int)(f - l);
}

void NKwcscpy(LPWSTR p1, LPCWSTR p2)
{
    while (*p1++ = *p2++)
        ;
}

DWORD NKwcsncpy (LPWSTR pDst, LPCWSTR pSrc, DWORD cchLen)
{
    DWORD cchRet = 0;

    DEBUGCHK ((int) cchLen > 0);

    if ((int) cchLen > 0) {
        for ( ; (cchRet + 1 < cchLen) && pSrc[cchRet]; cchRet ++) {
            pDst[cchRet] = pSrc[cchRet];
        }
        pDst[cchRet] = 0;   // EOS
    }
    return cchRet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD NKwcslen (const wchar_t *wcs )
{
    const wchar_t *eos = wcs;

    while (*eos)
        ++eos;

    return eos-wcs;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int NKwcscmp (const wchar_t *pwc1, const wchar_t *pwc2)
{
    int ret = 0;

    while ( !(ret = *pwc1 - *pwc2) && *pwc2)
        ++pwc1, ++pwc2;
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void NKAsciiToUnicode (LPWSTR wchptr, LPCSTR chptr, int maxlen)
{
    while ((maxlen-- > 1) && *chptr) {
        *wchptr++ = (WCHAR)*chptr++;
    }
    *wchptr = 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void NKUnicodeToAscii(LPCHAR chptr, LPCWSTR wchptr, int maxlen) {
    while ((maxlen-- > 1) && *wchptr) {
        *chptr++ = (CHAR)*wchptr++;
    }
    *chptr = 0;
}

PNAME DupStrToPNAME (LPCWSTR pszName)
{
    PNAME pDupName = NULL;
    if (pszName) {
        DWORD len;
        DWORD ccb;

        __try {
            len = NKwcslen (pszName);
            if (len < MAX_PATH) {
                ccb = (len+1) * sizeof(WCHAR);
                if (pDupName = AllocName (ccb)) {
                    memcpy (pDupName->name, pszName, ccb);
                    pDupName->name[len] = 0;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // exception, free memory we allocated
            if (pDupName) {
                FreeName (pDupName);
                pDupName = NULL;
            }
        }
    }
    return pDupName;
}