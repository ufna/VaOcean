// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

IMPLEMENT_SHADER_TYPE(, FUpdateSpectrumCS, TEXT("VaOcean_CS"), TEXT("UpdateSpectrumCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS, TEXT("VaOcean_FFT"), TEXT("Radix008A_CS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FRadix008A_CS2, TEXT("VaOcean_FFT"), TEXT("Radix008A_CS2"), SF_Compute);
