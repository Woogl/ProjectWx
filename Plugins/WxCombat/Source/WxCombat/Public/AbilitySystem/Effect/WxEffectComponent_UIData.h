// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectUIData.h"
#include "WxUIData.h"
#include "WxEffectComponent_UIData.generated.h"

/**
 * GE의 표시 데이터. 버프 목록은 아이콘을 채운 GE만 그린다.
 *
 * 베이스가 UGameplayEffectUIData 인 것은 WxUI 가 WxCombat 을 참조할 수 없고 GE 의 컴포넌트 배열도 클래스로만 뒤질 수 있어, 양쪽이 아는 이 엔진 클래스가 유일한 조회 앵커이기 때문이다.
 */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_UIData : public UGameplayEffectUIData, public IWxUIData
{
	GENERATED_BODY()

public:
	//~ Begin IWxUIData
	virtual FText GetTitle() const override;
	virtual FText GetDescription() const override;
	virtual TSoftObjectPtr<UObject> GetIcon() const override;
	//~ End IWxUIData

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Display")
	FText Title;

	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;
};
