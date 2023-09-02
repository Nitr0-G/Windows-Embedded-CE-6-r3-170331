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
#include "d3dm.h"

typedef
HRESULT
(*SET_STATES) (
	LPDIRECT3DMOBILEDEVICE pDevice,
	DWORD dwTableIndex,
	BOOL bAlphaBlend,
	BYTE DiffuseAlpha
	);

typedef
HRESULT
(*SUPPORTS_TABLE_INDEX) (
	LPDIRECT3DMOBILEDEVICE pDevice,
	DWORD dwTableIndex,
	BOOL bAlphaBlend
	);

typedef
HRESULT
(*SETUP_GEOMETRY) (
	LPDIRECT3DMOBILEDEVICE pDevice,
	HWND hWnd,
	BYTE DiffuseAlpha,
	LPDIRECT3DMOBILEVERTEXBUFFER *ppVB
	);
