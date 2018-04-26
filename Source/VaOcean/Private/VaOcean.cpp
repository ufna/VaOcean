// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

#include "VaOcean.h"
#include "VaOceanPrivate.h"

#define LOCTEXT_NAMESPACE "FVaOceanModule"

void FVaOceanModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FVaOceanModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVaOceanModule, VaOcean)

DEFINE_LOG_CATEGORY(LogVaOcean);
