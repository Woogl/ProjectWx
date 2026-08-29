// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/WxStateTreeTask_WaitForInteraction.h"
#include "WxGameplayTags.h"
#include "WxInteractable.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Interact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 선택 전달은 스캐너 컴포넌트의 ServerInteract RPC 가 담당하므로 LocalPredicted 의 페이로드 통로가 필요 없다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 상호작용 스캐너 컴포넌트(WxWorld)가 이 태그로 스펙을 찾아 CanActivateAbility 로 클라 표시 게이트를 삼는다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Interact);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Interact);

	// 배타 그룹 판정이 그 표시 게이트에 반영되어, 다른 액션 중(마시는 중·장치 연출 중 등)에는 표시가 사라진다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;

	// 이 차단 태그들이 서버 활성·클라 표시(스캐너 컴포넌트) 게이트의 단일 소스다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 처형 연출 도중 근처 다른 대상과 상호작용해 처형 흐름에 개입하는 것을 차단한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Finisher);

	// State.Dialogue는 PC의 WxDialogueSessionComponent가 세션 시작·종료에 맞춰 폰 ASC에 발행한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dialogue);
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// OptionalObject 는 const 라, 대상에 응답을 시켜야 하는 실행 경로를 위해 여기서 벗긴다.
	AActor* Selected = TriggerEventData
		? const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	// ServerOnly 라 항상 권위지만, 방어적으로 게이트한다.
	if (HasAuthority(&ActivationInfo))
	{
		ExecuteInteract(Selected, ActorInfo);
	}

	// 상호작용 모션·연출은 대상 StateTree 가 담당하므로 어빌리티는 즉시 종료한다.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Interact::ExecuteInteract(AActor* Selected, const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Selected || !Avatar)
	{
		return;
	}

	// 클라 비주얼은 각 대상의 복제 상태로 수렴하므로 여기서 따로 호출하지 않는다.
	IWxInteractable* Target = Cast<IWxInteractable>(Selected);
	if (!Target)
	{
		return;
	}

	// 서버 권위 자격 검증: 클라가 자격 없는 대상을(또는 자격을 잃은 직후에) 보내도 여기서 걸린다.
	if (!Target->CanInteract(Avatar))
	{
		return;
	}

	// 서버 권위 거리 검증: 감지·선택은 클라 로컬이라, 변조 클라가 임의의 원거리 대상을 보내 상호작용하는 것을 막는다.
	if (!IsInRange(Selected, Avatar->GetActorLocation()))
	{
		return;
	}

	Target->OnInteracted(Avatar);

	// 이 대상을 기다리던 퀘스트 스텝('상호작용 대기')이 있으면 여기서 완료된다. 기다리는 쪽이 없으면 무동작이다.
	FWxStateTreeTask_WaitForInteraction::NotifyInteracted(Selected);
}

bool UWxAbility_Interact::IsInRange(const AActor* Selected, const FVector& Origin) const
{
	for (const UActorComponent* Component : Selected->GetComponents())
	{
		const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
		if (!Primitive || !Primitive->IsQueryCollisionEnabled())
		{
			continue;
		}

		// 스켈레탈 메시는 OverlapComponent 오버라이드가 피직스 애셋의 모든 바디를 훑는다.
		if (Primitive->OverlapComponent(Origin, FQuat::Identity, FCollisionShape::MakeSphere(ScanRadius)))
		{
			return true;
		}
	}

	return false;
}
