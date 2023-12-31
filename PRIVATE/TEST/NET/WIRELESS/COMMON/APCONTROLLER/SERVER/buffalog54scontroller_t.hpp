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
// ----------------------------------------------------------------------------
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// ----------------------------------------------------------------------------
//
// Definitions and declarations for the BuffaloG54sController_t class.
//
// ----------------------------------------------------------------------------

#ifndef _DEFINED_BuffaloG54sController_t_
#define _DEFINED_BuffaloG54sController_t_
#pragma once

#include "HtmlAPController_t.hpp"

namespace ce {
namespace qa {

// ----------------------------------------------------------------------------
//
// Customized interface methods for the Buffalo WBR2-G54S Access Point.
//
class BuffaloG54sController_t : public HtmlAPTypeController_t
{
public:

    // Constructor and destructor:
    BuffaloG54sController_t(
        HttpPort_t         *pHttp,
        AccessPointState_t *pState)
        : HtmlAPTypeController_t(pHttp,pState)
    { }
    virtual
   ~BuffaloG54sController_t(void);

    // Initializes the device-controller:
    virtual DWORD
    InitializeDevice(void);

    // Gets the configuration settings from the device:
    virtual DWORD
    LoadSettings(void);

    // Updates the device's configuration settings:
    virtual DWORD
    UpdateSettings(const AccessPointState_t &NewState);
};

};
};
#endif /* _DEFINED_BuffaloG54sController_t_ */
// ----------------------------------------------------------------------------
