// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGameplayEffect;
class UWxEffect_Cooldown;
class AWxProjectileBase;
class UInputAction;
struct FWxAbilityTableRow;

UENUM(BlueprintType)
enum class EWxAbilityActivationPolicy : uint8
{
	/** 트리거(입력·이벤트·AI)를 기다려 활성화 */
	OnTriggered,
	/** 부여될 때 즉시 자동 활성화 (패시브, 상시 버프 등) */
	OnGiven,
};

/**
 * 어빌리티 발동을 그룹 단위로 묶어서 배타적으로 점유할 수 있다.
 * CancelAbilitiesWithTag로 상대를 지목한 어빌리티는 이 판정보다 우선해 발동할 수 있다.
 *
 * Exclusive_ 세 값은 몽타주 노티파이가 순서대로 밟는 하나의 축이다 — 닫힘에서 열림으로 Blocking → ComboWindow → Recovery.
 */
UENUM()
enum class EWxAbilityActivationGroup : uint8
{
	/** 막지도 막히지도 않는다. */
	Independent,

	/** 배타적으로 다른 Exclusive 어빌리티 발동을 막는다. */
	Exclusive_Blocking,

	/** (런타임 전환) 콤보 창. 자기 재발동만 통과시키고, 남의 발동은 Exclusive_Blocking처럼 막는다. */
	Exclusive_ComboWindow,

	/** (런타임 전환) 액션을 캔슬할 수 있게 된 후딜레이 상태. 막지 않는 것은 Independent와 같다. */
	Exclusive_Recovery,

	/** Exclusive 어빌리티에 막히지 않고 발동한다. 주로 HitReaction, Groggy, Death에서 사용. */
	Reaction, // Override 같은 이름으로 바꿀까???
};

/**
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	/**
	 * 기본적으로 쿨다운·코스트 수치는 AbilityDataRow에서 읽어서 공용 GE를 쓴다.
	 * 나중에 필요하다면 커스텀 쿨다운·코스트 GE를 만들 수 있음.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnTriggered;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UInputAction> ActivationInputAction;
	
	/** 후딜 전이(StartRecovery)가 Exclusive_Recovery로 바꾸고, 다음 활성화가 선언값으로 되돌린다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	EWxAbilityActivationGroup ActivationGroup = EWxAbilityActivationGroup::Independent;
	
	/**
	 * 활성 구간 동안 소유자에게 유지되는 효과. ActivationOwnedTags의 GE판으로, 활성화에서 걸고 종료에서 걷는다.
	 * 수명이 어빌리티에 묶이므로 각 GE는 지속시간을 두지 않는다(Infinite).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TSubclassOf<UGameplayEffect>> ActivationOwnedEffects;
	
	// TODO: GE Handle 기록해두어서 정확하게 자기가 부여했던 GE만 걷어야함
	void RemoveActivationOwnedEffect(TSubclassOf<UGameplayEffect> EffectClass) const;
	
	TSoftObjectPtr<UObject> GetIcon() const;

	/**
	 * 일반적으로는 아바타 ASPD가 반영된 몽타주 재생 속도 사용.
	 * 고정된 시간을 맞춰야하는 등 특수한 경우에는 1을 반환하도록 오버라이드한다.
	 */
	virtual float GetMontagePlayRate() const;

	/**
	 * 콤보 창 구간. 자기 재발동만 열어주므로 회피·가드 같은 남의 캔슬은 여전히 막힌다.
	 * 콤보를 후딜보다 이르게 잇되 공격에 리스크를 남기기 위한 구분이다.
	 */
	void OpenComboWindow();

	/** 후딜이 이미 열렸으면(창이 후딜보다 늦게 닫히는 배치) 그대로 둔다. */
	void CloseComboWindow();

	/**
	 * 후딜레이 구간.
	 * Exclusive_Blocking으로 막혀있던 발동 그룹 잠금을 풀어서 그 순간부터 이후 발동하는 Exclusive 어빌리티에 의한 캔슬을 허용한다.
	 * 코스트·쿨다운·ActivationBlockedTags는 그대로 검사한다.
	 */
	void StartRecovery();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CanBeCanceled() const override;

	/// WxAnimNotify_SpawnProjectile이 위임하는 투사체 스폰
	void SpawnProjectile(TSubclassOf<AWxProjectileBase> ProjectileClass, FName SpawnSocketName) const;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	// 발동 횟수 스택을 여러개 충전해두었다가 연속 발동할 수 있는 어빌리티도 있다. (Recharages)
	int32 QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	bool PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

	UAnimMontage* GetActiveMontage() const;

	UFUNCTION()
	virtual void HandleMontageCompleted();

	UFUNCTION()
	virtual void HandleMontageBlendOut();

	UFUNCTION()
	virtual void HandleMontageInterrupted();

	UFUNCTION()
	virtual void HandleMontageCancelled();

private:
	const FWxAbilityTableRow* GetTableRow() const;

	/**
	 * 다중 충전 어빌리티에서만 만드는 GE 인스턴스.
	 * StackLimitCount에 최대 충전 수를 실어 ViewModel에 전달한다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cooldown> CooldownEffect;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage;
};
