// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"
#include "VaOceanPlugin.generated.inl"

class FVaOceanPlugin : public IVaOceanPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE
	{

	}

	virtual void ShutdownModule() OVERRIDE
	{

	}
};

IMPLEMENT_MODULE( FVaOceanPlugin, VaOceanPlugin )

DEFINE_LOG_CATEGORY(LogVaOcean);
DEFINE_LOG_CATEGORY(LogVaOceanPhysics);
