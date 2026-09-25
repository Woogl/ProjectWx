---
title: "WxQuest — 퀘스트 실행과 저널"
category: topic
sources:
  - "raw/notes/2026-09-25-quest-transition-lifetime.md"
  - "raw/notes/2026-09-23-screen-classes-to-resolvers.md"
  - "raw/notes/2026-09-23-quest-presentation-vm.md"
  - "raw/notes/2026-09-22-current-quests.md"
  - "raw/notes/2026-09-22-current-quest-tasks.md"
  - "raw/notes/2026-09-22-current-game.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, quests]
aliases: ["WxQuest"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "WxQuest는 권위 측의 단일 StateTree 러너와 제목·목표 저널을 제공하며, 실제 진행 내용은 에셋이 구성한다."
---

# WxQuest — 퀘스트 실행과 저널

WxQuest는 권위 측의 단일 StateTree 러너와 제목·목표 저널을 제공하며, 실제 진행 내용은 에셋이 구성한다.

## 실행 모델

`UWxQuestComponent::BeginPlay`는 Owner의 권위를 확인한 뒤 자동 시작하지 않는 StateTreeComponent를 생성한다. `ActivateQuest`는 기존 로직을 정지하고 새 StateTree 참조를 설정한 뒤 시작한다. 따라서 현재 실행 모델은 동시에 여러 퀘스트를 유지하는 목록이 아니라 활성 퀘스트 하나를 교체하는 방식이다.

퀘스트 1개는 StateTree 에셋 1개이고 컴포넌트는 어떤 퀘스트 에셋도 알지 않는다. 수주는 레벨에 배치한 트리거 볼륨 등이 `UWxQuestLibrary::StartQuest`로 에셋을 넘기고, 체인은 `StartNextQuest` 태스크의 소프트 참조가 정한다. 러너가 권위에만 있으므로 비권위에서의 호출은 무시된다.

태스크 안에서 다음 퀘스트로 넘어갈 때는 `RequestActivateQuest`로 다음 틱을 예약한다. 현재 러너 콜스택 중 교체를 피하고, 예약 동안 에셋 수명이 달라져도 실행 시점에 소프트 참조를 다시 로드한다.

이 예약은 이전 러너를 정지하거나 다른 퀘스트를 새로 수주해도 취소되지 않는다. A가 B를 예약한 뒤 콜백 전에 C를 시작하면 남아 있던 예약이 C를 B로 교체할 수 있다. 같은 호출 스택의 재진입 회피와 이전 실행의 예약 무효화는 서로 다른 조건이다(`39f3629a4` [정적 확인](../../raw/notes/2026-09-25-quest-transition-lifetime.md), PIE 미검증).

## 저널과 종료

제목 설정은 목표 목록을 초기화하고 활성 저널을 표시한다. 목표는 문자열 대신 증가하는 핸들로 추가·제거하므로 같은 문구가 여러 번 존재할 수 있다. `SetQuestObjective` 태스크는 상태 진입 때 목표를 등록하고 상태를 떠날 때 자기 핸들로 걷어간다. 핸들을 재사용하지 않아 늦은 제거 요청이 다른 목표를 지우지 않는다. 변경마다 네이티브 멀티캐스트 `OnJournalChanged`를 발행한다(BP 바인딩 불가). 러너 상태가 Running이 아니게 되면 저널을 정리해 완료·실패·교체 경로를 모은다.

## 조립과 범위

WxGame의 GameState가 퀘스트 컴포넌트를 기본 서브오브젝트로 소유한다. Quest·QuestObjective ViewModel 자체는 WxUI의 순수 표시 데이터다. 연결은 WxGame의 `UWxViewModelResolver_Quest`가 맡고, 전용 C++ 위젯 클래스는 두지 않는다(`UWxQuestTracker`는 2026-09-23 제거). `WBP_QuestTracker`의 부모는 `UserWidget`이며 VM은 Resolver 방식으로 생성된다. 리졸버는 위젯마다 Quest VM을 만들고, GameState 컴포넌트의 `OnJournalChanged`에 VM을 소유자로 한 약한 람다를 걸어 SetJournal로 전달한다. 생성 시 현재 저널을 한 번 채우고, 해제는 그 VM의 구독만 `RemoveAll(VM)`로 끊는다. 컴포넌트가 없으면 빈 VM이라 추적기가 숨겨진다. 사용자가 인게임 동작을 확인했다. 목표 행 VM은 Quest VM이 소유한다. 컴포넌트가 늦게 준비될 때 자동 재연결하는 경로는 없다. StateTree에는 제목·목표·도달 대기·다음 퀘스트 태스크를 조합하며 보상·대화·장치 행동은 각 도메인 노드가 담당한다. 제목·목표·다음 퀘스트 태스크는 컨텍스트 오너(GameState)에서 컴포넌트를 찾고 없으면 Failed로 끝난다. 도달 대기는 0번 PlayerController의 Pawn과 Locator 대상의 거리를 틱마다 비교한다.

현재 컴포넌트에는 저널 복제·세이브 복원·복수 퀘스트 목록 구현이 없다. 권위 러너의 존재를 모든 클라이언트에서 동일한 저널을 볼 수 있다는 뜻으로 해석하지 않는다. 레벨 대상의 Locator가 있다고 해서 월드 파티션 로딩과 실제 도달 판정까지 검증된 것도 아니다.

[QuestComponent](../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp)에서 시작하고, [StateTree 태스크](../../../Plugins/WxQuest/Source/WxQuest/Private/Quest)와 실제 퀘스트 에셋을 함께 확인한다.

## 관련 문서

- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [퀘스트 전환 예약의 수명](../../raw/notes/2026-09-25-quest-transition-lifetime.md) — 다음 틱 예약과 새 수주 간 교체 조건
- [대화·퀘스트 화면 클래스 제거와 리졸버 연결](../../raw/notes/2026-09-23-screen-classes-to-resolvers.md) — 현재 저널 연결(리졸버·네이티브 델리게이트)
- [Quest 표시 VM 분리](../../raw/notes/2026-09-23-quest-presentation-vm.md) — 이력: VM의 WxUI 이전. QuestTracker가 구독하던 방식은 위 원자료가 대체한다.

- [근거 1](../../raw/notes/2026-09-22-current-quests.md)
- [퀘스트 계약·StateTree 태스크·저널 표시 조사](../../raw/notes/2026-09-22-current-quest-tasks.md) — 수주·태스크·저널 ViewModel
- [게임 조립·새 게임·부활 정적 조사](../../raw/notes/2026-09-22-current-game.md) — GameState의 컴포넌트 소유

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 태스크·저널 표시 설명은 HEAD `60c324c714b1dab10cd48d36cabad63ace232716` 기준으로 보강했다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다. 저널 표시 연결은 2026-09-23 커밋 `6daf3f804` 기준으로 갱신했다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
