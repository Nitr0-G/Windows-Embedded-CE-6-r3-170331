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

#define         DLL_NAME               TEXT("BaseDll.dll")
#define         DLL_FUNCTION_NAME      TEXT("BaseDllFunction")

BOOL BaseLoadLibrary(LPTSTR lpDllName, HINSTANCE *phModule);
BOOL BaseInvokeLibraryFunction(HINSTANCE hModule, LPTSTR lpProcName);
BOOL BaseFreeLibrary(HINSTANCE hModule);
