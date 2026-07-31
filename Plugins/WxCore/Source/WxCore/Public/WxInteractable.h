// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxInteractable.generated.h"

class AActor;
class UActorComponent;
class UPrimitiveComponent;

/**
 * 상호작용 대상의 공용 계약. 대상 액터가 직접 구현한다.
 * 상호작용 영역은 액터의 메시 그 자체이며, 어느 메시가 영역인지는 대상이 IsInteractionMeshActive 로 직접 답한다 — 어느 채널·응답을 쓰는지는 관여하지 않는다.
 * 다만 영역 메시에 쿼리 콜리전이 켜져 있는 것은 전제다(자세한 이유는 IsMeshInRange 참조). 응답·프롬프트도 그 메시의 소유 액터가 본 인터페이스로 제공한다.
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
	 * 영역 메시의 계약 구현체를 찾는다. 소유 액터가 직접 구현했으면 그것을, 아니면 그 액터의 컴포넌트를 답한다.
	 * 컴포넌트 갈래 덕에 호스트 액터를 순수 BP 로 둘 수 있다 — 이 인터페이스는 BP 에서 구현할 수 없으므로, C++ 없는 액터는 컴포넌트로만 상호작용 대상이 된다.
	 * 소비처(스캐너·상호작용 어빌리티)가 전부 이 조회를 거치므로 구현체를 어디에 두든 바뀌는 곳은 여기 하나다.
	 */
	static IWxInteractable* Find(const UActorComponent* Mesh);

	/**
	 * 영역 메시가 Origin 에서 Radius 안에 있는가. 서버 사거리 검증(상호작용 어빌리티)이 쓴다.
	 *
	 * 영역 메시엔 반드시 쿼리 콜리전이 켜져 있어야 한다(스켈레탈이면 피직스 애셋도 필요) — 이것이 이 판정의 전제다.
	 * 꺼져 있으면 이 함수가 항상 false 를 돌려주고 클라 스캐너의 구 오버랩에도 잡히지 않아 프롬프트조차 뜨지 않는다.
	 * 즉 쿼리 콜리전은 사거리뿐 아니라 감지 자체의 전제이며, 영역 메시를 NoCollision 으로 내리는 것은 그 대상의 상호작용을 통째로 끄는 것과 같다.
	 * 개발 빌드는 이 전제가 깨지면 진입부의 ensure 로 드러낸다.
	 *
	 * 반면 콜리전 "응답·프로파일"은 보지 않는다 — 채널이 아니라 바디에 직접 던지는 테스트이기 때문이다.
	 * 대상 자격 자체도 IsInteractionMeshActive 가 정하므로 프리셋과 무관하다. 콜리전은 "얼마나 가까워야 하는가"에만 쓰인다.
	 */
	static bool IsMeshInRange(const UPrimitiveComponent* Mesh, const FVector& Origin, float Radius);

	/**
	 * 이 메시가 지금 상호작용 가능한 내 영역인가. 어느 메시가 영역인가(표식)와 그 영역이 지금 켜져 있는가(활성)를 이 판정 하나가 함께 답한다.
	 * 켜고 끄는 방식은 구현체가 정한다 — 상시 활성이면 자기 메시인지만 보고, 상태별로 갈리면(기믹) 그 상태에서 켜진 영역인지 본다.
	 *
	 * 스캐너가 클라에서 주변 후보를 거를 때 묻고, 상호작용 어빌리티가 서버에서 선택 메시로 활성을 권위 검증한다.
	 * 양쪽이 같은 답에 수렴하도록 켜고 끄는 근거는 전 머신에서 동일해야 한다(기믹은 복제 State 로 구동되는 StateTree 가 각 피어에서 토글한다).
	 */
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const = 0;

	/**
	 * 상호작용 응답. 서버 권위에서 상호작용 어빌리티가 호출한다(비권위 호출 없음).
	 * Source 는 이번 상호작용이 일어난 대상 메시다 — 한 액터에 상호작용 영역이 여럿이면(예: 엘리베이터) 이것으로 영역을 가르고, 단일 영역이면 무시한다.
	 */
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) = 0;

	/**
	 * 이 주체가 지금 이 영역과 상호작용할 수 있는가.
	 * 주체별로 자격이 갈리는 대상(예: 처형 — 주체가 후방에 있어야 뒤잡)만 오버라이드한다. 기본은 항상 허용이라 기존 구현체는 영향이 없다.
	 * 활성 목록은 머신당 값이 하나뿐이라 "주체 A 에겐 가능, 주체 B 에겐 불가"를 표현할 수 없으므로, 그런 자격은 목록이 아니라 여기서 판정한다.
	 *
	 * 스캐너가 클라에서 로컬 폰을 주체로 호출해 표시를 거르고, 상호작용 어빌리티가 서버에서 실제 instigator 를 주체로 호출해 권위 검증한다.
	 * 판정 입력이 전부 복제돼야 양쪽이 같은 답에 수렴한다.
	 */
	virtual bool CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const { return true; }

	/**
	 * HUD 리스트에 표시할 프롬프트 텍스트. 스캐너가 스캔 때 대상에서 읽는다(pull).
	 * Source 는 프롬프트를 물어보는 대상 메시다 — 상호작용 영역이 여럿이면(예: 엘리베이터) 이것으로 영역별 문구를 가르고, 단일 영역이면 무시한다.
	 */
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const = 0;
};
