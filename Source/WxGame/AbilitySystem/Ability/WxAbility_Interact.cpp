// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
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
	// Ability 하위 태그이므로 GAS 순정 AreAbilityTagsBlocked(Ability) 차단(마시는 중·기믹 연출 중 등)도 함께 존중한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Interact);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Interact);

	// 이 차단 태그들이 서버 활성·클라 표시(스캐너 컴포넌트) 게이트의 단일 소스다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 처형 연출 도중 근처 다른 대상과 상호작용해 처형 흐름에 개입하는 것을 차단한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Finisher);

	// 대화 중에는 상호작용을 막는다(PC의 WxDialogueSessionComponent가 세션 시작·종료에 맞춰 State.Dialogue를 발행).
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dialogue);
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 실행 경로가 선택 메시를 읽기만 하므로 OptionalObject 의 const 를 그대로 들고 간다(const_cast 불필요).
	const UPrimitiveComponent* Selected = TriggerEventData
		? Cast<UPrimitiveComponent>(TriggerEventData->OptionalObject.Get())
		: nullptr;

	// ServerOnly 라 항상 권위지만, 방어적으로 게이트한다.
	if (HasAuthority(&ActivationInfo))
	{
		ExecuteInteract(Selected, ActorInfo);
	}

	// 상호작용 모션·연출은 대상 StateTree 가 담당하므로 어빌리티는 즉시 종료한다.
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Interact::ExecuteInteract(const UPrimitiveComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Selected || !Avatar)
	{
		return;
	}

	// 클라 비주얼은 각 대상의 복제 상태로 수렴하므로 여기서 따로 호출하지 않는다.
	// Source 로 선택 메시를 넘겨, 한 액터에 영역이 여럿이면(예: 엘리베이터) 어느 영역이었는지 가를 수 있게 한다.
	IWxInteractable* Target = IWxInteractable::Find(Selected);
	if (!Target)
	{
		return;
	}

	// 서버 권위 활성 검증: 클라가 비활성 대상을(또는 비활성 직후에) 보내도 여기서 걸린다.
	if (!Target->IsInteractionMeshActive(Selected))
	{
		return;
	}

	// 서버 권위 거리 검증: 감지·선택은 클라 로컬이라, 변조 클라가 임의의 원거리 메시를 보내 상호작용하는 것을 막는다.
	if (!IWxInteractable::IsMeshInRange(Selected, Avatar->GetActorLocation(), ScanRadius))
	{
		return;
	}

	// 서버 권위 자격 검증: 주체별로 자격이 갈리는 대상(처형 등)은 활성 목록으로 표현할 수 없으므로 실제 아바타를 주체로 대상에 묻는다.
	// 기본 구현이 true 라 기믹 등은 영향이 없다.
	if (!Target->CanBeInteractedBy(Avatar, Selected))
	{
		return;
	}

	Target->OnInteracted(Avatar, Selected);
}
