---
title: "대화 정의·호스트·행 데이터와 세션 계약 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, dialogue]
summary: "대화 정의 컴포넌트·호스트 액터·대화 행·세션 공개 계약의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: 60c324c714b1dab10cd48d36cabad63ace232716
---

# 대화 정의·호스트·행 데이터와 세션 계약 조사

조사일: 2026-09-22. 기준 HEAD: `60c324c714b1dab10cd48d36cabad63ace232716`, 대상 파일은 작업 트리 변경 없음. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h>)
- SHA-256: `d7e9d11c73ed338299e13eaf0227ab175a4201db28ecbe1d30b1f7c1c0787018`
```text
...
10: /**
11:  * 어느 노드에서 대화를 시작할지만 보유하고, 세션 진행은 상호작용한 플레이어 측(UWxDialogueSessionComponent)이 소유한다.
12:  *
13:  * 상호작용 계약은 호스트 액터(AWxDialogueActor)가 들고 이 컴포넌트로 넘긴다.
14:  * 그래서 이 컴포넌트를 아무 액터에 붙여도 말을 걸 수 있게 되지는 않는다.
15:  * 같은 이유로 Add Component 메뉴에 노출하지 않는다 — AWxDialogueActor 가 네이티브 서브오브젝트로 만든다.
16:  */
17: UCLASS()
18: class WXDIALOGUE_API UWxDialogueComponent : public UActorComponent
19: {
20: 	GENERATED_BODY()
21: 
22: public:
23: 	const FDataTableRowHandle& GetStartRow() const;
24: 
25: 	FText GetTalkPrompt() const;
26: 
27: 	/** 서버 권위에서 호스트 액터의 상호작용 응답이 부른다. */
28: 	void StartDialogueWith(AActor* Interactor);
29: 
30: protected:
31: 	/** 비우면 대화가 시작되지 않는다. */
32: 	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow", WxPreviewRow = "true"))
33: 	FDataTableRowHandle StartRow;
34: 
35: 	/** 상호작용 프롬프트에 쓰인다. */
36: 	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
37: 	FText SpeakerName;
38: };
```

## Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp>)
- SHA-256: `6dc4f4c75e3c115f5d003b8e71332652a62c79577ff37d1248c0082602cac7ba`
```text
...
20: void UWxDialogueComponent::StartDialogueWith(AActor* Interactor)
21: {
22: 	const APawn* Pawn = Cast<APawn>(Interactor);
23: 	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
24: 	UWxDialogueSessionComponent* Session = Controller ? Controller->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
25: 	if (!Session)
26: 	{
27: 		// 세션 컴포넌트는 AWxPlayerController 의 기본 서브오브젝트라, 다른 컨트롤러 클래스면 이 갈래로 떨어진다.
28: 		UE_LOG(LogWxDialogue, Warning, TEXT("StartDialogueWith: 대화 세션 컴포넌트를 찾지 못함(대상 %s / Interactor %s)."),
29: 			*GetNameSafe(GetOwner()), *GetNameSafe(Interactor));
30: 		return;
31: 	}
32: 
33: 	Session->StartDialogue(this);
34: }
```

## Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h>)
- SHA-256: `d5c21be6dfdea8fcd10805f239512aa5b11d7fe6733ce4db4ba5cbdb42e0cfee`
```text
...
13: /**
14:  * 말을 걸 수 있는 대상의 공통 호스트.
15:  *
16:  * 루트를 만들지 않는다 — 파생이 저마다 다른 몸통을 세운다(NPC 는 캡슐+스켈레탈, 말 거는 물체는 메시).
17:  * 계약은 액터 전용이라 컴포넌트가 들지 않는다.
18:  */
19: UCLASS(Abstract)
20: class WXDIALOGUE_API AWxDialogueActor : public AActor, public IWxInteractable
21: {
22: 	GENERATED_BODY()
23: 
24: public:
25: 	AWxDialogueActor();
26: 
27: 	//~ Begin IWxInteractable
28: 	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) override;
29: 	virtual FText GetInteractionPrompt() const override;
30: 	//~ End IWxInteractable
31: 
32: 	/** 대사 포즈를 얹을 메시. 스켈레탈 메시가 없는 대상은 비운다. */
33: 	virtual USkeletalMeshComponent* GetPoseMesh() const;
34: 
35: protected:
36: 	UPROPERTY(VisibleAnywhere, Category = "Wx|Dialogue")
37: 	TObjectPtr<UWxDialogueComponent> DialogueComponent;
38: };
```

## Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h>)
- SHA-256: `9bd496635ec82914b8c98f66e1c529dd6e94159e584d0bc2155f0aa2176317db`
```text
...
11: /**
12:  * 대화 노드 하나 = 대사 한 줄 = 행 하나. 대화 1편 = 테이블 1개.
13:  * 대사를 출력한 뒤 NextRow 로 이어가고, 가리키는 곳이 없으면 대화가 끝난다.
14:  */
15: USTRUCT(BlueprintType)
16: struct FWxDialogueTableRow : public FTableRowBase
17: {
18: 	GENERATED_BODY()
19: 
20: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
21: 	FText Speaker;
22: 
23: 	/** 모든 행이 채워야 한다 — 종료는 NextRow=None 으로 표시하며, 비어 있으면 잘못된 행으로 보고 경고와 함께 대화를 접는다. */
24: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue", meta = (MultiLine = "true"))
25: 	FText Line;
26: 	
27: 	/**
28: 	 * 이 대사 동안 NPC가 취할 포즈. 비우면 직전 포즈를 그대로 둔다.
29: 	 *
30: 	 * 소프트 참조다 — 대화 테이블은 배치 NPC 가 하드로 붙잡아 레벨과 함께 상주하므로, 하드로 두면 말을 걸기 전부터 이 테이블 모든 행의 몽타주가 메모리에 올라온다.
31: 	 * 세션이 대사를 넘길 때 비동기로 스트리밍한다.
32: 	 */
33: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
34: 	TSoftObjectPtr<UAnimMontage> TargetPose;
35: 
36: 	/** None 이면 대화 종료. */
37: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
38: 	FName NextRow;
39: };
```

## Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h
- [저장소 원문](<../../../Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h>)
- SHA-256: `7af1520b1414c9499a315d753000cb1eaae48f68545a4f1b82f5312ca6713d6d`
```text
...
22: /**
23:  * AWxPlayerController 생성자의 기본 서브오브젝트로 붙는다.
24:  *
25:  * 대화 대상은 비소유 액터라 Client RPC 를 쏠 수 없으므로, 클라 UI 로 가는 전달은 PC 측인 본 컴포넌트가 소유한다.
26:  * 그 RPC 때문에 복제 컴포넌트여야 한다.
27:  * 세션(현재 노드·라인)은 표시 전용 로컬 상태라 소유 클라가 진행을 소유하며 서버 검증은 없다. 대화가 게임 상태를 바꾸게 되면 그때 서버측으로 옮긴다.
28:  *
29:  * 대화는 뜻을 해석하지도 기록을 남기지도 않는다 — 그 의미(퀘스트 수주 등)는 종료를 기다린 쪽이 판정한다.
30:  * v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제라 권위 측 소비자가 이 로컬 상태를 직접 읽는다.
31:  *
32:  * UI 는 모른다 — 대사가 바뀌면 델리게이트로 발행해 뷰모델이 받아 가고, 세션이 열리고 닫힌 사실은 폰 ASC 의 State.Dialogue 태그가 알린다.
33:  * 대화 창을 여닫는 것은 그 태그를 보는 쪽(UI 매니저)의 몫이라 UI 를 위한 시작·종료 델리게이트는 두지 않는다.
34:  * 그래서 폰 ASC 는 세션의 전제다 — 태그를 올릴 곳이 없으면 창을 띄울 방법도 없으므로 세션을 열지 않는다.
35:  * 같은 이유로 빙의가 바뀌면 세션을 접는다 — 새 폰 ASC 에는 태그가 없어 창이 설 수 없고, 남겨 두면 넘길 수도 끝낼 수도 없다.
36:  *
37:  * 반면 대화 카메라는 여기서 직접 든다. 컨트롤러에 붙어 있어 뷰 타겟에 손이 닿고, 구도의 재료인 대상·시작·종료를 이미 다 알기 때문이다.
38:  * 대화 동안에는 전용 카메라를 세워 뷰 타겟을 그리로 넘긴다 — 게임플레이 카메라는 플레이어 등 뒤에 매여 있어, 두 사람을 잇는 선에서 크게 비껴선 구도를 잡을 수 없다.
39:  *
40:  * 대상의 포즈도 같은 이유로 여기서 든다. 어느 대사에 어떤 자세인지는 대화 데이터가 이미 들고 있어, 대사를 넘기는 이 자리가 그것을 갈아끼울 유일한 지점이다.
41:  * 다만 카메라와 달리 되돌리지 않는다 — 대화가 끝나도 대상은 마지막 자세로 남고, 다음 대사나 다음 대화가 그것을 갈아끼운다. 그래서 세션은 무엇을 재생했는지 기억할 필요가 없다.
42:  */
...
54: 	/** 서버 권위 진입점. 상호작용 응답이 대상의 대화 정의를 넘겨 호출하면 소유 클라에서 세션이 열린다. */
55: 	void StartDialogue(UWxDialogueComponent* Dialogue);
56: 
57: 	/**
58: 	 * 대화 정의 컴포넌트 없이 행을 직접 지정하는 진입점. 퀘스트 ST 의 Play Dialogue 태스크처럼 대사를 고르는 쪽이 액터가 아닐 때 쓴다.
59: 	 * Target 은 카메라 전환·포즈용일 뿐이라 비워도 되며(나레이션), 그때는 뷰 타겟이 플레이어 폰에 머문다.
60: 	 */
61: 	void StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target);
62: 
63: 	/** 뷰의 대사 넘기기 요청. NextRow 를 따라가고, 더 없으면 종료한다. */
64: 	void Advance();
65: 
66: 	bool HasActiveDialogue() const;
67: 
68: 	FText GetCurrentSpeaker() const;
69: 
70: 	FText GetCurrentLine() const;
71: 
72: 	UPROPERTY(BlueprintAssignable, Category = "Wx")
73: 	FWxOnDialogueLineChanged OnLineChanged;
74: 
75: 	/**
76: 	 * 대화가 끝나면 한 번 발화하고 스스로 비워진다. 종료를 기다리는 쪽('Play Dialogue' 태스크)이 대화를 연 직후 붙인다.
77: 	 * bCompleted 는 마지막 행까지 읽고 끝났는지다 — 중단(테이블 갈림·행 해석 실패·빙의 전이·대화 겹침)이면 false 다.
78: 	 * 대화의 의미를 판정하는 쪽이 읽지 않은 대사에 전진하지 않으려면 이 값을 봐야 한다.
79: 	 */
80: 	FWxOnDialogueEnded OnDialogueEnded;
...
```
