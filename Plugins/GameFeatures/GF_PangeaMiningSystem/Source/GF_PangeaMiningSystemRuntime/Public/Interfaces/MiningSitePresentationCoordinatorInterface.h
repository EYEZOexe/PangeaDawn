#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MiningSitePresentationCoordinatorInterface.generated.h"

class AActor;
class AMiningSiteActor;
class UMiningSiteComponent;
class USmartObjectComponent;

UINTERFACE()
class GF_PANGEAMININGSYSTEMRUNTIME_API UMiningSitePresentationCoordinatorInterface : public UInterface
{
	GENERATED_BODY()
};

class GF_PANGEAMININGSYSTEMRUNTIME_API IMiningSitePresentationCoordinatorInterface
{
	GENERATED_BODY()

public:
	virtual void RefreshPresentationActors(
		AMiningSiteActor& SiteActor,
		UMiningSiteComponent* MiningSiteComponent,
		AActor* SpawnedSiteChest,
		TArray<TObjectPtr<AActor>>& SpawnedWorkerActors,
		TArray<TObjectPtr<AActor>>& SpawnedGuardActors,
		TObjectPtr<AActor>& SpawnedCourierActor) = 0;

	virtual void ClearPresentationActors(
		TArray<TObjectPtr<AActor>>& SpawnedWorkerActors,
		TArray<TObjectPtr<AActor>>& SpawnedGuardActors,
		TObjectPtr<AActor>& SpawnedCourierActor) = 0;

	virtual void ConfigureSmartObjectComponents(
		AMiningSiteActor& SiteActor,
		UMiningSiteComponent* MiningSiteComponent,
		TArray<TObjectPtr<USmartObjectComponent>>& WorkerSmartObjectComponents,
		TArray<TObjectPtr<USmartObjectComponent>>& GuardSmartObjectComponents) const = 0;

	virtual void UpdatePresentationActorMovement(
		AMiningSiteActor& SiteActor,
		UMiningSiteComponent* MiningSiteComponent,
		AActor* SpawnedSiteChest,
		TArray<TObjectPtr<AActor>>& SpawnedWorkerActors,
		TArray<TObjectPtr<AActor>>& SpawnedGuardActors,
		TObjectPtr<AActor>& SpawnedCourierActor,
		const TArray<TObjectPtr<USmartObjectComponent>>& WorkerSmartObjectComponents,
		const TArray<TObjectPtr<USmartObjectComponent>>& GuardSmartObjectComponents) = 0;

	virtual float GetPresentationRefreshInterval() const = 0;
	virtual float GetPresentationMovementInterval() const = 0;
};
