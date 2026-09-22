---
title: "퀘스트 러너·저널 수명 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, quests]
summary: "퀘스트 러너·저널 수명 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 퀘스트 러너·저널 수명 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs>)
- SHA-256: `dca350a8cc942eed38d1afad3f86c5c12628e1b034834f86d41ce7e8b0989741`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"Core",
14: 			"CoreUObject",
15: 			"Engine",
16: 			"StateTreeModule",
17: 			"UniversalObjectLocator",
18: 			"WxCore",
19: 		});
20: 
21: 		PrivateDependencyModuleNames.AddRange(new string[]
22: 		{
23: 			"GameplayStateTreeModule",
24: 		});
25: 	}
26: }
```
## Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp
- [저장소 원문](<../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp>)
- SHA-256: `58c2210dd37d7a92e94fffdf9ebd3d25614b9e6754d9a6751a74f50ee012bb72`
```text
...
16: }
17: 
18: void UWxQuestComponent::ActivateQuest(UStateTree* QuestAsset)
19: {
20: 	if (!QuestAsset)
21: 	{
22: 		return;
23: 	}
24: 
25: 	// 러너는 권위에서만 생성되므로 비-권위 머신의 호출은 자연히 노옵이다.
26: 	if (!QuestStateTree)
27: 	{
28: 		return;
29: 	}
30: 
31: 	// 정지가 저널 정리(HandleStateTreeRunStatusChanged)를 발화시키고, 새 퀘스트 진행이 저널을 다시 채운다.
32: 	FStateTreeReference Quest;
33: 	Quest.SetStateTree(QuestAsset);
34: 	QuestStateTree->StopLogic(TEXT("ActivateQuest"));
35: 	QuestStateTree->SetStateTreeReference(Quest);
36: 	QuestStateTree->StartLogic();
37: }
38: 
39: void UWxQuestComponent::RequestActivateQuest(TSoftObjectPtr<UStateTree> QuestAsset)
40: {
41: 	if (QuestAsset.IsNull())
42: 	{
43: 		return;
44: 	}
45: 
46: 	// 타이머 대기 중 GC 로 로드가 풀릴 수 있어 포인터가 아닌 소프트 참조를 넘기고 실행 시점에 로드한다.
47: 	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UWxQuestComponent::HandleDeferredActivateQuest, QuestAsset));
48: }
49: 
50: void UWxQuestComponent::SetQuestTitle(const FText& InQuestTitle)
51: {
52: 	QuestTitle = InQuestTitle;
53: 	Objectives.Reset();
54: 	bHasActiveQuest = true;
55: 	OnJournalChanged.Broadcast();
56: }
57: 
58: int32 UWxQuestComponent::AddObjective(const FText& InObjectiveText)
59: {
60: 	FWxQuestObjective& Objective = Objectives.AddDefaulted_GetRef();
61: 	Objective.Handle = NextObjectiveHandle++;
62: 	Objective.Text = InObjectiveText;
63: 
64: 	OnJournalChanged.Broadcast();
65: 
66: 	return Objective.Handle;
67: }
68: 
69: void UWxQuestComponent::RemoveObjective(int32 ObjectiveHandle)
70: {
71: 	// 제목 교체·저널 정리가 목록을 통째로 비운 뒤라면 이미 사라진 핸들이 들어온다.
72: 	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
73: 	{
74: 		if (Objectives[Index].Handle == ObjectiveHandle)
75: 		{
76: 			Objectives.RemoveAt(Index);
...
103: }
104: 
105: void UWxQuestComponent::BeginPlay()
106: {
107: 	Super::BeginPlay();
108: 
109: 	AActor* Owner = GetOwner();
110: 	if (!Owner || !Owner->HasAuthority())
111: 	{
112: 		return;
113: 	}
114: 
115: 	// 실행할 에셋은 ActivateQuest 가 그때그때 지정하므로 자동 시작을 끈다.
116: 	QuestStateTree = NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"));
117: 	QuestStateTree->SetStartLogicAutomatically(false);
118: 	QuestStateTree->RegisterComponent();
119: 	QuestStateTree->OnStateTreeRunStatusChanged.AddDynamic(this, &UWxQuestComponent::HandleStateTreeRunStatusChanged);
120: }
121: 
122: void UWxQuestComponent::HandleStateTreeRunStatusChanged(EStateTreeRunStatus StateTreeRunStatus)
123: {
124: 	if (StateTreeRunStatus != EStateTreeRunStatus::Running)
125: 	{
126: 		ClearJournal();
127: 	}
128: }
129: 
130: void UWxQuestComponent::HandleDeferredActivateQuest(TSoftObjectPtr<UStateTree> QuestAsset)
131: {
132: 	ActivateQuest(QuestAsset.LoadSynchronous());
133: }
134: 
135: void UWxQuestComponent::ClearJournal()
136: {
137: 	if (!bHasActiveQuest)
138: 	{
139: 		return;
140: 	}
```
