// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.


#include "ACFGameplayAbility.h"
#include <Abilities/Tasks/AbilityTask_WaitGameplayEvent.h>
#include "Kismet/KismetMathLibrary.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include <Abilities/Tasks/AbilityTask_PlayMontageAndWait.h>
#include <Abilities/Tasks/AbilityTask_WaitGameplayEvent.h>
#include <GameplayTagsManager.h>
#include "ARSStatisticsComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/ACFAbilitySystemComponent.h"
#include "Logging.h"
#include "Animation/AnimMontage.h"


UACFGameplayAbility::UACFGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	bFullyInit = false;
}

void UACFGameplayAbility::SetAnimMontage(UAnimMontage* newMontage)
{
	animMontage = newMontage;
}

bool UACFGameplayAbility::IsFullyInit() const
{
	return bFullyInit;
}


FName UACFGameplayAbility::GetMontageSectionName_Implementation()
{
	return NAME_None;
}

FTransform UACFGameplayAbility::GetWarpTransform_Implementation()
{
	ensure(false);
	return FTransform();
}

class USceneComponent* UACFGameplayAbility::GetWarpTargetComponent_Implementation()
{
	ensure(false);
	return nullptr;
}

void UACFGameplayAbility::OnNotablePointReached_Implementation()
{
}

void UACFGameplayAbility::OnGameplayEventReceived_Implementation(const FGameplayTag eventTag)
{

}

void UACFGameplayAbility::HandleMontageFinished()
{

}

void UACFGameplayAbility::HandleMontageInterrupted()
{

}

void UACFGameplayAbility::HandleGameplayEventReceived(FGameplayEventData Payload)
{
	if (Payload.EventTag.MatchesTagExact(UGameplayTagsManager::Get().RequestGameplayTag(ACF::NotableTag))) {
		OnNotablePointReached();
	}
	else {
		OnGameplayEventReceived(Payload.EventTag);
	}
}

void UACFGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);

	selfHandle = Handle;
	actorInfo = *ActorInfo;
	activationInfo = ActivationInfo;
	CharacterOwner = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	ExtractPayloadFromEvent(TriggerEventData);

	if (!IsFullyInit()) {
		InitAbility();
	}
}

void UACFGameplayAbility::ExtractPayloadFromEvent(const FGameplayEventData* TriggerEventData)
{
	StoredPayload = FACFAbilityPayload();

	if (!TriggerEventData) {
		return;
	}

	// INT
	StoredPayload.FloatPayload = TriggerEventData->EventMagnitude;
	StoredPayload.PayloadTag = TriggerEventData->EventTag;
	// VECTOR + HITRESULT
	const FGameplayEffectContextHandle& context = TriggerEventData->ContextHandle;
	if (context.IsValid()) {
		StoredPayload.VectorPayload = context.GetOrigin();
		const FHitResult* HR = context.GetHitResult();
		if (HR) {
			StoredPayload.HitResult = *HR;
		}
		// TARGET
		if (TriggerEventData->Target) {
			StoredPayload.TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
		}
	}



	StoredPayload.bIsValid = true;
}

void UACFGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	GetACFAbilityComponent()->OnAbilityStarted(this);

	UAbilityTask_WaitGameplayEvent* WaitExitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag(), nullptr, false, false);
	WaitExitEvent->EventReceived.AddDynamic(this, &UACFGameplayAbility::HandleGameplayEventReceived);
	WaitExitEvent->ReadyForActivation();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CommitAbilityCost(Handle, ActorInfo, ActivationInfo)) {
		if (StatisticComp) {
			executionEffect = StatisticComp->AddAttributeSetModifier(ActionConfig.AttributeModifier);
		}
		else {
			UE_LOG(ACFLog, Warning, TEXT("Invalid Attributes Setup!"));
			return;
		}
	}
	else {
		EndAbility(Handle, ActorInfo, activationInfo, true, true);
		return;
	}

	// select the sections, updates the speed
	PrepareMontageInfo();

	// initializes root motion and warping
	InitWarp();

	if (ActionConfig.bAutoExecute) {
		ExecuteAction();
	}
}




