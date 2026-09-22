---
title: "AI 인지·Blackboard·락온 수명 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, ai]
summary: "AI 인지·Blackboard·락온 수명 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# AI 인지·Blackboard·락온 수명 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxAI/Source/WxAI/WxAI.Build.cs
- [저장소 원문](<../../../Plugins/WxAI/Source/WxAI/WxAI.Build.cs>)
- SHA-256: `7771b653372eeb3ea29ea57ff6b74f73bcdeaf0fdac85dc021b5a2cde460b754`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"AIModule",
14: 			"Core",
15: 			"CoreUObject",
16: 			"Engine",
17: 			"GameplayAbilities",
18: 			"GameplayTags",
19: 			"WxCore",
20: 		});
21: 
22: 		PrivateDependencyModuleNames.AddRange(new string[]
23: 		{
24: 			"NavigationSystem",
25: 		});
26: 	}
27: }
```
## Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h
- [저장소 원문](<../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h>)
- SHA-256: `50fdf967dae5fc0b03d9aa2dacc55a82e1142bc5d3561d6f854ecdeedefb927b`
```text
...
16:  * 키가 에셋에 없거나 타입이 어긋나면 엔진이 조용히 기본값을 돌려주므로, accessor 는 그런 접근을 경고 로그로 드러낸다(Shipping 빌드 제외).
17:  */
18: namespace WxBlackboardKeys
19: {
20: 	WXAI_API extern const FName SelfActor;
21: 	WXAI_API extern const FName TargetActor;
22: 	/** 소환자다. 주인 없이 태어난 폰에서는 비어 있다. */
23: 	WXAI_API extern const FName Master;
24: 	WXAI_API extern const FName HomeLocation;
25: 	WXAI_API extern const FName PatrolTargetLocation;
26: 	WXAI_API extern const FName TargetDistance;
27: 
28: 	// Object 키: null = 미설정이라 setter 에 nullptr 을 넘기면 Clear 와 동일하게 동작 → 별도 Clear 불필요.
29: 
30: 	WXAI_API AActor* GetTargetActor(const UBlackboardComponent* Blackboard);
31: 	WXAI_API void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value);
32: 
33: 	WXAI_API AActor* GetSelfActor(const UBlackboardComponent* Blackboard);
34: 	WXAI_API void SetSelfActor(UBlackboardComponent* Blackboard, AActor* Value);
35: 
36: 	WXAI_API AActor* GetMaster(const UBlackboardComponent* Blackboard);
...
42: 
43: 	// Float 키: Clear 가 0(=코앞)을 써 근거리 비교를 통과시키므로, 타겟이 없을 때는 대신 이 값을 기록해 "무한히 멀다"로 읽히게 한다.
44: 	WXAI_API extern const float NoTargetDistance;
45: 
46: 	WXAI_API void SetTargetDistance(UBlackboardComponent* Blackboard, float Value);
47: }
```
## Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp
- [저장소 원문](<../../../Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp>)
- SHA-256: `3a5d993ce2015571a51aea047a9fb669f66d77390f4f270e23e41eb4fdc8aeff`
```text
...
22: UWxAIBehaviorComponent::UWxAIBehaviorComponent()
23: {
24: 	bWantsInitializeComponent = true;
25: 
26: #if WITH_EDITOR
27: 	PrimaryComponentTick.bCanEverTick = true;
28: 	bTickInEditor = true;
29: #endif
30: }
31: 
32: void UWxAIBehaviorComponent::InitializeComponent()
33: {
34: 	Super::InitializeComponent();
35: 
36: 	// 빙의가 폰의 Owner 를 컨트롤러로 덮기 전인 지금만 스폰 주체를 알 수 있다.
37: 	const AActor* Spawner = GetOwner()->GetOwner();
38: 	PatrolPath = Spawner ? Spawner->FindComponentByClass<UWxPatrolComponent>() : nullptr;
39: }
40: 
41: void UWxAIBehaviorComponent::BeginPlay()
42: {
43: 	Super::BeginPlay();
44: 
45: #if WITH_EDITOR
46: 	// PIE에서도 저작용 드로우를 위한 Tick은 필요하지 않다.
47: 	SetComponentTickEnabled(false);
48: #endif
49: 
50: 	APawn* Pawn = GetOwner<APawn>();
...
57: 	// 배치된 폰은 이 컴포넌트의 BeginPlay 전에 빙의되므로 델리게이트만으로는 첫 컨트롤러를 놓친다.
58: 	Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UWxAIBehaviorComponent::HandleControllerChanged);
59: 	ApplySenseSettings(Pawn->GetController());
60: 
61: 	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
62: 	{
63: 		// 가드 브레이크 히트는 Event.Hit.GuardBreak 자식으로 나가므로 정확 매칭 구독은 놓친다.
64: 		ASC->AddGameplayEventTagContainerDelegate(FGameplayTagContainer(WxGameplayTags::Event_Hit),
65: 			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UWxAIBehaviorComponent::HandlePawnHit));
66: 	}
67: }
68: 
69: #if WITH_EDITOR
70: void UWxAIBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
71: {
72: 	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
73: 
74: 	UWorld* World = GetWorld();
75: 	const APawn* Pawn = GetOwner<APawn>();
76: 	if (!World || World->WorldType != EWorldType::Editor || !Pawn)
77: 	{
78: 		return;
79: 	}
80: 
81: 	// ChildActor 미리보기는 Pawn 대신 이를 배치한 부모 액터가 선택된다.
82: 	const AActor* SelectedActor = Pawn;
83: 	while (SelectedActor && !SelectedActor->IsSelected())
...
127: void UWxAIBehaviorComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
128: {
129: 	ApplySenseSettings(NewController);
130: }
131: 
132: void UWxAIBehaviorComponent::ApplySenseSettings(AController* Controller) const
133: {
134: 	AAIController* AIController = Cast<AAIController>(Controller);
135: 	UAIPerceptionComponent* Perception = AIController ? AIController->GetPerceptionComponent() : nullptr;
136: 	if (!Perception)
137: 	{
138: 		return;
139: 	}
140: 
141: 	if (UAISenseConfig_Sight* SightConfig = Perception->GetSenseConfig<UAISenseConfig_Sight>())
142: 	{
143: 		SightConfig->SightRadius = SightRadius;
144: 		// 반경 경계에서 붙었다 떨어졌다 하는 것은 리시 복귀가 다루므로 시야 상실 반경에 히스테리시스를 두지 않는다.
145: 		SightConfig->LoseSightRadius = SightRadius;
146: 		SightConfig->PeripheralVisionAngleDegrees = SightAngle;
147: 
...
158: }
159: 
160: void UWxAIBehaviorComponent::HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
161: {
162: 	// 패리 반동은 대미지 없이 Event.Hit.Parry 로 같은 구독에 걸리므로 자극에서 뺀다.
163: 	if (!Payload || Payload->EventMagnitude <= 0.f)
164: 	{
165: 		return;
166: 	}
167: 
168: 	APawn* Pawn = GetOwner<APawn>();
169: 	AActor* DamageInstigator = Payload->ContextHandle.GetInstigator();
170: 	if (!Pawn || !DamageInstigator)
171: 	{
172: 		return;
173: 	}
174: 
175: 	// Sight·Hearing 과 달리 Damage 센스에는 DetectionByAffiliation 이 없어 엔진이 가해자를 가려 주지 않는다.
176: 	// 여기서 막지 않으면 아군 오사 한 번에 서로를 타겟으로 확정한다.
177: 	if (FGenericTeamId::GetAttitude(Pawn, DamageInstigator) != ETeamAttitude::Hostile)
178: 	{
```
## Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp
- [저장소 원문](<../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp>)
- SHA-256: `e5a9c63d6881d57610c3e40d29281a99f2806cca80f352c004a3a501a299aaf5`
```text
...
22: }
23: 
24: void UWxBTService_UpdateTargetActor::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
25: {
26: 	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
27: 
28: 	AAIController* AIController = OwnerComp.GetAIOwner();
29: 	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
30: 	if (!AIController || !Blackboard)
31: 	{
32: 		return;
33: 	}
34: 
35: 	AActor* CurrentTarget = WxBlackboardKeys::GetTargetActor(Blackboard);
36: 	if (CanBeAggroTarget(CurrentTarget) && !IsActorDead(CurrentTarget))
37: 	{
38: 		return;
39: 	}
40: 
41: 	UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent();
42: 	if (!Perception)
43: 	{
44: 		WxBlackboardKeys::SetTargetActor(Blackboard, nullptr);
45: 		return;
46: 	}
47: 
48: 	// 타겟에서 내려오는 대상은 감지 기록까지 지운다 — 청각·촉각 자극은 MaxAge 안에 남아 있어, 자격을 되찾는 순간 그대로 어그로가 된다.
49: 	Perception->ForgetActor(CurrentTarget);
50: 
51: 	WxBlackboardKeys::SetTargetActor(Blackboard, FindPerceivedTarget(*Perception, AIController->GetPawn()));
52: }
53: 
54: AActor* UWxBTService_UpdateTargetActor::FindPerceivedTarget(const UAIPerceptionComponent& Perception, const AActor* SelfActor) const
55: {
56: 	TArray<AActor*> PerceivedActors;
57: 	Perception.GetCurrentlyPerceivedActors(nullptr, PerceivedActors);
58: 
59: 	for (AActor* PerceivedActor : PerceivedActors)
60: 	{
61: 		// 엔진 청각은 소리를 낸 본인의 리스너를 제외하지 않아, 자기 발소리가 그대로 자기 자극으로 돌아온다.
62: 		if (PerceivedActor != SelfActor && CanBeAggroTarget(PerceivedActor) && !IsActorDead(PerceivedActor))
63: 		{
64: 			return PerceivedActor;
65: 		}
66: 	}
67: 
68: 	return nullptr;
69: }
70: 
71: bool UWxBTService_UpdateTargetActor::IsActorDead(AActor* Actor) const
72: {
73: 	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
74: 	return ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
75: }
76: 
77: bool UWxBTService_UpdateTargetActor::CanBeAggroTarget(AActor* Actor) const
78: {
79: 	if (!IsValid(Actor))
80: 	{
```
## Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp
- [저장소 원문](<../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp>)
- SHA-256: `115be32edd01aaf0ab575bfb2e2a4f6856889d7718ed2856328324b2b6278dd1`
```text
...
15: 	NodeName = TEXT("Lock On");
16: 
17: 	// TickNode/OnBecomeRelevant/OnCeaseRelevant 오버라이드를 감지해 알림 플래그를 자동 설정한다(엔진 서비스 관용).
18: 	INIT_SERVICE_NODE_NOTIFY_FLAGS();
19: 
20: 	// bCallTickOnSearchStart 는 쓰지 않는다 — 그 틱은 aux 노드 등록이 커밋되기 전에 돌아, 탐색이 폐기되면 걸어 둔 포커스·회전 모드를 되돌릴 OnCeaseRelevant 가 영영 오지 않는다.
21: 	Interval = 0.1f;
22: 	RandomDeviation = 0.0f;
23: }
24: 
25: uint16 UWxBTService_LockOn::GetInstanceMemorySize() const
26: {
27: 	return sizeof(FWxLockOnMemory);
28: }
29: 
30: void UWxBTService_LockOn::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
31: {
32: 	InitializeNodeMemory<FWxLockOnMemory>(NodeMemory, InitType);
33: }
34: 
35: void UWxBTService_LockOn::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
36: {
37: 	CleanupNodeMemory<FWxLockOnMemory>(NodeMemory, CleanupType);
38: }
...
65: 	// StopTree 도 활성 aux 노드에 이 통지를 돌리므로, 사망·빙의 해제·컨트롤러 파괴가 모두 여기로 모인다.
66: 	FWxLockOnMemory* Memory = CastInstanceNodeMemory<FWxLockOnMemory>(NodeMemory);
67: 	ReleaseLockOn(OwnerComp.GetAIOwner(), *Memory);
68: }
69: 
70: void UWxBTService_LockOn::SyncLockOn(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
71: {
72: 	AAIController* AIController = OwnerComp.GetAIOwner();
73: 	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
74: 	if (!AIController || !Blackboard)
75: 	{
76: 		return;
77: 	}
78: 
79: 	FWxLockOnMemory* Memory = CastInstanceNodeMemory<FWxLockOnMemory>(NodeMemory);
80: 
81: 	AActor* Target = WxBlackboardKeys::GetTargetActor(Blackboard);
82: 	APawn* Pawn = AIController->GetPawn();
83: 	if (!Target || !Pawn)
84: 	{
85: 		ReleaseLockOn(AIController, *Memory);
86: 		return;
87: 	}
88: 
89: 	// 걸어 둔 그대로면 손대지 않는다 — 매 틱 같은 값을 다시 쓰면 그 사이 다른 곳이 바꾼 회전 모드를 덮어쓴다.
90: 	if (Memory->LockedOnPawn.Get() == Pawn && AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) == Target)
91: 	{
92: 		return;
93: 	}
94: 
95: 	// 폰이 바뀔 때만 이전 폰을 되돌린다.
96: 	// 대상만 갈리는 재타겟은 SetFocus 가 같은 우선순위를 먼저 비우므로, 회전 모드를 아키타입 값으로 왕복시킬 이유가 없다.
97: 	if (Memory->LockedOnPawn.Get() != Pawn)
98: 	{
99: 		ReleaseLockOn(AIController, *Memory);
100: 	}
101: 
102: 	ApplyLockOn(*AIController, *Pawn, *Target, *Memory);
103: }
104: 
105: void UWxBTService_LockOn::ApplyLockOn(AAIController& AIController, APawn& Pawn, AActor& Target, FWxLockOnMemory& Memory) const
106: {
107: 	// 이동 중에는 PathFollowing 이 Move 우선순위로 진행 방향을 응시시키므로, 그보다 높은 Gameplay 로 걸어야 전투 대상을 계속 본다.
108: 	AIController.SetFocus(&Target, EAIFocusPriority::Gameplay);
109: 
110: 	// 폰을 기록해 두는 이유: 빙의 해제는 컨트롤러의 폰 참조를 끊은 뒤에야 BT 를 멈추므로, 그 경로의 해제 시점엔 GetPawn() 이 이미 비어 있다.
111: 	Memory.LockedOnPawn = &Pawn;
112: 
113: 	const ACharacter* Character = Cast<ACharacter>(&Pawn);
114: 	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
115: 	if (!Movement)
116: 	{
117: 		return;
118: 	}
119: 
120: 	Movement->bOrientRotationToMovement = false;
121: 	Movement->bUseControllerDesiredRotation = true;
122: }
123: 
```
