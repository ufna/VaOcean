// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#pragma once

#include "CoreUObject.h"
#include "Engine.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVaOcean, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVaOceanPhysics, Log, All);

#include "IVaOceanPlugin.h"

#include "VaOceanTypes.h"
#include "VaOceanShaders.h"
#include "VaOceanRadixFFT.h"
#include "VaOceanSimulatorComponent.h"
#include "VaOceanBuoyancyComponent.h"
#include "VaOceanStateActor.h"
#include "VaOceanStateActorSimple.h"
