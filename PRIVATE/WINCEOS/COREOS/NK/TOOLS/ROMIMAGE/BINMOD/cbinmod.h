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
#pragma once

#include <windows.h>
#include <string>

using namespace std;

#include <pehdr.h>
#include <romldr.h>

#define MAX_RECORDS 2048
#define BUFFER_SIZE  64*1024

typedef struct _RECORD_DATA {
  DWORD dwStartAddress;
  DWORD dwLength;
  DWORD dwChecksum;
  DWORD dwFilePointer;
}RECORD_DATA, *PRECORD_DATA;

class CBinMod
{
protected:
  BYTE pBuffer[BUFFER_SIZE];
  string image;
  HANDLE hFile;
  FILE *fpLog;
  
  DWORD pTOC;
  DWORD dwImageStart;
  DWORD dwImageLength;
  DWORD dwNumRecords;
  DWORD dwROMOffset;
  
  RECORD_DATA Records[MAX_RECORDS];

  DWORD FindHole(DWORD size, ROMHDR *pToc);
  bool ReWriteCheckSum(DWORD dwAddress);
  bool ComputeRomOffset(DWORD &state);
  bool AccessBinFile(PBYTE pBuffer, DWORD dwAddress, DWORD dwLength, DWORD dwROMOffsetRead, bool bWrite);
  bool GetNewFileData(BYTE **buffer, DWORD *csize, DWORD *rsize, const char *file_name, bool compressed);

  virtual DWORD my_SetFilePointer(HANDLE handle, LONG dist, PLONG dist_high, DWORD method)
  {
    return SetFilePointer(handle, dist, dist_high, method);
  };
  virtual BOOL my_ReadFile(HANDLE handle, LPVOID buffer, DWORD bytes, LPDWORD cb, LPOVERLAPPED ol)
  {
    return ReadFile( handle,  buffer,  bytes,  cb, ol);
  };
  virtual BOOL my_WriteFile(HANDLE handle, LPVOID buffer, DWORD bytes, LPDWORD cb, LPOVERLAPPED ol)
  {
    return WriteFile( handle,  buffer,  bytes,  cb, ol);
  };
  virtual BOOL my_FlushFileBuffers(HANDLE hFile)
  {
    return FlushFileBuffers( hFile);
  };
  virtual HANDLE my_CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
  {
    return CreateFile( lpFileName,  dwDesiredAccess,  dwShareMode,  lpSecurityAttributes,  dwCreationDisposition,  dwFlagsAndAttributes,  hTemplateFile);
  };
  virtual BOOL my_CloseHandle(HANDLE hObject)
  {
    return CloseHandle( hObject);
  };
  virtual DWORD my_GetFileSize(HANDLE handle, LPDWORD lpFileSizeHigh)
  {
    return GetFileSize( handle,  lpFileSizeHigh);
  };

  virtual DWORD my_GetFileAttributes(LPCTSTR lpFileName)
  {
    return GetFileAttributes(lpFileName);
  };

  void dprintf(char *message, ...);
  
public:    
  CBinMod(bool log2file = false);
  ~CBinMod();

  bool Init(string _image);
  bool Replace(string _file);
  bool Extract(string _file);
};

