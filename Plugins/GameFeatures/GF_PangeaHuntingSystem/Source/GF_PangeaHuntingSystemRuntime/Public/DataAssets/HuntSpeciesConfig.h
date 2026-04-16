#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/HuntingTypes.h"
#include "HuntSpeciesConfig.generated.h"

class AActor;
class APangeaHuntClueActor;
class UCurveFloat;
class UMaterialInterface;
class UStaticMesh;
class UUserWidget;

USTRUCT(BlueprintType)
struct GF_PANGEAHUNTINGSYSTEMRUNTIME_API FHuntSpecialClueConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	EHuntClueType ClueType = EHuntClueType::Custom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	bool bEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	bool bUseRandomChance = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ChancePerEligibleTrack = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting", meta=(ClampMin="0.0"))
	float MinInterval = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting", meta=(ClampMin="0.0"))
	float MaxInterval = 35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting", meta=(ClampMin="1.0"))
	float Lifetime = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting")
	FVector DecalSize = FVector(16.f, 72.f, 72.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting", meta=(ClampMin="1.0"))
	float InteractionRadius = 88.f;
};

UCLASS(BlueprintType)
class GF_PANGEAHUNTINGSYSTEMRUNTIME_API UHuntSpeciesConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks")
	bool bLeavesTracks = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks", meta=(ClampMin="1.0"))
	float MinDistanceBetweenTracks = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks", meta=(ClampMin="0.0"))
	float MinSpeedToLeaveTracks = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks", meta=(ClampMin="1.0"))
	float TrackLifetime = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks", meta=(ClampMin="1"))
	int32 MaxTrackPoints = 32;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks")
	float TrackZOffset = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tracks")
	float FootLateralOffset = 28.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Reveal", meta=(ClampMin="100.0"))
	float RevealRadius = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Reveal")
	TObjectPtr<UCurveFloat> FreshnessVisibilityCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Visuals")
	TSubclassOf<AActor> FootprintVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Visuals")
	TObjectPtr<UMaterialInterface> FootprintDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Visuals")
	TObjectPtr<UStaticMesh> FootprintStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Visuals")
	TObjectPtr<UMaterialInterface> FootprintStaticMeshMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Visuals", meta=(ClampMin="0.01"))
	float FootprintVisualScale = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues")
	bool bSpawnBloodClues = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues")
	bool bSpawnBrokenFoliageClues = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BloodHealthThreshold = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues", meta=(ClampMin="0.1"))
	float MinSpecialClueInterval = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues")
	bool bUseRandomSpecialClueInterval = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues", meta=(ClampMin="0.1", EditCondition="bUseRandomSpecialClueInterval"))
	float MaxSpecialClueInterval = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues")
	TArray<FHuntSpecialClueConfig> AdditionalSpecialClues;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues")
	TSubclassOf<APangeaHuntClueActor> SpecialClueActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Blood")
	TObjectPtr<UMaterialInterface> BloodDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Blood")
	FVector BloodDecalSize = FVector(16.f, 72.f, 72.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Blood", meta=(ClampMin="1.0"))
	float BloodClueLifetime = 420.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Blood", meta=(ClampMin="1.0"))
	float BloodInteractionRadius = 88.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Broken Foliage")
	TObjectPtr<UMaterialInterface> BrokenFoliageDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Broken Foliage")
	FVector BrokenFoliageDecalSize = FVector(16.f, 96.f, 96.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Broken Foliage", meta=(ClampMin="1.0"))
	float BrokenFoliageClueLifetime = 240.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|SpecialClues|Broken Foliage", meta=(ClampMin="1.0"))
	float BrokenFoliageInteractionRadius = 96.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus")
	bool bEnableFootprintFocusIdentification = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus", meta=(ClampMin="0.05"))
	float FootprintFocusDuration = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus", meta=(ClampMin="1.0"))
	float FootprintFocusScreenRadius = 48.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus")
	bool bAutoIdentifyFocusedFootprints = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus")
	bool bShowFootprintFocusWidget = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus")
	FVector2D FootprintFocusWidgetSize = FVector2D(96.f, 96.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Focus")
	FVector2D FootprintFocusWidgetScreenOffset = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Messages")
	TSoftClassPtr<UUserWidget> ACFNotificationsListWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(TEXT("/Game/FullSample/Integrations/UIIntegrations/Widgets/ACF_NotificationsList_WB.ACF_NotificationsList_WB_C")));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Messages")
	TSoftClassPtr<UUserWidget> FallbackOnScreenMessageWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(TEXT("/Game/FullSample/UI/ACF_OnScreenMessage_WBP.ACF_OnScreenMessage_WBP_C")));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Messages")
	FText TracksIdentifiedMessage = FText::FromString(TEXT("Tracks Identified"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Messages", meta=(ClampMin="0.1"))
	float TracksIdentifiedMessageDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Debug")
	bool bEnableDebugMessages = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tags")
	FGameplayTag HuntingTrackTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunting|Tags")
	FGameplayTag HuntingRevealRequiredTag;
};
