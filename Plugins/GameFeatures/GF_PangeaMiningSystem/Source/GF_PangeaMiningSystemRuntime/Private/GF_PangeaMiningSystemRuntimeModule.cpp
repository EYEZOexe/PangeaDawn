// Copyright Epic Games, Inc. All Rights Reserved.

#include "GF_PangeaMiningSystemRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FGF_PangeaMiningSystemRuntimeModule"

void FGF_PangeaMiningSystemRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory;
	// the exact timing is specified in the .uplugin file per-module
}

void FGF_PangeaMiningSystemRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.
	// For modules that support dynamic reloading, we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_PangeaMiningSystemRuntimeModule, GF_PangeaMiningSystemRuntime)
