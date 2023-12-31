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

    kdbreak.c

Abstract:

    This module implements machine dependent functions to add and delete
    breakpoints from the kernel debugger breakpoint table.

Revision History:

--*/

#include "kdp.h"
#include "dbg_common.h"

// The following variables are global for a reason.  Do not move them to the stack or bad things happen
// when flushing instructions.
KDP_BREAKPOINT_TYPE Content;
KDP_BREAKPOINT_TYPE ReadBack;

static BREAKPOINT_ENTRY g_aBreakpointTable [BREAKPOINT_TABLE_SIZE];

// Map a breakpoint to a VM.
struct BreakpointKva
{
    PPROCESS pVM;
    void *pvBreakpointAddr;
} g_aBreakpointKva[BREAKPOINT_TABLE_SIZE];

// This int holds the total number of distinct SW Code breakpoints that are set or need to be set
int g_nTotalNumDistinctSwCodeBps;

BOOL g_fForceTestRomBp = FALSE; // Warning: test purpose only


// Workaround for earlier builds of Platform Builder. Empty VM-VA map.
void KdpFlushBreakpointKva(void)
{
    DWORD i;

    DEBUGGERMSG(KDZONE_SWBP, (L"++KdpFlushBreakpointKva\r\n"));
    for (i = 0; i < BREAKPOINT_TABLE_SIZE; i++)
    {
        g_aBreakpointKva[i].pvBreakpointAddr = NULL;
        g_aBreakpointKva[i].pVM = NULL;
    }
    DEBUGGERMSG(KDZONE_SWBP, (L"--KdpFlushBreakpointKva\r\n"));
}


// Workaround for earlier builds of Platform Builder. Recall a VM based on a VA.
BOOL KdpFindBreakpointKva(void *pvBpAddr, PROCESS **ppVM)
{
    BOOL fFound = FALSE;
    DWORD i;

    DEBUGGERMSG(KDZONE_SWBP, (L"++KdpFindBreakpointKva (0x%08x)\r\n", pvBpAddr));

    *ppVM = NULL;
    i = 0;
    while (!fFound && i < BREAKPOINT_TABLE_SIZE)
    {
        if (g_aBreakpointKva[i].pvBreakpointAddr == pvBpAddr && g_aBreakpointKva[i].pVM)
        {
            DEBUGGERMSG(KDZONE_SWBP, (L"  KdpFindBreakpointKva: Found 0x%08x mapped in process 0x%08x, index = %d\r\n", g_aBreakpointKva[i].pvBreakpointAddr,
                g_aBreakpointKva[i].pVM, i));
            *ppVM = g_aBreakpointKva[i].pVM;
            fFound = TRUE;
        }
        else
        {
            i++;
        }
    }
    DEBUGGERMSG(KDZONE_SWBP, (L"--KdpFindBreakpointKva  BOOL %d\r\n", fFound));
    return fFound;
}


// Workaround for earlier builds of platform builder.  Remember a VA + VM pair.
BOOL KdpSaveBreakpointKva(void *pvBpAddr, PPROCESS pVM)
{
    BOOL fFound = FALSE;
    DWORD i;

    i = 0;
    while (!fFound && i < BREAKPOINT_TABLE_SIZE)
    {
        if (g_aBreakpointKva[i].pvBreakpointAddr == pvBpAddr)
        {
            if (g_aBreakpointKva[i].pVM != pVM)
            {
                DEBUGGERMSG(KDZONE_ALERT, (L"  KdpSaveBreakpointKva: Address 0x%08x results in two different processes? 0x%08x vs 0x%08x\r\n",
                    pvBpAddr, g_aBreakpointKva[i].pVM, pVM));
            }
            fFound = TRUE;
        }
        else if (g_aBreakpointKva[i].pvBreakpointAddr == NULL && g_aBreakpointKva[i].pVM == NULL)
        {
            DEBUGGERMSG(KDZONE_SWBP, (L"  KdpSaveBreakpointKva: 0x%08x maps to 0x%08x, index = %d\r\n", pvBpAddr, pVM, i));
            g_aBreakpointKva[i].pvBreakpointAddr = pvBpAddr;
            g_aBreakpointKva[i].pVM = pVM;
            fFound = TRUE;
        }
        else
        {
            i++;
        }
    }
    return fFound;
}


BOOL KdpWriteBpAndVerify (void * pvAddress, void * pbData, ULONG nSize)
{
    BOOL fRet = FALSE;
    BOOL fMcRet = FALSE;
    ULONG nRbSize;
    ULONG nMmSize;

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpWriteBpAndVerify (Addr=0x%08X, size=%d)\r\n", pvAddress, nSize));

    ReadBack = 0;
    nRbSize = KdpMoveMemory ((char *) &ReadBack, (char *) pvAddress, nSize);

    DEBUGGERMSG (KDZONE_SWBP, (L"  KdpWriteBpAndVerify: %s %d byte%s at 0x%08X, Data = 0x%0*X \r\n",
                                (nRbSize == nSize) ? L"Read " : L"Failed to read ", nSize,
                                (nRbSize == 1) ? L"" : L"s", pvAddress, nSize*2, ReadBack));

    nMmSize = KdpMoveMemory ((char *) pvAddress, pbData, nSize);
    DEBUGGERMSG (KDZONE_SWBP, (L"  KdpWriteBpAndVerify: %s %d byte%s at 0x%08X, Data = 0x%0*X \r\n",
                                (nMmSize == nSize) ? L"Wrote" : L"Failed to write", nSize,
                                (nMmSize == 1) ? L"" : L"s", pvAddress, nSize*2 ,*((KDP_BREAKPOINT_TYPE *) pbData)));

    ReadBack = 0;
    nRbSize = KdpMoveMemory ((char *) &ReadBack, (char *) pvAddress, nSize);

    if (nRbSize != nSize)
    {
        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpWriteBpAndVerify: Failed to read back %d byte%s at 0x%08X (read %d)\r\n",
                                   nSize, (nMmSize == 1) ? L"" : L"s", nRbSize));
    }
    else
    {
        fMcRet = !memcmp (&ReadBack, pbData, nSize);
        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpWriteBpAndVerify: Write%s was successful", fMcRet ? L"" : L" NOT"));
        if (fMcRet)
        {
            DEBUGGERMSG (KDZONE_SWBP, (L" (read back OK)\r\n"));
            fRet = TRUE;
        }
        else
        {
            DEBUGGERMSG (KDZONE_SWBP, (L" (read back 0x%0*X instead of 0x%0*X)\r\n", nSize*2, ReadBack, nSize*2, *((KDP_BREAKPOINT_TYPE *) pbData)));
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpWriteBpAndVerify (%d)\r\n", (int) fRet));

    return fRet;
}


//------------------------------------------------------------------------------
// Get VM Page Table Entry for a VA of given ProcVM
//------------------------------------------------------------------------------
BOOL KdpGetPageTableEntryPtr (PPROCESS pVM, DWORD dwAddr, DWORD** ppPageTableEntry)
{
    BOOL fRet = FALSE;
    DWORD idxdir = VA2PDIDX (dwAddr);   // 1st-level PT index
    DWORD idx2nd = VA2PT2ND (dwAddr);   // 2nd-level PT index
    PPAGETABLE pptbl = Entry2PTBL (pVM->ppdir->pte[idxdir]);
    if (ppPageTableEntry) *ppPageTableEntry = NULL; // Init
    DEBUGGERMSG (KDZONE_SWBP, (TEXT("++KdpGetPageTableEntryPtr: va=%08X in Proc %08X(%s))\r\n"), 
                 dwAddr, pVM, pVM ? pVM->lpszProcName : L"null"));
    if (pptbl)
    {
        DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpGetPageTableEntryPtr: pptbl=%08X\r\n"), pptbl));
        if (ppPageTableEntry)
        {
            *ppPageTableEntry = &(pptbl->pte[idx2nd]);
            DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpGetPageTableEntryPtr: pPageTableEntry=%08X\r\n"), *ppPageTableEntry));
            if (IsPageCommitted(**ppPageTableEntry))
            {
                fRet = TRUE;
            }
        }
    }
    DEBUGGERMSG (KDZONE_SWBP, (TEXT("--KdpGetPageTableEntryPtr: ret=%i\r\n"), fRet));
    return fRet;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL KdpRemapVirtualMemory(
    PPROCESS pVMSrc, 
    DWORD dwAddrVirtualSrc, // Actual virtual address to be remapped
    PPROCESS pVMTgt, 
    DWORD dwAddrVirtualTgt, // Actual virtual address of the new mapping (to be duplicated)
    DWORD PageTableTgtOverride // if not 0, then override pVMTgt and dwAddrVirtualTgt (which should be 0)
    )
{
    BOOL fRet = FALSE;
    if (PageTableTgtOverride)
    {
        DEBUGGERMSG (KDZONE_SWBP, (TEXT("++KdpRemapVirtualMemory: va to remap=%08X in Proc %08X(%s), PteTgt=%08X\r\n"), 
                     dwAddrVirtualSrc, pVMSrc, pVMSrc ? pVMSrc->lpszProcName : L"null", PageTableTgtOverride));
    }
    else
    {
        DEBUGGERMSG (KDZONE_SWBP, (TEXT("++KdpRemapVirtualMemory: va to remap=%08X in Proc %08X(%s), va to be dup=%08X in Proc %08X(%s)\r\n"), 
                     dwAddrVirtualSrc, pVMSrc, pVMSrc ? pVMSrc->lpszProcName : L"null", dwAddrVirtualTgt, pVMTgt, pVMTgt ? pVMTgt->lpszProcName : L"null"));
    }

    if (!IsValidUsrPtr ((LPCVOID) dwAddrVirtualSrc, VM_PAGE_SIZE, FALSE))
    { // We don't want to kernel virtual address
        DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRemapVirtualMemory: Address is secure and/or in kernel space!\r\n")));
    }
    else
    {
        DWORD PageTableEntrySrc = 0;
        DWORD PageTableEntryTgt = PageTableTgtOverride;
        DWORD* pPageTableEntrySrc = NULL;
        DWORD* pPageTableEntryTgt = NULL;
        BOOL fTgtOK = TRUE;
        if (!PageTableTgtOverride)
        {
            if (fTgtOK = KdpGetPageTableEntryPtr (pVMTgt, dwAddrVirtualTgt, &pPageTableEntryTgt))
            {
                if (pPageTableEntryTgt) PageTableEntryTgt = *pPageTableEntryTgt;
            }
        }
        if (fTgtOK && KdpGetPageTableEntryPtr (pVMSrc, dwAddrVirtualSrc, &pPageTableEntrySrc))
        {
            if (pPageTableEntrySrc)
            {
                PageTableEntrySrc = *pPageTableEntrySrc;
                if (PageTableTgtOverride)
                {
                    *pPageTableEntrySrc = PageTableTgtOverride;
                }
                else
                {
                    // Merge destination page frame number with source page permission mask
                    *pPageTableEntrySrc = PFNfromEntry (PageTableEntryTgt) | (PageTableEntrySrc & PG_PERMISSION_MASK);
                }
                DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpRemapVirtualMemory: old SrcPTE=%08X, TgtPTE=%08X, new merged SrcPTE=%08X\r\n"), 
                             PageTableEntrySrc, PageTableEntryTgt, *pPageTableEntrySrc));

#ifdef ARM
                // call PTE barrier
                if (((OEMGLOBAL*)(g_pKData->pOem))->pfnPTEUpdateBarrier)
                {
                    ((OEMGLOBAL*)(g_pKData->pOem))->pfnPTEUpdateBarrier(pPageTableEntrySrc, sizeof (*pPageTableEntrySrc));
                }
#endif                
                // Flush the cache and TLB entries at this point
                kdpInvalidateRange ((void *) dwAddrVirtualSrc, VM_PAGE_SIZE);
                fRet = TRUE;
            }
        }
        else
        {
            DEBUGGERMSG (KDZONE_ALERT, (TEXT("  KdpRemapVirtualMemory: ERROR: cannot get PTE. va to remap=%08X in Proc %08X(%s), va to be dup=%08X in Proc %08X(%s)\r\n"), 
                         dwAddrVirtualSrc, pVMSrc, pVMSrc ? pVMSrc->lpszProcName : L"null", dwAddrVirtualTgt, pVMTgt, pVMTgt ? pVMTgt->lpszProcName : L"null"));
            fRet = FALSE;
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("--KdpRemapVirtualMemory\r\n")));
    return fRet;
}


