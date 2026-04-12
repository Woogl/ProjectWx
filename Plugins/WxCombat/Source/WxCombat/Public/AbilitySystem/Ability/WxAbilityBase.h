// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UWxEffect_Cooldown;
struct FWxAbilityTableRow;

/** 어빌리티 활성화 정책 */
UENUM(BlueprintType)
enum class EWxAbilityActivationPolicy : uint8
{
	/** 입력 트리거 시 활성화 */
	OnInputTriggered,
	/** 부여(Grant)될 때 즉시 자동 활성화 (패시브, 상시 버프 등) */
	OnGranted,
};

/**
 * 프로젝트 전체 어빌리티 베이스 클래스.
 * 모든 어빌리티는 이 클래스를 상속받아 작성.
 *
 * 쿨다운은 CooldownTime, MaxCharges 프로퍼티로 설정한다.
 * 내부적으로 공용 UWxEffect_Cooldown GE를 사용하며,
 * 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 *
 * AbilityDataRow가 설정되어 있으면 어빌리티 부여 시 테이블 Row에서 수치를 읽어온다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (PrioritizeCategories = "Wx"))
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnInputTriggered;

	/** 이 어빌리티를 활성화할 입력 태그. GiveAbility 시 DynamicAbilityTags에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx", meta = (Categories = "Input"))
	FGameplayTag ActivationInputTag;

	/** 어빌리티 아이콘. UI에서 표시 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	/** 어빌리티 수치 데이터테이블 Row 참조. 설정 시 OnGiveAbility에서 테이블 값으로 덮어쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;

	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown")
	float CooldownTime = 0.f;

	/** 최대 충전 수. 1이면 단일 쿨다운 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown")
	int32 MaxCharges = 1;

	/** MP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost")
	float MPCost = 0.f;

	/** UP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost")
	float UPCost = 0.f;

protected:
	/** 테이블 Row 데이터를 내부 변수에 적용한다 */
	virtual void ApplyAbilityTableRow(const FWxAbilityTableRow& Row);
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티 발동 시 자신에게 적용할 GameplayEffect 목록 (버프, 상태 부여 등). 각 GE의 Duration 정책에 따라 자연 만료된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<TSubclassOf<UGameplayEffect>> OnActivateEffects;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	/**
	 * GetCooldownGameplayEffect()가 반환하는 GE 인스턴스.
	 * ViewModel이 GetClass()로 쿨다운 GE 클래스를, StackLimitCount로 MaxCharges를 읽는다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cooldown> CooldownEffect;
};
