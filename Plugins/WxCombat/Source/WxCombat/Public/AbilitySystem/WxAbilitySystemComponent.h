// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	void GiveAbilitySet();

	/**
	 * 입력 액션이 트리거 조건을 만족하고 있다(레벨). 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 * 어빌리티 라우팅의 유일한 진입점이다. 매칭된 어빌리티에 발동을 시도하고, 재발동을 받지 않는 어빌리티에는 입력을 그대로 전달한다(락온 토글 해제, 패링 중 가드 복귀).
	 * 발동은 조건이 유지되는 동안 계속 시도하므로, 다른 어빌리티가 걸어 둔 차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다.
	 * 반대로 이미 돌고 있는 어빌리티에는 홀드 반복분을 넘기지 않는다.
	 */
	void AbilityInputActionTriggered(const UInputAction* Action);

	/** 입력 액션에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputActionReleased(const UInputAction* Action);

	/** AbilitySet의 어빌리티들이 요구하는 입력 액션 전체(플레이어 입력 바인딩용) */
	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. 어빌리티 몽타주 재생과 히트스톱 복원이 공유한다. */
	float GetMontagePlayRate() const;

	/** Event.HitStop이면 히트스톱을 적용하고, 라우팅은 순정 경로에 그대로 위임한다 */
	virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;

private:
	/** 히트스톱(역경직): 대미지를 준 어빌리티의 재생 중인 몽타주를 잠깐 얼리고 복원을 예약한다 */
	void ApplyHitStop(const FGameplayEventData& Payload);

	/** 얼렸던 그 몽타주의 재생률을 되돌린다 */
	void HandleHitStopElapsed(TWeakObjectPtr<UAnimMontage> FrozenMontage);

	FTimerHandle HitStopResumeTimer;

protected:
	/** Ability, Effect 초기 데이터 */
	// 소유 캐릭터가 "Wx|GAS"를 쓰므로 여기서 같은 경로를 쓰면 Class Defaults 패널에 GAS 헤더가 두 번 그려진다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

};