/*++

Routine Name:

    KdpRomSwBpAttemptRamPageRemap

Routine Description:

    This routine is called by the breakpoint addition routine when a breakpoint
    add request is made in ROM address space. When this function exits, if it
    has succeeded, the address can be used transparently by other functions -
    breakpoints can be successfully added or removed at that address.

    Basic algorithm:
    This is equivalent to Copy-On-Write.
    We have a certain number of pre-allocated RAM pages. The argument ROM virtual address
    is within a ROM page. That ROM page will be copied to one of the free, pre-allocated
    RAM pages. The section table will be remapped so that the VA that used to point to the
    ROM page (that contained the argument address) now points to the RAM page. SW BP
    (trap instructions) can then be safely set at that address, and excution of
    code can carry on. If a RAM page consists of an address range that the argument
    is within, then nothing needs to be done. Various error cases are handled and
    returned to the caller, to be returned to PB. If this function fails, other functions
    need not worry since no permanent changes would have been made. Also, a count variable
    is maintained for each RAM page, and is incremented when a breakpoint is set in it,
    and decremented when one is removed from within it. When the count variable is zero,
    the ROM virtual address is remapped to its original ROM page address, and
    the RAM page is marked as free (done by KdpRestoreRomVmPageMapping).

Arguments:

    [in] pVM - VM for the address.
    [in] pvRomAddr - supplies the ROM virtual address where a breakpoint needs to be set
    [in] pvRomKAddr - supplies the ROM statically mapped kernel address equivalent where a breakpoint needs to be set
    [out] pidxRomRamTbl - return index of Rom2Ram table entry just used for remapping if succeeded

Return Value:

    Will return greater than 0 for success, or 0 or negative values for various error conditions

--*/

int KdpRomSwBpAttemptRamPageRemap (PROCESS *pVM, void * pvRomAddr, void * pvRomKAddr, int *pidxRomRamTbl, void **ppvRamAddr)
{
    DWORD dwIndex = 0;
    DWORD dwIndToBeUsed = 0;
    DWORD idxRomRamTbl = 0;
    DWORD dwTempROMAddr = 0;
    DWORD dwTempRAMAddr = 0;
    DWORD dwRomAddr = (DWORD) pvRomAddr;
    DWORD dwRamAddr = 0;
    BOOL fAddrInRange = FALSE;
    BOOL fFreePageFound = FALSE;
    int bphRet = KD_BPHND_INVALID_GEN_ERR;
    int nBytesCopied = 0;

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("++KdpRomSwBpAttemptRamPageRemap (VAddr=%08X in Proc %08X(%s))\r\n"), dwRomAddr, pVM, pVM ? pVM->lpszProcName : L"null"));

    // Try and find a RAM remap page that contains the range of addresses
    // that the given address falls into OR find a free RAM remap page for it
    for (dwIndex = 0; (dwIndex < NB_ROM2RAM_PAGES) && !fAddrInRange; dwIndex++)
    {
        dwTempROMAddr = (DWORD) (g_aRom2RamPageTable[dwIndex].pvROMAddr);
        if (dwTempROMAddr)
        { // Used entry
            if ((dwRomAddr >= dwTempROMAddr) &&
                (dwRomAddr < dwTempROMAddr + VM_PAGE_SIZE))
            { // Argument within range of one existing remaped page
                fAddrInRange = TRUE;
                dwIndToBeUsed = dwIndex;
            }
        }
        else
        { // A free page available
            if (!fFreePageFound)
            {
                fFreePageFound = TRUE;
                dwIndToBeUsed = dwIndex;
            }
        }
    }

    if (fAddrInRange)
    {
        DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRomSwBpAttemptRamPageRemap: Address found in existing RAM page (%i)\r\n"), dwIndToBeUsed));
        idxRomRamTbl = dwIndToBeUsed;
        dwRamAddr = (DWORD) (g_aRom2RamPageTable[dwIndToBeUsed].pbRAMAddr); 
        dwRamAddr += (dwRomAddr & (VM_PAGE_SIZE - 1));   
        bphRet = KD_BPHND_ROMBP_SUCCESS;
    }
    else
    {
        if (!fFreePageFound)
        {
            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRomSwBpAttemptRamPageRemap: Address not in range and no free pages!\r\n")));
            bphRet = KD_BPHND_ROMBP_ERROR_INSUFFICIENT_PAGES;
        }
        else
        { // found a free page for the ROM address (and one page worth of code)
            char* pbROMSource = (char *) (dwRomAddr & ~(VM_PAGE_SIZE - 1)); // Get page start boundary
            dwTempRAMAddr = (DWORD) (g_aRom2RamPageTable[dwIndToBeUsed].pbRAMAddr);
            if  (dwTempRAMAddr & (VM_PAGE_SIZE - 1))
            {
                DEBUGGERMSG (KDZONE_ALERT, (TEXT("  KdpRomSwBpAttemptRamPageRemap: ERROR: RAMPageAddr=0x%08X is not align on a page.\r\n"), dwTempRAMAddr));
            }
            else
            {
                DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpRomSwBpAttemptRamPageRemap: Use new page (%i), RAMPageAddr=0x%08X, ROMPageAddr=0x%08X\r\n"), dwIndToBeUsed, dwTempRAMAddr, (DWORD) pbROMSource));
                // Attempt the copy of the ROM page containing the argument to the free RAM page found
                nBytesCopied = KdpMoveMemory ((char *) (dwTempRAMAddr & ~VM_PAGE_OFST_MASK), (char *) ((DWORD) pvRomKAddr & ~VM_PAGE_OFST_MASK), VM_PAGE_SIZE);
                if (nBytesCopied != VM_PAGE_SIZE)
                {
                    DEBUGGERMSG (KDZONE_ALERT, (TEXT("  KdpRomSwBpAttemptRamPageRemap: KdpMoveMemory failed, copied %d bytes\r\n"), nBytesCopied));
                    bphRet = KD_BPHND_ERROR_COPY_FAILED;
                }
                else
                {
                    DWORD* pPageTableEntry = NULL;
                    DWORD PageTableEntry = 0;
                    DWORD dwROMAddr = (DWORD) pbROMSource;
                    PROCESS *pProcess = g_pprcNK;
                    // Remap the addresses so that the ROM page boundary virtual address will now point to
                    // the RAM address (also the start of a page boundary)
                    DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpRomSwBpAttemptRamPageRemap: KdpMoveMemory succeeded, copied %d bytes\r\n"), nBytesCopied));

                    if (pVM && (dwROMAddr < VM_DLL_BASE))
                    { // If Process specific And in process only space, then remap only in that process:

                        pProcess = pVM;                        
                    }
                    else
                    { // DLL / shared mem case
                        pVM = NULL; // Make sure we have no specific process affinity
                    }

                    // Walk process list to change all the instances of this VA accross all processes (in DLL case)
                    // Note: -There is a risk of conflic with use of "Copy on Write" for DLL code - we assume all virtual copies are same as kernel copy
                    //       -We don't need to track new load notif refcount, as long as BP are normally regenerate on all module load by host

                    do
                    {
                        if (KdpGetPageTableEntryPtr (pProcess, dwROMAddr, &pPageTableEntry))
                        { // page exist for this process
                            PageTableEntry = *pPageTableEntry; // Save current mapping
                            // Perform the remapping
                            if (!KdpRemapVirtualMemory (pProcess, dwROMAddr, g_pprcNK, dwTempRAMAddr, 0))
                            {
                                DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRomSwBpAttemptRamPageRemap: KdpRemapVirtualMemory failed for process %08X(%s)\r\n"), pProcess, pProcess->lpszProcName));
                            }
                            else
                            {
                                DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRomSwBpAttemptRamPageRemap: KdpRemapVirtualMemory succeeded for process %08X(%s)\r\n"), pProcess, pProcess->lpszProcName));
                            }
                        }
                    } while (!pVM && (pProcess = (PROCESS *) pProcess->prclist.pFwd) && (pProcess != g_pprcNK));
                    idxRomRamTbl = dwIndToBeUsed;

                    // Save all pertinent info and return success, if we got to this point
                    g_aRom2RamPageTable[dwIndToBeUsed].pvROMAddr = (void *) pbROMSource;
                    g_aRom2RamPageTable[dwIndToBeUsed].ROMAddrOrgPTEntry = PageTableEntry;
                    dwRamAddr = (DWORD) (g_aRom2RamPageTable[dwIndToBeUsed].pbRAMAddr);
                    dwRamAddr += (dwRomAddr & (VM_PAGE_SIZE - 1));   
                    bphRet = KD_BPHND_ROMBP_SUCCESS;

                    DEBUGGERMSG(KDZONE_SWBP, (TEXT("  KdpRomSwBpAttemptRamPageRemap: ROM to RAM mapping Table updated\r\n")));
                }
            }
        }
    }
    if (pidxRomRamTbl) *pidxRomRamTbl = idxRomRamTbl;
    if (ppvRamAddr) *ppvRamAddr = (void *)dwRamAddr;
    DEBUGGERMSG (KDZONE_SWBP, (TEXT("--KdpRomSwBpAttemptRamPageRemap bphRet=%i, idxRomRamTbl=%i, dwRamAddr=0x%08X\r\n"), bphRet, idxRomRamTbl, dwRamAddr));
    return bphRet;
}


