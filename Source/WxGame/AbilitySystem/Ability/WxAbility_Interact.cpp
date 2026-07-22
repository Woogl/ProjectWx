// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "GameFramework/Actor.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	// 레지스트리 컴포넌트(클라)가 선택 대상을 실어 보낸 뒤, 서버가 폰 ASC 로 송출하는 GameplayEvent 로 발동한다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Interact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 클라 예측이 없다 — 코스메틱 연출은 대상 StateTree 가 담당하고, 실행은 서버 권위에서만 일어난다.
	// 선택 전달은 레지스트리 컴포넌트의 ServerInteract RPC 가 담당하므로 LocalPredicted 의 페이로드 통로가 필요 없다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 사망 중에는 활성화 거부.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 처형 연출 중에는 상호작용 재입력을 막는다(WxAbility_Finisher가 State.Finisher를 발행).
	// 연출 도중 근처 다른 대상과 상호작용해 처형 흐름에 개입하는 것을 차단한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Finisher);
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 대상은 이벤트 페이로드로 온다(서버가 레지스트리 컴포넌트의 선택을 실어 송출).
	// OptionalObject 가 const 라 실행을 위해 const_cast 한다(WxAbility_Finisher 의 Target 과 동일).
	UWxInteractionComponent* Selected = TriggerEventData
		? const_cast<UWxInteractionComponent*>(Cast<UWxInteractionComponent>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	// ServerOnly 라 항상 권위지만, 방어적으로 게이트한다.
	if (HasAuthority(&ActivationInfo))
	{
		ExecuteInteract(Selected, ActorInfo);
	}

	// 상호작용 모션·연출은 대상 StateTree 가 담당하므로 어빌리티는 즉시 종료한다.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Interact::ExecuteInteract(UWxInteractionComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Selected || !Avatar)
	{
		return;
	}

	// 서버 권위 거리 검증: 감지·선택은 클라 로컬이라, 변조 클라가 임의의 원거리 컴포넌트를 보내 상호작용하는 것을 막는다.
	// 클라 스캔의 overlap 과 동일 판정 — 중심간 거리 <= ScanRadius + 볼륨 바운딩 반경 — 으로 사거리를 재확인한다.
	const float ReachRadius = ScanRadius + Selected->GetInteractionReachRadius();
	if (FVector::DistSquared(Avatar->GetActorLocation(), Selected->GetInteractionLocation()) > FMath::Square(ReachRadius))
	{
		return;
	}

	Selected->TryInteract(Avatar);
}
