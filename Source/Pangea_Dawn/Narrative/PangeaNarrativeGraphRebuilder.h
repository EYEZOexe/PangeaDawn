#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PangeaNarrativeGraphRebuilder.generated.h"

UCLASS()
class PANGEA_DAWN_API UPangeaNarrativeGraphRebuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Pangea|Narrative|Editor")
	static bool RebuildNarrativeGraphForAsset(UObject* Asset);

	UFUNCTION(BlueprintCallable, Category = "Pangea|Narrative|Editor")
	static bool RebuildTutorialNarrativeGraphs();
};
