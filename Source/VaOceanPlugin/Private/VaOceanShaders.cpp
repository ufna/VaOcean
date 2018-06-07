// Created 2014-2016 Vladimir Alyamkin. MIT license 
#include "VaOceanShaders.h"
#include "VaOceanPluginPrivatePCH.h"

IMPLEMENT_SHADER_TYPE(, FUpdateSpectrumCS, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_CS.usf"), TEXT("UpdateSpectrumCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_FFT.usf"), TEXT("Radix008A_CS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS2, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_FFT.usf"), TEXT("Radix008A_CS2"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FQuadVS, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_VS_PS.usf"), TEXT("QuadVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FUpdateDisplacementPS, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_VS_PS.usf"), TEXT("UpdateDisplacementPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FGenGradientFoldingPS, TEXT("/Plugin/VaOceanPlugin/Private/VaOcean_VS_PS.usf"), TEXT("GenGradientFoldingPS"), SF_Pixel);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateSpectrumUniformParameters, TEXT("PerFrameSp"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateDisplacementUniformParameters, TEXT("PerFrameDisp"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FRadixFFTUniformParameters, TEXT("PerFrameFFT"));


//IMPLEMENT_SHADER_TYPE(, FLensDistortionUVGenerationPS, TEXT("/Plugin/ShaderTest/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)