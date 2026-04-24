#include "Components/MiningSitePresentationCoordinatorComponent.h"

#include "Actors/MiningSiteActor.h"
#include "AIController.h"
#include "Components/ACFInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogPangeaMiningPresentationActorSync, Log, All);

void UMiningSitePresentationCoordinatorComponent::SyncPresentationActorsForRole(
	AMiningSiteActor& SiteActor,
	TArray<TObjectPtr<AActor>>& ActorArray,
	TSoftClassPtr<AActor> ActorClass,
	int32 DesiredCount,
	float Radius,
	float AngleOffsetDegrees,
	EMiningPresentationRole Role)
{
	while (ActorArray.Num() > DesiredCount)
	{
		if (AActor* ExistingActor = ActorArray.Pop())
		{
			ReleaseSmartObjectForActor(ExistingActor);
			PresentationRoutePhaseMap.Remove(ExistingActor);
			PresentationMoveTargetMap.Remove(ExistingActor);
			PresentationSmartObjectLastStatusMap.Remove(ExistingActor);
			PresentationSmartObjectStartTimeMap.Remove(ExistingActor);
			PresentationTargetHoldStartedMap.Remove(ExistingActor);
			ApplyAgentPresentation(ExistingActor, Role, EMiningPresentationState::Idle);
			ExistingActor->Destroy();
		}
	}

	if (DesiredCount <= 0 || ActorClass.IsNull())
	{
		return;
	}

	UClass* LoadedClass = ActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	while (ActorArray.Num() < DesiredCount)
	{
		const int32 SpawnIndex = ActorArray.Num();
		const float AngleStep = DesiredCount > 1 ? 40.0f : 0.0f;
		const FVector SpawnLocation = GetPresentationSlotLocation(SiteActor, Radius, AngleOffsetDegrees + (SpawnIndex * AngleStep));

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = &SiteActor;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* SpawnedActor = SiteActor.GetWorld()->SpawnActor<AActor>(LoadedClass, SpawnLocation, SiteActor.GetActorRotation(), SpawnParameters);
		if (!SpawnedActor)
		{
			break;
		}

		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(SpawnedActor);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!PrimitiveComponent)
			{
				continue;
			}

			if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(PrimitiveComponent))
			{
				CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
				CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
				CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			}
			else
			{
				PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}

		if (UACFInteractionComponent* InteractionComponent = SpawnedActor->FindComponentByClass<UACFInteractionComponent>())
		{
			InteractionComponent->Deactivate();
		}

		SpawnedActor->Tags.Add(Role == EMiningPresentationRole::Worker ? FName(TEXT("Mining.Worker")) : FName(TEXT("Mining.Guard")));
		SpawnedActor->Tags.Add(FName(*FString::Printf(TEXT("Mining.Site.%s"), *SiteActor.GetName())));
		if (APawn* SpawnedPawn = Cast<APawn>(SpawnedActor))
		{
			SpawnedPawn->SpawnDefaultController();
			UE_LOG(LogPangeaMiningPresentationActorSync, Log, TEXT("Spawned mining pawn controller sync. Site=%s Pawn=%s Controller=%s"), *GetNameSafe(&SiteActor), *GetNameSafe(SpawnedPawn), *GetNameSafe(SpawnedPawn->GetController()));
		}

		PresentationRoutePhaseMap.Add(SpawnedActor, (SpawnIndex % 2) == 1);
		PresentationTargetHoldStartedMap.Add(SpawnedActor, false);
		ApplyAgentPresentation(SpawnedActor, Role, EMiningPresentationState::Idle);
		ActorArray.Add(SpawnedActor);
		UE_LOG(LogPangeaMiningPresentationActorSync, Log, TEXT("Spawned mining presentation actor. Site=%s Actor=%s Class=%s"), *GetNameSafe(&SiteActor), *GetNameSafe(SpawnedActor), *GetNameSafe(LoadedClass));
	}
}

void UMiningSitePresentationCoordinatorComponent::SyncCourierPresentationActor(
	AMiningSiteActor& SiteActor,
	TObjectPtr<AActor>& SpawnedCourierActor,
	TSoftClassPtr<AActor> ActorClass,
	bool bShouldExist,
	float Distance,
	float AngleOffsetDegrees)
{
	if (!bShouldExist)
	{
		if (SpawnedCourierActor)
		{
			ReleaseSmartObjectForActor(SpawnedCourierActor);
			PresentationRoutePhaseMap.Remove(SpawnedCourierActor);
			PresentationMoveTargetMap.Remove(SpawnedCourierActor);
			PresentationSmartObjectLastStatusMap.Remove(SpawnedCourierActor);
			PresentationSmartObjectStartTimeMap.Remove(SpawnedCourierActor);
			PresentationTargetHoldStartedMap.Remove(SpawnedCourierActor);
			ApplyAgentPresentation(SpawnedCourierActor, EMiningPresentationRole::Courier, EMiningPresentationState::Idle);
			SpawnedCourierActor->Destroy();
			SpawnedCourierActor = nullptr;
		}
		return;
	}

	if (SpawnedCourierActor)
	{
		return;
	}

	UClass* LoadedClass = ActorClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = &SiteActor;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedCourierActor = SiteActor.GetWorld()->SpawnActor<AActor>(LoadedClass, GetPresentationSlotLocation(SiteActor, Distance, AngleOffsetDegrees), SiteActor.GetActorRotation(), SpawnParameters);
	if (!SpawnedCourierActor)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(SpawnedCourierActor);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(PrimitiveComponent))
		{
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
			CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
		else
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (UACFInteractionComponent* InteractionComponent = SpawnedCourierActor->FindComponentByClass<UACFInteractionComponent>())
	{
		InteractionComponent->Deactivate();
	}

	SpawnedCourierActor->Tags.Add(FName(TEXT("Mining.Courier")));
	SpawnedCourierActor->Tags.Add(FName(*FString::Printf(TEXT("Mining.Site.%s"), *SiteActor.GetName())));
	if (APawn* SpawnedPawn = Cast<APawn>(SpawnedCourierActor))
	{
		SpawnedPawn->SpawnDefaultController();
		UE_LOG(LogPangeaMiningPresentationActorSync, Log, TEXT("Spawned mining courier controller sync. Site=%s Pawn=%s Controller=%s"), *GetNameSafe(&SiteActor), *GetNameSafe(SpawnedPawn), *GetNameSafe(SpawnedPawn->GetController()));
	}

	PresentationRoutePhaseMap.Add(SpawnedCourierActor, false);
	PresentationTargetHoldStartedMap.Add(SpawnedCourierActor, false);
	ApplyAgentPresentation(SpawnedCourierActor, EMiningPresentationRole::Courier, EMiningPresentationState::Idle);
	UE_LOG(LogPangeaMiningPresentationActorSync, Log, TEXT("Spawned mining courier presentation actor. Site=%s Actor=%s Class=%s"), *GetNameSafe(&SiteActor), *GetNameSafe(SpawnedCourierActor), *GetNameSafe(LoadedClass));
}
