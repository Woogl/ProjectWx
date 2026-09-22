---
title: "피니시 기획 충돌과 현재 C++ 경로 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, finisher]
summary: "피니시 기획 충돌과 현재 C++ 경로 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 피니시 기획 충돌과 현재 C++ 경로 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Docs/CombatDesign/그로기_피니시_시스템_기획서.md
- [저장소 원문](<../../../Docs/CombatDesign/그로기_피니시_시스템_기획서.md>)
- SHA-256: `7f0fcdd55e07e7ec41bc391f4ac818e396b171d98838464fafae2e97c6e44ec3`
```text
...
17: ## 3. 규칙
18: 
19: ### 3.1 발동 전 조건
20: 
21: 다른 GA를 발동하고 있지 않거나 GA발동 중에도 후딜레이 상태에서 반경 3m 내에 그로기 상태의 적이 있으면 그로기 피니시 UI가 활성화된다.
22: 
23: 그로기 피니시 UI는 상호작용 UI와 동일한 UI를 사용한다.
24: 
25: 그로기 피니시 상호작용 패널은 다른 상호작용 오브젝트 범위와 겹칠 시 가장 위에 출력 기본적으로 먼저 선택된다.
26: 
27: UI가 활성화된 상태에서 상호작용 키(F)를 누르면 그로기 피니시가 발동된다.
28: 
29: 3m 이내에 그로기 상태의 적이 복수 존재할 경우, 발동 대상은 아래 우선순위를 따른다.
30: 
31: 1. 가장 가까운 적
32: 
33: 주변에 아이템 습득 등 상호작용 가능한 오브젝트와 그로기 상태의 적이 함께 있을 경우, 그로기 피니시를 우선한다.
34: 
35: ### 3.2 발동 중 규칙
36: 
37: 1. 그로기 피니시를 발동하면 적이 PC근처로 이동, 위치가 조정된다.
38: 2. 그로기 피니시가 발동되면 적의 스태거 게이지 증감이 멈춘다.
39: 3. 그로기 피니시 발동 중 PC는 무적 상태가 된다.
40: 4. 그로기 피니시의 피해 발생 후 적의 스태거 게이지가 전부 제거된다.
41: 5. 그로기 피니시의 피해 발생 후 적은 다운된다.
42: 
43: ---
44: 
45: ## 4. 플로우 차트
46: 
47: ```mermaid
48: flowchart TD
49:     A[그로기 상태의 적 발생] --> B{PC 반경 3m 내에 있는가?}
50:     B -->|아니오| C[그로기 피니시 UI 비활성화]
51:     B -->|예| D[그로기 피니시 UI 활성화]
52:     D --> E{상호작용 키 입력}
53:     E -->|아니오| D
54:     E -->|예| F{대상 후보가 복수인가?}
55:     F -->|아니오| G[해당 적 대상 지정]
56:     F -->|예| H{락온된 그로기 적이 있는가?}
57:     H -->|예| I[락온된 적 대상 지정]
58:     H -->|아니오| J[가장 가까운 적 대상 지정]
59:     G --> K[그로기 피니시 발동]
60:     I --> K
61:     J --> K
62:     K --> L[PC가 적절한 위치로 이동]
63:     L --> M[PC 무적 적용]
64:     M --> N[대상 스태거 게이지 증감 정지]
65:     N --> O[그로기 피니시 피해 발생]
66:     O --> P[대상 스태거 게이지 제거]
67:     P --> Q[대상 다운]
68:     Q --> R[대상 그로기 상태 회복]
69: ```
70: 
71: ---
72: 
73: ## 5. 데이터
74: 
```
## Source/WxGame/Character/WxEnemyCharacter.cpp
- [저장소 원문](<../../../Source/WxGame/Character/WxEnemyCharacter.cpp>)
- SHA-256: `c6469a467e3a1c42b858577f6bc70da5ca9087395e4dc7d9618acc938cea32a9`
```text
...
83: }
84: 
85: bool AWxEnemyCharacter::CanInteract(const AActor* Interactor) const
86: {
87: 	if (!UWxCombatLibrary::IsHostile(Interactor, this) || !IsAlive())
88: 	{
89: 		return false;
90: 	}
91: 
92: 	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
93: 	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_PlayMontageOnce))
94: 	{
95: 		return false;
96: 	}
97: 
98: 	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
99: 	{
100: 		return true;
101: 	}
102: 
103: 	return !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Engaged) && IsInRearCone(Interactor);
104: }
105: 
106: void AWxEnemyCharacter::OnInteracted(AActor* Interactor, int32 OptionValue)
107: {
108: 	if (!Interactor)
109: 	{
110: 		return;
111: 	}
112: 
113: 	FGameplayEventData EventData;
114: 	EventData.Instigator = Interactor;
115: 	EventData.Target = this;
116: 	EventData.EventTag = WxGameplayTags::Event_Finisher;
117: 	GetAbilitySystemComponent()->GetOwnedGameplayTags(EventData.TargetTags);
118: 
119: 	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, WxGameplayTags::Event_Finisher, EventData);
120: }
121: 
122: FText AWxEnemyCharacter::GetInteractionPrompt() const
123: {
124: 	// 문구의 주인은 실제로 나갈 처형 어빌리티다. 프롬프트는 로컬 표시라 그 어빌리티를 들고 있는 주체는 항상 로컬 플레이어다.
```
## Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp
- [저장소 원문](<../../../Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp>)
- SHA-256: `4eda07ae5106a20d3cf99535f2c9410cefdcd7384c00129aaa327beb1f296cde`
```text
...
58: }
59: 
60: void UWxAbility_Interact::ExecuteInteract(AActor* Selected, int32 OptionValue, const FGameplayAbilityActorInfo* ActorInfo)
61: {
62: 	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
63: 	if (!Selected || !Avatar)
64: 	{
65: 		return;
66: 	}
67: 
68: 	// 클라 비주얼은 각 대상의 복제 상태로 수렴하므로 여기서 따로 호출하지 않는다.
69: 	IWxInteractable* Target = Cast<IWxInteractable>(Selected);
70: 	if (!Target)
71: 	{
72: 		return;
73: 	}
74: 
75: 	// 서버 권위 자격 검증: 클라가 자격 없는 대상을(또는 자격을 잃은 직후에) 보내도 여기서 걸린다.
76: 	if (!Target->CanInteract(Avatar))
77: 	{
78: 		return;
...
88: 	// 통과하지 못한 상호작용은 실행도 아래 퀘스트 통지도 하지 않는다.
89: 	TArray<FWxInteractionOption> Options;
90: 	Target->GetInteractionOptions(Avatar, Options);
91: 	if (!Options.ContainsByPredicate([OptionValue](const FWxInteractionOption& Option) { return Option.Value == OptionValue; }))
92: 	{
93: 		return;
94: 	}
95: 
96: 	Target->OnInteracted(Avatar, OptionValue);
97: 
98: 	// 이 대상을 기다리던 퀘스트 스텝('상호작용 대기')이 있으면 여기서 완료된다. 기다리는 쪽이 없으면 무동작이다.
99: 	FWxStateTreeTask_WaitForInteraction::NotifyInteracted(Selected);
100: }
101: 
102: bool UWxAbility_Interact::IsInRange(const AActor* Selected, const FVector& Origin) const
103: {
104: 	for (const UActorComponent* Component : Selected->GetComponents())
105: 	{
106: 		const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
107: 		if (!Primitive || !Primitive->IsQueryCollisionEnabled())
108: 		{
109: 			continue;
110: 		}
111: 
112: 		// 스켈레탈 메시는 OverlapComponent 오버라이드가 피직스 애셋의 모든 바디를 훑는다.
113: 		if (Primitive->OverlapComponent(Origin, FQuat::Identity, FCollisionShape::MakeSphere(ScanRadius)))
114: 		{
115: 			return true;
116: 		}
117: 	}
118: 
119: 	return false;
120: }
```
## Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp>)
- SHA-256: `7dbe20c72ddf246d5674d399134358d5c0a539f4084c6dd8a195a2f24d565d4e`
```text
...
179: 
180: 	// 신규 후보는 이 순서 그대로 목록 뒤에 붙는다.
181: 	Candidates.Sort([ScanOrigin](const AActor& A, const AActor& B)
182: 	{
183: 		return FVector::DistSquared(ScanOrigin, A.GetActorLocation()) < FVector::DistSquared(ScanOrigin, B.GetActorLocation());
184: 	});
185: 
186: 	UpdateInRange(Candidates);
187: }
188: 
189: void UWxInteractionScannerComponent::UpdateInRange(const TArray<AActor*>& InCandidates)
190: {
191: 	// 아무것도 없는 대부분의 스캔이 선택지 수집 없이 여기서 끝난다.
192: 	if (InCandidates.IsEmpty() && Rows.IsEmpty())
193: 	{
194: 		return;
195: 	}
196: 
197: 	// 목록이 스캔마다 뒤섞이지 않게 한다.
198: 	TArray<AActor*> Ordered;
199: 	for (const FWxInteractionRow& Row : Rows)
200: 	{
201: 		AActor* Existing = Row.Actor.Get();
202: 		if (Existing && InCandidates.Contains(Existing))
203: 		{
204: 			Ordered.AddUnique(Existing);
205: 		}
206: 	}
207: 	for (AActor* Candidate : InCandidates)
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp>)
- SHA-256: `5ee843d90a1f97d0497f34748caa79c080974ce04ab1a2a8814e47f860a78a02`
```text
...
57: 	// 대상에 가하는 변경은 전부 대상 ASC 를 거치고 액터 자체는 위치만 읽으므로 const 로 다룬다.
58: 	const AActor* Target = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
59: 	const bool bBackstab = TriggerEventData && !TriggerEventData->TargetTags.HasTag(WxGameplayTags::Ability_Groggy);
60: 	const FWxFinisherVariant& Variant = bBackstab ? BackstabVariant : FinisherVariant;
61: 	UAnimMontage* SelectedAttackerMontage = Variant.AttackerMontage;
62: 	UAnimMontage* SelectedVictimMontage = Variant.VictimMontage;
63: 	UWxFinisherDamageComponent* FinisherDamageComponent = AvatarActor ? AvatarActor->FindComponentByClass<UWxFinisherDamageComponent>() : nullptr;
64: 
65: 	if (!SelectedAttackerMontage || !AvatarActor || !Target || !FinisherDamageComponent || !CommitAbility(Handle, ActorInfo, ActivationInfo))
66: 	{
67: 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
68: 		return;
69: 	}
70: 
71: 	TargetActor = Target;
72: 
73: 	// 어빌리티 부여는 권위에서만 — 클라에서 부르면 엔진이 거부하며 Error를 남긴다.
74: 	if (ActorInfo->IsNetAuthority())
75: 	{
76: 		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
77: 		{
...
87: 	}
88: 
89: 	RegisterWarpTarget(AvatarActor, Target);
90: 	FinisherDamageComponent->BeginFinisherDamage(Target, Variant.DamageDataRow);
91: 
92: 	if (!PlayMontage(SelectedAttackerMontage))
93: 	{
94: 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
95: 	}
96: }
97: 
98: void UWxAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
99: {
100: 	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
101: 	if (UWxFinisherDamageComponent* FinisherDamageComponent = AvatarActor ? AvatarActor->FindComponentByClass<UWxFinisherDamageComponent>() : nullptr)
102: 	{
103: 		FinisherDamageComponent->EndFinisherDamage(TargetActor.Get());
104: 	}
105: 
106: 	if (ActorInfo && ActorInfo->IsNetAuthority())
107: 	{
108: 		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor.Get()))
109: 		{
110: 			if (UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get())
111: 			{
112: 				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
113: 				const FGameplayEffectSpecHandle ResetSpec = SourceASC->MakeOutgoingSpec(UWxEffect_ResetGP::StaticClass(), GetAbilityLevel(), Context);
114: 				if (ResetSpec.IsValid())
115: 				{
116: 					SourceASC->ApplyGameplayEffectSpecToTarget(*ResetSpec.Data.Get(), TargetASC);
...
124: }
125: 
126: void UWxAbility_Finisher::RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const
127: {
128: 	UMotionWarpingComponent* MotionWarping = AvatarActor ? AvatarActor->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
129: 	if (!MotionWarping || !Target)
130: 	{
131: 		return;
132: 	}
133: 
134: 	// 멈출 간격·상대 포즈는 공격 몽타주의 Motion Warping Warp Point(애니)가 소유한다.
135: 	// 몬스터가 플레이어를 향해 회전하는 것은 피해자에게 부여되는 UWxAbility_PlayMontageOnce가 담당한다.
136: 	const FVector TargetLocation = Target->GetActorLocation();
137: 	FVector Direction = TargetLocation - AvatarActor->GetActorLocation();
138: 	Direction.Z = 0.0;
139: 	if (Direction.IsNearlyZero())
140: 	{
141: 		return;
142: 	}
143: 
144: 	const FRotator WarpRotation = Direction.Rotation();
```
