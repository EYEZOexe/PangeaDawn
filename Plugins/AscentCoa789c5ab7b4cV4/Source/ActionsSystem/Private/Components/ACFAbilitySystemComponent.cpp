// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFAbilitySystemComponent.h"
#include "ACFActionTypes.h"
#include "ACFActionsFunctionLibrary.h"
#include "ARSStatisticsComponent.h"
#include "ARSTypes.h"
#include "Actions/ACFActionAbility.h"
#include "Actions/ACFSustainedAction.h"
#include "Components/SkeletalMeshComponent.h"
#include "MotionWarpingComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include <Abilities/GameplayAbility.h>
#include <Engine/Engine.h>
#include <Engine/World.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameplayAbilitySpecHandle.h>
#include <GameplayTagsManager.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Logging.h>
#include <TimerManager.h>
#include "ACFGameplayAbility.h"

// Sets default values for this component's properties
UACFAbilitySystemComponent::UACFAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);

	// ActionsSet = UACFActionsSet::StaticClass();
	CurrentPriority = -1;
	StoredAction = FGameplayTag();
	CurrentAbilityTag = FGameplayTag();
}

// Called when the game starts
void UACFAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<ACharacter>(GetOwner());

	if (bAutoInit) {
		GrantInitialAbilities();
	}

	if (CharacterOwner) {
		animInst = CharacterOwner->GetMesh()->GetAnimInstance();
		StatisticComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
		if (!StatisticComp) {
			UE_LOG(ACFLog, Warning, TEXT("No Statistiscs Component - ActionsManager"));
		}
	}
	else {
		UE_LOG(ACFLog, Warning, TEXT("Invalid Character - ActionsManager"));
	}
}

void UACFAbilitySystemComponent::GrantInitialAbilities()
{
	if (GetOwner()->HasAuthority()) {

		if (DefaultAbilitySet) {
			GrantAbilitySet(DefaultAbilitySet, FGameplayTag());
		}

		for (const auto& moveset : MovesetAbilities) {
			if (moveset.Value) {
				GrantAbilitySet(moveset.Value, moveset.Key);
			}
		}
	}
}

void UACFAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACFAbilitySystemComponent, CurrentAbilityTag);
	DOREPLIFETIME(UACFAbilitySystemComponent, CurrentPriority);
	DOREPLIFETIME(UACFAbilitySystemComponent, bIsPerformingAction);
	DOREPLIFETIME(UACFAbilitySystemComponent, currentMovesetActionsTag);
	DOREPLIFETIME(UACFAbilitySystemComponent, ComboCounters);
	DOREPLIFETIME(UACFAbilitySystemComponent, StoredAction);
}

void UACFAbilitySystemComponent::SetMovesetActions_Implementation(const FGameplayTag& movesetActionsTag)
{
	currentMovesetActionsTag = movesetActionsTag;
}

void UACFAbilitySystemComponent::LockActionsTrigger()
{
	bIsLocked = true;
}

bool UACFAbilitySystemComponent::TriggerAction(FGameplayTag abilityTag,
	EActionPriority Priority /*= EActionPriority::ELow*/, bool bCanBeStored /*= false*/)
{
	return Internal_TriggerAction(abilityTag, Priority, bCanBeStored, nullptr);
}


bool UACFAbilitySystemComponent::TriggerActionWithPayload(FGameplayTag AbilityTag, const FACFAbilityPayload& Payload,
	EActionPriority Priority /*= EActionPriority::ELow*/, bool bCanBeStored /*= false*/)
{
	FGameplayEventData EventData;
	EventData.Instigator = CharacterOwner;
	EventData.EventTag = Payload.PayloadTag;

	// INT
	EventData.EventMagnitude = Payload.FloatPayload;

	// Context, VECTOR & HITRESULT
	FGameplayEffectContextHandle Ctx = MakeEffectContext();
	Ctx.AddInstigator(CharacterOwner, CharacterOwner->GetController());
	Ctx.AddOrigin(Payload.VectorPayload);

	FHitResult HR = Payload.HitResult;
	Ctx.AddHitResult(HR, true);
	EventData.ContextHandle = Ctx;

	// TARGET
	EventData.Target = Payload.TargetActor;

	return Internal_TriggerAction(AbilityTag, Priority, bCanBeStored, &EventData);
}



