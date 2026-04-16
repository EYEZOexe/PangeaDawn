#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Types/HuntingTypes.h"
#include "PangeaHunterSenseComponent.generated.h"

class AACFCharacter;
class UACFCharacterMovementComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPangeaFootprintFocusWidget;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPangeaFootprintFocusSignature, bool, bHasTarget, float, Progress, AActor*, SourceActor);

USTRUCT()
struct FPangeaHuntVisualSlot
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> VisualComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance = nullptr;

	FName TrackKey = NAME_None;
	TWeakObjectPtr<AActor> SourceActor;
	EHuntClueType ClueType = EHuntClueType::Custom;
	bool bAssignedThisUpdate = false;
};

UCLASS(ClassGroup=(Hunting), meta=(BlueprintSpawnableComponent))
class GF_PANGEAHUNTINGSYSTEMRUNTIME_API UPangeaHunterSenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPangeaHunterSenseComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Hunting|Sense")
	bool IdentifyNearestVisibleFootprint();

	UFUNCTION(BlueprintPure, Category="Hunting|Sense")
	float GetFootprintFocusProgress() const { return FootprintFocusProgress; }

	UFUNCTION(BlueprintPure, Category="Hunting|Sense")
	bool HasFocusedFootprintTarget() const { return FocusedFootprintSourceActor.IsValid(); }

	UPROPERTY(BlueprintAssignable, Category="Hunting|Sense")
	FPangeaFootprintFocusSignature OnFootprintFocusChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense")
	bool bRequireAimAndCrouch = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense", meta=(ClampMin="0.01"))
	float ScanInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense")
	bool bDebugDrawTracks = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Debug")
	bool bEnableDebugMessages = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Debug", meta=(ClampMin="0.01"))
	float DebugMessageDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense", meta=(ClampMin="1.0"))
	float DebugSphereRadius = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense", meta=(ClampMin="0.01"))
	float DebugDrawDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense")
	bool bUsePlaceholderMeshVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense", meta=(ClampMin="1"))
	int32 MaxVisualPoolSize = 48;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	bool bEnableFootprintFocusIdentification = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus", meta=(ClampMin="0.05"))
	float FootprintFocusDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus", meta=(ClampMin="1.0"))
	float FootprintFocusScreenRadius = 48.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	bool bAutoIdentifyFocusedFootprints = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	bool bShowFootprintFocusWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	TSubclassOf<UPangeaFootprintFocusWidget> FootprintFocusWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	FVector2D FootprintFocusWidgetSize = FVector2D(96.f, 96.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Footprint Focus")
	FVector2D FootprintFocusWidgetScreenOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Messages")
	TSoftClassPtr<UUserWidget> ACFNotificationsListWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(TEXT("/Game/FullSample/Integrations/UIIntegrations/Widgets/ACF_NotificationsList_WB.ACF_NotificationsList_WB_C")));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Messages")
	TSoftClassPtr<UUserWidget> FallbackOnScreenMessageWidgetClass = TSoftClassPtr<UUserWidget>(FSoftClassPath(TEXT("/Game/FullSample/UI/ACF_OnScreenMessage_WBP.ACF_OnScreenMessage_WBP_C")));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Messages")
	FText TracksIdentifiedMessage = FText::FromString(TEXT("Tracks Identified"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Messages", meta=(ClampMin="0.1"))
	float TracksIdentifiedMessageDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Stats")
	bool bUseStatModifiedRevealRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Stats")
	FGameplayTag RevealRadiusStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Stats")
	float FlatRevealRadiusBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Stats")
	float RevealRadiusPerStatPoint = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Sense|Stats", meta=(ClampMin="0.0"))
	float MaxRevealRadiusBonus = 2500.f;

	UFUNCTION()
	void HandleAimChanged(bool bIsNowAiming);

	UFUNCTION()
	void HandleCrouchChanged(bool bIsNowCrouched);

private:
	void ResolveOwnerState();
	void RefreshSenseState();
	void DrawNearbyTracks();
	void UpdateTrackVisuals();
	void UpdateFootprintFocus(float DeltaTime);
	AActor* FindFocusedFootprintSource() const;
	void ResetFootprintFocus();
	void EnsureFootprintFocusWidget();
	void UpdateFootprintFocusWidget(bool bHasTarget, float Progress);
	void ShowTracksIdentifiedMessage();
	void PrintDebugMessage(const FString& Message, const FColor& Color = FColor::Green, bool bLogAlso = false) const;
	void ResetVisualAssignments();
	UStaticMeshComponent* AcquireVisualSlot(const FName& TrackKey);
	void ReleaseUnusedVisuals();
	FName BuildTrackKey(const AActor* SourceActor, const FHuntTrackPoint& TrackPoint) const;
	UStaticMesh* ResolveFallbackMesh() const;
	UMaterialInterface* ResolveFallbackMaterial() const;
	bool IsLocallyControlledHunter() const;
	float GetModifiedRevealRadius(float BaseRevealRadius) const;

	UFUNCTION(Server, Reliable)
	void ServerIdentifyFootprints(AActor* SourceActor);

	TWeakObjectPtr<AACFCharacter> CachedOwnerCharacter;
	TWeakObjectPtr<UACFCharacterMovementComponent> CachedMovementComponent;
	TWeakObjectPtr<AActor> NearestVisibleFootprintSourceActor;
	TWeakObjectPtr<AActor> FocusedFootprintSourceActor;
	UPROPERTY(Transient)
	TObjectPtr<UPangeaFootprintFocusWidget> FootprintFocusWidget;
	bool bIsAiming = false;
	bool bIsCrouched = false;
	bool bHuntingSenseActive = false;
	bool bFocusedFootprintAlreadyIdentified = false;
	float ScanAccumulator = 0.f;
	float FootprintFocusProgress = 0.f;
	TArray<FPangeaHuntVisualSlot> VisualPool;
};
