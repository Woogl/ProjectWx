// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Interact.generated.h"

class AActor;

/**
 * 감지(주변 스캔)·선택·프롬프트는 이 어빌리티가 아니라 PlayerController 의 UWxInteractionScannerComponent 가 담당한다.
 * 이 어빌리티는 "선택된 대상에 대한 서버 권위 실행"만 책임진다.
 *
 * 입력을 받은 클라의 스캐너 컴포넌트가 선택 액터를 실어 ServerInteract RPC 를 보내고, 서버가 그 페이로드로 Event.Interact 를 폰 ASC 에 송출해 이 어빌리티를 활성화한다.
 * ServerOnly 라 클라 인스턴스는 없다 — 상호작용 연출은 대상 StateTree 가 담당하므로 코스메틱 예측이 필요 없다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_Interact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Interact();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * 서버 사거리 검증 반경(cm). 스캐너 컴포넌트의 스캔 반경과 일치시켜야 클라 감지와 서버 검증이 정합한다.
	 * 감지는 클라 컴포넌트가, 검증은 서버 어빌리티가 독립적으로 수행하므로(변조 방지) 양쪽이 각자 보유한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanRadius = 150.f;

private:
	/** 권위에서만 호출한다. */
	void ExecuteInteract(AActor* Selected, const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * 프리미티브엔 쿼리 콜리전이 켜져 있어야 한다(스켈레탈이면 피직스 애셋도 필요) — 이것이 이 판정의 전제다.
	 * 하나도 켜져 있지 않으면 이 함수가 항상 false 를 돌려주고 클라 스캐너의 구 오버랩에도 잡히지 않아 프롬프트조차 뜨지 않는다.
	 *
	 * 반면 콜리전 "응답·프로파일"은 보지 않는다 — 채널이 아니라 바디에 직접 던지는 테스트이기 때문이다.
	 */
	bool IsInRange(const AActor* Selected, const FVector& Origin) const;
};
