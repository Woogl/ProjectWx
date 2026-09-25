// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Dodge.generated.h"

class UCapsuleComponent;
struct FGameplayAbilityTargetDataHandle;

/** 캐릭터 정면을 기준으로 한 시계 방향 8분면 */
UENUM(BlueprintType)
enum class EWxDodgeDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackRight,
	Back,
	BackLeft,
	Left,
	ForwardLeft
};

/**
 * 입력 방향에 해당하는 8방향 섹션(이동 입력이 없으면 Backstep 섹션)을 재생하고, 몽타주의 Effect.Invincible 구간에 피격되면 극한 회피로 이어진다.
 *
 * 행 몽타주에는 섹션 세 종류를 둔다. 각 섹션은 다음 섹션과의 링크를 끊어 하나만 재생되도록 구성한다.
 * - 방향: EWxDodgeDirection 항목명(Forward, ForwardRight, ...). 구성되지 않은 방향은 Forward로 폴백한다.
 * - Backstep: 이동 입력이 없을 때. 없으면 Back 방향 섹션을 쓴다.
 * - 극한 회피: Success 뒤에 방향 항목명(SuccessForward, ...). 끼어드는 시점의 진행 방향으로 고른다. 없으면 극한 회피가 없다.
 *
 * 회피 반격은 여기서 다루지 않는다 — Ability.Dodge만 발행하면 공격 어빌리티가 그 태그로 자기 반격 세트를 고른다.
 * 진입 시점은 회피 몽타주의 StartRecovery가 차단을 푸는 때다.
 *
 * 극한 회피 판정은 몸통 캡슐을 그대로 둔 채 판정 캡슐이 "피하지 않았다면 맞았을 자리"를 추가로 덮는 방식이다.
 * 둘 중 어느 쪽이 잡히든 타겟은 플레이어 액터 하나이므로, 무적의 Immunity가 피해를 막은 통지(OnImmunityBlockGameplayEffectDelegate)로 성공을 판정한다.
 *
 * 8분면 결정은 소유 클라이언트에서 한 번만 하고, 캐릭터 로컬 공간으로 변환한 방향을 TargetData로 보낸다.
 * 서버는 자신의 facing과 무관하게 같은 8분면을 얻으므로 클라이언트/서버 몽타주가 항상 일치한다.
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

	EWxDodgeDirection ResolveDodgeDirection(const FVector& LocalDirection) const;
	FName SelectDodgeSection(const FVector& LocalDirection, const FString& Prefix) const;

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
