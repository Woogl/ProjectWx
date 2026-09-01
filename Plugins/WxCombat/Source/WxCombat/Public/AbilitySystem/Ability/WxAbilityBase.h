// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "WxUIData.h"
#include "WxAbilityBase.generated.h"

class FDataValidationContext;
class UAbilitySystemComponent;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGameplayEffect;
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
 * 기획자가 선언하는 값이며 런타임에 바뀌지 않는다 — 발동 중의 캔슬 창은 EWxAbilityActionPhase가 따로 받는다.
 *
 * CancelAbilitiesWithTag로 상대를 지목한 어빌리티는 이 판정보다 우선해 발동할 수 있다.
 */
UENUM()
enum class EWxAbilityActivationGroup : uint8
{
	/** 막지도 막히지도 않는다. */
	Independent,

	/** 배타적으로 다른 Exclusive 어빌리티 발동을 막는다. */
	Exclusive,

	/** Exclusive 점유를 덮어쓰고 발동하며 캔슬되지도 않는다. 주로 HitReact, Groggy, Death에서 사용. */
	Override,
};

/**
 * Exclusive 어빌리티가 발동 한 번 동안 밟는 캔슬 창.
 * 몽타주 노티파이가 닫힘에서 열림 순으로 전이시킨다 — Blocking → ComboWindow → Recovery.
 */
enum class EWxAbilityActionPhase : uint8
{
	/** 본동작. 남의 배타 발동을 막는다. */
	Blocking,

	/** 콤보 창. 자기 재발동만 통과시키고, 남의 발동은 본동작처럼 막는다. */
	ComboWindow,

	/** 액션을 캔슬할 수 있게 된 후딜레이. 점유를 놓아 다른 배타 어빌리티가 끊고 들어올 수 있다. */
	Recovery,
};

UCLASS(Abstract, BlueprintType, Blueprintable, PrioritizeCategories = ("Wx"))
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility, public IWxUIData
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	/**
	 * 쿨다운·코스트 수치는 이 행에서 읽는다.
	 * 코스트는 공용 GE(UWxEffect_Cost)가, 쿨다운은 어빌리티가 지정한 UWxEffect_Cooldown 파생 GE가 그 수치를 쓴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnTriggered;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UInputAction> ActivationInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	EWxAbilityActivationGroup ActivationGroup = EWxAbilityActivationGroup::Independent;

	/** 지금 열려 있는 캔슬 창. Exclusive일 때만 뜻이 있고, 활성화마다 Blocking에서 다시 시작한다. */
	EWxAbilityActionPhase ActionPhase = EWxAbilityActionPhase::Blocking;

	/**
	 * 활성 구간 동안 소유자에게 유지되는 효과. ActivationOwnedTags의 GE판으로, 활성화에서 걸고 종료에서 걷는다.
	 * 수명이 어빌리티에 묶이므로 각 GE는 지속시간을 두지 않는다(Infinite).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TSubclassOf<UGameplayEffect>> ActivationOwnedEffects;

	//~ Begin IWxUIData
	virtual FText GetTitle() const override;
	virtual FText GetDescription() const override;
	virtual TSoftObjectPtr<UObject> GetIcon() const override;
	//~ End IWxUIData

	int32 GetMaxRecharges() const;

	/** 충전 1개의 회복 시간(초). 테이블에 수치가 없으면 0 */
	float GetCooldownTime() const;

	/**
	 * 일반적으로는 ASPD가 반영된 몽타주 재생 속도 사용.
	 * 고정된 시간을 맞춰야하는 등 특수한 경우에는 1을 반환하도록 오버라이드한다.
	 */
	virtual float GetMontagePlayRate() const;

	void OpenComboWindow();
	void CloseComboWindow();

	/**
	 * 본동작이 걸고 있던 발동 그룹 잠금을 풀어서 그 순간부터 이후 발동하는 Exclusive 어빌리티에 의한 캔슬을 허용한다.
	 * 코스트·쿨다운·ActivationBlockedTags는 그대로 검사한다.
	 */
	void StartRecovery();

	/**
	 * 점유자(후딜에 들지 않은 Exclusive·Override) 중 Candidate의 발동을 막는 첫 어빌리티. 없으면 nullptr.
	 * Candidate가 없으면 점유자 존재 여부를 묻는 것으로 보아 첫 점유자를 반환한다.
	 * Override는 서로를 끊지 않아 점유가 둘 이상일 수 있으므로, 통과하려면 점유자 전원을 지나야 한다.
	 */
	static const UWxAbilityBase* FindActivationGroupBlocker(const UAbilitySystemComponent& ASC, const UWxAbilityBase* Candidate = nullptr);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CanBeCanceled() const override;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

#if WITH_EDITOR
	//~ Begin UObject
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	//~ End UObject
#endif

	/** 테이블에 쿨다운 수치가 없으면 nullptr — 호출자들이 이것을 "쿨다운 없음" 게이트로 쓴다. */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** 남은 충전이 있으면 쿨다운 태그가 붙어 있어도 통과시킨다. (MaxRecharges) */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage;
	
	TArray<FActiveGameplayEffectHandle> ActivationOwnedEffectHandles;
};
