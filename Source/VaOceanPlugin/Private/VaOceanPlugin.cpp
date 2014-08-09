// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOceanPluginPrivatePCH.h"

class FVaOceanPlugin : public IVaOceanPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{

	}

	virtual void ShutdownModule() override
	{

	}
};

IMPLEMENT_MODULE( FVaOceanPlugin, VaOceanPlugin )

DEFINE_LOG_CATEGORY(LogVaOcean);
DEFINE_LOG_CATEGORY(LogVaOceanPhysics);
