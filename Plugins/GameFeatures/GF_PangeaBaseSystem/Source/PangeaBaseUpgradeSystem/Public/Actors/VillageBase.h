// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ALSSavableInterface.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "VillageBase.generated.h"

class UBoxComponent;
class UPangeaFacilityManagerComponent;
class UPangeaUpgradeSystemComponent;

UCLASS()
class PANGEABASEUPGRADESYSTEM_API AVillageBase : public AActor, public IACFInteractableInterface, public IALSSavableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AVillageBase();

protected:
	virtual void PreInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual TArray<UActorComponent*> GetComponentsToSave_Implementation() const override;
	virtual void OnLoaded_Implementation() override;
	

public:

	/** Defines the spatial size of the base */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Village")
	UBoxComponent* VillageBounds;

	/** Player interaction zone for upgrading */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Village")
	UBoxComponent* InteractionVolume;

	// -----------------------------
	// ACFU INTERACTION INTERFACE
	// -----------------------------

	/** Text that appears when player looks at interaction point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	FText InteractionText;

	/** Triggered when player interacts (ACFU interface) */
	virtual void OnLocalInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

	/** Interaction Name for UI */
	virtual FText GetInteractableName_Implementation() override;

	/** Interaction allowed? */
	virtual bool CanBeInteracted_Implementation(class APawn* Pawn) override;
	
	bool UpgradeBase(APawn* InstigatorPawn) const;
	
	//UI Integration
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade|UI")
	TSubclassOf<class UVillageUpgradeMenuWidget> UpgradeMenuClass;

	UFUNCTION(BlueprintCallable, Category="Upgrade|UI")
	void OpenUpgradeMenu(APawn* InteractingPawn);

	UFUNCTION(BlueprintPure, Category="Upgrade")
	UPangeaUpgradeSystemComponent* GetUpgradeSystem() const;

	UFUNCTION(BlueprintPure, Category="Upgrade")
	UPangeaFacilityManagerComponent* GetFacilityManager() const;

private:
	class UActorComponent* GetUpgradeProviderComponent() const;
	bool HasActiveUpgradeFeature() const;

};
