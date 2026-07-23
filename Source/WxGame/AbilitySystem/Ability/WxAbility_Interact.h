// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Interact.generated.h"

class UPrimitiveComponent;

/**
 * 상호작용 실행 어빌리티(권위 실행 전용).
 *
 * 감지(주변 스캔)·선택·프롬프트는 이 어빌리티가 아니라 PlayerController 의 UWxInteractionRegistryComponent 가 담당한다.
 * 이 어빌리티는 "선택된 대상에 대한 서버 권위 실행"만 책임진다.
 *
 * 실행은 Event.Interact GameplayEvent 로 발동한다(ServerOnly).
 * 입력을 받은 클라의 레지스트리 컴포넌트가 선택 컴포넌트를 실어 ServerInteract RPC 를 보내고,
 * 서버가 그 페이로드로 Event.Interact 를 폰 ASC 에 송출해 이 어빌리티를 권위에서 활성화한다.
 * ServerOnly 라 클라 인스턴스는 없다 — 코스메틱 예측이 없고(상호작용 연출은 대상 StateTree 가 담당), 실행은 서버 권위에서만 일어난다.
 *
 * 활성화 흐름(ActivateAbility): 사거리·활성 검증 후 대상 액터의 IWxInteractable::OnInteracted 를 호출하고 곧바로 종료한다(fire-and-forget).
 *  - 선택이 없으면 무동작. 사망(State.Dead)·처형 중(State.Finisher)에는 CanActivateAbility 가 활성화를 막는다.
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
	 * 서버 사거리 검증 반경(cm). 레지스트리 컴포넌트의 스캔 반경과 일치시켜야 클라 감지와 서버 검증이 정합한다.
	 * 감지는 클라 컴포넌트가, 검증은 서버 어빌리티가 독립적으로 수행하므로(변조 방지) 양쪽이 각자 보유한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanRadius = 150.f;

private:
	/**
	 * 선택 메시가 유효하고 활성이며 사거리 안이면 아바타를 instigator 로 소유 액터의 OnInteracted 를 호출한다.
	 * 권위에서만 호출한다.
	 */
	void ExecuteInteract(const UPrimitiveComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo);
};
