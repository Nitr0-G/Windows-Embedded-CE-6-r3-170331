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


//
// The following table provides a wide variety of RGBA color combinations.
//
// For the purposes of this table, D3DMVALUE is float
//
CONST UINT uiNumSampleSingleColorValues = 33;
CONST UINT uiNumSampleColorValues = 84;
__declspec(selectany) D3DMCOLORVALUE SampleColorValues[uiNumSampleColorValues] = {
//|          RED              |           GREEN           |            BLUE           |           ALPHA           |
//+---------------------------+---------------------------+---------------------------+---------------------------+
//|                           |                           |                           |                           |
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //*
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.1f) }, //**
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //***
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.3f) }, //****
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) }, //***** RED COLORS OF
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //***** VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) }, //*****
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.1f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.3f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) }, //***** GREEN COLORS OF
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //***** VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.1f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.3f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.4f) }, //***** BLUE COLORS OF
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //***** VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.6f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***** RED/GREEN COMBOS
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***** OF VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***** RED/BLUE COMBOS
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***** OF VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***** GREEN/BLUE COMBOS
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //***** OF VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.7f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.6f) , D3DM_MAKE_D3DMVALUE(0.4f) , D3DM_MAKE_D3DMVALUE(0.9f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.0f) , D3DM_MAKE_D3DMVALUE(0.8f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //*

  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //*
  { D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //**
  { D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.0f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.2f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //***** RED/GREEN/BLUE COMBOS
  { D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) }, //***** OF VARIOUS OPACITY
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.8f) }, //*****
  { D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //****
  { D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(1.0f) }, //***
  { D3DM_MAKE_D3DMVALUE(0.5f) , D3DM_MAKE_D3DMVALUE(0.2f) , D3DM_MAKE_D3DMVALUE(0.3f) , D3DM_MAKE_D3DMVALUE(1.0f) }  //**

};
