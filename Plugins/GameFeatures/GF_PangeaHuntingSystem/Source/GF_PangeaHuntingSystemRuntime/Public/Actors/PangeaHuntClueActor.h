#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "Types/HuntingTypes.h"
#include "PangeaHuntClueActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UACFInteractionComponent;
class UDecalComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class GF_PANGEAHUNTINGSYSTEMRUNTIME_API APangeaHuntClueActor : public AActor, public IACFInteractableInterface
{
	GENERATED_BODY()

public:
	APangeaHuntClueActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeClue(const FHuntTrackPoint& InTrackPoint, const FString& InSourceCreatureName);
	void SetBloodDecalMaterial(UMaterialInterface* InMaterial);
	void SetBloodDecalSize(const FVector& InDecalSize);
	void SetBrokenFoliageDecalMaterial(UMaterialInterface* InMaterial);
	void SetBrokenFoliageDecalSize(const FVector& InDecalSize);
	void SetGenericDecalMaterial(UMaterialInterface* InMaterial);
	void SetGenericDecalSize(const FVector& InDecalSize);
	void SetInteractionRadius(float InInteractionRadius);
	void SetRevealState(bool bRevealed, float VisibilityAlpha);

	float GetCreatedServerTime() const { return CreatedServerTime; }
	float GetLifetime() const { return Lifetime; }
	EHuntClueType GetClueType() const { return ClueType; }
	UFUNCTION(BlueprintPure, Category="Hunting")
	bool IsClueIdentified() const { return bClueIdentified; }

	virtual void OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual void OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual FText GetInteractableName_Implementation() override;
	virtual bool CanBeInteracted_Implementation(APawn* Pawn) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	TObjectPtr<UDecalComponent> BloodDecal;

	UPROPERTY(ReplicatedUsing=OnRep_ClueData, VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	EHuntClueType ClueType = EHuntClueType::Custom;

	UPROPERTY(ReplicatedUsing=OnRep_ClueData, VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	FString SourceCreatureName;

	UPROPERTY(ReplicatedUsing=OnRep_ClueData, VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	float CreatedServerTime = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_ClueData, VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	float Lifetime = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_ClueData, VisibleAnywhere, BlueprintReadOnly, Category="Hunting")
	bool bClueIdentified = false;

	UFUNCTION()
	void OnRep_ClueData();

private:
	void ApplyVisualStyle();
	void RegisterForLocalInteraction(bool bShouldRegister);
	void MarkOwningTrackSetIdentified(APawn* Pawn);
	UStaticMesh* ResolveFallbackMesh() const;
	UMaterialInterface* ResolveFallbackMaterial() const;
	UMaterialInterface* ResolveFallbackBloodDecalMaterial() const;
	UMaterialInterface* ResolveFallbackBrokenFoliageDecalMaterial() const;
	UMaterialInterface* ResolveFallbackGenericDecalMaterial() const;
	FText ResolveClueDisplayName() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DecalMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BloodDecalMaterialOverride;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BrokenFoliageDecalMaterialOverride;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GenericDecalMaterialOverride;

	UPROPERTY(Transient)
	TWeakObjectPtr<UACFInteractionComponent> RegisteredInteractionComponent;

	FVector BloodDecalSize = FVector(16.f, 72.f, 72.f);
	FVector BrokenFoliageDecalSize = FVector(16.f, 96.f, 96.f);
	FVector GenericDecalSize = FVector(16.f, 72.f, 72.f);
	float InteractionRadius = 88.f;

	bool bIsRevealedLocally = false;
	bool bRegisteredForLocalInteraction = false;
};
