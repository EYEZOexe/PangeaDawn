// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/PangeaBreedableComponent.h"

#include "AdvancedRPGSystem/Public/ARSStatisticsComponent.h"
#include "Components/PangeaBreedingFarmComponent.h"
#include "Definitions/PangeaBreedingFragment.h"
#include "Definitions/PangeaCreatureDefinition.h"
#include "Interfaces/PDDefinitionProviderInterface.h"

UPangeaBreedableComponent::UPangeaBreedableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

bool UPangeaBreedableComponent::IsFertile_Implementation() const
{
    UE_LOG(LogTemp, Warning, TEXT("%s fertility check - bIsFertile=%d, bIsOnCooldown=%d, Remaining=%.1f"),
        *GetOwner()->GetName(), bIsFertile, bIsOnFertilityCooldown, FertilityCooldownRemaining);
    return !bIsOnFertilityCooldown && bIsFertile;
}

bool UPangeaBreedableComponent::SetFertile_Implementation(const bool bNewFertile)
{
    bIsFertile = bNewFertile;
    return bIsFertile;
}

void UPangeaBreedableComponent::SetGender(const ECreatureGender NewGender)
{
    Gender = NewGender;
}

void UPangeaBreedableComponent::BeginPlay()
{
    Super::BeginPlay();

    AssignGenderFromDefinition(false);

    if (!ACFAttributes && GetOwner())
    {
        ACFAttributes = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
    }

    if (AActor* Owner = GetOwner())
    {
        if (USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
        {
            for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
            {
                UMaterialInterface* Mat = Mesh->GetMaterial(Index);
                if (Mat && !Cast<UMaterialInstanceDynamic>(Mat))
                {
                    Mesh->CreateAndSetMaterialInstanceDynamic(Index);
                    UE_LOG(LogTemp, Warning, TEXT("%s: Converted material slot %d to dynamic."), *Owner->GetName(), Index);
                }
            }
        }
    }
}

ECreatureGender UPangeaBreedableComponent::AssignGenderFromDefinition(const bool bForceReassign)
{
    if (!bForceReassign && Gender != ECreatureGender::Unspecified)
    {
        return Gender;
    }

    const UPangeaCreatureDefinition* Definition = GetCreatureDefinition();
    if (!Definition)
    {
        Gender = Gender == ECreatureGender::Unspecified ? GetRandomGender() : Gender;
        return Gender;
    }

    switch (Definition->GenderAssignmentMode)
    {
    case ECreatureGenderAssignmentMode::UseDefault:
        Gender = Definition->DefaultGender == ECreatureGender::Unspecified
            ? ECreatureGender::Female
            : Definition->DefaultGender;
        break;

    case ECreatureGenderAssignmentMode::Randomize:
    default:
        Gender = GetRandomGender();
        break;
    }

    return Gender;
}

void UPangeaBreedableComponent::StartFertilityCooldown(float CooldownDuration)
{
    if (CooldownDuration <= 0.f || bIsOnFertilityCooldown)
    {
        return;
    }

    bIsOnFertilityCooldown = true;
    FertilityCooldownRemaining = CooldownDuration;
    OnFertilityStateChanged.Broadcast(false);

    GetWorld()->GetTimerManager().SetTimer(
        FertilityCooldownTimerHandle,
        this,
        &UPangeaBreedableComponent::EndFertilityCooldown,
        CooldownDuration,
        false
    );

    GetWorld()->GetTimerManager().SetTimer(
        FertilityTickTimerHandle,
        this,
        &UPangeaBreedableComponent::UpdateFertilityCooldown,
        1.0f,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("%s: fertility cooldown started (%.1f s)"), *GetOwner()->GetName(), CooldownDuration);
}

void UPangeaBreedableComponent::UpdateFertilityCooldown()
{
    if (!bIsOnFertilityCooldown)
    {
        GetWorld()->GetTimerManager().ClearTimer(FertilityTickTimerHandle);
        return;
    }

    FertilityCooldownRemaining = FMath::Max(0.f, FertilityCooldownRemaining - 1.f);
    OnFertilityCooldownTick.Broadcast(FertilityCooldownRemaining);

    if (FertilityCooldownRemaining <= 0.f)
    {
        EndFertilityCooldown();
    }
}

void UPangeaBreedableComponent::EndFertilityCooldown()
{
    bIsOnFertilityCooldown = false;
    FertilityCooldownRemaining = 0.0f;
    GetWorld()->GetTimerManager().ClearTimer(FertilityTickTimerHandle);
    OnFertilityStateChanged.Broadcast(true);

    UE_LOG(LogTemp, Warning, TEXT("%s: fertility cooldown ended"), *GetOwner()->GetName());
}

FParentSnapshot UPangeaBreedableComponent::BuildParentSnapshot() const
{
    FParentSnapshot Snapshot;
    if (const UPangeaCreatureDefinition* Definition = GetCreatureDefinition())
    {
        Snapshot.SpeciesID = Definition->SpeciesId;
    }

    Snapshot.CreatureId = FGuid::NewGuid();
    Snapshot.Traits = GeneticTraits;
    Snapshot.ParentActor = GetOwner();
    Snapshot.VisualData = CollectMaterialGenetics();
    Snapshot.MaterialParams = Snapshot.VisualData;

    const UPangeaBreedingFragment* BreedingFragment = GetBreedingFragment();
    AActor* Owner = GetOwner();
    if (!BreedingFragment || !Owner)
    {
        return Snapshot;
    }

    USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    if (!Mesh)
    {
        return Snapshot;
    }

    const int32 SlotIndex = BreedingFragment->InheritanceMaterialSlot;
    if (SlotIndex >= Mesh->GetNumMaterials())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Invalid slot index %d for species %s"),
            *Owner->GetName(), SlotIndex, *GetNameSafe(BreedingFragment));
        return Snapshot;
    }

    UMaterialInterface* Mat = Mesh->GetMaterial(SlotIndex);
    if (!Mat)
    {
        return Snapshot;
    }

    const TArray<FMaterialColorGeneticGroup>& ColorGroups = BreedingFragment->MaterialColorGroups.Num() > 0
        ? BreedingFragment->MaterialColorGroups
        : BreedingFragment->MaterialGeneticGroups;

    for (const FMaterialColorGeneticGroup& Group : ColorGroups)
    {
        for (const FName& Param : Group.ParameterNames)
        {
            FMaterialParameterInfo Info(Param);
            FLinearColor Value = FLinearColor::White;
            Mat->GetVectorParameterValue(Info, Value);
            Snapshot.MaterialParams.Add(Param, Value);
        }
    }

    for (const FMaterialScalarGeneticGroup& Group : BreedingFragment->MaterialScalarGroups)
    {
        for (const FName& Param : Group.ParameterNames)
        {
            float Value = 0.f;
            if (Mat->GetScalarParameterValue(Param, Value))
            {
                Snapshot.ScalarParams.Add(Param, Value);
            }
        }
    }

    return Snapshot;
}

