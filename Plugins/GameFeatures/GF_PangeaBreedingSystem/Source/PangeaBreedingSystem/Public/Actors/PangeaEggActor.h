// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Definitions/PangeaBreedingFragment.h"
#include "Definitions/PangeaCreatureDefinition.h"
#include "Types/BreedingTypes.h"
#include "Items/ACFWorldItem.h"
#include "PangeaEggActor.generated.h"

class UPangeaGeneticStrategy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEggProgress, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEggHatched, AActor*, NewCreature);

/**
 * 
 */
UCLASS()
class PANGEABREEDINGSYSTEM_API APangeaEggActor : public AACFWorldItem
{
	GENERATED_BODY()
    
public:
    APangeaEggActor();
    
    UPROPERTY(ReplicatedUsing=OnRep_CreatureDefinition, Transient)
    TObjectPtr<UPangeaCreatureDefinition> CreatureDefinition;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Breeding")
    FParentSnapshot ParentA;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Breeding")
    FParentSnapshot ParentB;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Breeding")
    FGeneticTraitSet ChildTraits;

    UFUNCTION(BlueprintCallable, Category="Breeding")
    UPangeaCreatureDefinition* GetCreatureDefinition() const { return CreatureDefinition; }

    UFUNCTION(BlueprintCallable, Category="Breeding")
    FName GetSpeciesID() const { return CreatureDefinition ? CreatureDefinition->SpeciesId : NAME_None; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEggProgress OnEggProgress;

    UPROPERTY(BlueprintAssignable)
    FOnEggHatched OnEggHatched;

    void InitializeEgg(const FParentSnapshot& InA, const FParentSnapshot& InB, UPangeaGeneticStrategy* Strategy, UPangeaCreatureDefinition* InCreatureDefinition);

    UFUNCTION(BlueprintCallable)
    AActor* Hatch();
        void ApplyVisualInheritance(AActor* NewCreature);

        virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void OnSaved_Implementation() override;
    virtual bool ShouldBeIgnored_Implementation() override;
    virtual TArray<UActorComponent*> GetComponentsToSave_Implementation() const override;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_CreatureDefinition();

    FTimerHandle HatchTimerHandle;
    FTimerHandle IncubationTickHandle;

    float CurrentIncubationTime = 0.0f;
    float TotalIncubationTime = 0.0f;

    void StartIncubation();
    void TickIncubation();
    void FinishIncubation();

    bool ValidateVisualInheritance(AActor* NewCreature) const;
    UMaterialInstanceDynamic* CreateDynamicMaterial(AActor* NewCreature, int32 SlotIndex);
    FLinearColor MixParentColors(const FLinearColor& ColorA, const FLinearColor& ColorB) const;
    float MixParentScalars(float ValueA, float ValueB) const;
    void ApplyColorGroupInheritance(UMaterialInstanceDynamic* MID, const FMaterialColorGeneticGroup& Group) const;
    void ApplyScalarGroupInheritance(UMaterialInstanceDynamic* MID, const FMaterialScalarGeneticGroup& Group) const;
};
