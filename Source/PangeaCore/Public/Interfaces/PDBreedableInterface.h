#pragma once

#include "CoreMinimal.h"
#include "Types/BreedingTypes.h"
#include "UObject/Interface.h"
#include "PDBreedableInterface.generated.h"

UINTERFACE(BlueprintType)
class UPDBreedableInterface : public UInterface
{
	GENERATED_BODY()
};

class PANGEACORE_API IPDBreedableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Breeding")
	FParentSnapshot GetParentSnapshot() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Breeding")
	bool IsFertile() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Breeding")
	bool SetFertile(bool bNewFertile);
};
