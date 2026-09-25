// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Ultimate.generated.h"

/**
 * 컷신(Level Sequence)을 재생한 뒤 공격 몽타주를 실행하며, 컷신 동안 월드는 시간 정지한다.
 * 컷신은 몽타주의 UWxAnimNotify_SkillCutscene이 담고, 재생 전에 읽는다. 없으면 몽타주만 재생한다.
 * 활성 구간 내내 슈퍼 아머가 붙어 경직(HitReact)이 막힌다 — 대미지는 그대로 들어온다.
 *
 * 컷신 시퀀스는 몽타주를 거친 하드 참조다.
 * 부여는 소유 클라에만 복제되지만 이 클래스는 시전자 캐릭터와 함께 모든 머신에 올라오므로, 관전 머신도 로드 대기 없이 재생을 시작한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Ultimate : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Ultimate();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleCutsceneCompleted();

	UFUNCTION()
	void HandleCutsceneCancelled();
};
