#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PangeaFootprintFocusWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class GF_PANGEAHUNTINGSYSTEMRUNTIME_API UPangeaFootprintFocusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Hunting|Focus")
	void SetFocusState(bool bInHasTarget, float InProgress);

	UFUNCTION(BlueprintCallable, Category="Hunting|Focus")
	void SetRingCenter(FVector2D InRingCenter);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Focus")
	float RingRadius = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Focus")
	float RingThickness = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Focus")
	int32 SegmentCount = 48;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Focus")
	FLinearColor BackgroundColor = FLinearColor(0.05f, 0.95f, 0.45f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hunting|Focus")
	FLinearColor ProgressColor = FLinearColor(0.05f, 0.95f, 0.45f, 0.95f);

private:
	bool bHasTarget = false;
	float Progress = 0.f;
	FVector2D RingCenter = FVector2D::ZeroVector;
};
