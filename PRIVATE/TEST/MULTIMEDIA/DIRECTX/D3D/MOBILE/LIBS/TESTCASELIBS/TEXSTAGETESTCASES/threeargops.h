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
#include <d3dm.h>

HRESULT SetThreeArgStates(LPDIRECT3DMOBILEDEVICE pDevice,
                          DWORD dwTableIndex);

HRESULT GetThreeArgColors(LPDIRECT3DMOBILEDEVICE pDevice,
                          DWORD dwTableIndex,
                          PDWORD *ppdwDiffuse, 
                          PDWORD *ppdwSpecular,
                          PDWORD *ppdwTFactor, 
                          PDWORD *ppdwTexture); 

BOOL IsThreeArgTestOpSupported(LPDIRECT3DMOBILEDEVICE pDevice,
                               DWORD dwTableIndex);

typedef struct _THREE_ARG_OP_TESTS {
	D3DMTEXTUREOP TextureOp;
	D3DMCOLOR ColorValue0;
	D3DMCOLOR ColorValue1;
	D3DMCOLOR ColorValue2;
	DWORD TextureArg0;
	DWORD TextureArg1;
	DWORD TextureArg2;
} THREE_ARG_OP_TESTS;

#define D3DQA_THREE_ARG_OP_TESTS_COUNT 96

__declspec(selectany) THREE_ARG_OP_TESTS ThreeArgOpCases[D3DQA_THREE_ARG_OP_TESTS_COUNT] = {
// |           TextureOp             | ColorValue0 | ColorValue1 | ColorValue2 |   TextureArg0   |   TextureArg1   |   TextureArg2   |
// +---------------------------------+-------------+-------------+-------------+-----------------+-----------------+-----------------+

//
// D3DMTOP_MULTIPLYADD Operation Tests (no D3DMTA_OPTIONMASK options)
//
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE00000,   0xFFA00000,  D3DMTA_DIFFUSE,     D3DMTA_TEXTURE,   D3DMTA_TFACTOR}, // RED (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E000,   0xFF00A000,  D3DMTA_DIFFUSE,     D3DMTA_TEXTURE,  D3DMTA_SPECULAR}, // GREEN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF0000E0,   0xFF0000A0,  D3DMTA_DIFFUSE,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE}, // BLUE (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE0E000,   0xFFA0A000,  D3DMTA_DIFFUSE,     D3DMTA_TFACTOR,  D3DMTA_SPECULAR}, // YELLOW (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E0E0,   0xFF00A0A0,  D3DMTA_DIFFUSE,    D3DMTA_SPECULAR,   D3DMTA_TEXTURE}, // CYAN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE000E0,   0xFFA000A0,  D3DMTA_DIFFUSE,    D3DMTA_SPECULAR,   D3DMTA_TFACTOR}, // MAGENTA (DARK)

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE00000,   0xFFA00000,  D3DMTA_TEXTURE,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR}, // RED (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E000,   0xFF00A000,  D3DMTA_TEXTURE,     D3DMTA_DIFFUSE,  D3DMTA_SPECULAR}, // GREEN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF0000E0,   0xFF0000A0,  D3DMTA_TEXTURE,     D3DMTA_TFACTOR,   D3DMTA_DIFFUSE}, // BLUE (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE0E000,   0xFFA0A000,  D3DMTA_TEXTURE,     D3DMTA_TFACTOR,  D3DMTA_SPECULAR}, // YELLOW (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E0E0,   0xFF00A0A0,  D3DMTA_TEXTURE,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE}, // CYAN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE000E0,   0xFFA000A0,  D3DMTA_TEXTURE,    D3DMTA_SPECULAR,   D3DMTA_TFACTOR}, // MAGENTA (DARK)
																																	 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE00000,   0xFFA00000,  D3DMTA_TFACTOR,     D3DMTA_DIFFUSE,   D3DMTA_TEXTURE}, // RED (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E000,   0xFF00A000,  D3DMTA_TFACTOR,     D3DMTA_DIFFUSE,  D3DMTA_SPECULAR}, // GREEN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF0000E0,   0xFF0000A0,  D3DMTA_TFACTOR,     D3DMTA_TEXTURE,   D3DMTA_DIFFUSE}, // BLUE (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE0E000,   0xFFA0A000,  D3DMTA_TFACTOR,     D3DMTA_TEXTURE,  D3DMTA_SPECULAR}, // YELLOW (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E0E0,   0xFF00A0A0,  D3DMTA_TFACTOR,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE}, // CYAN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE000E0,   0xFFA000A0,  D3DMTA_TFACTOR,    D3DMTA_SPECULAR,   D3DMTA_TEXTURE}, // MAGENTA (DARK)

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE00000,   0xFFA00000, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TEXTURE}, // RED (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E000,   0xFF00A000, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR}, // GREEN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF0000E0,   0xFF0000A0, D3DMTA_SPECULAR,     D3DMTA_TEXTURE,   D3DMTA_DIFFUSE}, // BLUE (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE0E000,   0xFFA0A000, D3DMTA_SPECULAR,     D3DMTA_TEXTURE,   D3DMTA_TFACTOR}, // YELLOW (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E0E0,   0xFF00A0A0, D3DMTA_SPECULAR,     D3DMTA_TFACTOR,   D3DMTA_DIFFUSE}, // CYAN (DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE000E0,   0xFFA000A0, D3DMTA_SPECULAR,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE}, // MAGENTA (DARK)