APangeaEggActor* UPangeaBreedableComponent::BreedWith(UPangeaBreedableComponent* OtherParent, UPangeaBreedingFarmComponent* Farm)
{
    if (!OtherParent || !Farm || !bIsFertile || !OtherParent->bIsFertile)
    {
        return nullptr;
    }

    const FParentSnapshot MySnapshot = BuildParentSnapshot();
    const FParentSnapshot OtherSnapshot = OtherParent->BuildParentSnapshot();
    if (MySnapshot.SpeciesID != OtherSnapshot.SpeciesID)
    {
        return nullptr;
    }

    APangeaEggActor* Egg = Farm->TryBreed(this, OtherParent);
    if (Egg)
    {
        OnBred.Broadcast(Egg, OtherParent->GetOwner());
    }

    return Egg;
}

void UPangeaBreedableComponent::PullAttributesIntoTraits_Implementation(FGeneticTraitSet& InOutTraits) const
{
}

TMap<FName, FLinearColor> UPangeaBreedableComponent::CollectMaterialGenetics() const
{
    TMap<FName, FLinearColor> Out;

    const UPangeaBreedingFragment* BreedingFragment = GetBreedingFragment();
    if (!BreedingFragment || !BreedingFragment->bInheritParentMaterials)
    {
        return Out;
    }

    USkeletalMeshComponent* Mesh = GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
    if (!Mesh)
    {
        return Out;
    }

    const int32 SlotIndex = BreedingFragment->InheritanceMaterialSlot;
    if (SlotIndex >= Mesh->GetNumMaterials())
    {
        return Out;
    }

    UMaterialInterface* Mat = Mesh->GetMaterial(SlotIndex);
    if (!Mat)
    {
        return Out;
    }

    const TArray<FMaterialColorGeneticGroup>& ColorGroups = BreedingFragment->MaterialColorGroups.Num() > 0
        ? BreedingFragment->MaterialColorGroups
        : BreedingFragment->MaterialGeneticGroups;

    for (const FMaterialColorGeneticGroup& Group : ColorGroups)
    {
        for (const FName& Param : Group.ParameterNames)
        {
            FLinearColor Value;
            if (Mat->GetVectorParameterValue(Param, Value))
            {
                Out.Add(Param, Value);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("CollectMaterialGenetics: %d params from %s (slot %d)"),
        Out.Num(), *GetOwner()->GetName(), SlotIndex);

    return Out;
}

UPangeaCreatureDefinition* UPangeaBreedableComponent::GetCreatureDefinition() const
{
    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetClass()->ImplementsInterface(UPDDefinitionProviderInterface::StaticClass()))
    {
        return nullptr;
    }

    return IPDDefinitionProviderInterface::Execute_GetCreatureDefinition(Owner);
}

const UPangeaBreedingFragment* UPangeaBreedableComponent::GetBreedingFragment() const
{
    UPangeaCreatureDefinition* Definition = GetCreatureDefinition();
    return Definition ? Definition->GetFragment<UPangeaBreedingFragment>() : nullptr;
}

ECreatureGender UPangeaBreedableComponent::GetRandomGender()
{
    return FMath::RandBool() ? ECreatureGender::Male : ECreatureGender::Female;
}
