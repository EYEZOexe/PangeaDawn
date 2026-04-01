#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDDefinitionProviderInterface.generated.h"

class UPangeaCreatureDefinition;

UINTERFACE(BlueprintType)
class UPDDefinitionProviderInterface : public UInterface
{
	GENERATED_BODY()
};

class PANGEACORE_API IPDDefinitionProviderInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Definition")
	UPangeaCreatureDefinition* GetCreatureDefinition() const;
};
