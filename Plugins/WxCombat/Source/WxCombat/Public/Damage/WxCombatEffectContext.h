// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "WxCombatEffectContext.generated.h"

UENUM()
enum class EWxDamageResult : uint8
{
	/** ExecCalc가 돌지 않았거나 판정 전에 빠져나갔다(팀 등) — 연출 대상이 아니다 */
	None,

	/** 무적으로 무효화 */
	Evaded,

	PerfectGuard,

	/** 일반 가드로 감소된 경우를 포함 */
	Damaged
};

/**
 * 대미지 판정 결과를 실어 나르는 EffectContext.
 *
 * UWxExecCalc_Damage가 계산 중에 쓰고, UWxAbilitySystemComponent::HandleGameplayEffectAppliedToSelf가 GE 적용이 끝난 뒤 읽어 Cue·이벤트를 발행한다.
 * 둘 사이의 유일한 연결 통로다.
 *
 * 클라이언트 예측 경로는 ExecCalc를 건너뛰므로 DamageResult가 None으로 남고, 발행 측이 연출을 건너뛴다.
 * 히트스톱만은 이 결과를 쓰지 않고 UWxExecCalc_Damage::CheckDamage를 직접 돌려, 클라이언트도 서버와 같은 결론에 이른다.
 *
 * 하나의 컨텍스트가 대미지 GE와 AdditionalEffects 전체에 공유된다(FWxDamageInfo::MakeSpecs).
 * 판정 결과는 대미지 GE 한 발의 것이므로, 발행 측은 GE 종류로 한 번 더 거른다.
 *
 * 할당은 UWxAbilitySystemGlobals가 맡으므로 MakeEffectContext를 타는 모든 GE가 이 타입을 갖는다.
 */
USTRUCT()
struct WXCOMBAT_API FWxCombatEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FGameplayEffectContext* Duplicate() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	EWxDamageResult GetDamageResult() const;
	bool IsCritical() const;

	/** Damaged면 대상이 받은 최종 대미지, PerfectGuard면 공격자에게 반사한 양 */
	float GetFinalDamage() const;

	/** Damaged일 때 피격자에게 보낼 반응 이벤트 태그. 무효면 미발송. */
	const FGameplayTag& GetHitReactTag() const;

	/** 컨텍스트가 여러 스펙에 재사용되므로 판정을 새로 시작하기 전에 호출해야 한다 */
	void ClearDamageResult();

	void SetEvaded();
	void SetPerfectGuard(float ReflectAmount);
	void SetDamaged(float InFinalDamage, bool bIsCritical, const FGameplayTag& InHitReactTag);

protected:
	UPROPERTY()
	EWxDamageResult DamageResult = EWxDamageResult::None;

	UPROPERTY()
	bool bCritical = false;

	UPROPERTY()
	float FinalDamage = 0.f;

	UPROPERTY()
	FGameplayTag HitReactTag;
};

template<>
struct TStructOpsTypeTraits<FWxCombatEffectContext> : public TStructOpsTypeTraitsBase2<FWxCombatEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
