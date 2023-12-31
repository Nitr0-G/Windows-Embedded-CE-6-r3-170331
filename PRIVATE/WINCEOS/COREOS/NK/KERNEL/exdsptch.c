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

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames.

--*/

#include "kernel.h"
#include "excpt.h"
#include "pager.h"

FARPROC g_pfnKrnRtlDispExcp;
FARPROC g_pfnUsrRtlDispExcp;

#ifndef x86
FARPROC g_pfnRtlUnwindOneFrame;
#endif

ERRFALSE(offsetof (CallSnapshotEx, dwReturnAddr) == 0);
ERRFALSE(offsetof (CallSnapshot3, dwReturnAddr) == 0);

ERRFALSE(offsetof (CallSnapshot3, dwFramePtr) == offsetof (CallSnapshotEx, dwFramePtr));
ERRFALSE(offsetof (CallSnapshot3, dwActvProc) == offsetof (CallSnapshotEx, dwCurProc));



// Obtain current stack limits
//  if we're on original stack, or if there is a secure stack (pth->tlsSecure != pth->tlsNonSecure)
//
static BOOL GetStackLimitsFromTLS (LPDWORD tlsPtr, LPDWORD pLL, LPDWORD pHL)
{
    *pLL = tlsPtr[PRETLS_STACKBASE];
    *pHL = tlsPtr[PRETLS_STACKBASE] + tlsPtr[PRETLS_STACKSIZE] - 4 * REG_SIZE;

    return (IsKModeAddr ((DWORD) tlsPtr)       // on secure stack - trust it
            || (   IsValidUsrPtr ((LPVOID) (*pLL), sizeof (DWORD), TRUE)    // lower-bound is a user address
                && IsValidUsrPtr ((LPVOID) (*pHL), sizeof (DWORD), TRUE)    // upper-bound is a user address
                && (*pHL > *pLL)));                                         // upper bound > lower bound
}

static PCALLSTACK ReserveCSTK (DWORD dwCtxSP, DWORD dwMode)
{
    PCALLSTACK pcstk = pCurThread->pcstkTop;

    DWORD dwSP = (USER_MODE != dwMode)
                ? dwCtxSP
                : (pcstk
                        ? (DWORD) pcstk
                        : ((DWORD) pCurThread->tlsSecure - SECURESTK_RESERVE));

    return (PCALLSTACK) (dwSP - ALIGNSTK (sizeof(CALLSTACK)));
}

static void SetStackOverflow (PEXCEPTION_RECORD pExr)
{
    pExr->ExceptionCode = STATUS_STACK_OVERFLOW;
    pExr->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
}

