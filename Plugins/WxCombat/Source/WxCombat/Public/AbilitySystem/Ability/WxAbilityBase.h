// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UWxEffect_Cooldown;
class UWxEffect_Cost;
class UWxAbilityComponent;
struct FWxAbilityTableRow;

/** 어빌리티 발동 시 적용할 GameplayEffect 항목 */
USTRUCT(BlueprintType)
struct FWxAbilityEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "SetByCaller"))
	TMap<FGameplayTag, float> SetByCallers;
};

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
 * 쿨다운은 CooldownTime, MaxRecharges 프로퍼티로 설정한다.
 * 내부적으로 공용 UWxEffect_Cooldown GE를 사용하며,
 * 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 * 소모된 충전 1개당 GE 1개를 적용하고, 기존 GE는 제거하지 않고 자연 만료로 충전을 회복한다.
 *
 * 코스트는 MPCost, UPCost 프로퍼티로 설정한다.
 * GetCostGameplayEffect()가 공용 UWxEffect_Cost GE에 모디파이어를 채워 반환하므로 검사는 엔진 순정 CheckCost를 그대로 사용한다.
 * ApplyCost는 엔진이 GE의 GetClass() CDO로 스펙을 다시 만드는 탓에 런타임 구성 인스턴스가 무시되어, 인스턴스 Def로 스펙을 만드는 얇은 오버라이드만 둔다.
 *
 * AbilityDataRow가 설정되어 있으면 어빌리티 부여 시 테이블 Row에서 수치를 읽어온다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnInputTriggered;

	/**
	 * 어빌리티에 부착된 컴포넌트 모음. 필요한 어빌리티에서만 BP 디테일 패널(Wx)에서 EditInline 으로 추가한다.
	 * 표시 데이터 등은 도메인별 UWxAbilityComponent 파생 클래스로 부착한다.
	 * (예: UI 아이콘은 WxUI 의 UWxAbilityComponent_UIData)
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Wx")
	TArray<TObjectPtr<UWxAbilityComponent>> Components;

	/** 첫 번째로 일치하는 컴포넌트 포인터 반환. 없으면 nullptr. */
	const UWxAbilityComponent* FindComponent(TSubclassOf<UWxAbilityComponent> ComponentClass) const;

	template <typename T>
	const T* FindComponent() const
	{
		static_assert(TIsDerivedFrom<T, UWxAbilityComponent>::IsDerived, "T must derive from UWxAbilityComponent");
		return Cast<T>(FindComponent(T::StaticClass()));
	}

	/** 어빌리티 수치 데이터테이블 Row 참조. 설정 시 OnGiveAbility에서 테이블 값으로 덮어쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;

	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown", meta = (ClampMin = "0"))
	float CooldownTime = 0.f;

	/** 최대 충전 수. 1이면 단일 쿨다운 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cooldown", meta = (ClampMin = "1"))
	int32 MaxRecharges = 1;

	/** MP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost", meta = (ClampMin = "0"))
	float MPCost = 0.f;

	/** UP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost", meta = (ClampMin = "0"))
	float UPCost = 0.f;

	/**
	 * 현재 아바타의 ASPD가 반영된 몽타주 재생 속도. ASC/AttributeSet 미가용 시 1.0
	 * 각 어빌리티는 PlayMontage 호출 시 PlayRate 인자로 이 값을 사용한다.
	 */
	float GetMontagePlayRate() const;

	/**
	 * 후딜레이 구간 진입. 이 프로젝트에서 후딜레이 구간 = 캔슬 가능 구간이다.
	 * 이 어빌리티가 건 하드 차단(BlockAbilitiesWithTag)을 해제해, 후딜 동안 평소 막히던 어빌리티로 캔슬 진입할 수 있게 한다.
	 * 진입한 어빌리티는 자신의 CancelAbilitiesWithTag(또는 동일 슬롯 몽타주 인터럽트)로 이 어빌리티를 끊는다.
	 *
	 * 복원하지 않는다 — 후딜은 몽타주의 마지막 구간이므로 한 번 진입하면 어빌리티 종료까지 캔슬 가능 상태로 두고, 차단은 어빌리티 종료 시 엔진이 자연히 되돌린다.
	 * 비용/쿨다운/ActivationBlockedTags는 그대로 검사되므로 못 쓰는 입력은 후딜을 끊지 못한다. BlockAbilitiesWithTag가 비어 있으면(예: Attack) 무효과.
	 */
	void StartRecovery();

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/**
	 * 엔진 순정 구현은 쿨다운 GE의 GrantedTags 쿼리 기반이라, 태그를 부여하지 않는 공용 쿨다운 GE에서는 항상 0을 반환한다.
	 * CDO 기반 쿼리로 대체해 순정 API(BP 노드 포함) 호출자가 올바른 값을 받게 한다.
	 * 다중 충전 시 모든 충전이 회복되는 시점까지의 시간을 반환한다(순정의 최장 잔여시간 의미와 동일).
	 */
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/** 테이블 Row 데이터를 내부 변수에 적용한다 */
	virtual void ApplyAbilityTableRow(const FWxAbilityTableRow& Row);
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 어빌리티 발동 시 자신에게 적용할 GameplayEffect 목록 (버프, 상태 부여 등). 각 GE의 Duration 정책에 따라 자연 만료된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TArray<FWxAbilityEffect> OnActivateEffects;

private:
	/**
	 * 이 어빌리티(소스 CDO 기준)가 적용한 활성 쿨다운 GE를 집계한다. 활성 GE 1개 = 회복 대기 중인 충전 1개.
	 * 가장 늦게 만료되는 GE의 잔여시간과 전체 지속시간을 출력 인자로 채우고, 활성 GE 수를 반환한다.
	 */
	int32 QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const;

	/**
	 * GetCooldownGameplayEffect()가 반환하는 GE 인스턴스.
	 * ViewModel이 GetClass()로 쿨다운 GE 클래스를, StackLimitCount로 MaxRecharges를 읽는다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cooldown> CooldownEffect;

	/**
	 * GetCostGameplayEffect()가 반환하는 GE 인스턴스.
	 * Modifiers는 호출 시점의 MPCost/UPCost로 갱신된다. ViewModel이 Modifiers에서 코스트 어트리뷰트를 읽는다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cost> CostEffect;
};