void UACFGameplayAbility::InitAbility()
{
	TriggeringTag = GetCurrentAbilitySpec()->GetDynamicSpecSourceTags().First();

	UACFGameplayAbility* Template = Cast<UACFGameplayAbility>(GetCurrentSourceObject());

	if (!Template || Template->GetClass() != GetClass()) {
		return;
	}

	for (TFieldIterator<FProperty> PropIt(Template->GetClass()); PropIt; ++PropIt) {
		FProperty* Property = *PropIt;

		if (Property && Property->HasAnyPropertyFlags(CPF_Edit)) {
			void* SourceValue = Property->ContainerPtrToValuePtr<void>(Template);
			void* DestValue = Property->ContainerPtrToValuePtr<void>(this);

			Property->CopyCompleteValue(DestValue, SourceValue);
		}
	}
	/*
  TO DO: BETTER TO KEEP ALL THIS DATA ALREADY WITHIN THE ABILITY AND NOT OUTSIDE!
   if (animMontage != Source.MontageAction) {
	   animMontage = Source.MontageAction;
   }*/

	bFullyInit = true;
}

float UACFGameplayAbility::GetPlayRate_Implementation() const
{
	return 1.f;
}

UAnimMontage* UACFGameplayAbility::GetMontage_Implementation() const
{
	return animMontage;
}


void UACFGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&ActivationInfo)) {
		if (bIsExecutingAction) {
			bIsExecutingAction = false;
		}
	}

	if (CharacterOwner) {
		UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();

		if (motionComp) {
			motionComp->RemoveWarpTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint);
		}
	}

	if (StatisticComp) {
		StatisticComp->RemoveAttributeSetModifier(executionEffect);
	}
	if (ActionConfig.bAutoStartCooldown) {
		CommitAbilityCooldown(selfHandle, ActorInfo, ActivationInfo, true);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (ACFAbilityComponent) {
		ACFAbilityComponent->OnAbilityEnded(this);
	}
}

void UACFGameplayAbility::PlayCurrentMontage()
{
	const float rootMotionScale = ActionConfig.MontageReproductionType == EMontageReproductionType::ERootMotionScaled ? ActionConfig.RootMotionScale : 1.f;

	// Using a gameplay Task to play the montage

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, MontageInfo.StartSectionName,
		MontageInfo.MontageAction, GetPlayRate(), MontageInfo.StartSectionName, true, rootMotionScale);
	if (Task) {
		Task->OnBlendOut.AddDynamic(this, &UACFGameplayAbility::HandleMontageFinished);
		Task->OnInterrupted.AddDynamic(this, &UACFGameplayAbility::HandleMontageInterrupted);
		Task->OnCancelled.AddDynamic(this, &UACFGameplayAbility::HandleMontageInterrupted);
		Task->OnCompleted.AddDynamic(this, &UACFGameplayAbility::HandleMontageFinished);
		Task->ReadyForActivation();
	}
}

void UACFGameplayAbility::ExecuteAction()
{
	if (animMontage && ACFAbilityComponent) {
		PlayCurrentMontage();
		bIsExecutingAction = true;
	}
	else {
		EndAbility(selfHandle, &actorInfo, activationInfo, true, true);

	}
}

void UACFGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	CharacterOwner = Cast<ACharacter>(ActorInfo->AvatarActor);
	ACFAbilityComponent = Cast<UACFAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	if (ActorInfo->AvatarActor->IsValidLowLevelFast()) {
		StatisticComp = ActorInfo->AvatarActor->FindComponentByClass<UARSStatisticsComponent>();
	}
	K2_OnPawnAvatarSet();
}

void UACFGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{

	if (StatisticComp && ActionConfig.CostGEType == EGEType::ESetByCallerFromConfig) {
		StatisticComp->ConsumeStatistics(ActionConfig.ActionCost);
	}
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

}

bool UACFGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	if (!ActorInfo->AvatarActor->IsValidLowLevelFast()) {
		return false;
	}

	if (!StatisticComp) {
		return false;
	}

	const TObjectPtr<UARSLevelingComponent> levelingComp = ActorInfo->AvatarActor->FindComponentByClass<UARSLevelingComponent>();
	if (!levelingComp || levelingComp->GetCurrentLevel() < ActionConfig.RequiredLevel) {
		return false;
	}

	if (!StatisticComp->CheckCosts(ActionConfig.ActionCost)) {
		return false;
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}



//WARp functions

