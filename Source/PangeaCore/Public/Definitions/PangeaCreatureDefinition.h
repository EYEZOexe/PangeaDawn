#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PangeaCreatureDefinition.generated.h"

class AActor;
class UPangeaDefinitionFragment;

UCLASS(BlueprintType)
class PANGEACORE_API UPangeaCreatureDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Definition")
	FName SpeciesId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Definition")
	TSoftClassPtr<AActor> CreatureClass;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Definition")
	TArray<TObjectPtr<UPangeaDefinitionFragment>> Fragments;

	UFUNCTION(BlueprintPure, Category="Definition")
	UPangeaDefinitionFragment* GetFragmentByClass(TSubclassOf<UPangeaDefinitionFragment> FragmentClass) const;

	template <typename T>
	T* GetFragment() const
	{
		return Cast<T>(GetFragmentByClass(T::StaticClass()));
	}
};
