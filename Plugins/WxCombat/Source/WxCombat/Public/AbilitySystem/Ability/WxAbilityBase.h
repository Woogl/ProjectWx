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
class UAbilityTask_WaitGameplayEvent;
class AWxProjectileBase;
class UInputAction;
class UTexture2D;
struct FWxAbilityTableRow;

/** 어빌리티 활성화 정책 */
UENUM(BlueprintType)
enum class EWxAbilityActivationPolicy : uint8
{
	/** 트리거(입력·이벤트·AI)를 기다려 활성화 */
	OnTriggered,
	/** 부여(Grant)될 때 즉시 자동 활성화 (패시브, 상시 버프 등) */
	OnGranted,
};

/**
 * 프로젝트 전체 어빌리티 베이스 클래스.
 * 모든 어빌리티는 이 클래스를 상속받아 작성.
 *
 * 쿨다운/코스트 수치는 AbilityDataRow(FWxAbilityTableRow)에서만 읽는다. 필요한 시점마다 GetTableRow()로 Row를 해석해 온디맨드로 사용한다.
 * 쿨다운은 공용 UWxEffect_Cooldown GE를 사용하며, 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 * Duration(CooldownTime + 직렬 회복분)은 UWxEffect_Cooldown의 MMC가 AbilityDataRow에서 조회해 계산하므로 ApplyCooldown 오버라이드는 없다(엔진 순정 경로가 적용).
 * 소모된 충전 1개당 GE 1개가 활성화되고, 기존 GE는 제거하지 않고 자연 만료로 충전을 회복한다. CheckCooldown이 활성 GE 수를 MaxRecharges와 비교해 다중 충전을 판정한다.
 *
 * 코스트는 공용 UWxEffect_Cost GE가 MP·UP 모디파이어를 정적으로 선언하고 각 MMC가 AbilityDataRow에서 MPCost/UPCost를 조회하므로, 검사·적용 모두 엔진 순정 경로를 그대로 사용한다(코스트 관련 오버라이드 없음).
 *
 * CooldownGameplayEffectClass/CostGameplayEffectClass의 기본값은 각 공용 GE(UWxEffect_Cooldown/UWxEffect_Cost)를 가리키는 "마커"다.
 * 마커 그대로면 프로젝트 방식(AbilityDataRow 기반), 다른 GE로 바꾸면 그 어빌리티는 엔진 순정 GE 경로를 쓴다(AbilityDataRow와 상호배타). 디테일 패널에서 어느 쪽인지 바로 읽힌다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWxAbilityBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
	EWxAbilityActivationPolicy ActivationPolicy = EWxAbilityActivationPolicy::OnTriggered;

	/**
	 * 이 어빌리티를 발동시키는 입력 액션(플레이어 입력 라우팅 키).
	 * ASC가 눌린 InputAction을 IsActivationInput으로 대조해 어빌리티를 활성화한다.
	 * 입력으로 발동하지 않는 어빌리티(AI 패턴, 반응형, 패시브)는 비워둔다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> ActivationInputAction;

	/**
	 * 눌린 InputAction이 이 어빌리티를 발동시키는 입력인지 반환한다. 기본은 ActivationInputAction 일치.
	 * 복수 발동 입력을 갖는 어빌리티(예: 약/강 공격)가 override로 확장한다.
	 */
	virtual bool IsActivationInput(const UInputAction* Action) const;

	/**
	 * 이 어빌리티가 바인딩을 요구하는 입력 액션 전부를 OutActions에 더한다(중복은 AddUnique). 기본은 ActivationInputAction 하나.
	 * 플레이어 입력 바인딩이 이 목록으로 EnhancedInput에 어떤 액션을 등록할지 결정한다(발동/관찰 판정과 별개, 바인딩 열거용).
	 * 복수 입력을 갖는 어빌리티(예: 약/강 공격, 가드/회피의 반격)가 override로 확장한다.
	 */
	virtual void GetInputActions(TArray<const UInputAction*>& OutActions) const;

	/** 어빌리티 데이터(쿨다운·충전·코스트·아이콘) 데이터테이블 Row 참조. 쿨다운/코스트/아이콘 접근자가 이 Row에서 값을 읽는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;

	/** UI 표시 아이콘(텍스처 또는 머터리얼)의 소프트 참조. AbilityDataRow에서 읽으며 로드하지 않는다(소비자가 비동기 로드). 행/아이콘 미설정 시 null 소프트. */
	TSoftObjectPtr<UObject> GetIcon() const;

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

	/**
	 * 재생 중인 이 어빌리티가 확정(서버)에서 투사체를 스폰한다. WxAnimNotify_SpawnProjectile 가 위임한다.
	 * 아바타 SkeletalMesh 의 SpawnSocketName 소켓 위치 + 아바타 회전으로 스폰한다. 대미지는 투사체 클래스가 저작한다.
	 * 투사체는 서버가 스폰해 복제하므로 클라(예측 인스턴스)에선 authority 게이트로 무동작.
	 */
	void SpawnProjectile(TSubclassOf<AWxProjectileBase> ProjectileClass, FName SpawnSocketName) const;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * 엔진 순정 구현은 쿨다운 GE의 GrantedTags 쿼리 기반이라, 태그를 부여하지 않는 공용 쿨다운 GE에서는 항상 0을 반환한다.
	 * CDO 기반 쿼리로 대체해 순정 API(BP 노드 포함) 호출자가 올바른 값을 받게 한다.
	 * 다중 충전 시 모든 충전이 회복되는 시점까지의 시간을 반환한다(순정의 최장 잔여시간 의미와 동일).
	 */
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	/**
	 * 이 어빌리티(소스 CDO 기준)가 적용한 활성 쿨다운 GE를 집계한다. 활성 GE 1개 = 회복 대기 중인 충전 1개.
	 * 가장 늦게 만료되는 GE의 잔여시간과 전체 지속시간을 출력 인자로 채우고, 활성 GE 수를 반환한다.
	 * CheckCooldown·GetCooldownTimeRemaining과 Duration MMC(UWxMMC_CooldownDuration)가 공유한다.
	 */
	int32 QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const;

