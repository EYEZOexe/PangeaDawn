#include "Components/MiningPresentationAgentComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UMiningPresentationAgentComponent::UMiningPresentationAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	WorkerWorkLoopAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/FullSample/Animations/Quaternius/AL_Anim_Rig_PickUp_Table.AL_Anim_Rig_PickUp_Table")));
	GuardHoldLoopAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/FullSample/Animations/Locomotion/Unarmed/MM_Unarmed_Idle_Ready.MM_Unarmed_Idle_Ready")));
	CourierInteractionLoopAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/FullSample/Animations/Quaternius/AL_Anim_Rig_PickUp_Table.AL_Anim_Rig_PickUp_Table")));
}

void UMiningPresentationAgentComponent::SetTravelSpeedOverride(const EMiningPresentationRole Role, const float TravelSpeed)
{
	if (TravelSpeed <= 0.0f)
	{
		return;
	}

	switch (Role)
	{
	case EMiningPresentationRole::Worker:
		WorkerTravelSpeed = TravelSpeed;
		break;
	case EMiningPresentationRole::Guard:
		GuardTravelSpeed = TravelSpeed;
		break;
	case EMiningPresentationRole::Courier:
		CourierTravelSpeed = TravelSpeed;
		break;
	default:
		break;
	}

	ApplyMovementForState();
}

void UMiningPresentationAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerReferences();
	ApplyMovementForState();
	ApplyAnimationForState();
}

void UMiningPresentationAgentComponent::SetMiningPresentationRole_Implementation(EMiningPresentationRole Role)
{
	if (CurrentRole == Role)
	{
		return;
	}

	CurrentRole = Role;
	ApplyMovementForState();
	ApplyAnimationForState();
}

void UMiningPresentationAgentComponent::SetMiningPresentationState_Implementation(EMiningPresentationState State)
{
	if (CurrentState == State)
	{
		return;
	}

	CurrentState = State;
	ApplyMovementForState();
	ApplyAnimationForState();
}

void UMiningPresentationAgentComponent::SetMiningPresentationFocus_Implementation(const FVector& WorldLocation)
{
	if (bHasFocus && CurrentFocusLocation.Equals(WorldLocation, 1.0f))
	{
		return;
	}

	CurrentFocusLocation = WorldLocation;
	bHasFocus = true;
}

void UMiningPresentationAgentComponent::CacheOwnerReferences()
{
	if (CachedCharacterOwner.IsValid() && CachedMovementComponent.IsValid())
	{
		return;
	}

	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	CachedCharacterOwner = CharacterOwner;
	CachedMovementComponent = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	CachedAnimInstance = (CharacterOwner && CharacterOwner->GetMesh()) ? CharacterOwner->GetMesh()->GetAnimInstance() : nullptr;
	if (CachedMovementComponent.IsValid() && DefaultMaxWalkSpeed <= 0.0f)
	{
		DefaultMaxWalkSpeed = CachedMovementComponent->MaxWalkSpeed;
	}
}

void UMiningPresentationAgentComponent::ApplyMovementForState()
{
	CacheOwnerReferences();
	if (!CachedMovementComponent.IsValid())
	{
		return;
	}

	if (CurrentState == EMiningPresentationState::Traveling)
	{
		CachedMovementComponent->MaxWalkSpeed = GetTravelSpeedForRole();
		return;
	}

	if (DefaultMaxWalkSpeed > 0.0f)
	{
		CachedMovementComponent->MaxWalkSpeed = DefaultMaxWalkSpeed;
	}
}

void UMiningPresentationAgentComponent::ApplyAnimationForState()
{
	CacheOwnerReferences();
	if (!CachedAnimInstance.IsValid())
	{
		return;
	}

	if (CurrentRole == EMiningPresentationRole::Worker && CurrentState == EMiningPresentationState::Working)
	{
		if (PlayLoopingPresentationAnimation(WorkerWorkLoopAnimation, WorkerWorkLoopPlayRate, WorkerWorkLoopCount))
		{
			return;
		}
	}

	if (CurrentRole == EMiningPresentationRole::Guard && CurrentState == EMiningPresentationState::Guarding)
	{
		if (PlayLoopingPresentationAnimation(GuardHoldLoopAnimation, GuardHoldLoopPlayRate, GuardHoldLoopCount))
		{
			return;
		}
	}

	if (CurrentRole == EMiningPresentationRole::Courier &&
		(CurrentState == EMiningPresentationState::Loading || CurrentState == EMiningPresentationState::Unloading))
	{
		if (PlayLoopingPresentationAnimation(CourierInteractionLoopAnimation, CourierInteractionLoopPlayRate, CourierInteractionLoopCount))
		{
			return;
		}
	}

	StopPresentationAnimation();
}

bool UMiningPresentationAgentComponent::PlayLoopingPresentationAnimation(TSoftObjectPtr<UAnimSequenceBase>& AnimationAsset, float PlayRate, int32 LoopCount)
{
	if (!CachedAnimInstance.IsValid())
	{
		return false;
	}

	UAnimSequenceBase* Animation = AnimationAsset.LoadSynchronous();
	if (!Animation)
	{
		return false;
	}

	if (ActivePresentationMontage && CachedAnimInstance->Montage_IsPlaying(ActivePresentationMontage))
	{
		return true;
	}

	if (!bHasSavedRootMotionMode)
	{
		SavedRootMotionMode = CachedAnimInstance->RootMotionMode;
		bHasSavedRootMotionMode = true;
	}

	CachedAnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	ActivePresentationMontage = CachedAnimInstance->PlaySlotAnimationAsDynamicMontage(
		Animation,
		PresentationMontageSlotName,
		PresentationMontageBlendInTime,
		PresentationMontageBlendOutTime,
		PlayRate,
		LoopCount,
		-1.0f,
		0.0f);
	return ActivePresentationMontage != nullptr;
}

void UMiningPresentationAgentComponent::StopPresentationAnimation()
{
	if (!CachedAnimInstance.IsValid())
	{
		return;
	}

	if (ActivePresentationMontage)
	{
		CachedAnimInstance->Montage_Stop(PresentationMontageBlendOutTime, ActivePresentationMontage);
		ActivePresentationMontage = nullptr;
	}

	if (bHasSavedRootMotionMode)
	{
		CachedAnimInstance->SetRootMotionMode(SavedRootMotionMode);
		bHasSavedRootMotionMode = false;
	}
}

float UMiningPresentationAgentComponent::GetTravelSpeedForRole() const
{
	switch (CurrentRole)
	{
	case EMiningPresentationRole::Worker:
		return WorkerTravelSpeed;
	case EMiningPresentationRole::Guard:
		return GuardTravelSpeed;
	case EMiningPresentationRole::Courier:
		return CourierTravelSpeed;
	default:
		return DefaultMaxWalkSpeed > 0.0f ? DefaultMaxWalkSpeed : WorkerTravelSpeed;
	}
}
