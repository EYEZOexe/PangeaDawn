// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/PDDinosaurBase.h"
#include "Components/PangeaBreedableComponent.h"
#include "ACFMountComponent.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFQuadrupedMovementComponent.h"
#include "ACFVaultComponent.h"
#include "ALSLoadAndSaveComponent.h"
#include "Definitions/PangeaCreatureDefinition.h"
#include "EditorCategoryUtils.h"
#include "Components/ACFTeamComponent.h"
#include "Interfaces/PDTameableInterface.h"
#include "Components/GameFrameworkComponentManager.h"


APDDinosaurBase::APDDinosaurBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BreedableComponent = CreateDefaultSubobject<UPangeaBreedableComponent>(TEXT("Pangea Breeding Component"));
	MountComponent = CreateDefaultSubobject<UACFMountComponent>(TEXT("ACF Mount Component"));
	VaultComponent = CreateDefaultSubobject<UACFVaultComponent>(TEXT("ACF Vault Component"));
	ALSLoadAndSaveComponent = CreateDefaultSubobject<UALSLoadAndSaveComponent>(TEXT("ALS Load And Save Component"));
}

void APDDinosaurBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void APDDinosaurBase::BeginPlay()
{
	Super::BeginPlay();
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		this, 
		UGameFrameworkComponentManager::NAME_GameActorReady);
}

void APDDinosaurBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}



void APDDinosaurBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ChangeVelocityState();
}

#pragma region Save System

TArray<UActorComponent*> APDDinosaurBase::GetComponentsToSave_Implementation() const
{
	TArray<UActorComponent*> ComponentsToSave;

	ComponentsToSave.Add(BreedableComponent);
	//ComponentsToSave.Add(TamingComponent);
	ComponentsToSave.Add(TeamComponent);

	return ComponentsToSave;
}

#pragma endregion

#pragma region ACF Interaction Interface

bool APDDinosaurBase::CanBeInteracted_Implementation(class APawn* Pawn)
{
	UActorComponent* TamingComponent = FindComponentByInterface(UPDTameableInterface::StaticClass());
	if (!TamingComponent) return false;
	
	IPDTameableInterface* Tameable = Cast<IPDTameableInterface>(TamingComponent);
	
	return !MountComponent->IsMounted() && Tameable->CanBeTamed();
}

void APDDinosaurBase::OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
	UActorComponent* TamingComponent = FindComponentByInterface(UPDTameableInterface::StaticClass());
    
	if (!TamingComponent)
	{
		return;
	}
	
	IPDTameableInterface* Tameable = Cast<IPDTameableInterface>(TamingComponent);
	AACFCharacter* ACFCharacter = Cast<AACFCharacter>(Pawn);

	if (Tameable->GetTameState() != ETameState::Tamed)
	{
		// Not tamed yet → maybe start a taming attempt instead
		Tameable->StartTameAttempt(Pawn);
		return;
	}

	if (Tameable->GetTamedRole() == ETamedRole::Mount)
	{
		//request gameplay tag
		FGameplayTag MountTag = FGameplayTag::RequestGameplayTag("Actions.Mount");
		FGameplayTag DismountTag = FGameplayTag::RequestGameplayTag("Actions.Dismount");
	
		if (!ACFCharacter) return;

		if (MountComponent->IsMounted())
		{
			ACFCharacter->TriggerAction(DismountTag, EActionPriority::ELow, false);
		}
		else
		{
			ACFCharacter->TriggerAction(MountTag, EActionPriority::EHigh, false);
		}
	}
	else if (Tameable->GetTamedRole() == ETamedRole::Companion)
	{
		UE_LOG(LogEngine, Display, TEXT("You Pet Dino!"));
	}
}

FText APDDinosaurBase::GetInteractableName_Implementation()
{
	UActorComponent* TamingComponent = FindComponentByInterface(UPDTameableInterface::StaticClass());
	if (!TamingComponent) return FText::FromString("Interact");
	
	IPDTameableInterface* Tameable = Cast<IPDTameableInterface>(TamingComponent);
	
	if (Tameable->GetTameState() == ETameState::Wild)
	{
		return FText::FromString("Tame");
	}
	
	if (Tameable->GetTameState() == ETameState::Tamed)
	{
		if (Tameable->GetTamedRole() == ETamedRole::Mount) return FText::FromString("Mount");
		if (Tameable->GetTamedRole() == ETamedRole::Companion) return FText::FromString("Pet");
	}
	
	return FText::FromString("Interact");
}

UPangeaCreatureDefinition* APDDinosaurBase::GetCreatureDefinition_Implementation() const
{
	return CreatureDefinition;
}

#pragma endregion

#pragma region Private Movement Functions

void APDDinosaurBase::Accelerate(float Value)
{
	//cast movement component to ACFQuadrupedMovementComponent
	UACFQuadrupedMovementComponent* QuadMovementComp = Cast<UACFQuadrupedMovementComponent>(GetMovementComponent());
	if (QuadMovementComp)
	{
		QuadMovementComp->MoveForwardLocal(Value);
	}
}

void APDDinosaurBase::Brake(float Value)
{
	UACFQuadrupedMovementComponent* QuadMovementComp = Cast<UACFQuadrupedMovementComponent>(GetMovementComponent());
	if (QuadMovementComp)
	{
		QuadMovementComp->MoveForwardLocal(Value);
	}
}

void APDDinosaurBase::OnLoaded_Implementation()
{
	if (UActorComponent* TamingComponent = FindComponentByInterface(UPDTameableInterface::StaticClass()))
	{
		IPDTameableInterface* Tameable = Cast<IPDTameableInterface>(TamingComponent);
		
		Tameable->HandleLoadedActor();
	}
}

void APDDinosaurBase::ChangeVelocityState()
{
	if (bIsAccelerating)
	{
		Accelerate(DefaultAcceleration);
	}
	else if (bIsBraking)
	{
		Brake(DefaultDeceleration);
	}
}

#pragma endregion