/*++

Routine Name:

    KdpRestoreRomVmPageMapping

Routine Description:

    This routine will attempt to restore the old page mapping that
    existed between a copied ROM/RAM page and the original ROM
    page, before it was remapped by KdpRomSwBpAttemptRamPageRemap.

Arguments:

    [in] pVM    - Process affinity of breakpoint if any
    [in] ulROMTbaleIndex - supplies the index into the ROMRAM page table

Return Value:


--*/

BOOL KdpRestoreRomVmPageMapping (PROCESS *pVM, ULONG ulROMBPTableIndex)
{
    DWORD dwIndex = ulROMBPTableIndex;
    BOOL fRet = FALSE;
    DWORD dwFlags = 0;

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("++KdpRestoreRomVmPageMapping (index=%d in Proc %08X(%s))\r\n"), ulROMBPTableIndex, pVM, pVM ? pVM->lpszProcName : L"null"));

    // Bounds check
    if (dwIndex < NB_ROM2RAM_PAGES)
    {
        // Is the RAM page still in use?
        if (g_aRom2RamPageTable[dwIndex].nBPCount > 0)
        {
            DEBUGGERMSG(KDZONE_SWBP, (TEXT("  KdpRestoreRomVmPageMapping: Cannot free page %d, still in use\r\n"), dwIndex));
            fRet = TRUE;
        }
        else
        { // Page unused now, attempt the unmap
            DWORD dwROMAddr = (DWORD) (g_aRom2RamPageTable[dwIndex].pvROMAddr);
            PROCESS *pProcess = g_pprcNK;

            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRestoreRomVmPageMapping: Remapping page %d from %08X back to %08X\r\n"), 
                         dwIndex, (DWORD) (g_aRom2RamPageTable[dwIndex].pbRAMAddr), (DWORD) (g_aRom2RamPageTable[dwIndex].pvROMAddr)));

            // Remap the addresses so that the ROM page boundary virtual address will now point to
            // the RAM address (also the start of a page boundary)

            if (pVM && (dwROMAddr < VM_DLL_BASE))
            { // If Process specific And in process only space, then remap only in that process:

                pProcess = pVM;                        
            }
            else
            { // DLL / shared mem case
                pVM = NULL; // Make sure we have no specific process affinity
            }

            // Walk process list to change all the instances of this VA accross all processes (in DLL case)
            do
            {
                DWORD* pPageTableEntryDummy = NULL;
                if (KdpGetPageTableEntryPtr (pProcess, dwROMAddr, &pPageTableEntryDummy))
                { // page exist for this process
                    // Perform the remapping
                    if (!KdpRemapVirtualMemory (pProcess, dwROMAddr, 0, 0, g_aRom2RamPageTable[dwIndex].ROMAddrOrgPTEntry))
                    {
                        DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRestoreRomVmPageMapping: KdpRemapVirtualMemory failed for process %08X(%s)\r\n"), pProcess, pProcess->lpszProcName));
                    }
                    else
                    {
                        DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpRestoreRomVmPageMapping: KdpRemapVirtualMemory succeeded for process %08X(%s)\r\n"), pProcess, pProcess->lpszProcName));
                    }
                }

            } while (!pVM && (pProcess = (PROCESS *) pProcess->prclist.pFwd) && (pProcess != g_pprcNK));
            
            g_aRom2RamPageTable[dwIndex].pvROMAddr = 0; // Mark the page as free
            fRet = TRUE;
            DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpRestoreRomVmPageMapping: Page remapped and freed\r\n")));
        }
    }
    else
    {
        DEBUGGERMSG(KDZONE_SWBP, (TEXT("  KdpRestoreRomVmPageMapping: Error, bad index (%d)\r\n"), dwIndex));
    }
    DEBUGGERMSG (KDZONE_SWBP, (TEXT("--KdpRestoreRomVmPageMapping\r\n")));
    return fRet;
}


/*++

Routine Name:

    KdpIncCountIfRomSwBp

Routine Description:

    This routine is usually called when a breakpoint has sucessfully
    been written to the argument address. It determines whether the
    argument address is within the range of any of the used pre-allocated
    RAM pages, and if so increment the breakpoint use count variable
    for that page. 

Arguments:

    [in] pvAddr - supplies the address where a breakpoint was just written

Return Value:

    TRUE if BP was in ROM.

--*/

BOOL KdpIncCountIfRomSwBp (void * pvAddr)
{
    DWORD dwIndex = 0;
    DWORD dwROMAddrTemp = 0;
    DWORD dwAddr = (DWORD) pvAddr;
    BOOL fRet = FALSE;

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("++KdpIncCountIfRomSwBp (0x%08X)\r\n"), dwAddr));

    for (dwIndex = 0; (dwIndex < NB_ROM2RAM_PAGES) && !fRet; dwIndex++)
    {
        dwROMAddrTemp = (DWORD) (g_aRom2RamPageTable[dwIndex].pvROMAddr);
        
        // Check if the address of the breakpoint just added
        // falls in any of our preallocated/remapped ROM/RAM pages
        if ((dwAddr >= dwROMAddrTemp) && (dwAddr < dwROMAddrTemp + VM_PAGE_SIZE))
        {
            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpIncCountIfRomSwBp: Address found in page %d between (0x%08X) and (0x%08X)\r\n"), dwIndex, dwROMAddrTemp, dwROMAddrTemp+VM_PAGE_SIZE));
            fRet = TRUE;

            // Increment the breakpoint use count for that page
            g_aRom2RamPageTable[dwIndex].nBPCount++;

            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpIncCountIfRomSwBp: Page %d Count incremented to %d\r\n"), dwIndex, g_aRom2RamPageTable[dwIndex].nBPCount));
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("--KdpIncCountIfRomSwBp (found=%d)\r\n"),fRet));
    return fRet;
}

/*++

Routine Description:

    This routine intanciate (replace original instruction with software trap)
    on a given breakpoint table entry.

Arguments:

    [in] BpHandle - Supplies the BpHandle to instanciate

Return Value:

    Same value as BpHandle if succeeded
    0 or negative value if failed

--*/

