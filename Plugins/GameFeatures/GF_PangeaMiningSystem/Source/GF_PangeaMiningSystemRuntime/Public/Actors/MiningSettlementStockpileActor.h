#pragma once

#include "CoreMinimal.h"
#include "ALSSavableInterface.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "Items/ACFItem.h"
#include "MiningSettlementStockpileActor.generated.h"

class UACFInteractionComponent;
class UACFStorageComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UUserWidget;

UCLASS(BlueprintType, Blueprintable)
class GF_PANGEAMININGSYSTEMRUNTIME_API AMiningSettlementStockpileActor : public AActor, public IACFInteractableInterface, public IALSSavableInterface
{
	GENERATED_BODY()

public:
	AMiningSettlementStockpileActor();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual TArray<UActorComponent*> GetComponentsToSave_Implementation() const override;
	virtual void OnLoaded_Implementation() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UStaticMeshComponent> StockpileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UTextRenderComponent> StockpileLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining|UI")
	TSubclassOf<UUserWidget> StockpileWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<USceneComponent> CourierUnloadMarker;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining")
	TObjectPtr<UACFStorageComponent> StorageComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining")
	TArray<FBaseItem> InitialStock;

	UFUNCTION(BlueprintCallable, Category="Mining|UI")
	void OpenStockpileMenu(APawn* InteractingPawn);

	void ClearActiveStockpileWidget(UUserWidget* Widget);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Mining|Storage")
	void ServerWithdrawAllToPawn(APawn* InteractingPawn);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Mining|Storage")
	void ServerDepositAllFromPawn(APawn* InteractingPawn);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Mining|Storage")
	void ServerTransferItemToPawn(APawn* InteractingPawn, TSubclassOf<class UACFItem> ItemClass, int32 Count);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Mining|Storage")
	void ServerTransferItemFromPawn(APawn* InteractingPawn, TSubclassOf<class UACFItem> ItemClass, int32 Count);

	FVector GetCourierUnloadLocation() const;

	virtual void OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual void OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType = "") override;
	virtual void OnInteractableRegisteredByPawn_Implementation(APawn* Pawn) override;
	virtual void OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn) override;
	virtual FText GetInteractableName_Implementation() override;
	virtual bool CanBeInteracted_Implementation(APawn* Pawn) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> InteractionSphere;

	TWeakObjectPtr<UUserWidget> ActiveStockpileWidget;

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
