// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;

/** 상호작용 선택지 하나. 한 대상이 여럿을 내놓으면 HUD 목록에 그 수만큼 행이 생긴다. */
struct FWxInteractionOption
{
	FText Prompt;

	/** 고른 선택지를 대상에 되돌려 줄 때 쓰는 값. 뜻은 대상이 정한다(엘리베이터는 정차 지점 번호). 선택지가 하나뿐인 대상은 INDEX_NONE. */
	int32 Value = INDEX_NONE;
};

/**
 * 상호작용 대상의 공용 계약. 대상 액터가 구현한다 — 컴포넌트는 구현하지 않는다.
 *
 * 능력이 컴포넌트에 담기더라도(대화·장치) 계약은 액터가 들고 그 컴포넌트로 넘긴다.
 * 다만 감지와 사거리는 여전히 콜리전 형상 위에서 돈다.
 * 액터에 쿼리 콜리전이 켜진 프리미티브가 하나도 없으면 스캔에도 사거리에도 걸리지 않는다(스켈레탈이면 피직스 애셋도 필요).
 * 계약이 WxCore 에 있으므로 소비 도메인(예: WxInventory 픽업)이 WxWorld 에 의존하지 않고도 자기 액터를 상호작용 대상으로 만들 수 있다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxInteractable : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxInteractable
{
	GENERATED_BODY()

public:
	/**
	 * 선택지가 비면 지금은 상호작용할 수 없다는 뜻이다. 켜짐은 구현체가 자기 상태에서 파생한다 — 밖에서 켜고 끄는 진입점은 두지 않는다.
	 * 클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다.
	 */
	virtual void GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const = 0;

	/** OptionValue 는 플레이어가 고른 선택지의 Value 다. 클라가 보낸 값이지만, 호출하는 상호작용 어빌리티가 서버에서 지금의 선택지와 대조한 뒤에만 부른다. */
	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) = 0;
};
