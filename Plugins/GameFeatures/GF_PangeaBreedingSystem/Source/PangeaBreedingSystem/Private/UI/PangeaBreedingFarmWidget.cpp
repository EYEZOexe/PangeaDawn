// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.


#include "UI/PangeaBreedingFarmWidget.h"

#include "Actors/PangeaEggActor.h"
#include "Components/PangeaBreedableComponent.h"
#include "Components/PangeaBreedingFarmComponent.h"
#include "Helpers/PangeaBreedingLibrary.h"

UPangeaBreedingFarmWidget::UPangeaBreedingFarmWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    
}

void UPangeaBreedingFarmWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (CurrentFarm)
    {
        // Bind UI to farm events
        CurrentFarm->OnEggSpawned.AddDynamic(this, &UPangeaBreedingFarmWidget::OnEggSpawned);
    }
}

void UPangeaBreedingFarmWidget::OnEggSpawned(APangeaEggActor* NewEgg)
{
    // Forward to Blueprint for UI updates
    AddEggEntry(NewEgg);
}

bool UPangeaBreedingFarmWidget::ValidateParentPair(UPangeaBreedableComponent* Male,UPangeaBreedableComponent* Female, FString& OutReason) const
{
    if (!Male || !Female)
    {
        OutReason = TEXT("Missing parent(s).");
        return false;
    }

    if (Male->BuildParentSnapshot().SpeciesID != Female->BuildParentSnapshot().SpeciesID)
    {
        OutReason = TEXT("Mismatched species.");
        return false;
    }

    if (Female->Gender != ECreatureGender::Female)
    {
        OutReason = TEXT("Second parent must be female.");
        return false;
    }

    if (!Male->IsFertile_Implementation() || !Female->IsFertile_Implementation())
    {
        OutReason = TEXT("One or both parents are not fertile.");
        return false;
    }

    if (!UPangeaBreedingLibrary::ResolveEggActorClass(Female, CurrentFarm->EggClass))
    {
        OutReason = TEXT("No egg class configured for this species.");
        return false;
    }

    return true;
}

void UPangeaBreedingFarmWidget::TryBreedUI()
{
    if (!CurrentFarm)
    {
        UE_LOG(LogTemp, Warning, TEXT("UBreedingFarmWidget::TryBreed — No CurrentFarm set"));
        return;
    }

    FString FailureReason;
    if (!ValidateParentPair(SelectedMale, SelectedFemale, FailureReason))
    {
        UE_LOG(LogTemp, Warning, TEXT("UBreedingFarmWidget::ValidateParentPair: %s"), *FailureReason);
        return;
    }

    APangeaEggActor* OutEgg = CurrentFarm->TryBreed(SelectedMale, SelectedFemale);
    if (OutEgg)
    {
        UE_LOG(LogTemp, Log, TEXT("Breeding successful: Egg spawned"));
        ClearSelection();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Breeding attempt failed"));
    }
}