//
// D3DMTOP_MULTIPLYADD Operation Tests (with D3DMTA_COMPLEMENT)
//
{                  D3DMTOP_MULTIPLYADD,   0xFFC0C0C0,   0xFFE00000,   0xFFA00000, D3DMTA_SPECULAR | D3DMTA_COMPLEMENT,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR }, // RED 
{                  D3DMTOP_MULTIPLYADD,   0xFFC0C0C0,   0xFF00E000,   0xFF00A000, D3DMTA_DIFFUSE  | D3DMTA_COMPLEMENT,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE }, // GREEN
{                  D3DMTOP_MULTIPLYADD,   0xFFC0C0C0,   0xFF0000E0,   0xFF0000A0, D3DMTA_TFACTOR  | D3DMTA_COMPLEMENT,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR}, // BLUE 
{                  D3DMTOP_MULTIPLYADD,   0xFFC0C0C0,   0xFFE0E000,   0xFFA0A000, D3DMTA_TEXTURE  | D3DMTA_COMPLEMENT,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE }, // YELLOW

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF2FFFFF,   0xFFA00000, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE | D3DMTA_COMPLEMENT,   D3DMTA_TFACTOR }, // RED 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFF2FFF,   0xFF00A000, D3DMTA_DIFFUSE ,     D3DMTA_TFACTOR | D3DMTA_COMPLEMENT,   D3DMTA_TEXTURE }, // GREEN
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFFFF2F,   0xFF0000A0, D3DMTA_TFACTOR ,     D3DMTA_TEXTURE | D3DMTA_COMPLEMENT,   D3DMTA_SPECULAR}, // BLUE 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF2F1FFF,   0xFFA0A000, D3DMTA_TEXTURE ,    D3DMTA_SPECULAR | D3DMTA_COMPLEMENT,   D3DMTA_DIFFUSE }, // YELLOW

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE00000,   0xFF6FFFFF, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR  | D3DMTA_COMPLEMENT}, // RED 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF00E000,   0xFFFF6FFF, D3DMTA_DIFFUSE ,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE  | D3DMTA_COMPLEMENT}, // GREEN
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFF0000E0,   0xFFFFFF6F, D3DMTA_TFACTOR ,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR | D3DMTA_COMPLEMENT}, // BLUE 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFE0E000,   0xFF6F6FFF, D3DMTA_TEXTURE ,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE  | D3DMTA_COMPLEMENT}, // YELLOW

