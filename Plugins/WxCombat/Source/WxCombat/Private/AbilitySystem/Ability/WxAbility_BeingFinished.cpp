// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_BeingFinished.h"
#include "Animation/AnimMontage.h"
#include "WxGameplayTags.h"

UWxAbility_BeingFinished::UWxAbility_BeingFinished()
{
	// 부여도 발동도 서버가 한다. 피해자는 소유 클라가 없어 몽타주는 복제로 퍼진다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 그로기 위에 겹쳐 재생되어야 하고, 그로기의 전체 취소 지목에도 끊기지 않아야 한다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_BeingFinished);
	SetAssetTags(AssetTags);

	// 활성 태그는 존재가 전원에 복제되므로 다른 클라의 프롬프트 게이트에도 닿는다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_BeingFinished);
}

float UWxAbility_BeingFinished::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_BeingFinished::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
