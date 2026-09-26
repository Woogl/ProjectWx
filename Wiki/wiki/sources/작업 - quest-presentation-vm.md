---
type: source
title: "작업 - quest-presentation-vm"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "MVVM"
  - "퀘스트"
summary: "Quest·QuestObjective VM을 WxUI 순수 표시 데이터로 옮기고, 화면 클래스를 거쳐 WxGame 퀘스트 리졸버 세 층 구조로 정리한 완료 작업 기록"
source_type: task-record
source_id: src-8111c305354c561da569
sha256: bf8c3017b5d459b888e5ecfa0c72a0e7425553b1191805f4c3b9af62b210e18f
authority: primary
independence_key: ".agents/workflow/tasks/quest-presentation-vm.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/quest-presentation-vm.md"
raw_copy: ".raw/captured/bf8c3017b5d459b888e5ecfa0c72a0e7425553b1191805f4c3b9af62b210e18f.md"
claim_ids:
  - clm-ffd457a168-c1
  - clm-ffd457a168-c2
  - clm-ffd457a168-c3
  - clm-ffd457a168-c4
key_claims:
  - "퀘스트 추적기는 WxQuest 컴포넌트, WxGame의 UWxViewModelResolver_Quest, WxUI의 Quest VM 세 층으로 구성되며 UWxQuestTracker 화면 클래스는 삭제되었다."
  - "FWxOnQuestJournalChanged는 VM 소유 약한 람다를 걸기 위해 동적 델리게이트에서 네이티브 멀티캐스트 델리게이트로 바뀌었다."
  - "퀘스트 리졸버는 GameState 퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다."
  - "퀘스트 추적기 리졸버 구조는 2026-09-23 사용자가 인게임에서 문제없다고 확인했다."
---

# 작업 - quest-presentation-vm

- 원본: `.agents/workflow/tasks/quest-presentation-vm.md`
- 원자료 사본: `.raw/captured/bf8c3017b5d459b888e5ecfa0c72a0e7425553b1191805f4c3b9af62b210e18f.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

퀘스트 추적기의 VM(Quest·QuestObjective)을 WxUI로 옮겨 순수 표시 데이터로 만들고, 저널 구독 책임을 어디에 둘지 정리한 작업이다. 대화 VM 작업과 같은 경로(화면 클래스 → 리졸버 복귀)를 밟았다. 상태는 완료이며 후속 구조는 2026-09-23 사용자 인게임 확인을 통과했다.

## 합의와 제약 (확정 결정)

- Quest와 QuestObjective VM을 WxUI로 이전하고 순수 표시 데이터로 유지한다.
- Wx 기능 모듈 간 의존성 추가와 WxCore 정의 추가는 하지 않는다.

## 1차 구현: WxUI 이전과 화면 클래스

- Quest·QuestObjective h/cpp를 WxUI Public/Private로 옮기고 `WXUI_API`를 적용했다. `SetJournal`은 표시 필드와 목표별 VM만 갱신한다.
- WxGame `QuestTracker`(화면 클래스)가 GameState 조회·구독·초기 저널 동기화를 맡았고, Quest Resolver를 제거했다. 두 VM 클래스 경로 리다이렉트는 2026-09-24 `[CoreRedirects]` 전체 제거 때 사라졌다(참조 WBP 6개가 누락 클래스 경고 없이 로드, 인게임 표시는 미확인).
- 자동화 `Wx.UI.Quest.TrackerLifecycle`로 초기 시드·중복 목표·목표 제거·복수 추적기·해제·재생성·소스 부재를 검증해 성공 1, 실패 0이었다. 이후 사용자 요청으로 `WxQuestTrackerTest.cpp`와 테스트용 friend 선언을 제거했다.
- 첫 빌드에서 다른 작업의 WxCombat 전방 선언 누락과 테스트 헤더 누락을 발견해 보완했다.

## 최종: 화면 클래스 제거·리졸버 복귀 (2026-09-23)

> 사용자 2026-09-23: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없다."

- 구조: 모델(WxQuest 컴포넌트) / 연결(WxGame 리졸버) / 표시(WxUI VM).
- WxQuest의 `FWxOnQuestJournalChanged`를 동적 델리게이트에서 네이티브 멀티캐스트로 바꾸고 `BlueprintAssignable`을 없앴다(인자 없는 동적 델리게이트는 VM 소유 람다를 걸 수 없음). BP에서 바인딩한 에셋은 없었다.
- `UWxViewModelResolver_Quest`(신규): CreateInstance에서 GameState 퀘스트 컴포넌트에 VM 소유 약한 람다(CreateWeakLambda)를 걸고 현재 저널을 반영한다. 컴포넌트가 없으면 `bHasActiveQuest=false`인 빈 VM이다. DestroyInstance는 `RemoveAll(VM)`.
- `WBP_QuestTracker`: 부모 `WxQuestTracker` → `UserWidget`, VM 생성 방식 → Resolver. `WxQuestTracker.h/.cpp` 삭제.
- 유지되는 제약: GameState·퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다(클라 복제 지연).

## 검증 범위

- AI 빌드: WxEditor Win64 Development 성공(클래스 삭제 후 최종, 종료 코드 0).
- AI 에셋: 부모 변경·리졸버 전환 후 경고를 오류로 취급한 WBP 컴파일 통과, 새 프로세스 재검증 통과, 삭제 클래스 바이너리 참조 0건.
- 런타임 구독·해제는 코드와 엔진 수명으로만 확인했다(정적).
- 사람 인게임(2026-09-23): 퀘스트 수주·목표 갱신·완료 시 추적기 표시를 확인했다.

> 사용자 2026-09-23: "테스트 문제 없습니다."

- 인간 코드 리뷰의 별도 승인 기록은 없다. StateTree 퀘스트 진행과 멀티플레이는 1차 검증 범위 밖이라고 기록되어 있다.

## 미결정·충돌

- 위젯이 GameState·퀘스트 컴포넌트보다 먼저 생기면 늦게 연결하지 않는 제약은 해결되지 않고 유지된다.

## 관련 주제

- [[퀘스트와 대화]]
- [[UI 표시 구조]]
- [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]

## 핵심 주장

- 퀘스트 추적기는 WxQuest 컴포넌트, WxGame의 UWxViewModelResolver_Quest, WxUI의 Quest VM 세 층으로 구성되며 UWxQuestTracker 화면 클래스는 삭제되었다. ^c1
- FWxOnQuestJournalChanged는 VM 소유 약한 람다를 걸기 위해 동적 델리게이트에서 네이티브 멀티캐스트 델리게이트로 바뀌었다. ^c2
- 퀘스트 리졸버는 GameState 퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다. ^c3
- 퀘스트 추적기 리졸버 구조는 2026-09-23 사용자가 인게임에서 문제없다고 확인했다. ^c4
