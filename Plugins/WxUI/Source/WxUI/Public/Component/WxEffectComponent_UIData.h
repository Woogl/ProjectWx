// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "WxEffectComponent_UIData.generated.h"

UCLASS(DisplayName = "Wx UI Data")
class WXUI_API UWxEffectComponent_UIData : public UGameplayEffectUIData
{
	GENERATED_BODY()

public:
	UWxEffectComponent_UIData();

	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;

	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;
};
