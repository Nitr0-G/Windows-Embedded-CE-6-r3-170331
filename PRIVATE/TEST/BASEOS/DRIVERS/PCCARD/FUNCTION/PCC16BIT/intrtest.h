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
//******************************************************************************
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//******************************************************************************

/*++
Module Name:  
	Intrtest.h

Abstract:

    Definition of interrupt-related test class and other needed functions/data structures

--*/

#ifndef _INTRTEST_H_
#define _INTRTEST_H_

#include "minithread.h"

class IntrTest : public MiniThread {
public:
	IntrTest(DWORD dwTestCase, DWORD dwThreadNo, UINT8 uSock, UINT8 uFunc) :
			MiniThread (0,THREAD_PRIORITY_NORMAL,TRUE),
			m_dwCaseID(dwTestCase),
			m_dwThreadID(dwThreadNo),
			m_uLocalSock(uSock),
			m_uLocalFunc(uFunc){
		NKDMSG(TEXT("IntrTest: CaseID: %u; Thread No. %u; Socket %u; Function %u"), 
					m_dwCaseID, m_dwThreadID, m_uLocalSock, m_uLocalFunc);
		SetResult(TRUE);
	};
	BOOL Init();
	~IntrTest(){;};
private:
	virtual DWORD ThreadRun();
	VOID Test_CardRequestIRQ();
       BOOL GetTupleData(PCARD_TUPLE_PARMS pTupleParams,
                                  PVOID pTupleData,
                                  DWORD dwLength,
                                  DWORD dwOffset = 0);
                                  
        BOOL FindTuple( CARD_SOCKET_HANDLE hSocket,
                               PCARD_TUPLE_PARMS pTupleParams,
                               BOOL bFirst,
                               UINT8 uDesiredTuple,
                               UINT16 attributes = 0);

       DWORD	m_dwCaseID;
	DWORD 	m_dwThreadID;
	UINT8	m_uLocalSock, m_uLocalFunc;
	
};

VOID ISRFunction(UINT32 uContext);

#endif

