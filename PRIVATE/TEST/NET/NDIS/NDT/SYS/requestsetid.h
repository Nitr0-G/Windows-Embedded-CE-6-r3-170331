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
#ifndef __REQUEST_SET_ID_H
#define __REQUEST_SET_ID_H

//------------------------------------------------------------------------------

#include "Request.h"

//------------------------------------------------------------------------------

class CRequestSetId : public CRequest
{
public:
   // Input params
   USHORT m_usLocalId;
   USHORT m_usRemoteId;
 
public:
   CRequestSetId(CBinding *pBinding = NULL);
   
   virtual NDIS_STATUS Execute();
   virtual NDIS_STATUS UnmarshalInpParams(LPVOID *ppvBuffer, DWORD *pcbBuffer);
};

//------------------------------------------------------------------------------

#endif
