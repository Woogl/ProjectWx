// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;
class USkeletalMeshComponent;

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

	/** 몽타주를 시작한 엔진의 AnimatingAbility 전이에 맞춰 메시 본 갱신 정책을 승격한다. */
	virtual float PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, float StartTimeSeconds = 0.0f) override;

	/** 마지막 AnimatingAbility가 해제될 때 몽타주 전의 메시 본 갱신 정책을 복원한다. */
	virtual void ClearAnimatingAbility(UGameplayAbility* Ability) override;

	void GiveAbilitySet();

	/**
	 * 입력 액션이 트리거 조건을 만족하고 있다(레벨).
	 * 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 *
	 * 어빌리티 라우팅의 유일한 진입점이다.
	 */
	void AbilityInputActionTriggered(const UInputAction* Action);

	void AbilityInputActionReleased(const UInputAction* Action);

	/**
	 * 창이 열리는 전이점(콤보 창·후딜)에서 어빌리티가 부른다.
	 * 만료된 항목은 버리고 남은 항목을 누른 순서로 시도한다.
	 */
	void FlushBufferedInputs();

	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. 어빌리티가 오버라이드하지 않으면 그 어빌리티의 몽타주 재생 속도가 된다. */
	float GetMontagePlayRate() const;

	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;

	/** 후딜에 들어 점유를 놓은 배타 어빌리티를 끊는다. */
	void CancelRecoveringAbilities(UGameplayAbility* IgnoreAbility);

private:
	/** 버퍼 재생 경로다. 뗀 뒤의 재생이라 스펙의 키 상태는 세우지 않는다. */
	bool TryActivateByInputAction(const UInputAction* Action);

	void EnableAnimatingMontageMeshTick();
	void RestoreAnimatingMontageMeshTick();

	TArray<FWxBufferedInput> BufferedInputs;

	/** AnimatingAbility가 존재하는 동안에만 강제한 메시와 원래 옵션. 단일 소유자라 별도 참조 수가 필요 없다. */
	TWeakObjectPtr<USkeletalMeshComponent> MontageTickMesh;
	EVisibilityBasedAnimTickOption PreviousMontageTickOption;

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
