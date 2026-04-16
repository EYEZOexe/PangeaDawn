// Copyright Epic Games, Inc. All Rights Reserved.

#include "GF_PangeaHuntingSystemRuntimeModule.h"

#include "Actors/ACFCharacter.h"
#include "Components/PangeaHunterSenseComponent.h"
#include "Components/PangeaTrackEmitterComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Characters/PDDinosaurBase.h"

#define LOCTEXT_NAMESPACE "FGF_PangeaHuntingSystemRuntimeModule"

namespace PangeaHuntingSystem
{
	static const TCHAR* HunterPlayerBlueprintClassPath = TEXT("/Game/_Game/Characters/Player/Blueprints/BP_Pangea_Player.BP_Pangea_Player_C");
	static int32 ModuleDebugMessageKey = 470100;
	static constexpr bool bEnableModuleDebugMessages = false;

	static UClass* ResolveHunterPlayerClass()
	{
		static TWeakObjectPtr<UClass> CachedClass;
		if (CachedClass.IsValid())
		{
			return CachedClass.Get();
		}

		UClass* LoadedClass = LoadObject<UClass>(nullptr, HunterPlayerBlueprintClassPath);
		CachedClass = LoadedClass;
		return LoadedClass;
	}

	static void PrintDebugMessage(const FString& Message, const FColor& Color = FColor::Cyan)
	{
		if (!bEnableModuleDebugMessages)
		{
			return;
		}

		if (GEngine)
		{
			const uint64 Key = static_cast<uint64>(ModuleDebugMessageKey++);
			GEngine->AddOnScreenDebugMessage(Key, 2.5f, Color, FString::Printf(TEXT("[Hunting] %s"), *Message));
		}

		UE_LOG(LogTemp, Log, TEXT("[Hunting] %s"), *Message);
	}
}

void FGF_PangeaHuntingSystemRuntimeModule::StartupModule()
{
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(
		this, &FGF_PangeaHuntingSystemRuntimeModule::HandlePostWorldInitialization);
}

void FGF_PangeaHuntingSystemRuntimeModule::ShutdownModule()
{
	if (PostWorldInitializationHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
		PostWorldInitializationHandle.Reset();
	}

	for (TPair<TWeakObjectPtr<UWorld>, FDelegateHandle>& Entry : WorldSpawnHandles)
	{
		if (UWorld* World = Entry.Key.Get())
		{
			World->RemoveOnActorSpawnedHandler(Entry.Value);
		}
	}

	WorldSpawnHandles.Empty();
}

void FGF_PangeaHuntingSystemRuntimeModule::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (WorldSpawnHandles.Contains(World))
	{
		return;
	}

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		TryAddRuntimeComponents(*ActorIterator);
	}

	const FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateRaw(this, &FGF_PangeaHuntingSystemRuntimeModule::HandleActorSpawned));

	WorldSpawnHandles.Add(World, SpawnHandle);
}

void FGF_PangeaHuntingSystemRuntimeModule::HandleActorSpawned(AActor* Actor)
{
	TryAddRuntimeComponents(Actor);
}

void FGF_PangeaHuntingSystemRuntimeModule::TryAddRuntimeComponents(AActor* Actor) const
{
	if (!Actor || !IsValid(Actor) || Actor->IsPendingKillPending())
	{
		return;
	}

	if (Actor->IsA<APDDinosaurBase>())
	{
		if (!Actor->FindComponentByClass<UPangeaTrackEmitterComponent>())
		{
			UPangeaTrackEmitterComponent* TrackEmitterComponent = NewObject<UPangeaTrackEmitterComponent>(
				Actor, UPangeaTrackEmitterComponent::StaticClass(), TEXT("PangeaTrackEmitterComponent"));
			Actor->AddInstanceComponent(TrackEmitterComponent);
			TrackEmitterComponent->RegisterComponent();
			PangeaHuntingSystem::PrintDebugMessage(FString::Printf(TEXT("Injected track emitter into %s"), *GetNameSafe(Actor)), FColor::Yellow);
		}
	}

	if (Actor->IsA<AACFCharacter>() && !Actor->IsA<APDDinosaurBase>())
	{
		UClass* HunterPlayerClass = PangeaHuntingSystem::ResolveHunterPlayerClass();
		const bool bShouldInjectHunterSense = HunterPlayerClass ? Actor->IsA(HunterPlayerClass) : Actor->IsA<AACFCharacter>();
		if (bShouldInjectHunterSense && !Actor->FindComponentByClass<UPangeaHunterSenseComponent>())
		{
			UPangeaHunterSenseComponent* HunterSenseComponent = NewObject<UPangeaHunterSenseComponent>(
				Actor, UPangeaHunterSenseComponent::StaticClass(), TEXT("PangeaHunterSenseComponent"));
			Actor->AddInstanceComponent(HunterSenseComponent);
			HunterSenseComponent->RegisterComponent();
			PangeaHuntingSystem::PrintDebugMessage(FString::Printf(TEXT("Injected hunter sense into %s"), *GetNameSafe(Actor)), FColor::Cyan);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_PangeaHuntingSystemRuntimeModule, GF_PangeaHuntingSystemRuntime)
