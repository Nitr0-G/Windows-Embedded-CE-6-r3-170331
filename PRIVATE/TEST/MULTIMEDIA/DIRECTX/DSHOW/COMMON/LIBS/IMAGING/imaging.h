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
#ifndef IMAGING_H
#define IMAGING_H

#include <streams.h>

extern HINSTANCE globalInst;

#define FOURCC_YVYU MAKEFOURCC('Y','V','Y','U')
#define FOURCC_UYVY MAKEFOURCC('U','Y','V','Y')
#define FOURCC_YUY2 MAKEFOURCC('Y','U','Y','2')
#define FOURCC_YV12 MAKEFOURCC('Y','V','1','2')
#define FOURCC_YV16 MAKEFOURCC('Y','V','1','6')
#define FOURCC_AYUV MAKEFOURCC('A','Y','U','V')

// This is what our color converter seems to be using
//B0 = ((38142 * (Y0 - 16))                        + (66126 * (V - 128))) / 32768;
//G0 = ((38142 * (Y0 - 16)) - (5997  * (U - 128))  - (12812 * (V - 128))) / 32768;
//R0 = ((38142 * (Y0 - 16)) + (52298 * (U - 128))) / 32768;

// Which can also be written as
#define RGB2YUV(r, g, b, y, u, v) \
y = (0.257f*r) + (0.504f*g) + (0.098f*b) + 16; \
u = -(0.148f*r) - (0.291f*g) + (0.439f*b) + 128; \
v = (0.439f*r) - (0.368f*g) - (0.071f*b) + 128;

#define YUV2RGB(y, u, v, r, g, b) \
r = 1.164f*(y - 16) + 1.596f*(v - 128); \
g = 1.164f*(y - 16) - 0.813f*(v - 128) - 0.391f*(u - 128); \
b = 1.164f*(y - 16)                  + 2.018f*(u - 128)


//#define RGB2YUV(r, g, b, y, u, v)		\
//	y = 0.299f*r + 0.587f*g + 0.114f*b;	\
//	u = 0.596f*r - 0.274f*g - 0.322f*b;	\
//	v = 0.212f*r - 0.523f*g + 0.311f*b;
//
//#define YUV2RGB(y, u, v, r, g, b)	\
//	r = y + 0.956f*u + 0.621f*v;		\
//	g = y - 0.272f*u - 0.647f*v;		\
//	b = y - 1.105f*u + 1.702f*v;

#define CLAMP(x, lo, hi)		\
	if (x < lo)					\
		x = lo;					\
	if (x > hi)					\
		x = hi;


#define INTERMEDIATE_RGB24_CONVERT(FormatX, FormatY, pSrcBits, srcBmi, srcStride, srcX, srcY, convWidth, convHeight, pDstBits, dstBmi, dstStride, dstX, dstY)        \
{                                                                                            \
    BYTE* pRGB24Bits = NULL;                                                                \
    BITMAPINFO rgb24bmi; \
    memset(&rgb24bmi, 0, sizeof(BITMAPINFO));   \
    rgb24bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);  \
    rgb24bmi.bmiHeader.biWidth = convWidth; \
    rgb24bmi.bmiHeader.biHeight = convHeight;   \
    rgb24bmi.bmiHeader.biPlanes = 1;    \
    rgb24bmi.bmiHeader.biBitCount = 24; \
    rgb24bmi.bmiHeader.biCompression = BI_RGB;  \
    rgb24bmi.bmiHeader.biSizeImage = convWidth * convHeight;   \
    AllocateBitmapBuffer(convWidth, convHeight, 24, BI_RGB, &pRGB24Bits);                            \
    if (!pRGB24Bits)        \
        return E_FAIL;        \
    HRESULT hr = FormatX##_To_RGB24(pSrcBits, srcBmi, srcStride, srcX, srcY, convWidth, convHeight, pRGB24Bits, &rgb24bmi, 0, 0, 0);                                \
    if (FAILED(hr))    \
    {                \
        delete pRGB24Bits;    \
        return hr;        \
    }    \
    hr = RGB24_To_##FormatY(pRGB24Bits, &rgb24bmi, 0, 0, 0, convWidth, convHeight, pDstBits, dstBmi, dstStride, dstX, dstY);                                \
    if (pRGB24Bits)                                                                            \
        delete pRGB24Bits;                                                                    \
    return hr;                                                                            \
}

#define DECLARE_INTERMEDIATE_RGB24_CONVERT(FormatX, FormatY)        \
HRESULT FormatX##_To_##FormatY(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY) \
{ \
    INTERMEDIATE_RGB24_CONVERT(FormatX,FormatY,pSrcBits,srcBmi,srcStride,srcX,srcY,convWidth,convHeight,pDstBits,dstBmi,dstStride,dstX,dstY); \
} \

