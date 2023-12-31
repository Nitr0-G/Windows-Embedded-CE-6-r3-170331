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
//  Graph Event Logger 
//
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "globals.h"
#include "logging.h"
#include "EventLogger.h"
#include "GraphEvent.h"
#include "FilterDesc.h"
#include "TestGraph.h"
#include "ValidType.h"

HRESULT CreateGraphEventLogger(EventLoggerType type, void* pEventLoggerData, TestGraph* pTestGraph, IEventLogger** ppEventLogger)
{
	HRESULT hr = S_OK;
	IEventLogger* pEventLogger = NULL;

	switch(type)
	{
	case GraphEvents:
		pEventLogger = new GraphEventLogger();
		if (!pEventLogger)
			hr = E_OUTOFMEMORY;
		break;
	
	default:
		hr = E_NOTIMPL;
		break;
	};
	
	if (SUCCEEDED(hr))
	{
		hr = pEventLogger->Init(pTestGraph, type, pEventLoggerData);
		if (FAILED(hr))
			delete pEventLogger;
		else 
			*ppEventLogger = pEventLogger;
	}
	
	return hr;
}


GraphEventLogger::GraphEventLogger()
{
	// Set the type to something not valid
	m_EventLoggerType = EventLoggerEndMarker;

	m_nSamples = 0;

	memset(&m_mt, 0, sizeof(m_mt));

	m_hr = S_OK;
}

GraphEventLogger::~GraphEventLogger()
{
}

HRESULT GraphEventLogger::Init(TestGraph* pTestGraph, EventLoggerType type, void* pEventLoggerData)
{
	ITapFilter* pTapFilter = NULL;
	
	GraphEventLoggerData* pGraphEventLoggerData = (GraphEventLoggerData*)pEventLoggerData;

	// BUGBUG: We need a tap filter at the output specified
	HRESULT hr = pTestGraph->InsertTapFilter(pGraphEventLoggerData->filterType, pGraphEventLoggerData->pindir, pGraphEventLoggerData->mediaType, &pTapFilter);
	if (FAILED_F(hr))
	{
		LOG(TEXT("Failed to insert tap filter\n"));
		return hr;
	}

	// Save the media type
	pTapFilter->GetMediaType(&m_mt, PINDIR_INPUT);

	// Register a callback
	hr = pTapFilter->RegisterEventCallback(GenericLoggerCallback, pEventLoggerData, (void*)this);
	if (FAILED_F(hr))
	{
		LOG(TEXT("Failed to register callback\n"));
		return hr;
	}

	// Store the verification requested
	m_EventLoggerType = type;

	// Store the output file name
	_tcsncpy(m_szLogFile, pGraphEventLoggerData->szLogFile, countof(m_szLogFile));

	return S_OK;
}