bool UACFAbilitySystemComponent::Internal_TriggerAction(
	FGameplayTag abilityTag,
	EActionPriority Priority,
	bool bCanBeStored,
	const FGameplayEventData* OptionalEventData)
{
	if (abilityTag == FGameplayTag()) {
		UE_LOG(ACFLog, Warning, TEXT("Invalid Action Ability Tag - UACFAbilitySystemComponent::TriggerAction"));
		return false;
	}

	if (bIsLocked) {
		return false;
	}

	const FGameplayAbilitySpecHandle handle = FromAbilityTagToHandle(abilityTag);

	if ((CanExecuteAbilityByHandle(handle) && ((int32)Priority > CurrentPriority)) || Priority == EActionPriority::EHighest) {
		// Use LaunchAbilityWithPayload wrapper (handles both cases)
		return LaunchAbilityWithPayload(handle, Priority, OptionalEventData);
	}
	else if (CurrentAbilityTag != FGameplayTag() && bCanStoreAction && bCanBeStored && GetStoredAction() == FGameplayTag()) {
		StoreAbilityInBuffer(abilityTag);
		return false;
	}
	else if (!GetAbilityByTag(abilityTag) && ((int32)Priority > CurrentPriority || Priority == EActionPriority::EHighest)) {
		FGameplayTagContainer tagCont;
		tagCont.AddTag(abilityTag);
		return TryActivateAbilitiesByTag(tagCont);
	}
	return false;
}

bool UACFAbilitySystemComponent::LaunchAbilityWithPayload(
	const FGameplayAbilitySpecHandle& handle,
	const EActionPriority priority,
	const FGameplayEventData* OptionalEventData)
{
	ExitCurrentAction();
	SetPendingPriority((int32)priority);

	if (OptionalEventData)
	{
		// Use TriggerAbilityFromGameplayEvent for event-based activation with payload
		return TriggerAbilityFromGameplayEvent(
			handle,
			AbilityActorInfo.Get(),
			OptionalEventData->EventTag,
			OptionalEventData,
			*this);
	}
	else
	{
		// Standard activation without payload
		return TryActivateAbility(handle);
	}
}

bool UACFAbilitySystemComponent::LaunchAbility(const FGameplayAbilitySpecHandle& handle, const EActionPriority priority)
{
	ExitCurrentAction();
	SetPendingPriority((int32)priority);

	return TryActivateAbility(handle);
	//return TriggerAbilityFromGameplayEvent(handle, AbilityActorInfo.Get(), EventData.EventTag, &EventData, *this);
}



bool UACFAbilitySystemComponent::TriggerAbility(FGameplayTag abilityTag)
{
	const FGameplayAbilitySpecHandle handle = FromAbilityTagToHandle(abilityTag);
	if (handle.IsValid()) {
		return TryActivateAbility(handle);
	}
	return false;
}

void UACFAbilitySystemComponent::StoreAbilityInBuffer_Implementation(const FGameplayTag& ActionState)
{
	StoredAction = ActionState;
	TriggerGameplayEvent(ActionState);
}

void UACFAbilitySystemComponent::GrantAbilitySet(UACFAbilitySet* abilitySet, FGameplayTag dynamicTag)
{
	if (abilitySet) {
		for (const auto& ability : abilitySet->Abilities) {
			GrantAbility(ability, dynamicTag);
		}
		for (const auto& ability : abilitySet->ActionAbilities) {
			GrantACFAbility(ability, dynamicTag);
		}

		/*
		if (!MovesetAbilities.Contains(dynamicTag)) {
			MovesetAbilities.Add(dynamicTag, abilitySet);
		}*/
	}
	else {
		UE_LOG(ACFLog, Error, TEXT("Invalid Ability Set - UACFAbilitySystemComponent::GrantAbilitySet"));
	}
}

