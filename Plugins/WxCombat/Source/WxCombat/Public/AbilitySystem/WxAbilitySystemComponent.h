// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;
class UWxAbilityBase;
enum class EWxAbilityActivationGroup : uint8;

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	void GiveAbilitySet();

	/**
	 * 입력 액션이 트리거 조건을 만족하고 있다(레벨).
	 * 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 *
	 * 어빌리티 라우팅의 유일한 진입점이다.
	 * 매칭된 어빌리티에 발동을 시도하고, 재발동을 받지 않는 어빌리티에는 입력을 그대로 전달한다(락온 토글 해제).
	 * 발동은 조건이 유지되는 동안 계속 시도하므로, 다른 어빌리티가 걸어 둔 차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다.
	 */
	void AbilityInputActionTriggered(const UInputAction* Action);

	void AbilityInputActionReleased(const UInputAction* Action);

	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. 어빌리티 몽타주 재생과 히트스톱 복원이 공유한다. */
	float GetMontagePlayRate() const;

	/**
	 * 히트스톱(역경직): 재생 중인 몽타주를 잠깐 얼리고 복원을 예약한다.
	 *
	 * 대미지를 적용한 쪽이 자기 ASC를 잡아 부른다.
	 * SourceAbility가 아직 몽타주를 쥐고 있을 때만 걸려, 같은 적중 처리에서 먼저 발동한 반응(패리 등)에 양보한다.
	 */
	void ApplyHitStop(float Duration, const UGameplayAbility* SourceAbility);

	/** 발동이 거부된 어빌리티와 그 사유(차단·쿨다운·코스트 등)를 남긴다. */
	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;

	/**
	 * 지금 배타 점유 중인(Exclusive_Blocking·Reaction) 어빌리티. 없으면 nullptr.
	 * 반응형은 서로를 끊지 않으므로 점유가 둘 이상일 수 있고, 그때는 먼저 찾은 하나만 돌려준다.
	 */
	const UWxAbilityBase* FindActivationGroupBlocker() const;

	void CancelActivationGroupAbilities(EWxAbilityActivationGroup Group, UGameplayAbility* IgnoreAbility);

private:
	void HandleHitStopElapsed(TWeakObjectPtr<UAnimMontage> FrozenMontage);

	FTimerHandle HitStopResumeTimer;

protected:
	// 소유 캐릭터가 "Wx|GAS"를 쓰므로 여기서 같은 경로를 쓰면 Class Defaults 패널에 GAS 헤더가 두 번 그려진다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

};
