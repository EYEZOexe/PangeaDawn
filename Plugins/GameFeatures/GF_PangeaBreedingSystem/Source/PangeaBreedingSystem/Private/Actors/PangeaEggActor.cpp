// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Actors/PangeaEggActor.h"

#include "Components/PangeaBreedableComponent.h"
#include "Net/UnrealNetwork.h"
#include "Objects/PangeaGeneticStrategy.h"

APangeaEggActor::APangeaEggActor()
{
    bReplicates = true;
}

void APangeaEggActor::BeginPlay()
{
    Super::BeginPlay();
}

void APangeaEggActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APangeaEggActor, CreatureDefinition);
    DOREPLIFETIME(APangeaEggActor, ParentA);
    DOREPLIFETIME(APangeaEggActor, ParentB);
    DOREPLIFETIME(APangeaEggActor, ChildTraits);
}

void APangeaEggActor::InitializeEgg(const FParentSnapshot& InA, const FParentSnapshot& InB, UPangeaGeneticStrategy* Strategy, UPangeaCreatureDefinition* InCreatureDefinition)
{
    check(HasAuthority());

    ParentA = InA;
    ParentB = InB;
    CreatureDefinition = (InA.SpeciesID == InB.SpeciesID) ? InCreatureDefinition : nullptr;
    if (!CreatureDefinition)
    {
        UE_LOG(LogTemp, Error, TEXT("AEggActor::InitializeEgg - CreatureDefinition is null, cannot proceed"));
        return;
    }

    if (Strategy)
    {
        ChildTraits = Strategy->CombineTraits(ParentA, ParentB);
    }

    StartIncubation();
}

void APangeaEggActor::StartIncubation()
{
    if (!HasAuthority())
    {
        return;
    }

    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition ? CreatureDefinition->GetFragment<UPangeaBreedingFragment>() : nullptr;
    if (!BreedingFragment)
    {
        return;
    }

    TotalIncubationTime = FMath::Max(BreedingFragment->Incubation.IncubationSeconds, 0.1f);
    CurrentIncubationTime = 0.0f;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(IncubationTickHandle, this, &APangeaEggActor::TickIncubation, 0.1f, true);
        World->GetTimerManager().SetTimer(HatchTimerHandle, this, &APangeaEggActor::FinishIncubation, TotalIncubationTime, false);
    }
}

void APangeaEggActor::TickIncubation()
{
    if (!TotalIncubationTime)
    {
        return;
    }

    CurrentIncubationTime += 0.1f;
    OnEggProgress.Broadcast(FMath::Clamp(CurrentIncubationTime / TotalIncubationTime, 0.f, 1.f));
}

void APangeaEggActor::FinishIncubation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IncubationTickHandle);
        World->GetTimerManager().ClearTimer(HatchTimerHandle);
    }

    OnEggProgress.Broadcast(1.0f);
    Hatch();
}