void UACFAbilitySystemComponent::RemoveAbilitySetByTag(const FGameplayTag& movesetTag)
{
	if (MovesetAbilities.Contains(movesetTag)) {
		const UACFAbilitySet* abilitySet = MovesetAbilities.FindChecked(movesetTag);
		RemoveAbilitySet(abilitySet, movesetTag);
	}
}

void UACFAbilitySystemComponent::RemoveAbilitySet(const UACFAbilitySet* abilitySet, FGameplayTag movesetTag)
{
	if (abilitySet) {
		for (const auto& ability : abilitySet->Abilities) {
			const FGameplayAbilitySpecHandle handle = GetAbilityHandle(ability.TriggeringTag, movesetTag);
			if (handle.IsValid()) {
				ClearAbility(handle);
			}
		}
		for (const auto& action : abilitySet->ActionAbilities) {
			const FGameplayAbilitySpecHandle handle = GetAbilityHandle(action.TriggeringTag, movesetTag);
			if (handle.IsValid()) {
				ClearAbility(handle);
			}
		}
	}
}

FGameplayAbilitySpecHandle UACFAbilitySystemComponent::GrantAbility(const FAbilityConfig& ability, FGameplayTag dynamicTag)
{
	if (!ability.GameplayAbility) {
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec spec(ability.GameplayAbility, ability.AbilityLevel, INDEX_NONE, CharacterOwner);
	spec.GetDynamicSpecSourceTags().AddTag(ability.TriggeringTag);
	if (dynamicTag != FGameplayTag::EmptyTag) {
		spec.GetDynamicSpecSourceTags().AddTag(dynamicTag);
	}
	return GiveAbility(spec);
}

FGameplayAbilitySpecHandle UACFAbilitySystemComponent::GrantACFAbility(const FActionAbilityConfig& ability, FGameplayTag dynamicTag)
{
	if (!ability.Action) {
		return FGameplayAbilitySpecHandle();
	}

	// DO: check for duplicates
	/*
	const FGameplayAbilitySpecHandle handle = GetAbilityHandle(ability.TriggeringTag, dynamicTag);
	if (handle.IsValid()) {
		UE_LOG(ACFLog, Warning, TEXT("Duplicate Ability Tags!!!- UACFAbilitySystemComponent::GrantActionAbility"));
		return FGameplayAbilitySpecHandle();
	}*/

	FGameplayAbilitySpec spec(ability.Action->GetClass(), ability.AbilityLevel, INDEX_NONE, ability.Action);
	spec.GetDynamicSpecSourceTags().AddTag(ability.TriggeringTag);
	if (dynamicTag != FGameplayTag::EmptyTag) {
		spec.GetDynamicSpecSourceTags().AddTag(dynamicTag);
	}
	FGameplayAbilitySpecHandle abilityHandle = GiveAbility(spec);
	TObjectPtr<UACFGameplayAbility> actionAbility = Cast<UACFGameplayAbility>(GetAbilityInstance(abilityHandle));
	if (actionAbility) {
		actionAbility->InitAbility();
	}
	return abilityHandle;
}

void UACFAbilitySystemComponent::TriggerGameplayEvent(const FGameplayTag& gameplayEvent)
{
	FGameplayEventData EventData;
	EventData.Instigator = CharacterOwner;
	HandleGameplayEvent(gameplayEvent, &EventData);
}







void UACFAbilitySystemComponent::OnAbilityStarted(TObjectPtr<UACFGameplayAbility> ability)
{
	if (PerformingAction) {
		LastActivatedAbility = PerformingAction;
	}

	PerformingAction = Cast<UACFActionAbility>(ability);
	CurrentAbilityTag = ability->GetActionTag();
	bIsPerformingAction = true;
	if (CharacterOwner && CharacterOwner->HasAuthority()) {
		SetCurrentPriority(PendingPriority);
	}
	OnAbilityStartedEvent.Broadcast(CurrentAbilityTag);
	PrintStateDebugInfo(true);
}

void UACFAbilitySystemComponent::OnAbilityEnded(TObjectPtr<UACFGameplayAbility> ability)
{
	PrintStateDebugInfo(false);

	OnAbilityFinishedEvent.Broadcast(CurrentAbilityTag);
	CurrentAbilityTag = FGameplayTag();

	bIsPerformingAction = false;
	LastActivatedAbility = ability;
	PerformingAction = nullptr;
	EvaluateBuffer();
}

void UACFAbilitySystemComponent::EvaluateBuffer()
{
	if (CharacterOwner->HasAuthority()) {
		SetCurrentPriority((-1));
		if (GetStoredAction() != FGameplayTag()) {
			TriggerAction(StoredAction, EActionPriority::EMedium, false);
			StoredAction = FGameplayTag();
		}
	}
}

bool UACFAbilitySystemComponent::CanExecuteAbility(FGameplayTag ActionState) const
{
	const FGameplayAbilitySpecHandle handle = FromAbilityTagToHandle(ActionState);
	return CanExecuteAbilityByHandle(handle);
}

bool UACFAbilitySystemComponent::CanExecuteAbilityByHandle(const FGameplayAbilitySpecHandle& handle) const
{
	if (handle.IsValid()) {

		if (bIsLocked) {
			return false;
		}

		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(handle);
		if (!Spec) {
			UE_LOG(ACFLog, Error, TEXT("Invalid Spec Handle!!!!!"));
			return false;
		}
		FGameplayTagContainer FailureTags;
		const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
		TObjectPtr<UGameplayAbility> baseAbility = GetAbilityInstance(handle);

		if (baseAbility) {
			UACFGameplayAbility* actionAbility = Cast<UACFGameplayAbility>(baseAbility);
			if (actionAbility) {
				if (!actionAbility->IsFullyInit()) {
					actionAbility->InitAbility();
				}

				return baseAbility->CanActivateAbility(handle, ActorInfo, nullptr, nullptr, &FailureTags);
			}
			// for non action ability
			return true;
		}

	}
	else {
		UE_LOG(ACFLog, Warning, TEXT("Actions Conditions are not verified"));
	}
	return false;
}

void UACFAbilitySystemComponent::ReleaseSustainedAction(FGameplayTag actionTag)
{
	if (PerformingAction && PerformingAction->GetActionTag() == actionTag) {
		UACFSustainedAction* sustAction = Cast<UACFSustainedAction>(PerformingAction);
		if (sustAction) {
			sustAction->ReleaseAction();
		}
	}
}

void UACFAbilitySystemComponent::SetCurrentPriority(const int32 newPriority)
{
	CurrentPriority = newPriority;
}

void UACFAbilitySystemComponent::SetPendingPriority(const int32 newPriority)
{
	PendingPriority = newPriority;
}

FGameplayAbilitySpecHandle UACFAbilitySystemComponent::FromAbilityTagToHandle(const FGameplayTag& actionTag) const
{
	const FGameplayTag abilitySetTag = currentMovesetActionsTag;

	return GetAbilityHandle(actionTag, abilitySetTag);
}

FGameplayAbilitySpecHandle UACFAbilitySystemComponent::GetAbilityHandle(const FGameplayTag& actionTag, const FGameplayTag abilitySetTag) const
{
	TArray<FGameplayAbilitySpecHandle> foundAbilities;

	if (actionTag.IsValid()) {
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items) {

			// first check the abilitytag
			if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(actionTag)) {
				continue;
			}

			// remove hard code here, this is just for testing purposes
			const FGameplayTag movesetRoot = UGameplayTagsManager::Get().RequestGameplayTag(FName("Moveset"));

			if (abilitySetTag != FGameplayTag()) {
				if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(abilitySetTag)) {
					return AbilitySpec.Handle;
					// those are the defualt abilities
				}
				else if (AbilitySpec.GetDynamicSpecSourceTags().Num() < 2) {
					foundAbilities.AddUnique(AbilitySpec.Handle);
				}
			}
			else if (!AbilitySpec.GetDynamicSpecSourceTags().HasTag(movesetRoot)) {
				return AbilitySpec.Handle;
			}
		}
	}

	if (foundAbilities.IsValidIndex(0)) {
		return foundAbilities[0];
	}

	return FGameplayAbilitySpecHandle();
}

