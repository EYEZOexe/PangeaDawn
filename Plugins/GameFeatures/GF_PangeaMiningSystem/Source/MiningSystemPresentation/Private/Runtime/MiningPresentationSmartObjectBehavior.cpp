#include "Components/MiningSitePresentationCoordinatorComponent.h"

#include "Actors/MiningSiteActor.h"
#include "AI/AITask_UseGameplayInteraction.h"
#include "AIController.h"
#include "Components/MiningSiteComponent.h"
#include "SmartObjectComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayInteractionSmartObjectBehaviorDefinition.h"
#include "SmartObjectBlueprintFunctionLibrary.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogPangeaMiningPresentationRuntime, Log, All);

bool UMiningSitePresentationCoordinatorComponent::HasSmartObjectInteractionTimedOut(AActor* Actor, float MaxDuration) const
{
	if (!Actor || MaxDuration <= 0.0f)
	{
		return false;
	}

	const TObjectPtr<UAITask_UseGameplayInteraction>* ExistingTask = PresentationSmartObjectTaskMap.Find(Actor);
	if (!ExistingTask || !ExistingTask->Get() || ExistingTask->Get()->IsFinished())
	{
		return false;
	}

	const float* StartTime = PresentationSmartObjectStartTimeMap.Find(Actor);
	if (!StartTime || !GetWorld())
	{
		return false;
	}

	return (GetWorld()->GetTimeSeconds() - *StartTime) >= MaxDuration;
}