void UACFGameplayAbility::GetWarpInfo_Implementation(FACFWarpReproductionInfo& outWarpInfo)
{
	outWarpInfo.WarpConfig = ActionConfig.WarpInfo;
	if (!animMontage) {
		return;
	}
	const FName sectionName = GetMontageSectionName();
	int32 currentIndex = animMontage->GetSectionIndex(sectionName);
	if (currentIndex < 0) {
		currentIndex = 0;
	}
	/* float endTime;
	 animMontage->GetSectionStartAndEndTime(currentIndex, outWarpInfo.WarpConfig.WarpStartTime, endTime);
	 outWarpInfo.WarpConfig.WarpEndTime = outWarpInfo.WarpConfig.WarpStartTime + ActionConfig.WarpInfo.WarpEndTime;*/
	if (ActionConfig.WarpInfo.TargetType == EWarpTargetType::ETargetTransform) {
		const FTransform endTransform = GetWarpTransform();
		FVector localScale = FVector(1.f);
		UKismetMathLibrary::BreakTransform(endTransform, outWarpInfo.WarpLocation, outWarpInfo.WarpRotation, localScale);
	}
	else if (ActionConfig.WarpInfo.TargetType == EWarpTargetType::ETargetComponent) {
		outWarpInfo.TargetComponent = GetWarpTargetComponent();
	}
}

void UACFGameplayAbility::PrepareMontageInfo()
{
	MontageInfo.MontageAction = GetMontage();
	MontageInfo.ReproductionSpeed = GetPlayRate();
	if (ActionConfig.bPlayRandomMontageSection) {
		const int32 numSections = MontageInfo.MontageAction->CompositeSections.Num();

		const int32 sectionToPlay = FMath::RandHelper(numSections);

		MontageInfo.StartSectionName = animMontage->GetSectionName(sectionToPlay);
	}
	else {
		MontageInfo.StartSectionName = GetMontageSectionName();
	}
	MontageInfo.ReproductionType = ActionConfig.MontageReproductionType;
	MontageInfo.RootMotionScale = 1.f;
}

void UACFGameplayAbility::InitWarp()
{
	const UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
	switch (MontageInfo.ReproductionType) {
	case EMontageReproductionType::ERootMotionScaled:
		MontageInfo.RootMotionScale = ActionConfig.RootMotionScale;
		break;
	case EMontageReproductionType::ERootMotion:
		break;
	case EMontageReproductionType::EMotionWarped:
		if (motionComp) {
			FACFWarpReproductionInfo WarpInfo;
			GetWarpInfo(WarpInfo);
			MontageInfo.WarpInfo = WarpInfo;
			UpdateWarp();
		}
		break;
	}
}

void UACFGameplayAbility::UpdateWarp()
{
	UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
	const FTransform targetTransform = FTransform(MontageInfo.WarpInfo.WarpRotation, MontageInfo.WarpInfo.WarpLocation);

	if (motionComp) {

		FMotionWarpingTarget targetPoint;
		if (MontageInfo.WarpInfo.WarpConfig.TargetType == EWarpTargetType::ETargetComponent && MontageInfo.WarpInfo.TargetComponent) {
			targetPoint.Name = MontageInfo.WarpInfo.WarpConfig.SyncPoint;
			targetPoint.Component = MontageInfo.WarpInfo.TargetComponent;
			targetPoint.bFollowComponent = true;
		}
		else {
			targetPoint = FMotionWarpingTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint, targetTransform);
		}
		motionComp->AddOrUpdateWarpTarget(targetPoint);

		/* USE THE NOTIFY INSTEAD
		if (MontageInfo.WarpInfo.WarpConfig.bAutoWarp) {
			URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(motionComp, MontageInfo.MontageAction, MontageInfo.WarpInfo.WarpConfig.WarpStartTime,
				MontageInfo.WarpInfo.WarpConfig.WarpEndTime, MontageInfo.WarpInfo.WarpConfig.SyncPoint, EWarpPointAnimProvider::None, targetTransform, NAME_None, true, true, true,
				MontageInfo.WarpInfo.WarpConfig.RotationType, EMotionWarpRotationMethod::Slerp, MontageInfo.WarpInfo.WarpConfig.WarpRotationTime);
		}*/

		if (ACFAbilityComponent->bPrintDebugInfo) {
			UKismetSystemLibrary::DrawDebugSphere(this, MontageInfo.WarpInfo.WarpLocation, 100.f, 12, FLinearColor::Red, 5.f);
			const FVector Start = MontageInfo.WarpInfo.WarpLocation;
			const FVector Direction = MontageInfo.WarpInfo.WarpRotation.Vector(); // Convert ROTATION TO DIRECTION
			const FVector End = Start + Direction * 200.f;

			UKismetSystemLibrary::DrawDebugArrow(this, Start, End, 200, FLinearColor::Red, 10.f);
		}
	}
	else {
		motionComp->RemoveWarpTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint);
	}
}
