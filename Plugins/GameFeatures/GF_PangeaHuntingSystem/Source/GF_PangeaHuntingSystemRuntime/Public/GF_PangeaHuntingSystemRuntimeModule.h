// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class AActor;
class UWorld;

class FGF_PangeaHuntingSystemRuntimeModule : public IModuleInterface
{
public:
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface

private:
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleActorSpawned(AActor* Actor);
	void TryAddRuntimeComponents(AActor* Actor) const;

	FDelegateHandle PostWorldInitializationHandle;
	TMap<TWeakObjectPtr<UWorld>, FDelegateHandle> WorldSpawnHandles;
};