protected:
	/** 히트스톱 복원 타이머를 정리한다. 몽타주로 대미지를 주는 파생 어빌리티는 Super 체인으로 이 정리를 받는다. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	/**
	 * 히트스톱(역경직) 이벤트 리스너를 시작한다. 몽타주로 대미지를 주는 어빌리티가 ActivateAbility에서 opt-in 호출한다.
	 * Event.HitStop 수신 시 재생 중인 자기 몽타주 재생률을 잠깐 0 근처로 낮췄다가 GetMontagePlayRate()로 복원한다.
	 */
	void StartHitStopListener();

private:
	/** AbilityDataRow가 가리키는 수치 Row를 해석해 반환한다. 미설정/무효면 nullptr. */
	const FWxAbilityTableRow* GetTableRow() const;

	/**
	 * GetCooldownGameplayEffect()가 다중 충전 어빌리티에서 반환하는 GE 인스턴스.
	 * ViewModel이 StackLimitCount로 최대 충전 수를 읽는다. 단일 충전은 공유 CDO를 반환하므로 이 인스턴스를 만들지 않는다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cooldown> CooldownEffect;

	/** Event.HitStop 수신 시 재생 중인 몽타주를 정지시키고 복원 타이머를 (재)예약한다 */
	UFUNCTION()
	void HandleHitStopEvent(FGameplayEventData Payload);

	/** 히트스톱 복원. 몽타주 재생률을 GetMontagePlayRate()(ASPD 반영)로 되돌린다 */
	void ResumeFromHitStop();

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> HitStopListenerTask;

	FTimerHandle HitStopResumeTimer;
};
