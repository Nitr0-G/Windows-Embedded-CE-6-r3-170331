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

#ifndef _DISK_IOCTL_GETINFO_H
#define _DISK_IOCTL_GETINFO_H
#include "main.h"
#include "parameterfuncs.h"
#include "IoctlTestBase.h"

/*typedef struct _IOCTL_DATA
{
	DWORD        dwIoControlCode;	//IOCTL
	LPVOID       lpInBuffer;
	DWORD        nInBufferSize;
	LPVOID       lpOutBuffer;
	DWORD        nOutBufferSize;
	DWORD        lpBytesReturned;
	LPOVERLAPPED lpOverLapped;
	BOOL	     RetVal;	//return value of IOCTL
	DWORD	     LastError; //value of GetLastError();
}IOCTL_DATA,*PIOCTL_DATA;*/


typedef class _DISK_IOCTL_GETINFO_TEST : public  DRIVER_TEST
{
	//HANDLE       hDevice;	//handle to driver
	//IOCTL_DATA   inData;	//data sent to IOCTL
	//IOCTL_DATA   outData;	//data received from IOCTL
	//BOOL Init();	  //function to call before run()
	//BOOL Start();    //starts the test
	virtual BOOL CustomFill(void);
	virtual BOOL LoadCustomParameters(void);
	virtual BOOL CustomCleanup(){return TRUE;};
public:
	_DISK_IOCTL_GETINFO_TEST(void){m_pKato = NULL;m_hDevice = NULL;dwDeviceType = 0;inData.dwIoControlCode = DISK_IOCTL_GETINFO;}
	_DISK_IOCTL_GETINFO_TEST(HANDLE Handle,CKato *pKato,DWORD deviceType){m_pKato = pKato;m_hDevice = Handle;dwDeviceType = deviceType;inData.dwIoControlCode = DISK_IOCTL_GETINFO;}
private:
	virtual BOOL RunIOCTL();	  //runs the IOCTL
	virtual BOOL Validate();  //function to validate that it ran properly
	BOOL ATAPIValidate(void);
	BOOL CDRomValidate(void);
}DISK_IOCTL_GETINFO_TEST,*PDISK_IOCTL_GETINFO_TEST;
#endif