//
// D3DMTOP_MULTIPLYADD Operation Tests (with D3DMTA_ALPHAREPLICATE)
//
{                  D3DMTOP_MULTIPLYADD,   0x20FF0000,   0xFFE00000,   0xFFA00000, D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR }, // RED (DARK)
{                  D3DMTOP_MULTIPLYADD,   0x4000FF00,   0xFFE00000,   0xFFA00000, D3DMTA_DIFFUSE  | D3DMTA_ALPHAREPLICATE,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE }, //   || (blend) 
{                  D3DMTOP_MULTIPLYADD,   0x600000FF,   0xFFE00000,   0xFFA00000, D3DMTA_TFACTOR  | D3DMTA_ALPHAREPLICATE,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR}, //   \/  
{                  D3DMTOP_MULTIPLYADD,   0x80FFFF00,   0xFFE00000,   0xFFA00000, D3DMTA_TEXTURE  | D3DMTA_ALPHAREPLICATE,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE }, // RED (LIGHT)

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0x20FFFFFF,   0xFFFF0000, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE | D3DMTA_ALPHAREPLICATE,   D3DMTA_TFACTOR }, // RED (VERY DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0x40FFFFFF,   0xFFFF0000, D3DMTA_DIFFUSE ,     D3DMTA_TFACTOR | D3DMTA_ALPHAREPLICATE,   D3DMTA_TEXTURE }, //   || (blend) 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0x60FFFFFF,   0xFFFF0000, D3DMTA_TFACTOR ,     D3DMTA_TEXTURE | D3DMTA_ALPHAREPLICATE,   D3DMTA_SPECULAR}, //   \/  
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0x80FFFFFF,   0xFFFF0000, D3DMTA_TEXTURE ,    D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE,   D3DMTA_DIFFUSE }, // RED

{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFF0000,   0x20FFFFFF, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR  | D3DMTA_ALPHAREPLICATE}, // RED (VERY DARK)
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFF0000,   0x40FFFFFF, D3DMTA_DIFFUSE ,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE  | D3DMTA_ALPHAREPLICATE}, //   || (blend) 
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFF0000,   0x60FFFFFF, D3DMTA_TFACTOR ,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE}, //   \/  
{                  D3DMTOP_MULTIPLYADD,   0xFF404040,   0xFFFF0000,   0x80FFFFFF, D3DMTA_TEXTURE ,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE  | D3DMTA_ALPHAREPLICATE}, // RED

//
// D3DMTOP_LERP Operation Tests (no D3DMTA_OPTIONMASK options)
//
{                         D3DMTOP_LERP,   0xFF000000,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,     D3DMTA_TEXTURE,   D3DMTA_TFACTOR}, //  GREEN
{                         D3DMTOP_LERP,   0xFF303030,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,     D3DMTA_TEXTURE,  D3DMTA_SPECULAR}, //   || 
{                         D3DMTOP_LERP,   0xFF606060,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE}, //   || (blend)
{                         D3DMTOP_LERP,   0xFF909090,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,     D3DMTA_TFACTOR,  D3DMTA_SPECULAR}, //   ||
{                         D3DMTOP_LERP,   0xFFC0C0C0,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,    D3DMTA_SPECULAR,   D3DMTA_TEXTURE}, //   \/
{                         D3DMTOP_LERP,   0xFFFFFFFF,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_DIFFUSE,    D3DMTA_SPECULAR,   D3DMTA_TFACTOR}, // MAGENTA

{                         D3DMTOP_LERP,   0xFF000000,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR}, //  GREEN 
{                         D3DMTOP_LERP,   0xFF303030,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,     D3DMTA_DIFFUSE,  D3DMTA_SPECULAR}, //   ||  
{                         D3DMTOP_LERP,   0xFF606060,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,     D3DMTA_TFACTOR,   D3DMTA_DIFFUSE}, //   || (blend) 
{                         D3DMTOP_LERP,   0xFF909090,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,     D3DMTA_TFACTOR,  D3DMTA_SPECULAR}, //   || 
{                         D3DMTOP_LERP,   0xFFC0C0C0,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE}, //   \/ 
{                         D3DMTOP_LERP,   0xFFFFFFFF,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TEXTURE,    D3DMTA_SPECULAR,   D3DMTA_TFACTOR}, // MAGENTA 

