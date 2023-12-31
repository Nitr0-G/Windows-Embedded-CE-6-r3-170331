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

    optionmgr.h

Abstract:

    Options Manager for DhcpV6 client.

Author:

    FrancisD

Environment:

    User Level: Windows

Revision History:


--*/
#ifdef UNDER_CE
#define DHCPV6_MAX_OPTIONS  5
#else
#define DHCPV6_MAX_OPTIONS  1
#endif

typedef struct _DHCPV6_ADAPT DHCPV6_ADAPT, * PDHCPV6_ADAPT;

typedef DWORD
(*PDHCPV6_OPTION_PROCESS_RECV_FN) (
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );

//
// Option Function Table
//
typedef struct _DHCPV6_OPTION {
    DHCPV6_OPTION_TYPE DhcpV6OptionType;
    BOOL bEnabled;
    BOOL bRequired;
    PDHCPV6_OPTION_PROCESS_RECV_FN pDhcpV6OptionProcessRecv;
} DHCPV6_OPTION, * PDHCPV6_OPTION;


//
// Option Module
//
typedef struct _DHCPV6_OPTION_MODULE {
    ULONG uNumOfOptionsEnabled;
    DHCPV6_OPTION OptionsTable[DHCPV6_MAX_OPTIONS];
} DHCPV6_OPTION_MODULE, * PDHCPV6_OPTION_MODULE;

#ifdef UNDER_CE
DWORD
InitDhcpV6OptionMgr(
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule, BOOL UsePD
    );
#else
DWORD
InitDhcpV6OptionMgr(
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule
    );
#endif

DWORD
DeInitDhcpV6OptionMgr(
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule
    );

DWORD
DhcpV6OptionMgrCreateOptionRequestPD(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule,
    PUCHAR pucOptionMessageBuffer,
    USHORT usOptionLength
    );

DWORD
DhcpV6OptionMgrCreateOptionRequest(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule,
    PUCHAR pucOptionMessageBuffer,
    USHORT usOptionLength
    );

DWORD
DhcpV6OptionMgrProcessReply(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    PDHCPV6_OPTION_MODULE pDhcpV6OptionModule,
    PUCHAR pucMessageBuffer,
    ULONG uMessageBufferLength
    );

DWORD
DhcpV6OptionMgrProcessRecvDNS(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );

#ifdef UNDER_CE
DWORD
DhcpV6OptionMgrProcessRecvDomainList(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );

DWORD
DhcpV6OptionMgrProcessRecvPD(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );

DWORD
DhcpV6OptionMgrProcessRecvClientID(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );


DWORD
DhcpV6OptionMgrProcessRecvServerID(
    PDHCPV6_ADAPT pDhcpV6Adapt,
    UNALIGNED DHCPV6_OPTION_HEADER *pDhcpV6OptionHeader
    );

#endif  // UNDER_CE

