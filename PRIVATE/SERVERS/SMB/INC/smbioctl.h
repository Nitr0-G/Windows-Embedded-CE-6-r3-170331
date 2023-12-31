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


#ifndef SMB_IOCTL_H
#define SMB_IOCTL_H


#define IOCTL_SET_MAX_CONNS 4

#ifdef DEBUG
#define IOCTL_DEBUG_PRINT 5
#endif


#define SMB_IOCTL_INVOKE                    1000
#define IOCTL_CHANGE_ACL                            1
#define IOCTL_ADD_SHARE                             2
#define IOCTL_DEL_SHARE                             3
#define IOCTL_LIST_USERS_CONNECTED      4
#define IOCTL_QUERY_AMOUNT_TRANSFERED   5
#define IOCTL_NET_UPDATE_ALIASES            6
#define IOCTL_NET_UPDATE_IP_ADDRESS     7


#endif
