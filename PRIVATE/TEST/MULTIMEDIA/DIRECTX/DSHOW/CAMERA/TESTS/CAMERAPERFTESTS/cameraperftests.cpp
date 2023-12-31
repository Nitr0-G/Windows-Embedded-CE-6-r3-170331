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
#include <windows.h>
#include "logging.h"
#include "Globals.h"
#include "captureframework.h"
#include <imaging.h>
#include <GdiplusEnums.h>
#include "testvector.h"
#include "csvlog.h"

CAPTUREFRAMEWORK g_CaptureGraph;
TESTVECTOR g_VectorData;
CAMERACSV g_CSVLogger;

int g_nAudioDeviceCount;
HWND g_hwnd;

AM_MEDIA_TYPE *g_pamtPreview = NULL;
BYTE *g_bPreviewData = NULL;

AM_MEDIA_TYPE *g_pamtStill = NULL;
BYTE *g_bStillData = NULL;

AM_MEDIA_TYPE *g_pamtCapture = NULL;
BYTE *g_bCaptureData = NULL;

AM_MEDIA_TYPE *g_pamtAudio = NULL;
BYTE *g_bAudioData = NULL;


static TCHAR g_szClassName[] = TEXT("CameraTest");

void
pumpMessages(void)
{
    MSG     msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT APIENTRY WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;

    switch(msg)
    {
        // handle the WM_SETTINGCHANGE message indicating rotation, move the window.
        case WM_SETTINGCHANGE:
            if(wParam == SETTINGCHANGE_RESET)
            {
                SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
                MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);

}

HWND GetWindow()
{
    HWND hwnd;
    WNDCLASS wc;

    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc = (WNDPROC) WndProc;
    wc.hInstance = globalInst;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = g_szClassName;

    // RegisterClass can fail if the previous test bombed out on cleanup (due to an exception or such).
    // so, if we allow ourselves to continue even if the registering failed, 
    // we might be able to run a test which would otherwise be blocked.
    RegisterClass(&wc);

    hwnd = CreateWindow( g_szClassName, TEXT("Camera Test Application"),
                                        WS_VISIBLE | WS_BORDER,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        NULL, NULL, globalInst, NULL);

    SetFocus(hwnd);
    SetForegroundWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    Sleep(100);                   // NT4.0 needs this
    pumpMessages();
    Sleep(100);                   // NT4.0 needs this

    if(!hwnd)
    {
        ABORT(TEXT("Failed to create a window for testing."));
    }

    return hwnd;
}

void
ReleaseWindow(HWND hwnd)
{
    DestroyWindow(hwnd);
    UnregisterClass(g_szClassName, globalInst);
}

HRESULT SetFormatInfo(cTestVector * VectorData, DWORD dwStream, WCHAR *Name, WCHAR *Recipient, AM_MEDIA_TYPE **pamt, BYTE **bData )
{
    HRESULT hr = S_OK;
    int nCount, nSize;
    double dValue = 0;
    DWORD dwIndex;

    if(!pamt || !bData)
    {
        FAIL(TEXT("Invalid media type or data pointer given"));
        hr = E_FAIL;
        goto cleanup;
    }

    // First, determine if there is an entry for this stream in the configuration file,
    // if there isn't then there's nothing to be done.
    nCount = VectorData->FindEntryIndex(0, Name, Recipient, &dwIndex, &dValue);
    if(nCount > 1)
        FAIL(TEXT("Multiple entries in the configuration file, only using the first."));

    if(nCount == 0)
    {
        Log(TEXT("Entry for %s,%s not found in the configuration file."), Name, Recipient);
        goto cleanup;
    }

    // now we retrieve the capabilities for the stream, after we know that the configuration file contains a request to change the stream.
    // This is necessary in situations like a device which doesn't contain any audio driver.
    // if we can't retrieve the number of capabilities, then it's because there's no preview stream available.
    if(FAILED(hr = g_CaptureGraph.GetNumberOfCapabilities(dwStream, &nCount, &nSize)))
    {
        // the preview stream may fail this if the driver only has two pins.
        if(dwStream != STREAM_PREVIEW)
            FAIL(TEXT("Failed to retrieve the number capbilities available."));
        else hr = S_FALSE;
        goto cleanup;
    }

    // alloc a location to store the format info
    *bData = new BYTE[nSize];
    if(!*bData)
    {
        FAIL(TEXT("Failed to allocate storage for format data"));
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto cleanup;
    }
    if(S_OK != (hr = g_CaptureGraph.GetStreamCaps(dwStream, (int) dValue, pamt, *bData)))
    {
        FAIL(TEXT("Unable to retrieve the stream format specified."));
        hr = E_FAIL;
        goto cleanup;
    }

    if(FAILED(hr = g_CaptureGraph.SetFormat(dwStream, *pamt)))
    {
        FAIL(TEXT("Unable to set the stream format specified."));
        goto cleanup;
    }

cleanup:
    return hr;
}

HRESULT BuildPerfTestGraph(HWND hwnd, RECT *rc, cTestVector * VectorData)
{
    HRESULT hr = S_OK;
    DWORD dwIndex;
    double dValue;
    int nCount;
    VARIANT Var;
    WCHAR *wzName, *wzRecipient;
    DWORD dwComponents = ALL_COMPONENTS;

    VariantInit(&Var);

    if(!VectorData)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // get the components test information
    nCount = VectorData->FindEntryIndex(0, L"Components", L"*", &dwIndex, &dValue);
    // if something was retrieved, then use it.  If nothing was found, then fall back to the default.
    if(nCount)
        dwComponents = (DWORD) dValue;
    if(nCount > 1)
        FAIL(TEXT("Multiple \"Components\" entries in the configuration file, only using the first."));

    // if audio isn't available in the system, then remove it.
    if(g_nAudioDeviceCount <= 0)
    {
        dwComponents  &= ~AUDIO_CAPTURE_FILTER;
    }

    // init the core
    if(FAILED(hr = g_CaptureGraph.InitCore(hwnd, rc, dwComponents)))
    {
        FAIL(TEXT("Initializing the capture graph failed."));
        goto cleanup;
    }

    // audio format #, only do this if the audio capture filter is requested
    if((dwComponents & AUDIO_CAPTURE_FILTER) && FAILED(hr = SetFormatInfo(VectorData, STREAM_AUDIO, L"AudioFormat", L"AudioCaptureFilter", &g_pamtAudio, &g_bAudioData)))
    {
        FAIL(TEXT("Failed set the format info for the audio stream."));
        goto cleanup;
    }

    // preview format #
    if(FAILED(hr = SetFormatInfo(VectorData, STREAM_PREVIEW, L"PreviewFormat", L"VideoCaptureFilter", &g_pamtPreview, &g_bPreviewData)))
    {
        FAIL(TEXT("Failed set the format info for the preview stream."));
        goto cleanup;
    }

    // still format #
    if(FAILED(hr = SetFormatInfo(VectorData, STREAM_STILL, L"StillImageFormat", L"VideoCaptureFilter", &g_pamtStill, &g_bStillData)))
    {
        FAIL(TEXT("Failed set the format info for the still stream."));
        goto cleanup;
    }

    // cap format #
    if(FAILED(hr = SetFormatInfo(VectorData, STREAM_CAPTURE, L"CaptureFormat", L"VideoCaptureFilter", &g_pamtCapture, &g_bCaptureData)))
    {
        FAIL(TEXT("Failed set the format info for the capture stream."));
        goto cleanup;
    }

    // video encoder properties
    for(dwIndex = 0; VectorData->FindEntryIndex(dwIndex, L"*", L"VideoEncoder", &dwIndex, NULL) > 0; dwIndex++)
    {
        if(SUCCEEDED(hr = VectorData->GetEntry(dwIndex, &wzName, &wzRecipient, &dValue)))
        {
            Log(TEXT("Setting VideoEncoder property %s"), wzName);

            // video encoder properties
            VariantClear(&Var);
            Var.vt = VT_I4;
            Var.lVal = (LONG) dValue;
            hr = g_CaptureGraph.SetVideoEncoderProperty(wzName, &Var);
            if(FAILED(hr))
            {
                FAIL(TEXT("Failed to set encoder quality property."));
                goto cleanup;
            }
        }
        else
        {
            FAIL(TEXT("Failed to retrieve the entry returned from FindEntryIndex"));
            goto cleanup;
        }
    }

    // audio encoder properties
    for(dwIndex = 0; VectorData->FindEntryIndex(dwIndex, L"*", L"AudioEncoder", &dwIndex, NULL) > 0; dwIndex++)
    {
        if(SUCCEEDED(hr = VectorData->GetEntry(dwIndex, &wzName, &wzRecipient, &dValue)))
        {
            Log(TEXT("Setting AudioEncoder property %s"), wzName);

            // video encoder properties
            VariantClear(&Var);
            Var.vt = VT_I4;
            Var.lVal = (LONG) dValue;
            hr = g_CaptureGraph.SetAudioEncoderProperty(wzName, &Var);
            if(FAILED(hr))
            {
                FAIL(TEXT("Failed to set encoder quality property."));
                goto cleanup;
            }
        }
        else
        {
            FAIL(TEXT("Failed to retrieve the entry returned from FindEntryIndex"));
            goto cleanup;
        }
    }

    // set the orientation
    nCount = VectorData->FindEntryIndex(0, L"Orientation", L"*", &dwIndex, &dValue);
    if(nCount > 1)
        FAIL(TEXT("Multiple \"Orientation\"entries in the configuration file, only using the first."));
    if(nCount > 0)
    {
        if(dValue == 0)
            dwIndex = DMDO_0;
        else if(dValue == 90)
            dwIndex = DMDO_90;
        else if(dValue == 180)
            dwIndex = DMDO_180;
        else if(dValue == 270)
            dwIndex = DMDO_270;

        g_CaptureGraph.SetScreenOrientation(dwIndex);
    }

    // finalize the graph
    if(FAILED(hr = g_CaptureGraph.FinalizeGraph()))
    {
        if(hr != E_GRAPH_UNSUPPORTED)
            FAIL(TEXT("Failed to finalize the graph, unable to continue."));
        goto cleanup;
    }

    // setup the file names after the graph has been finalized.
    // set the still file destination
    if(nCount = VectorData->FindEntryIndex(0, L"StillImageDestination", L"*", &dwIndex, &dValue))
    {
        if(nCount > 1)
            FAIL(TEXT("Multiple \"StillImageDestination\"entries in the configuration file, only using the first."));

        if(SUCCEEDED(hr = VectorData->GetEntry(dwIndex, &wzName, &wzRecipient, &dValue)))
        {
            if(FAILED(hr = g_CaptureGraph.SetStillCaptureFileName(wzRecipient)))
            {
                FAIL(TEXT("Failed to set the still image file name"));
                goto cleanup;
            }
        }
        else
        {
            FAIL(TEXT("Failed to retrieve the entry returned from FindEntryIndex"));
            goto cleanup;
        }
    }
    else if(FAILED(hr = g_CaptureGraph.SetStillCaptureFileName(TEXT("\\StillImage%d.jpg"))))
    {
        FAIL(TEXT("Failed to set the still image file name"));
        goto cleanup;
    }

    // set the capture file destination
    if(nCount = VectorData->FindEntryIndex(0, L"CaptureFileDestination", L"*", &dwIndex, &dValue))
    {
        if(nCount > 1)
            FAIL(TEXT("Multiple \"CaptureFileDestination\"entries in the configuration file, only using the first."));

        if(SUCCEEDED(hr = VectorData->GetEntry(dwIndex, &wzName, &wzRecipient, &dValue)))
        {
            if(FAILED(hr = g_CaptureGraph.SetVideoCaptureFileName(wzRecipient)))
            {
                FAIL(TEXT("Failed to set the video capture file name"));
                goto cleanup;
            }
        }
        else
        {
            FAIL(TEXT("Failed to retrieve the entry returned from FindEntryIndex"));
            goto cleanup;
        }
    }
    else if(FAILED(hr = g_CaptureGraph.SetVideoCaptureFileName(TEXT("\\VideoCapture%d.asf"))))
    {
        FAIL(TEXT("Failed to set the video capture file name"));
        goto cleanup;
    }


cleanup:
    return hr;
}

HRESULT
CleanupPerfTestGraph()
{
    HRESULT hr = S_OK;

    if(g_pamtAudio)
    {
        DeleteMediaType(g_pamtAudio);
        g_pamtAudio = NULL;
    }
    if(g_bAudioData)
    {
        delete[] g_bAudioData;
        g_bAudioData = NULL;
    }

    if(g_pamtPreview)
    {
        DeleteMediaType(g_pamtPreview);
        g_pamtPreview = NULL;
    }
    if(g_bPreviewData)
    {
        delete[] g_bPreviewData;
        g_bPreviewData = NULL;
    }

    if(g_pamtStill)
    {
        DeleteMediaType(g_pamtStill);
        g_pamtStill = NULL;
    }
    if(g_bStillData)
    {
        delete[] g_bStillData;
        g_bStillData = NULL;
    }

   if(g_pamtCapture)
    {
        DeleteMediaType(g_pamtCapture);
        g_pamtCapture = NULL;
    }
    if(g_bCaptureData)
    {
        delete[] g_bCaptureData;
        g_bCaptureData = NULL;
    }

    if(FAILED(hr = g_CaptureGraph.Cleanup()))
    {
        FAIL(TEXT("Cleaning up the graph failed."));
        goto cleanup;
    }

cleanup:
    return hr;
}

TESTPROCAPI PausedGraphPerformanceTests( UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE )
{
    NO_MESSAGES;

    HWND hwnd;
    RECT rc = {0, 0, 0, 0 };
    int nCount = 0;
    int nStallTime;

    if(!lpFTE)
    {
        FAIL(TEXT("No function table entry received"));
        return GetTestResult();
    }

    nStallTime = lpFTE->dwUserData;
    hwnd = GetWindow();

    if(hwnd)
    {
        // initialize the vector data.
        nCount = g_VectorData.IterationCount();

        if(nCount >= 0)
        {
            for(int j = 0; j < nCount; j++)
            {
                Log(TEXT("Iteration %d of %d"), j + 1, nCount);
                // build the test graph
                if(SUCCEEDED(BuildPerfTestGraph(hwnd, &rc, &g_VectorData)))
                {
                    if(FAILED(g_CaptureGraph.RunGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }

                    Sleep(500);

                    if(FAILED(g_CaptureGraph.PauseGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }

                    // do performance work here, must be done after the graph is built
                    if(!g_CSVLogger.StartTest(lpFTE->dwUniqueID, nStallTime, STREAM_PREVIEW))
                        FAIL(TEXT("Failed to start the test logger"));

                    if(!g_CSVLogger.StartRun())
                        FAIL(TEXT("Failed to start the logger run"));

                    // let the preview run for 10 seconds to gather some data.
                    Sleep(nStallTime);

                    if(!g_CSVLogger.StopRun())
                        FAIL(TEXT("Failed to stop the logger run"));

                    // must be done after everything is completed (but before graph teardown)
                    if(!g_CSVLogger.StopTest())
                        FAIL(TEXT("Failed to stop the test logger"));
                }
                else
                {
                    FAIL(TEXT("Failed to build the test graph."));
                }
                
                if(FAILED(CleanupPerfTestGraph()))
                {
                    FAIL(TEXT("Cleaning up the graph failed."));
                }

                // move to the next test vector
                ++g_VectorData;
                Log(TEXT(" "));
            }
        }
        else
        {
            FAIL(TEXT("Iteration count is invalid"));
        }

        ReleaseWindow(hwnd);
    }

    return GetTestResult();
}

TESTPROCAPI PreviewPerformanceTests( UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE )
{
    NO_MESSAGES;

    HWND hwnd;
    RECT rc = {0, 0, 0, 0 };
    int nCount = 0;
    int nRunTime;

    if(!lpFTE)
    {
        FAIL(TEXT("No function table entry received"));
        return GetTestResult();
    }

    nRunTime = lpFTE->dwUserData;
    hwnd = GetWindow();

    if(hwnd)
    {
        // initialize the vector data.
        nCount = g_VectorData.IterationCount();

        if(nCount >= 0)
        {
            for(int j = 0; j < nCount; j++)
            {
                Log(TEXT("Iteration %d of %d"), j + 1, nCount);
                // build the test graph
                if(SUCCEEDED(BuildPerfTestGraph(hwnd, &rc, &g_VectorData)))
                {
                    // do performance work here, must be done after the graph is built
                    if(!g_CSVLogger.StartTest(lpFTE->dwUniqueID, nRunTime, STREAM_PREVIEW))
                        FAIL(TEXT("Failed to start the test logger"));

                    if(FAILED(g_CaptureGraph.RunGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }
                    else
                    {

                        if(!g_CSVLogger.StartRun())
                            FAIL(TEXT("Failed to start the logger run"));

                        // let the preview run for 10 seconds to gather some data.
                        Sleep(nRunTime);

                        if(!g_CSVLogger.StopRun())
                            FAIL(TEXT("Failed to stop the logger run"));
                    }

                    // must be done after everything is completed (but before graph teardown)
                    if(!g_CSVLogger.StopTest())
                        FAIL(TEXT("Failed to stop the test logger"));
                }
                else
                {
                    FAIL(TEXT("Failed to build the test graph."));
                }
                
                if(FAILED(CleanupPerfTestGraph()))
                {
                    FAIL(TEXT("Cleaning up the graph failed."));
                }

                // move to the next test vector
                ++g_VectorData;
                Log(TEXT(" "));
            }
        }
        else
        {
            FAIL(TEXT("Iteration count is invalid"));
        }

        ReleaseWindow(hwnd);
    }

    return GetTestResult();
}

TESTPROCAPI StillPerformanceTests( UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE )
{
    NO_MESSAGES;


    HWND hwnd;
    RECT rc = {0, 0, 0, 0 };
    int nCount = 0;
    int nStillImageCount;
    TCHAR tszFileName[MAX_PATH] = {NULL};
    int nStringLength;
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    if(!lpFTE)
    {
        FAIL(TEXT("No function table entry received"));
        return GetTestResult();
    }

    nStillImageCount = lpFTE->dwUserData;
    hwnd = GetWindow();

    if(hwnd)
    {
        // initialize the vector data.
        nCount = g_VectorData.IterationCount();

        if(nCount >= 0)
        {
            for(int j = 0; j < nCount; j++)
            {
                Log(TEXT("Iteration %d of %d"), j + 1, nCount);
                // build the test graph
                if(SUCCEEDED(BuildPerfTestGraph(hwnd, &rc, &g_VectorData)))
                {
                    // do performance work here, must be done after the graph is built
                    if(!g_CSVLogger.StartTest(lpFTE->dwUniqueID, nStillImageCount, STREAM_STILL))
                        FAIL(TEXT("Failed to start the test logger"));

                    if(FAILED(g_CaptureGraph.RunGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }
                    else
                    {
                        for(int i = 0; i < nStillImageCount; i++)
                        {
                            if(!g_CSVLogger.StartRun())
                                FAIL(TEXT("Failed to start the logger run"));

                            if(FAILED(g_CaptureGraph.CaptureStillImage()))
                            {
                                FAIL(TEXT("The still image capture failed."));
                            }

                            if(!g_CSVLogger.StopRun())
                                FAIL(TEXT("Failed to stop the logger run"));

                            nStringLength = countof(tszFileName);
                            if(FAILED(g_CaptureGraph.GetStillCaptureFileName(tszFileName, &nStringLength)))
                            {
                               FAIL(TEXT("Failed to retrieve the still capture file name."));
                            }

                            hFile = FindFirstFile(tszFileName, &fd);
                            if(hFile == INVALID_HANDLE_VALUE)
                            {
                                FAIL(TEXT("Failed to find the image file."));
                            }
                            else if(FALSE == FindClose(hFile))
                            {
                                FAIL(TEXT("Failed to close the image file handle."));
                            }

                            if(FALSE == DeleteFile(tszFileName))
                            {
                                FAIL(TEXT("Failed to delete the still image file."));
                            }
                        }
                    }

                    // must be done after everything is completed (but before graph teardown)
                    if(!g_CSVLogger.StopTest())
                        FAIL(TEXT("Failed to stop the test logger"));
                }
                else
                {
                    FAIL(TEXT("Failed to build the test graph."));
                }
                
                if(FAILED(CleanupPerfTestGraph()))
                {
                    FAIL(TEXT("Cleaning up the graph failed."));
                }

                // move to the next test vector
                ++g_VectorData;
                Log(TEXT(" "));
            }
        }
        else
        {
            FAIL(TEXT("Iteration count is invalid"));
        }

        ReleaseWindow(hwnd);
    }

    return GetTestResult();
}

 TESTPROCAPI StillBurstPerformanceTests( UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE )
{
    NO_MESSAGES;


    HWND hwnd;
    RECT rc = {0, 0, 0, 0 };
    int nCount = 0;
    int nStillImageCount;

    if(!lpFTE)
    {
        FAIL(TEXT("No function table entry received"));
        return GetTestResult();
    }

    nStillImageCount = lpFTE->dwUserData;
    hwnd = GetWindow();

    if(hwnd)
    {
        // initialize the vector data.
        nCount = g_VectorData.IterationCount();

        if(nCount >= 0)
        {
            for(int j = 0; j < nCount; j++)
            {
                Log(TEXT("Iteration %d of %d"), j + 1, nCount);
                // build the test graph
                if(SUCCEEDED(BuildPerfTestGraph(hwnd, &rc, &g_VectorData)))
                {
                    // do performance work here, must be done after the graph is built
                    if(!g_CSVLogger.StartTest(lpFTE->dwUniqueID, nStillImageCount, STREAM_STILL))
                        FAIL(TEXT("Failed to start the test logger"));

                    if(FAILED(g_CaptureGraph.RunGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }
                    else
                    {
                        // start the run, trigger all of the still images,
                        // and then wait for all of the still image events to come back.
                        if(!g_CSVLogger.StartRun())
                            FAIL(TEXT("Failed to start the logger run"));

                            if(FAILED(g_CaptureGraph.CaptureStillImageBurst(nStillImageCount)))
                            {
                                FAIL(TEXT("The still image capture failed."));
                            }

                        if(!g_CSVLogger.StopRun())
                            FAIL(TEXT("Failed to stop the logger run"));

                        // delete all sill image files
                        DShowEvent dse;
                        DShowEvent *dseReceived;

                        dse.Code = EC_CAP_FILE_COMPLETED;
                        dse.FilterFlags = EVT_EVCODE_EQUAL;
                        dseReceived = g_CaptureGraph.FindFirstEvent(1, &dse);
                        do
                        {
                            if(dseReceived && dseReceived->Code == EC_CAP_FILE_COMPLETED)
                            {
                                if(dseReceived->Param2 && FALSE == DeleteFile((TCHAR *) dseReceived->Param2))
                                {
                                    FAIL(TEXT("Failed to delete the still image file."));
                                }
                            }

                            dseReceived = g_CaptureGraph.FindNextEvent();
                        } while(dseReceived);
                    }

                    // must be done after everything is completed (but before graph teardown)
                    if(!g_CSVLogger.StopTest())
                        FAIL(TEXT("Failed to stop the test logger"));
                }
                else
                {
                    FAIL(TEXT("Failed to build the test graph."));
                }
                
                if(FAILED(CleanupPerfTestGraph()))
                {
                    FAIL(TEXT("Cleaning up the graph failed."));
                }

                // move to the next test vector
                ++g_VectorData;
                Log(TEXT(" "));
            }
        }
        else
        {
            FAIL(TEXT("Iteration count is invalid"));
        }

        ReleaseWindow(hwnd);
    }

    return GetTestResult();
}
 
TESTPROCAPI CapturePerformanceTests( UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE )
{
    NO_MESSAGES;

    HWND hwnd;
    RECT rc = {0, 0, 0, 0 };
    int nCount = 0;
    int nCaptureLength;
    TCHAR tszFileName[MAX_PATH] = {NULL};
    int nStringLength;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    HRESULT hr = S_OK;

    if(!lpFTE)
    {
        FAIL(TEXT("No function table entry received"));
        return GetTestResult();
    }

    nCaptureLength = lpFTE->dwUserData;
    hwnd = GetWindow();

    if(hwnd)
    {
        // initialize the vector data.
        nCount = g_VectorData.IterationCount();

        if(nCount >= 0)
        {
            for(int j = 0; j < nCount; j++)
            {
                Log(TEXT("Iteration %d of %d"), j + 1, nCount);
                // build the test graph
                if(SUCCEEDED(hr = BuildPerfTestGraph(hwnd, &rc, &g_VectorData)))
                {
                    // do performance work here, must be done after the graph is built
                    if(!g_CSVLogger.StartTest(lpFTE->dwUniqueID, nCaptureLength, STREAM_CAPTURE))
                        FAIL(TEXT("Failed to start the test logger"));

                    if(FAILED(g_CaptureGraph.RunGraph()))
                    {
                        FAIL(TEXT("Running the capture graph failed."));
                    }
                    else
                    {

                        if(!g_CSVLogger.StartRun())
                            FAIL(TEXT("Failed to start the logger run"));

                        Log(TEXT("Capture length %d seconds"), nCaptureLength/1000);

                        Log(TEXT("  Capture starting"));

                        // loop through a few times verifying the capture was correct.
                        if(FAILED(g_CaptureGraph.StartStreamCapture()))
                        {
                            FAIL(TEXT("Failed to start the stream capture."));
                        }
                        else
                        {
                            Log(TEXT("  Capture running"));

                            Sleep(nCaptureLength);

                            Log(TEXT("  Capture complete"));

                            // make sure we properly handle the case where we ran out of memory while doing the test.
                            hr = g_CaptureGraph.StopStreamCapture();
                            if(FAILED(hr))
                            {
                                if(hr == E_WRITE_ERROR_EVENT)
                                    Log(TEXT("received a write error event while doing the capture (out of storage space?)."));
                                else if(hr == E_OOM_EVENT)
                                    Log(TEXT("received an OOM event while doing the capture (insufficient memory for this capture length?)."));
                                else
                                    FAIL(TEXT("Failed to stop the stream capture."));
                            }

                            if(FAILED(g_CaptureGraph.StopGraph()))
                            {
                                FAIL(TEXT("Failed to stop the graph after capture"));
                            }

                            Log(TEXT("  Capture stream stopped"));
                        }

                        if(!g_CSVLogger.StopRun())
                            FAIL(TEXT("Failed to stop the logger run"));

                        // the test run completed, delete the file
                        nStringLength = countof(tszFileName);
                        if(FAILED(g_CaptureGraph.GetVideoCaptureFileName(tszFileName, &nStringLength)))
                        {
                           FAIL(TEXT("Failed to retrieve the video capture file name."));
                        }

                        if(FAILED(g_CaptureGraph.VerifyVideoFileCaptured(tszFileName)))
                        {
                            FAIL(TEXT("The video file captured failed file conformance verification"));
                        }

                        hFile = FindFirstFile(tszFileName, &fd);
                        if(hFile == INVALID_HANDLE_VALUE)
                        {
                            FAIL(TEXT("Failed to find the video capture file."));
                        }
                        else if(FALSE == FindClose(hFile))
                        {
                            FAIL(TEXT("Failed to close the video capture file handle."));
                        }

                        if(FALSE == DeleteFile(tszFileName))
                        {
                            FAIL(TEXT("Failed to delete the video capture file."));
                        }
                    }

                    // must be done after everything is completed (but before graph teardown)
                    if(!g_CSVLogger.StopTest())
                        FAIL(TEXT("Failed to stop the test logger"));
                }
                else
                {
                    if(hr != E_GRAPH_UNSUPPORTED)
                        FAIL(TEXT("Failed to build the test graph."));
                    else Log(TEXT("This graph is unsupported, continuing test."));
                }
                
                if(FAILED(CleanupPerfTestGraph()))
                {
                    FAIL(TEXT("Cleaning up the graph failed."));
                }

                // move to the next test vector
                ++g_VectorData;
                Log(TEXT(" "));
            }
        }
        else
        {
            FAIL(TEXT("Iteration count is invalid"));
        }

        ReleaseWindow(hwnd);
    }

    return GetTestResult();
}
