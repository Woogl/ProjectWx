---
title: "Quest 표시 VM과 화면 연결의 분리"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, quests, lifecycle]
summary: "Quest·QuestObjective는 WxUI의 표시 데이터이며 WxGame QuestTracker가 저널 구독을 소유한다."
---

# Quest 표시 VM과 화면 연결의 분리

2026-09-23 사용자는 Dialogue와 같은 방식으로 Quest·QuestObjective도 WxUI로 완전히 이전하도록 요청했다. 미커밋 구현의 정적 확인이며, 이전 퀘스트 조사에서 VM이 직접 저널을 구독한다는 설명을 대체한다.

- `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Quest.h` 및 Private 대응 cpp: SetJournal은 활성 여부·제목·목표 텍스트를 표시 필드로 반영하고 목표별 VM을 생성한다. 게임 컴포넌트 참조와 구독은 없다.
- 같은 위치의 `WxViewModel_QuestObjective`: 목표 한 줄의 텍스트와 FieldNotify만 유지한다. 기존 클래스 자체를 이전하며 추가 래퍼 VM은 없다.
- `Source/WxGame/UI/WxQuestTracker.h/.cpp`: 일반 UserWidget의 NativeConstruct에서 Super 호출 뒤 MVVM 소스를 얻고 GameState의 QuestComponent를 구독한다. 현재 저널로 초기화하며 NativeDestruct에서는 Super 호출 전에 구독을 해제한다. 재생성 시 새 VM에 현재 저널을 주입한다.
- Quest Resolver는 제거하고 WBP_QuestTracker는 Create Instance로 전환한다. 두 VM의 옛 WxGame 경로는 Config/DefaultEngine.ini의 ClassRedirects로 보존한다.
- WxCore에 타입·게임 로직을 추가하지 않고 기능 모듈 간 의존성도 추가하지 않는다. GameState나 컴포넌트가 생성 이후 늦게 제공될 때 자동 재연결하는 기능은 기존과 동일하게 없다.

에셋과 런타임 검증 결과는 [작업 기록](../../../.agents/workflow/tasks/quest-presentation-vm.md)에 별도로 남긴다. 표시 VM 이전은 퀘스트 복제·실행 권위·게임 규칙을 바꾸지 않는다.