FGameplayTag UACFAbilitySystemComponent::GetCurrentActionTag() const
{
	return CurrentAbilityTag;
}

void UACFAbilitySystemComponent::ExitCurrentAction()
{
	if (PerformingAction) {
		PerformingAction->ExitAction(true);
	}
}

UGameplayAbility* UACFAbilitySystemComponent::GetAbilityByTag(const FGameplayTag& abilityTag) const
{

	const FGameplayAbilitySpecHandle specHandle = FromAbilityTagToHandle(abilityTag);
	if (specHandle.IsValid()) {
		return GetAbilityInstance(specHandle);
	}
	return nullptr;
}

bool UACFAbilitySystemComponent::GetAbilityFromAbilitySet(const FGameplayTagContainer& abilityTagContainer, FActionAbilityConfig& outAction) const
{
	if (abilityTagContainer.Num() == 0) {
		UE_LOG(ACFLog, Warning, TEXT("Invalid Ability Tag Container - UACFAbilitySystemComponent::GetAbilityFromAbilitySet"));
		return false;
	}

	// Check if the action is in the current moveset actions tag, if not we will try to get it from the default ability set
	if (abilityTagContainer.IsValidIndex(1) && MovesetAbilities.Contains(abilityTagContainer.GetByIndex(1))) {
		const UACFAbilitySet* abilitySet = MovesetAbilities.FindChecked(abilityTagContainer.GetByIndex(1));
		return abilitySet->TryGetActionAbilityByTag(abilityTagContainer.GetByIndex(0), outAction);
	}

	return DefaultAbilitySet->TryGetActionAbilityByTag(abilityTagContainer.GetByIndex(0), outAction);
}