AActor* APangeaEggActor::Hatch()
{
    if (!HasAuthority() || !CreatureDefinition)
    {
        return nullptr;
    }

    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition->GetFragment<UPangeaBreedingFragment>();
    UPangeaCreatureDefinition* EffectiveDefinition = BreedingFragment && BreedingFragment->OffspringCreatureDefinition
        ? BreedingFragment->OffspringCreatureDefinition
        : CreatureDefinition;

    if (!EffectiveDefinition)
    {
        Destroy();
        return nullptr;
    }

    const TSoftClassPtr<AActor> HatchClass = EffectiveDefinition->CreatureClass;
    if (HatchClass.IsNull())
    {
        Destroy();
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* NewCreature = GetWorld()->SpawnActor<AActor>(HatchClass.LoadSynchronous(), GetActorLocation(), GetActorRotation(), Params);
    if (NewCreature)
    {
        if (BreedingFragment)
        {
            if (UPangeaBreedableComponent* BreedableComponent = NewCreature->FindComponentByClass<UPangeaBreedableComponent>())
            {
                BreedableComponent->SetInheritedStatProfile(GenerateInheritedStatProfile(BreedingFragment), true);
            }
        }

        ApplyVisualInheritance(NewCreature);
        OnEggHatched.Broadcast(NewCreature);
    }

    Destroy();
    return NewCreature;
}

FPangeaInheritedStatProfile APangeaEggActor::GenerateInheritedStatProfile(const UPangeaBreedingFragment* BreedingFragment) const
{
    FPangeaInheritedStatProfile Profile;
    if (!BreedingFragment)
    {
        return Profile;
    }

    for (const FPangeaInheritedStatRule& Rule : BreedingFragment->InheritedStatRules)
    {
        if (!Rule.StatTag.IsValid())
        {
            continue;
        }

        const float ParentAValue = GetSnapshotInheritedValue(ParentA, Rule.StatType, Rule.StatTag);
        const float ParentBValue = GetSnapshotInheritedValue(ParentB, Rule.StatType, Rule.StatTag);
        const float AverageValue = (ParentAValue + ParentBValue) * 0.5f;
        const float BestParentValue = FMath::Max(ParentAValue, ParentBValue);
        float ChildValue = FMath::Lerp(AverageValue, BestParentValue, FMath::Clamp(Rule.BestParentBias, 0.f, 1.f));

        if (FMath::FRand() < Rule.MutationChance)
        {
            ChildValue *= 1.f + FMath::FRandRange(Rule.MutationPercentMin, Rule.MutationPercentMax);
        }

        if (Rule.MaxValue > Rule.MinValue)
        {
            ChildValue = FMath::Clamp(ChildValue, Rule.MinValue, Rule.MaxValue);
        }
        else if (Rule.MinValue > 0.f)
        {
            ChildValue = FMath::Max(ChildValue, Rule.MinValue);
        }

        FPangeaInheritedStatValue& Value = Profile.Values.AddDefaulted_GetRef();
        Value.StatTag = Rule.StatTag;
        Value.StatType = Rule.StatType;
        Value.Value = ChildValue;

        UE_LOG(LogTemp, Warning, TEXT("[BreedingStats] ChildProfile Tag=%s Type=%d ParentA=%.2f ParentB=%.2f Result=%.2f"),
            *Rule.StatTag.ToString(), static_cast<int32>(Rule.StatType), ParentAValue, ParentBValue, ChildValue);
    }

    return Profile;
}

float APangeaEggActor::GetSnapshotInheritedValue(const FParentSnapshot& Snapshot, const EPangeaInheritedStatType StatType, const FGameplayTag& StatTag) const
{
    switch (StatType)
    {
    case EPangeaInheritedStatType::Statistic:
        return Snapshot.InheritedStatistics.FindRef(StatTag);

    case EPangeaInheritedStatType::PrimaryAttribute:
        return Snapshot.InheritedPrimaryAttributes.FindRef(StatTag);

    case EPangeaInheritedStatType::Attribute:
        return Snapshot.InheritedAttributes.FindRef(StatTag);

    default:
        return 0.f;
    }
}

void APangeaEggActor::ApplyVisualInheritance(AActor* NewCreature)
{
    if (!ValidateVisualInheritance(NewCreature))
    {
        return;
    }

    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition->GetFragment<UPangeaBreedingFragment>();
    const int32 SlotIndex = BreedingFragment->InheritanceMaterialSlot;
    UMaterialInstanceDynamic* MID = CreateDynamicMaterial(NewCreature, SlotIndex);
    if (!MID)
    {
        return;
    }

    const TArray<FMaterialColorGeneticGroup>& ColorGroups = BreedingFragment->MaterialColorGroups.Num() > 0
        ? BreedingFragment->MaterialColorGroups
        : BreedingFragment->MaterialGeneticGroups;

    for (const FMaterialColorGeneticGroup& Group : ColorGroups)
    {
        ApplyColorGroupInheritance(MID, Group);
    }

    for (const FMaterialScalarGeneticGroup& Group : BreedingFragment->MaterialScalarGroups)
    {
        ApplyScalarGroupInheritance(MID, Group);
    }
}

bool APangeaEggActor::ValidateVisualInheritance(AActor* NewCreature) const
{
    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition ? CreatureDefinition->GetFragment<UPangeaBreedingFragment>() : nullptr;
    if (!BreedingFragment || !BreedingFragment->bInheritParentMaterials)
    {
        return false;
    }

    if (!NewCreature || !NewCreature->FindComponentByClass<USkeletalMeshComponent>())
    {
        return false;
    }

    return ParentA.MaterialParams.Num() > 0 || ParentA.ScalarParams.Num() > 0 || ParentB.MaterialParams.Num() > 0 || ParentB.ScalarParams.Num() > 0;
}

UMaterialInstanceDynamic* APangeaEggActor::CreateDynamicMaterial(AActor* NewCreature, int32 SlotIndex)
{
    USkeletalMeshComponent* Mesh = NewCreature->FindComponentByClass<USkeletalMeshComponent>();
    if (!Mesh || SlotIndex >= Mesh->GetNumMaterials())
    {
        return nullptr;
    }

    return Mesh->CreateAndSetMaterialInstanceDynamic(SlotIndex);
}

FLinearColor APangeaEggActor::MixParentColors(const FLinearColor& ColorA, const FLinearColor& ColorB) const
{
    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition ? CreatureDefinition->GetFragment<UPangeaBreedingFragment>() : nullptr;
    const float ParentBiasPower = BreedingFragment ? BreedingFragment->ParentBiasPower : 0.8f;
    const float MutationChance = BreedingFragment ? BreedingFragment->VisualMutationChance : 0.15f;
    const float MutationIntensityMax = BreedingFragment ? BreedingFragment->VisualMutationIntensity : 0.08f;

    const float InheritBias = FMath::FRandRange(0.3f, 0.7f);
    const float CurvedBlend = FMath::Pow(InheritBias, ParentBiasPower);
    FLinearColor Mixed = FMath::Lerp(ColorA, ColorB, CurvedBlend);

    if (FMath::FRand() < MutationChance)
    {
        const float MutationIntensity = FMath::FRandRange(0.02f, MutationIntensityMax);
        const FLinearColor NeutralGray(0.5f, 0.5f, 0.5f, 1.0f);
        const FLinearColor Mutation = Mixed + (FLinearColor::MakeRandomColor() - NeutralGray) * MutationIntensity;
        Mixed = FLinearColor::LerpUsingHSV(Mutation, Mixed, 0.7f).GetClamped();
    }

    return Mixed;
}

float APangeaEggActor::MixParentScalars(float ValueA, float ValueB) const
{
    const UPangeaBreedingFragment* BreedingFragment = CreatureDefinition ? CreatureDefinition->GetFragment<UPangeaBreedingFragment>() : nullptr;
    const float ParentBiasPower = BreedingFragment ? BreedingFragment->ParentBiasPower : 0.8f;
    const float MutationChance = BreedingFragment ? BreedingFragment->VisualMutationChance : 0.15f;
    const float MutationIntensityMax = BreedingFragment ? BreedingFragment->VisualMutationIntensity : 0.08f;

    const float InheritBias = FMath::FRandRange(0.3f, 0.7f);
    const float CurvedBlend = FMath::Pow(InheritBias, ParentBiasPower);
    float Mixed = FMath::Lerp(ValueA, ValueB, CurvedBlend);

    if (FMath::FRand() < MutationChance)
    {
        Mixed += FMath::FRandRange(-MutationIntensityMax, MutationIntensityMax);
    }

    return Mixed;
}

void APangeaEggActor::ApplyColorGroupInheritance(UMaterialInstanceDynamic* MID, const FMaterialColorGeneticGroup& Group) const
{
    for (const FName& Param : Group.ParameterNames)
    {
        const FLinearColor ColorA = ParentA.MaterialParams.Contains(Param) ? ParentA.MaterialParams[Param] : FLinearColor::White;
        const FLinearColor ColorB = ParentB.MaterialParams.Contains(Param) ? ParentB.MaterialParams[Param] : FLinearColor::White;
        MID->SetVectorParameterValue(Param, MixParentColors(ColorA, ColorB));
    }
}

void APangeaEggActor::ApplyScalarGroupInheritance(UMaterialInstanceDynamic* MID, const FMaterialScalarGeneticGroup& Group) const
{
    for (const FName& Param : Group.ParameterNames)
    {
        const float ValueA = ParentA.ScalarParams.Contains(Param) ? ParentA.ScalarParams[Param] : 0.f;
        const float ValueB = ParentB.ScalarParams.Contains(Param) ? ParentB.ScalarParams[Param] : 0.f;
        MID->SetScalarParameterValue(Param, MixParentScalars(ValueA, ValueB));
    }
}

void APangeaEggActor::OnRep_CreatureDefinition()
{
}

void APangeaEggActor::OnSaved_Implementation()
{
    Super::OnSaved_Implementation();
}

bool APangeaEggActor::ShouldBeIgnored_Implementation()
{
    return Super::ShouldBeIgnored_Implementation();
}

TArray<UActorComponent*> APangeaEggActor::GetComponentsToSave_Implementation() const
{
    return TArray<UActorComponent*>();
}
