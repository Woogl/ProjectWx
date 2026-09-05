// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_PlayMontageOnce.h"
#include "Animation/AnimMontage.h"
#include "WxGameplayTags.h"

UWxAbility_PlayMontageOnce::UWxAbility_PlayMontageOnce()
{
	// 부여도 발동도 서버가 한다. 소유 클라가 없는 폰에도 걸리므로 몽타주는 복제로 퍼진다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 진행 중인 액션 위에 겹쳐 재생되어야 하고, 그 액션의 전체 취소 지목에도 끊기지 않아야 한다.
	ActivationGroup = EWxAbilityActivationGroup::Override;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_PlayMontageOnce);
	SetAssetTags(AssetTags);

	// 활성 태그는 존재가 전원에 복제되므로 다른 클라의 프롬프트 게이트에도 닿는다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_PlayMontageOnce);
}

float UWxAbility_PlayMontageOnce::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_PlayMontageOnce::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// OptionalObject 는 const 라 재생 API 에 맞춰 여기서 벗긴다.
	UAnimMontage* Montage = TriggerEventData
		? const_cast<UAnimMontage*>(Cast<UAnimMontage>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	if (!PlayMontage(Montage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 회전은 몽타주가 실제로 걸린 뒤에 — 실패해 곧장 종료하는 경로에서 자세만 바뀌는 일이 없다.
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	if (!AvatarActor || !Instigator)
	{
		return;
	}

	FVector Direction = Instigator->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (!Direction.IsNearlyZero())
	{
		AvatarActor->SetActorRotation(Direction.ToOrientationRotator());
	}
}
