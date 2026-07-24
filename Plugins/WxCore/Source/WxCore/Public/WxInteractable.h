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

	/**
	 * 이 주체가 지금 이 영역과 상호작용할 수 있는가.
	 * 주체별로 자격이 갈리는 대상(예: 처형 — 주체가 후방에 있어야 뒤잡)만 오버라이드한다. 기본은 항상 허용이라 기존 구현체는 영향이 없다.
	 * 채널 응답은 머신당 값이 하나뿐이라 "주체 A 에겐 가능, 주체 B 에겐 불가"를 표현할 수 없으므로, 그런 자격은 채널이 아니라 여기서 판정한다.
	 *
	 * 스캐너가 클라에서 로컬 폰을 주체로 호출해 표시를 거르고, 상호작용 어빌리티가 서버에서 실제 instigator 를 주체로 호출해 권위 검증한다.
	 * 판정 입력이 전부 복제돼야 양쪽이 같은 답에 수렴한다.
	 */
	virtual bool CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const { return true; }

	/** HUD 리스트에 표시할 프롬프트 텍스트. 스캐너가 스캔 때 대상에서 읽는다(pull). */
	virtual FText GetInteractionPrompt() const = 0;
};
