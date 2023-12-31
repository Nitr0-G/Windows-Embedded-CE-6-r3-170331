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
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    kitlp.h
    
Abstract:  
    Private defs for KITL library
    
Functions:

Notes: 

--*/
#ifndef _KITLP_H
#define _KITLP_H 1

#include <kerncmn.h>
#include <KitlProt.h>
#include <kitlpriv.h>

/*
 * This struct describes the state of a registered ethernet debug service
 */
typedef struct _KITL_CLIENT
{
    char  ServiceName[MAX_SVC_NAMELEN+1]; // Service name
    UCHAR State;
#define KITL_CLIENT_REGISTERED  0x01  // Indicates client has requested debug service
#define KITL_PEER_CONNECTED     0x02  // Set when we have address info for desktop
#define KITL_WAIT_CFG           0x04  // Set when waiting for config from desktop
#define KITL_NACK_SENT          0x08  // Only allow one NACK outstanding
#define KITL_CLIENT_REGISTERING 0x10  // Set while in KITLRegisterClient    
#define KITL_USE_SYSCALLS       0x20  // Set if system calls are allowed
#define KITL_SYNCED             0x40  // Set if we've get anything from desktop
    
    UCHAR CfgFlags;      // Flags passed into client register function (see below)

    UCHAR ServiceId;     // Service registered for (dbgmsg, kdbg, etc)
    UCHAR WindowSize;    // Window size of the client

    // Variables for keeping track of the transmit/receive window
    UCHAR TxSeqNum;      // Current send seq num (top of Tx window)
    UCHAR AckExpected;   // Seq number of expected ack (bottom of Tx window)
    UCHAR RxSeqNum;      // Next seq num expected (bottom of Rx window)
    UCHAR RxWinEnd;      // Highest seq # we're willing to accept (top of Rx window)
    
    // Buffer pool pointers
    UCHAR *pTxBufferPool;// Pointer to transmit packet buffer pool
    UCHAR *pRxBufferPool;// Pointer to Rx packet buffer pool

    // Indicate the amount of data in each slot. If 0, slot is empty
    USHORT TxFrameLen[KITL_MAX_WINDOW_SIZE];
    USHORT RxFrameLen[KITL_MAX_WINDOW_SIZE];

    UCHAR  NextRxIndex;   // Index of slot which contains next frame to read
    
    // Synchronization objects
    CRITICAL_SECTION ClientCS; // Protects this structure    

    // we cannot use "HANDLE" here because it'll require locking the handle-table to
    // signal an event. We'll deadlock in case a thread holding the handle-table lock is sending a
    // debug message.
    PHDATA  phdRecvEvt;        // Event to block on while waiting for recv
    PHDATA  phdTxFullEvt;      // Event to block on while waiting for acks
    PHDATA  phdCfgEvt;         // Event to block on while waiting for config
    
} KITL_CLIENT, *PKITL_CLIENT;

#define NUM_DFLT_KITL_SERVICES      3
#define IS_DFLT_SVC(id)             ((id) < NUM_DFLT_KITL_SERVICES)
#define IS_VALID_ID(id)             ((id) < MAX_KITL_CLIENTS)

#define HASH(x)         ((x) % (MAX_KITL_CLIENTS - NUM_DFLT_KITL_SERVICES) + NUM_DFLT_KITL_SERVICES)

#ifdef DEBUG

void DoDebugBreak (void);

#undef DEBUGCHK
#define DEBUGCHK(exp) \
   ((void)((exp)?1:(          \
       KITLOutputDebugString ("KITL: DEBUGCHK failed in file %s at line %d \r\n", \
                 __FILE__ ,__LINE__ ),    \
       DoDebugBreak(), \
	   0  \
   )))

#endif

// Secret zones 
#define KITL_ZONE_NOSERIAL  0x10000     // We write debug messages out debug serial port
                                        // if can't send them over ether, unless this
                                        // flag is set.

// Defs for KITLGlobalState (bitmask)
#define KITL_ST_ADAPTER_INITIALIZED 0x0001  // Set when adapter has been initialized
#define KITL_ST_MULTITHREADED       0x0002  // Set when the kernel is scheduling
#define KITL_ST_IST_STARTED         0x0004  // Interrupt thread has started
#define KITL_ST_INT_ENABLED         0x0008  // HW interrupts enabled
#define KITL_ST_TIMER_INIT          0x0010  // Retransmit timer thread started
#define KITL_ST_DESKTOP_CONNECTED   0x0020  // indicate that desktop has connected
#define KITL_ST_KITLSTARTED         0x0040  // indicate if KITL is started
#define KITL_ST_SYSTEMSTARTED       0x0080  // indicate system initialization done, (use of CS is allowed)