bool UACFAbilitySystemComponent::GetAbilityFromAbilitySet(const FGameplayTag& abilityTag, FActionAbilityConfig& outAction) const
{
	FGameplayTagContainer tags;
	tags.AddTag(abilityTag);
	if (currentMovesetActionsTag != FGameplayTag()) {
		tags.AddTag(currentMovesetActionsTag);
	}
	return GetAbilityFromAbilitySet(tags, outAction);
}

UGameplayAbility* UACFAbilitySystemComponent::GetAbilityInstance(const FGameplayAbilitySpecHandle& abilityHandle) const
{
	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(abilityHandle);
	if (AbilitySpec) {
		return AbilitySpec->GetPrimaryInstance();
	}
	return nullptr;
}

void UACFAbilitySystemComponent::PrintStateDebugInfo(bool bIsEntring)
{
	if (bPrintDebugInfo && GEngine && CharacterOwner) {
		FString ActionName;
		CurrentAbilityTag.GetTagName().ToString(ActionName);
		FString MessageToPrint;
		if (bIsEntring) {
			MessageToPrint = CharacterOwner->GetName() + FString("Started Ability:") + ActionName;
		}
		else {
			MessageToPrint = CharacterOwner->GetName() + FString("Exited Ability:") + ActionName;
		}

		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, MessageToPrint,
			false);
	}
}

/*COMBO COUNTERS*/

void UACFAbilitySystemComponent::SetComboCounter(const FGameplayTag& comboTag, int32 value)
{
	if (ComboCounters.Contains(comboTag)) {
		ComboCounters.Remove(FComboCounter(comboTag, value));
	}
	ComboCounters.Add(FComboCounter(comboTag, value));
	GetOwner()->ForceNetUpdate();
}

int32 UACFAbilitySystemComponent::GetComboCount(const FGameplayTag& comboTag) const
{
	if (ComboCounters.Contains(comboTag)) {
		return ComboCounters.FindByKey(comboTag)->Counter;
	}
	return 0;
}

void UACFAbilitySystemComponent::ResetComboCount(const FGameplayTag& comboTag)
{
	SetComboCounter(comboTag, 0);
}