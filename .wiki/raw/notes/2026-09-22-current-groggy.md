---
title: "그로기 기획과 활성화 수명 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, groggy]
summary: "그로기 기획과 활성화 수명 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 그로기 기획과 활성화 수명 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Docs/CombatDesign/그로기_시스템.md
- [저장소 원문](<../../../Docs/CombatDesign/그로기_시스템.md>)
- SHA-256: `8d84e67b0cfd15473d4029589c9c10762b0cfb1305ed65ecb19bbefd8c88518b`
```text
...
33: - 구체적인 스태거 누적량은 개별 적 문서에서 정의한다.
34: 
35: ### 3.2 그로기 상태 효과
36: 
37: 그로기 상태에 진입한 적은 아래 규칙을 따른다.
38: 
39: | 항목 | 규칙 |
40: |---|---|
41: | 이동 | 불가능 |
42: | 공격 | 불가능 |
43: | 받는 피해 | 30% 증가 |
44: | 스태거 게이지 | 서서히 감소 |
45: | 스태거 추가 누적 | 불가능 |
46: | 피격 반응 | 그로기 히트 리액션만 발생 |
47: 
48: ---
49: 
50: ## 4. 몬스터 경직 시스템과의 관계
51: 
52: 그로기 시스템과 몬스터 경직 시스템은 서로 다른 역할을 가진다.
53: 
...
82: ## 6. 그로기 종료 규칙
83: 
84: ### 6.1 기본 종료 조건
85: 
86: - 그로기 상태 중 스태거 게이지는 서서히 감소한다.
87: - 스태거 게이지가 0이 되면 그로기 상태를 종료한다.
88: - 그로기 종료 후 적은 전투 상태로 복귀한다.
89: 
90: ### 6.2 종료 후 처리
91: 
92: 그로기 종료 시 아래 처리가 필요하다.
93: 
94: | 항목 | 처리 |
95: |---|---|
96: | 상태 | 그로기 상태 해제 |
97: | 이동 | 다시 가능 |
98: | 공격 | 다시 가능 |
99: | 받는 피해 증가 | 해제 |
100: | 스태거 게이지 | 0부터 시작 |
101: | 다음 행동 | 개별 적 BT 규칙에 따라 선택 |
102: 
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp>)
- SHA-256: `80f51ee7fd57c124574e686f7e35eb8290bbda98591c9071dd1d55092c57409e`
```text
...
12: #include "WxGameplayTags.h"
13: 
14: UWxAbility_Groggy::UWxAbility_Groggy()
15: {
16: 	// 로컬 조종 액터에는 복제 몽타주가 적용되지 않아 소유 클라도 활성화해야 한다.
17: 	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
18: 
19: 	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
20: 
21: 	FGameplayTagContainer AssetTags;
22: 	AssetTags.AddTag(WxGameplayTags::Ability_Groggy);
23: 	SetAssetTags(AssetTags);
24: 	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Groggy);
25: 	
26: 	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);
27: 
28: 	ActivationGroup = EWxAbilityActivationGroup::Override;
29: 
30: 	// Override 어빌리티는 캔슬되지 않아, 처형 짝 피격처럼 겹쳐야 할 반응이 보존된다.
31: 	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
32: 
...
41: 	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
42: 
43: 	if (!GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
44: 	{
45: 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
46: 		return;
47: 	}
48: 
49: 	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
50: 	if (!ASC)
51: 	{
52: 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
53: 		return;
54: 	}
55: 
56: 	StartMontagePolling();
57: 
58: 	// 클라의 복제 GP로 종료를 판정하면 서버와 종료 시점이 어긋난다.
59: 	if (ActorInfo->IsNetAuthority())
60: 	{
61: 		StartGroggyDrain(Handle, ActorInfo, ActivationInfo);
62: 	}
63: 
64: 	SetAILogicPaused(ActorInfo, true);
65: 
66: 	HandleMontagePollTick();
67: }
68: 
69: void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
70: {
71: 	StopMontagePolling();
72: 
73: 	if (ActorInfo)
74: 	{
75: 		SetAILogicPaused(ActorInfo, false);
76: 
77: 		if (ActorInfo->AbilitySystemComponent.IsValid())
78: 		{
79: 			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
80: 
81: 			// GroggyMontage 미설정 경로에서는 즉시 종료될 수 있다.
82: 			if (GroggyMontage)
83: 			{
84: 				ASC->StopMontageIfCurrent(*GroggyMontage);
...
92: }
93: 
94: void UWxAbility_Groggy::HandleGPChanged(const FOnAttributeChangeData& Data)
95: {
96: 	if (FMath::IsNearlyZero(Data.NewValue) || Data.NewValue < 0.f)
97: 	{
98: 		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
99: 	}
100: }
101: 
102: void UWxAbility_Groggy::HandleMontagePollTick()
103: {
104: 	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
105: 	if (!ASC || ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
106: 	{
107: 		// 사망 어빌리티가 Override 그로기를 취소하지 못해 여기서 종료한다.
108: 		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
109: 		return;
110: 	}
111: 
112: 	if (ASC->GetCurrentMontage() != nullptr)
113: 	{
114: 		return;
115: 	}
116: 
117: 	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
118: }
119: 
120: void UWxAbility_Groggy::StartMontagePolling()
...
134: }
135: 
136: void UWxAbility_Groggy::StartGroggyDrain(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
137: {
138: 	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
139: 	if (!ASC)
140: 	{
141: 		return;
142: 	}
143: 
144: 	GPDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute())
145: 		.AddUObject(this, &UWxAbility_Groggy::HandleGPChanged);
146: 
147: 	const float GroggyDuration = GroggyMontage->GetPlayLength();
148: 	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainGP::StaticClass(), GetAbilityLevel());
149: 	if (DrainSpecHandle.IsValid())
150: 	{
151: 		// 잠가서 넣어야 적용 시점의 Def 기반 Duration 재계산이 이 값을 덮어쓰지 않는다.
152: 		DrainSpecHandle.Data->SetDuration(GroggyDuration, true);
153: 		DrainGPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
154: 	}
155: }
156: 
157: void UWxAbility_Groggy::StopGroggyDrain(UAbilitySystemComponent& ASC)
158: {
159: 	if (DrainGPEffectHandle.IsValid())
160: 	{
161: 		ASC.RemoveActiveGameplayEffect(DrainGPEffectHandle);
162: 		DrainGPEffectHandle.Invalidate();
163: 	}
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp>)
- SHA-256: `aee562971380734774f9c67d444101cc53de6f6ac43f295906bafa44451be2be`
```text
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
```
