---
title: "대화·퀘스트 화면 클래스 제거와 리졸버 연결"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, dialogue, quests, lifecycle, architecture, editor]
summary: "사용자 요청으로 UWxDialogueScreen·UWxQuestTracker를 제거하고, WBP가 WxGame 리졸버가 연결한 WxUI 뷰모델로 구동되게 했다. 대화 진행 입력은 VM 명령 델리게이트로 세션에 전달하고, 퀘스트 저널 델리게이트는 네이티브로 바꿨다. 사용자가 인게임 동작을 확인했다."
---

# 대화·퀘스트 화면 클래스 제거와 리졸버 연결

2026-09-23 사용자 요청과 커밋 `570e72562`·`6daf3f804`의 정적 확인이다. 작업 기록은 [Dialogue VM](../../../.agents/workflow/tasks/dialogue-presentation-vm.md)과 [Quest VM](../../../.agents/workflow/tasks/quest-presentation-vm.md)에 있다. 같은 날의 `2026-09-23-dialogue-screen-lifecycle.md`(화면 수명이 연결을 소유)와 `2026-09-23-quest-presentation-vm.md` 중 QuestTracker가 저널 구독을 맡는다는 설명을 대체한다.

## 사용자 결정

- 두 C++ 위젯 클래스를 제거하고 WBP가 뷰모델로 구동되게 한다.
- 이유(사용자 원문): "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없기 때문이에요."
- 구조는 보스 바와 같은 세 층이다. 모델은 도메인, 연결은 WxGame 리졸버, VM은 WxUI다.

## 구현 (정적 확인)

- **WxUI `UWxViewModel_Dialogue`**: 진행 명령 `RequestAdvance()`(BlueprintCallable)와 네이티브 단일 델리게이트 `FSimpleDelegate OnAdvanceRequested`를 추가했다. VM은 세션을 모른다.
- **`Source/WxGame/MVVM/WxViewModelResolver_Dialogue.h/.cpp`** (신규)
  - `CreateInstance`: 위젯을 Outer로 VM을 만든다. 소유 PC의 `UWxDialogueSessionComponent::OnLineChanged`(동적)에 `AddUniqueDynamic(VM, &SetLine)`으로 건다. 현재 대사를 한 번 채우고, `OnAdvanceRequested`를 `BindUObject(Session, &Advance)`로 잇는다. 세션이 없으면 빈 VM이다.
  - `DestroyInstance`: VM의 Outer 위젯에서 세션을 다시 찾아 `RemoveAll(VM)`로 그 VM의 구독만 끊는다.
- **WxQuest `FWxOnQuestJournalChanged`**: 동적 멀티캐스트에서 네이티브 멀티캐스트로 바꾸고 `BlueprintAssignable`을 없앴다. 인자 없는 동적 델리게이트에는 VM을 소유자로 한 람다를 걸 수 없기 때문이다. BP 바인딩 에셋은 없었다(Content 바이너리 검색 0건).
- **`Source/WxGame/MVVM/WxViewModelResolver_Quest.h/.cpp`** (신규)
  - `CreateInstance`: 위젯을 Outer로 VM을 만든다. GameState 퀘스트 컴포넌트에 `CreateWeakLambda(VM, ...)`로 저널 반영을 걸고, 현재 저널을 한 번 반영한다. 컴포넌트가 없으면 빈 VM(`bHasActiveQuest=false`)이다.
  - `DestroyInstance`: `RemoveAll(VM)`로 그 VM의 구독만 끊는다.
- **WBP**
  - `WBP_DialogueScreen`: 부모 `WxDialogueScreen` → `WxActivatableWidget`. VM 생성 방식 Create Instance → Resolver(`WxViewModelResolver_Dialogue`). 진행 이벤트 `AdvanceButton.OnClicked`의 목적지 `Self.RequestAdvance` → `WxViewModel_Dialogue.RequestAdvance`. 표시 바인딩 3개는 유지했다.
  - `WBP_QuestTracker`: 부모 `WxQuestTracker` → `UserWidget`. VM 생성 방식 Create Instance → Resolver(`WxViewModelResolver_Quest`). 바인딩 2개는 유지했다.
- **삭제**: `Source/WxGame/UI/WxDialogueScreen.h/.cpp`, `Source/WxGame/UI/WxQuestTracker.h/.cpp`. Build.cs·WxCore 변경 없음.
- **WxToolset** (`570e72562`): `SetEventDestinationWidgetFunction(WidgetBlueprint, EventIndex, FunctionName)`을 `SetEventDestination(WidgetBlueprint, EventIndex, DestinationPath)`로 일반화했다. 경로는 `"Self.함수"` 또는 `"뷰모델이름.함수"`이며 기존 `ResolvePropertyPath`로 해석하고, 끝 필드가 BlueprintCallable 함수인지 검사한다.

## 수명과 동작 차이 (로컬 UE 5.8 소스)

- MVVM 소스는 기본 설정에서 `UMVVMView::Construct`에서 초기화되고(`MVVMView.cpp` 93), `Destruct`에서 해제된다(146). 따라서 리졸버의 `CreateInstance`·`DestroyInstance`는 위젯의 `NativeConstruct`·`NativeDestruct`를 따른다.
- CommonUI 스택은 창을 닫을 때 `GeneratedWidgetsPool.Release(Widget, true)`로 Slate까지 해제한다(`CommonActivatableWidgetContainer.cpp` 244). 다시 띄우면 풀의 같은 위젯이 새로 Construct되어, 새 VM이 현재 대사로 채워진다.
- 이전 화면 클래스는 활성화 여부로 구독과 진행 입력을 제한했다. 이제는 위젯이 생성되어 있는 동안 구독한다. 비활성 화면은 보이지 않아 클릭될 수 없고, 세션 `Advance`는 활성 대화가 없으면 무시한다.
- 퀘스트는 기존 제약을 유지한다. GameState·퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다.

## 검증

- WxEditor Win64 Development 빌드 성공.
- 헤드리스 에디터 이관: `Saved/Logs/ScreensToResolver.log`. 클래스 삭제 후 새 프로세스 재검증: `Saved/Logs/ScreensResolverVerify.log`. 부모·리졸버·VM 클래스·이벤트 목적지를 확인했고, `WBP_DialogueScreen`·`WBP_QuestTracker`·`WBP_QuestObjective`·`WBP_GameLayout`이 경고를 오류로 취급한 컴파일을 통과했다.
- 인게임: 사용자가 2026-09-23 확인했다("테스트 문제 없습니다").
