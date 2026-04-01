// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PangeaBreedingLibrary.generated.h"

class AActor;
class APangeaEggActor;
class UPangeaBreedableComponent;
class UPangeaBreedingFragment;
class UPangeaCreatureDefinition;
class UPangeaGeneticStrategy;

UCLASS()
class PANGEABREEDINGSYSTEM_API UPangeaBreedingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Pangea|Breeding")
	static UPangeaCreatureDefinition* GetCreatureDefinitionFromActor(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Pangea|Breeding")
	static UPangeaCreatureDefinition* GetCreatureDefinitionFromBreedable(const UPangeaBreedableComponent* Breedable);

	UFUNCTION(BlueprintPure, Category="Pangea|Breeding")
	static UPangeaBreedingFragment* GetBreedingFragmentFromActor(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Pangea|Breeding")
	static UPangeaBreedingFragment* GetBreedingFragmentFromBreedable(const UPangeaBreedableComponent* Breedable);

	UFUNCTION(BlueprintPure, Category="Pangea|Breeding")
	static TSubclassOf<APangeaEggActor> ResolveEggActorClass(const UPangeaBreedableComponent* Breedable, TSubclassOf<APangeaEggActor> FallbackEggClass);

	UFUNCTION(BlueprintCallable, Category="Pangea|Breeding")
	static UPangeaGeneticStrategy* CreateGeneticStrategy(UObject* Outer, const UPangeaBreedableComponent* Breedable, UPangeaGeneticStrategy* FallbackStrategy);
};
