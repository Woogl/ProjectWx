// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractionSource.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractedSignature, AActor*, InstigatorActor);

/**
 * 상호작용 알림을 발행하는 컴포넌트의 공용 계약. 구현체는 WxWorld 의 UWxInteractionComponent.
 * 소비 도메인(예: WxInventory 의 픽업)이 WxWorld 에 의존하지 않고, BP 에서 오너에 추가된
 * 상호작용 컴포넌트를 본 인터페이스로 찾아 델리게이트 바인딩과 프롬프트 텍스트 갱신을 할 수 있게 한다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxInteractionSource : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxInteractionSource
{
	GENERATED_BODY()

public:
	/** 상호작용 발생 델리게이트 접근자. 서버+모든 클라이언트에서 fire 된다. */
	virtual FWxOnInteractedSignature& GetOnInteractedDelegate() = 0;

	/** 프롬프트 텍스트 갱신. */
	virtual void SetInteractionText(const FText& InText) = 0;
};
