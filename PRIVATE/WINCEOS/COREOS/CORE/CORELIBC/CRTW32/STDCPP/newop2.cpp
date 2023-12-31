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

// newop2 operator new(size_t, const nothrow_t&) for Microsoft C++

// Disable "C++ Exception Specification ignored" warning in OS build (-W3)
#pragma warning( disable : 4290 )  

#include <cstdlib>
#include <new>

extern "C" {
int _callnewh(size_t size);
}

void *operator new(size_t size, const std::nothrow_t&) throw()
	{	// try to allocate size bytes
	void *p;
	while ((p = malloc(size)) == 0)
		{	// buy more memory or return null pointer
			try {
				if (_callnewh(size) == 0)
					break;
			} catch (std::bad_alloc) {
				return (0);
			}
		}
	return (p);
	}

/*
 * Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */
