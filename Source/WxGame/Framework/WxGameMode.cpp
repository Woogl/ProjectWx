// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "WxSaveSubsystem.h"

APawn* AWxGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FTransform UseTransform = SpawnTransform;

	const UWxSaveSubsystem* Save = UWxSaveSubsystem::Get(this);
	if (Save && Save->IsSaveApplied() && Save->HasSavedLocation())
	{
		UseTransform = Save->GetSavedPlayerTransform();
	}

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, UseTransform);
}
