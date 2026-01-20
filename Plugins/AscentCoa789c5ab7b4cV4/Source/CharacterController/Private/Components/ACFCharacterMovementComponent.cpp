// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2025. All Rights Reserved.

#include "Components/ACFCharacterMovementComponent.h"
#include "ACFCCTypes.h"
#include "ACFRPGFunctionLibrary.h"
#include "ACFRPGTypes.h"
#include "ARSStatisticsComponent.h"
#include "Animation/ACFAnimInstance.h"
#include "Components/ACFAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include <Camera/CameraComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include "ACFCCFunctionLibrary.h"
#include "ACFActionTypes.h"
#include "Actions/ACFActionAbility.h"

UACFCharacterMovementComponent::UACFCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bEditableWhenInherited = true;
	PrimaryComponentTick.bCanEverTick = true;

	LocomotionStates.Add(FACFLocomotionState(ELocomotionState::EIdle, 0.f, 0.f));
	LocomotionStates.Add(FACFLocomotionState(ELocomotionState::EWalk, 250.f, 180.f));
	LocomotionStates.Add(FACFLocomotionState(ELocomotionState::EJog, 500.f, 350.f));
	LocomotionStates.Add(FACFLocomotionState(ELocomotionState::ESprint, 650.f, 500.f));

	SetIsReplicatedByDefault(true);
	currentLocomotionState = ELocomotionState::EIdle;
	currentMovestance = FGameplayTag();
	SetIsAiming(false);
}


float UACFCharacterMovementComponent::GetRotationRateYaw() const
{
	return RotationRate.Yaw;
}

void UACFCharacterMovementComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UACFCharacterMovementComponent, targetLocomotionState);
	DOREPLIFETIME(UACFCharacterMovementComponent, bAiming);
	DOREPLIFETIME(UACFCharacterMovementComponent, LocomotionStates);
	DOREPLIFETIME(UACFCharacterMovementComponent, CharacterMaxSpeed);
	DOREPLIFETIME(UACFCharacterMovementComponent, bCanMove);
	DOREPLIFETIME(UACFCharacterMovementComponent, currentMovestance);
	DOREPLIFETIME(UACFCharacterMovementComponent, RotationMode);
	DOREPLIFETIME(UACFCharacterMovementComponent, currentLocomotionState);
	DOREPLIFETIME(UACFCharacterMovementComponent, currentMoveset);
	DOREPLIFETIME(UACFCharacterMovementComponent, currentOverlay);
	DOREPLIFETIME(UACFCharacterMovementComponent, currentRider);
}

void UACFCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration) {
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else {
		Super::SimulateMovement(DeltaTime);
	}
}

bool UACFCharacterMovementComponent::CanAttemptJump() const
{
	const UACFAbilitySystemComponent* actionsManager = GetOwner()->FindComponentByClass<UACFAbilitySystemComponent>();
	if (actionsManager && actionsManager->IsPerformingAction()) {
		return IsJumpAllowed() && actionsManager->GetCurrentAction()->GetActionConfig().PerformableInMovementModes.Contains(EMovementMode::MOVE_Falling);
	}

	return IsJumpAllowed(); //(IsMovingOnGround() || IsFalling() Falling included for double-jump and non-zero jump hold time, but validated by character.
}

void UACFCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

void UACFCharacterMovementComponent::SetIsAiming_Implementation(bool bIsAiming)
{
	if (bIsAiming) {
		ActivateLocomotionStance(UGameplayTagsManager::Get().RequestGameplayTag(ACF::AimTag));
		// SetLocomotionState(LocomotionStateWhileAiming);
	}
	else {
		DeactivateCurrentLocomotionStance();
		// ResetToDefaultLocomotionState();
	}
	bAiming = bIsAiming;
	OnAimChanged.Broadcast(bAiming);
}



void UACFCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Character = Cast<ACharacter>(GetOwner());
	UpdateCharacterMaxSpeed();

	LocomotionStates.Sort();
}

void UACFCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	defaultRotMode = RotationMode;

	if (GetOwner()->HasAuthority()) {
		SetLocomotionState(DefaultState);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Owner not found!"));

	}

	if (Character && Character->GetMesh()) {
		animInstance = Cast<UACFAnimInstance>(Character->GetMesh()->GetAnimInstance());
		if (animInstance) {
			SetCurrentMoveset(UACFCCFunctionLibrary::GetMovesetTypeTagRoot());
			SetCurrentOverlay(UACFCCFunctionLibrary::GetAnimationOverlayTagRoot());
		}
	}
	Internal_SetStrafe();

	/* TODO: climbing
	ClimbQueryParams.AddIgnoredActor(GetOwner());
	*/
}

void UACFCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLocomotion();
	/* TODO: climbing
	if (IsClimbing())
	{
		SweepAndStoreWallHits();
	}*/
}

void UACFCharacterMovementComponent::UpdateLocomotion()
{
	if (!Character) {
		return;
	}

	if (GetOwner() && !IsFalling() && animInstance && !animInstance->IsAnyMontagePlaying()) {
		if (MovementMode == MOVE_Walking) {
			for (int i = 0; i < LocomotionStates.Num() - 1; i++) {
				const float Speed = GetOwner()->GetVelocity().Size();
				if (FMath::IsNearlyZero(Speed) && currentLocomotionState != ELocomotionState::EIdle) {
					HandleStateChanged(ELocomotionState::EIdle);
				}

				else if (LocomotionStates[i + 1].State != currentLocomotionState && Speed > LocomotionStates[i].MaxStateSpeed + 5.f && Speed <= LocomotionStates[i + 1].MaxStateSpeed + 5.f) {
					HandleStateChanged(LocomotionStates[i + 1].State);
				}
			}

			if (GetOwner()->HasAuthority() && currentLocomotionState == ELocomotionState::ESprint) {
				const float Direction = animInstance->GetDirection();
				if (FMath::Abs(Direction) > SprintDirectionCone) {
					SetLocomotionState(ELocomotionState::EJog);
				}
			}
		}

	}
}

