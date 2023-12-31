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
//
//    syncobj.c - implementations of synchronization objects
//
#include <windows.h>
#include <kernel.h>
#include <watchdog.h>

/*

Note about the API set: All APIs should either return 0 on error or return
no value at-all. This will allow us to return gracefully to the user when an
argument to an API doesn't pass the PSL checks. Otherwise we will end
up throwing exceptions on invalid arguments to API calls.

*/

//
// event methods
//
static const PFNVOID EvntMthds[] = {
    (PFNVOID)EVNTCloseHandle,
    (PFNVOID)MSGQPreClose,
    (PFNVOID)EVNTModify,
    (PFNVOID)EVNTGetData,
    (PFNVOID)EVNTSetData,
    (PFNVOID)MSGQRead,
    (PFNVOID)MSGQWrite,
    (PFNVOID)MSGQGetInfo,
    (PFNVOID)WDStart,
    (PFNVOID)WDStop,
    (PFNVOID)WDRefresh,
    (PFNVOID)EVNTResumeMainThread,     // BC work around - allow ResumeThread called on process event
};

static const ULONGLONG evntSigs [] = {
    FNSIG1 (DW),                                // CloseHandle
    0,                                          // PreCloseHandle - never called from outside
    FNSIG2 (DW, DW),                            // EventModify
    FNSIG1 (DW),                                // GetEventData
    FNSIG2 (DW, DW),                            // SetEventData
    FNSIG7 (DW, O_PTR, DW, O_PDW, DW, O_PDW, O_PDW), // ReadMsgQueueEx
    FNSIG5 (DW, I_PTR, DW, DW, DW),             // WriteMsgQueue
    FNSIG2 (DW, IO_PDW),                        // GetMsgQueueInfo (2nd arg - fixed sized struct, use IO_PDW)
    FNSIG1 (DW),                                // StartWatchDogTimer
    FNSIG1 (DW),                                // StopWatchDogTimer
    FNSIG1 (DW),                                // RefreshWatchDogTimer
    FNSIG2 (DW, O_PDW),                         // ResumeMainThread
};

ERRFALSE ((sizeof(EvntMthds) / sizeof(EvntMthds[0])) == (sizeof(evntSigs) / sizeof(evntSigs[0])));


//
// mutex methods
//
static const PFNVOID MutxMthds[] = {
    (PFNVOID)MUTXCloseHandle,
    (PFNVOID)0,
    (PFNVOID)MUTXRelease,
};

static const ULONGLONG mutxSigs [] = {
    FNSIG1 (DW),                                // CloseHandle
    0,                                          // PreCloseHandle - never called from outside
    FNSIG1 (DW),                                // ReleaseMutex
};

ERRFALSE ((sizeof(MutxMthds) / sizeof(MutxMthds[0])) == (sizeof(mutxSigs) / sizeof(mutxSigs[0])));


//
// semaphore methods
//
static const PFNVOID SemMthds[] = {
    (PFNVOID)SEMCloseHandle,
    (PFNVOID)0,
    (PFNVOID)SEMRelease,
};

static const ULONGLONG semSigs [] = {
    FNSIG1 (DW),                                // CloseHandle
    0,                                          // PreCloseHandle - never called from outside
    FNSIG3 (DW, DW, IO_PDW),                    // ReleaseMutex
};

ERRFALSE ((sizeof(SemMthds) / sizeof(SemMthds[0])) == (sizeof(semSigs) / sizeof(semSigs[0])));


//
// critical section methods
//
static const PFNVOID CritMthds [] = {
    (PFNVOID) CRITDelete,
    (PFNVOID) 0,
    (PFNVOID) Win32CRITEnter,
    (PFNVOID) CRITLeave,
};

static const ULONGLONG critSigs [] = {
    FNSIG1 (DW),                                // CloseHandle
    0,                                          // PreCloseHandle - never called from outside
    FNSIG2 (DW, IO_PDW),                        // CRITEnter, 2nd arguemt lpcs, fixed sized struct
    FNSIG2 (DW, IO_PDW),                        // CRITLeave, 2nd arguemt lpcs, fixed sized struct
};

ERRFALSE ((sizeof(CritMthds) / sizeof(CritMthds[0])) == (sizeof(critSigs) / sizeof(critSigs[0])));


const CINFO cinfEvent = {
    "EVNT",
    DISPATCH_KERNEL_PSL,
    HT_EVENT,
    sizeof(EvntMthds)/sizeof(EvntMthds[0]),
    EvntMthds,
    EvntMthds,
    evntSigs,
    0,
    0,
    0,
};

const CINFO cinfMutex = {
    "MUTX",
    DISPATCH_KERNEL_PSL,
    HT_MUTEX,
    sizeof(MutxMthds)/sizeof(MutxMthds[0]),
    MutxMthds,
    MutxMthds,
    mutxSigs,
    0,
    0,
    0,
};

