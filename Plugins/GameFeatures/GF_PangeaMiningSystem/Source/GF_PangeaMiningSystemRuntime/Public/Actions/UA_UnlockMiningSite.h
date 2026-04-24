#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Objects/UpgradeAction.h"
#include "UA_UnlockMiningSite.generated.h"

UENUM(BlueprintType)
enum class EMiningUnlockMode : uint8
{
	Unlock,
	Lock,
	Toggle
};

UCLASS(BlueprintType, EditInlineNew)
class GF_PANGEAMININGSYSTEMRUNTIME_API UUA_UnlockMiningSite : public UUpgradeAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining")
	FGameplayTag FacilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mining")
	EMiningUnlockMode Mode = EMiningUnlockMode::Unlock;

	virtual void ExecuteForContext(const FBaseUpgradeContext& Context) override;
	virtual void Execute_Implementation(UObject* ContextObject) override;

private:
	void ApplyToWorld(UWorld* World) const;
	void ApplyToActor(AActor* Actor) const;
};
