// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

UINTERFACE(MinimalAPI)
class UWxInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 가능한 대상의 계약.
 * UWxAbility_Interact가 본 인터페이스를 통해 구체 클래스 의존 없이 상호작용을 트리거한다.
 * 구현체(예: AWxInteractableActor)는 자체적으로 권한 검증과 복제 처리를 책임진다.
 */
class WXWORLD_API IWxInteractable
{
	GENERATED_BODY()

public:
	/** 서버 권한에서 호출되는 상호작용 진입점. 구현체는 자체적으로 권한/조건을 검증해야 한다. */
	virtual void TryInteract(AActor* InstigatorActor) = 0;
};
