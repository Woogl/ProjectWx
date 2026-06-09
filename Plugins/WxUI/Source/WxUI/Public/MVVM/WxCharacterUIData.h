// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxCharacterUIData.generated.h"

class UTexture2D;

/**
 * 캐릭터의 UI 표시 데이터.
 * 캐릭터 BP 디폴트에서 저작하여 UWxViewModel_Character 로 주입한다.
 * Effect 의 UWxEffectComponent_UIData 에 대응하는 캐릭터 버전으로, "캐릭터가 UI 에 내놓는 데이터 모양"을 WxUI 가 소유한다.
 * 함께 저작·주입·소비되는 값 묶음이라 개별 인자로 펼치지 않고 한 단위로 전달한다.
 */
USTRUCT(BlueprintType)
struct FWxCharacterUIData
{
	GENERATED_BODY()

	/** 네임플레이트/HUD 등에 출력되는 표시 이름. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText CharacterName;

	/** 초상화. UI 측에서 Soft 참조로 비동기 로드한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** 설명 문구. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (MultiLine = "true"))
	FText Description;
};
