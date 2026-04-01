#pragma once

#include "CoreMinimal.h"
#include "Definitions/PangeaDefinitionFragment.h"
#include "Types/BreedingTypes.h"
#include "PangeaBreedingFragment.generated.h"

class AActor;
class APangeaEggActor;
class UPangeaGeneticStrategy;
class UPangeaCreatureDefinition;

USTRUCT(BlueprintType)
struct FMaterialColorGeneticGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName GroupName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> ParameterNames;
};

USTRUCT(BlueprintType)
struct FMaterialScalarGeneticGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName GroupName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> ParameterNames;
};

UCLASS(BlueprintType, EditInlineNew)
class PANGEABREEDINGSYSTEM_API UPangeaBreedingFragment : public UPangeaDefinitionFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Egg")
	TSubclassOf<APangeaEggActor> EggActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Egg")
	TObjectPtr<UPangeaCreatureDefinition> OffspringCreatureDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Genetics")
	TSubclassOf<UPangeaGeneticStrategy> GeneticStrategyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding")
	FIncubationConfig Incubation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding")
	FFertilityConfig Fertility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Appearance")
	bool bInheritParentMaterials = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Appearance")
	int32 InheritanceMaterialSlot = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Breeding|Appearance")
	float VisualMutationChance = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Breeding|Appearance")
	float ParentBiasPower = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Breeding|Appearance")
	float VisualMutationIntensity = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Appearance")
	TArray<FMaterialColorGeneticGroup> MaterialColorGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Appearance")
	TArray<FMaterialScalarGeneticGroup> MaterialScalarGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Breeding|Appearance", meta=(DeprecatedProperty, DeprecationMessage="Use MaterialColorGroups instead."))
	TArray<FMaterialColorGeneticGroup> MaterialGeneticGroups;
};
