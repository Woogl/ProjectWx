// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "WxEffectComponent_UIData.generated.h"

/**
 * GE의 표시 데이터. 버프 목록은 아이콘을 채운 GE만 그린다.
 *
 * WxGame 리졸버와 WxEditor 썸네일이 이 컴포넌트를 읽는다.
 */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_UIData : public UGameplayEffectUIData
{
	GENERATED_BODY()

public:
	FText GetTitle() const;
	FText GetDescription() const;
	TSoftObjectPtr<UObject> GetIcon() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Display")
	FText Title;

	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;
};
