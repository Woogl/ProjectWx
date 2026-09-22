---
title: "장치 복제·상호작용·체크포인트 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, world]
summary: "장치 복제·상호작용·체크포인트 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 장치 복제·상호작용·체크포인트 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs>)
- SHA-256: `d5f800e8bd2b5f5b29ad80ef065c15af32a6d48eb24efdad2e6ad0b1732bdcfe`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"Core",
14: 			"CoreUObject",
15: 			"DeveloperSettings",
16: 			"Engine",
17: 			"GameplayAbilities",
18: 			"GameplayTags",
19: 			"StateTreeModule",
20: 			"UniversalObjectLocator",
21: 			"WxCore",
22: 		});
23: 
24: 		PrivateDependencyModuleNames.AddRange(new string[]
25: 		{
26: 			"AIModule",
27: 			"GameplayStateTreeModule",
28: 			"GameplayTasks",
29: 			"LevelSequence",
30: 			"MovieScene",
31: 			"Niagara",
32: 		});
33: 
34: 		if (Target.bBuildEditor)
35: 		{
36: 			PrivateDependencyModuleNames.Add("UnrealEd");
37: 			PrivateDependencyModuleNames.Add("PropertyBindingUtils");
38: 		}
39: 	}
40: }
```
## Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp>)
- SHA-256: `cd37f2cb9138e37a0788040ebbf49291d365315232ca7561a8ff15ae17564494`
```text
...
66: 	}
67: 
68: 	SynchronizeAfterStart();
69: }
70: 
71: void UWxDeviceStateTreeComponent::RestartLogic()
72: {
73: 	{
74: 		TGuardValue<bool> RestoreGuard(bRestoringState, true);
75: 		Super::RestartLogic();
76: 	}
77: 
78: 	SynchronizeAfterStart();
79: }
80: 
81: void UWxDeviceStateTreeComponent::SynchronizeAfterStart()
82: {
83: 	if (GetOwnerRole() == ROLE_Authority)
84: 	{
85: 		if (InitialTarget.IsValid())
86: 		{
87: 			EnterState(InitialTarget, true);
88: 		}
89: 		else
90: 		{
91: 			// 틱 없이 잠드는 첫 상태('작동 대기' 뿐인 상태)도 발행되게 한다.
92: 			PublishState();
93: 		}
94: 	}
95: 	else if (StateSnapshot.EntrySerial != 0)
96: 	{
...
127: }
128: 
129: void UWxDeviceStateTreeComponent::PublishState()
130: {
131: 	const FGameplayTag ActiveTag = FindActiveStateTag();
132: 	if (!ActiveTag.IsValid() || ActiveTag.GetTagName() == StateSnapshot.StateTagName)
133: 	{
134: 		return;
135: 	}
136: 
137: 	StateSnapshot.StateTagName = ActiveTag.GetTagName();
138: 	if (++StateSnapshot.EntrySerial == 0)
139: 	{
140: 		++StateSnapshot.EntrySerial;
141: 	}
142: 	const AWxDevice* Device = Cast<AWxDevice>(GetOwner());
143: 	ACharacter* Interactor = Device ? Device->GetInteractingCharacter() : nullptr;
144: 	StateSnapshot.Interactor = IsValid(Interactor) ? Interactor : nullptr;
145: 	StateSnapshot.SelectedOptionValue = Device ? Device->GetSelectedOptionValue() : INDEX_NONE;
146: 	GetOwner()->ForceNetUpdate();
147: 	UE_LOG(LogWxWorld, Verbose, TEXT("Device publish: %s"), *DescribeSynchronization());
148: }
149: 
150: void UWxDeviceStateTreeComponent::OnRep_StateSnapshot(const FWxDeviceStateSnapshot& Previous)
151: {
152: 	// 당사자 참조가 뒤늦게 해소되면 같은 번호로 한 번 더 통지된다.
153: 	ApplyInteractor();
154: 	UE_LOG(LogWxWorld, Verbose, TEXT("Device receive: %s"), *DescribeSynchronization());
155: 
156: 	// 트리가 아직 시작 전이면 StartLogic 이 이 스냅샷을 적용한다.
157: 	if (Previous.EntrySerial == StateSnapshot.EntrySerial || GetStateTreeRunStatus() == EStateTreeRunStatus::Unset)
158: 	{
159: 		return;
160: 	}
161: 
162: 	const FGameplayTag AuthorityTag = GetStateTag();
163: 	const bool bLive = Previous.EntrySerial != 0 && StateSnapshot.EntrySerial == Previous.EntrySerial + 1;
164: 	if (bLive && FindActiveStateTag() == AuthorityTag)
165: 	{
166: 		// 클라가 제 타이머로 먼저 도착했다.
167: 		return;
168: 	}
...
181: }
182: 
183: void UWxDeviceStateTreeComponent::EnterState(FGameplayTag Tag, bool bRestore)
184: {
185: 	const UStateTree* Asset = StateTreeRef.GetStateTree();
186: 	const FStateTreeStateHandle State = Asset ? Asset->GetStateHandleFromGameplayTag(Tag, UStateTree::EStateGameplayTagQueryMethod::MatchesExact) : FStateTreeStateHandle::Invalid;
187: 	if (!State.IsValid())
188: 	{
189: 		UE_LOG(LogWxWorld, Error, TEXT("Device: 태그 '%s' 상태가 루트 에셋에 없다 — %s"), *Tag.ToString(), *DescribeSynchronization());
190: 		return;
191: 	}
192: 
193: 	if (!IsRunning())
194: 	{
195: 		// 끝난 트리는 전이 요청을 받지 않는다.
196: 		TGuardValue<bool> RestoreGuard(bRestoringState, true);
197: 		Super::RestartLogic();
198: 	}
199: 
200: 	FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData);
201: 	if (!IsRunning() || !SetContextRequirements(Context))
```
## Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceExecutionPolicy.cpp>)
- SHA-256: `5c127934d648696d3ea58c731dd2540f10ab25262ce908b1e1b81f61dc2581f3`
```text
...
6: #include "StateTreeExecutionContext.h"
7: 
8: bool FWxDeviceExecutionPolicy::IsRestoring(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
9: {
10: 	return !Transition.SourceStateID.IsValid() || IsRestoringDevice(Context);
11: }
12: 
13: bool FWxDeviceExecutionPolicy::IsRestoringDevice(const FStateTreeExecutionContext& Context)
14: {
15: 	const AActor* Owner = Cast<AActor>(Context.GetOwner());
16: 	const UWxDeviceStateTreeComponent* Component = Owner ? Owner->FindComponentByClass<UWxDeviceStateTreeComponent>() : nullptr;
17: 	return Component && Component->IsRestoringState();
18: }
```
## Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp>)
- SHA-256: `7dbe20c72ddf246d5674d399134358d5c0a539f4084c6dd8a195a2f24d565d4e`
```text
...
26: FWxOnScannerReady UWxInteractionScannerComponent::OnAnyScannerReady;
27: 
28: void UWxInteractionScannerComponent::BeginPlay()
29: {
30: 	Super::BeginPlay();
31: 
32: 	OnAnyScannerReady.Broadcast(this);
33: 
34: 	const AController* OwningController = Cast<AController>(GetOwner());
35: 	if (!OwningController || !OwningController->IsLocalController())
36: 	{
37: 		return;
38: 	}
39: 
40: 	if (UWorld* World = GetWorld())
41: 	{
42: 		World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UWxInteractionScannerComponent::HandleScanTimer, FMath::Max(ScanInterval, 0.01f), true);
43: 	}
44: 
45: 	HandleScanTimer();
46: }
...
108: }
109: 
110: void UWxInteractionScannerComponent::ServerInteract_Implementation(AActor* Selected, int32 OptionValue)
111: {
112: 	APawn* Pawn = GetOwnerPawn();
113: 	if (!Pawn)
114: 	{
115: 		return;
116: 	}
117: 
118: 	FGameplayEventData EventData;
119: 	EventData.Instigator = Pawn;
120: 	EventData.EventTag = WxGameplayTags::Event_Interact;
121: 	EventData.OptionalObject = Selected;
122: 	EventData.EventMagnitude = OptionValue;
123: 	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, WxGameplayTags::Event_Interact, EventData);
124: }
125: 
126: void UWxInteractionScannerComponent::HandleScanTimer()
127: {
128: 	APawn* Pawn = GetOwnerPawn();
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
## Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp>)
- SHA-256: `d8422e06e185f9523cb23960ec1af7e3d2badd0bc614d921ca5a82c10ee96690`
```text
...
5: #include "Engine/World.h"
6: 
7: void UWxCheckpointSubsystem::RecordCheckpoint(const UWorld* World, const FTransform& Transform)
8: {
9: 	if (!World || !World->IsNetMode(NM_Standalone) || Transform.ContainsNaN())
10: 	{
11: 		return;
12: 	}
13: 	LevelPackage = FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
14: 	RespawnTransform = FTransform(Transform.GetRotation(), Transform.GetLocation());
15: }
16: 
17: bool UWxCheckpointSubsystem::TryGetCheckpoint(const UWorld* World, FTransform& OutTransform) const
18: {
19: 	if (!World || !World->IsNetMode(NM_Standalone) || LevelPackage.IsNone()
20: 		|| LevelPackage != FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())))
21: 	{
22: 		return false;
23: 	}
24: 	OutTransform = RespawnTransform;
25: 	return true;
26: }
27: 
28: void UWxCheckpointSubsystem::ResetCheckpoint()
29: {
30: 	LevelPackage = NAME_None;
31: 	RespawnTransform = FTransform::Identity;
32: }
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
