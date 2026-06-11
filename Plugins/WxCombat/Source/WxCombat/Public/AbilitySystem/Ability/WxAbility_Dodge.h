// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UWxAbilityTask_WaitInputTagPressed;
struct FGameplayAbilityTargetDataHandle;

/**
 * 캐릭터 기준 회피 방향 (전방을 기준으로 시계 방향 8분면).
 * 입력 방향과 캐릭터 정면 사이의 각도를 45° 단위로 양자화해 결정한다.
 */
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
 * 회피 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → 회피 몽타주를 입력 방향에 해당하는 8방향 섹션부터 재생, Event.DodgeSuccess 대기
 *  2. 몽타주의 State.Invincible 구간 동안 무적
 *  3. 무적 중 피격(극한 회피) → PerfectDodgeMontage 재생
 *  4. ANS_ComboWindow 구간 내 공격 입력 시 DodgeCounterMontage로 전환
 *  5. 몽타주 완료/중단 → EndAbility
 *
 * 회피 방향은 캐릭터 정면 기준으로 계산한다.
 * 비락온은 섹션 양자화 잔차만큼 시작 시 몸을 회전시켜 루트모션 이동을 입력 방향과 일치시킨다.
 * 락온 중에는 락온 태스크가 회피 내내 몸을 타겟으로 추적하므로 사이드 회피가 타겟 중심 호 궤적이 된다.
 * 8분면 결정은 소유 클라이언트에서 한 번만 수행하고, 입력을 캐릭터 로컬 공간으로 변환해 TargetData로 전송한다.
 * 서버는 자신의 facing과 무관하게 동일한 8분면을 계산하므로 클라이언트/서버 몽타주가 항상 일치한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Dodge : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Dodge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * 8방향 회피 섹션을 담은 몽타주. 입력 방향을 캐릭터 정면 기준 8분면으로 양자화해 섹션을 선택한다.
	 * 섹션 이름은 EWxDodgeDirection 항목명(Forward, ForwardRight, ...)과 동일해야 하며, 각 섹션은 다음 섹션과의 링크를 끊어 한 방향만 재생되도록 구성한다.
	 * 구성되지 않은 방향 섹션은 Forward 섹션으로 폴백하므로, 일부 섹션만 채워 점진적으로 구성할 수 있다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** 극한 회피 성공 시 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> PerfectDodgeMontage;

	/** 극한 회피 성공 중 공격 입력 시 재생할 반격 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeCounterMontage;

	/** 극한 회피 성공 시 회복하는 MP량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float PerfectDodgeMPRecovery = 5.f;

	/** 극한 회피 성공 시 적용할 GlobalTimeDilation 값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.01"))
	float PerfectDodgeSlowTimeDilation = 0.4f;

	/** 극한 회피 성공 시 슬로우 타임 지속 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.0"))
	float PerfectDodgeSlowTimeDuration = 0.4f;

private:
	/** 캐릭터 로컬 공간 입력 방향(정면 +X, 오른쪽 +Y)을 8분면으로 양자화한다. 방향이 0이면 Back(백스텝). facing 무관(클라/서버 동일 결과). */
	EWxDodgeDirection ResolveDodgeDirection(const FVector& LocalDirection) const;

	/** 로컬 공간 입력 방향에 해당하는 회피 몽타주 섹션 이름을 반환한다. 미구성 섹션은 Forward로 폴백, Forward 섹션도 없으면 NAME_None(몽타주 처음부터 재생). */
	FName SelectDodgeSection(const FVector& LocalDirection) const;

	/** 로컬 공간 입력 방향에 맞는 회피 몽타주 섹션을 선택해 재생한다(비락온은 잔차만큼 몸 보정, 락온은 태스크 추적에 위임). 실패 시 EndAbility 후 false 반환. */
	bool StartDodge(const FVector& LocalDirection);

	/** 진행 중인 몽타주 태스크를 정리하고 새 몽타주를 StartSection부터 재생한다. 재생 실패 시 false 반환. */
	bool PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
	void ListenForDodgeSuccess();
	void PlayPerfectDodgeMontage();
	void PlayDodgeCounterMontage();
	void ListenForCounterInput();

	UFUNCTION()
	void HandleDodgeSuccess(FGameplayEventData Payload);

	UFUNCTION()
	void HandleCounterInputPressed();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UWxAbilityTask_WaitInputTagPressed> WaitInputTask;
};
