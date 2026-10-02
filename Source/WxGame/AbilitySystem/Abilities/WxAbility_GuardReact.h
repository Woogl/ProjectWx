// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "WxAbility_GuardReact.generated.h"

/**
 * 가드가 흡수한 히트의 연출을 맡는다 — 가드 피격·넉 계열 가드·가드 브레이크·퍼펙트 가드.
 * 행 몽타주에는 섹션 GuardHit, GuardKnockback(넉 계열 공격을 가드), GuardBreak, PerfectGuard를 둔다.
 *
 * 가드 어빌리티에서 떼어낸 이유는 복제다.
 * 대미지 GE가 피격자 클라에서는 예측 키가 없어 적용되지 않으니 그 이벤트도 서버에만 남는데, ServerInitiated 트리거는 엔진이 페이로드째 소유 클라에 복제해 준다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_GuardReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_GuardReact();

	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool PlayMontageInternal(UAnimMontage* Montage, FName StartSection) override;

	/** GuardKnockback 섹션 전체에 걸쳐 이동할 거리. 해당 구간의 애니메이션 루트모션은 노티파이로 억제한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability|Knockback", meta = (ClampMin = "0.0", Units = "cm"))
	float KnockbackDistance = 100.f;

	/** 넉백은 이동 구간을 완주한다. 그 외 반응은 블렌드아웃부터 가드 자세로 돌아간다. */
	virtual void HandleMontageBlendOut() override;

private:
	bool bWaitForKnockbackCompletion = false;
	FVector KnockbackDirection = FVector::ZeroVector;

	static FName SelectSection(FGameplayTag TriggerTag, FGameplayTag ReactionTag);

	static const FName GuardHitSectionName;
	static const FName GuardKnockbackSectionName;
	static const FName GuardBreakSectionName;
	static const FName PerfectGuardSectionName;
};
