// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/PangeaBreedingFarmComponent.h"

#include "Actors/PangeaBreedingFarmActor.h"
#include "Actors/PangeaEggActor.h"
#include "Components/BoxComponent.h"
#include "Components/PangeaBreedableComponent.h"
#include "Definitions/PangeaBreedingFragment.h"
#include "Definitions/PangeaCreatureDefinition.h"
#include "Helpers/PangeaBreedingLibrary.h"
#include "Interfaces/PDDefinitionProviderInterface.h"

UPangeaBreedingFarmComponent::UPangeaBreedingFarmComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UPangeaBreedingFarmComponent::BeginPlay()
{
    Super::BeginPlay();

    if (BreedingZone)
    {
        UE_LOG(LogTemp, Warning, TEXT("FarmLogic bound to breeding zone: %s"), *BreedingZone->GetName());
        BreedingZone->OnComponentBeginOverlap.AddDynamic(this, &UPangeaBreedingFarmComponent::OnOverlapBegin);
        BreedingZone->OnComponentEndOverlap.AddDynamic(this, &UPangeaBreedingFarmComponent::OnOverlapEnd);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FarmLogic has no BreedingZone assigned!"));
    }
}

void UPangeaBreedingFarmComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("Breedable Entered: %s"), *OtherActor->GetName());

    if (UPangeaBreedableComponent* Breedable = OtherActor->FindComponentByClass<UPangeaBreedableComponent>())
    {
        if (!ContainedBreedables.Contains(Breedable))
        {
            ContainedBreedables.Add(Breedable);
        }
    }
}

void UPangeaBreedingFarmComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (UPangeaBreedableComponent* Breedable = OtherActor->FindComponentByClass<UPangeaBreedableComponent>())
    {
        ContainedBreedables.Remove(Breedable);
    }
}

void UPangeaBreedingFarmComponent::GetBreedablesByGender(TArray<UPangeaBreedableComponent*>& OutMales, TArray<UPangeaBreedableComponent*>& OutFemales) const
{
    OutMales.Reset();
    OutFemales.Reset();

    for (UPangeaBreedableComponent* Breedable : ContainedBreedables)
    {
        if (!IsValid(Breedable)) continue;

        switch (Breedable->Gender)
        {
            case ECreatureGender::Male:   OutMales.Add(Breedable); break;
            case ECreatureGender::Female: OutFemales.Add(Breedable); break;
            default: break;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Farm GetBreedablesByGender: Males=%d, Females=%d"), OutMales.Num(), OutFemales.Num());
}

APangeaEggActor* UPangeaBreedingFarmComponent::TryBreed(UPangeaBreedableComponent* Male, UPangeaBreedableComponent* Female)
{
    UPangeaCreatureDefinition* CreatureDefinition = UPangeaBreedingLibrary::GetCreatureDefinitionFromBreedable(Female);
    const UPangeaBreedingFragment* BreedingFragment = UPangeaBreedingLibrary::GetBreedingFragmentFromBreedable(Female);

    if (!CreatureDefinition || !BreedingFragment)
    {
        return nullptr;
    }

    APangeaEggActor* Egg = SpawnEgg(Male, Female, CreatureDefinition, BreedingFragment);
    if (!Egg)
    {
        return nullptr;
    }

    ApplyFertilityCooldowns(Male, Female, BreedingFragment);
    OnEggSpawned.Broadcast(Egg);

    UE_LOG(LogTemp, Log, TEXT("Breeding successful - Egg spawned for species: %s"), *CreatureDefinition->SpeciesId.ToString());
    return Egg;
}

void UPangeaBreedingFarmComponent::RefreshContainedBreedables()
{
    APangeaBreedingFarmActor* FarmActor = Cast<APangeaBreedingFarmActor>(GetOwner());
    if (!FarmActor->BreedingZone)
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshContainedBreedables - no valid collision component on %s"), *GetOwner()->GetName());
        return;
    }

    TArray<AActor*> OverlappingActors;
    FarmActor->BreedingZone->GetOverlappingActors(OverlappingActors, AActor::StaticClass());

    int32 AddedCount = 0;

    ContainedBreedables.RemoveAll([](const UPangeaBreedableComponent* Breedable)
    {
        return !IsValid(Breedable);
    });

    for (AActor* Actor : OverlappingActors)
    {
        if (!Actor || Actor == GetOwner())
            continue;

        if (UPangeaBreedableComponent* Breedable = Actor->FindComponentByClass<UPangeaBreedableComponent>())
        {
            if (!ContainedBreedables.Contains(Breedable))
            {
                ContainedBreedables.Add(Breedable);
                AddedCount++;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("RefreshContainedBreedables - found %d breedables inside %s"), AddedCount, *GetOwner()->GetName());
}

void UPangeaBreedingFarmComponent::ApplyFertilityCooldowns(UPangeaBreedableComponent* Male, UPangeaBreedableComponent* Female, const UPangeaBreedingFragment* BreedingFragment)
{
    const float Cooldown = BreedingFragment->Fertility.FertilityCooldownSeconds;
    const bool bAffectsBoth = BreedingFragment->Fertility.bAffectsBothParents;

    if (Female)
    {
        Female->StartFertilityCooldown(Cooldown);
    }

    if (Male && bAffectsBoth)
    {
        Male->StartFertilityCooldown(Cooldown);
    }
}

APangeaEggActor* UPangeaBreedingFarmComponent::SpawnEgg(UPangeaBreedableComponent* Male, UPangeaBreedableComponent* Female, UPangeaCreatureDefinition* CreatureDefinition, const UPangeaBreedingFragment* BreedingFragment)
{
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const FVector SpawnLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    TSubclassOf<APangeaEggActor> EggClassToSpawn = BreedingFragment && BreedingFragment->EggActorClass ? BreedingFragment->EggActorClass : EggClass;
    if (!EggClassToSpawn)
    {
        return nullptr;
    }

    UPangeaGeneticStrategy* StrategyToUse = UPangeaBreedingLibrary::CreateGeneticStrategy(this, Female, GeneticStrategy);
    if (!StrategyToUse)
    {
        return nullptr;
    }

    APangeaEggActor* Egg = GetWorld()->SpawnActor<APangeaEggActor>(
        EggClassToSpawn,
        SpawnLocation,
        FRotator::ZeroRotator,
        Params
    );

    if (Egg)
    {
        Egg->InitializeEgg(Male->BuildParentSnapshot(), Female->BuildParentSnapshot(), StrategyToUse, CreatureDefinition);
    }

    return Egg;
}
