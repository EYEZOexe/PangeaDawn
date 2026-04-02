#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FacilityManagerComponent.generated.h"


class AFacilityGroup;

USTRUCT()
struct FFacilityEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag FacilityTag;

	UPROPERTY()
	TWeakObjectPtr<AFacilityGroup> Group;

	UPROPERTY()
	bool bUnlocked = false;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Pangea), meta=(BlueprintSpawnableComponent))
class PANGEABASEUPGRADESYSTEM_API UPangeaFacilityManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPangeaFacilityManagerComponent();

	virtual void BeginPlay() override;

	/* Enable/disable a facility by tag */
	void EnableFacility(const FGameplayTag& FacilityTag);
	void DisableFacility(const FGameplayTag& FacilityTag);

	/* Check state */
	bool IsFacilityEnabled(const FGameplayTag& FacilityTag) const;
	
	//Get all facilities
	const TArray<FFacilityEntry>& GetAllFacilities() const { return Facilities; }

protected:

	/* Locate all FacilityGroups under this actor */
	void DiscoverFacilityGroups();

	/* Storage */
	UPROPERTY()
	TArray<FFacilityEntry> Facilities;
};
