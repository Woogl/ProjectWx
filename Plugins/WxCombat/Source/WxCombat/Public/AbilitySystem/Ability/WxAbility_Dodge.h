// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Dodge.generated.h"

class UCapsuleComponent;
struct FGameplayAbilityTargetDataHandle;

/**
 * 입력 방향에 해당하는 8방향 섹션(이동 입력이 없으면 Backstep 섹션)을 재생하고, 몽타주의 Effect.Invincible 구간에 피격되면 극한 회피로 이어진다.
 *
 * AbilityMontage에는 섹션 세 종류를 둔다. 각 섹션은 다음 섹션과의 링크를 끊어 하나만 재생되도록 구성한다.
 * - 방향: 공용 EWxAbilityDirection 항목명(Forward, ForwardRight, ...). 구성되지 않은 방향은 Forward로 폴백한다.
 * - Backstep: 이동 입력이 없을 때. 없으면 Back 방향 섹션을 쓴다.
 * - 극한 회피: 현재 진행 방향의 Success + 방향 항목명(SuccessForward, ...)을 고르고, 없으면 SuccessForward로 폴백하며 둘 다 없으면 전환하지 않는다.
 *
 * 회피 반격은 여기서 다루지 않는다 — Ability.Dodge만 발행하면 UWxAbility_Attack_DodgeCounter가 그 태그를 발동 조건으로 삼는다.
 *
 * 극한 회피 판정은 몸통 캡슐을 그대로 둔 채 판정 캡슐이 "피하지 않았다면 맞았을 자리"를 추가로 덮는 방식이다.
 * 둘 중 어느 쪽이 잡히든 타겟은 플레이어 액터 하나이므로, 무적의 Immunity가 피해를 막은 통지(OnImmunityBlockGameplayEffectDelegate)로 성공을 판정한다.
 *
 * 소유 클라이언트가 캐릭터 로컬 공간의 입력 방향을 TargetData로 보낸다.
 * 클라이언트와 서버는 같은 로컬 방향으로 각각 회피 시작 섹션을 고른다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Dodge : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Dodge();

	/** 섹션 루트모션이 이동 거리를 정하므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	static const FName BackstepSectionName;

	/** 극한 회피 섹션은 이 접두사 뒤에 방향 항목명을 붙인다. */
	static const FString SuccessSectionPrefix;

	/** 실패 시 EndAbility 후 false 반환 */
	bool StartDodge(const FVector& LocalDirection);

	void ListenForDodgeSuccess();
	void ListenForInvincibleWindow();
	
	void ActivateJudgementCapsule();
	void DeactivateJudgementCapsule();
	
	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	void HandleImmunityBlock(const FGameplayEffectSpec& BlockedSpec, const FActiveGameplayEffect* ImmunityEffect);

	UFUNCTION()
	void HandleDodgeSuccess();

	/** 무적 구간에 여러 공격이 막혀도 극한 회피는 회피 1회당 한 번만 전환한다. */
	bool bDodgeSuccessHandled = false;

	FDelegateHandle ImmunityBlockHandle;

	UFUNCTION()
	void HandleInvincibleTagAdded();

	UFUNCTION()
	void HandleInvincibleTagRemoved();

	/**
	 * 평상시엔 콜리전을 끈 채 아바타에 붙어 다니다 무적이 시작되면 켜고 떼어내므로, 판정 위치를 따로 계산할 필요가 없다.
	 * 아바타의 컴포넌트여야 공격 쿼리의 타겟 필터(팀 적대 판정)를 그대로 통과한다.
	 * 무장은 무적 구간에만 이뤄지므로 이 캡슐이 실제 피해로 이어질 일은 없다.
	 */
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> JudgementCapsule;
};
