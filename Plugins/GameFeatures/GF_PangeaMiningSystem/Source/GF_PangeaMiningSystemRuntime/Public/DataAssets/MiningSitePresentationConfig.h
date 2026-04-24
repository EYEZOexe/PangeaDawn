#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interfaces/MiningPresentationAgentInterface.h"
#include "Misc/DataValidation.h"
#include "MiningSitePresentationConfig.generated.h"

class AActor;
class UAnimSequenceBase;
class USmartObjectDefinition;

USTRUCT(BlueprintType)
struct GF_PANGEAMININGSYSTEMRUNTIME_API FMiningPresentationStation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	FName PrimaryMarkerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	FName SecondaryMarkerName;
};

USTRUCT(BlueprintType)
struct GF_PANGEAMININGSYSTEMRUNTIME_API FMiningPresentationRoleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	EMiningPresentationRole Role = EMiningPresentationRole::Worker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	TSoftClassPtr<AActor> ActorClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	TSoftObjectPtr<USmartObjectDefinition> SmartObjectDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation", meta=(ClampMin="0.0"))
	float InteractionDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation", meta=(ClampMin="0.0"))
	float InteractionDurationVariance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation", meta=(ClampMin="0.0"))
	float TravelSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	TSoftObjectPtr<UAnimSequenceBase> OccupiedLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	TArray<FMiningPresentationStation> Stations;
};

UCLASS(BlueprintType)
class GF_PANGEAMININGSYSTEMRUNTIME_API UMiningSitePresentationConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	FMiningPresentationRoleConfig WorkerRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	FMiningPresentationRoleConfig GuardRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|Presentation")
	FMiningPresentationRoleConfig CourierRole;

	const FMiningPresentationRoleConfig* FindRoleConfig(EMiningPresentationRole Role) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