int KdpInstantiateSwBreakpoint (int BpHandle)
{
    int bphRet = BpHandle;
    DWORD Index = BpHandle - 1;
    BOOL fAccessible = FALSE;
    void * Address = NULL;
    ULONG ulBpInstrLen = sizeof (KDP_BREAKPOINT_TYPE);
    KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
    PROCESS* pVM = g_aBreakpointTable[Index].pVM;

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpInstantiateSwBreakpoint (bph=%d)\r\n", BpHandle));

    if ((Index < BREAKPOINT_TABLE_SIZE) &&
        g_aBreakpointTable [Index].wRefCount &&
        (g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_SUSPENDED))
    { // Check that entry is good
        BYTE *pbBpKAddr;

        Address = g_aBreakpointTable [Index].Address;
        g_aBreakpointTable[Index].KAddr = NULL;

        DEBUGGERMSG(KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Processing interactive breakpoint.\r\n"));
        pbBpKAddr = MapToDebuggeeCtxKernEquivIfAcc(pVM, (BYTE*) Address, TRUE);

        DEBUGGERMSG(KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Breakpoint @%08X maps to KVA %08X\r\n", Address, pbBpKAddr));
        
        if (pbBpKAddr)
        { // Memory is paged
#if defined(MIPSII) || defined(THUMBSUPPORT)
            if (g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_16BIT)
            {
                ulBpInstrLen = sizeof (KDP_BREAKPOINT_16BIT_TYPE);
                KdpBreakpointInstruction = KDP_BREAKPOINT_16BIT_VALUE;
            }
#endif
            /* Save the KAddr for the scenario in which the debugger is unable to find the
             * KAddr during delete (module pages decommitted/lost before attempt to delete
             * breakpoint). */
            g_aBreakpointTable[Index].KAddr = pbBpKAddr;

            // Get the instruction to be replaced
            if (KdpMoveMemory ((char *) &Content, pbBpKAddr, ulBpInstrLen ) != ulBpInstrLen)
            {
                DEBUGGERMSG (KDZONE_ALERT, (L"  KdpInstantiateSwBreakpoint: Cannot read access apparently paged KAddress (%08X)\r\n", pbBpKAddr));
            }
            else
            {
                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Successfully reading BP location\r\n"));
                fAccessible = TRUE;
                KdpSanitize ((BYTE *) &Content, pbBpKAddr, ulBpInstrLen, FALSE);
                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Successfully read %08X at %08X\r\n", (DWORD) Content, pbBpKAddr));
            }

            if (fAccessible)
            {
                BOOL fBpSet = FALSE;
                BOOL fRomBp = FALSE;
                
                // determine whether the address is in ROM space
                if (g_fForceTestRomBp || kdpIsROM (pbBpKAddr, 1))
                {
                    fRomBp = TRUE;
                    DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Breakpoint is reported in ROM\r\n"));
                }
                else
                {
                    fBpSet = KdpWriteBpAndVerify (pbBpKAddr, &KdpBreakpointInstruction, ulBpInstrLen);
                    if (!fBpSet)
                    { // the write failed
                        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Breakpoint could not be set - assume it is a ROM BP\r\n"));
                        fRomBp = TRUE;
                    }
                }

                if (fRomBp && *(g_kdKernData.pfForcedPaging))
                { // if BP in ROM and we are in High VM intrusiveness mode
                    int bphRomRet;
                    int idxRom2Ram = 0;
                    void* pvBpRamAddr = 0;

                    DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpInstantiateSwBreakpoint: Address is in ROM space\r\n")));

                    bphRomRet = KdpRomSwBpAttemptRamPageRemap (pVM, Address, pbBpKAddr, &idxRom2Ram, &pvBpRamAddr);

                    if (bphRomRet != KD_BPHND_ROMBP_SUCCESS)
                    {
                        // If we failed, there is no point doing anything else,
                        // so just leave returning the ROM BP error
                        bphRet = bphRomRet;
                    }
                    else
                    { // Try again
                        void* pvBpRamKAddr = MapToDebuggeeCtxKernEquivIfAcc (g_pprcNK, (BYTE*) pvBpRamAddr, TRUE);

                        fBpSet = KdpWriteBpAndVerify (pvBpRamKAddr, &KdpBreakpointInstruction, ulBpInstrLen);
                        if (!fBpSet)
                        { // the write failed again
                            bphRet = KD_BPHND_INVALID_GEN_ERR;

                            DEBUGGERMSG (KDZONE_ALERT, (TEXT("  KdpInstantiateSwBreakpoint: Failed to write in remapped ROM!\r\n")));
                            // fail to instanciate ROM BP, revert mapping
                            KdpRestoreRomVmPageMapping (pVM, idxRom2Ram);
                        }
                    }
                }

                if (fBpSet)
                { // The BP was written
                    DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Updating BP entry\r\n"));
                    g_aBreakpointTable [Index].Content = Content;
                    g_aBreakpointTable [Index].Flags &= ~KD_BREAKPOINT_SUSPENDED;
                    g_aBreakpointTable[Index].Flags |= KD_BREAKPOINT_WRITTEN;

                    // Check if the address is in ROM and increment the use count.
                    // We need to do this even if fRomBp == FALSE, since 
                    // a ROM page may already be remapped to RAM.
                    fRomBp = KdpIncCountIfRomSwBp(Address);
                    
                    if (fRomBp)
                    {
                        g_aBreakpointTable [Index].Flags |= KD_BREAKPOINT_INROM;
                    }
                }
                else
                {
                    DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: BP cannot set (probably ROM BP in kernel code or ROM BP while in non-intrusive mode)!\r\n"));
                    if (bphRet > KD_BPHND_INVALID_GEN_ERR) bphRet = KD_BPHND_INVALID_GEN_ERR;
                }
            }
        }
        else
        {
            DEBUGGERMSG (KDZONE_SWBP, (L"  KdpInstantiateSwBreakpoint: Unpaged address\r\n"));
        }
    }
    else
    {
        DEBUGGERMSG (KDZONE_ALERT, (L"  KdpInstantiateSwBreakpoint: invalid BP entry\r\n"));
    }

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpInstantiateSwBreakpoint (bph=%i)\r\n", bphRet));
    return bphRet;
}


/*++

Routine Name:

    KdpHandlePageInBreakpoints

Routine Description:

   This routine is called in response to the OS having performed a page in.
   The function will determine whether any breakpoints need to be set in the
   address range of the recently paged-in page and will attempt to instantiate
   those breakpoints by calling into KdpAddBreakpoint.

   ASSUMPTIONS:
   -XIP ROM code is never paged in or out
   -We get multiple notifications in case of multiple VM mappings of the same code.
    This is not the case for virtually copied data but we are going to assume that
    users will never put code in virtually copied pages themselves. In that unlikely case,
    the breakpoints will simply have potential side effects in case of page out / back in.

Arguments:

    [in] ulAddress - supplies the starting address of the page just paged in
    [in] ulNumPages - The number of pages that were paged in.  This is added to make
                    module load / process load of non-pageable modules / processes faster.

Return Value:

    None.

--*/

VOID KdpHandlePageInBreakpoints (ULONG ulAddress, ULONG ulNumPages)
{
    int cBp = 0; // Counter of BP tested to speed up iteration
    DWORD dwIndex;
    ULONG ulStartAddress = ulAddress;
    ULONG ulEndAddress = ulStartAddress + (ulNumPages * VM_PAGE_SIZE);

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpHandlePageInBreakpoints (0x%08X - 0x%08X, %d)\r\n", ulStartAddress, ulEndAddress, ulNumPages));

    // Search the global table to see if there are any addresses within
    // the page that was just paged in that were supposed
    // to have BP's instantiated. This includes addresses that had trap
    // instructions in them, but were paged out, and those that were
    // not paged in until now and never had a BP written to them. Attempt
    // to write BP's to all addresses within the page that fullfil those
    // requirements.

    // NOTE: Don't try to debug this code using KdStub itself as it will likely modify the BP table
    //  in order to step

    for (dwIndex = 0;
         (cBp < g_nTotalNumDistinctSwCodeBps) && (dwIndex < BREAKPOINT_TABLE_SIZE);
         dwIndex++)
    {
        BYTE bFlagsTemp = g_aBreakpointTable [dwIndex].Flags;
        if (g_aBreakpointTable [dwIndex].wRefCount)
        {
            ULONG ulAddrTemp = (ULONG) g_aBreakpointTable [dwIndex].Address;
            cBp++; // Note: even suspended BP are counted here as they are part of the g_nTotalNumDistinctSwCodeBps

            if (!(g_pKData->dwInDebugger && (bFlagsTemp & KD_BREAKPOINT_SUSPENDED))  &&
                (ulAddrTemp >= ulStartAddress) && 
                (ulAddrTemp < ulEndAddress))
            { // Any BP in range, except if suspended while in debugger:
                PPROCESS pCurrentVM;

                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpHandlePageInBreakpoints: Potential Breakpoint %08X (pVM=%08X)\r\n", ulAddrTemp, g_aBreakpointTable [dwIndex].pVM));

                pCurrentVM = GetProperVM (pVMProc, (void *)PAGE_ALIGN(ulAddrTemp));

                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpHandlePageInBreakpoints: pVM = %08X\r\n", pCurrentVM));
                if (pCurrentVM == g_aBreakpointTable [dwIndex].pVM)
                {
                    ULONG ulBpInstrLen = sizeof (KDP_BREAKPOINT_TYPE);
                    KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
#if defined(MIPSII) || defined(THUMBSUPPORT)
                    if (bFlagsTemp & KD_BREAKPOINT_16BIT)
                    {
                        ulBpInstrLen = sizeof (KDP_BREAKPOINT_16BIT_TYPE);
                        KdpBreakpointInstruction = KDP_BREAKPOINT_16BIT_VALUE;
                    }
#endif
                    ReadBack = 0;
                    if (ulBpInstrLen != KdpMoveMemory ((char *) &ReadBack, (char *) ulAddrTemp, ulBpInstrLen))
                    {
                        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpHandlePageInBreakpoints: Failed to read instruction of BP\r\n"));
                    }
                    else
                    {
                        BOOL fIsBpInstr = !memcmp (&ReadBack, &KdpBreakpointInstruction, ulBpInstrLen);
                        if (fIsBpInstr)
                        { // SW Breakpoint instruction already present - Re-instantiation not necessary - Probably new mapping (VirtualCopy) notification only
                            DEBUGGERMSG (KDZONE_SWBP, (L"  KdpHandlePageInBreakpoints: existing instruction already a SW breakpoint - this must be a VirtualCopy PageIn notification - bypassing BP instantiation\r\n"));
                        }
                        else
                        { // should be reinstantiated
                            DEBUGGERMSG (KDZONE_SWBP, (L"  KdpHandlePageInBreakpoints: Found address (0x%08X) within page range(0x%08X - 0x%08X) with flag %02X\r\n", ulAddrTemp, ulStartAddress, ulEndAddress, bFlagsTemp));              
                            g_aBreakpointTable [dwIndex].Flags |= KD_BREAKPOINT_SUSPENDED; // tag as suspended until actual instantiation succeeds
                            KdpInstantiateSwBreakpoint (dwIndex + 1); // Attempt instanciations
                        }
                    }
                }
            }
        }
    }

    // If we are in break state while this notification happens, then tell the host debugger to refresh its memory
    if ((g_pKData->dwInDebugger) && !(*(g_kdKernData.pfForcedPaging))) g_fDbgKdStateMemoryChanged = TRUE; // Note: will only be passed on next reply to debugger command

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpHandlePageInBreakpoints\r\n"));
}


/*++

Routine Name:

    KdpCleanupIfRomSwBp

Routine Description:

    This routine is usually called when a breakpoint has sucessfully
    been removed from the argument address. It determines whether the
    argument address is within the range of any of the used pre-allocated
    RAM pages, and if so decrements the breakpoint use count variable
    for that page. If the count is zero (since that was the last breakpoint)
    then the page is remapped to point to the orginal ROM page address and
    the RAM page is marked as free.

Arguments:

    [in] pVM    - Process affinity of breakpoint if any
    [in] pvAddr - supplies the address where a breakpoint was just removed from

Return Value:

    TRUE if BP was in ROM.

--*/

BOOL KdpCleanupIfRomSwBp (PROCESS *pVM, void * pvAddr)
{
    DWORD dwIndex = 0;
    DWORD dwROMAddrTemp = 0;
    DWORD dwAddr = (DWORD) pvAddr;
    BOOL fRet = FALSE;

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("++KdpCleanupIfRomSwBp (vaddr=%08X in Proc %08X(%s))\r\n"), dwAddr, pVM, pVM ? pVM->lpszProcName : L"null"));

    for (dwIndex = 0; (dwIndex < NB_ROM2RAM_PAGES) && !fRet; dwIndex++)
    {
        dwROMAddrTemp = (DWORD) (g_aRom2RamPageTable[dwIndex].pvROMAddr);

        // Check if the address of the breakpoint just deleted
        // falls in any of our preallocated/remapped ROM/RAM pages
        if ((dwAddr >= dwROMAddrTemp) && (dwAddr < dwROMAddrTemp + VM_PAGE_SIZE))
        {
            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpCleanupIfRomSwBp: Address found in page %d between (0x%08X) and (0x%08X)\r\n"), dwIndex, dwROMAddrTemp, dwROMAddrTemp+VM_PAGE_SIZE));
            fRet = TRUE;

            // Decrement the breakpoint use count for that page
             g_aRom2RamPageTable[dwIndex].nBPCount--;

            DEBUGGERMSG (KDZONE_SWBP, (TEXT ("  KdpCleanupIfRomSwBp: Page %d Count decremented to %d\r\n"), dwIndex, g_aRom2RamPageTable[dwIndex].nBPCount));

            // If that pages count variable is zero, we better remap the VA that used
            // to point to ROM, back to ROM (since it was changed to point to the RAM
            // page until now) - mark the page as free
            if (!g_aRom2RamPageTable[dwIndex].nBPCount)
            {
                DEBUGGERMSG (KDZONE_SWBP, (TEXT("  KdpCleanupIfRomSwBp: Count is 0, freeing page\r\n")));

                if (!KdpRestoreRomVmPageMapping (pVM, dwIndex))
                {
                    DEBUGGERMSG (KDZONE_ALERT, (TEXT("  KdpCleanupIfRomSwBp: ROM to RAM unmap failed! System may become unstable\r\n")));
                    // If remapping failed, there's not much we can do
                }
            }
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (TEXT ("--KdpCleanupIfRomSwBp (found=%d)\r\n"),fRet));
    return fRet;
}

/*++

Routine Description:

    This routine adds an entry to the breakpoint table and returns a handle
    to the breakpoint table entry.

Arguments:

    [in] pVM - VM for the address.
    [in] Address - Supplies the address where to set the breakpoint.

Return Value:

    A value of zero is returned if the specified address is already in the
    breakpoint table, there are no free entries in the breakpoint table, the
    specified address is not correctly aligned, or the specified address is
    not valid. Otherwise, the index of the assigned breakpoint table entry
    plus one is returned as the function value.

--*/

ULONG KdpAddBreakpoint (PPROCESS pVM, PVOID Address)
{
    int bphRet = KD_BPHND_INVALID_GEN_ERR;
    ULONG Index;
    BOOLEAN Accessible = FALSE;
    BOOLEAN Mode16Bit = FALSE; // used for Thumb and MIPS16
    BOOLEAN KAccessible = FALSE;
    KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
    ULONG ulBpInstrLen = sizeof (KDP_BREAKPOINT_TYPE);
    BOOL fBpSet = FALSE;

    BOOL fAddrUnpaged = FALSE;
    BOOL fKAddrUnpaged = FALSE;
    BOOL fUpdateBPTable = FALSE;
    BOOL fAttemptToWrite = FALSE;

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpAddBreakpoint (%08X, 0x%08X)\r\n", pVM, Address));

#if defined(MIPSII) || defined(THUMBSUPPORT)
    //  update the breakpoint Instruction and ulBpInstrLen if stopped within
    //  16-bit code. (16-bit code indicated by LSB of Address)
    if (Is16BitSupported && ((ULONG) Address & 1))
    {
        DEBUGGERMSG (KDZONE_SWBP,(L"  KdpAddBreakpoint: 16 Bit breakpoint\r\n"));
        ulBpInstrLen = sizeof (KDP_BREAKPOINT_16BIT_TYPE);
        KdpBreakpointInstruction = KDP_BREAKPOINT_16BIT_VALUE;
        Address = (PVOID) ((ULONG) Address & ~1);
        Mode16Bit = TRUE;
    }
#endif

    // If the specified address is not properly aligned, then return zero.
    if ((ULONG) Address & (ulBpInstrLen - 1))
    {
        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpAddBreakpoint: Address not aligned\r\n"));
    }
    else
    {
        int bphFreeEntry = KD_BPHND_INVALID_GEN_ERR;
        if (!pVM)
        {
            if(g_fHandlingLoadNotification && g_fWorkaroundPBForVMBreakpoints)
            {
                if(!KdpFindBreakpointKva(Address, &pVM))
                {
                    pVM = g_pFocusProcOverride? g_pFocusProcOverride : pVMProc;
                    DEBUGGERMSG(KDZONE_SWBP, (L"  KdpAddBreakpoint: Defaulting to process %08X\r\n", pVM));
                }
                else
                {
                    DEBUGGERMSG(KDZONE_SWBP, (L"  KdpAddBreakpoint: Found previous VM %08X\r\n", pVM));
                }
            }
            else
            {
                pVM = g_pFocusProcOverride? g_pFocusProcOverride : pVMProc;
                DEBUGGERMSG(KDZONE_SWBP, (L"  KdpAddBreakpoint: pVM replaced with %08X\r\n", pVM));
            }
        }

        DEBUGGERMSG(KDZONE_SWBP, (L"  KdpAddBreakpoint: Breakpoint @%08X in proc %08X(%s)\r\n", Address, pVM, pVM ? pVM->lpszProcName : L"null"));
        
        // Find an empty spot or identical entry in the table of BP
        for (Index = 0; (Index < BREAKPOINT_TABLE_SIZE) && (bphRet == KD_BPHND_INVALID_GEN_ERR); Index++)
        {
            if ((bphFreeEntry == KD_BPHND_INVALID_GEN_ERR) && !(g_aBreakpointTable [Index].wRefCount))
            {
                bphFreeEntry = Index + 1; // remember 1st free entry (+1 to convert index to handle)
            }
            if (g_aBreakpointTable [Index].wRefCount &&
                (g_aBreakpointTable [Index].Address == Address) &&
                (g_aBreakpointTable[Index].pVM == pVM))
            {
                // Then we have a dup entry
                DEBUGGERMSG (KDZONE_ALERT, (L"  KdpAddBreakpoint: Dup - Found existing BP (0x%08X) - should not happening if host-side aliasing / ref count! Risk of improper single steping!!!!\r\n", Address));
                bphRet = Index + 1;
                ++(g_aBreakpointTable [Index].wRefCount);
            }
        }

        if (bphRet == KD_BPHND_INVALID_GEN_ERR)
        { // if no dup, use free entry
            bphRet = bphFreeEntry;
            if (bphRet != KD_BPHND_INVALID_GEN_ERR)
            { // free entry available
                int bphTemp;
                DWORD bpIdx = bphRet - 1;
                // Create table entry
                g_aBreakpointTable [bpIdx].wRefCount = 1;
                g_aBreakpointTable [bpIdx].Address = Address;
                g_aBreakpointTable[bpIdx].pVM = pVM;
                g_aBreakpointTable [bpIdx].Flags = KD_BREAKPOINT_SUSPENDED; // Reserve suspended until actually instantiated
                if (Mode16Bit)
                {
                    g_aBreakpointTable [bpIdx].Flags |= KD_BREAKPOINT_16BIT;
                }
                bphTemp = KdpInstantiateSwBreakpoint (bphRet);
                if (bphRet != bphTemp)
                { // Instantiation will never be possible for this BP
                    g_aBreakpointTable [bpIdx].Flags = 0; // discard BP entry
                    g_aBreakpointTable [bpIdx].Content = 0;
                    g_aBreakpointTable [bpIdx].Address = NULL;
                    g_aBreakpointTable[bpIdx].pVM = NULL;
                    g_aBreakpointTable [bpIdx].wRefCount = 0;
                    bphRet = bphTemp;
                }
                else
                {
                    g_nTotalNumDistinctSwCodeBps++;
                }
            }
            else
            {
                DEBUGGERMSG (KDZONE_ALERT, (L"  KdpAddBreakpoint: Maximum number of Breakpoints reach. Cannot handle this one\r\n"));
            }
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpAddBreakpoint: (handle = %i)\r\n", bphRet));
    return bphRet;
}


/*++

Routine Description:

    This routine deletes an entry from the breakpoint table.

Arguments:

    [in] Handle - Supplies the index plus one of the breakpoint table entry
        which is to be deleted.

Return Value:

    A value of FALSE is returned if the specified handle is not a valid
    value or the breakpoint cannot be deleted because the old instruction
    cannot be replaced. Otherwise, a value of TRUE is returned.

--*/

BOOLEAN KdpDeleteBreakpoint (ULONG Handle)
{
    BOOLEAN fRet = FALSE;
    KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
    ULONG ulBpInstrLen = sizeof (KDP_BREAKPOINT_TYPE);
    ULONG Index = Handle - 1; // Handles are 1-based; 0 is an invalid handle value

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpDeleteBreakpoint Handle=%i\r\n", Handle));

    // If the specified handle is not valid, then return FALSE.
    if ((Index >= BREAKPOINT_TABLE_SIZE) ||
        !(g_aBreakpointTable[Index].wRefCount))
    {
        DEBUGGERMSG (KDZONE_ALERT, (L"  KdpDeleteBreakpoint: Invalid handle\r\n"));
    }
    else 
    {
        --g_aBreakpointTable [Index].wRefCount;
        if (!(g_aBreakpointTable [Index].wRefCount))
        { // ref count null: really delete BP
            // There may be an issue when the code page is paged out.  Debugger may force it to page in.
            BYTE *pbBpKAddr = MapToDebuggeeCtxKernEquivIfAcc(g_aBreakpointTable[Index].pVM, g_aBreakpointTable[Index].Address, TRUE);

            if(g_fHandlingLoadNotification && g_fWorkaroundPBForVMBreakpoints)
            {
                DEBUGGERMSG(KDZONE_SWBP, (L"  KdpDeleteBreakpoint: Remembering %08X maps to %08X\r\n", g_aBreakpointTable[Index].Address,
                                          g_aBreakpointTable[Index].pVM));
                KdpSaveBreakpointKva(g_aBreakpointTable[Index].Address, g_aBreakpointTable[Index].pVM);
            }

#if defined(MIPSII) || defined(THUMBSUPPORT)
            // Determine the breakpoint instruction and size
            if (Is16BitSupported && (g_aBreakpointTable[Index].Flags & KD_BREAKPOINT_16BIT))
            {
                ulBpInstrLen = sizeof (KDP_BREAKPOINT_16BIT_TYPE);
                KdpBreakpointInstruction = KDP_BREAKPOINT_16BIT_VALUE;
            }
#endif

            // Kdstub does not know when page is paged out. If we did, we could
            // sanitize it to have readonly mem in RAM behave similarily to
            // readonly mem in ROM. This block assumes that the kaddr used to
            // write the breakpoint is still valid.
            if (!pbBpKAddr)
            {
                if (g_aBreakpointTable[Index].KAddr)
                {
                    KDP_BREAKPOINT_TYPE Instruction = 0;

                    // Just in case there is an access violation on memmove.
                    __try
                    {
                        if (memmove (&Instruction, g_aBreakpointTable[Index].KAddr, ulBpInstrLen))
                        {
                            if (Instruction == KdpBreakpointInstruction)            // If Breakpoint Found.
                            {
                                DEBUGGERMSG (KDZONE_SWBP,
                                    (L"  KdpDeleteBreakpoint: Found BP at kaddr(0x%08X) - Preemptively restoring instruction.\r\n",
                                        g_aBreakpointTable[Index].KAddr));
                                pbBpKAddr = g_aBreakpointTable[Index].KAddr;        // Mark it for cleaning.
                            }
                            else
                                DEBUGGERMSG (KDZONE_SWBP,
                                    (L"  KdpDeleteBreakpoint: No BP found at kaddr(0x%08X) - Not restoring instruction.\r\n",
                                        g_aBreakpointTable[Index].KAddr));
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        DEBUGGERMSG (KDZONE_SWBP,
                            (L"  KdpDeleteBreakpoint: Unable to read kaddr(0x%08X) - Not restoring instruction.\r\n",
                                g_aBreakpointTable[Index].KAddr));
                    }
                }
            }

            if (!pbBpKAddr) // Test accessibility (won't page-in)
            { // If the breakpoint is not instantiated, then just nullify the table entry, no need to copy anything
                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpDeleteBreakpoint: Deleting entry for unpaged address (0x%08X)\r\n", g_aBreakpointTable[Index].Address));
                fRet = TRUE;
            }
            else if (g_aBreakpointTable[Index].Flags & KD_BREAKPOINT_WRITTEN)
            { // Otherwise, if the BP is actually set, then replace the old instruction and nullify the entry
                fRet = KdpWriteBpAndVerify (pbBpKAddr, &g_aBreakpointTable [Index].Content, ulBpInstrLen);
                if (!fRet)
                {
                    DEBUGGERMSG (KDZONE_ALERT, (L"  KdpDeleteBreakpoint: Could not write original instruction back for BP (%i) Addr=0x%08X, KAddr=0x%08X\r\n", Index, g_aBreakpointTable[Index].Address, pbBpKAddr));
                }
            }
            else
            {
                // Breakpoint not written?  Don't write back.
                DEBUGGERMSG(KDZONE_SWBP, (L"  KdpDeleteBreakpoint: g_aBreakpointTable[Index].Flags = %08X\r\n", g_aBreakpointTable[Index].Flags));
                fRet = TRUE;
            }

            if (g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_INROM)
            { // If BP was in ROM, cleanup remapping to RAM
                KdpCleanupIfRomSwBp (g_aBreakpointTable[Index].pVM, g_aBreakpointTable[Index].Address);
            }

            // Delete breakpoint table entry and return TRUE.
            g_aBreakpointTable[Index].Flags = 0;
            g_aBreakpointTable[Index].Content = 0;
            g_aBreakpointTable[Index].Address = NULL;
            g_aBreakpointTable[Index].KAddr = NULL;
            if (g_nTotalNumDistinctSwCodeBps) g_nTotalNumDistinctSwCodeBps--;
        }
        else if (!(g_aBreakpointTable[Index].Flags & KD_BREAKPOINT_WRITTEN))
        {
            // Breakpoint was never written.  This should never happen.
            DEBUGGERMSG (KDZONE_ALERT, (L"  KdpDeleteBreakpoint: Deleting entry for unwritten breakpoint @(0x%08X)\r\n", g_aBreakpointTable[Index].Address));
            fRet = TRUE;
        } 
        else
        {
            DEBUGGERMSG (KDZONE_ALERT, (L"  KdpDeleteBreakpoint: Dup - Removing multi-ref BP @(0x%08X) - should not happening if host-side aliasing / ref count! Risk of improper single steping!!!!\r\n", g_aBreakpointTable[Index].Address));
            fRet = TRUE;
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpDeleteBreakpoint (%d)\r\n", (int) fRet));
    return fRet;
}


/*++

Routine Description:

    Call KdpDeleteBreakpoint on every breakpoint handle and ignore the result.  If all
    goes well, this should remove all breakpoints in the system and allow it to
    execute normally.

--*/

VOID KdpDeleteAllBreakpoints (VOID)
{
    ULONG i;

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpDeleteAllBreakpoints\r\n"));

    // handles are 1-based
    for (i = 1; g_nTotalNumDistinctSwCodeBps && (i <= BREAKPOINT_TABLE_SIZE); i++)
    {
        if (g_aBreakpointTable [i - 1].wRefCount) KdpDeleteBreakpoint (i);
    }

    memset (g_aBreakpointTable, 0, sizeof (g_aBreakpointTable)); // clean up (necessary for 1st time init)

    DEBUGGERMSG(KDZONE_SWBP, (L"--KdpDeleteAllBreakpoints\r\n"));
}


/*++

Routine Description:

    Suspend the specified breakpoint by restoring its instruction

Arguments:

    Index - index of the breakpoint to suspend (NOTE: NOT THE HANDLE)

Return Value:

    TRUE - the breakpoint was suspended
    FALSE - otherwise

--*/

BOOLEAN KdpSuspendBreakpoint (ULONG Index)
{
    BOOLEAN fRet = FALSE;
    KDP_BREAKPOINT_TYPE KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
    ULONG ulBpInstrLen = sizeof (KDP_BREAKPOINT_TYPE);

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpSuspendBreakpoint (handle=%i)\r\n", Index + 1));

    // Determine the breakpoint instruction and size
#if defined(MIPSII) || defined(THUMBSUPPORT)
    if (Is16BitSupported && (g_aBreakpointTable[Index].Flags & KD_BREAKPOINT_16BIT))
    {
        ulBpInstrLen = sizeof(KDP_BREAKPOINT_16BIT_TYPE);
        KdpBreakpointInstruction = KDP_BREAKPOINT_16BIT_VALUE;
    }
#endif
    // Replace the instruction contents.
    if (g_aBreakpointTable [Index].wRefCount &&
        !(g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_SUSPENDED))
    {
        BYTE *pbBpKAddr = MapToDebuggeeCtxKernEquivIfAcc (g_aBreakpointTable[Index].pVM, (BYTE *) (g_aBreakpointTable [Index].Address), TRUE);

        if (pbBpKAddr)
        {
            DEBUGGERMSG (KDZONE_SWBP, (L"  KdpSuspendBreakpoint: Suspending BP 0x%08X\r\n", g_aBreakpointTable[Index].Address));
            fRet = KdpWriteBpAndVerify (pbBpKAddr, &g_aBreakpointTable [Index].Content, ulBpInstrLen);
            if (!fRet)
            {
                DEBUGGERMSG (KDZONE_ALERT, (L"  KdpSuspendBreakpoint: Suspend failed!\r\n"));
            }
        }
        else
        {
            DEBUGGERMSG (KDZONE_ALERT, (L"  KdpSuspendBreakpoint: BP at 0x%08X not mapped!\r\n", g_aBreakpointTable[Index].Address));
        }
        g_aBreakpointTable [Index].Flags |= KD_BREAKPOINT_SUSPENDED; // Tag suspended in all cases (in not writable, the SW trap instr is virtually removed)
    }

    DEBUGGERMSG(KDZONE_SWBP, (L"--KdpSuspendBreakpoint (%d)\r\n", (int) fRet));

    return fRet;
}


/*++

Routine Description:

    Suspend all breakpoints that are currently in use and not already suspened.

--*/

VOID KdpSuspendAllBreakpoints (VOID)
{
    ULONG Index;

    DEBUGGERMSG(KDZONE_SWBP, (L"++KdpSuspendAllBreakpoints\r\n"));

    // Examine each entry in the table in turn
    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++)
    {
        // Make sure breakpoint is in use and not already suspended
        if (g_aBreakpointTable [Index].wRefCount &&
            !(g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_SUSPENDED))
        {
            KdpSuspendBreakpoint (Index);
        }
    }
    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpSuspendAllBreakpoints\r\n"));
}


/*++

Routine Description:

    Reinstate any breakpoints that were suspended. This function should be called before exitting KdpTrap.

--*/

VOID KdpReinstateSuspendedBreakpoints (VOID)
{
    int Index;
    DEBUGGERMSG(KDZONE_SWBP, (L"++KdpReinstateSuspendedBreakpoints.\r\n"));

    // Examine each entry in the table in turn
    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++)
    {
        if (g_aBreakpointTable [Index].wRefCount &&
            (g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_SUSPENDED))
        {
            int bph = KdpInstantiateSwBreakpoint (Index + 1); // Attempt instantiations
            if (bph <= KD_BPHND_INVALID_GEN_ERR)
            {
                DEBUGGERMSG (KDZONE_SWBP, (L"  KdpReinstateSuspendedBreakpoints: Reinstate failed for BP %i (Addr=0x%08X)!\r\n",
                                           Index, g_aBreakpointTable [Index].Address));
            }
        }
    }
    DEBUGGERMSG(KDZONE_SWBP, (L"--KdpReinstateSuspendedBreakpoints.\r\n"));
}


/*++

Routine Description:

    Suspend the breakpoint, if one exists, at the specified location.  It is assumed this routine
    will be called if the debugger "trips" over one of its own breakpoints which was set in code
    intended to be shared by KdStub.  This should allow stepping through such code.

    Note: currently, this function will not suspend more than one (1) breakpoint, under the
    assumption that KdpAddBreakpoint will not instantiate multiple breakpoints at the same location.

Arguments:

    Address - the address of the instruction that was just hit

Return Value:

    TRUE - there was a breakpoint at that location and it was suspended
    FALSE - otherwise

--*/

BOOLEAN KdpSuspendBreakpointIfHitByKd (void * pvAddress)
{ // TODO: Replace this with a call to KdpSuspendAllBreakpoints(). There is no reason to suspend only one.
    BOOLEAN fRet = FALSE;
    ULONG Index;
    BYTE* pbKAddr;

    DEBUGGERMSG (KDZONE_SWBP, (L"++KdpSuspendBreakpointIfHitByKd (0x%08X)\r\n", (DWORD) pvAddress));

    if (Is16BitSupported)
    {
        pvAddress = (void *) (((DWORD) pvAddress) & ~1);
    }

    pbKAddr = (BYTE *) MapToDebuggeeCtxKernEquivIfAcc (NULL, (BYTE *) pvAddress, TRUE); // Kernel equivalent

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++)
    {
        BYTE *pbBpKAddr = MapToDebuggeeCtxKernEquivIfAcc (g_aBreakpointTable[Index].pVM, (BYTE *) (g_aBreakpointTable [Index].Address), TRUE);
        if (g_aBreakpointTable [Index].wRefCount &&
            !(g_aBreakpointTable [Index].Flags & KD_BREAKPOINT_SUSPENDED) &&
            (pbBpKAddr == pbKAddr))
        {
            fRet = KdpSuspendBreakpoint (Index);
        }
    }

    DEBUGGERMSG (KDZONE_SWBP, (L"--KdpSuspendBreakpointIfHitByKd (%d)\r\n", (int) fRet));

    return fRet;
}


/*++

Routine Description:

    Sanitize a region of memory.  If the specified region of memory contains a breakpoint, write the data
    into the output buffer as it would appear if there were no breakpoints.

    The memory address (pvAddress) is must be qualified to a particular process' address space before being
    compared with addresses of breakpoints.

Arguments:

    pbClean:     Output buffer to which sanitized memory is written
    pvAddress:   First virtual address of the region of memory to be cleaned.  This must be the actual
                 address of the memory (so it can be matched against the breakpoint table) and it must
                 be readable.
    nSize:       Size of both the output buffer and the memory to be cleaned
    fAlwaysCopy: Copy the memory into the output buffer regardless of whether the memory contains a
                 breakpoint.  Normally, the memory is only copied if a breakpoint was present.

Return Value:

    TRUE  - there was a breakpoint in the specified range and it has been sanitized in the output buffer
    FALSE - otherwise

--*/

BOOL KdpSanitize (BYTE* pbClean, VOID* pvAddress, ULONG nSize, BOOL fAlwaysCopy)
{
    BOOL fRet = FALSE;
    ULONG iBp;
    BYTE* pbKAddr;

    DEBUGGERMSG(KDZONE_BREAK,(L"++KdpSanitize dest %08X, addr %08X, bytes %d, alwaysCopy %d\r\n",
                              pbClean, pvAddress, nSize, fAlwaysCopy));

    // Defaulting to the default VM.  This should be ok because NKGetThreadCallstack will actually
    // switch the current VM through PSL calls.
    pbKAddr = (BYTE*) MapToDebuggeeCtxKernEquivIfAcc (NULL, (BYTE *) pvAddress, TRUE); // Kernel equivalent
    if (pbKAddr)
    {
        if (fAlwaysCopy)
        { // initialize the buffer
            KdpMoveMemory (pbClean, pbKAddr, nSize);
        }

        for (iBp = 0; iBp < BREAKPOINT_TABLE_SIZE; iBp++)
        {
            if (g_aBreakpointTable[iBp].wRefCount &&
                !(g_aBreakpointTable[iBp].Flags & KD_BREAKPOINT_SUSPENDED))
            {
                BYTE *pbBpKAddr = MapToDebuggeeCtxKernEquivIfAcc (g_aBreakpointTable[iBp].pVM, (BYTE *) (g_aBreakpointTable [iBp].Address), TRUE);
                if (pbBpKAddr)
                {
                    ULONG nBpSize = sizeof (KDP_BREAKPOINT_TYPE);

#if defined(MIPSII) || defined(THUMBSUPPORT)
                    if (Is16BitSupported && (g_aBreakpointTable[iBp].Flags & KD_BREAKPOINT_16BIT))
                    {
                        nBpSize = sizeof (KDP_BREAKPOINT_16BIT_TYPE);
                    }
#endif
                    if ((pbKAddr < (pbBpKAddr + nBpSize)) &&
                        ((pbKAddr + nSize) > pbBpKAddr))
                    {
                        ULONG nCommonBytes = nBpSize;
                        BYTE* pbSrc = (BYTE*) &g_aBreakpointTable [iBp].Content;
                        BYTE* pbDst = pbClean + (pbBpKAddr - pbKAddr);
                        LONG  lDiff;

                        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpSanitize: cleaning BP h=%d, addr=0x%08X\r\n", iBp + 1, pbBpKAddr));

                        // check for memory buffer beginning in the middle of the breakpoint
                        lDiff = pbKAddr - pbBpKAddr;
                        if (lDiff > 0)
                        {
                            pbSrc  += lDiff;
                            pbDst  += lDiff;
                            nCommonBytes -= lDiff;
                        }

                        // check for memory buffer ending in the middle of the breakpoint
                        lDiff = (pbBpKAddr + nBpSize) - (pbKAddr + nSize);
                        if (lDiff > 0)
                        {
                            nCommonBytes -= lDiff;
                        }

                        KD_ASSERT ((nCommonBytes > 0) && (nCommonBytes <= nBpSize));
                        KD_ASSERT ((pbClean <= pbDst) && ((pbDst + nCommonBytes) <= (pbClean + nSize)));

                        DEBUGGERMSG (KDZONE_SWBP, (L"  KdpSanitize: copy %d byte(s) from 0x%08X to 0x%08X\r\n", nCommonBytes, (DWORD)pbSrc, (DWORD)pbDst));

                        // initialize the clean buffer if it hasn't been done by now
                        if (!fRet && !fAlwaysCopy) memcpy (pbClean, pvAddress, nSize);

                        KdpMoveMemory (pbDst, pbSrc, nCommonBytes);

                        fRet = TRUE;
                    }
                }
                else
                {
                    g_aBreakpointTable [iBp].Flags |= KD_BREAKPOINT_SUSPENDED; // Assume that it is suspended since page is out (faster handling next time)
                }
            }
        }
    }

    DEBUGGERMSG(KDZONE_BREAK,(L"--KdpSanitize returning %d\r\n", fRet));
    return fRet;
}


VOID
KdpManipulateBreakPoint(
    IN DBGKD_COMMAND *pdbgkdCmdPacket,
    IN PSTRING AdditionalData
    )
/*++

Routine Description:

    This function is called in response of a manipulate breakpoint
    message.  Its function is to query/write/set a breakpoint
    and return a handle to the breakpoint.

Arguments:

    pdbgkdCmdPacket - Supplies the command message.

    AdditionalData - Supplies any additional data for the message.

Return Value:

    None.

--*/
{
    // TODO: Add support for physical address breakpoints
    DBGKD_MANIPULATE_BREAKPOINT *a = &pdbgkdCmdPacket->u.ManipulateBreakPoint;
    STRING MessageHeader;
    BOOL fSuccess = FALSE;
    DWORD dwExpAddDataSize;
    DWORD i;
    BOOL fHALBP=FALSE;
    KD_BPINFO bpInfo;

    MessageHeader.Length = sizeof (*pdbgkdCmdPacket);
    MessageHeader.Buffer = (PCHAR) pdbgkdCmdPacket;

    DEBUGGERMSG(KDZONE_BREAK, (L"++KdpManipulateBreakPoint\r\n"));

    if (a->dwMBpFlags & DBGKD_MBP_FLAG_DATABP)
    {
        dwExpAddDataSize = sizeof (DBGKD_DATA_BREAKPOINT_ELEM1);
    }
    else
    {
        dwExpAddDataSize = sizeof (DBGKD_CODE_BREAKPOINT_ELEM1);
    }
    dwExpAddDataSize *= a->dwBpCount;

    if (AdditionalData->Length != dwExpAddDataSize)
    {
        pdbgkdCmdPacket->dwReturnStatus = STATUS_UNSUCCESSFUL;
        KdpSendKdApiCmdPacket (&MessageHeader, AdditionalData);
        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Length mismatch\n"));
    }

    DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: dwBpCount = %ld\n", a->dwBpCount));
    for (i = 0; i < a->dwBpCount; i++)
    {
        if (a->dwMBpFlags & DBGKD_MBP_FLAG_DATABP)
        { // DATA BP
            // TODO: Add support for triggering on Register change and Data
            DBGKD_DATA_BREAKPOINT_ELEM1 *b = (DBGKD_DATA_BREAKPOINT_ELEM1 *) AdditionalData->Buffer;
            DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Address = %08X Flags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
            if (g_kdKernData.pKDIoControl)
            { // Try to Hardware Data BP
                if (a->dwMBpFlags & DBGKD_MBP_FLAG_SET)
                {
                    bpInfo.nVersion = 1;
                    bpInfo.ulAddress = b[i].dwAddress; // TODO: Use physical address or switch VA DataBP with context
                    bpInfo.ulFlags = 0;
                    bpInfo.ulHandle = 0;
                    if (KDIoControl (KD_IOCTL_SET_DBP, &bpInfo, sizeof (KD_BPINFO)))
                    {
                        fSuccess = TRUE;
                        fHALBP = TRUE;
                        b[i].dwBpHandle = bpInfo.ulHandle | 0x80000000;
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Set Hard DBP Address = %08X wBpFlags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
                    }
                }
                else if (b[i].dwBpHandle & 0x80000000)
                {
                    fHALBP = TRUE;
                    bpInfo.nVersion = 1;
                    bpInfo.ulHandle = b[i].dwBpHandle & 0x7FFFFFFF;
                    bpInfo.ulFlags = 0;
                    bpInfo.ulAddress = 0;
                    if (KDIoControl (KD_IOCTL_CLEAR_DBP, &bpInfo, sizeof(KD_BPINFO)))
                    {
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Clear Hard DBP Address = %08X wBpFlags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
                        fSuccess = TRUE;
                    }
                }
            }
            if (!fHALBP)
            { // SW Data BP
                // TODO: Add support for DATA BP emulation
            }
        }
        else
        { // CODE BP
            DBGKD_CODE_BREAKPOINT_ELEM1 *b = (DBGKD_CODE_BREAKPOINT_ELEM1 *) AdditionalData->Buffer;
            DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Address = %08X Flags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
            if ((b[i].wBpFlags == DBGKD_BP_FLAG_SOFTWARE) && g_kdKernData.pKDIoControl)
            { // Try to Hardware Code BP
                if (a->dwMBpFlags & DBGKD_MBP_FLAG_SET)
                {
                    bpInfo.nVersion = 1;
                    bpInfo.ulAddress = b[i].dwAddress; // TODO: Use physical address or switch VA HW BP with context
                    bpInfo.ulFlags = 0;
                    if (KDIoControl (KD_IOCTL_SET_CBP, &bpInfo, sizeof (KD_BPINFO)))
                    {
                        fSuccess = TRUE;
                        fHALBP = TRUE;
                        b[i].dwBpHandle = bpInfo.ulHandle | 0x80000000;
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Set  Hard CBP Address = %08X wBpFlags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
                    }
                }
                else if (b[i].dwBpHandle & 0x80000000)
                {
                    fHALBP = TRUE;
                    bpInfo.nVersion = 1;
                    bpInfo.ulHandle = b[i].dwBpHandle & 0x7FFFFFFF;
                    bpInfo.ulFlags = 0;
                    if (KDIoControl (KD_IOCTL_CLEAR_CBP, &bpInfo, sizeof(KD_BPINFO)))
                    {
                        fSuccess = TRUE;
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Clear  Hard CBP Address = %08X wBpFlags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
                    }
                }
            }
            if (!fHALBP)
            { // SW Code BP
                if (a->dwMBpFlags & DBGKD_MBP_FLAG_SET)
                {
                    PPROCESS pVM = NULL;
                    
                    b[i].wBpFlags |= DBGKD_BP_FLAG_SOFTWARE;
                    if (Is16BitSupported && (b[i].wBpExecMode == DBGKD_EXECMODE_16BIT)) b[i].dwAddress |= 0x1;

                    if (a->dwMBpFlags & DBGKD_MBP_FLAG_AFFINITY)
                    {
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Process Affinity %08X\r\n", a->dwProcessHandle));
                        if (a->dwProcessHandle)
                        {
                            OSAXSFN_PROCESSFROMHANDLE osaxsArgs = {0};
                            osaxsArgs.hProc = (HANDLE) a->dwProcessHandle;

                            if (SUCCEEDED(Hdstub.pfnCallClientIoctl(OSAXST1_NAME, OSAXST1_IOCTL_PROCESS_FROM_HANDLE, &osaxsArgs)))
                            {
                                pVM = osaxsArgs.pProc;
                            }
                            else
                            {
                                pVM = NULL;
                            }
                            DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Process Ptr %08X\r\n", pVM));
                        }

                        if(!pVM)
                        {
                            if(((b[i].dwAddress >= VM_DLL_BASE) && (b[i].dwAddress < VM_SHARED_HEAP_BASE)) || (b[i].dwAddress >= VM_KMODE_BASE))
                            {
                                DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: %08X <= %08X < %08X, defaulting to NK.EXE VM\r\n", VM_DLL_BASE, b[i].dwAddress, VM_SHARED_HEAP_BASE));
                                pVM = g_pprcNK;
                            }
                            else if(g_pFocusProcOverride)
                            {
                                DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: defaulting VM to override %08X\r\n", g_pFocusProcOverride));
                                pVM = g_pFocusProcOverride;
                            }
                            else
                            {
                                DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: defaulting VM to current %08X\r\n", pVMProc));
                                pVM = pVMProc;
                            }
                        }
                    }

                    if (b[i].dwBpHandle = KdpAddBreakpoint (pVM, (PVOID) b[i].dwAddress))
                    {
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Set Soft CBP Address = %08X wBpFlags=%08X dwBpHandle=%08X\n", b[i].dwAddress, b[i].wBpFlags, b[i].dwBpHandle));
                        fSuccess = TRUE;
                    }
                }
                else
                {
                    if (KdpDeleteBreakpoint (b[i].dwBpHandle))
                    {
                        DEBUGGERMSG(KDZONE_BREAK, (L"  KdpManipulateBreakPoint: Clear Soft CBP dwBpHandle=%08X\n", b[i].dwBpHandle));
                        fSuccess = TRUE;
                    }
                }
            }
        }
    }

    if (fSuccess)
    {
         pdbgkdCmdPacket->dwReturnStatus = fSuccess ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }
    a->NbBpAvail.dwNbHwCodeBpAvail = 0; // TODO: Get this from OAL
    a->NbBpAvail.dwNbSwCodeBpAvail = BREAKPOINT_TABLE_SIZE - g_nTotalNumDistinctSwCodeBps;
    a->NbBpAvail.dwNbHwDataBpAvail = 0; // TODO: Get this from OAL
    a->NbBpAvail.dwNbSwDataBpAvail = 0;
    DEBUGGERMSG (KDZONE_BREAK, (L"--KdpManipulateBreakPoint Status = %ld\n", pdbgkdCmdPacket->dwReturnStatus));
    KdpSendKdApiCmdPacket (&MessageHeader, AdditionalData);
    return;
}


DWORD KdpFindBreakpoint(void* pvOffset)
{
    DWORD i = 0;
    DWORD dwHandle = 0;

    void* pvKAddr = MapToDebuggeeCtxKernEquivIfAcc(NULL, pvOffset, TRUE);
    while (!dwHandle && i < BREAKPOINT_TABLE_SIZE)
    {
        if (g_aBreakpointTable[i].KAddr == pvKAddr || g_aBreakpointTable[i].Address == pvOffset)
        {
            dwHandle = i + 1;
        }
        else
        {
            ++i;
        }
    }

    return dwHandle;
}
