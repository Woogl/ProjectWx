// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;

/**
 * 상호작용 대상의 공용 계약. 대상 액터나 그 액터의 컴포넌트가 구현한다.
 * 상호작용은 액터 단위다 — 액터 안의 특정 메시만 상호작용 영역이 되는 개념은 없다. 영역을 갈라야 하면 액터를 나눠 배치하고 각자 대상을 지목한다(예: AWxLeverDevice).
 * 다만 감지와 사거리는 여전히 콜리전 형상 위에서 돈다. 액터에 쿼리 콜리전이 켜진 프리미티브가 하나도 없으면 스캔에도 사거리에도 걸리지 않는다(자세한 이유는 IsActorInRange 참조).
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
	 * 액터가 직접 구현했으면 그것을, 아니면 그 액터의 컴포넌트를 답한다.
	 * 컴포넌트 갈래 덕에 호스트 액터를 순수 BP 로 둘 수 있다 — 이 인터페이스는 BP 에서 구현할 수 없으므로, C++ 없는 액터는 컴포넌트로만 상호작용 대상이 된다.
	 */
	static IWxInteractable* Find(AActor* Actor);

	/**
	 * 서버 사거리 검증(상호작용 어빌리티)이 쓴다. 액터의 프리미티브 중 하나라도 구에 겹치면 사거리 안이다.
	 *
	 * 프리미티브엔 쿼리 콜리전이 켜져 있어야 한다(스켈레탈이면 피직스 애셋도 필요) — 이것이 이 판정의 전제다.
	 * 하나도 켜져 있지 않으면 이 함수가 항상 false 를 돌려주고 클라 스캐너의 구 오버랩에도 잡히지 않아 프롬프트조차 뜨지 않는다.
	 *
	 * 반면 콜리전 "응답·프로파일"은 보지 않는다 — 채널이 아니라 바디에 직접 던지는 테스트이기 때문이다.
	 */
	static bool IsActorInRange(const AActor* Actor, const FVector& Origin, float Radius);

	/**
	 * 이 대상이 지금 상호작용을 받을 수 있는가.
	 * 켜고 끄는 방식은 구현체가 정한다 — 상시 활성이면 true 만 답하고, 상태별로 갈리면(기믹) 그 상태가 켠 값을 답한다.
	 *
	 * 스캐너가 클라에서 주변 후보를 거를 때 묻고, 상호작용 어빌리티가 서버에서 선택 액터로 활성을 권위 검증한다.
	 * 양쪽이 같은 답에 수렴하도록 켜고 끄는 근거는 전 머신에서 동일해야 한다(기믹은 각 피어에서 같은 전이를 밟는 StateTree 가 토글하고, 어긋난 피어는 복제된 상태 Tag 로 수렴한다).
	 */
	virtual bool IsInteractionEnabled() const = 0;

	/**
	 * '상호작용 켜기' 태스크가 액터를 지목했을 때 부른다.
	 * 켜고 끄는 수단은 구현체가 정한다 — IsInteractionEnabled 의 답이 그에 맞게 바뀌기만 하면 된다(대화 상대는 영역 메시의 쿼리 콜리전을 내린다).
	 * 여닫을 일이 없는 대상(픽업·적)은 구현하지 않는다 — 기본은 무동작이다.
	 */
	virtual void SetInteractionEnabled(bool bEnabled);

	/** 상호작용 응답. 서버 권위에서 상호작용 어빌리티가 호출한다(비권위 호출 없음). */
	virtual void OnInteracted(AActor* Interactor) = 0;

	/**
	 * 주체별로 자격이 갈리는 대상(예: 처형 — 주체가 후방에 있어야 뒤잡)만 오버라이드한다. 기본은 항상 허용이다.
	 * 활성 판정은 머신당 값이 하나뿐이라 "주체 A 에겐 가능, 주체 B 에겐 불가"를 표현할 수 없으므로, 그런 자격은 거기가 아니라 여기서 판정한다.
	 *
	 * 스캐너가 클라에서 로컬 폰을 주체로 호출해 표시를 거르고, 상호작용 어빌리티가 서버에서 실제 instigator 를 주체로 호출해 권위 검증한다.
	 * 판정 입력이 전부 복제돼야 양쪽이 같은 답에 수렴한다.
	 */
	virtual bool CanBeInteractedBy(const AActor* Interactor) const;

	/** HUD 리스트에 표시할 프롬프트 텍스트. 스캐너가 스캔 때 대상에서 읽는다(pull). */
	virtual FText GetInteractionPrompt() const = 0;
};
