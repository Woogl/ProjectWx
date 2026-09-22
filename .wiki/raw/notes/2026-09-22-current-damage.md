---
title: "피해 판정·자원 반영 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, damage]
summary: "피해 판정·자원 반영 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 피해 판정·자원 반영 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp>)
- SHA-256: `a30f350b173b7086e7a7d23d2b2ea19f510eafd97fcea9ed0f307308b77d395a`
```text
...
10: #include "Weapon/WxProjectileBase.h"
11: 
12: bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
13: {
14: 	const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(Source);
15: 	if (!SourceTeamAgent)
16: 	{
17: 		return false;
18: 	}
19: 	
20: 	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
21: 	if (!TargetTeamAgent)
22: 	{
23: 		return false;
24: 	}
25: 	
26: 	return SourceTeamAgent->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
27: }
28: 
29: bool UWxCombatLibrary::ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult)
30: {
31: 	if (!Causer || !Target)
32: 	{
33: 		return false;
34: 	}
35: 
36: 	AActor* SourceActor = Causer;
37: 	UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Causer);
38: 	if (!Source)
39: 	{
40: 		SourceActor = Causer->GetOwner();
41: 		Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
42: 	}
43: 
44: 	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
45: 	if (!Source || !TargetASC || !Source->IsOwnerActorAuthoritative())
46: 	{
47: 		return false;
...
65: 	}
66: 
67: 	FGameplayEffectContextHandle Context(new FWxHitEffectContext(*Source->MakeEffectContext().Get(), DamageTableRow));
68: 	Context.AddInstigator(SourceActor, Causer);
69: 	Context.SetAbility(SourceAbility);
70: 	Context.AddHitResult(HitResult);
71: 
72: 	const FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(ANSI_TO_TCHAR(__FUNCTION__));
73: 	if (!DamageRow)
74: 	{
75: 		return false;
76: 	}
77: 
78: 	const FGameplayEffectSpecHandle HitSpec = DamageRow->MakeHitSpec(Source, Context, DamageLevel);
79: 	if (!HitSpec.IsValid())
80: 	{
81: 		return false;
82: 	}
83: 
84: 	// 적중 GE와 Cue는 서버 판정을 따른다. 과거 활성화 키를 실으면 예측본 잔류나 소유 클라의 Cue 생략이 발생한다.
85: 	Source->ApplyGameplayEffectSpecToTarget(*HitSpec.Data.Get(), TargetASC, FPredictionKey());
86: 	// Wrapper 접수와 자식 피해 적용은 다르다. 회피와 자식 거부에서는 히트스톱을 켜지 않는다.
87: 	return FWxHitEffectContext::Get(Context)->bDamageApplied;
88: }
89: 
90: void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* SourceAbility)
91: {
92: 	if (!TargetASC || !EffectClass)
93: 	{
94: 		return;
95: 	}
96: 
97: 	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
98: 	// 어빌리티 없이 걸리는 GE 도 GetAbilityLevel 의 기본 반환과 같은 레벨 1 로 만든다.
99: 	const float Level = SourceAbility ? SourceAbility->GetAbilityLevel() : 1.f;
100: 	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), Level);
101: 	// 발동의 활성화 키를 직접 실으면 창 밖의 권위 적용에도 키가 남아 소유 클라가 이 GE의 Cue를 건너뛴다.
102: 	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, TargetASC->GetPredictionKeyForNewAction());
103: }
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp>)
- SHA-256: `23e5247e190f2cb472b0ad9a0c5eceea6f43c7e8caa892befd73b4e07f43dc15`
```text
...
12: #include "WxGameplayTags.h"
13: 
14: bool UWxEffectComponent_Hit::CanGameplayEffectApply(const FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec) const
15: {
16: 	const UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
17: 	const UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
18: 	const FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext());
19: 	return Source && Target && Target->IsOwnerActorAuthoritative() && Context && Context->DamageTable.IsValid()
20: 		&& Context->DamageTable->FindRow<FWxDamageTableRow>(Context->DamageRowName, TEXT("HitRequirements"), false)
21: 		&& UWxCombatLibrary::IsHostile(Source->GetAvatarActor(), Target->GetAvatarActor());
22: }
23: 
24: void UWxEffectComponent_Hit::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
25: {
26: 	UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
27: 	UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
28: 	FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext());
29: 	if (!Source || !Target || !Context || !Context->DamageTable.IsValid())
30: 	{
31: 		return;
32: 	}
33: 
34: 	Context->ResetDamageResult();
35: 	if (Target->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible))
36: 	{
37: 		FGameplayEventData EventData;
38: 		EventData.EventTag = WxGameplayTags::Event_DodgeSuccess;
39: 		EventData.Instigator = GESpec.GetContext().GetInstigator();
40: 		EventData.Target = Target->GetOwnerActor();
41: 		EventData.ContextHandle = GESpec.GetContext();
42: 		Target->HandleGameplayEvent(WxGameplayTags::Event_DodgeSuccess, &EventData);
43: 		return;
44: 	}
45: 
46: 	const FWxDamageTableRow* Row = Context->DamageTable->FindRow<FWxDamageTableRow>(Context->DamageRowName, TEXT("Hit"), false);
47: 	if (!Row)
48: 	{
49: 		return;
50: 	}
51: 	const bool bCanGuard = GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanGuard);
52: 	Context->bPerfectGuard = bCanGuard && Target->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);
53: 	Context->bGuarded = bCanGuard && !Context->bPerfectGuard && Target->HasMatchingGameplayTag(WxGameplayTags::Effect_GuardReduction);
54: 
55: 	FGameplayEffectSpec DamageSpec;
56: 	DamageSpec.InitializeFromLinkedSpec(GetDefault<UWxEffect_Damage>(), GESpec);
57: 	// 같은 타격의 Context를 공유하되 LinkedSpec의 Source 태그 스냅샷은 유지한다.
58: 	DamageSpec.SetContext(GESpec.GetContext(), true);
59: 	// LinkedSpec은 SetByCaller를 복사하지만 DynamicAssetTags 자체는 복사하지 않는다.
60: 	DamageSpec.AppendDynamicAssetTags(GESpec.GetDynamicAssetTags());
61: 	TArray<FGameplayEffectSpecHandle> ExtraSpecs;
62: 	// 피해와 반응 이벤트가 소스 상태를 바꾸기 전에 추가 효과의 태그를 캡처한다.
63: 	if (!Context->bPerfectGuard)
64: 	{
65: 		for (const TSubclassOf<UGameplayEffect>& EffectClass : Row->AdditionalEffects)
66: 		{
67: 			if (EffectClass)
68: 			{
69: 				ExtraSpecs.Add(Source->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), GESpec.GetContext()));
70: 			}
...
72: 	}
73: 	const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(DamageSpec, Target, PredictionKey);
74: 	Context->bDamageApplied = AppliedHandle.WasSuccessfullyApplied();
75: 	if (!Context->bDamageApplied)
76: 	{
77: 		return;
78: 	}
79: 
80: 	// Cue나 반응 이벤트가 다른 효과를 적용하기 전에 이번 실행 결과를 보관한다.
81: 	const float Damage = Context->DamageMagnitude;
82: 	const float Reflect = Context->ReflectMagnitude;
83: 	const bool bHasReflect = Context->bHasReflect;
84: 	DamageSpec.AppendDynamicAssetTags(Context->DamageResultTags);
85: 	// 반사량이 0이어도 퍼펙트 가드의 출력 기록은 남으므로 타격 연출을 유지한다.
86: 	if (Damage > 0.f || bHasReflect)
87: 	{
88: 		FGameplayCueParameters HitCue;
89: 		UAbilitySystemGlobals::Get().InitGameplayCueParameters_GESpec(HitCue, DamageSpec);
90: 		HitCue.AggregatedTargetTags = Context->DamageTargetTags;
91: 		UAbilitySystemGlobals::Get().GetGameplayCueManager()->InvokeGameplayCueExecuted_WithParams(Target, WxGameplayTags::GameplayCue_Hit, PredictionKey, HitCue);
92: 	}
93: 
...
98: 	if (bHasReflect)
99: 	{
100: 		ProcessPerfectGuard(Target, DamageSpec, Reflect);
101: 	}
102: 
103: 	for (const FGameplayEffectSpecHandle& ExtraSpec : ExtraSpecs)
104: 	{
105: 		if (ExtraSpec.IsValid())
106: 		{
107: 			Source->ApplyGameplayEffectSpecToTarget(*ExtraSpec.Data.Get(), Target, PredictionKey);
108: 		}
109: 	}
110: }
111: 
112: void UWxEffectComponent_Hit::ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const
113: {
114: 	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
115: 	const FGameplayTagContainer& DamageTags = Spec.GetDynamicAssetTags();
116: 
117: 	AActor* TargetActor = ASC->GetOwnerActor();
118: 	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();
...
159: }
160: 
161: void UWxEffectComponent_Hit::ProcessPerfectGuard(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float ReflectAmount) const
162: {
163: 	const FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
164: 	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();
165: 
166: 	// 컨텍스트를 함께 실어야 가드 리액션이 피격 이벤트와 같은 방식으로 원인 액터를 집는다.
167: 	FGameplayEventData EventData;
168: 	EventData.EventTag = WxGameplayTags::Event_PerfectGuard;
169: 	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
170: 	EventData.Target = ASC->GetOwnerActor();
171: 	EventData.EventMagnitude = ReflectAmount;
172: 	EventData.ContextHandle = ContextHandle;
173: 	ASC->HandleGameplayEvent(WxGameplayTags::Event_PerfectGuard, &EventData);
174: 
175: 	// 가드 어빌리티의 구독 수명과 무관하게 이 GE에서 성립한 퍼펙트 가드 결과를 처리한다.
176: 	if (SourceASC)
177: 	{
178: 		// 이미 그로기면 GP를 더해 남은 드레인 시간보다 회복을 늦추지 않는다.
179: 		// 방어자 컨텍스트를 사용해야 반사 GP에 의한 그로기의 원인이 방어자로 기록된다.
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp>)
- SHA-256: `b3292eebdc1f11f3546bb2600a30c72ef4a8b596857a9d264b86becc9e8da92c`
```text
...
95: }
96: 
97: static float CalculateFinalDamage(float SourceATK, float TargetDEF, float ATKCoeff, float SourceCritDMG, bool bIsCritical, float GuardReductionScale)
98: {
99: 	const float DefenseMultiplier = CalculateDefenseMultiplier(TargetDEF);
100: 	const float BaseDamage = CalculateBaseDamage(SourceATK, ATKCoeff, DefenseMultiplier);
101: 	const float CriticalMultiplier = CalculateCriticalMultiplier(SourceCritDMG, bIsCritical);
102: 	const float GuardMultiplier = CalculateGuardMultiplier(GuardReductionScale);
103: 
104: 	// 여기서 정수로 못박아야 HP·GP·가드 SP·반사량이 함께 정수로 움직인다 — 소수 잔량으로 생존하거나 그로기 임계에 못 닿는 상황을 없앤다.
105: 	return FMath::RoundToFloat(BaseDamage * CriticalMultiplier * GuardMultiplier);
106: }
107: 
108: UWxExecCalc_Damage::UWxExecCalc_Damage()
109: {
110: 	const FWxDamageBaseStatics& BaseStatics = GetDamageBaseStatics();
111: 	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();
112: 	RelevantAttributesToCapture.Add(BaseStatics.ATKDef);
113: 	RelevantAttributesToCapture.Add(BaseStatics.DEFDef);
114: 	RelevantAttributesToCapture.Add(BaseStatics.GuardReductionScaleDef);
115: 	RelevantAttributesToCapture.Add(ExecutionStatics.CritRateDef);
...
163: 
164: 	float GuardReductionScale = 0.f;
165: 	if (bGuardHit)
166: 	{
167: 		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BaseStatics.GuardReductionScaleDef, EvalParams, GuardReductionScale);
168: 	}
169: 
170: 	const float FinalDamage = CalculateFinalDamage(SourceATK, TargetDEF, ATKCoeff, SourceCritDMG, bIsCritical, GuardReductionScale);
171: 
172: 	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();
173: 
174: 	if (bPerfectGuardApplied)
175: 	{
176: 		// 피해 대신 반사 메타 속성을 출력해 타격 Wrapper가 퍼펙트 가드 결과를 식별하게 한다.
177: 		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.IncomingReflectProperty, EGameplayModOp::Additive, FinalDamage));
178: 		return;
179: 	}
180: 
181: 	if (FinalDamage <= 0.f)
182: 	{
183: 		return;
184: 	}
185: 
186: 	if (bIsCritical)
187: 	{
188: 		ExecutionParams.GetOwningSpecForPreExecuteMod()->AddDynamicAssetTag(WxGameplayTags::Damage_Critical);
189: 	}
190: 
191: 	if (bGuardHit)
192: 	{
193: 		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.SPProperty, EGameplayModOp::Additive, -FinalDamage));
194: 
195: 		// 가드 브레이크는 차감 전 SP가 있어야 판정할 수 있다.
196: 		float TargetSP = 0.f;
197: 		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.SPDef, EvalParams, TargetSP);
198: 		if (TargetSP <= FinalDamage)
199: 		{
200: 			ExecutionParams.GetOwningSpecForPreExecuteMod()->AddDynamicAssetTag(WxGameplayTags::Damage_GuardBreak);
201: 		}
202: 	}
203: 
204: 	// 모디파이어마다 PostGameplayEffectExecute가 돌아 출력 순서가 곧 이벤트 순서다.
205: 	// GP가 먼저면 HP가 아직 안 깎여 그로기가 Ability.Death 가드를 지나친다.
206: 	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.IncomingDamageProperty, EGameplayModOp::Additive, FinalDamage));
207: 
208: 	if (!bIsGroggy)
209: 	{
210: 		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.GPProperty, EGameplayModOp::Additive, FinalDamage));
211: 	}
212: }
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp>)
- SHA-256: `aee562971380734774f9c67d444101cc53de6f6ac43f295906bafa44451be2be`
```text
...
62: 	Super::PreAttributeChange(Attribute, NewValue);
63: 
64: 	NewValue = ClampAttributeValue(Attribute, NewValue);
65: }
66: 
67: void UWxCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
68: {
69: 	Super::PreAttributeBaseChange(Attribute, NewValue);
70: 
71: 	NewValue = ClampAttributeValue(Attribute, NewValue);
72: }
73: 
74: void UWxCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
75: {
76: 	Super::PostAttributeChange(Attribute, OldValue, NewValue);
77: 
78: 	// 이 훅은 클라이언트의 복제 수신 경로에서도 호출된다. 아래 파생 갱신은 서버가 정해 복제하므로 권위 측에서만 실행한다.
79: 	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
80: 	if (!ASC || !ASC->IsOwnerActorAuthoritative())
81: 	{
82: 		return;
83: 	}
84: 
85: 	AdjustAttributeForMaxChange(Attribute, OldValue, NewValue);
86: }
87: 
88: void UWxCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
89: {
90: 	Super::PostGameplayEffectExecute(Data);
91: 
92: 	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
93: 	if (!ASC)
94: 	{
95: 		return;
96: 	}
97: 
98: 	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
99: 	{
100: 		// 리셋이 베이스만 지우므로, 현재값을 읽으면 지속형 모디파이어 몫이 매 타격 피해량에 얹힌다.
101: 		const float Damage = ASC->GetNumericAttributeBase(GetIncomingDamageAttribute());
102: 		SetIncomingDamage(0.f);
103: 
104: 		if (Damage > 0.f)
105: 		{
106: 			// 현재값이 아니라 베이스에서 뺀다. 현재값에서 빼면 지속형 모디파이어 몫이 베이스로 굳는다.
107: 			SetHP(ASC->GetNumericAttributeBase(GetHPAttribute()) - Damage);
108: 
109: 			// 사망 표식(Ability.Death)은 사망 어빌리티가 활성 동안 들고 있으므로, 여기서는 발동만 알린다.
110: 			if (GetHP() <= 0.f && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
111: 			{
112: 				FGameplayEventData EventData;
113: 				EventData.EventTag = WxGameplayTags::Event_Death;
114: 				EventData.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
115: 				EventData.Target = GetOwningActor();
116: 				ASC->HandleGameplayEvent(WxGameplayTags::Event_Death, &EventData);
117: 			}
118: 		}
119: 	}
...
122: 		SetIncomingReflect(0.f);
123: 	}
124: 	else if (Data.EvaluatedData.Attribute == GetGPAttribute() && GetMaxGP() > 0.f)
125: 	{
126: 		// 그로기 진입만 알리고 해제는 관여하지 않는다 — 어빌리티가 GP를 직접 보고 스스로 끝낸다.
127: 		if (GetGP() >= GetMaxGP() && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
128: 		{
129: 			FGameplayEventData EventData;
130: 			EventData.EventTag = WxGameplayTags::Event_Groggy;
131: 			EventData.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
132: 			EventData.Target = GetOwningActor();
133: 			ASC->HandleGameplayEvent(WxGameplayTags::Event_Groggy, &EventData);
134: 		}
135: 	}
136: }
137: 
138: float UWxCombatAttributeSet::ClampAttributeValue(const FGameplayAttribute& Attribute, float NewValue) const
139: {
140: 	const float MinimumValue = Attribute == GetASPDAttribute() ? 0.001f : 0.f;
141: 	NewValue = FMath::Max(NewValue, MinimumValue);
142: 
...
151: }
152: 
153: void UWxCombatAttributeSet::AdjustAttributeForMaxChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
154: {
155: 	const FWxMaxAttributePair* Pair = FindMaxAttributePair(Attribute);
156: 	if (!Pair || Pair->MaxAttribute != Attribute || OldValue <= 0.f || FMath::IsNearlyEqual(OldValue, NewValue))
157: 	{
158: 		return;
159: 	}
160: 
161: 	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
162: 	if (!ASC)
163: 	{
164: 		return;
165: 	}
166: 
167: 	// 현재값이 아니라 베이스를 읽어 베이스에 쓴다. 현재값을 읽으면 지속형 모디파이어 몫까지 베이스로 굳어 GE가 걷혀도 남는다.
168: 	const float ScaledBase = ASC->GetNumericAttributeBase(Pair->Attribute) * NewValue / OldValue;
169: 	ASC->SetNumericAttributeBase(Pair->Attribute, ScaledBase);
170: }
171: 
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp>)
- SHA-256: `86346fcc2662af33af4cb8cf2847d98239e829c19933385769a38ae081c8c62d`
```text
...
8: #include "WxGameplayTags.h"
9: 
10: void UWxEffectComponent_DamageResponse::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
11: {
12: 	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
13: 	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
14: 	if (!ASC)
15: 	{
16: 		return;
17: 	}
18: 
19: 	// 메타 속성은 이미 초기화됐다. 실행 기록을 사용해 남은 HP로 플로터 수치가 잘리지 않게 한다.
20: 	const FGameplayEffectModifiedAttribute* Damage = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingDamageAttribute());
21: 	if (FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext()))
22: 	{
23: 		const FGameplayEffectModifiedAttribute* Reflect = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingReflectAttribute());
24: 		Context->DamageMagnitude = Damage ? Damage->TotalMagnitude : 0.f;
25: 		Context->ReflectMagnitude = Reflect ? Reflect->TotalMagnitude : 0.f;
26: 		Context->bHasReflect = Reflect != nullptr;
27: 		Context->DamageResultTags = GESpec.GetDynamicAssetTags();
28: 		Context->DamageTargetTags = *GESpec.CapturedTargetTags.GetAggregatedTags();
29: 	}
30: 	if (Damage && Damage->TotalMagnitude > 0.f && GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Attack))
```
