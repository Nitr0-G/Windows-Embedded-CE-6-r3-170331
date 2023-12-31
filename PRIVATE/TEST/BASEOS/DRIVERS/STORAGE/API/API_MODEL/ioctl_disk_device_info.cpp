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

#include "main.h"

TESTPROCAPI tst_IOCTL_DISK_DEVICE_INFO(UINT uMsg,TPPARAM tpParam, 
LPFUNCTION_TABLE_ENTRY lpFTE )
// --------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(lpFTE);
    	if( TPM_QUERY_THREAD_COUNT == uMsg )
    	{
        	((LPTPS_QUERY_THREAD_COUNT)tpParam)->dwThreadCount = 0;
        	return SPR_HANDLED;
    	}
    	else if( TPM_EXECUTE != uMsg )
    	{
        	return TPR_NOT_HANDLED;
    	}
	DISK_INFO originalInfo = {0};
	GetDiskInfo(g_hDisk,_T(""),&originalInfo);
	//set some bogus DISK_INFO information
	g_diskInfo.di_total_sectors = DI_TOTAL_SECTORS;
	g_diskInfo.di_bytes_per_sect = DI_BYTES_PER_SECT;
	g_diskInfo.di_cylinders = DI_CYLINDERS;
	g_diskInfo.di_heads = DI_HEADS;
	g_diskInfo.di_sectors = DI_SECTORS;
	g_diskInfo.di_flags = DI_FLAGS;
	SetDiskInfo(g_hDisk,NULL,&g_diskInfo);
	
	//somewhere up here, we should check to see if this is a valid IOCTL 
	//for the driver being tested.
	
	DRIVER_TEST *pMyTest;
	pMyTest = new IOCTL_DISK_DEVICE_INFO_TEST(g_hDisk,g_pKato,g_storageDeviceInfo.dwDeviceType);
	if(!pMyTest)
	{
		g_pKato->Log(LOG_FAIL,_T("FAILED to allocate a new IOCTL_DISK_DEVICE_INFO_TEST class.\n"));
		return FALSE;
	}
	int bRet = RunIOCTLCommon(pMyTest); 
    	if(pMyTest)
    	{
		delete pMyTest;
		pMyTest = NULL;
    	}	
	//restore disk info
	SetDiskInfo(g_hDisk,NULL,&originalInfo);
	return bRet;
}

///This is where we can add new items to loop through, and add our own custom params
BOOL IOCTL_DISK_DEVICE_INFO_TEST::CustomFill(void)
{
	BOOL bRet = Fill();
	return bRet;
}

BOOL IOCTL_DISK_DEVICE_INFO_TEST::LoadCustomParameters(void)
{
	#define NUM_BUFFER_SIZES 4
	TCHAR* g_BufferSizes [NUM_BUFFER_SIZES] = {_T("size_of_storage_device_info"),_T("zero"),_T("gt_size_of_storage_device_info"),_T("lt_size_of_storage_device_info")};
	PGROUP tempGroup = new GROUP(PARAM_nInBufferSize);
	PGROUP tempGroup1 = new GROUP(PARAM_nOutBufferSize);
	if(!tempGroup || !tempGroup1)
	{
		if(tempGroup)
			delete tempGroup;
		if(tempGroup1)
			delete tempGroup1;
		return FALSE;
	}
	for(int i = 0; i < NUM_BUFFER_SIZES; i++)
	{
		PPARAMETER tempParam = new PARAMETER(PARAM_nInBufferSize,g_BufferSizes[i]);
		tempGroup->m_ParameterList.AddTail(tempParam);
		PPARAMETER tempParam1 = new PARAMETER(PARAM_nOutBufferSize,g_BufferSizes[i]);
		tempGroup1->m_ParameterList.AddTail(tempParam1);
	}
	paramGroupList.AddTail(tempGroup);
	paramGroupList.AddTail(tempGroup1);
	
	return TRUE;
}
BOOL IOCTL_DISK_DEVICE_INFO_TEST::RunIOCTL(void)
{
	try
	{
		if(m_hDevice == NULL || INVALID_HANDLE(m_hDevice))
		{
			m_pKato->Log(LOG_DETAIL, TEXT("Invalid disk handle"));
			return FALSE;
		}
		cpyData.LastError = GetLastError();
		inData.RetVal = DeviceIoControl(m_hDevice,inData.dwIoControlCode,inData.lpInBuffer,inData.nInBufferSize,
		inData.lpOutBuffer,inData.nOutBufferSize,(LPDWORD)inData.lpBytesReturned,inData.lpOverLapped);
		inData.LastError = GetLastError();
	}
	catch(...)
	{
		m_pKato->Log(LOG_DETAIL, TEXT("Exception during DeviceIoControl.  Failing.\n"));
		return FALSE;
	}
	return TRUE;
}

