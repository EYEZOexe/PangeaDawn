#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "MiningSiteChestActor.generated.h"

class UACFInteractionComponent;
class UACFStorageComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UUserWidget;

UCLASS(BlueprintType, Blueprintable)
class GF_PANGEAMININGSYSTEMRUNTIME_API AMiningSiteChestActor : public AActor, public IACFInteractableInterface
{
	GENERATED_BODY()

public:
	AMiningSiteChestActor();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UStaticMeshComponent> ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UACFStorageComponent> StorageComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|UI")
	TSubclassOf<UUserWidget> ChestWidgetClass;

	UFUNCTION(BlueprintCallable, Category="Mining|UI")
	void OpenChestMenu(APawn* InteractingPawn);

	void ClearActiveChestWidget(UUserWidget* Widget);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Mining|Storage")
	void ServerWithdrawAllToPawn(APawn* InteractingPawn);

	virtual void OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual void OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual void OnInteractableRegisteredByPawn_Implementation(APawn* Pawn) override;
	virtual void OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn) override;
	virtual FText GetInteractableName_Implementation() override;
	virtual bool CanBeInteracted_Implementation(APawn* Pawn) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> StatusText;

	TWeakObjectPtr<UUserWidget> ActiveChestWidget;

	TArray<TWeakObjectPtr<UACFInteractionComponent>> RegisteredInteractionComponents;

	UFUNCTION()
	void UpdateStatusText();
	void RefreshLocalInteractionRegistration();
	void RegisterWithInteractionComponent(UACFInteractionComponent* InteractionComponent);
	void UnregisterFromInteractionComponent(UACFInteractionComponent* InteractionComponent);
	UFUNCTION()
	void HandleInteractionBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleInteractionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