const CINFO cinfSem = {
    "SEMP",
    DISPATCH_KERNEL_PSL,
    HT_SEMAPHORE,
    sizeof(SemMthds)/sizeof(SemMthds[0]),
    SemMthds,
    SemMthds,
    semSigs,
    0,
    0,
    0,
};

const CINFO cinfCRIT = {
    "CRIT",
    DISPATCH_KERNEL_PSL,
    HT_CRITSEC,
    sizeof(CritMthds)/sizeof(CritMthds[0]),
    CritMthds,
    CritMthds,
    critSigs,
    0,
    0,
    0,
};

typedef void (* PFN_InitSyncObj) (LPVOID pObj, DWORD dwParam1, DWORD dwParam2);
typedef void (* PFN_DeInitSyncObj) (LPVOID pObj);

//
// structure to define object specific methods of kernel sync-objects (mutex, event, and semaphore)
// NOTE: name comparision is object specific because it's not at the same offset
//       for individual objects. We should probabaly look into changing the structures
//       so they're at the same offset and then we don't need a compare-name method.
//
typedef struct _SYNCOBJ_METHOD {
    PFN_InitSyncObj     pfnInit;
    PFN_DeInitSyncObj   pfnDeInit;
    const CINFO         *pci;
} SYNCOBJ_METHOD, *PSYNCOBJ_METHOD;

//------------------------------------------------------------------------------
// initialize a mutex
//------------------------------------------------------------------------------
static void InitMutex (LPVOID pObj, DWORD bInitialOwner, DWORD unused)
{
    PMUTEX pMutex = (PMUTEX) pObj;
    if (bInitialOwner) {
        pMutex->LockCount = 1;
        pMutex->pOwner = pCurThread;
        KCall ((PKFN)LaterLinkCritMut, pMutex, 0);
    }
}


//------------------------------------------------------------------------------
// initialize an event
//------------------------------------------------------------------------------
static void InitEvent (LPVOID pObj, DWORD fManualReset, DWORD fInitState)
{
    PEVENT pEvent = (PEVENT) pObj;
    pEvent->state = (BYTE) fInitState;
    pEvent->manualreset = (fManualReset? 1 : 0);
    pEvent->bMaxPrio = THREAD_RT_PRIORITY_IDLE;
}

//------------------------------------------------------------------------------
// initialize a semaphore
//------------------------------------------------------------------------------
static void InitSemaphore (LPVOID pObj, DWORD lInitCount, DWORD lMaxCount)
{
    PSEMAPHORE pSem = (PSEMAPHORE) pObj;

    pSem->lMaxCount = (LONG) lMaxCount;
    pSem->lCount = (LONG) lInitCount;
}

//------------------------------------------------------------------------------
// delete a mutex
//------------------------------------------------------------------------------
static void DeInitMutex (LPVOID pObj)
{
    PMUTEX pMutex = (PMUTEX) pObj;

    BOOL fOtherWaiting = FALSE;

    CELOG_MutexDelete (pMutex);

    while (pMutex->pProxList) {
        if (KCall ((PKFN)DequeuePrioProxy, pMutex->pProxList)) {
            KCall ((PKFN)DoReprioMutex, pMutex);
            fOtherWaiting = TRUE;
        }
    }

    DEBUGMSG (fOtherWaiting, (L"WARNING: Mutex %8.8lx is deleted while other thread is waiting for it\r\n", pMutex));
    DEBUGCHK (!fOtherWaiting);

    // init might have assigned the owner of the mutex to pCurThread. Need to release it.
    KCall ((PKFN)DoFreeMutex, pMutex);

}

//------------------------------------------------------------------------------
// delete an event
//------------------------------------------------------------------------------
static void DeInitEvent (LPVOID pObj)
{
    PEVENT pEvent = (PEVENT) pObj;

    PKFN pfnDequeue = (pEvent->phdIntr || pEvent->manualreset)? (PKFN) DequeueFlatProxy : (PKFN) DequeuePrioProxy;
    BOOL fOtherWaiting = FALSE;

    CELOG_EventDelete (pEvent);

    // close the message queue if this is a message queue
    MSGQClose (pEvent);
    
    // delete the watchdog object if this is a watchdog event
    WDDelete (pEvent);

    while (pEvent->pProxList) {
        if (KCall (pfnDequeue, pEvent->pProxList))
            fOtherWaiting = TRUE;
    }

    if (pEvent->phdIntr)
        FreeIntrFromEvent (pEvent);
    
    DEBUGMSG (fOtherWaiting, (L"WARNING: Event %8.8lx is deleted while other thread is waiting for it\r\n", pEvent));
}