BOOL IOCTL_DISK_DEVICE_INFO_TEST::ATAPIValidate(void)
{
	BOOL retVal = TRUE;
	TCHAR* lpInBuffer = loopList.FindParameterByName(PARAM_lpInBuffer)->value;
	TCHAR* lpOutBuffer = loopList.FindParameterByName(PARAM_lpOutBuffer)->value;
	TCHAR* BufferType = loopList.FindParameterByName(PARAM_BufferType)->value;
	TCHAR* lpBytesReturned = loopList.FindParameterByName(PARAM_IpBytesReturned)->value;
	TCHAR* nInBufferSize = loopList.FindParameterByName(PARAM_nInBufferSize)->value;
	TCHAR* nOutBufferSize = loopList.FindParameterByName(PARAM_nOutBufferSize)->value;

	if(_tcscmp(lpInBuffer,_T("NULL")) == 0  || _tcscmp(nInBufferSize,_T("zero")) == 0 || _tcscmp(nInBufferSize,_T("lt_size_of_storage_device_info")) == 0)
	{
		if(_tcscmp(nInBufferSize,_T("lt_size_of_storage_device_info")) == 0 && _tcscmp(lpInBuffer,_T("NULL")) != 0 && _tcscmp(nInBufferSize,_T("zero")) != 0)
		{
			if(inData.LastError != ERROR_INSUFFICIENT_BUFFER)
			{
				//fail
				m_pKato->Log(LOG_FAIL,_T("inData.LastError != ERROR_INSUFFICIENT_BUFFER as expected: %d\n"),inData.LastError);
				retVal = FALSE;
			}
		}
		else
		{
			if(inData.LastError != ERROR_INVALID_PARAMETER)
			{
				//fail
				m_pKato->Log(LOG_FAIL,_T("inData.LastError != ERROR_INVALID_PARAMETER as expected: %d\n"),inData.LastError);
				retVal = FALSE;
			}
		}
		if((inData.lpInBuffer != NULL && cpyData.lpInBuffer != NULL) && (memcmp(inData.lpInBuffer,cpyData.lpInBuffer,inData.nActualInBufferSize) != 0))
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.lpInBuffer changed unexpectedly\n"));
			retVal = FALSE;
		}
		if((inData.lpOutBuffer != NULL && cpyData.lpOutBuffer != NULL) && memcmp(inData.lpOutBuffer,cpyData.lpOutBuffer,inData.nActualOutBufferSize) != 0)
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.lpOutBuffer changed unexpectedly\n"));
			retVal = FALSE;
		}
		if(inData.RetVal != FALSE)
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.retVal is not FALSE as expected\n"));
			retVal = FALSE;
		}
		if(inData.lpBytesReturned && *((LPDWORD)inData.lpBytesReturned) != 0)
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.lpBytesReturned is %d, not %d\n"),*((LPDWORD)inData.lpBytesReturned),0);
			retVal = FALSE;
		}
		
	}
	else
	{
		//valid case
		if(g_fOpenAsStore)
		{
			if(inData.LastError != 0)
			{
				//fail
				m_pKato->Log(LOG_FAIL,_T("inData.LastError is not zero, as expected.  It should be zero because we opened the device through the Storage Manager\n"));
				retVal = FALSE;
			}
		}
		else
		{
			if(inData.LastError != cpyData.LastError)
			{
				//fail
				m_pKato->Log(LOG_FAIL,_T("inData.LastError changed unexpectedly\n"));
				retVal = FALSE;
			}
		}
		if((inData.lpOutBuffer != NULL && cpyData.lpOutBuffer != NULL) && memcmp(inData.lpOutBuffer,cpyData.lpOutBuffer,inData.nActualOutBufferSize) != 0)
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.lpOutBuffer changed unexpectedly\n"));
			retVal = FALSE;
		}
		if(inData.RetVal != TRUE)
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.retVal is not TRUE as expected\n"));
			retVal = FALSE;
		}
		if(inData.lpBytesReturned && *((LPDWORD)inData.lpBytesReturned) != sizeof(STORAGEDEVICEINFO ))
		{
			//fail
			m_pKato->Log(LOG_FAIL,_T("inData.lpBytesReturned is %d, not %d\n"),*((LPDWORD)inData.lpBytesReturned),sizeof(STORAGEDEVICEINFO ));
			retVal = FALSE;
		}
		TCHAR* expectedString;
		if(dwDeviceType & STORAGE_DEVICE_TYPE_PCCARD)
		{
			expectedString = 	ATAPI_CF_DISK_PROFILE;	
		}
		else
		{
			expectedString = ATAPI_HD_DISK_PROFILE;
		}
		if(_tcscmp(expectedString,g_storageDeviceInfo.szProfile) == 0)
		{
			//make sure device flags are set properly
			PSTORAGEDEVICEINFO pMyInfo = (PSTORAGEDEVICEINFO)inData.lpInBuffer;
			if(_tcscmp(g_szProfile,_T("PCMCIA")) == 0)
			{
				if(pMyInfo->dwDeviceType != (STORAGE_DEVICE_TYPE_PCCARD | STORAGE_DEVICE_TYPE_ATA | STORAGE_DEVICE_TYPE_REMOVABLE_DRIVE))
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceType is not STORAGE_DEVICE_TYPE_PCCARD and STORAGE_DEVICE_TYPE_ATA and STORAGE_DEVICE_TYPE_REMOVABLE_DRIVE as expected\n"));
					retVal = FALSE;
				}
				if(pMyInfo->dwDeviceClass != STORAGE_DEVICE_CLASS_BLOCK)
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceClass is not STORAGE_DEVICE_CLASS_BLOCK as expected\n"));
					retVal = FALSE;
				}
				if(pMyInfo->dwDeviceFlags  != STORAGE_DEVICE_FLAG_READWRITE)
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceFlags is not STORAGE_DEVICE_FLAG_READWRITE as expected\n"));
					retVal = FALSE;
				}
			}
			else if(_tcscmp(g_szProfile,_T("HDProfile")) == 0)
			{
				if(pMyInfo->dwDeviceType != (STORAGE_DEVICE_TYPE_PCIIDE | STORAGE_DEVICE_TYPE_ATA))
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceType is not STORAGE_DEVICE_TYPE_PCIIDE and STORAGE_DEVICE_TYPE_ATA as expected\n"));
					retVal = FALSE;
				}
				if(pMyInfo->dwDeviceClass != STORAGE_DEVICE_CLASS_BLOCK)
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceClass is not STORAGE_DEVICE_CLASS_BLOCK as expected\n"));
					retVal = FALSE;
				}
				if(pMyInfo->dwDeviceFlags  != STORAGE_DEVICE_FLAG_READWRITE)
				{
					//fail
					m_pKato->Log(LOG_FAIL,_T("dwDeviceFlags is not STORAGE_DEVICE_FLAG_READWRITE as expected\n"));
					retVal = FALSE;
				}
			}
			else if(_tcscmp(g_szProfile,_T("")) == 0)
			{
			//	m_pKato->Log(LOG_DETAIL,_T("No profile selected %s\n"),g_szProfile);
			}
			else
			{
				//fail
					m_pKato->Log(LOG_FAIL,_T("UNKNOWN PROFILE %s\n"),g_szProfile);
					retVal = FALSE;
			}
		}
		else
		{
			//fail, because we have an invalid profile
			m_pKato->Log(LOG_FAIL,_T("Profile is not %s as expected: %s\n"),expectedString,g_storageDeviceInfo.szProfile);
			retVal = FALSE;
		}
	}
	return retVal;
}

BOOL IOCTL_DISK_DEVICE_INFO_TEST::Validate(void)
{
	if(dwDeviceType & STORAGE_DEVICE_TYPE_ATAPI || dwDeviceType & STORAGE_DEVICE_TYPE_ATA)
	{
		return ATAPIValidate();
	}
	else
	{
		return FALSE;
	}
}

