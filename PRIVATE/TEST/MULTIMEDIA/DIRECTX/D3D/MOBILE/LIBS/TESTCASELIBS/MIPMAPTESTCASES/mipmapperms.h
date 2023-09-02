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
#include "mipmaptestcases.h"

//
// Test permutation data structure
//
typedef struct _MIPMAPTEST {
	D3DMTEXTUREFILTERTYPE MipFilter;
	D3DMTEXTUREFILTERTYPE MagFilter;
	D3DMTEXTUREFILTERTYPE MinFilter;
	DWORD dwLevel;
	FLOAT fBias;
} MIPMAPTEST, *LPMIPMAPTEST;


// 
// Some permutations are commented out, because they currently are not
// legal:  D3DMTEXF_NONE is only valid as a mip filter, and for StretchRect.
//
__declspec(selectany) extern CONST MIPMAPTEST TestCasePerms[D3DMQA_MIPMAPTEST_COUNT] = 
{
// | D3DMTSS_MIPFILTER | D3DMTSS_MAGFILTER | D3DMTSS_MINFILTER | Level | Bias |
// +-------------------+-------------------+-------------------+-------+------+

//
// LOD Bias test
//
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,    -2.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,    -1.5f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,    -1.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,    -0.5f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.5f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     1.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     1.5f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     2.0f },

//
// Mip Level Test
//
//
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  1,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  2,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  3,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  4,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  5,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  6,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  7,     0.0f },

{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  0,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  1,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  2,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  3,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  4,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  5,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  6,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  7,     0.0f },

{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  0,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  1,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  2,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  3,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  4,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  5,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  6,     0.0f },
{   D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  7,     0.0f },

//
// Sampling/Filtering tests
//
{ D3DMTEXF_NONE    , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_POINT      , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_POINT      , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_LINEAR     , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_LINEAR     , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_ANISOTROPIC, D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_ANISOTROPIC, D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_NONE    , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_POINT      , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_POINT      , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_LINEAR     , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_LINEAR     , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_ANISOTROPIC, D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_ANISOTROPIC, D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_POINT   , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_POINT      , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_POINT      , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_POINT      , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_LINEAR     , D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_LINEAR     , D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_LINEAR     , D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_POINT        ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_LINEAR       ,  0,     0.5f },
{ D3DMTEXF_LINEAR  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_ANISOTROPIC  ,  0,     0.5f },

// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_POINT      
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_POINT      
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR     
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR     
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_NONE  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_NONE  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       
// { D3DMTEXF_NONE  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_NONE      
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_NONE      
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_POINT     
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_POINT      
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR     
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR     
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_POINT  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       
// { D3DMTEXF_POINT  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_NONE      
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_NONE      
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_POINT     
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_POINT     
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR    
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_LINEAR    
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_LINEAR  , D3DMTEXF_NONE       , D3DMTEXF_ANISOTROPIC
// { D3DMTEXF_LINEAR  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_POINT      , D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_LINEAR     , D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       
// { D3DMTEXF_LINEAR  , D3DMTEXF_ANISOTROPIC, D3DMTEXF_NONE       

};
