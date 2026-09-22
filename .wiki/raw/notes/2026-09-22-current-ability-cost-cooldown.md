---
title: "어빌리티 비용·쿨다운 GE 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, combat]
summary: "어빌리티 비용·쿨다운 GameplayEffect의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: 60c324c714b1dab10cd48d36cabad63ace232716
---

# 어빌리티 비용·쿨다운 GE 정적 조사

조사일: 2026-09-22. 기준 HEAD: `60c324c714b1dab10cd48d36cabad63ace232716`, 대상 파일은 작업 트리 변경 없음. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Cost.h
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Cost.h>)
- SHA-256: `06fb6a07b44b33d5d75518e569f176efaeca2f54f7e50c468d01ded8c3f8f65e`
```text
...
11: /**
12:  * 값은 각 MMC가 계산 시점에 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow를 조회해 만든다.
13:  * CDO가 완전 자기완결이라 엔진 순정 CheckCost(CanApplyAttributeModifiers)/ApplyCost/GetCostGameplayEffect를 그대로 사용한다.
14:  *
15:  * 아울러 어빌리티 CostGameplayEffectClass의 "프로젝트 방식(테이블 기반)" 기본 클래스를 겸한다.
16:  */
17: UCLASS()
18: class WXCOMBAT_API UWxEffect_Cost : public UGameplayEffect
19: {
20: 	GENERATED_BODY()
21: 
22: public:
23: 	UWxEffect_Cost();
24: };
25: 
26: /**
27:  * MMC API가 평가 중인 Modifier 인덱스를 주지 않아, 자원별로 파생 클래스를 분리한다.
28:  * Row가 고른 자원과 일치하는 파생 클래스만 값을 내고 나머지는 0이다.
29:  */
30: UCLASS(Abstract)
31: class WXCOMBAT_API UWxMMC_Cost : public UGameplayModMagnitudeCalculation
32: {
33: 	GENERATED_BODY()
34: 
35: protected:
36: 	/** 자원 감산이라 CostAmount를 음수로 반환한다 */
37: 	float GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const;
38: };
...
```

## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp>)
- SHA-256: `0481db0869d6b42d99f9f79301ca86d40d806d2a7236f9a8620959927b1441fd`
```text
...
7: UWxEffect_Cost::UWxEffect_Cost()
8: {
9: 	DurationPolicy = EGameplayEffectDurationType::Instant;
10: 
11: 	// Additive를 쓰는 이유는 순정 CanApplyAttributeModifiers가 Additive 모디파이어만 보고 자원 부족을 판정하기 때문이다.
12: 	FGameplayModifierInfo MPModifier;
13: 	MPModifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
14: 	MPModifier.ModifierOp = EGameplayModOp::Additive;
15: 	FCustomCalculationBasedFloat MPCalc;
16: 	MPCalc.CalculationClassMagnitude = UWxMMC_MPCost::StaticClass();
17: 	MPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPCalc);
18: 	Modifiers.Add(MPModifier);
...
37: float UWxMMC_Cost::GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const
38: {
39: 	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
40: 	// 정적 데이터라 서버/클라 동일.
41: 	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
42: 	if (!Ability || Ability->AbilityDataRow.IsNull())
43: 	{
44: 		return 0.f;
45: 	}
46: 
47: 	const FWxAbilityTableRow* Row = Ability->AbilityDataRow.GetRow<FWxAbilityTableRow>(ANSI_TO_TCHAR(__FUNCTION__));
48: 	return Row && Row->CostResource == Resource ? -Row->CostAmount : 0.f;
49: }
...
```

## Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Cooldown.h
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Cooldown.h>)
- SHA-256: `53ed7387e20e520c9a445596a330f80e875f2f90f3801fd201188a324a06204c`
```text
...
12: /**
13:  * 충전형 쿨다운의 베이스.
14:  * 지속시간은 UWxMMC_CooldownDuration이 소스 어빌리티의 테이블 행에서 읽으므로, 엔진 순정 ApplyCooldown/쿨다운 조회 API를 그대로 쓴다.
15:  *
16:  * 소모한 충전 하나가 스택 하나다.
17:  * 스택이 더 쌓여도 진행 중인 회복은 건드리지 않고(NeverRefresh), 만료마다 스택 하나만 돌려주며 다음 회복을 시작한다(RemoveSingleStackAndRefreshDuration) — 충전이 직렬로 돌아온다.
18:  * 충전 상한은 GE가 아니라 테이블의 MaxRecharges이며 UWxAbilityBase::CheckCooldown이 판정한다.
19:  *
20:  * 엔진은 스택을 GE 클래스 단위로 병합한다.
21:  * 그래서 쿨다운을 따로 굴리는 어빌리티마다 파생 클래스를 하나씩 두고, 어빌리티가 CooldownGameplayEffectClass로 자기 것을 지정한다.
22:  * 파생 클래스가 부여하는 태그가 순정 쿨다운 API의 식별자다.
23:  */
24: UCLASS(Abstract)
25: class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
26: {
27: 	GENERATED_BODY()
28: 
29: public:
30: 	UWxEffect_Cooldown();
31: 
32: protected:
33: 	void GrantCooldownTag(const FGameplayTag& CooldownTag);
34: };
...
```

## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp>)
- SHA-256: `d314401e07a93d5aa29dad263f330def26781ea2c0a79ec470c4b204e23f12bd`
```text
...
9: UWxEffect_Cooldown::UWxEffect_Cooldown()
10: {
11: 	DurationPolicy = EGameplayEffectDurationType::HasDuration;
12: 
13: 	FCustomCalculationBasedFloat DurationCalc;
14: 	DurationCalc.CalculationClassMagnitude = UWxMMC_CooldownDuration::StaticClass();
15: 	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCalc);
16: 
17: 	// 쿨다운은 언제나 자기 자신에게 걸리므로 소스를 따지지 않는다 — 인스티게이터 ASC가 비면 스택이 갈라지는 AggregateBySource와 달리 항상 하나로 모인다.
18: 	// StackLimitCount 0은 무제한이다.
19: PRAGMA_DISABLE_DEPRECATION_WARNINGS
20: 	StackingType = EGameplayEffectStackingType::AggregateByTarget;
21: PRAGMA_ENABLE_DEPRECATION_WARNINGS
22: 	StackLimitCount = 0;
23: 	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
24: 	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
25: }
26: 
27: void UWxEffect_Cooldown::GrantCooldownTag(const FGameplayTag& CooldownTag)
28: {
29: 	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
30: 	FInheritedTagContainer GrantedTags;
31: 	GrantedTags.Added.AddTag(CooldownTag);
32: 	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
33: 	GEComponents.Add(TargetTagsComp);
34: }
35: 
36: float UWxMMC_CooldownDuration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
37: {
38: 	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
39: 	// 정적 데이터라 서버/클라 동일.
40: 	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
41: 	const float CooldownTime = Ability ? Ability->GetCooldownTime() : 0.f;
42: 	if (CooldownTime > 0.f)
43: 	{
44: 		return CooldownTime;
45: 	}
46: 
47: 	// 어빌리티를 거치지 않고 적용되면 수치를 알 수 없다. 0을 내면 엔진이 만료 타이머를 걸지 않아 쿨다운이 영구히 남으므로 즉시 만료시킨다.
48: 	UE_LOG(LogWxCombat, Warning, TEXT("%s: 소스 어빌리티에서 쿨다운 수치를 읽지 못해 즉시 만료시킨다."), *GetNameSafe(Spec.Def));
49: 	return UE_KINDA_SMALL_NUMBER;
50: }
...
```
