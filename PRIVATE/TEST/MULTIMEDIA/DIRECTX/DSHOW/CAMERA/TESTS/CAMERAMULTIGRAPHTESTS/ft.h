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
////////////////////////////////////////////////////////////////////////////////
//
//  TUXTEST TUX DLL
//
//  Module: ft.h
//          Declares the TUX function table and test function prototypes EXCEPT
//          when included by globals.cpp, in which case it DEFINES the function
//          table.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

// needed for the camera components
#include "captureframework.h"
#include "playbackframework.h"
#include "cameramultigraphtests.h"

#if (!defined(__FT_H__) || defined(__GLOBALS_CPP__))
#ifndef __FT_H__
#define __FT_H__
#endif

////////////////////////////////////////////////////////////////////////////////
// Local macros

#ifdef __DEFINE_FTE__
#undef BEGIN_FTE
#undef FTE
#undef FTH
#undef END_FTE
#define BEGIN_FTE FUNCTION_TABLE_ENTRY g_lpFTE[] = {
#define FTH(a, b) { TEXT(b), a, 0, 0, NULL },
#define FTE(a, b, c, d, e) { TEXT(b), a, d, c, e },
#define END_FTE { NULL, 0, 0, 0, NULL } };
#else // __DEFINE_FTE__
#ifdef __GLOBALS_CPP__
#define BEGIN_FTE
#else // __GLOBALS_CPP__
#define BEGIN_FTE extern FUNCTION_TABLE_ENTRY g_lpFTE[];
#endif // __GLOBALS_CPP__
#define FTH(a, b)
#define FTE(a, b, c, d, e) TESTPROCAPI e(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
#define END_FTE
#endif // __DEFINE_FTE__

////////////////////////////////////////////////////////////////////////////////
// TUX Function table
//  To create the function table and function prototypes, two macros are
//  available:
//
//      FTH(level, description)
//          (Function Table Header) Used for entries that don't have functions,
//          entered only as headers (or comments) into the function table.
//
//      FTE(level, description, code, param, function)
//          (Function Table Entry) Used for all functions. DON'T use this macro
//          if the "function" field is NULL. In that case, use the FTH macro.
//
//  You must not use the TEXT or _T macros here. This is done by the FTH and FTE
//  macros.
//
//  In addition, the table must be enclosed by the BEGIN_FTE and END_FTE macros.

BEGIN_FTE
    FTH(0, "Capture playback tests")
    FTE(1,     "Capture playback test, all components.", 100, ALL_COMPONENTS, CapturePlaybackTest)
    FTE(1,     "Capture playback test, audio only.", 101, AUDIO_CAPTURE_FILTER | FILE_WRITER | AUDIO_ENCODER, CapturePlaybackTest)
    FTE(1,     "Capture playback test, video only.", 102, FILE_WRITER | VIDEO_CAPTURE_FILTER | VIDEO_ENCODER, CapturePlaybackTest)

    FTH(0, "Multigraph capture during playback tests")
    FTE(1,     "Multigraph capture during playback test, all components.", 200, ALL_COMPONENTS, MultigraphCaptureAndPlaybackTest)

    FTH(0, "MultiThreadedBuildTest")
    FTE(1,     "MultiThreadedBuildTest, all components, 2 threads, parent/child windows.", 300, 0, MultiThreadedBuildTest)
    FTE(1,     "MultiThreadedBuildTest, all components, 2 threads, individual windows.", 301, 1, MultiThreadedBuildTest2)

END_FTE

////////////////////////////////////////////////////////////////////////////////

#endif // !__FT_H__ || __GLOBALS_CPP__
