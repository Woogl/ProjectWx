// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "WxAbilityBase.generated.h"

class UAbilitySystemComponent;
class UAbilityTask_PlayMontageAndWait;
class UWxAbilityTask_MontageEvents;
class UAnimMontage;
class UGameplayEffect;
class UInputAction;
struct FGameplayAbilityTargetDataHandle;
struct FGameplayAttribute;

/** 로컬 +X(정면)에서 +Y(오른쪽)으로 45도씩 나눈 8방향. 항목명이 몽타주 섹션 이름이다. */
UENUM(BlueprintType)
enum class EWxAbilityDirection : uint8
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

UENUM(BlueprintType)
enum class EWxAbilityCostResource : uint8
{
	Custom,
	SP,
	MP,
	UP,
};

/**
 * 어빌리티 하나는 데이터 전용 GA_ 하나다. C++ 파생 클래스가 타입이고, 생성자에서 식별 태그와 차단·취소·발동 조건 같은 태그 관계의 기본값을 정한다.
 * GA_는 몽타주·입력·수치·표시를 채운다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXGAME_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	/** 착지할 때 이동 컴포넌트가 재생 중인 몽타주를 이 섹션으로 옮긴다. 공중에서 도는 몽타주(공중 공격·넉업)가 쓴다. */
	static const FName LandingSectionName;

#if WITH_EDITOR
	/**
	 * 같은 입력을 쓰는 두 어빌리티가 함께 발동 조건을 만족할 수 없는지. 한쪽이 요구하는 태그를 다른 쪽이 막으면 배타적이다.
	 * 엔진이 태그 조건을 protected로 두어 판정을 여기서 한다.
	 */
	bool IsActivationExclusive(const UWxAbilityBase& Other) const;
