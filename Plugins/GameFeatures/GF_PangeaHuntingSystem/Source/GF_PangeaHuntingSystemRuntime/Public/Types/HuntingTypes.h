#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HuntingTypes.generated.h"

UENUM(BlueprintType)
enum class EHuntClueType : uint8
{
	Footprint UMETA(DisplayName="Footprint"),
	Blood UMETA(DisplayName="Blood"),
	BrokenFoliage UMETA(DisplayName="Broken Foliage"),
	EatenFood UMETA(DisplayName="Eaten Food"),
	Scent UMETA(DisplayName="Scent"),
	Droppings UMETA(DisplayName="Droppings"),
	Custom UMETA(DisplayName="Custom")
};

USTRUCT(BlueprintType)
struct GF_PANGEAHUNTINGSYSTEMRUNTIME_API FHuntTrackPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	float CreatedServerTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	float Lifetime = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	EHuntClueType ClueType = EHuntClueType::Footprint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	FGameplayTag SurfaceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	FGameplayTag GaitTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting")
	float Strength = 1.f;
};
