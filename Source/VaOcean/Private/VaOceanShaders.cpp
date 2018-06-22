// Copyright 2014-2018 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanShaders.h"
#include "ShaderParameterUtils.h"
#include "SimpleElementShaders.h"

IMPLEMENT_SHADER_TYPE(, FQuadVS, TEXT("/Plugin/VaOcean/Private/VaOcean_VS_PS.usf"), TEXT("QuadVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FUpdateDisplacementPS, TEXT("/Plugin/VaOcean/Private/VaOcean_VS_PS.usf"), TEXT("UpdateDisplacementPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FGenGradientFoldingPS, TEXT("/Plugin/VaOcean/Private/VaOcean_VS_PS.usf"), TEXT("GenGradientFoldingPS"), SF_Pixel);

//IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateSpectrumUniformParameters, TEXT("PerFrameSp"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FUpdateDisplacementUniformParameters, TEXT("PerFrameDisp"));