// Defs for WriteDebugLED
#define KITL_DEBUGLED(offset, value)  ((void)((KITLDebugZone&ZONE_LEDS)? OEMWriteDebugLED(offset,value),1:0))

#define LED_IST_ENTRY     1  // Set when our IST gets an interrupt event
#define LED_IST_EXIT      2  // Set when interrupt processing is complete
#define LED_IP_ID         3  // Print IP ID to LEDS, to correlate with netmon
#define LED_PEM_SEQ       4  // Print received sequence #s here

// Timer indexes for internal KITL support
#define KITL_TIMER_RENEW_DHCP   0
#define KITL_TIMER_ARP_TIMEOUT  1
#define KITL_TIMER_DHCP_RETRY   2

// Externs
extern PKITL_CLIENT KITLClients[MAX_KITL_CLIENTS];
extern DWORD KITLGlobalState;

// Prototypes
BOOL ExchangeConfig(KITL_CLIENT *pClient);
BOOL RetransmitFrame(KITL_CLIENT *pClient, UCHAR Index, BOOL fUseSysCalls);
BOOL TimerStart(KITL_CLIENT *pClient, DWORD Index, DWORD dwMSecs, BOOL fUseSysCalls);
BOOL TimerStop(KITL_CLIENT *pClient, DWORD Index, BOOL fUseSysCalls);
void CancelTimersForClient(KITL_CLIENT *pClient);
DWORD TimerThread(LPVOID pUnused);
BOOL SendConfig(KITL_CLIENT *pClient, BOOL fIsResp);

typedef BOOL (* PFN_CHECK) (LPVOID);
typedef BOOL (* PFN_TRANSMIT) (LPVOID pData, BOOL fUseSysCalls);
BOOL KITLPollResponse (BOOL fUseSysCalls, PFN_CHECK pfnCheck, PFN_TRANSMIT pfnTransmit, LPVOID pData);
void HandleRecvInterrupt(UCHAR *pRecvBuf, BOOL fUseSysCalls, PFN_TRANSMIT pfnTransmit, LPVOID pData);

int ChkDfltSvc (LPCSTR ServiceName);

// NOTE: pFrame is pointing to the start of the frame, cbData is ONLY the size of the data
//       NOT the size of the frame
//          ----------------
//          | Frame Header |
//          |--------------|
//          |     Data     |        // of size cbData
//          |--------------|
//          | Frame Tailer |
//          |--------------|
BOOL KitlSendFrame (LPBYTE pbFrame, WORD cbData);

extern KITLPRIV g_kpriv;

#define AllocMem            g_kpriv.pfnAllocMem
#define FreeMem             g_kpriv.pfnFreeMem
#define VMAlloc             g_kpriv.pfnVMAlloc
#define VMFreeAndRelease    g_kpriv.pfnVMFreeAndRelease
#define LockHandleData      g_kpriv.pfnLockHandleData
#define UnlockHandleData    g_kpriv.pfnUnlockHandleData
#define HNDLCloseHandle     g_kpriv.pfnHNDLCloseHandle
#define NKCreateEvent       g_kpriv.pfnNKCreateEvent
#define EVNTModify          g_kpriv.pfnEVNTModify
#define INTRInitialize      g_kpriv.pfnIntrInitialize
#define KCall               g_kpriv.pfnKCall
#define DoWaitForObjects    g_kpriv.pfnDoWaitForObjects
#define CreateKernelThread  g_kpriv.pfnCreateKernelThread
#define dwTimerThId         g_pKData->dwIdKitlTimer
#define dwIntrThId          g_pKData->dwIdKitlIST
#ifdef ARM
#define InSysCall           g_kpriv.pfnInSysCall
#endif
    

#define KLOG(x)     KITLOutputDebugString ("%x: %s == 0x%x\r\n", dwCurThId, (#x), x)
#define KLOGS(x)    KITLOutputDebugString ("%x: %s == '%s'\r\n", dwCurThId, (#x), x)
#define ODS         KITLOutputDebugString

#endif // _KITLP_H