{                         D3DMTOP_LERP,   0xFF000000,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,     D3DMTA_DIFFUSE,   D3DMTA_TEXTURE}, //  GREEN  
{                         D3DMTOP_LERP,   0xFF303030,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,     D3DMTA_DIFFUSE,  D3DMTA_SPECULAR}, //   ||  
{                         D3DMTOP_LERP,   0xFF606060,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,     D3DMTA_TEXTURE,   D3DMTA_DIFFUSE}, //   || (blend)  
{                         D3DMTOP_LERP,   0xFF909090,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,     D3DMTA_TEXTURE,  D3DMTA_SPECULAR}, //   || 
{                         D3DMTOP_LERP,   0xFFC0C0C0,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE}, //   \/ 
{                         D3DMTOP_LERP,   0xFFFFFFFF,   0xFFFF00FF,   0xFF00FF00,  D3DMTA_TFACTOR,    D3DMTA_SPECULAR,   D3DMTA_TEXTURE}, // MAGENTA  

{                         D3DMTOP_LERP,   0xFF000000,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TEXTURE}, //  GREEN  
{                         D3DMTOP_LERP,   0xFF303030,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR}, //   ||  
{                         D3DMTOP_LERP,   0xFF606060,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_TEXTURE,   D3DMTA_DIFFUSE}, //   || (blend)  
{                         D3DMTOP_LERP,   0xFF909090,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_TEXTURE,   D3DMTA_TFACTOR}, //   || 
{                         D3DMTOP_LERP,   0xFFC0C0C0,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_TFACTOR,   D3DMTA_DIFFUSE}, //   \/ 
{                         D3DMTOP_LERP,   0xFFFFFFFF,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE}, // MAGENTA 

//
// D3DMTOP_LERP Operation Tests (with D3DMTA_COMPLEMENT)
//
{                         D3DMTOP_LERP,   0xFF303030,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR | D3DMTA_COMPLEMENT,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR }, //   MAGENTA  
{                         D3DMTOP_LERP,   0xFF707070,   0xFFFF00FF,   0xFF00FF00, D3DMTA_DIFFUSE  | D3DMTA_COMPLEMENT,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE }, //    || (blend)   
{                         D3DMTOP_LERP,   0xFFB0B0B0,   0xFFFF00FF,   0xFF00FF00, D3DMTA_TFACTOR  | D3DMTA_COMPLEMENT,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR}, //    \/   
{                         D3DMTOP_LERP,   0xFFF0F0F0,   0xFFFF00FF,   0xFF00FF00, D3DMTA_TEXTURE  | D3DMTA_COMPLEMENT,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE }, //   GREEN
																																							 
{                         D3DMTOP_LERP,   0xFF808080,   0xFF0000FF,   0xFFFFFF00,  D3DMTA_DIFFUSE,   D3DMTA_SPECULAR | D3DMTA_COMPLEMENT,    D3DMTA_TFACTOR }, //  YELLOW
{                         D3DMTOP_LERP,   0xFF808080,   0xFF4000FF,   0xFFFFFF00,  D3DMTA_TFACTOR,   D3DMTA_DIFFUSE  | D3DMTA_COMPLEMENT,    D3DMTA_TEXTURE }, //    || (blend) 
{                         D3DMTOP_LERP,   0xFF808080,   0xFFC000FF,   0xFFFFFF00,  D3DMTA_TEXTURE,   D3DMTA_TFACTOR  | D3DMTA_COMPLEMENT,    D3DMTA_SPECULAR}, //    \/    
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFF00FF,   0xFFFFFF00, D3DMTA_SPECULAR,   D3DMTA_TEXTURE  | D3DMTA_COMPLEMENT,    D3DMTA_DIFFUSE }, //  GREEN 
																																							 