//------------------------------------------------------------------------------
// delete a semaphore
//------------------------------------------------------------------------------
static void DeInitSemaphore (LPVOID pObj)
{
    PSEMAPHORE pSem = (PSEMAPHORE) pObj;

    BOOL fOtherWaiting = FALSE;
    
    CELOG_SemaphoreDelete (pSem);

    while (pSem->pProxList) {
        if (KCall ((PKFN)DequeuePrioProxy, pSem->pProxList))
            fOtherWaiting = TRUE;
    }

    DEBUGMSG (fOtherWaiting, (L"WARNING: Semaphore %8.8lx is deleted while other thread is waiting for it\r\n", pObj));
        
}
//
// Event methods
//
static SYNCOBJ_METHOD eventMethod = { 
    InitEvent,
    DeInitEvent,
    &cinfEvent,
    };

//
// Mutex methods
//
static SYNCOBJ_METHOD mutexMethod = {
    InitMutex,
    DeInitMutex,
    &cinfMutex,
    };

//
// Semaphore methods
//
static SYNCOBJ_METHOD semMethod   = {
    InitSemaphore,
    DeInitSemaphore,
    &cinfSem,
    };

//------------------------------------------------------------------------------
// common function to create/open a sync-object
//------------------------------------------------------------------------------
static HANDLE OpenOrCreateSyncObject (
    LPSECURITY_ATTRIBUTES   lpsa,
    DWORD                   dwParam1, 
    DWORD                   dwParam2, 
    LPCWSTR                 pszName,
    PSYNCOBJ_METHOD         pObjMethod,
    BOOL                    fCreate)
{
    DWORD  dwErr = 0;
    HANDLE hObj = NULL;
    PNAME  pObjName = DupStrToPNAME (pszName);

    // error if open existing without a name
    if ((!fCreate && !pszName)          // open with no name
        || (pszName && !pObjName)) {    // invalid name
        dwErr = ERROR_INVALID_PARAMETER;

    } else {

        LPVOID  pObj = NULL;
        PPROCESS pprc = pActvProc;

        if (fCreate) {

            // create object
            if ((pObj = AllocMem (HEAP_SYNC_OBJ))) {
            
                memset (pObj, 0, HEAP_SIZE_SYNC_OBJ);

                // call object init function
                pObjMethod->pfnInit (pObj, dwParam1, dwParam2);
                
            } else {
                dwErr = ERROR_OUTOFMEMORY;
            }
        } else {
            DEBUGCHK (pObjName);
        }

        if (!dwErr) {
            hObj = pObjName
                ? HNDLCreateNamedHandle (pObjMethod->pci, pObj, pprc, lpsa, pObjName, fCreate, &dwErr)
                : HNDLCreateHandle (pObjMethod->pci, pObj, pprc);

            if (!hObj && !dwErr) {
                dwErr = ERROR_OUTOFMEMORY;
            }
        }
            
        // clean up on error. In case of ERROR_ALREADY_EXIST, we still need to free the object
        // created because the handle refers to the existing object.
        if (dwErr) {
            if (pObj) {
                // call object de-init if object already created
                pObjMethod->pfnDeInit (pObj);
                FreeMem (pObj, HEAP_SYNC_OBJ);
            }

            if (pObjName) {
                FreeName (pObjName);
            }
            
        }
    }


    KSetLastError (pCurThread, dwErr);
    return hObj;
   
}

//------------------------------------------------------------------------------
// common function to close a sync-object handle
//------------------------------------------------------------------------------
static BOOL CloseSyncObjectHandle (LPVOID pObj, PSYNCOBJ_METHOD pSyncMethod)
{
    pSyncMethod->pfnDeInit (pObj);
    FreeMem (pObj, HEAP_SYNC_OBJ);

    return TRUE;
}

