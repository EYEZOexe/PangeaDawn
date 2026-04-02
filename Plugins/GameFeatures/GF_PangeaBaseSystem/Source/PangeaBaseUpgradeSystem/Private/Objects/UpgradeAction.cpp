// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/UpgradeAction.h"

void UUpgradeAction::ExecuteForContext(const FBaseUpgradeContext& Context)
{
	Execute(Context.GetPreferredActionObject());
}

void UUpgradeAction::Execute_Implementation(UObject* ContextObject)
{
	// Default: no-op
	UE_LOG(LogTemp, Verbose, TEXT("UUpgradeAction::Execute default no-op on %s"), *GetNameSafe(ContextObject));
}
