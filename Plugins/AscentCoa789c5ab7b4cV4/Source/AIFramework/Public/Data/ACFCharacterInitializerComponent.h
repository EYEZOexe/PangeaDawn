// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACFAITypes.h"
#include "ACFCharacterInitializerComponent.generated.h"

class UACFCharacterDataAsset;


UCLASS(ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class AIFRAMEWORK_API UACFCharacterInitializerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UACFCharacterInitializerComponent();

	/**
	 * Initializes the character from a data asset at the specified level.
	 * This applies stats, abilities, equipment, and appearance defined in the data asset.
	 *
	 * @param charData The character data asset containing initialization parameters.
	 * @param Level The starting level for stat and ability scaling.
	 */
	UFUNCTION(BlueprintCallable, Category = ACF)
	void InitFromDataAsset(UACFCharacterDataAsset* charData, int32 Level);

	/**
	 * Applies all mesh data (skeletal meshes, materials, morph targets) to the owning character.
	 * Uses the currently cached mesh configuration.
	 */
	UFUNCTION(BlueprintCallable, Category = ACF)
	void ApplyAllMeshData();

	/**
	 * Applies visual appearance from a data asset to the owning character. 
	 * NOT REPLICATED
	 *
	 * @param charData The character data asset containing appearance configuration.
	 * @param bApplyEquip If true, also equips items defined in the data asset; if false, only applies mesh and material data.
	 */
	UFUNCTION(BlueprintCallable, Category = ACF)
	void ApplyAppearanceFromDataAsset(UACFCharacterDataAsset* charData, bool bApplyEquip);

	// Get DataAsset applied to the character
	UFUNCTION(BlueprintCallable, Category = ACF)
	UACFCharacterDataAsset* GetCharacterDataAsset() { return CharacterDataAsset; };

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InternalHandleServerInit(int32 Level);
	void InternalHandleClientInit();
	// Custom initialization logic for server and client
	UFUNCTION(BlueprintNativeEvent, Category = ACF)
	void HandleServerInit();

	UFUNCTION(BlueprintNativeEvent, Category = ACF)
	void HandleClientInit();

	// Auto - initialization on BeginPlay
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Auto Initialization")
	bool bAutoInit = false;

	// Soft reference to data asset for auto-initialization
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Auto Initialization",
		meta = (EditCondition = "bAutoInit", EditConditionHides))
	TSoftObjectPtr<UACFCharacterDataAsset> AutoInitDataAsset;

	// Default level for auto-initialization
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Auto Initialization",
		meta = (EditCondition = "bAutoInit", EditConditionHides, ClampMin = "1", ClampMax = "100"))
	int32 AutoInitLevel = 1;

	// Replicated data asset ID - more efficient than replicating the asset pointer
	UPROPERTY(ReplicatedUsing = OnRep_CharacterDataAssetId, BlueprintReadOnly, Category = "ACF|Character Data")
	FPrimaryAssetId CharacterDataAssetId;

	// Apply specific mesh data to a component
	void ApplyMeshDataToComponent(class USkeletalMeshComponent* Component,
		const FSkeletalMeshComponentData& MeshData);

	// Local cached data asset (not replicated)
	UPROPERTY(BlueprintReadOnly, Category = "ACF|Character Data")
	UACFCharacterDataAsset* CharacterDataAsset;

	// Called when data asset ID is replicated
	UFUNCTION()
	void OnRep_CharacterDataAssetId();

	UPROPERTY(Category = ACF, BlueprintReadOnly)
	AACFCharacter* OwningPawn;

	void ApplyFragmentsData();
	
	void ApplyEquipData();
private:

	// Load data asset from ID
	void LoadDataAssetFromId();

	void OnDataAssetLoaded();



	bool bHasInitialized = false;

	// Handle for async loading
	TSharedPtr<struct FStreamableHandle> LoadHandle;


};