//------------------------------------------------------------------------------
// create an event
//------------------------------------------------------------------------------
HANDLE NKCreateEvent (LPSECURITY_ATTRIBUTES lpsa, BOOL fManReset, BOOL fInitState, LPCWSTR lpEventName) 
{
    HANDLE hEvent;

    DEBUGMSG(ZONE_ENTRY, (L"NKCreateEvent entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                          lpsa, fManReset, fInitState, lpEventName));
    
    hEvent = OpenOrCreateSyncObject (lpsa, fManReset, fInitState, lpEventName, &eventMethod, TRUE);
    
    
    DEBUGMSG(ZONE_ENTRY, (L"NKCreateEvent exit: %8.8lx\r\n", hEvent));

    return hEvent;
}


//------------------------------------------------------------------------------
// open an existing named event
//------------------------------------------------------------------------------
HANDLE NKOpenEvent (DWORD dwDesiredAccess, BOOL fInheritHandle, LPCWSTR lpEventName) 
{
    HANDLE hEvent = NULL;

    DEBUGMSG(ZONE_ENTRY, (L"NKOpenEvent entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                          dwDesiredAccess, fInheritHandle, lpEventName));
    
    // Validate args
    if ((EVENT_ALL_ACCESS != dwDesiredAccess) || fInheritHandle) {

        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);

    } else {
    
        hEvent = OpenOrCreateSyncObject (NULL, FALSE, FALSE, lpEventName, &eventMethod, FALSE);
    }
    
    DEBUGMSG(ZONE_ENTRY, (L"NKOpenEvent exit: %8.8lx\r\n", hEvent));

    return hEvent;
}

//------------------------------------------------------------------------------
// EVNTSetData - set datea field of an event
//------------------------------------------------------------------------------
BOOL EVNTSetData (PEVENT lpe, DWORD dwData)
{
    BOOL fRet = !(PROCESS_EVENT & lpe->manualreset);    // SetEventData not allowed for process event
    DEBUGMSG(ZONE_ENTRY,(L"EVNTSetData entry: %8.8lx %8.8lx\r\n",lpe,dwData));
    if (fRet) {
        lpe->dwData = dwData;
    }
    DEBUGMSG(ZONE_ENTRY,(L"EVNTSetData exit: %d\r\n", fRet));
    return fRet;
}

//------------------------------------------------------------------------------
// EVNTGetData - get datea field of an event
//------------------------------------------------------------------------------
DWORD EVNTGetData (PEVENT lpe)
{
    DEBUGMSG(ZONE_ENTRY,(L"EVNTGetData entry: %8.8lx\r\n",lpe));

    if (!lpe->dwData) {
        KSetLastError (pCurThread, 0);  // no error
    }

    DEBUGMSG(ZONE_ENTRY,(L"EVNTGetData exit: %8.8lx\r\n", lpe->dwData));
    return lpe->dwData;

}

//------------------------------------------------------------------------------
// EVNTResumeMainThread - BC workaround, call ResumeThread on process event
//------------------------------------------------------------------------------
BOOL EVNTResumeMainThread (PEVENT lpe, LPDWORD pdwRetVal)
{
    DWORD dwRet = (DWORD)-1;

    DEBUGMSG(ZONE_ENTRY,(L"EVNTResumeMainThread entry: %8.8lx\r\n",lpe));
    if (PROCESS_EVENT & lpe->manualreset) {
        PHDATA phdThrd = LockHandleData ((HANDLE)lpe->dwData, g_pprcNK);
        PTHREAD    pth = GetThreadPtr (phdThrd);

        if (pth) {
            dwRet = KCall ((PKFN)ThreadResume, pth);
        } else {
            KSetLastError (pCurThread, ERROR_INVALID_HANDLE);
        }
        
        UnlockHandleData (phdThrd);
    }
    *pdwRetVal = dwRet;
    DEBUGMSG(ZONE_ENTRY,(L"EVNTResumeMainThread exit: dwRet = %8.8lx\r\n", dwRet));

    return (DWORD) -1 != dwRet;
}


//------------------------------------------------------------------------------
// ForceEventModify - set/pulse/reset an event, bypass PROCESS_EVENT checking
//------------------------------------------------------------------------------
DWORD ForceEventModify (PEVENT lpe, DWORD type)
{
    PTHREAD pThWake;
    PSTUBEVENT lpse;
    DWORD   dwErr = 0;
    DEBUGMSG(ZONE_SCHEDULE,(L"ForceEventModify entry: %8.8lx %8.8lx\r\n",lpe,type));

    CELOG_EventModify(lpe, type);

    switch (type) {
    case EVENT_PULSE:
    case EVENT_SET:
        if (lpe->phdIntr) {
            if (pThWake = (HANDLE)KCall((PKFN)EventModIntr,lpe,type))
                KCall((PKFN)MakeRunIfNeeded,pThWake);
        } else if (lpe->manualreset) {
            DWORD dwOldPrio, dwNewPrio;
            // *lpse can't be stack-based since other threads won't have access and might dequeue/requeue
            if (!(lpse = AllocMem(HEAP_STUBEVENT))) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }
            dwOldPrio = GET_BPRIO(pCurThread);
            dwNewPrio = KCall((PKFN)EventModMan,lpe,lpse,type);
            if (lpse->pProxList) {
                SET_NOPRIOCALC(pCurThread);
                if (dwNewPrio < dwOldPrio)
                    SET_CPRIO(pCurThread,dwNewPrio);
                while (lpse->pProxList) {
                    pThWake = 0;
                    KCall((PKFN)WakeOneThreadFlat,lpse,&pThWake);
                    if (pThWake)
                        KCall((PKFN)MakeRunIfNeeded,pThWake);
                }
                CLEAR_NOPRIOCALC(pCurThread);
                if (dwNewPrio < dwOldPrio)
                    KCall((PKFN)AdjustPrioDown);
            }
            FreeMem(lpse,HEAP_STUBEVENT);
        } else {
            pThWake = 0;
            while (KCall((PKFN)EventModAuto,lpe,type,&pThWake))
                ;
            if (pThWake)
                KCall((PKFN)MakeRunIfNeeded,pThWake);
        }
        break;
    case EVENT_RESET:
        lpe->state = 0;
        break;
    default:
        dwErr = ERROR_INVALID_HANDLE;
        DEBUGCHK(0);
    }
    DEBUGMSG(ZONE_SCHEDULE,(L"ForceEventModify exit: dwErr = %8.8lx\r\n",dwErr));
    return dwErr;
}

//------------------------------------------------------------------------------
// EVNTModify - set/pulse/reset an event
//------------------------------------------------------------------------------
BOOL EVNTModify (PEVENT lpe, DWORD type)
{
    DWORD dwErr;
    DEBUGMSG(ZONE_ENTRY,(L"EVNTModify entry: %8.8lx %8.8lx\r\n",lpe,type));

    dwErr = (lpe->manualreset & PROCESS_EVENT)
        ? ERROR_INVALID_HANDLE              // wait-only event, cannot be signaled
        : ForceEventModify (lpe, type);

    if (dwErr) {
        SetLastError (dwErr);
    }
    DEBUGMSG(ZONE_ENTRY,(L"EVNTModify exit: dwErr = %8.8lx\r\n", dwErr));
    return !dwErr;
}

//------------------------------------------------------------------------------
// closehandle an event
//------------------------------------------------------------------------------
BOOL EVNTCloseHandle (PEVENT lpe)
{
    DEBUGMSG(ZONE_ENTRY,(L"EVNTCloseHandle entry: %8.8lx\r\n",lpe));
    
    CloseSyncObjectHandle (lpe, &eventMethod);

    DEBUGMSG(ZONE_ENTRY,(L"EVNTCloseHandle exit: %8.8lx\r\n",TRUE));
    return TRUE;
}

//------------------------------------------------------------------------------
// NKEventModify - set/reset/pulse an event, with handle
//------------------------------------------------------------------------------
BOOL NKEventModify (PPROCESS pprc, HANDLE hEvent, DWORD type)
{
    PHDATA phd = LockHandleData (hEvent, pprc);
    PEVENT lpe = GetEventPtr (phd);
    BOOL   fRet = FALSE;

    if (lpe) {
        fRet = EVNTModify (lpe, type);
    }

    UnlockHandleData (phd);

    DEBUGMSG (!fRet, (L"NKEventModify Failed\r\n"));

    return fRet;
}

//------------------------------------------------------------------------------
// NKIsNamedEventSignaled - check if an event is signaled
//------------------------------------------------------------------------------
BOOL NKIsNamedEventSignaled (LPCWSTR pszName, DWORD dwFlags)
{
    HANDLE hEvt = NKOpenEvent (EVENT_ALL_ACCESS, FALSE, pszName);
    BOOL fRet = FALSE;
    if (hEvt) {
        PHDATA phd = LockHandleData (hEvt, pActvProc);
        PEVENT lpe = GetEventPtr (phd);
        if (lpe) {
            fRet = lpe->state;
        }
        UnlockHandleData (phd);
    }
    return fRet;
}

//------------------------------------------------------------------------------
// LockIntrEvt - called from InterruptInitialize, lock an interrupt event to
//               prevent it from being freed. ptr to HDATA is saved in the event
//               structure.
//------------------------------------------------------------------------------
PEVENT LockIntrEvt (HANDLE hIntrEvt)
{
    PHDATA phd = LockHandleData (hIntrEvt, pActvProc);
    PEVENT lpe = GetEventPtr (phd);
    if  (lpe && !lpe->manualreset && !lpe->pProxList) {
        lpe->phdIntr = phd;
    } else {
        UnlockHandleData (phd);
        lpe = NULL;
    }
    return lpe;
}

//------------------------------------------------------------------------------
// UnlockIntrEvt - called from InterruptDisable, unlock an interrupt event
//------------------------------------------------------------------------------
BOOL UnlockIntrEvt (PEVENT pIntrEvt)
{
    return (pIntrEvt && pIntrEvt->phdIntr)? UnlockHandleData (pIntrEvt->phdIntr) : FALSE;
}


//------------------------------------------------------------------------------
// create a semaphore
//------------------------------------------------------------------------------
HANDLE NKCreateSemaphore (LPSECURITY_ATTRIBUTES lpsa, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName) 
{
    HANDLE hSem = NULL;

    DEBUGMSG(ZONE_ENTRY,(L"NKCreateSemaphore entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",lpsa,lInitialCount,lMaximumCount,lpName));
    
    if ((lInitialCount < 0) || (lMaximumCount < 0) || (lInitialCount > lMaximumCount))
        KSetLastError (pCurThread, ERROR_INVALID_PARAMETER);
    else 
        hSem = OpenOrCreateSyncObject (lpsa, (DWORD) lInitialCount, (DWORD) lMaximumCount, lpName, &semMethod, TRUE);

    DEBUGMSG(ZONE_ENTRY,(L"NKCreateSemaphore exit: %8.8lx\r\n",hSem));
    return hSem;
}

//------------------------------------------------------------------------------
// open an existing named semaphore
//------------------------------------------------------------------------------
HANDLE NKOpenSemaphore (DWORD dwDesiredAccess, BOOL fInheritHandle, LPCWSTR lpName) 
{
    HANDLE hSem = NULL;

    DEBUGMSG(ZONE_ENTRY, (L"NKOpenSemaphore: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                          dwDesiredAccess, fInheritHandle, lpName));
    
    // Validate args
    if ((SEMAPHORE_ALL_ACCESS != dwDesiredAccess) || fInheritHandle) {

        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);

    } else {
    
        hSem  = OpenOrCreateSyncObject (NULL, FALSE, FALSE, lpName, &semMethod, FALSE);
    }
    
    DEBUGMSG(ZONE_ENTRY, (L"NKOpenSemaphore exit: %8.8lx\r\n", hSem));

    return hSem ;
}


//------------------------------------------------------------------------------
// SEMRelease - release a semaphore
//------------------------------------------------------------------------------
BOOL SEMRelease (PSEMAPHORE pSem, LONG lReleaseCount, LPLONG lpPreviousCount) 
{
    LONG    prev;
    PTHREAD pth;
    DWORD   dwErr = 0;

    DEBUGMSG(ZONE_ENTRY,(L"SEMRelease entry: %8.8lx %8.8lx %8.8lx\r\n", pSem, lReleaseCount, lpPreviousCount));
    if ((prev = KCall ((PKFN)SemAdd, pSem, lReleaseCount)) == -1) {
        dwErr = ERROR_INVALID_PARAMETER;
        
    } else {

        CELOG_SemaphoreRelease(pSem, lReleaseCount, prev);

        pth = 0;
        while (KCall ((PKFN)SemPop, pSem, &lReleaseCount, &pth)) {
            if (pth) {
                KCall ((PKFN)MakeRunIfNeeded, pth);
                pth = 0;
            }
        }

        if (lpPreviousCount) {
            __try {
                *lpPreviousCount = prev;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
    }

    KSetLastError (pCurThread, dwErr);
    return !dwErr;
}


//------------------------------------------------------------------------------
// closehandle a semaphore
//------------------------------------------------------------------------------
BOOL SEMCloseHandle (PSEMAPHORE lpSem) 
{
    DEBUGMSG(ZONE_ENTRY,(L"SEMCloseHandle entry: %8.8lx\r\n",lpSem));

    CloseSyncObjectHandle (lpSem, &semMethod);

    DEBUGMSG(ZONE_ENTRY,(L"SEMCloseHandle exit: %8.8lx\r\n",TRUE));
    return TRUE;
}

//------------------------------------------------------------------------------
//  create a mutex
//------------------------------------------------------------------------------
HANDLE NKCreateMutex (LPSECURITY_ATTRIBUTES lpsa, BOOL bInitialOwner, LPCTSTR lpName) 
{
    HANDLE hMutex;
    DEBUGMSG(ZONE_ENTRY,(L"NKCreateMutex entry: %8.8lx %8.8lx %8.8lx\r\n",lpsa,bInitialOwner,lpName));


    hMutex = OpenOrCreateSyncObject (lpsa, bInitialOwner, 0, lpName, &mutexMethod, TRUE);

    DEBUGMSG(ZONE_ENTRY,(L"NKCreateMutex exit: %8.8lx\r\n",hMutex));
    return hMutex;
}


//------------------------------------------------------------------------------
// open an existing named mutex
//------------------------------------------------------------------------------
HANDLE NKOpenMutex (DWORD dwDesiredAccess, BOOL fInheritHandle, LPCWSTR lpName) 
{
    HANDLE hMutex = NULL;

    DEBUGMSG(ZONE_ENTRY, (L"NKOpenMutex entry: %8.8lx %8.8lx %8.8lx %8.8lx\r\n",
                          dwDesiredAccess, fInheritHandle, lpName));
    
    // Validate args
    if ((MUTANT_ALL_ACCESS != dwDesiredAccess) || fInheritHandle) {

        KSetLastError(pCurThread, ERROR_INVALID_PARAMETER);

    } else {
    
        hMutex  = OpenOrCreateSyncObject (NULL, FALSE, FALSE, lpName, &mutexMethod, FALSE);
    }
    
    DEBUGMSG(ZONE_ENTRY, (L"NKOpenMutex exit: %8.8lx\r\n", hMutex));

    return hMutex ;
}


//------------------------------------------------------------------------------
// DoLeaveMutex - worker function to release a mutex
//------------------------------------------------------------------------------
void DoLeaveMutex (PMUTEX lpm) 
{
    PTHREAD pth;
    KCall ((PKFN) PreUnlinkMutex, lpm);
    pth = 0;
    while (KCall ((PKFN) LeaveMutex, lpm, &pth))
        ;
    if (pth)
        KCall ((PKFN) MakeRunIfNeeded, pth);
    KCall ((PKFN) PostUnlinkCritMut);
}

//------------------------------------------------------------------------------
//  release a mutex
//------------------------------------------------------------------------------
BOOL MUTXRelease (PMUTEX lpm) 
{
    BOOL fRet = (lpm->pOwner == pCurThread);

    if (!fRet) {
        SetLastError(ERROR_NOT_OWNER);
        
    } else {
        CELOG_MutexRelease(lpm);
        if (lpm->LockCount > 1) {
            lpm->LockCount--;
        } else {
            DEBUGCHK (lpm->LockCount == 1);
            DoLeaveMutex(lpm);
        }
    }
    
    return fRet;
}


//------------------------------------------------------------------------------
// closehandle a mutex
//------------------------------------------------------------------------------
BOOL MUTXCloseHandle (PMUTEX lpm) 
{
    DEBUGMSG(ZONE_ENTRY,(L"MUTXCloseHandle entry: %8.8lx\r\n",lpm));

    CloseSyncObjectHandle (lpm, &mutexMethod);

    DEBUGMSG(ZONE_ENTRY,(L"MUTXCloseHandle exit: %8.8lx\r\n",TRUE));
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IMPORTANT NOTE FOR CRITICAL SECTION:
//
//  Kernel and user mode CS are handled differently. The hCrit member of CS is a pointer when CS is created by
//  kernel (ONLY KERNEL, NOT INCLUDING OTHER DLL LOADED INTO KERNEL). For the others, hCrit member is a handle.
//
//  Therefore, the hCrit member SHOULD NEVER BE REFERENCED WITHIN CS HANDLING CODE. An PCRIT argument is passed
//  along all the CS functions to access the CRIT structure.
//
//


//------------------------------------------------------------------------------
// Critical section related functions

//------------------------------------------------------------------------------
// CRITFree - final clean before deleting a critical section
//------------------------------------------------------------------------------
void CRITFree (PCRIT pcrit) 
{
    while (pcrit->pProxList)
        KCall((PKFN)DequeuePrioProxy,pcrit->pProxList);
    KCall((PKFN)DoFreeCrit,pcrit);
    FreeMem(pcrit,HEAP_CRIT);
}


//------------------------------------------------------------------------------
// CRITCreate - create a critical section
//------------------------------------------------------------------------------
PCRIT CRITCreate (LPCRITICAL_SECTION lpcs) 
{
    PCRIT pcrit;
    DEBUGMSG(ZONE_ENTRY,(L"CRITCreate entry: %8.8lx\r\n",lpcs));

    if (pcrit = AllocMem(HEAP_CRIT)) {
        memset (pcrit, 0, sizeof(CRIT));
        pcrit->lpcs     = lpcs;
        pcrit->wCritSig = CRIT_SIGNATURE;
    }
    DEBUGMSG(ZONE_ENTRY,(L"CRITCreate exit: %8.8lx\r\n",pcrit));
    return pcrit;
}


//------------------------------------------------------------------------------
// FreeCrit - clean up and delete a PCRIT object
//------------------------------------------------------------------------------
void FreeCrit (PCRIT pcrit) 
{
    CELOG_CriticalSectionDelete(pcrit);

    while (pcrit->pProxList)
        KCall ((PKFN)DequeuePrioProxy, pcrit->pProxList);
    KCall ((PKFN)DoFreeCrit, pcrit);
    FreeMem (pcrit, HEAP_CRIT);
}

//------------------------------------------------------------------------------
// CRITDelete - delete a critical section
//------------------------------------------------------------------------------
BOOL CRITDelete (PCRIT pcrit) 
{
    LPCRITICAL_SECTION lpcs = pcrit->lpcs;

    DEBUGMSG(ZONE_ENTRY,(L"CRITDelete entry: %8.8lx\r\n", pcrit));

    FreeCrit (pcrit);

    return TRUE;
}


//------------------------------------------------------------------------------
// CRITEnter - EnterCriticalSection
//------------------------------------------------------------------------------
void CRITEnter (PCRIT pCrit, LPCRITICAL_SECTION lpcs) 
{
    BOOL        bRetry;
    PROXY       Prox;
    CLEANEVENT  ce;
    PCRIT       pCritMut;
    bRetry = 1;
    ce.ceptr = NULL;  //Initialize the first field in the struct so we will not fault in KCall (stack grows downward)
    
    DEBUGCHK (!InSysCall ());

    if (pCrit->lpcs != lpcs) {
        DEBUGCHK (0);
        return;
    }
    do {

        Prox.pQNext = Prox.pQPrev = Prox.pQDown = 0;
        Prox.wCount = pCurThread->wCount;
        Prox.pObject = (LPBYTE)pCrit;
        Prox.bType = HT_CRITSEC;
        Prox.pTh = pCurThread;
        Prox.prio = (BYTE)GET_CPRIO(pCurThread);
        Prox.dwRetVal = WAIT_OBJECT_0;
        Prox.pThLinkNext = 0;
#ifdef DEBUG
        DEBUGCHK (!((pCurThread->wInfo >> DEBUG_LOOPCNT_SHIFT) & 1));
        pCurThread->wInfo |= (1 << DEBUG_LOOPCNT_SHIFT);
#endif
        pCurThread->dwPendTime = INFINITE;    // required??
        KCall ((PKFN)WaitConfig, &Prox, &ce);
        pCritMut = 0;
        switch (KCall ((PKFN)CSWaitPart1, &pCritMut, &Prox, pCrit)) {
        case CSWAIT_TAKEN: 
            DEBUGCHK(!pCritMut || (pCritMut == pCrit));
            if (pCritMut && ((HANDLE) dwCurThId == lpcs->OwnerThread))
                KCall((PKFN)LaterLinkCritMut,pCritMut,1);
            break;
        case CSWAIT_PEND:
            DEBUGCHK(!pCritMut || (pCritMut == pCrit));
            if (pCritMut) {
                KCall ((PKFN)PostBoostCrit1, pCritMut);
                KCall ((PKFN)PostBoostCrit2, pCritMut);
            }
            CELOG_CriticalSectionEnter(pCrit);
#ifdef DEBUG
            pCurThread->wInfo &= ~(1 << DEBUG_LOOPCNT_SHIFT);
#endif    
            KCall((PKFN)CSWaitPart2);
            break;
        case CSWAIT_ABORT:
            bRetry = 0;
            break;
        }
#ifdef DEBUG
        pCurThread->wInfo &= ~(1 << DEBUG_LOOPCNT_SHIFT);
#endif    
        DEBUGCHK (!pCurThread->lpProxy);
        DEBUGCHK (pCurThread->lpce == &ce);
        DEBUGCHK (((ce.base == &Prox) && !ce.size) || (!ce.base && (ce.size == (DWORD)&Prox)));
        if (Prox.pQDown)
            if (KCall ((PKFN)DequeuePrioProxy, &Prox))
                KCall ((PKFN)DoReprioCrit, pCrit);
        DEBUGCHK (!Prox.pQDown);
        pCurThread->lpce = 0;

    } while ((lpcs->OwnerThread != (HANDLE) dwCurThId) && bRetry);
    if (lpcs->OwnerThread == (HANDLE) dwCurThId)
        KCall((PKFN)CritFinalBoost, pCrit, lpcs);
}

//------------------------------------------------------------------------------
// CRITLeave - LeaveCriticalSection
//------------------------------------------------------------------------------
void CRITLeave (PCRIT pCrit, LPCRITICAL_SECTION lpcs) 
{
    PTHREAD pth;
    if (pCrit->lpcs != lpcs) {
        DEBUGCHK (0);
        return;
    }
    if (KCall ((PKFN)PreLeaveCrit, pCrit, lpcs)) {
        pth = 0;
        while (KCall((PKFN)LeaveCrit, pCrit, lpcs, &pth))
            ;
        CELOG_CriticalSectionLeave(pCrit, pth);
        if (pth)
            KCall ((PKFN)MakeRunIfNeeded, pth);
        KCall ((PKFN)PostUnlinkCritMut);
    }
}

//------------------------------------------------------------------------------
// WIN32 exports (for EnterCriticalSection and InitializeCriticalSection)
//------------------------------------------------------------------------------

// EnterCriticalSection - set the UserBlock bit
void Win32CRITEnter (PCRIT pCrit, LPCRITICAL_SECTION lpcs) 
{
    SET_USERBLOCK(pCurThread);
    CRITEnter (pCrit, lpcs);
    CLEAR_USERBLOCK(pCurThread);
}

// InitializeCriticalSection - call CRITCreate and make it a handle
HANDLE Win32CRITCreate (LPCRITICAL_SECTION lpcs)
{
    PCRIT pCrit = CRITCreate (lpcs);
    HANDLE hCrit = NULL;

    if (pCrit) {
        // create the CRIT handle
        hCrit = HNDLCreateHandle (&cinfCRIT, pCrit, pActvProc);

        if (!hCrit) {
            CRITDelete (pCrit);
        }
    }

    return hCrit;
}
