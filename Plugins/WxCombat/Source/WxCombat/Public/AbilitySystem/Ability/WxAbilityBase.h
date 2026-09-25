// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "WxUIData.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UGameplayEffect;
class UInputAction;

UENUM(BlueprintType)
enum class EWxAbilityCostResource : uint8
{
	Custom,
	SP,
	MP,
	UP,
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

/**
 * 어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 파생 클래스가 타입이고, 생성자에서 태그 관계와 발동 그룹 같은 규칙 기본값을 정한다.
 * GA_는 몽타주·입력·수치·표시를 채운다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility, public IWxUIData
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	/** 착지할 때 이동 컴포넌트가 재생 중인 몽타주를 이 섹션으로 옮긴다. 공중에서 도는 몽타주(공중 공격·넉업)가 쓴다. */
	static const FName LandingSectionName;

#if WITH_EDITOR
	/** 쿨다운 시간에 쿨다운 GE가 있는지 본다. GA_를 저장할 때 엔진이 CDO에 대고 부른다. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** 같은 입력을 쓰는 두 어빌리티가 함께 발동 조건을 만족할 수 없는지. 한쪽이 요구하는 태그를 다른 쪽이 막으면 배타적이다. 엔진이 태그 조건을 protected로 두어 판정을 여기서 한다. */
	bool IsActivationExclusive(const UWxAbilityBase& Other) const;

	/** 같은 쿨다운 GE를 쓰는지. 엔진이 쿨다운 GE를 protected로 두어 판정을 여기서 한다. */
	bool SharesCooldownGroup(const UWxAbilityBase& Other) const;
#endif

	/** AI·이벤트로만 발동하면 비운다. 같은 입력의 어빌리티가 여럿이면 세트 순서대로 시도해 처음 성공한 것을 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UInputAction> ActivationInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	EWxAbilityActivationGroup ActivationGroup = EWxAbilityActivationGroup::Independent;

	/** 지금 열려 있는 캔슬 창. Exclusive일 때만 뜻이 있고, 활성화마다 Blocking에서 다시 시작한다. */
	EWxAbilityActionPhase GetActionPhase() const;

	/**
	 * 활성 구간 동안 소유자에게 유지되는 효과. ActivationOwnedTags의 GE판으로, 활성화에서 걸고 종료에서 걷는다.
	 * 수명이 어빌리티에 묶이므로 각 GE는 지속시간을 두지 않는다(Infinite).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TSubclassOf<UGameplayEffect>> ActivationOwnedEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost")
	EWxAbilityCostResource CostResource = EWxAbilityCostResource::Custom;

	/** 질주처럼 지속 소모하는 어빌리티에서는 진입 비용이다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost", meta = (ClampMin = "0.0"))
	float CostAmount = 0.f;

	//~ Begin IWxUIData
	virtual FText GetTitle() const override;
	virtual FText GetDescription() const override;
	virtual TSoftObjectPtr<UObject> GetIcon() const override;
	virtual int32 GetMaxRecharges() const override;
	//~ End IWxUIData

	/** 충전 1개의 회복 시간(초). */
	float GetCooldownTime() const;

	/** 변형은 섹션으로 나눈다. */
	UAnimMontage* GetMontage() const;

	/** 일반적으로는 ASPD가 반영된 몽타주 재생 속도 사용. */
	virtual float GetMontagePlayRate() const;

	/** 노티파이를 보낸 몽타주 인스턴스가 지금 재생 중인 것이 아니면 무시한다 — 끊긴 앞 단 몽타주도 블렌드아웃 동안 노티파이를 보낸다. */
	void OpenComboWindow(int32 MontageInstanceID);
	void CloseComboWindow(int32 MontageInstanceID);

	/**
	 * 본동작이 걸고 있던 발동 그룹 잠금을 풀어서 그 순간부터 이후 발동하는 Exclusive 어빌리티에 의한 캔슬을 허용한다.
	 * 코스트·쿨다운·ActivationBlockedTags는 그대로 검사한다.
	 */
	void StartRecovery(int32 MontageInstanceID);

	/**
	 * 점유자(후딜에 들지 않은 Exclusive·Override) 중 Candidate의 발동을 막는 첫 어빌리티. 없으면 nullptr.
	 * Candidate가 없으면 점유자 존재 여부를 묻는 것으로 보아 첫 점유자를 반환한다.
	 * Override는 서로를 끊지 않아 점유가 둘 이상일 수 있으므로, 통과하려면 점유자 전원을 지나야 한다.
	 */
	static const UWxAbilityBase* FindActivationGroupBlocker(const UAbilitySystemComponent& ASC, const UWxAbilityBase* Candidate = nullptr);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 이 어빌리티가 선언한 진입 태그 조건은 처음 발동에서만 묻는다 — 콤보 창의 재발동은 이미 성립한 액션의 다음 단이다. 효과가 건 어빌리티 차단은 그 창에서도 유효하다. */
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual bool CanBeCanceled() const override;

	/** UWxEffect_Cooldown 파생 GE는 쿨다운 시간이 없으면 nullptr — 호출자들이 이것을 "쿨다운 없음" 게이트로 쓴다. */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** 남은 충전이 있으면 쿨다운 태그가 붙어 있어도 통과시킨다. (MaxRecharges) */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 소유자에게 Effect.IgnoreCosts가 있으면 무조건 통과한다. */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 소유자에게 Effect.IgnoreCosts가 있으면 코스트를 치르지 않는다. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 몽타주에 없는 시작 섹션이면 경고하고 실패한다. 엔진은 그런 섹션을 무시하고 처음부터 재생한다. */
	bool PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

	/** 콤보·패턴의 단계는 번호 섹션(1, 2, …)이다. 번호 섹션이 없는 몽타주는 처음부터 한 단계로 본다. */
	static int32 GetComboStageCount(const UAnimMontage* Montage);

	/** 번호 섹션이 없는 몽타주면 처음부터 재생하도록 NAME_None을 돌려준다. */
	static FName GetComboStageSection(const UAnimMontage* Montage, int32 StageIndex);

	/** 창이 닫힌 뒤의 발동은 첫 단부터 시작해야 한다. */
	virtual void OnComboWindowClosed();

	UFUNCTION()
	virtual void HandleMontageCompleted();

	UFUNCTION()
	virtual void HandleMontageBlendOut();

	UFUNCTION()
	virtual void HandleMontageInterrupted();

	UFUNCTION()
	virtual void HandleMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UAnimMontage> AbilityMontage;

	/** 0 이하이면 쿨다운 미적용. 쿨다운 GE(CooldownGameplayEffectClass)는 UWxEffect_Cooldown 파생이어야 한다. 같은 GE를 쓰는 어빌리티끼리 쿨다운을 나눠 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cooldown")
	float CooldownTime = 0.f;

	/** 1이면 단일 쿨다운 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cooldown")
	int32 MaxRecharges = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display")
	FText Title;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;

private:
	void SetActionPhase(EWxAbilityActionPhase NewPhase);
	EWxAbilityActionPhase ActionPhase = EWxAbilityActionPhase::Blocking;

	bool IsPlayingMontageInstance(int32 MontageInstanceID) const;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	TArray<FActiveGameplayEffectHandle> ActivationOwnedEffectHandles;
};
