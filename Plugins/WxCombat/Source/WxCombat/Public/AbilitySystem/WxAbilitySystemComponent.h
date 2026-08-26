// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;
class UWxAbilityBase;
enum class EWxAbilityActivationGroup : uint8;

/** 액션 중에 막힌 채 떼어진 발동 입력. IA는 어빌리티 CDO가 쥐고 있어 ASC보다 오래 살므로 생 포인터로 둔다. */
struct FWxBufferedInput
{
	const UInputAction* Action = nullptr;
	double TriggeredTime = 0.0;
};

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
	 * 액션 중에 막힌 탭은 뗀 뒤에도 InputBufferDuration 동안 기억해 두었다가, 창이 열리는 전이점에서 같은 시도를 한다(입력 버퍼).
	 */
	void AbilityInputActionTriggered(const UInputAction* Action);

	void AbilityInputActionReleased(const UInputAction* Action);

	/**
	 * 창이 열리는 전이점(콤보 창·후딜)에서 어빌리티가 부른다.
	 * 만료된 항목은 버리고 남은 항목을 누른 순서로 시도한다. 하나가 성립하면 나머지는 낡은 것이라 전부 버리고, 실패한 항목은 다음 전이점까지 남긴다.
	 */
	void FlushBufferedInputs();

	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. 어빌리티가 오버라이드하지 않으면 그 어빌리티의 몽타주 재생 속도가 된다. */
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
	 * 지금 배타 점유 중인(Exclusive_Blocking·Exclusive_ComboWindow·Reaction) 어빌리티 전원.
	 * 반응형은 서로를 끊지 않으므로 점유가 둘 이상일 수 있다. 태그 뚫기 판정은 어빌리티가 조립한다.
	 *
	 * 발동 시도마다 불리고 홀드 입력이면 매 프레임이라, 통상 개수는 인라인 저장으로 받아 힙 할당을 피한다.
	 */
	TArray<const UWxAbilityBase*, TInlineAllocator<4>> FindActivationGroupBlockers() const;

	void CancelActivationGroupAbilities(EWxAbilityActivationGroup Group, UGameplayAbility* IgnoreAbility);

private:
	/** 인스턴스가 배타 점유자면 그 어빌리티를, 아니면 nullptr을 준다. */
	const UWxAbilityBase* AsActivationGroupBlocker(const UGameplayAbility* Instance) const;

	/** 버퍼 재생: Action에 매칭된 어빌리티들에 발동을 시도해 하나라도 성립하면 true. 뗀 뒤의 재생이라 스펙의 키 상태는 세우지 않는다. */
	bool TryActivateByInputAction(const UInputAction* Action);

	void HandleHitStopElapsed();

	TArray<FWxBufferedInput> BufferedInputs;

	/** 히트스톱이 얼린 몽타주와 얼리기 직전의 재생 속도. 복원 예약이 살아 있는 동안에만 읽는다. */
	TWeakObjectPtr<UAnimMontage> HitStopMontage;
	float HitStopResumePlayRate = 1.f;

	FTimerHandle HitStopResumeTimer;

protected:
	// 소유 캐릭터가 "Wx|GAS"를 쓰므로 여기서 같은 경로를 쓰면 Class Defaults 패널에 GAS 헤더가 두 번 그려진다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	/**
	 * 액션 중에 막힌 탭 입력을 뗀 뒤에도 이 시간(실시간 초) 동안 기억한다. 전이점이 그 안에 오지 않으면 버린다.
	 * 슬로우모션이 게임 시간을 늘려도 플레이어의 시계는 그대로라, 입력이 유효한 길이는 실시간으로 잰다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = "0"))
	float InputBufferDuration = 0.4f;

	bool bAbilitySetGranted = false;

};