const FCharacterGroundInfo& UACFCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame)) {
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking) {
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else {
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(CharacterOwner->GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ACFCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking) {
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit) {
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}

		const FVector normal = HitResult.ImpactNormal;
		const FRotator actorRot = CharacterOwner->GetActorRotation();
		const FVector upVector = UKismetMathLibrary::GetUpVector(actorRot);
		const FVector rightVector = UKismetMathLibrary::GetUpVector(FRotator(0.f, actorRot.Yaw, 0.f));
		UKismetMathLibrary::GetSlopeDegreeAngles(rightVector, normal, upVector,
			CachedGroundInfo.SlopePitch, CachedGroundInfo.SlopeRoll);
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

const float UACFCharacterMovementComponent::GetGroundDistance()
{
	return GetGroundInfo().GroundDistance;
}


void UACFCharacterMovementComponent::SetLocomotionState_Implementation(ELocomotionState State)
{
	if (targetLocomotionState == State) {
		return;
	}

	UpdateMaxSpeed(State);

	OnTargetLocomotionStateChanged.Broadcast(State);
}

void UACFCharacterMovementComponent::SetLocomotionStateSpeed_Implementation(
	ELocomotionState State, float speed, float swimSpeed)
{
	FACFLocomotionState newState = FACFLocomotionState(State, speed, swimSpeed);
	newState.StateModifier = LocomotionStates.FindByKey(State)->StateModifier;
	LocomotionStates.Remove(State);
	LocomotionStates.AddUnique(newState);
	UpdateCharacterMaxSpeed();
	// needed to force the update of the speed
	SetLocomotionState(currentLocomotionState);
}

void UACFCharacterMovementComponent::SetCanMove_Implementation(bool inCanMove)
{
	bCanMove = inCanMove;
}

float UACFCharacterMovementComponent::GetCharacterMaxSpeedByState(ELocomotionState State)
{
	FACFLocomotionState* state = LocomotionStates.FindByKey(State);
	if (state) {
		return state->MaxStateSpeed;
	}
	return 0.0f;
}

float UACFCharacterMovementComponent::GetCharacterMaxSwimSpeedByState(ELocomotionState State)
{
	FACFLocomotionState* state = LocomotionStates.FindByKey(State);
	if (state) {
		return state->MaxStateSwimSpeed;
	}
	return 0.0f;
}

void UACFCharacterMovementComponent::UpdateCharacterMaxSpeed()
{
	if (Character->HasAuthority()) {
		float maxspeed = 0.0f;
		for (const FACFLocomotionState& state : LocomotionStates) {
			if (state.MaxStateSpeed >= maxspeed) {
				maxspeed = state.MaxStateSpeed;
			}
		}
		CharacterMaxSpeed = maxspeed;
	}
}

void UACFCharacterMovementComponent::OnRep_LocomotionState()
{
	MaxWalkSpeed = GetCharacterMaxSpeedByState(targetLocomotionState.State);
	targetLocomotionState.MaxStateSpeed = GetCharacterMaxSpeedByState(targetLocomotionState.State);
}

void UACFCharacterMovementComponent::OnRep_Moveset()
{
	if (GetACFAnimInstance()) {
		GetACFAnimInstance()->SetMoveset(currentMoveset);
	}
}

void UACFCharacterMovementComponent::OnRep_Overlay()
{
	if (GetACFAnimInstance()) {
		GetACFAnimInstance()->SetAnimationOverlay(currentOverlay);
	}
}

void UACFCharacterMovementComponent::OnRep_RiderLayer()
{
	if (GetACFAnimInstance()) {
		GetACFAnimInstance()->SetRidingLayer(currentRider);
	}
}

void UACFCharacterMovementComponent::OnRep_CurrentLocomotionState()
{
	OnLocomotionStateChanged.Broadcast(currentLocomotionState);
}

void UACFCharacterMovementComponent::HandleStateChanged(ELocomotionState newState)
{
	if (currentLocomotionState == newState) {
		return;
	}

	FACFLocomotionState* oldState = LocomotionStates.FindByKey(currentLocomotionState);
	FACFLocomotionState* nextState = LocomotionStates.FindByKey(newState);

	if (oldState && nextState && Character) {
		if (GetOwner()->HasAuthority()) {

			if (activeEffect.IsValid()) {
				UACFRPGFunctionLibrary::RemovesActiveGameplayEffectFromActor(activeEffect, GetOwner());
				activeEffect.Invalidate();
			}

			UARSStatisticsComponent* statComp = Character->FindComponentByClass<UARSStatisticsComponent>();
			if (statComp) {
				activeEffect = statComp->AddAttributeSetModifier(nextState->StateModifier);
			}
		}
	}
	currentLocomotionState = newState;
	OnLocomotionStateChanged.Broadcast(newState);
}

void UACFCharacterMovementComponent::OnRep_LocomotionStance()
{
	OnLocomotionStanceChanged.Broadcast(currentMovestance);
}

void UACFCharacterMovementComponent::OnRep_IsStrafing()
{
	Internal_SetStrafe();
}

void UACFCharacterMovementComponent::OnRep_IsAiming()
{
	OnAimChanged.Broadcast(bAiming);
}

void UACFCharacterMovementComponent::Internal_SetStrafe()
{
	bOrientRotationToMovement = !(RotationMode == ERotationMode::EStrafing); // Character moves in the direction of input..
	bUseControllerDesiredRotation = RotationMode == ERotationMode::EStrafing;
	OnRotationModeChanged.Broadcast(RotationMode);
}

void UACFCharacterMovementComponent::SetRotationMode(ERotationMode inRotMode)
{
	RotationMode = inRotMode;
	Internal_SetStrafe();
}

void UACFCharacterMovementComponent::ResetStrafeMovement()
{
	SetRotationMode(defaultRotMode);
}

void UACFCharacterMovementComponent::ActivateLocomotionStance_Implementation(FGameplayTag locStance)
{
	if (currentMovestance == locStance) {
		return;
	}

	if (currentMovestance != FGameplayTag()) {
		DeactivateCurrentLocomotionStance();
	}

	currentMovestance = locStance;
	// UACFRPGFunctionLibrary::AddGameplayTagToActor(GetOwner(), currentMovestance);
	OnLocomotionStanceChanged.Broadcast(currentMovestance);
}

void UACFCharacterMovementComponent::DeactivateLocomotionStance_Implementation(FGameplayTag locStance)
{
	if (currentMovestance == locStance) {
		DeactivateCurrentLocomotionStance();
	}
}

void UACFCharacterMovementComponent::DeactivateCurrentLocomotionStance_Implementation()
{
	//  UACFRPGFunctionLibrary::RemoveGameplayTagFromActor(GetOwner(), currentMovestance);
	currentMovestance = FGameplayTag();

	OnLocomotionStanceChanged.Broadcast(currentMovestance);
}

void UACFCharacterMovementComponent::AccelerateToNextState_Implementation()
{
	LocomotionStates.Sort();

	const int32 actualindex = LocomotionStates.IndexOfByKey(currentLocomotionState);

	if (LocomotionStates.IsValidIndex(actualindex + 1)) {
		SetLocomotionState(LocomotionStates[actualindex + 1].State);
	}
}

void UACFCharacterMovementComponent::BrakeToPreviousState_Implementation()
{
	LocomotionStates.Sort();

	const int32 actualindex = LocomotionStates.IndexOfByKey(currentLocomotionState);

	if (LocomotionStates.IsValidIndex(actualindex - 1)) {
		SetLocomotionState(LocomotionStates[actualindex - 1].State);
	}
}

void UACFCharacterMovementComponent::TurnAtRate(float Rate)
{
	if (Character) {
		Character->AddControllerYawInput(Rate * TurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void UACFCharacterMovementComponent::LookUpAtRate(float Rate)
{
	if (Character) {
		Character->AddControllerPitchInput(Rate * LookUpRate * GetWorld()->GetDeltaSeconds());
	}
}



void UACFCharacterMovementComponent::TurnAtRateLocal(float Value)
{
	if (Value != 0 && Character && Character->Controller) {
		const FRotator Rotation = Character->Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const float RotationValue = Value * GetRotationRateYaw() * UGameplayStatics::GetWorldDeltaSeconds(this);
		const FRotator rotationDir(0, Rotation.Yaw + RotationValue, 0);
		Direction = rotationDir.RotateVector(Direction);

		AddInputVector(Direction * Value, false);
	}
}

void UACFCharacterMovementComponent::MoveForward(float Value)
{
	if (!bCanMove) {
		return;
	}
	// catch the forward axis

	MoveForwardAxis = Value;

	if (Character && Character->Controller && (MoveForwardAxis != 0.0f)) {
		// find out which way is forward
		const FRotator Rotation = Character->Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);


		// get forward vector
		/* TODO
		const FVector Direction = IsClimbing() ?
			FVector::CrossProduct(GetClimbSurfaceNormal(), -GetOwner()->GetActorRightVector()) :
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);*/


		AddInputVector(Direction * Value, false);
	}
}

void UACFCharacterMovementComponent::MoveRight(float Value)
{
	if (!bCanMove)
		return;

	// Catch the right axis

	MoveRightAxis = Value;

	if (Character && Character->Controller && (MoveRightAxis != 0.0f)) {
		// find out which way is right
		const FRotator Rotation = Character->Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		/* TO DO Climbing
		const FVector Direction = IsClimbing() ?
			FVector::CrossProduct(GetClimbSurfaceNormal(), GetOwner()->GetActorUpVector()) :
			FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);*/


		// add movement in that direction
		Character->AddMovementInput(Direction, Value);
	}
}

void UACFCharacterMovementComponent::MoveUp(float Value)
{
	if (!bCanMove)
		return;

	// Catch the right axis

	MoveUpAxis = Value;

	if (Character && Character->Controller && (MoveUpAxis != 0.0f)) {
		// get up vector
		const FVector Direction = FVector(0.f, 0.f, 1.f);
		// add movement in that direction
		Character->AddMovementInput(Direction, Value);
	}
}

FVector UACFCharacterMovementComponent::GetWorldMovementInputVector()
{
	if (Character) {
		const FVector localDir = FVector(MoveForwardAxis, MoveRightAxis, 0.f);

		FVector WorlDir = Character->GetActorForwardVector().Rotation().RotateVector(localDir);
		WorlDir.Normalize();
		return WorlDir;
	}
	return FVector();
}

FVector UACFCharacterMovementComponent::GetCameraMovementInputVector()
{
	if (Character) {
		const FVector localDir = FVector(MoveForwardAxis, MoveRightAxis, 0.f);
		if (localDir.IsNearlyZero()) {
			return FVector::ZeroVector;
		}
		FVector WorlDir = Character->GetController()->GetControlRotation().RotateVector(localDir);
		WorlDir.Normalize();
		return WorlDir;
	}
	return FVector();
}

FVector UACFCharacterMovementComponent::GetCurrentInputVector()
{
	FVector dir = FVector(MoveForwardAxis, MoveRightAxis, 0.f);
	dir.Normalize();
	return dir;
}

EACFDirection UACFCharacterMovementComponent::GetCurrentInputDirection()
{
	if (Character) {
		const FVector direction = GetCurrentInputVector();

		if (FMath::Abs(direction.X) > FMath::Abs(direction.Y)) {
			if (direction.X > 0) {
				return EACFDirection::Front;
			}
			else {
				return EACFDirection::Back;
			}
		}
		else {
			if (direction.Y > 0) {
				return EACFDirection::Right;
			}
			else {
				return EACFDirection::Left;
			}
		}
	}
	return EACFDirection::Front;
}

void UACFCharacterMovementComponent::ResetToDefaultLocomotionState()
{
	SetLocomotionState(DefaultState);
}

void UACFCharacterMovementComponent::UpdateMaxSpeed(ELocomotionState State)
{
	FACFLocomotionState* locState = LocomotionStates.FindByKey(State);

	if (locState) {
		targetLocomotionState = *(locState);
		MaxWalkSpeed = GetCharacterMaxSpeedByState(State);
		MaxSwimSpeed = GetCharacterMaxSwimSpeedByState(State);
		targetLocomotionState.MaxStateSpeed = GetCharacterMaxSpeedByState(State);

	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Locomotion State inexistent"));
	}
}

/*CLIMBING EXPERIMENTAL

void UACFCharacterMovementComponent::TryClimbing_Implementation()
{
	SweepAndStoreWallHits();

	const FVector Forward = UpdatedComponent->GetForwardVector();
	auto HitIt = CurrentWallHits.CreateConstIterator();
	while (!bWantsToClimb && HitIt)
	{
		bWantsToClimb = IsWallClimbable(*HitIt, Forward);
		++HitIt;
	}
}

void UACFCharacterMovementComponent::CancelClimbing_Implementation()
{
	bWantsToClimb = false;
}

bool UACFCharacterMovementComponent::IsClimbing() const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == EACFCustomMovementMode::Climbing;
}

FVector UACFCharacterMovementComponent::GetClimbSurfaceNormal() const
{
	return CurrentClimbingNormal;
}


void UACFCharacterMovementComponent::SweepAndStoreWallHits()
{
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CollisionCapsuleRadius, CollisionCapsuleHalfHeight);

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 20.;

	// Avoid using the same Start/End location for a Sweep, as it doesn't trigger hits on Landscapes.
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	TArray<FHitResult> Hits;
	const bool HitWall = GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionShape, ClimbQueryParams);

#ifdef WITH_EDITOR
	DrawDebugCapsule(GetWorld(), Start, CollisionCapsuleHalfHeight, CollisionCapsuleRadius, FQuat::Identity, FColor::Green, false, -1, 0, 3);
	for (const auto& Hit : Hits)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 8, FColor::Yellow, false, -1.f, 0, .5f);
	}
#endif
	// Before storing them we could filter non-walls out:
	// We could either create a custom trace channel or decide if any specific kind of actor should be filtered out, e.g. Pawns
	CurrentWallHits = MoveTemp(Hits);

}

bool UACFCharacterMovementComponent::IsWallClimbable(const FHitResult& Hit, const FVector& Forward) const noexcept
{
	const FVector HorizontalNormal = Hit.Normal.GetSafeNormal2D();

	const float HorizontalDot = FVector::DotProduct(Forward, -HorizontalNormal);
	const float VerticalDot = FVector::DotProduct(Hit.Normal, HorizontalNormal);

	const float HorizontalDegrees = FMath::RadiansToDegrees(FMath::Acos(HorizontalDot));

	const bool bIsCeiling = FMath::IsNearlyZero(VerticalDot);

	return HorizontalDegrees <= MinHorizontalDegreesToStartClimbing && !bIsCeiling && IsFacingSurface(VerticalDot);
}

bool UACFCharacterMovementComponent::EyeHeightTrace(const float TraceDistance) const noexcept
{
	FHitResult UpperEdgeHit;

	const FVector Start = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * GetCharacterOwner()->BaseEyeHeight);
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);
#if WITH_EDITOR
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1.f, 0, 1.f);
#endif
	return GetWorld()->LineTraceSingleByChannel(UpperEdgeHit, Start, End, ECC_WorldStatic, ClimbQueryParams);
}

bool UACFCharacterMovementComponent::IsFacingSurface(const float Steepness) const
{
	constexpr float BASE_LENGTH = 80.f;
	const float SteepnessMultiplier = 1 + (1 - Steepness) * 5;

	return EyeHeightTrace(BASE_LENGTH * SteepnessMultiplier);
}

void UACFCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	if (bWantsToClimb)
	{
		SetMovementMode(EMovementMode::MOVE_Custom, EACFCustomMovementMode::Climbing);
	}

	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UACFCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;

		// TODO: Check if needed
		//UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		//Capsule->SetCapsuleHalfHeight(Capsule->GetUnscaledCapsuleHalfHeight() - ClimbingCollisionShrinkAmount);

		StopMovementImmediately();
	}

	if (PreviousMovementMode == EMovementMode::MOVE_Custom && PreviousCustomMode == EACFCustomMovementMode::Climbing)
	{
		bOrientRotationToMovement = true;
		const FRotator StandRotation = FRotator(0., UpdatedComponent->GetComponentRotation().Yaw, 0.);
		UpdatedComponent->SetRelativeRotation(StandRotation);

		// TODO: Check if needed
		//UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		//Capsule->SetCapsuleHalfHeight(Capsule->GetUnscaledCapsuleHalfHeight() + ClimbingCollisionShrinkAmount);

		StopMovementImmediately();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
		OnMovementModeChangedEvent.Broadcast(MovementMode);
}

void UACFCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode == EACFCustomMovementMode::Climbing)
	{
		PhysClimbing(DeltaTime, Iterations);
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

void UACFCharacterMovementComponent::PhysClimbing_Implementation(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	ComputeSurfaceInfo();

	if (ShouldStopClimbing() || ClimbDownToFloor())
	{
		StopClimbing(DeltaTime, Iterations);
		return;
	}

	ComputeClimbingVelocity(DeltaTime);

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	MoveAlongClimbingSurface(DeltaTime);

	TryClimbUpLedge();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / DeltaTime;
	}

	SnapToClimbingSurface(DeltaTime);

}

void UACFCharacterMovementComponent::ComputeSurfaceInfo()
{
	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentClimbingPosition = FVector::ZeroVector;

	if (CurrentWallHits.IsEmpty())
	{
		return;
	}

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(6);

	for (const auto& Hit : CurrentWallHits)
	{
		const FVector End = Start + (Hit.ImpactPoint - Start).GetSafeNormal() * 120.f;

		// TODO: Check if in more complex scenarios this is really needed, simple ones like flat surface don't
		FHitResult AssistHit;
		GetWorld()->SweepSingleByChannel(AssistHit, Start, End, FQuat::Identity, ECC_WorldStatic, CollisionSphere, ClimbQueryParams);

		CurrentClimbingPosition += AssistHit.ImpactPoint;
		CurrentClimbingNormal += AssistHit.Normal;
	}

	CurrentClimbingPosition /= CurrentWallHits.Num();
	CurrentClimbingNormal = CurrentClimbingNormal.GetSafeNormal();

#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), CurrentClimbingPosition, 5.f, 8, FColor::Blue, false, -1.f, 0, .5f);
	DrawDebugLine(GetWorld(), CurrentClimbingPosition, CurrentClimbingPosition + 10.f * CurrentClimbingNormal, FColor::Blue, false, -1.f, 0, 1.f);
#endif

}

void UACFCharacterMovementComponent::ComputeClimbingVelocity(float DeltaTime)
{
	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(DeltaTime, .0f, false, BrakingDecelerationClimbing);
	}

	ApplyRootMotionToVelocity(DeltaTime);
}

bool UACFCharacterMovementComponent::ShouldStopClimbing()
{
	const bool bIsOnCeiling = FVector::Parallel(CurrentClimbingNormal, FVector::UpVector);
	return !bWantsToClimb || CurrentClimbingNormal.IsZero() || bIsOnCeiling;
}

void UACFCharacterMovementComponent::StopClimbing(float DeltaTime, int32 Iterations)
{
	bWantsToClimb = false;
	SetMovementMode(EMovementMode::MOVE_Falling);
	StartNewPhysics(DeltaTime, Iterations);
}

void UACFCharacterMovementComponent::MoveAlongClimbingSurface(float DeltaTime)
{
	const FVector Adjusted = Velocity * DeltaTime;

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, GetClimbingRotation(DeltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}
}

void UACFCharacterMovementComponent::SnapToClimbingSurface(float DeltaTime) const
{
	const FVector Forward = UpdatedComponent->GetForwardVector();
	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();

	const FVector ForwardDifference = (CurrentClimbingPosition - Location).ProjectOnTo(Forward);
	const FVector Offset = -CurrentClimbingNormal * (ForwardDifference.Length() - DistanceFromSurface);

	UpdatedComponent->MoveComponent(Offset * ClimbingSnapSpeed * DeltaTime, Rotation, true);
}

float UACFCharacterMovementComponent::GetMaxSpeed() const
{
	return IsClimbing() ? MaxClimbingSpeed : Super::GetMaxSpeed();
}

float UACFCharacterMovementComponent::GetMaxAcceleration() const
{
	return IsClimbing() ? MaxClimbingAcceleration : Super::GetMaxAcceleration();
}

FQuat UACFCharacterMovementComponent::GetClimbingRotation(float DeltaTime) const
{
	const FQuat Current = UpdatedComponent->GetComponentQuat();

	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return Current;
	}

	const FQuat Target = FRotationMatrix::MakeFromX(-CurrentClimbingNormal).ToQuat();
	return FMath::QInterpTo(Current, Target, DeltaTime, ClimbingRotationSpeed);
}

bool UACFCharacterMovementComponent::ClimbDownToFloor() const
{
	FHitResult FloorHit = CheckFloor(GetWorld(), UpdatedComponent->GetComponentLocation(), FloorCheckDistance, ClimbQueryParams);
	if (!FloorHit.bBlockingHit)
	{
		return false;
	}

	const bool bOnWalkableFloor = FloorHit.Normal.Z > GetWalkableFloorZ();
	const float DownSpeed = FVector::DotProduct(Velocity, -FloorHit.Normal);
	const bool bIsMovingTowardsFloor = DownSpeed >= MaxClimbingSpeed / 3 && bOnWalkableFloor;

	const bool bIsClimbingFloor = CurrentClimbingNormal.Z > GetWalkableFloorZ();

	return bIsMovingTowardsFloor || (bIsClimbingFloor && bOnWalkableFloor);
}

bool UACFCharacterMovementComponent::TryClimbUpLedge() const
{
	if (animInstance && LedgeClimbMontage && animInstance->Montage_IsPlaying(LedgeClimbMontage))
	{
		return false;
	}

	const float UpSpeed = FVector::DotProduct(Velocity, UpdatedComponent->GetUpVector());
	const bool bIsMovingUp = UpSpeed >= MaxClimbingSpeed / 10;

	if (bIsMovingUp && HasReachedEdge() && CanMoveToLedgeClimbLocation())
	{
		const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
		UpdatedComponent->SetRelativeRotation(StandRotation);

		animInstance->Montage_Play(LedgeClimbMontage);

		return true;
	}

	return false;
}

bool UACFCharacterMovementComponent::HasReachedEdge() const
{
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float TraceDistance = Capsule->GetUnscaledCapsuleRadius() * 2.5f;

	return !EyeHeightTrace(TraceDistance);
}

bool UACFCharacterMovementComponent::CanMoveToLedgeClimbLocation() const
{
	const FVector VerticalOffset = FVector::UpVector * ClimbUpVerticalOffset;
	const FVector HorizontalOffset = UpdatedComponent->GetForwardVector() * ClimbUpHorizontalOffset;

	const FVector LocationToCheck = UpdatedComponent->GetComponentLocation() + HorizontalOffset + VerticalOffset;

	if (!IsLocationWalkable(GetWorld(), LocationToCheck, GetWalkableFloorZ(), ClimbQueryParams))
	{
		return false;
	}

	FHitResult CapsuleHit;
	const FVector CapsuleStartCheck = LocationToCheck - HorizontalOffset;
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();

#if WITH_EDITOR
	DrawDebugCapsule(GetWorld(), LocationToCheck, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Red, false, -1, 0, 2.f);
#endif

	return !GetWorld()->SweepSingleByChannel(
		CapsuleHit, CapsuleStartCheck, LocationToCheck, FQuat::Identity, ECC_WorldStatic, Capsule->GetCollisionShape(), ClimbQueryParams);

}

bool UACFCharacterMovementComponent::IsLocationWalkable(const UWorld* World, const FVector& LocationToCheck, const float WalkableHeight, const FCollisionQueryParams& QueryParams) const
{

	const FVector CheckEnd = LocationToCheck + (FVector::DownVector * 250.);
	FHitResult LedgeHit;
	const bool bHitLedgeGround = World->LineTraceSingleByChannel(LedgeHit, LocationToCheck, CheckEnd, ECC_WorldStatic, QueryParams);

#if WITH_EDITOR
	DrawDebugLine(World, LocationToCheck, CheckEnd, FColor::Red, false, -1.f, 0, 4.f);
#endif

	return bHitLedgeGround && LedgeHit.Normal.Z >= WalkableHeight;
}

FHitResult UACFCharacterMovementComponent::CheckFloor(const UWorld* World, const FVector& Location, float MaxDistance, const FCollisionQueryParams& QueryParams) const {
	FHitResult Hit{};
	const FVector End = Location + FVector::DownVector * MaxDistance;
	World->LineTraceSingleByChannel(Hit, Location, End, ECC_WorldStatic, QueryParams);
	return Hit;
}*/