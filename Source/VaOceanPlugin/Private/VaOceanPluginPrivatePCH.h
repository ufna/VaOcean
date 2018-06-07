// Created 2014-2016 Vladimir Alyamkin. MIT license 

#pragma once

#include "CoreUObject.h"
#include "Engine.h"
#include "UniformBuffer.h"
#include "GlobalShader.h"
#include "Public/PipelineStateCache.h"
#include "ObjectMacros.h"


// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVaOcean, Log, All);

#include "IVaOceanPlugin.h"

#include "VaOceanRadixFFT.h"
#include "VaOceanTypes.h"
#include "VaOceanShaders.h"
#include "VaOceanSimulator.h"



