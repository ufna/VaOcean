// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

IMPLEMENT_SHADER_TYPE(, FUpdateSpectrumCS, TEXT("VaOcean_CS"), TEXT("UpdateSpectrumCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS, TEXT("VaOcean_FFT"), TEXT("Radix008A_CS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS2, TEXT("VaOcean_FFT"), TEXT("Radix008A_CS2"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FQuadVS, TEXT("VaOcean_VS_PS"), TEXT("QuadVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FUpdateDisplacementPS, TEXT("VaOcean_VS_PS"), TEXT("UpdateDisplacementPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FGenGradientFoldingPS, TEXT("VaOcean_VS_PS"), TEXT("GenGradientFoldingPS"), SF_Pixel);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateSpectrumUniformParameters, TEXT("VaPerFrameSp"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateDisplacementUniformParameters, TEXT("VaPerFrameDisp"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FRadixFFTUniformParameters, TEXT("VaPerFrameFFT"));
