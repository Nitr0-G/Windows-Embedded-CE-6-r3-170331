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
#include <corecrt.h>

void __cdecl _swab(char *src, char *dest, int nbytes) {
	char b1, b2;
	while (nbytes > 1) {
		b1 = *src++;
		b2 = *src++;
		*dest++ = b2;
		*dest++ = b1;
		nbytes -= 2;
	}
}