#endif

	/** AI·이벤트로만 발동하면 비운다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UInputAction> ActivationInputAction;

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

	/**
	 * 비용 GE에서 속성이 유효하고 계산값이 0이 아닌 첫 항목의 속성과 양의 크기를 돌려주고, 없으면 무효 속성과 0을 돌려준다.
	 * 복수 비용을 합산하지 않고, Effect.IgnoreCosts 면제와 무관하게 정의된 비용을 돌려준다.
	 */
	float QueryCost(const UAbilitySystemComponent& ASC, FGameplayAttribute& OutCostAttribute) const;

	FText GetTitle() const;
	FText GetDescription() const;
	TSoftObjectPtr<UObject> GetIcon() const;

	/** 쿨다운 GE의 스택 상한이 충전 수다. 쌓이지 않는 쿨다운이나 쿨다운이 없으면 1이다. */
	int32 GetMaxRecharges() const;

	/** 방향별 변형은 각 몽타주의 섹션으로 나눈다. */
	virtual UAnimMontage* GetMontage() const;

	/** 로컬 XY 방향을 8방향으로 나눈다. 수평 입력이 없으면 DefaultDirection을 쓴다. */
	static EWxAbilityDirection ResolveDirection(const FVector& LocalDirection, EWxAbilityDirection DefaultDirection = EWxAbilityDirection::Forward);

	/** Prefix + EWxAbilityDirection 항목명 섹션이 없으면 Prefix + Forward, 그것도 없으면 NAME_None을 반환한다. */
	FName SelectDirectionalSection(const FVector& LocalDirection, const FString& Prefix = TEXT(""), EWxAbilityDirection DefaultDirection = EWxAbilityDirection::Forward) const;
	static FName SelectDirectionalSection(const UAnimMontage* Montage, const FVector& LocalDirection, const FString& Prefix = TEXT(""), EWxAbilityDirection DefaultDirection = EWxAbilityDirection::Forward);

	/** 정확한 SectionName 또는 접두사 SectionName에 Forward를 붙인 섹션이 있는지 검사한다. NAME_None은 빈 접두사다. */
	static bool HasMontageSection(const UAnimMontage* Montage, FName SectionName);

	virtual float GetMontagePlayRate() const;

	/**
	 * 액션의 후딜레이를 시작한다. 후딜은 엔진의 다른 어빌리티 차단을 끈 상태이고, 뒤이어 발동한 액션이 이 액션을 취소한다.
	 * 코스트·쿨다운·ActivationBlockedTags는 그대로 검사한다.
	 * 몽타주 이벤트 태스크가 이 어빌리티의 몽타주 인스턴스에서 온 노티파이인지 확인한 뒤 부른다.
	 */
	void StartRecovery();

	/** 취소 대상의 차단 기여와 IgnoreAbilityActivationTags를 반영한다. */
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 소유자 태그만으로 본 발동 조건(ActivationRequiredTags·ActivationBlockedTags). 재생 중인 액션의 차단은 보지 않아 같은 슬롯의 후보를 고르는 데 쓴다. */
	bool DoesOwnerSatisfyActivationTags(const UAbilitySystemComponent& AbilitySystemComponent) const;

	/** 순정 판정은 쿨다운 태그가 붙어 있기만 하면 막으므로, 쿨다운 GE의 스택이 충전 수보다 적으면 통과시킨다. */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 소유자에게 Effect.IgnoreCosts가 있으면 무조건 통과한다. */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 소유자에게 Effect.IgnoreCosts가 있으면 코스트를 치르지 않는다. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/** 새 동작을 시작할 때 방향 입력을 비우고, 후딜에서 이어진 액션이면 본동작의 차단을 다시 건다. */
	void ResetActionState();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * StartSection이 비었거나 존재하지 않고 [StartSection]Forward가 있으면 동작 시작에 확정한 이동 입력 방향으로 섹션을 자동 선택한다.
	 * LocalPredicted·ServerInitiated에서는 로컬 클라이언트가 방향을 보내고 원격 플레이어의 서버 실행은 수신을 기다린다.
	 * 수신을 기다리는 동안에도 true(요청 접수)를 반환하고, 나중에 재생이 실패하면 어빌리티를 취소한다.
	 */
	bool PlayMontage(UAnimMontage* Montage, FName StartSection = NAME_None);

	/** 방향 선택 이후의 재생 수명. 그로기는 태스크 종료 대신 GP와 폴링으로 수명을 관리한다. */
	virtual bool PlayMontageInternal(UAnimMontage* Montage, FName StartSection);

	/** PlayMontage가 방향 섹션을 자동 선택할 때, 동기화한 로컬 입력 방향으로 섹션을 고른다. */
	virtual FName SelectInputDirectionSection(const UAnimMontage* Montage, const FString& Prefix, const FVector& LocalDirection);

	UFUNCTION()
	virtual void HandleMontageCompleted();

	UFUNCTION()
	virtual void HandleMontageBlendOut();

	UFUNCTION()
	virtual void HandleMontageInterrupted();

	UFUNCTION()
	virtual void HandleMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Montage")
	TObjectPtr<UAnimMontage> AbilityMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display")
	FText Title;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWxAbilityTask_MontageEvents> MontageEventsTask;

	/** 액션의 다른 어빌리티 차단을 켜고 끄며, 비활성이거나 값이 같으면 엔진 SetShouldBlockOtherAbilities가 무시한다. */
	void SetActionBlocking(bool bBlocking);

	FVector GetLocalMontageInputDirection() const;
	void HandleMontageDirectionReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ApplicationTag);
	void ClearPendingDirectionalMontage();

	// 동작 시작에 확정해 후속 섹션도 서버와 같은 로컬 좌표를 사용한다.
	FVector MontageInputDirection = FVector::ZeroVector;
	bool bHasMontageInputDirection = false;
	FDelegateHandle MontageDirectionHandle;
	FName PendingDirectionalSection;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingDirectionalMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	TArray<FActiveGameplayEffectHandle> ActivationOwnedEffectHandles;
};
