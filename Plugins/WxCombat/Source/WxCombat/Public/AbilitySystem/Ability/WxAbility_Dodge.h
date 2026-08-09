// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
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
 * 회피 어빌리티.
 * 입력 방향에 해당하는 8방향 섹션(이동 입력이 없으면 BackstepMontage)을 재생하고, 몽타주의 State.Invincible 구간에 피격되면 극한 회피로 이어진다.
 *
 * 회피 반격은 여기서 다루지 않는다 — State.Dodge만 발행하면 공격 어빌리티가 그 태그로 자기 반격 세트를 고른다.
 * 진입 시점은 회피 몽타주의 StartRecovery가 차단을 푸는 때다.
 *
 * 극한 회피 판정은 몸통 캡슐을 그대로 둔 채 판정 캡슐이 "피하지 않았다면 맞았을 자리"를 추가로 덮는 방식이다.
 * 둘 중 어느 쪽이 잡히든 타겟은 플레이어 액터 하나이므로, 무적을 확인한 데미지 파이프라인이 Event.DodgeSuccess를 발송한다.
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

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * 8방향 회피 섹션을 담은 몽타주.
	 * 섹션 이름은 EWxDodgeDirection 항목명(Forward, ForwardRight, ...)과 동일해야 하며, 각 섹션은 다음 섹션과의 링크를 끊어 한 방향만 재생되도록 구성한다.
	 * 구성되지 않은 방향 섹션은 Forward 섹션으로 폴백하므로, 일부 섹션만 채워 점진적으로 구성할 수 있다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** 이동 입력 없이 회피할 때 재생할 백스텝 몽타주. 미설정 시 DodgeMontage의 Back 섹션으로 폴백한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstepMontage;

	/** 극한 회피 성공 시 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> PerfectDodgeMontage;

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
	EWxDodgeDirection ResolveDodgeDirection(const FVector& LocalDirection) const;
	FName SelectDodgeSection(const FVector& LocalDirection) const;

	/** 실패 시 EndAbility 후 false 반환 */
	bool StartDodge(const FVector& LocalDirection);
	bool PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);
	
	void ListenForDodgeSuccess();
	void ListenForInvincibleWindow();
	
	void ActivateJudgementCapsule();
	void DeactivateJudgementCapsule();
	
	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	UFUNCTION()
	void HandleDodgeSuccess(FGameplayEventData Payload);

	UFUNCTION()
	void HandleInvincibleTagAdded();

	UFUNCTION()
	void HandleInvincibleTagRemoved();

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

	/**
	 * 극한 회피 판정용 캡슐.
	 * 평상시엔 콜리전을 끈 채 아바타에 붙어 다니다 무적이 시작되면 켜고 떼어내므로, 판정 위치를 따로 계산할 필요가 없다.
	 * 아바타의 컴포넌트여야 공격 쿼리의 타겟 필터(ACharacter 요구·팀)를 그대로 통과한다.
	 * 무장은 무적 구간에만 이뤄지므로 이 캡슐이 실제 피해로 이어질 일은 없다.
	 */
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> JudgementCapsule;
};
