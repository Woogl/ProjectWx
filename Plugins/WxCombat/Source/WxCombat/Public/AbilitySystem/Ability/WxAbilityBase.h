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
class AWxProjectileBase;
class UInputAction;
class UTexture2D;
struct FWxAbilityTableRow;

UENUM(BlueprintType)
enum class EWxAbilityActivationPolicy : uint8
{
	/** 트리거(입력·이벤트·AI)를 기다려 활성화 */
	OnTriggered,
	/** 부여(Grant)될 때 즉시 자동 활성화 (패시브, 상시 버프 등) */
	OnGranted,
};

/**
 * 쿨다운·코스트 수치는 AbilityDataRow에서만 읽는다.
 * 쿨다운은 공용 UWxEffect_Cooldown GE를 소스 어빌리티 CDO로 구분하며, 소모된 충전 1개당 GE 1개가 붙어 자연 만료로 회복된다.
 * Duration과 자원 모디파이어는 각 MMC가 Row에서 조회하므로 적용 경로는 엔진 순정 그대로다.
 *
 * CooldownGameplayEffectClass/CostGameplayEffectClass의 기본값은 그 공용 GE를 가리키는 "마커"다.
 * 마커 그대로면 Row 기반, 다른 GE로 바꾸면 엔진 순정 GE 경로를 쓴다(Row와 상호배타).
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
	 * ASC가 눌린 액션을 이 값과 대조하고, AbilitySet이 모아 입력 바인딩 목록을 만든다.
	 * 입력으로 발동하지 않는 어빌리티(AI 패턴·반응형·패시브)는 비워둔다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> ActivationInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (RowType = "/Script/WxCombat.WxAbilityTableRow"))
	FDataTableRowHandle AbilityDataRow;

	/**
	 * UI 표시 아이콘(텍스처 또는 머터리얼)의 소프트 참조.
	 * 로드하지 않으므로 소비자가 비동기로 로드한다.
	 */
	TSoftObjectPtr<UObject> GetIcon() const;

	/** 아바타 ASPD가 반영된 몽타주 재생 속도 */
	float GetMontagePlayRate() const;

	/**
	 * 이 프로젝트에서 후딜레이 구간 = 캔슬 가능 구간이다.
	 * 이 어빌리티가 건 하드 차단(BlockAbilitiesWithTag)을 해제해, 평소 막히던 어빌리티로 캔슬 진입할 수 있게 한다.
	 * 진입한 어빌리티는 자신의 CancelAbilitiesWithTag(또는 동일 슬롯 몽타주 인터럽트)로 이 어빌리티를 끊는다.
	 *
	 * 복원하지 않는다 — 후딜은 몽타주의 마지막 구간이므로 한 번 진입하면 종료까지 캔슬 가능 상태로 둔다.
	 * 코스트·쿨다운·ActivationBlockedTags는 그대로 검사되고, 차단을 걸지 않는 어빌리티(스프린트·상호작용 등)에서는 무효과.
	 */
	void StartRecovery();

	/**
	 * WxAnimNotify_SpawnProjectile이 위임하는 투사체 스폰.
	 * 서버에서만 스폰해 복제로 전파한다.
	 * 아바타 SkeletalMesh의 SpawnSocketName 소켓 위치 + 아바타 회전으로 스폰하며, 대미지는 투사체 클래스가 저작한다.
	 */
	void SpawnProjectile(TSubclassOf<AWxProjectileBase> ProjectileClass, FName SpawnSocketName) const;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * 엔진 순정 구현은 쿨다운 GE의 GrantedTags 쿼리 기반이라, 태그를 부여하지 않는 공용 쿨다운 GE에서는 항상 0을 반환한다.
	 * CDO 기반 쿼리로 대체해 순정 API(BP 노드 포함) 호출자가 올바른 값을 받게 한다.
	 */
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	/**
	 * 이 어빌리티(소스 CDO 기준)가 적용한 활성 쿨다운 GE 수를 반환한다.
	 * 출력 인자는 가장 늦게 만료되는 GE 기준이다.
	 */
	int32 QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const;

private:
	const FWxAbilityTableRow* GetTableRow() const;

	/**
	 * 다중 충전 어빌리티에서만 만드는 GE 인스턴스.
	 * StackLimitCount에 최대 충전 수를 실어 ViewModel에 전달한다.
	 */
	UPROPERTY(Transient)
	mutable TObjectPtr<UWxEffect_Cooldown> CooldownEffect;
};