HRESULT GraphEventLogger::ProcessEvent(GraphEvent event, void* pEventData, void* pCallBackData)
{
	HRESULT hr = S_OK;

	tstring xml = TEXT("");
	tstring prefix = TEXT("\t");

	TCHAR tsz[256];			 			

	if (FAILED_F(m_hr))
		return m_hr;

	if (m_EventLoggerType == GraphEvents)
	{
		if (event == SAMPLE)
		{
			GraphSample* pSample = (GraphSample*)pEventData;
			IMediaSample* pMediaSample = pSample->pMediaSample;

			// Check if the media type has changed
			AM_MEDIA_TYPE* pMediaType = NULL;
			hr = pMediaSample->GetMediaType(&pMediaType);
			if (SUCCEEDED_F(hr))
			{
				const char* szTypeName = GetTypeName(pMediaType);
				szTypeName = (NULL == szTypeName) ? "Undetermined" : szTypeName;
				
				// If there was a media type change - log the change
				_stprintf(tsz, _T("Media Type Change: %S\r\n"), szTypeName);

				// Delete the type
				FreeMediaType(*pMediaType);
				DeleteMediaType(pMediaType);
			}

			TCHAR szDiscontinuity[8] = TEXT(" ");
			TCHAR szSyncPoint[8] = TEXT(" ");
			TCHAR szPreroll[8] = TEXT(" ");

			// Log the sample itself
			// Get the start and stop time
			LONGLONG start, stop;
			hr = pMediaSample->GetTime(&start, &stop);
			if (hr != S_OK)
			{
				LOG(TEXT("Failed to get time\n"));
				m_hr = E_FAIL;
				return m_hr;
			}
			
			bool bSyncPoint = (S_OK == pMediaSample->IsSyncPoint());
			bool bPreroll = (S_OK == pMediaSample->IsPreroll());
			long datalen = pMediaSample->GetActualDataLength();
			bool bDiscontinuity = (S_OK == pMediaSample->IsDiscontinuity());

			if (bSyncPoint)
				_tcscpy(szSyncPoint, TEXT("K"));

			if (bPreroll)
				_tcscpy(szPreroll, TEXT("P"));

			if (bDiscontinuity)
				_tcscpy(szDiscontinuity, TEXT("D"));

			// Output time-stamps
			_stprintf(tsz, _T("S%5d %I64d:%I64d %I64d %d %s%s%s\r\n"), m_nSamples, start, stop, pSample->ats, datalen, szSyncPoint, szPreroll, szDiscontinuity);

#if 0
			// Save the sample itself
			BYTE* pBits = NULL;
			pMediaSample->GetPointer(&pBits);

			TCHAR szfile[64];
			_sntprintf(szfile, countof(szfile), TEXT("\\Hard Disk\\Out\\out%d.bmp"), m_nSamples);
			if (m_nSamples%20 == 0)
				SaveBitmap(szfile, &((VIDEOINFOHEADER*)m_mt.pbFormat)->bmiHeader, pBits);
#endif
			m_nSamples++;
		}
		else if (event == BEGIN_FLUSH)
		{
			GraphBeginFlush* pBeginFlush = (GraphBeginFlush*)pEventData;
			_stprintf(tsz, _T("Begin Flush @ %I64d\r\n"), pBeginFlush->ats);
		}
		else if (event == END_FLUSH)
		{
			GraphEndFlush* pEndFlush = (GraphEndFlush*)pEventData;
			_stprintf(tsz, _T("End Flush @ %I64d\r\n"), pEndFlush->ats);
		}
		else if (event == NEW_SEG)
		{
			GraphNewSegment* pNewSeg = (GraphNewSegment*)pEventData;
			_stprintf(tsz, _T("NewSeg @ %I64d, start: %I64d, stop: %I64d, rate: %f \r\n"), pNewSeg->ats, pNewSeg->start, pNewSeg->stop, pNewSeg->rate);
		}
		else if (event == QUERY_ACCEPT)
		{
			GraphQueryAccept* pGraphQueryAccept = (GraphQueryAccept*)pEventData;
			const char* szTypeName = GetTypeName(pGraphQueryAccept->pMediaType);
			szTypeName = (NULL == szTypeName) ? "Undetermined" : szTypeName;
			_stprintf(tsz, _T("QUERY_ACCEPT @ %I64d, Type: %S, bAccept: %d\r\n"), pGraphQueryAccept->ats, szTypeName, pGraphQueryAccept->bAccept);
		}
		else if (event == EOS)
		{
			GraphEndOfStream* pEndOfStream = (GraphEndOfStream*)pEventData;
			_stprintf(tsz, _T("EOS @ %I64d\r\n"), pEndOfStream->ats);
		}

		// Write the string to file
		DWORD nBytesWritten = 0;
		xml += prefix + tsz;
		WriteFile(m_hFile, (const void*)xml.c_str(), xml.length()*sizeof(TCHAR), &nBytesWritten, NULL);
	}

	return hr;
}

HRESULT GraphEventLogger::Start()
{
	m_hFile = CreateFile(m_szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;
	tstring xml = TEXT("");	
	xml += _T("<TimeStamps>\r\n");
	DWORD nBytesWritten = 0;
	WriteFile(m_hFile, (const void*)xml.c_str(), xml.length()*sizeof(TCHAR), &nBytesWritten, NULL);
	return S_OK;
}

void GraphEventLogger::Stop()
{
	tstring xml = TEXT("");	
	xml += _T("</TimeStamps>\r\n");
	DWORD nBytesWritten = 0;
	WriteFile(m_hFile, (const void*)xml.c_str(), xml.length()*sizeof(TCHAR), &nBytesWritten, NULL);
	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	// BUGBUG: Remove the tap filter
}

void GraphEventLogger::Reset()
{
	// Flush the log so far
	// Reset for a fresh capture
}