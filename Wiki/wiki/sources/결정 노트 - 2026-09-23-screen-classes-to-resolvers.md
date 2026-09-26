---
type: source
title: "결정 노트 - 2026-09-23-screen-classes-to-resolvers"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "MVVM"
  - "퀘스트"
summary: "UWxDialogueScreen·UWxQuestTracker C++ 위젯 클래스를 제거하고 WBP가 WxGame 리졸버가 연결한 WxUI 뷰모델로 구동되게 한 사용자 결정과 구현 기록."
source_type: decision-note
source_id: src-a4166a32f9143168d056
sha256: ef0aa55fead92e04406ea9314857498ef630e94fee3d4ae87ce744265f7b365f
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-screen-classes-to-resolvers.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-screen-classes-to-resolvers.md"
raw_copy: ".raw/captured/ef0aa55fead92e04406ea9314857498ef630e94fee3d4ae87ce744265f7b365f.md"
claim_ids:
  - clm-0d4dfc9134-c1
  - clm-0d4dfc9134-c2
  - clm-0d4dfc9134-c3
  - clm-0d4dfc9134-c4
key_claims:
  - "사용자는 MVVM을 쓰므로 Widget 클래스를 늘릴 필요가 없다는 이유로 UWxDialogueScreen·UWxQuestTracker C++ 클래스 제거를 요청했다."
  - "2026-09-23 구현에서 대화·퀘스트 WBP는 WxGame 리졸버가 만든 WxUI VM으로 구동되며 리졸버 DestroyInstance는 해당 VM의 구독만 끊는다."
  - "WxQuest FWxOnQuestJournalChanged는 VM 소유 람다를 걸기 위해 네이티브 멀티캐스트로 바뀌고 BlueprintAssignable이 제거되었다."
  - "사용자는 2026-09-23 대화·퀘스트 화면 리졸버 전환의 인게임 동작을 확인했다."
---

# 결정 노트 - 2026-09-23-screen-classes-to-resolvers

- 원본: `.wiki/raw/notes/2026-09-23-screen-classes-to-resolvers.md`
- 원자료 사본: `.raw/captured/ef0aa55fead92e04406ea9314857498ef630e94fee3d4ae87ce744265f7b365f.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-screen-classes-to-resolvers.md` (source: MANUAL, ingested: 2026-09-23).
- 2026-09-23 사용자 요청과 커밋 `570e72562`·`6daf3f804`의 정적 확인 기록이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.
- 같은 날의 dialogue-screen-lifecycle 노트(화면 수명이 연결을 소유)와 quest-presentation-vm 노트 중 QuestTracker가 저널 구독을 맡는다는 설명을 대체한다. [[결정 노트 - 2026-09-23-dialogue-presentation-vm]]의 `WxDialogueScreen` 설명도 이 노트로 대체된다.

## 사람의 판단 원문

- 결정: 두 C++ 위젯 클래스를 제거하고 WBP가 뷰모델로 구동되게 한다.

> 사용자 2026-09-23: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없기 때문이에요."

- 구조는 보스 바와 같은 세 층이다(모델은 도메인, 연결은 WxGame 리졸버, VM은 WxUI).

## 구현 관찰(정적 확인)

- WxUI `UWxViewModel_Dialogue`: 진행 명령 `RequestAdvance()`와 네이티브 단일 델리게이트 `FSimpleDelegate OnAdvanceRequested`를 추가했다. VM은 세션을 모른다.
- `WxViewModelResolver_Dialogue`(신규): 위젯을 Outer로 VM을 만들고 소유 PC의 `UWxDialogueSessionComponent::OnLineChanged`에 `SetLine`을 걸며, `OnAdvanceRequested`를 세션 `Advance`에 잇는다. `DestroyInstance`는 `RemoveAll(VM)`로 그 VM의 구독만 끊는다.
- WxQuest `FWxOnQuestJournalChanged`를 동적 멀티캐스트에서 네이티브 멀티캐스트로 바꾸고 `BlueprintAssignable`을 없앴다(인자 없는 동적 델리게이트에 VM 소유 람다를 걸 수 없음). BP 바인딩 에셋은 없었다.
- `WxViewModelResolver_Quest`(신규): GameState 퀘스트 컴포넌트에 `CreateWeakLambda(VM, ...)`로 저널 반영을 건다. 컴포넌트가 없으면 빈 VM(`bHasActiveQuest=false`).
- WBP: `WBP_DialogueScreen` 부모 → `WxActivatableWidget`, VM 생성 방식 → Resolver, 진행 이벤트 목적지 → `WxViewModel_Dialogue.RequestAdvance`. `WBP_QuestTracker` 부모 → `UserWidget`, Resolver 방식.
- 삭제: `Source/WxGame/UI/WxDialogueScreen.h/.cpp`, `Source/WxGame/UI/WxQuestTracker.h/.cpp`. Build.cs·WxCore 변경 없음.
- WxToolset: `SetEventDestinationWidgetFunction`을 `SetEventDestination(WidgetBlueprint, EventIndex, DestinationPath)`로 일반화(`"Self.함수"` 또는 `"뷰모델이름.함수"`).

## 수명과 동작 차이(로컬 UE 5.8 소스 조사)

- MVVM 소스는 `UMVVMView::Construct`에서 초기화, `Destruct`에서 해제되므로 리졸버 생성·해제는 위젯 `NativeConstruct`·`NativeDestruct`를 따른다.
- CommonUI 스택은 창을 닫을 때 풀로 Slate까지 해제하고, 다시 띄우면 같은 위젯이 새로 Construct되어 새 VM이 현재 대사로 채워진다.
- 이전 화면 클래스는 활성화 여부로 구독·입력을 제한했으나 이제 위젯이 생성된 동안 구독한다. 비활성 화면은 클릭될 수 없고 세션 `Advance`는 활성 대화가 없으면 무시한다.
- 퀘스트 제약 유지: GameState·퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다.

## 검증 범위

- WxEditor Win64 Development 빌드 성공.
- 헤드리스 에디터 이관·재검증 로그(`Saved/Logs/ScreensToResolver.log`, `Saved/Logs/ScreensResolverVerify.log`): 부모·리졸버·VM 클래스·이벤트 목적지 확인, 관련 WBP 4개가 경고를 오류로 취급한 컴파일 통과.
- 인게임:

> 사용자 2026-09-23: "테스트 문제 없습니다"

## 미결정·충돌

- 퀘스트 트래커는 GameState·퀘스트 컴포넌트보다 먼저 생성되면 나중에 연결하지 않는 기존 제약이 남아 있다.

## 관련 주제

- [[UI 표시 구조]]
- [[퀘스트와 대화]]
- [[에디터 도구]]
- [[작업 - dialogue-presentation-vm]]
- [[작업 - quest-presentation-vm]]

## 핵심 주장

- 사용자는 MVVM을 쓰므로 Widget 클래스를 늘릴 필요가 없다는 이유로 UWxDialogueScreen·UWxQuestTracker C++ 클래스 제거를 요청했다. ^c1
- 2026-09-23 구현에서 대화·퀘스트 WBP는 WxGame 리졸버가 만든 WxUI VM으로 구동되며 리졸버 DestroyInstance는 해당 VM의 구독만 끊는다. ^c2
- WxQuest FWxOnQuestJournalChanged는 VM 소유 람다를 걸기 위해 네이티브 멀티캐스트로 바뀌고 BlueprintAssignable이 제거되었다. ^c3
- 사용자는 2026-09-23 대화·퀘스트 화면 리졸버 전환의 인게임 동작을 확인했다. ^c4