bool UMiningSitePresentationCoordinatorComponent::TryStartSmartObjectBehavior(AMiningSiteActor& SiteActor, AActor* Actor, USmartObjectComponent* SmartObjectComponent, EMiningPresentationRole Role, int32 PreferredSlotIndex)
{
	const auto SetSmartObjectStatus = [this, &SiteActor, Actor](const TCHAR* Status)
	{
		if (!Actor)
		{
			return;
		}

		const FString NewStatus(Status);
		if (const FString* ExistingStatus = PresentationSmartObjectLastStatusMap.Find(Actor))
		{
			if (*ExistingStatus == NewStatus)
			{
				return;
			}
		}

		PresentationSmartObjectLastStatusMap.Add(Actor, NewStatus);
		UE_LOG(LogPangeaMiningPresentationRuntime, Log, TEXT("Mining site GI status. Site=%s Actor=%s Status=%s"), *GetNameSafe(&SiteActor), *GetNameSafe(Actor), *NewStatus);
	};

	if (!Actor || !SmartObjectComponent || !SmartObjectComponent->IsSmartObjectEnabled())
	{
		SetSmartObjectStatus(TEXT("SmartObject disabled"));
		return false;
	}

	if (TObjectPtr<UAITask_UseGameplayInteraction>* ExistingTask = PresentationSmartObjectTaskMap.Find(Actor))
	{
		if (ExistingTask->Get() && !ExistingTask->Get()->IsFinished())
		{
			SetSmartObjectStatus(TEXT("GameplayInteraction active"));
			return true;
		}

		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float StartedAt = PresentationSmartObjectStartTimeMap.FindRef(Actor);
		const float HoldUntil = FMath::Max(Now, StartedAt + GetInteractionDurationForActor(SiteActor.MiningSiteComponent, Role, Actor));
		PresentationSmartObjectTaskMap.Remove(Actor);
		PresentationClaimHandleMap.Remove(Actor);
		PresentationSmartObjectStartTimeMap.Remove(Actor);
		if (HoldUntil > Now)
		{
			PresentationHoldUntilTimeMap.Add(Actor, HoldUntil);
			SetSmartObjectStatus(TEXT("GameplayInteraction presentation hold"));
			return true;
		}

		PresentationHoldUntilTimeMap.Remove(Actor);
		PresentationRoutePhaseMap.Add(Actor, !PresentationRoutePhaseMap.FindRef(Actor));
		SetSmartObjectStatus(TEXT("GameplayInteraction finished"));
		return false;
	}

	if (HasPresentationHoldActive(Actor))
	{
		SetSmartObjectStatus(TEXT("GameplayInteraction presentation hold"));
		return true;
	}

	PresentationHoldUntilTimeMap.Remove(Actor);

	APawn* Pawn = Cast<APawn>(Actor);
	AAIController* AIController = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	if (!Pawn || !AIController)
	{
		SetSmartObjectStatus(TEXT("Missing pawn or AI controller"));
		return false;
	}

	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
	if (!SmartObjectSubsystem)
	{
		SetSmartObjectStatus(TEXT("Missing SmartObject subsystem"));
		return false;
	}

	const FSmartObjectHandle RegisteredHandle = SmartObjectComponent->GetRegisteredHandle();
	if (!RegisteredHandle.IsValid())
	{
		SetSmartObjectStatus(TEXT("SmartObject handle invalid"));
		return false;
	}

	TArray<FSmartObjectSlotHandle> AllSlots;
	SmartObjectSubsystem->GetAllSlots(RegisteredHandle, AllSlots);

	TArray<FSmartObjectSlotHandle> RequestSlots;
	FSmartObjectRequestFilter Filter;
	Filter.BehaviorDefinitionClasses.Add(UGameplayInteractionSmartObjectBehaviorDefinition::StaticClass());
	SmartObjectSubsystem->FindSlots(RegisteredHandle, Filter, RequestSlots);
	const bool bUseSecondarySlot = PresentationRoutePhaseMap.FindRef(Actor);

	auto TryClaimFromSlots = [this, Pawn, bUseSecondarySlot, PreferredSlotIndex](const TArray<FSmartObjectSlotHandle>& CandidateSlots, int32& OutChosenIndex) -> FSmartObjectClaimHandle
	{
		if (CandidateSlots.IsEmpty())
		{
			return FSmartObjectClaimHandle();
		}

		TArray<int32> SlotOrder;
		SlotOrder.Reserve(CandidateSlots.Num());
		const int32 PreferredIndex = PreferredSlotIndex != INDEX_NONE
			? FMath::Clamp(PreferredSlotIndex, 0, CandidateSlots.Num() - 1)
			: ((CandidateSlots.Num() > 1 && bUseSecondarySlot) ? 1 : 0);
		SlotOrder.Add(PreferredIndex);
		for (int32 Index = 0; Index < CandidateSlots.Num(); ++Index)
		{
			if (Index != PreferredIndex)
			{
				SlotOrder.Add(Index);
			}
		}

		for (const int32 CandidateIndex : SlotOrder)
		{
			const FSmartObjectClaimHandle CandidateClaimHandle = USmartObjectBlueprintFunctionLibrary::MarkSmartObjectSlotAsClaimed(this, CandidateSlots[CandidateIndex], Pawn, ESmartObjectClaimPriority::Normal);
			if (CandidateClaimHandle.IsValid())
			{
				OutChosenIndex = CandidateIndex;
				return CandidateClaimHandle;
			}
		}

		return FSmartObjectClaimHandle();
	};

	int32 ChosenSlotIndex = INDEX_NONE;
	const FSmartObjectClaimHandle ClaimHandle = TryClaimFromSlots(RequestSlots, ChosenSlotIndex);

	if (!ClaimHandle.IsValid())
	{
		FSmartObjectRequestFilter NoConditionFilter = Filter;
		NoConditionFilter.bShouldEvaluateConditions = false;
		TArray<FSmartObjectSlotHandle> NoConditionSlots;
		SmartObjectSubsystem->FindSlots(RegisteredHandle, NoConditionFilter, NoConditionSlots);
		if (!NoConditionSlots.IsEmpty())
		{
			SetSmartObjectStatus(*FString::Printf(TEXT("No SmartObject request results (conditions failed, total_slots=%d)"), AllSlots.Num()));
			return false;
		}

		FSmartObjectRequestFilter NoBehaviorClassFilter = NoConditionFilter;
		NoBehaviorClassFilter.BehaviorDefinitionClasses.Reset();
		TArray<FSmartObjectSlotHandle> NoBehaviorClassSlots;
		SmartObjectSubsystem->FindSlots(RegisteredHandle, NoBehaviorClassFilter, NoBehaviorClassSlots);
		if (!NoBehaviorClassSlots.IsEmpty())
		{
			SetSmartObjectStatus(*FString::Printf(TEXT("No SmartObject request results (behavior class mismatch, total_slots=%d)"), AllSlots.Num()));
			return false;
		}

		SetSmartObjectStatus(*FString::Printf(TEXT("No SmartObject request results (total_slots=%d)"), AllSlots.Num()));
		return false;
	}

	UAITask_UseGameplayInteraction* SmartObjectTask = UAITask_UseGameplayInteraction::MoveToAndUseSmartObjectWithGameplayInteraction(AIController, ClaimHandle, true);
	if (!SmartObjectTask)
	{
		USmartObjectBlueprintFunctionLibrary::MarkSmartObjectSlotAsFree(this, ClaimHandle);
		SetSmartObjectStatus(TEXT("GameplayInteraction task creation failed"));
		return false;
	}

	SmartObjectTask->ReadyForActivation();
	PresentationClaimHandleMap.Add(Actor, ClaimHandle);
	PresentationSmartObjectTaskMap.Add(Actor, SmartObjectTask);
	PresentationSmartObjectStartTimeMap.Add(Actor, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
	PresentationMoveTargetMap.Remove(Actor);
	SetSmartObjectStatus(*FString::Printf(TEXT("GameplayInteraction started slot=%d results=%d total_slots=%d"), ChosenSlotIndex, RequestSlots.Num(), AllSlots.Num()));
	return true;
}

void UMiningSitePresentationCoordinatorComponent::ReleaseSmartObjectForActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	bool bTaskCanceled = false;
	if (TObjectPtr<UAITask_UseGameplayInteraction>* ExistingTask = PresentationSmartObjectTaskMap.Find(Actor))
	{
		if (ExistingTask->Get() && !ExistingTask->Get()->IsFinished())
		{
			ExistingTask->Get()->ExternalCancel();
			bTaskCanceled = true;
		}
	}

	if (!bTaskCanceled)
	{
		if (const FSmartObjectClaimHandle* ExistingClaim = PresentationClaimHandleMap.Find(Actor))
		{
			if (ExistingClaim->IsValid())
			{
				USmartObjectBlueprintFunctionLibrary::MarkSmartObjectSlotAsFree(this, *ExistingClaim);
			}
		}
	}

	PresentationSmartObjectTaskMap.Remove(Actor);
	PresentationClaimHandleMap.Remove(Actor);
	PresentationSmartObjectLastStatusMap.Remove(Actor);
	PresentationSmartObjectStartTimeMap.Remove(Actor);
	PresentationHoldUntilTimeMap.Remove(Actor);
}