static BOOL CheckCommitStack (PEXCEPTION_RECORD pExr, DWORD dwAddr, DWORD mode)
{
    LPDWORD pTlsPtr = (KERNEL_MODE == mode)? pCurThread->tlsSecure : pCurThread->tlsNonSecure;
    
    // if we're committing non-secure stack of a untrusted app, check for stack overflow
    if ((dwAddr < pTlsPtr[PRETLS_STACKBOUND])
        && (dwAddr >= pTlsPtr[PRETLS_STACKBASE])) {
        // committed user non-secure stack, check stack overflow
        dwAddr &= -VM_PAGE_SIZE;
        pTlsPtr[PRETLS_STACKBOUND] = dwAddr;
        if (dwAddr < (pTlsPtr[PRETLS_STACKBASE] + MIN_STACK_RESERVE)) {
            // stack overflow
            SetStackOverflow (pExr);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL IsLastStackPage (DWORD dwAddr, DWORD mode)
{
    LPDWORD pTlsPtr = (KERNEL_MODE == mode)? pCurThread->tlsSecure : pCurThread->tlsNonSecure;
    return (dwAddr - pTlsPtr[PRETLS_STACKBASE]) < VM_PAGE_SIZE;
}

static BOOL SetupContextOnUserStack (PEXCEPTION_RECORD pExr, PCONTEXT pCtx)
{
    PCONTEXT pUsrCtx;
    PEXCEPTION_RECORD pUsrExr;
    // make room for CONTEXT, EXCEPTION_RECORD
    DWORD dwNewSP = (DWORD) CONTEXT_TO_STACK_POINTER(pCtx) - sizeof (CONTEXT) - sizeof (EXCEPTION_RECORD);
    BOOL  fRet = TRUE;

    pUsrCtx = (PCONTEXT) dwNewSP;
    pUsrExr = (PEXCEPTION_RECORD) (pUsrCtx+1);

#ifdef x86
    // x86 needs to push arguments/return address on stack
    dwNewSP -= 4 * sizeof (DWORD);
#endif

    // commit user stack if required. 
    if (!VMProcessPageFault (pVMProc, dwNewSP, TRUE)) {
        SetStackOverflow (pExr);
    } else {
        CheckCommitStack (pExr, dwNewSP, USER_MODE);
    } 

    __try {
        if (dwNewSP < (pCurThread->tlsNonSecure[PRETLS_STACKBASE] + VM_PAGE_SIZE)) {
            // fatal stack overflow
            SetStackOverflow (pExr);
            fRet = FALSE;
        } else {
            memcpy (pUsrCtx, pCtx, sizeof (CONTEXT));                           // copy context
            memcpy (pUsrExr, pExr, sizeof (EXCEPTION_RECORD));                  // copy exception record

#ifdef x86

            ((LPDWORD) dwNewSP)[0] = 0;                                         // return address - any invalid value
            ((LPDWORD) dwNewSP)[1] = (DWORD)pUsrExr;                            // arg0 - pExr
            ((LPDWORD) dwNewSP)[2] = (DWORD)pUsrCtx;                            // arg1 - pCtx
            ((LPDWORD) dwNewSP)[3] = (DWORD)pCurThread->pcstkTop;               // arg2 - pcstk

#else
            CONTEXT_TO_PARAM_1(pCtx) = (REGTYPE) pUsrExr;
            CONTEXT_TO_PARAM_2(pCtx) = (REGTYPE) pUsrCtx;
            CONTEXT_TO_PARAM_3(pCtx) = (REGTYPE) pCurThread->pcstkTop;
#endif
            CONTEXT_TO_STACK_POINTER (pCtx) = dwNewSP;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // bad user stack, take care of it
        SetStackOverflow (pExr);
        fRet = FALSE;
    }

    DEBUGMSG (ZONE_SEH, (L"Stack Switched, pUsrCtx = %8.8lx pUsrExr = %8.8lx NewSP = %8.8lx, fRet = %d\r\n",
                    pUsrCtx, pUsrExr, dwNewSP, fRet));
    return fRet;
}


DWORD ResetThreadStack (PTHREAD pth)
{
    DWORD dwNewSP;

    pth->tlsNonSecure = TLSPTR (pth->dwOrigBase, pth->dwOrigStkSize);
    dwNewSP = (DWORD) pth->tlsNonSecure - SECURESTK_RESERVE - (VM_PAGE_SIZE >> 1);
    pth->tlsNonSecure[PRETLS_STACKBASE] = pth->dwOrigBase;
    pth->tlsNonSecure[PRETLS_STACKSIZE] = pth->dwOrigStkSize;
    pth->tlsNonSecure[PRETLS_STACKBOUND] = dwNewSP & ~VM_PAGE_OFST_MASK;
    return dwNewSP;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void KPSLExceptionRoutine (DWORD ExceptionFlags, DWORD dwPC)
/*++
Routine Description:
    This function is invoked when an exception occurs in a kernel PSL call which is
    not caught by specific handler. It simply forces an error return from the api
    call.

Arguments:
    ExceptionFlag - specify excepiton flag.

    dwPC - the PC where exception occurs

Return Value:
    An exception disposition value of execute handler is returned.
--*/
{
    extern void SurrenderCritSecs(void);
    RETAILMSG(1, (TEXT("KPSLExceptionHandler: flags=%x ControlPc=%8.8lx\r\n"),
            ExceptionFlags, dwPC));

    SurrenderCritSecs();
}

//------------------------------------------------------------------------------
// Get a copy of the current callstack.  Returns the number of frames if the call 
// succeeded and 0 if it failed (if the given buffer is not large enough to receive
// the entire callstack, and STACKSNAP_FAIL_IF_INCOMPLETE is specified in the flags).
// 
// If STACKSNAP_FAIL_IF_INCOMPLETE is not specified, the call will always fill in
// upto the number of nMax frames.
//------------------------------------------------------------------------------

extern BOOL CanTakeCS (LPCRITICAL_SECTION lpcs);

//
// Try taking ModListcs, return TRUE if we get it. FALSE otherwise
//
BOOL TryLockModuleList ()
{
    extern CRITICAL_SECTION ModListcs;
    volatile CRITICAL_SECTION *vpcs = &ModListcs;
    BOOL fRet = TRUE;

    // if we can take ModListcs right away, take it.
    if ((DWORD) vpcs->OwnerThread == dwCurThId) {
        vpcs->LockCount++; /* We are the owner - increment count */

    } else if (InterlockedTestExchange((LPLONG)&vpcs->OwnerThread,0,dwCurThId) == 0) {
        vpcs->LockCount = 1;

    // ModListcs is owned by other thread, block waiting for it only if
    // CS order isn't violated.
    } else if (fRet = (!InSysCall () && CanTakeCS (&ModListcs))) {
        EnterCriticalSection (&ModListcs);
    }
    return fRet;
}

#ifdef x86
// Determines if PC is in prolog
static DWORD IsPCInProlog (DWORD dwPC, BOOL fInKMode)
{
    const BYTE bPrologx86[] = {0x55, 0x8b, 0xec};
    DWORD dwRetVal = 0; // Default to not in prolog

    // if we're not in KMode, validate PC being a user address before reading.
    if (fInKMode || IsValidUsrPtr ((LPVOID) dwPC, sizeof (DWORD), FALSE)) {
        __try {
            if (!memcmp((void*)dwPC, bPrologx86, sizeof(bPrologx86))) { 
                // PC on first instruction (55, push ebp)
                dwRetVal = 1;
            } else if (!memcmp((void*)(dwPC-1), bPrologx86, sizeof(bPrologx86))) { 
                // PC on second instruction (8b ec, mov ebp esp)
                dwRetVal = 2;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwRetVal = 0; // Assume non-Prolog if we blew up trying on access
        }    
    }
    return dwRetVal;
}
#else
__inline BOOL IsAddrInModule (const e32_lite *eptr, DWORD dwAddr)
{
    return (DWORD) (dwAddr - eptr->e32_vbase) < eptr->e32_vsize;
}


BOOL FindPEHeader (PPROCESS pprc, DWORD dwAddr, e32_lite *eptr)
{
    BOOL fRet = FALSE;
    if (dwAddr - (DWORD) pprc->BasePtr < pprc->e32.e32_vsize) {
        if (fRet = CeSafeCopyMemory (eptr, &pprc->e32, sizeof (e32_lite))) {
            eptr->e32_vbase = (DWORD) pprc->BasePtr;
        }
        
    } else if (TryLockModuleList ()) {
    
        PMODULE pMod = FindModInProcByAddr (pprc, dwAddr);

        if (pMod) {
            if (fRet = CeSafeCopyMemory (eptr, &pMod->e32, sizeof (e32_lite))) {
                eptr->e32_vbase = (DWORD) pMod->BasePtr;
            }
        }
        
        UnlockModuleList ();
    }
    return fRet;
}
#endif

ULONG NKGetThreadCallStack (PTHREAD pth, ULONG dwMaxFrames, LPVOID lpFrames, DWORD dwFlags, DWORD dwSkip, PCONTEXT pCtx)
{
    ULONG       HighLimit;
    ULONG       LowLimit;
    ULONG       Cnt = 0;
    PPROCESS    pprcOldVM = pCurThread->pprcVM;
    PPROCESS    pprcVM = pVMProc;
    PPROCESS    pprc   = pth->pprcActv;
    PCALLSTACK  pcstk  = pth->pcstkTop;
    LPDWORD     tlsPtr = pth->tlsPtr;
    PPROCESS    pprcOldActv = SwitchActiveProcess (g_pprcNK);
    DWORD       err = 0;
    DWORD       saveKrn;
    LPVOID      pCurFrame = lpFrames;
    ULONG       Frames;
    LPDWORD     pParams;
    DWORD       cbFrameSize = (STACKSNAP_NEW_VM & dwFlags)
                            ? sizeof (CallSnapshot3)
                            : ((STACKSNAP_EXTENDED_INFO & dwFlags)? sizeof (CallSnapshotEx) : sizeof (CallSnapshot));

    dwMaxFrames += dwSkip; // we'll subtract dwSkip when reference lpFrames

    DEBUGCHK (pCurThread->tlsPtr == pCurThread->tlsSecure);
    DEBUGCHK (IsKModeAddr ((DWORD) lpFrames));
    DEBUGCHK (IsKModeAddr ((DWORD) pCtx));
    DEBUGCHK (!pprcOldVM || (pprcOldVM == pprcVM));

    saveKrn = pCurThread->tlsPtr[TLSSLOT_KERNEL];
    pCurThread->tlsPtr[TLSSLOT_KERNEL] |= TLSKERN_NOFAULT | TLSKERN_NOFAULTMSG;

    // skip any exception callstack
    if (pcstk && (pcstk->dwPrcInfo & CST_EXCEPTION)) {
        // Fix up the actvproc
        pprc  = pcstk->pprcLast;

        //
        // switch tlsPtr
        //
        if (pcstk->dwPrcInfo & CST_MODE_FROM_USER) {
            tlsPtr = pcstk->pOldTls;
        } else {
            tlsPtr = pth->tlsSecure;
        }

        pcstk = pcstk->pcstkNext;
    }

    if (pCurThread != pth)
    {
        // Not the current thread, we need to switch VM.
        // OK because pprcOldVM is stores the current thread's old VM.
        SwitchVM (pth->pprcVM);
    }

    // get stack limits
    if (!GetStackLimitsFromTLS (tlsPtr, &LowLimit, &HighLimit)) {
        err = ERROR_INVALID_PARAMETER;
        
    } else {

        __try {
#ifdef x86

#define RTN_ADDR(x)                     (*(LPDWORD) ((x) + sizeof(DWORD)))
#define NEXT_FRAME(x)                   (*(LPDWORD) (x))
#define FRAMEPTR_TO_PARAM(fptr, idx)    (*(LPDWORD) ((fptr) + (2+(idx))*sizeof(DWORD)))

            DWORD RtnAddr  = pCtx->Eip;
            DWORD FramePtr = pCtx->Ebp;
            DWORD idxParam;

            DWORD dwSavedEBP = 0;
            DWORD dwInProlog = IsPCInProlog (RtnAddr, tlsPtr == pth->tlsSecure);
            if (1 == dwInProlog) {
                RtnAddr += 3; // Increment by 3 bytes to skip past end of prolog
                dwSavedEBP = pCtx->Ebp;
                FramePtr = pCtx->Esp - 4; // Since ESP hasn't been pushed yet
            } else if (2 == dwInProlog) {
                RtnAddr += 2; // Increment past end of prolog
                FramePtr = pCtx->Esp; // Get it directly since we've already done the push
            }

            while (TRUE) {

                if (dwMaxFrames <= Cnt) {
                    if (STACKSNAP_FAIL_IF_INCOMPLETE & dwFlags) {
                        err = ERROR_INSUFFICIENT_BUFFER;
                    }
                    break;
                }

                pParams = NULL;
                if (!(STACKSNAP_INPROC_ONLY & dwFlags) || (pprc == pth->pprcOwner)) {
                    if (Cnt >= dwSkip) {

                        // NOTE: dwReturnAddr is the 1st field on all snapshot format. So
                        //       we can safely do this.
                        ((CallSnapshot *) pCurFrame)->dwReturnAddr = RtnAddr;

                        if ((STACKSNAP_NEW_VM|STACKSNAP_EXTENDED_INFO) & dwFlags) {
                            
                            ((CallSnapshotEx *) pCurFrame)->dwFramePtr = FramePtr;
                            ((CallSnapshotEx *) pCurFrame)->dwCurProc  = pprc->dwId;

                            if (STACKSNAP_NEW_VM & dwFlags) {
                                ((CallSnapshot3 *) pCurFrame)->dwVMProc = pVMProc? pVMProc->dwId : 0;
                                pParams = ((CallSnapshot3 *) pCurFrame)->dwParams;
                            } else {
                                pParams = ((CallSnapshotEx *) pCurFrame)->dwParams;
                            }

                        }
                        
                        pCurFrame = (LPVOID) ((DWORD) pCurFrame + cbFrameSize);
                    }
                } else {
                    // For INPROC stacks, skip frames that are not in the thread's owner process
                    dwSkip++;
                    dwMaxFrames++;
                }

                Cnt++;

                if (   !IsInRange (FramePtr, LowLimit, HighLimit)
                    || !IsDwordAligned (FramePtr)) {
                    break;
                }

                // frame is valid, copy arguments from it
                if (pParams) {
                    for (idxParam = 0; idxParam < 4; idxParam ++) {
                        pParams[idxParam] = FRAMEPTR_TO_PARAM (FramePtr, idxParam);
                    }
                }
                
                RtnAddr = RTN_ADDR (FramePtr);
                if (!RtnAddr) {
                    break;
                }

                if ((SYSCALL_RETURN == RtnAddr)
                    || (DIRECT_RETURN  == RtnAddr)) {

                    if (!pcstk) {
                        err = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    
                    // across PSL boundary
                    RtnAddr = (DWORD) pcstk->retAddr;
                    FramePtr = pcstk->regs[REG_EBP];
                    pprc = pcstk->pprcLast;
                    SwitchVM (pcstk->pprcVM);
                
                    //
                    // switch tlsPtr
                    //
                    if (pcstk->dwPrcInfo & CST_MODE_FROM_USER) {
                        tlsPtr = pcstk->pOldTls;
                    } else {
                        tlsPtr = pth->tlsSecure;
                    }

                    if (!GetStackLimitsFromTLS (tlsPtr, &LowLimit, &HighLimit)) {
                        break;
                    }

                    pcstk = pcstk->pcstkNext;

                } else if (dwSavedEBP) {
                    FramePtr = dwSavedEBP;
                    dwSavedEBP = 0;
                } else {
                    FramePtr = NEXT_FRAME(FramePtr);
                }

            }

#else

            ULONG    ControlPc = (ULONG) CONTEXT_TO_PROGRAM_COUNTER (pCtx);
            e32_lite e32 = {0};

            // Start with the frame specified by the context record and search
            // backwards through the call frame hierarchy.
            do { 

                if (dwMaxFrames <= Cnt) {
                    if (STACKSNAP_FAIL_IF_INCOMPLETE & dwFlags) {
                        err = ERROR_INSUFFICIENT_BUFFER;
                    }
                    break;
                }

                pParams = NULL;
                if (!(STACKSNAP_INPROC_ONLY & dwFlags) || (pprc == pth->pprcOwner)) {
                    if (Cnt >= dwSkip) {



                        // NOTE: dwReturnAddr is the 1st field on all snapshot format. So
                        //       we can safely do this.
                        ((CallSnapshot *) pCurFrame)->dwReturnAddr = ControlPc;

                        if ((STACKSNAP_NEW_VM|STACKSNAP_EXTENDED_INFO) & dwFlags) {
                            
                            ((CallSnapshotEx *) pCurFrame)->dwFramePtr = (ULONG) pCtx->IntSp;
                            ((CallSnapshotEx *) pCurFrame)->dwCurProc  = pprc->dwId;

                            if (STACKSNAP_NEW_VM & dwFlags) {
                                ((CallSnapshot3 *) pCurFrame)->dwVMProc = pVMProc? pVMProc->dwId : 0;
                                pParams = ((CallSnapshot3 *) pCurFrame)->dwParams;
                            } else {
                                pParams = ((CallSnapshotEx *) pCurFrame)->dwParams;
                            }

                        }
                        
                        pCurFrame = (LPVOID) ((DWORD) pCurFrame + cbFrameSize);


                    }
                } else {
                    // For INPROC stacks, skip frames that are not in the thread's owner process
                    dwSkip++;
                    dwMaxFrames++;
                }

                if (IsAddrInModule (&e32, ControlPc)
                    || FindPEHeader (IsKModeAddr (ControlPc)? g_pprcNK : pprc, ControlPc, &e32)) {
                    ControlPc = (* g_pfnRtlUnwindOneFrame) (ControlPc,
                                                    pCtx,
                                                    &e32);
                } else {
                    // can't find function pdata, assume LEAF function
                    ControlPc = 0;
                }


                if (!ControlPc) {
                    // unwind to a NULL address, assume end of callstack
                    if (Cnt) {
                        break;
                    }
                    ControlPc = (ULONG) pCtx->IntRa - INST_SIZE;
                }


                if (pParams) {
                    pParams[0] = (DWORD) CONTEXT_TO_PARAM_1(pCtx);
                    pParams[1] = (DWORD) CONTEXT_TO_PARAM_2(pCtx);
                    pParams[2] = (DWORD) CONTEXT_TO_PARAM_3(pCtx);
                    pParams[3] = (DWORD) CONTEXT_TO_PARAM_4(pCtx);
                }

                Cnt++;

                // Set point at which control left the previous routine.
                if (IsPSLBoundary (ControlPc)) {
                    if (!pcstk) {
                        break;
                    }
                    
                    // across PSL boundary
                    MDRestoreCalleeSavedRegisters (pcstk, pCtx);
                    
                    pCtx->IntSp = pcstk->dwPrevSP;
                    pCtx->IntRa = (ULONG)pcstk->retAddr;
                    ControlPc = (ULONG) pCtx->IntRa - INST_SIZE;
                    
                    pprc = pcstk->pprcLast;
                    SwitchVM (pcstk->pprcVM);
                
                    //
                    // switch tlsPtr
                    //
                    if (pcstk->dwPrcInfo & CST_MODE_FROM_USER) {
                        tlsPtr = pcstk->pOldTls;
                    } else {
                        tlsPtr = pth->tlsSecure;
                    }

                    // invalid stack
                    if (!GetStackLimitsFromTLS (tlsPtr, &LowLimit, &HighLimit)) {
                        break;
                    }

                    pcstk = pcstk->pcstkNext;
                }
            
            } while (((ULONG) pCtx->IntSp < HighLimit) && ((ULONG) pCtx->IntSp > LowLimit));

#endif
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_INVALID_PARAMETER;
        }
    }

    SwitchActiveProcess (pprcOldActv);
    SwitchVM (pprcVM);
    pCurThread->pprcVM = pprcOldVM;
    pCurThread->tlsPtr[TLSSLOT_KERNEL] = saveKrn;

    if (Cnt < dwSkip) {
        err = ERROR_INVALID_PARAMETER;
        Frames = 0;
    } else {
        Frames = Cnt - dwSkip;
    }

    if ((err) || (STACKSNAP_RETURN_FRAMES_ON_ERROR & dwFlags)) {
        KSetLastError (pCurThread, err);
        if (!(STACKSNAP_RETURN_FRAMES_ON_ERROR & dwFlags))
        {
            Frames = 0;
        }
    }

    return Frames;
}

//
// Find find module and offset given a PC
//
LPCWSTR FindModuleNameAndOffset (DWORD dwPC, LPDWORD pdwOffset)
{
    LPCWSTR pszName = L"???";
    DWORD   dwBase  = 0;

    if ((dwPC - (DWORD) pActvProc->BasePtr) < pActvProc->e32.e32_vsize) {
        pszName = pActvProc->lpszProcName;
        dwBase  = (DWORD) pActvProc->BasePtr;
        
    } else if (TryLockModuleList ()) {
        PMODULE pMod = FindModInProcByAddr (IsKModeAddr (dwPC)? g_pprcNK : pActvProc, dwPC);

        if (pMod) {
            pszName = pMod->lpszModName;
            dwBase  = (DWORD) pMod->BasePtr;
        }
        
        UnlockModuleList ();
    }

    *pdwOffset = dwPC - dwBase;

    return pszName;
}



static BOOL IsBreakPoint (DWORD dwExcpCode)
{
    return (STATUS_BREAKPOINT == dwExcpCode)
        || (STATUS_SINGLE_STEP == dwExcpCode);
}


static DWORD SetupExceptionTerminate (PTHREAD pth, PEXCEPTION_RECORD pExr, PCONTEXT pCtx)
{
    DWORD mode = (pth->pprcOwner == g_pprcNK)? KERNEL_MODE : USER_MODE;
    CleanupAllCallstacks (pth);
    SET_DEAD (pth);
    CLEAR_USERBLOCK (pth);
    CLEAR_DEBUGWAIT (pth);
    pth->pprcActv = pth->pprcVM = pth->pprcOwner;
    KCall ((PKFN) SetCPUASID, pth);
    if (pExr->ExceptionCode == STATUS_STACK_OVERFLOW) {
        // Make sure we have enough room to run ExcpExitThread
        CONTEXT_TO_STACK_POINTER(pCtx) = ResetThreadStack (pth);
    }
    
    CONTEXT_TO_PROGRAM_COUNTER (pCtx) = (KERNEL_MODE == mode)? (ULONG) pKrnExcpExitThread : (ULONG) pUsrExcpExitThread;
    CONTEXT_TO_PARAM_1 (pCtx) = pExr->ExceptionCode;            // argument, exception code
    CONTEXT_TO_PARAM_2 (pCtx) = (DWORD) pExr->ExceptionAddress; // argument, exception address

    SetContextMode (pCtx, mode);

#ifdef ARM
    // run in ARM mode
    pCtx->Psr &= ~THUMB_STATE;
#endif

    return mode;
}

#define EXCEPTION_ID_RAISED_EXCEPTION               ((DWORD) -1)
#define EXCEPTION_ID_SECURE_STACK_OVERFLOW          ((DWORD) -2)
#define EXCEPTION_ID_USER_STACK_OVERFLOW            ((DWORD) -3)

#define EXCEPTION_STRING_RAISED_EXCEPTION           "Raised Exception"
#define EXCEPTION_STRING_SECURE_STACK_OVERFLOW      "Secure Stack Overflow"
#define EXCEPTION_STRING_USER_STACK_OVERFLOW        "User Stack Overflow"
#define EXCEPTION_STRING_UNKNOWN                    "Unknown Exception"

const LPCSTR ppszOsExcp [] = {
        EXCEPTION_STRING_USER_STACK_OVERFLOW,          // ID == -3
        EXCEPTION_STRING_SECURE_STACK_OVERFLOW,        // ID == -2
        EXCEPTION_STRING_RAISED_EXCEPTION,             // ID == -1
};
const DWORD nOsExcps = sizeof(ppszOsExcp)/sizeof(ppszOsExcp[0]);


extern const LPCSTR g_ppszMDExcpId [MD_MAX_EXCP_ID+1];

LPCSTR GetExceptionString (DWORD dwExcpId)
{
    LPCSTR pszExcpStr;
    if (dwExcpId <= MD_MAX_EXCP_ID) {
        pszExcpStr = g_ppszMDExcpId[dwExcpId];
        
    } else {
        dwExcpId += nOsExcps;
        if (dwExcpId < nOsExcps) {
            // original id is -1, -2, or -3
            pszExcpStr = ppszOsExcp[dwExcpId];
            
        } else {
            pszExcpStr = EXCEPTION_STRING_UNKNOWN;
        }
    }

    return pszExcpStr;
}

#ifdef x86
DWORD SafeGetReturnAddress (const CONTEXT *pCtx)
{
    LPDWORD pFrame = (LPDWORD) (pCtx->Ebp + 4);
    DWORD   dwRa = 0;
    if (   IsDwordAligned (pFrame)
        && ((DWORD)pFrame > pCtx->Esp)
        && ((KERNEL_MODE == GetContextMode (pCtx)) || IsValidUsrPtr (pFrame, sizeof(DWORD), TRUE))) {
        DWORD saveKrn = pCurThread->tlsPtr[TLSSLOT_KERNEL];
        pCurThread->tlsPtr[TLSSLOT_KERNEL] |= TLSKERN_NOFAULT | TLSKERN_NOFAULTMSG;
        CeSafeCopyMemory (&dwRa, pFrame, sizeof (DWORD));
        pCurThread->tlsPtr[TLSSLOT_KERNEL] = saveKrn;
    }
    return dwRa;
}

#else
#define SafeGetReturnAddress(pCtx)      CONTEXT_TO_RETURN_ADDRESS(pCtx)
#endif


//
// print exception message
//
static void PrintException (DWORD dwExcpId, PEXCEPTION_RECORD pExr, PCONTEXT pCtx)
{
    LPCWSTR pszNamePC, pszNameRA;
    DWORD dwPC     = CONTEXT_TO_PROGRAM_COUNTER(pCtx);
    DWORD dwOfstPC = dwPC;
    DWORD dwRA     = SafeGetReturnAddress (pCtx);
    DWORD dwOfstRA = dwRA;
    pszNamePC = FindModuleNameAndOffset (dwOfstPC, &dwOfstPC);
    pszNameRA = FindModuleNameAndOffset (dwOfstRA, &dwOfstRA);
    
    NKDbgPrintfW(L"Exception '%a' (%d): Thread-Id=%8.8lx(pth=%8.8lx), Proc-Id=%8.8lx(pprc=%8.8lx) '%s', VM-active=%8.8lx(pprc=%8.8lx) '%s'\r\n",
            GetExceptionString (dwExcpId), dwExcpId, 
            dwCurThId, pCurThread,
            pActvProc->dwId, pActvProc, SafeGetProcName (pActvProc),
            pVMProc->dwId,   pVMProc,   SafeGetProcName (pVMProc)
            );
    NKDbgPrintfW(L"PC=%8.8lx(%s+0x%8.8lx) RA=%8.8lx(%s+0x%8.8lx) SP=%8.8lx, BVA=%8.8lx\r\n", 
            dwPC, pszNamePC, dwOfstPC, 
            dwRA, pszNameRA, dwOfstRA,
            CONTEXT_TO_STACK_POINTER (pCtx),
            pExr->ExceptionInformation[1]);

}


BOOL DelaySuspendOrTerminate (void);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
ExceptionDispatch(
    PEXCEPTION_RECORD pExr,
    PCONTEXT pCtx
    ) 
{
    PTHREAD pth = pCurThread;
    PCALLSTACK pcstk = pth->pcstkTop;
    PCALLSTACK pcstkNext = pcstk->pcstkNext;
    DWORD mode = (pcstk->dwPrcInfo & CST_MODE_FROM_USER)? USER_MODE : KERNEL_MODE;
    BOOL fHandled = FALSE;
    BOOL fRunSEH = TRUE;
    BOOL f2ndChance = FALSE;
    DWORD dwExcpId = EXCEPTION_ID_RAISED_EXCEPTION;
    LPDWORD lptls = (KERNEL_MODE != mode) ? pth->tlsNonSecure : pth->tlsPtr;
    PEXCEPTION_RECORD pExrOrig;
    PCONTEXT pCtxOrig;

    DEBUGMSG (ZONE_SEH, (L"", MDDbgPrintExceptionInfo (pExr, pcstk)));
    
    memset(pExr, 0, sizeof(EXCEPTION_RECORD));
    DEBUGCHK (pth->tlsPtr == pth->tlsSecure);

    // Update CONTEXT with infomation saved in the EXCINFO structure
    CONTEXT_TO_PROGRAM_COUNTER (pCtx) = (DWORD) pcstk->retAddr;
    CONTEXT_TO_STACK_POINTER (pCtx) = pcstk->dwPrevSP;
    pExr->ExceptionAddress = pcstk->retAddr;

#ifdef ARM
    // we always build with THUMB support. No longer "ifdef THUMBSUPPORT" here
    if (CONTEXT_TO_PROGRAM_COUNTER (pCtx) & 1) {
        pCtx->Psr |= THUMB_STATE;
    }
#endif

    // Check for RaiseException call versus a CPU detected exception.
    // RaiseException just becomes a call to CaptureContext as a KPSL.
    //
    // HandleExcepion leave pprcLast field to be NULL.
    //
    if (pcstk->pprcLast) {

        // From RaiseException. The stack looks like this. The only difference between
        // a trusted/untrusted caller is that the stack is switched
        //
        //      |                       |
        //      |                       |
        //      -------------------------
        //      |                       |<---- pCtx
        //      |                       |
        //      |                       |
        //      -------------------------
        //      |                       |<---- pExr
        //      |                       |
        //      |    context record     |
        //      |                       |
        //      |                       |
        //      -------------------------
        //      | RA (x86 only) and     |<----- (pExr+1)
        //      |args of RaiseExcepiton |
        //      |                       |
        //      -------------------------
        
        // Fill in exception record information from the parameters passed to
        // the RaiseException call.
        LPDWORD pArgs = MDGetRaisedExceptionInfo (pExr, pCtx, pcstk);

#ifdef x86        
        // Remove return address from the stack
        pCtx->Esp += 4;

        // x86 restore exception pointer
        TLS2PCR (lptls)->ExceptionList = pcstk->regs[REG_EXCPLIST];

#endif
        // The callee-saved registers are saved inside callstack structure. CaptureContext gets 
        // the register contents right after the ObjectCall. We need to restore the callee saved
        // registers or the context will not be correct.
        MDRestoreCalleeSavedRegisters (pcstk, pCtx);
                
        //
        // 2nd chance exception report always give 0 as arg count and lpArguments as ptr to original context
        //
        f2ndChance = (EXCEPTION_FLAG_UNHANDLED == pExr->ExceptionFlags)
                && !pExr->NumberParameters
                && pArgs;

        if (f2ndChance) {
            
            // 2nd chance exception:
            DEBUGMSG (ZONE_SEH, (L"2nd chance Exception, pcstkNext = %8.8lx\r\n", pcstkNext));
            
            pExr->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        
            // if we're in callback/API, report the exeption to the caller, as if the exception occurs
            // right at the point of the API call.
            if (pcstkNext) {
                // Since the exception was unhandled within the current PSL context, we need to assume
                // that a Watson dump is necessary.  Invoke Watson now before any stack unwinding occurs.
                // If Watson was previously invoked on this exception, it will ignore the redundant call.
                // Unfortunately, we can't pass the exr/ctx at hand to Watson because they represent the
                // raise of the unhandled exception, not the original exception itself.  So we will pick
                // apart the stack frame at the time of the call to RaiseException to find the original
                // exr/ctx.
                pCtxOrig = (PCONTEXT)pArgs;
                pExrOrig = (PEXCEPTION_RECORD)(pCtxOrig + 1);
                HDException(pExrOrig, pCtxOrig, HD_EXCEPTION_PHASE_DUMPONLY);

                fHandled = TRUE;
                
                //
                // What we need to do here is to pop off the last callstack, restore all
                // callee saved registers and redo SEH handling as if the exception happens
                // right after returning form PerformCallback
                DEBUGMSG (ZONE_SEH, (L"callstack not empty, walk the callstack, pcstkNext = %8.8lx\r\n", pcstkNext));
        
                pth->pcstkTop = pcstk = pcstkNext;
                pcstkNext = pcstk->pcstkNext;

                CONTEXT_TO_PROGRAM_COUNTER (pCtx) = (DWORD) pcstk->retAddr - INST_SIZE;
                CONTEXT_TO_STACK_POINTER (pCtx)   = pcstk->dwPrevSP;

                MDRestoreCalleeSavedRegisters (pcstk, pCtx);

                // perform kernel clean up if faulted inside kernel
                if (!(CST_UMODE_SERVER & pcstk->dwPrcInfo)) {
                    KPSLExceptionRoutine (pExr->ExceptionFlags, CONTEXT_TO_PROGRAM_COUNTER(pCtx));
                }
                
                UnwindOneCstk (pcstk, TRUE);

                if (pcstk->dwPrcInfo) {
                    mode = USER_MODE;
                    // clear volatile registers if returning to user mode? might disclose information if we don't
                    MDClearVolatileRegs (pCtx);
                } else {
                    mode = KERNEL_MODE;
                }

            }
            fRunSEH = fHandled;     // unhandled 2nd chance exception resumes at pExcpExitThread
                                    // while 'handled' 2nd chance exception require SEH dispatch
                                    // again in PSL's context

            if (!fHandled) {
#ifdef MIPS
                // MIPS had 4 register save area defined in CONTEXT structure!!!
                DWORD dwOfst = 4 * REG_SIZE;
#else
                DWORD dwOfst = 0;
#endif
                // add CST_EXCEPTION flag for exception unwinding.
                pcstk->dwPrcInfo |= CST_EXCEPTION;

                // prepare context for debugger
                CeSafeCopyMemory ((LPBYTE)pCtx+dwOfst, (LPBYTE) pArgs+dwOfst, sizeof (CONTEXT)-dwOfst);
                pExr->ExceptionAddress = (PVOID) CONTEXT_TO_PROGRAM_COUNTER (pCtx);
                DEBUGMSG (ZONE_SEH, (L"Unhandled Exception, original (IP = %8.8lx, SP = %8.8lx)\r\n",
                    CONTEXT_TO_PROGRAM_COUNTER(pCtx), CONTEXT_TO_STACK_POINTER (pCtx)));
            }

        }
    } else {

        // hardware exception

        pcstk->pprcLast = pActvProc;   // fix callstack to make it normal

        if (TEST_STACKFAULT (pth)) {
            // secure stack overflow (non-fatal)
            dwExcpId = EXCEPTION_ID_SECURE_STACK_OVERFLOW;
            pExr->ExceptionCode  = STATUS_STACK_OVERFLOW;
            pExr->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            fHandled = FALSE;
            CLEAR_STACKFAULT (pth);
        } else {
            fHandled = MDHandleHardwareException (pcstk, pExr, pCtx, &dwExcpId);
        }

        if (fHandled                                                                          // handled
            && MDIsPageFault (dwExcpId)                                                       // page fault
            && !(fHandled = CheckCommitStack (pExr, pExr->ExceptionInformation[1], mode))) {  // is stack overflow

            dwExcpId = EXCEPTION_ID_USER_STACK_OVERFLOW;

            if (IsLastStackPage (pExr->ExceptionInformation[1], mode)){                        // using the last page

                // Last page of the stack committed - don't run SEH in this case for we don't have any protection anymore.
                NKDbgPrintfW (L"!!! Committed last page of the stack (0x%8.8lx), SEH bypassed, thread terminated !!!\r\n",
                        pExr->ExceptionInformation[1]);
                PrintException (dwExcpId, pExr, pCtx);
                fRunSEH = FALSE;
                f2ndChance = TRUE;
            }
        } else {
            fRunSEH = !fHandled;
        }
    }

    DEBUGMSG (ZONE_SEH, (L"fHandled = %d, fRunSEH = %d, f2ndChance = %d, mode = 0x%x, SP = %8.8lx\r\n",
            fHandled, fRunSEH, f2ndChance, mode, CONTEXT_TO_STACK_POINTER (pCtx)));
    
    // update context mode
    SetContextMode (pCtx, mode);

    // invoke Watson on 2nd chance exception
    if (f2ndChance) {
        // TBD - invoke watson

    // print exception info on 1st chance excepiton
    } else if (!fHandled
        && !IsBreakPoint (pExr->ExceptionCode)
        && !IsNoFaultMsgSet (lptls)) {
        PrintException (dwExcpId, pExr, pCtx);
    }

    // special handling for break point (never call SEH, do 1st and 2nd in a row, always resume)
    if (IsBreakPoint (pExr->ExceptionCode)) {
        
        BOOL fSkipUserDbgTrap = FALSE;
        fRunSEH = FALSE;

        if (pvHDNotifyExdi
            && (CONTEXT_TO_PROGRAM_COUNTER(pCtx) >= (DWORD)pvHDNotifyExdi)
            && (CONTEXT_TO_PROGRAM_COUNTER(pCtx) <= ((DWORD)pvHDNotifyExdi + HD_NOTIFY_MARGIN))) {
            fSkipUserDbgTrap = TRUE;
        }

        if ((fSkipUserDbgTrap || !UserDbgTrap (pExr, pCtx, FALSE))        // user 1st chance
            && !HDException(pExr, pCtx, HD_EXCEPTION_PHASE_FIRSTCHANCE)      // KD 1st chance
            && (fSkipUserDbgTrap || !UserDbgTrap (pExr, pCtx, TRUE))      // user 2nd chance
            && !HDException(pExr, pCtx, HD_EXCEPTION_PHASE_SECONDCHANCE)) {    // KD 2nd chance
            
            if (!fSkipUserDbgTrap) {
                RETAILMSG(1, (TEXT("DEBUG_BREAK @%8.8lx Ignored.\r\n"), CONTEXT_TO_PROGRAM_COUNTER(pCtx)));
            }
            MDSkipBreakPoint (pExr, pCtx);
        }
        
    // normal exception - notify debugger, kill thread on 2nd chance exception (not continue-able)
    } else if (!fHandled) {
    
        if (UserDbgTrap (pExr, pCtx, f2ndChance) || (!IsNoFaultSet (lptls) &&
            HDException(pExr, pCtx, f2ndChance ? HD_EXCEPTION_PHASE_SECONDCHANCE : HD_EXCEPTION_PHASE_FIRSTCHANCE))) {
            // debugger handled the exception
            fRunSEH = f2ndChance = FALSE;
        }
        if (f2ndChance) {
            DEBUGCHK (!fRunSEH);
            // Unhandled exception, terminate the process.
            RETAILMSG(1, (TEXT("\r\nUnhandled exception %8.8lx:\r\n"), pExr->ExceptionCode));

            if (InSysCall()) {
                MDDumpFrame (pth, dwExcpId, pExr, pCtx, 0);
                OEMIoControl (IOCTL_HAL_HALT, NULL, 0, NULL, 0, NULL);

            } else if (!GET_DEAD(pth)) {
                mode = SetupExceptionTerminate (pth, pExr, pCtx);
                
                RETAILMSG(1, (TEXT("Terminating thread %8.8lx\r\n"), pth));
                
            } else {
                MDDumpFrame(pth, dwExcpId, pExr, pCtx, 0);
                RETAILMSG(1, (TEXT("Can't terminate thread %8.8lx, sleeping forever\r\n"), pth));
                SurrenderCritSecs();
                Sleep(INFINITE);
                DEBUGCHK(0);    // should never get here
            }
        }
    }
    
    // copy context to non-secure stack if stack switched, and update TLS
    if (KERNEL_MODE != mode) {

        if (!IsValidUsrPtr ((LPVOID) CONTEXT_TO_STACK_POINTER (pCtx), REGSIZE, TRUE)
            || (fRunSEH && !SetupContextOnUserStack (pExr, pCtx))) {
            // bad stack pointer, or fatal stack error. Don't bother run SEH, kill the thread on the spot
            DEBUGMSG (1, (L"Fatal stack error (overflow), thread terminated (sp = %8.8lx)\r\n", CONTEXT_TO_STACK_POINTER (pCtx)));
            fRunSEH = FALSE;
            pExr->ExceptionCode = STATUS_STACK_OVERFLOW;
            mode = SetupExceptionTerminate (pth, pExr, pCtx);
            
        } else if ((pActvProc == pth->pprcOwner) && DelaySuspendOrTerminate ()) {

            // being terminated.
            CONTEXT_TO_PROGRAM_COUNTER (pCtx) = (LONG) pExitThread;
        }

        
        // TLS must be updated before updating fs base. For any context switch
        // will cause fs to reload from TLS
        KPlpvTls = pth->tlsPtr = pth->tlsNonSecure;
#ifdef x86
        UpdateRegistrationPtr (TLS2PCR (pth->tlsPtr));
#endif
    }

    // pop off the Exception callstack. If pcstkTop is NULL, it's being terminated for exception and
    // callstack is cleaned-up.
    if (pth->pcstkTop) {
        pth->pcstkTop = pcstkNext;
    }
    

    // If returning from handling a stack overflow, reset the thread's stack overflow
    // flag. It would be good to free the tail of the stack at this time
    // so that the thread will stack fault again if the stack gets too big. But we
    // are currently using that stack page.
    if (pExr->ExceptionCode == STATUS_STACK_OVERFLOW) {
        CLEAR_STACKFAULT (pth);
    }

    SetContextMode (pCtx, mode);
    
#ifdef ARM
    // THUMB specific
    if (!fRunSEH && (CONTEXT_TO_PROGRAM_COUNTER (pCtx) & 1)) {
        pCtx->Psr |= THUMB_STATE;
    }
#endif

    return fRunSEH;


}

static BOOL IsPcAtCaptureContext (DWORD dwPC)
{
#ifdef x86
    return dwPC == (DWORD)CaptureContext;
#else
    return dwPC == ((DWORD)CaptureContext+4);
#endif
}

BOOL KC_CommonHandleException (PTHREAD pth, DWORD arg1, DWORD arg2, DWORD arg3)
{
    PCALLSTACK pcstk;
    DWORD dwThrdPC = THRD_CTX_TO_PC (pth);
    DWORD dwThrdSP = (DWORD) THRD_CTX_TO_SP (pth);
    DWORD dwMode = GetThreadMode (pth);
    DWORD dwSpaceLeft;
    DWORD dwRslt = DCMT_OLD;

    KCALLPROFON(65);

    pcstk = ReserveCSTK (dwThrdSP, dwMode);

    if (!IsInKVM ((DWORD) pcstk)) {
        RETAILMSG (1, (L"\r\nFaulted in KCall, pCurThread->dwStartAddr = %8.8lx, PageFreeCount = %8.8lx!!\r\n",
            pCurThread->dwStartAddr, PageFreeCount));
        RETAILMSG (1, (L"Original Context when thread faulted:\r\n"));
        MDDumpThreadContext (pCurThread, arg1, arg2, arg3, 0);
        RETAILMSG (1, (L"Context when faulted in KCall:\r\n"));
        MDDumpThreadContext (pth, arg1, arg2, arg3, 0);
    }
    DEBUGMSG (ZONE_SCHEDULE, (L"+HandleException: pcstk = %8.8lx\r\n", pcstk));

    if (((DWORD) pcstk & ~VM_PAGE_OFST_MASK) != ((DWORD) (pcstk+1) & ~VM_PAGE_OFST_MASK)) {
        // pcstk cross page boundary, commit the next page
        dwRslt = KC_VMDemandCommit ((DWORD) (pcstk+1));
    }

    if (DCMT_FAILED != dwRslt) {
        dwRslt = KC_VMDemandCommit ((DWORD) pcstk);
    }
    
    // before we touch pcstk, we need to commit stack or we'll fault while
    // accessing it.
    switch (dwRslt) {
    case DCMT_OLD:
        // already commited. continue exception handling
        break;

    case DCMT_NEW:
        // commited a new page. check if we hit the last page.
        // Generate fatal stack overflow exception if yes (won't get into exception handler).

        if (pCurThread->tlsSecure[PRETLS_STACKBOUND] <= (DWORD)pcstk) {
            // within bound, restart the instruction
            KCALLPROFOFF(65);
            DEBUGMSG (ZONE_SCHEDULE, (L"-HandleException: committed secure stack page %8.8lx\r\n", (DWORD)pcstk & ~VM_PAGE_OFST_MASK));
            return TRUE; // restart instruction
        }

        // update stack bound
        pCurThread->tlsSecure[PRETLS_STACKBOUND] = (DWORD)pcstk & ~VM_PAGE_OFST_MASK;

        // calculate room left
        dwSpaceLeft = pCurThread->tlsSecure[PRETLS_STACKBOUND] - pCurThread->tlsSecure[PRETLS_STACKBASE];

        if (dwSpaceLeft >= MIN_STACK_RESERVE) {
            // within bound, restart the instruction
            KCALLPROFOFF(65);
            DEBUGMSG (ZONE_SCHEDULE, (L"-HandleException: committed secure stack page %8.8lx\r\n", (DWORD)pcstk & ~VM_PAGE_OFST_MASK));
            return TRUE; // restart instruction
        }

        // normal stack overflow exception if still room left
        if (dwSpaceLeft >= VM_PAGE_SIZE) {
            SET_STACKFAULT (pCurThread);
            break;
        }

        // fall through for fatal stack error
        __fallthrough;
        
    default:
    
        // fatal stack error
        KPlpvTls = pCurThread->tlsPtr = pCurThread->tlsSecure;
        THRD_CTX_TO_SP (pth) = (DWORD) (pcstk+1);
        THRD_CTX_TO_PARAM_1 (pth) = STATUS_STACK_OVERFLOW;
        THRD_CTX_TO_PARAM_2 (pth) = dwThrdPC;

        // NOTE: fatal stack error can lead to handle leak
        pCurThread->pcstkTop = NULL;        // have to discard all the callstack for they're no longer valid
        THRD_CTX_TO_PC (pth) = (DWORD) NKExitThread;

        SET_DEAD (pCurThread);
        CLEAR_USERBLOCK (pCurThread);
        CLEAR_DEBUGWAIT (pCurThread);
        pCurThread->pprcActv = pCurThread->pprcVM = pCurThread->pprcOwner;
        SetCPUASID (pCurThread);

        NKDbgPrintfW  (L"!FATAL ERROR!: Secure stack overflow - IP = %8.8lx\r\n", dwThrdPC);
        NKDbgPrintfW  (L"!FATAL ERROR!: Killing thread - pCurThread = %8.8lx\r\n", pCurThread);
        
        MDDumpThreadContext (pth, arg1, arg2, arg3, 10);
        SetThreadMode (pth, KERNEL_MODE);      // run in kernel mode

        KCALLPROFOFF(65);
        DEBUGMSG (ZONE_SCHEDULE, (L"-HandleException: Fatal stack error pcstk = %8.8lx\r\n", pcstk));
        
        return TRUE;
    }

    // Setup to capture the exception context in kernel mode but
    // running in thread context to allow preemption and stack growth.
    if (!IsPcAtCaptureContext (THRD_CTX_TO_PC (pth)))
    {
        pcstk->dwPrevSP  = dwThrdSP;             // original SP
        pcstk->retAddr   = (RETADDR) dwThrdPC;   // retaddr: IP
        pcstk->pprcLast  = NULL;                 // no process to indicate exception callstack
        pcstk->pprcVM    = pCurThread->pprcVM;   // current VM active
        pcstk->dwPrcInfo = ((USER_MODE == dwMode)? CST_MODE_FROM_USER : 0) | CST_EXCEPTION; // mode
        pcstk->pOldTls   = pCurThread->tlsNonSecure;          // save old TLS
        pcstk->phd       = 0;

        // use the cpu dependent fields to pass extra information to ExceptionDispatch
        MDSetupExcpInfo (pth, pcstk, arg1, arg2, arg3);

        KPlpvTls = pCurThread->tlsPtr = pCurThread->tlsSecure;
        
        pcstk->pcstkNext = pCurThread->pcstkTop;
        pCurThread->pcstkTop = pcstk;
        THRD_CTX_TO_SP (pth) = (DWORD) pcstk;
        THRD_CTX_TO_PC (pth) = (ulong)CaptureContext;

        KCALLPROFOFF(65);
        DEBUGMSG (ZONE_SCHEDULE, (L"-HandleException: prepare getting into ExceptionDispatch pcstk = %8.8lx\r\n", pcstk));
        return TRUE;            // continue execution
    }

    // recursive exception
    MDDumpThreadContext (pth, arg1, arg2, arg3, 10);
    RETAILMSG(1, (TEXT("Halting thread %8.8lx\r\n"), pCurThread));
    SurrenderCritSecs();
    SET_RUNSTATE (pCurThread, RUNSTATE_BLOCKED);
    RunList.pth = 0;
    SetReschedule();
    KCALLPROFOFF(65);
    return FALSE;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DumpDwords (const DWORD *pdw, int len)
{
    pdw = (const DWORD *) ((DWORD) pdw & ~3);   // dword align pdw
    NKDbgPrintfW(L"\r\nDumping %d dwords at address %8.8lx\r\n", len, pdw);
    for ( ; len > 0; pdw+=4, len -= 4) {
        NKDbgPrintfW(L"%8.8lx - %8.8lx %8.8lx %8.8lx %8.8lx\r\n", pdw, pdw[0], pdw[1], pdw[2], pdw[3]);
    }
    NKDbgPrintfW(L"\r\n");
}