extern int AllocateBitmapBuffer(int width, int height, WORD bitcount, DWORD compression, BYTE** ppBuffer);

typedef HRESULT (*ImageLoader)(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
typedef float (*ImageDiffer)(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
typedef HRESULT (*ImageConverter)(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight,
								  BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);

struct LoaderTableEntry
{
	GUID subtype;
	ImageLoader loader;
};

struct DifferTableEntry
{
	GUID subtype;
	ImageDiffer differ;
};

struct ConverterTableEntry
{
	GUID subtype1;
	GUID subtype2;
	ImageConverter converter1;
	ImageConverter converter2;
};

// image information
extern HRESULT GetFormatInfo(TCHAR *tszFormatInfo, UINT MaxStringLength, AM_MEDIA_TYPE *pMediaType);


// BITMAP API
HRESULT GetBitmapFileInfo(TCHAR* imagefile, BITMAP* pBitmap);
HRESULT SaveBitmap(TCHAR *tszFileName, BITMAPINFO* pBmiHeader, GUID MediaType, BYTE* pBuffer);
HRESULT SaveRGBBitmap(const TCHAR * tszFileName, BYTE *pPixels, INT iWidth, INT iHeight, INT iWidthBytes);
HRESULT GetBitmapInfo(HBITMAP hBitmap, BITMAP* pBitmap);
BITMAPINFO* GetBitmapInfo(const AM_MEDIA_TYPE* pMediaType);

// Loader API
ImageLoader GetLoader(BITMAPINFO* pBmi, GUID subtype);
extern bool IsImageFile(const TCHAR* file);
extern bool IsVideoFile(const TCHAR* file);
bool IsYUVImage(GUID subtype);
bool IsRGBImage(GUID subtype);
extern HRESULT LoadImage(BYTE* pBuffer, BITMAPINFO* bmi, GUID subtype, const TCHAR* imagefile);
extern HRESULT LoadBitmapFile(BYTE* pBuffer, BITMAPINFO *pBmInfo, const TCHAR* imagefile);
extern HRESULT LoadYVYUImage(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadUYVYImage(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadYUY2Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadYV12Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadYV16Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadRGB24Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadRGB32Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadRGB555Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);
extern HRESULT LoadRGB565Image(BYTE* pBuffer, BITMAPINFO* bmi, const TCHAR* imagefile);

// Differ API
extern float DiffImage(BYTE* pSrcBits, BYTE* pDstBits, BITMAPINFO* pBmi, GUID subtype);
extern float DiffImage(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, BITMAPINFO* pBmi, GUID subtype);
extern ImageDiffer GetDiffer(BITMAPINFO* pBmi, GUID subtype);
extern float DiffRGB32Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffRGB24Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffRGB555Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffRGB565Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffUYVYImage(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffYUY2Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffYVYUImage(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffYV12Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffYV16Image(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);
extern float DiffUYVYImage(BYTE* pSrcBits, int srcStride, BYTE* pDstBits, int dstStride, int width, int height);

// Converter API
extern ImageConverter GetConverter(BITMAPINFO* pSrcBmi, GUID srcsubtype, BITMAPINFO* pDstBmi, GUID dstsubtype);
extern bool CanConvertImage(BITMAPINFO* pSrcBmi, GUID srcsubtype, BITMAPINFO* pDstBmih, GUID dstsubtype);

extern HRESULT ConvertImage(BYTE* pSrcBits, BITMAPINFO* pSrcBmi, GUID srcsubtype, int srcX, int srcY, int convWidth, int convHeight,
							BYTE* pDstBits, BITMAPINFO* pDstBmi, GUID dstsubtype, int dstX, int dstY);

extern HRESULT ConvertImage(BYTE* pSrcBits, BITMAPINFO* pSrcBmi, GUID srcsubtype, int srcX, int srcY, int srcStride, int convWidth, int convHeight,
							BYTE* pDstBits, BITMAPINFO* pDstBmi, GUID dstsubtype, int dstX, int dstY, int dstStride);


// These are real conversions (not through intermediate)
extern HRESULT RGB24_To_YUY2(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_YVYU(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_UYVY(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_YV12(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_YV16(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_AYUV(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_RGB32(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_RGB565(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB24_To_RGB555(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB32_To_RGB32(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB32_To_ARGB32(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YUY2_To_YV12(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YUY2_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YVYU_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT UYVY_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT AYUV_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YV12_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YV16_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YV12_To_YV12(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YV16_To_YV16(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT YUY2_To_YUY2(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT UYVY_To_UYVY(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB555_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB565_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB565_To_RGB565(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);
extern HRESULT RGB32_To_RGB24(BYTE* pSrcBits, BITMAPINFO *srcBmi, int srcStride, int srcX, int srcY, int convWidth, int convHeight, BYTE* pDstBits, BITMAPINFO *dstBmi, int dstStride, int dstX, int dstY);


#endif
