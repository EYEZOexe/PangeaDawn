#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BreedingTypes.generated.h"

UENUM(BlueprintType)
enum class ECreatureGender : uint8
{
	Unspecified UMETA(DisplayName="Unspecified"),
	Male UMETA(DisplayName="Male"),
	Female UMETA(DisplayName="Female")
};

UENUM(BlueprintType)
enum class ECreatureGenderAssignmentMode : uint8
{
	UseDefault UMETA(DisplayName="Use Default"),
	Randomize UMETA(DisplayName="Randomize")
};

USTRUCT(BlueprintType)
struct PANGEACORE_API FGeneticTrait
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.f;
};

USTRUCT(BlueprintType)
struct PANGEACORE_API FGeneticTraitSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGeneticTrait> Traits;

	float GetValue(FName InName, float Default = 0.f) const
	{
		if (const FGeneticTrait* Found = Traits.FindByPredicate([&](const FGeneticTrait& T) { return T.Name == InName; }))
		{
			return Found->Value;
		}
		return Default;
	}

	void SetValue(FName InName, float InValue)
	{
		if (FGeneticTrait* Found = Traits.FindByPredicate([&](const FGeneticTrait& T) { return T.Name == InName; }))
		{
			Found->Value = InValue;
		}
		else
		{
			Traits.Add({InName, InValue});
		}
	}
};

USTRUCT(BlueprintType)
struct PANGEACORE_API FParentSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SpeciesID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid CreatureId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGeneticTraitSet Traits;

	UPROPERTY()
	TSoftObjectPtr<AActor> ParentActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FLinearColor> VisualData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FLinearColor> MaterialParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, float> ScalarParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, float> InheritedStatistics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, float> InheritedPrimaryAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, float> InheritedAttributes;
};

USTRUCT(BlueprintType)
struct PANGEACORE_API FIncubationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IncubationSeconds = 120.f;
};

USTRUCT(BlueprintType)
struct PANGEACORE_API FFertilityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding")
	float FertilityCooldownSeconds = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding")
	bool bAffectsBothParents = true;
};
