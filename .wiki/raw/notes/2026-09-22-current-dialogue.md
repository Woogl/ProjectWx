---
title: "대화 세션과 StateTree 연결 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, dialogue]
summary: "대화 세션과 StateTree 연결 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 대화 세션과 StateTree 연결 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs>)
- SHA-256: `d08672b118b09118325adab76f4e76fc1ac33e81c3182d899109463a92b2528c`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"Core",
14: 			"CoreUObject",
15: 			"Engine",
16: 			"GameplayAbilities",
17: 			"GameplayTags",
18: 			"StateTreeModule",
19: 			"WxCore",
20: 		});
21: 	}
22: }
```
## Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp>)
- SHA-256: `9852b1e0a8a01e5248dedb13b3568dff1684359b7510ca5619451588842a707a`
```text
...
72: }
73: 
74: void UWxDialogueSessionComponent::Advance()
75: {
76: 	if (!HasActiveDialogue())
77: 	{
78: 		return;
79: 	}
80: 
81: 	const FWxDialogueTableRow* Row = FindCurrentRow();
82: 	if (!Row)
83: 	{
84: 		// 세션 도중 테이블이 갈린 경우다(에디터 재임포트). 이어갈 곳이 없으니 접는다 — 남겨 두면 진행도 종료도 없는 세션이 굳는다.
85: 		EndDialogue(/*bCompleted*/ false);
86: 		return;
87: 	}
88: 
89: 	const FName NextRowName = Row->NextRow;
90: 	if (NextRowName.IsNone())
91: 	{
92: 		// EndDialogue 에 true 가 가는 유일한 경로다.
...
125: }
126: 
127: void UWxDialogueSessionComponent::ClientStartDialogue_Implementation(const FDataTableRowHandle& StartRow, AActor* Target)
128: {
129: 	// 겹쳐 열리는 경로가 실재한다(퀘스트 트리의 Play Dialogue 는 상호작용 차단 태그의 게이트를 거치지 않는다).
130: 	// 앞 세션을 그대로 덮으면 그쪽 태그·카메라를 되돌릴 주체가 사라지므로, 새 세션을 열기 전에 접는다.
131: 	if (HasActiveDialogue())
132: 	{
133: 		EndDialogue(/*bCompleted*/ false);
134: 	}
135: 
136: 	const AController* Controller = Cast<AController>(GetOwner());
137: 	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
138: 	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
139: 	if (!ASC)
140: 	{
141: 		UE_LOG(LogWxDialogue, Warning, TEXT("ClientStartDialogue: 폰 ASC 가 없어 대화를 열지 않는다(테이블 %s / 행 %s). 대화 창을 띄울 신호가 없다."),
142: 			*GetNameSafe(StartRow.DataTable), *StartRow.RowName.ToString());
143: 		return;
144: 	}
145: 
...
158: 	// 대화 창을 여는 관찰자는 여기서 현재 대사를 pull 해 시드하므로, 세션이 다 채워진 뒤인 이 자리에서 올린다.
159: 	// 이 태그를 loose 로 쓰는 곳은 이 컴포넌트뿐이고, 소비자(UI 매니저)는 0↔비0 전이만 듣기 때문에 카운트가 1 이라도 남으면 대화 창이 영영 닫히지 않는다.
160: 	ASC->SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 1);
161: 	TaggedAbilitySystem = ASC;
162: 
163: 	BeginDialogueCamera();
164: 	ApplyCurrentPose();
165: }
166: 
167: void UWxDialogueSessionComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
168: {
169: 	if (HasActiveDialogue())
170: 	{
171: 		// 대화 중 사망·리스폰이다. 읽던 대사가 남았으므로 완주가 아니다.
172: 		EndDialogue(/*bCompleted*/ false);
173: 	}
174: }
175: 
176: bool UWxDialogueSessionComponent::EnterRow(FName RowName)
177: {
178: 	const UDataTable* Table = CurrentStartRow.DataTable;
...
215: }
216: 
217: void UWxDialogueSessionComponent::EndDialogue(bool bCompleted)
218: {
219: 	CurrentStartRow = FDataTableRowHandle();
220: 	CurrentRowName = NAME_None;
221: 	CurrentTarget.Reset();
222: 
223: 	// 발행과 대칭으로 절대값 0 을 지정한다 — 감산이면 겹침 이력에 따라 잔량이 남을 수 있다.
224: 	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
225: 	{
226: 		ASC->SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 0);
227: 	}
228: 	TaggedAbilitySystem.Reset();
229: 
230: 	// 진행 중인 포즈 스트리밍은 접지 않는다 — 마지막 대사의 자세가 늦게 도착했을 뿐이다.
231: 	EndDialogueCamera();
232: 
233: 	// 멤버를 먼저 비우고 사본으로 발화한다 — 발화 도중 리스너가 새 대화를 열어 건 바인딩까지 뒤의 Clear 가 지우지 않게 한다.
234: 	FWxOnDialogueEnded Ended = MoveTemp(OnDialogueEnded);
235: 	OnDialogueEnded.Clear();
...
286: }
287: 
288: void UWxDialogueSessionComponent::EndDialogueCamera()
289: {
290: 	AActor* CameraActor = DialogueCamera.Get();
291: 	if (!CameraActor)
292: 	{
293: 		return;
294: 	}
295: 	DialogueCamera.Reset();
296: 
297: 	// bLockOutgoing=true: 블렌드 시작 시점의 출발 POV 를 고정한다. 복귀 도중 대화 카메라가 정리돼도 화면이 튀지 않는다.
298: 	if (APlayerController* PlayerController = GetLocalPlayerController())
299: 	{
300: 		PlayerController->SetViewTargetWithBlend(PlayerController->GetPawn(), CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.f, true);
301: 	}
302: 
303: 	// 블렌드가 끝날 때까지는 남아 있어야 하므로 즉시 파괴하지 않고 수명만 준다.
304: 	CameraActor->SetLifeSpan(CameraBlendTime + 1.f);
305: }
306: 
307: void UWxDialogueSessionComponent::ApplyCurrentPose()
308: {
309: 	// 한 자세로 여러 대사를 이어가는 것이 기본값이라 매 행에 같은 몽타주를 반복 기입시키지 않는다.
310: 	// 앞 대사가 띄운 스트리밍도 그대로 둔다. 그것이 곧 이어갈 "직전 포즈"다.
311: 	const FWxDialogueTableRow* Row = FindCurrentRow();
312: 	if (!Row || Row->TargetPose.IsNull())
313: 	{
314: 		return;
315: 	}
316: 
317: 	// CancelHandle 은 지연 콜백 큐에 들어간 완료 델리게이트까지 취소한다.
318: 	if (PoseLoadHandle.IsValid())
319: 	{
320: 		PoseLoadHandle->CancelHandle();
321: 		PoseLoadHandle.Reset();
322: 	}
323: 
324: 	PendingPose = Row->TargetPose;
325: 	PendingPoseTarget = CurrentTarget;
```
## Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp>)
- SHA-256: `6aa0b830e98d1982e171237ed0f0001c6961162654b264814c7502fef2377165`
```text
...
19: }
20: 
21: EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
22: {
23: 	FInstanceDataType& Instance = Context.GetInstanceData(*this);
24: 
25: 	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
26: 	{
27: 		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 시작 행이 지정되지 않음(StartRow)."));
28: 		return EStateTreeRunStatus::Failed;
29: 	}
30: 
31: 	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
32: 	UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
33: 	if (!Session)
34: 	{
35: 		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 0번 컨트롤러에서 대화 세션을 찾지 못함."));
36: 		return EStateTreeRunStatus::Failed;
37: 	}
38: 
39: 	Session->StartDialogueRow(Instance.StartRow, nullptr);
40: 
41: 	// 소유 클라와 권위가 같은 머신이라 세션은 위 호출 안에서 열린다.
42: 	if (!Session->HasActiveDialogue())
43: 	{
44: 		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 대화를 열지 못함(사유는 직전 경고): %s"), *Instance.StartRow.RowName.ToString());
45: 		return EStateTreeRunStatus::Failed;
46: 	}
47: 
48: 	// 약한 실행 컨텍스트를 넘기는 것이 엔진이 제시하는 방식이라 여기선 람다를 쓴다.
49: 	// 신호는 발화와 함께 비워지므로 상태를 먼저 떠난 노드의 등록도 남지 않는다(그 경우 이 컨텍스트가 무효라 무시된다).
50: 	Session->OnDialogueEnded.AddLambda([WeakContext = Context.MakeWeakExecutionContext()](bool bCompleted)
51: 	{
52: 		if (bCompleted)
53: 		{
54: 			WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
55: 		}
56: 	});
57: 
58: 	return EStateTreeRunStatus::Running;
59: }
60: 
61: #if WITH_EDITOR
62: FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
63: {
64: 	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
65: 	check(InstanceData);
66: 
67: 	const FText RowText = InstanceData->StartRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->StartRow.RowName);
68: 	return FText::Format(INVTEXT("대화 재생 ({0})"), RowText);
```
## Source/WxGame/Controller/WxPlayerController.cpp
- [저장소 원문](<../../../Source/WxGame/Controller/WxPlayerController.cpp>)
- SHA-256: `af122cc3eed03e7698093e252cd835191c383ca7422b2cd2471394aaa5397b03`
```text
...
9: #include "WxDialogueSessionComponent.h"
10: 
11: AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
12: 	: Super(ObjectInitializer)
13: {
14: 	CheatClass = UWxCheatManager::StaticClass();
15: 
16: 	InventoryComponent = CreateDefaultSubobject<UWxInventoryComponent>(TEXT("InventoryComponent"));
17: 	InteractionScannerComponent = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScannerComponent"));
18: 	DialogueSessionComponent = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSessionComponent"));
19: 	PlayerLayoutComponent = CreateDefaultSubobject<UWxPlayerLayoutComponent>(TEXT("PlayerLayoutComponent"));
20: }
```
