---
title: "퀘스트 계약·StateTree 태스크·저널 표시 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, quests]
summary: "퀘스트 컴포넌트 공개 계약·StateTree 태스크·수주 라이브러리·저널 ViewModel의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: 60c324c714b1dab10cd48d36cabad63ace232716
---

# 퀘스트 계약·StateTree 태스크·저널 표시 조사

조사일: 2026-09-22. 기준 HEAD: `60c324c714b1dab10cd48d36cabad63ace232716`, 대상 파일은 작업 트리 변경 없음. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h>)
- SHA-256: `3f2e3906265400d78cac0c2e991373413eb1a7ce1f892abb6fb310d2d399b2af`
```text
...
13: DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnQuestJournalChanged);
14: 
15: /**
16:  * 목표는 등록한 태스크가 자기 상태를 떠날 때 스스로 걷어가므로, 문구가 아니라 발급 핸들로 지목한다(같은 문구가 둘일 수 있다).
17:  */
18: USTRUCT()
19: struct FWxQuestObjective
20: {
21: 	GENERATED_BODY()
22: 
23: 	UPROPERTY()
24: 	int32 Handle = INDEX_NONE;
25: 
26: 	UPROPERTY()
27: 	FText Text;
28: };
29: 
30: /**
31:  * GameState 에 부착되어 퀘스트 StateTree 실행과 저널(제목·목표)을 서버 권위로 관리하는 컴포넌트.
32:  *
33:  * 퀘스트 1개 = UStateTree 에셋 1개이며, 활성 퀘스트는 동시 1개다(새 시작은 교체).
34:  * UStateTreeComponent 와는 다중상속이 불가하므로, 권위 측 BeginPlay 에서 순정 러너를 런타임 생성해 소유하고 실행을 위임한다.
35:  * 퀘스트 태스크는 컨텍스트 오너(GameState)에서 본 컴포넌트를 찾아 저널 갱신·다음 퀘스트 시작을 요청한다.
36:  * 러너가 권위에만 존재하므로 월드 부수효과(스폰·보상)는 단일 구동이 보장되고, 저널도 권위(싱글/리슨 호스트)에서만 채워진다.
37:  * 저널 정리는 태스크가 아니라 러너의 실행 상태 변경 통지로 한다 — 완료·실패·교체 세 종료 경로가 전부 한 곳으로 수렴한다.
38:  *
39:  * 본 컴포넌트는 어떤 퀘스트 에셋도 알지 않는다(에셋 불가지). 무엇을 실행할지는 전부 데이터가 지정한다:
40:  *  - 수주: 레벨에 배치한 트리거 볼륨이 UWxQuestLibrary::StartQuest 로 넘기는 에셋
41:  *  - 체인: StartNextQuest 태스크의 Quest 소프트 참조
42:  *
43:  * 부착은 AWxGameState 생성자의 기본 서브오브젝트다. 클라 GameState 에도 사본이 있으므로, 러너를 권위에서만 띄우는 것은 본 클래스의 책임이다.
44:  */
45: UCLASS()
46: class WXQUEST_API UWxQuestComponent : public UActorComponent
47: {
48: 	GENERATED_BODY()
49: 
50: public:
51: 	UWxQuestComponent(const FObjectInitializer& ObjectInitializer);
52: 
53: 	/** 진행 중인 퀘스트를 정지하고 지정 퀘스트를 활성화한다. 러너가 Running 중엔 에셋 교체가 거부되므로 ST 실행 콜스택 밖에서만 호출한다. */
54: 	void ActivateQuest(UStateTree* QuestAsset);
55: 
56: 	/** ST 태스크 등 러너 실행 콜스택 안에서의 활성화 요청. 재진입이 막히므로 다음 틱에 로드·활성화한다. */
57: 	void RequestActivateQuest(TSoftObjectPtr<UStateTree> QuestAsset);
58: 
59: 	/** SetQuestTitle 태스크 진입점. 저널을 새 퀘스트 제목으로 등록한다(목표는 비움). */
60: 	void SetQuestTitle(const FText& InQuestTitle);
61: 
62: 	/** SetQuestObjective 태스크 진입점. 나중에 걷어갈 핸들을 돌려준다. */
63: 	int32 AddObjective(const FText& InObjectiveText);
64: 
65: 	/** SetQuestObjective 태스크 이탈점. 이미 없는 핸들은 무시한다. */
66: 	void RemoveObjective(int32 ObjectiveHandle);
67: 
68: 	bool HasActiveQuest() const;
69: 	FText GetQuestTitle() const;
70: 
71: 	/** 등록 순서대로 돌려준다. */
72: 	TArray<FText> GetObjectiveTexts() const;
73: 
74: 	/** 저널 등록·목표 갱신·정리 시 발화. HUD 뷰모델이 구독해 현재 값을 pull 한다. */
75: 	UPROPERTY(BlueprintAssignable, Category = "Wx|Quest")
76: 	FWxOnQuestJournalChanged OnJournalChanged;
77: 
78: 	virtual void BeginPlay() override;
79: 
80: private:
81: 	/** 이 콜백 안 재시작은 엔진 재진입 가드에 막힌다. */
82: 	UFUNCTION()
83: 	void HandleStateTreeRunStatusChanged(EStateTreeRunStatus StateTreeRunStatus);
84: 
85: 	void HandleDeferredActivateQuest(TSoftObjectPtr<UStateTree> QuestAsset);
86: 
87: 	void ClearJournal();
88: 
89: 	/** 권위 측 BeginPlay 에서 생성되며 비-권위 머신에선 null 이다. */
90: 	UPROPERTY()
91: 	TObjectPtr<UStateTreeComponent> QuestStateTree;
92: 
93: 	FText QuestTitle;
94: 
95: 	TArray<FWxQuestObjective> Objectives;
96: 
97: 	/** 재사용하지 않으므로 뒤늦은 제거 요청이 엉뚱한 목표를 걷어가지 않는다. */
98: 	int32 NextObjectiveHandle = 0;
99: 
100: 	bool bHasActiveQuest = false;
...
```

## Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h>)
- SHA-256: `0ad336f26e0e1152df08d61302ad78918a636f0e771df2fb6fa9e1f9cf302fa2`
```text
...
11: /**
12:  * 월드 GameState 의 퀘스트 컴포넌트를 찾아 위임한다(레벨에 배치한 트리거 볼륨 등에서 호출).
13:  *
14:  * 러너가 권위에만 존재하므로 서버 권위 호출만 의미가 있고, 컴포넌트가 없거나 비-권위면 내부에서 무시된다.
15:  */
16: UCLASS()
17: class WXQUEST_API UWxQuestLibrary : public UBlueprintFunctionLibrary
18: {
19: 	GENERATED_BODY()
20: 
21: public:
22: 	/** 진행 중인 퀘스트를 정지하고 지정 퀘스트를 시작한다. */
23: 	UFUNCTION(BlueprintCallable, Category = "Wx|Quest", meta = (WorldContext = "WorldContextObject"))
24: 	static void StartQuest(const UObject* WorldContextObject, UStateTree* QuestAsset);
...
```

## Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp>)
- SHA-256: `96d69f50c7a0c872cf50ec7618eeecb30f0ce2159d7d780b8096c818e57dd08a`
```text
...
20: EStateTreeRunStatus FWxStateTreeTask_SetQuestTitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
21: {
22: 	const AActor* Owner = Cast<AActor>(Context.GetOwner());
23: 	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
24: 	if (!QuestComponent)
25: 	{
26: 		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Title: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 제목이 등록되지 않는다."),
27: 			*GetNameSafe(Context.GetOwner()));
28: 		return EStateTreeRunStatus::Failed;
29: 	}
30: 
31: 	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
32: 	QuestComponent->SetQuestTitle(Instance.QuestTitle);
33: 
34: 	return EStateTreeRunStatus::Succeeded;
35: }
...
```

## Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp>)
- SHA-256: `586ec524d97c3856365891e371c537fa692acf516211d0590a1f2d699bd1313b`
```text
...
20: EStateTreeRunStatus FWxStateTreeTask_SetQuestObjective::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
21: {
22: 	const AActor* Owner = Cast<AActor>(Context.GetOwner());
23: 	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
24: 	if (!QuestComponent)
25: 	{
26: 		UE_LOG(LogWxQuest, Warning, TEXT("Set Quest Objective: 오너 %s 에서 퀘스트 컴포넌트를 찾지 못함(퀘스트 러너 밖 조립). 목표가 등록되지 않는다."),
27: 			*GetNameSafe(Context.GetOwner()));
28: 		return EStateTreeRunStatus::Failed;
29: 	}
30: 
31: 	FInstanceDataType& Instance = Context.GetInstanceData(*this);
32: 	Instance.ObjectiveHandle = QuestComponent->AddObjective(Instance.ObjectiveText);
33: 
34: 	return EStateTreeRunStatus::Succeeded;
35: }
36: 
37: void FWxStateTreeTask_SetQuestObjective::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
38: {
39: 	FInstanceDataType& Instance = Context.GetInstanceData(*this);
40: 
41: 	const AActor* Owner = Cast<AActor>(Context.GetOwner());
42: 	if (UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr)
43: 	{
44: 		QuestComponent->RemoveObjective(Instance.ObjectiveHandle);
45: 	}
46: 	Instance.ObjectiveHandle = INDEX_NONE;
47: }
...
```

## Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp>)
- SHA-256: `3934e874288ca75bd5254af20210bffbd94a75fda04e45ca273d5540df0524ab`
```text
...
19: EStateTreeRunStatus FWxStateTreeTask_StartNextQuest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
20: {
21: 	const AActor* Owner = Cast<AActor>(Context.GetOwner());
22: 	UWxQuestComponent* QuestComponent = Owner ? Owner->FindComponentByClass<UWxQuestComponent>() : nullptr;
23: 	if (!QuestComponent)
24: 	{
25: 		return EStateTreeRunStatus::Failed;
26: 	}
27: 
28: 	// 빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다.
29: 	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
30: 	QuestComponent->RequestActivateQuest(Instance.Quest);
31: 
32: 	return EStateTreeRunStatus::Succeeded;
33: }
...
```

## Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp>)
- SHA-256: `141d3575771630720a748b8bea47e6ed227c763b11334e678cdbaf0fbc4d0479`
```text
...
13: FWxStateTreeTask_WaitMoveToTarget::FWxStateTreeTask_WaitMoveToTarget()
14: {
15: 	// 도달 대기 중 같은 상태가 재선택되어도 진행을 끊을 이유가 없다.
16: 	bShouldStateChangeOnReselect = false;
17: }
18: 
19: EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
20: {
21: 	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
22: 
23: 	if (Instance.Target.IsEmpty())
24: 	{
25: 		UE_LOG(LogWxQuest, Warning, TEXT("Wait Move To Target: 도달을 판정할 대상이 지정되지 않음."));
26: 	}
27: 
28: 	return EStateTreeRunStatus::Running;
29: }
30: 
31: EStateTreeRunStatus FWxStateTreeTask_WaitMoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
32: {
33: 	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
34: 
35: 	AActor* Owner = Cast<AActor>(Context.GetOwner());
36: 	if (!Owner)
37: 	{
38: 		return EStateTreeRunStatus::Running;
39: 	}
40: 
41: 	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
42: 	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
43: 	if (!Pawn)
44: 	{
45: 		return EStateTreeRunStatus::Running;
46: 	}
47: 
48: 	const AActor* Target = Cast<AActor>(Instance.Target.SyncFind(Owner));
49: 	if (Target && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= Instance.AcceptRadius)
50: 	{
51: 		return EStateTreeRunStatus::Succeeded;
52: 	}
53: 
54: 	return EStateTreeRunStatus::Running;
55: }
...
```

## Source/WxGame/MVVM/WxViewModel_Quest.cpp
- [저장소 원문](<../../../Source/WxGame/MVVM/WxViewModel_Quest.cpp>)
- SHA-256: `96ceac0b0d6dabeef931d840caea896dd5ef90d5774ea84ae8de1173c0d30756`
```text
...
10: void UWxViewModel_Quest::Initialize(UWxQuestComponent* InQuestComponent)
11: {
12: 	Deinitialize();
13: 	if (!InQuestComponent)
14: 	{
15: 		return;
16: 	}
17: 
18: 	CachedQuestComponent = InQuestComponent;
19: 
20: 	InQuestComponent->OnJournalChanged.AddDynamic(this, &ThisClass::HandleJournalChanged);
21: 
22: 	// 구독 전에 끝난 broadcast 가 있을 수 있으므로 현재 저널로 시드한다.
23: 	HandleJournalChanged();
24: }
...
71: UObject* UWxViewModelResolver_Quest::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
72: {
73: 	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_Quest::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
74: 	{
75: 		return nullptr;
76: 	}
77: 
78: 	const UWorld* World = UserWidget ? UserWidget->GetWorld() : nullptr;
79: 	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
80: 	UWxQuestComponent* QuestComponent = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr;
81: 
82: 	// 퀘스트 소스가 늦게 준비돼도 이 인스턴스에 다시 Initialize 로 주입하는 경로는 없다.
83: 	UWxViewModel_Quest* ViewModel = NewObject<UWxViewModel_Quest>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
84: 	ViewModel->Initialize(QuestComponent);
85: 	return ViewModel;
86: }
...
```
