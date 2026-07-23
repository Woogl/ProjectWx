// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;
class UActorComponent;

/**
 * 상호작용 대상의 공용 계약. 대상 액터가 직접 구현한다.
 * 감지(스캔·볼륨)는 WxWorld 의 UWxInteractionComponent 가 맡고, 응답·프롬프트는 그 소유 액터가 본 인터페이스로 제공한다.
 * 소비 도메인(예: WxInventory 픽업)이 WxWorld 에 의존하지 않고도 자기 액터를 상호작용 대상으로 만들 수 있게 한다.
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
	 * 상호작용 응답. 서버 권위에서 UWxInteractionComponent::TryInteract 가 호출한다(비권위 호출 없음).
	 * Source 는 이번 상호작용을 일으킨 인터랙션 컴포넌트다 — 한 액터에 상호작용 영역이 여럿이면(예: 엘리베이터) 이것으로 영역을 가르고, 단일 영역이면 무시한다.
	 */
	virtual void OnInteracted(AActor* Interactor, UActorComponent* Source) = 0;

	/**
	 * HUD 리스트에 표시할 프롬프트 텍스트. 레지스트리가 스캔 때 대상에서 읽는다(pull).
	 * Source 는 이 프롬프트를 요청한 인터랙션 컴포넌트다(다중 영역 대상이 영역별 텍스트를 낼 때 쓴다).
	 */
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const = 0;
};