{                         D3DMTOP_LERP,   0xFF808080,   0xFF00FFFF,   0xFFFF0000,  D3DMTA_DIFFUSE,   D3DMTA_SPECULAR,    D3DMTA_TFACTOR  | D3DMTA_COMPLEMENT}, //   CYAN
{                         D3DMTOP_LERP,   0xFF808080,   0xFF00FFFF,   0xFFFF4000,  D3DMTA_TFACTOR,   D3DMTA_DIFFUSE ,    D3DMTA_TEXTURE  | D3DMTA_COMPLEMENT}, //    || (blend) 
{                         D3DMTOP_LERP,   0xFF808080,   0xFF00FFFF,   0xFFFFC000,  D3DMTA_TEXTURE,   D3DMTA_TFACTOR ,    D3DMTA_SPECULAR | D3DMTA_COMPLEMENT}, //    \/    
{                         D3DMTOP_LERP,   0xFF808080,   0xFF00FFFF,   0xFFFFFF00, D3DMTA_SPECULAR,   D3DMTA_TEXTURE ,    D3DMTA_DIFFUSE  | D3DMTA_COMPLEMENT}, //   BLUE

//
// D3DMTOP_LERP Operation Tests (with D3DMTA_ALPHAREPLICATE)
//
{                         D3DMTOP_LERP,   0x30FFFFFF,   0xFFFF00FF,   0xFF00FF00, D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE,     D3DMTA_DIFFUSE,   D3DMTA_TFACTOR }, //  GREEN 
{                         D3DMTOP_LERP,   0x70FFFFFF,   0xFFFF00FF,   0xFF00FF00, D3DMTA_DIFFUSE  | D3DMTA_ALPHAREPLICATE,     D3DMTA_TFACTOR,   D3DMTA_TEXTURE }, //   || (blend) 
{                         D3DMTOP_LERP,   0xA0FFFFFF,   0xFFFF00FF,   0xFF00FF00, D3DMTA_TFACTOR  | D3DMTA_ALPHAREPLICATE,     D3DMTA_TEXTURE,   D3DMTA_SPECULAR}, //   \/ 
{                         D3DMTOP_LERP,   0xF0FFFFFF,   0xFFFF00FF,   0xFF00FF00, D3DMTA_TEXTURE  | D3DMTA_ALPHAREPLICATE,    D3DMTA_SPECULAR,   D3DMTA_DIFFUSE }, // MAGENTA

{                         D3DMTOP_LERP,   0xFF808080,   0x8FFFFFFF,   0xFFFF0000,  D3DMTA_DIFFUSE,   D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE,    D3DMTA_TFACTOR }, // RED (DARK)
{                         D3DMTOP_LERP,   0xFF808080,   0xAFFFFFFF,   0xFFFF0000,  D3DMTA_TFACTOR,   D3DMTA_DIFFUSE  | D3DMTA_ALPHAREPLICATE,    D3DMTA_TEXTURE }, //   || (blend) 
{                         D3DMTOP_LERP,   0xFF808080,   0xCFFFFFFF,   0xFFFF0000,  D3DMTA_TEXTURE,   D3DMTA_TFACTOR  | D3DMTA_ALPHAREPLICATE,    D3DMTA_SPECULAR}, //   \/  
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFFFFFF,   0xFFFF0000, D3DMTA_SPECULAR,   D3DMTA_TEXTURE  | D3DMTA_ALPHAREPLICATE,    D3DMTA_DIFFUSE }, // RED (LIGHT)
																																							 
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFF0000,   0x8FFFFFFF,  D3DMTA_DIFFUSE,   D3DMTA_SPECULAR,    D3DMTA_TFACTOR  | D3DMTA_ALPHAREPLICATE}, // RED (DARK)
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFF0000,   0xAFFFFFFF,  D3DMTA_TFACTOR,   D3DMTA_DIFFUSE ,    D3DMTA_TEXTURE  | D3DMTA_ALPHAREPLICATE}, //   || (blend) 
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFF0000,   0xCFFFFFFF,  D3DMTA_TEXTURE,   D3DMTA_TFACTOR ,    D3DMTA_SPECULAR | D3DMTA_ALPHAREPLICATE}, //   \/  
{                         D3DMTOP_LERP,   0xFF808080,   0xFFFF0000,   0xFFFFFFFF, D3DMTA_SPECULAR,   D3DMTA_TEXTURE ,    D3DMTA_DIFFUSE  | D3DMTA_ALPHAREPLICATE}, // RED (LIGHT)

};