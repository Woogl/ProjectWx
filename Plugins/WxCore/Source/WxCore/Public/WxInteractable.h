// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;
class UActorComponent;

/**
 * 상호작용 대상의 공용 계약. 대상 액터가 직접 구현한다.
 * 상호작용 영역은 액터의 메시 그 자체다 — WxInteractable 채널에 Overlap 응답으로 표식하면 플레이어 측 스캐너에 잡히고, Ignore 로 바꾸면 탈락한다.
 * 응답·프롬프트는 그 메시의 소유 액터가 본 인터페이스로 제공한다.
 * 채널 정의가 WxCore 에 있으므로 소비 도메인(예: WxInventory 픽업)이 WxWorld 에 의존하지 않고도 자기 액터를 상호작용 대상으로 만들 수 있다.
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
	 * 상호작용 응답. 서버 권위에서 상호작용 어빌리티가 호출한다(비권위 호출 없음).
	 * Source 는 이번 상호작용이 일어난 대상 메시다 — 한 액터에 상호작용 영역이 여럿이면(예: 엘리베이터) 이것으로 영역을 가르고, 단일 영역이면 무시한다.
	 */
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) = 0;

	/** HUD 리스트에 표시할 프롬프트 텍스트. 레지스트리가 스캔 때 대상에서 읽는다(pull). */
	virtual FText GetInteractionPrompt() const = 0;
};
