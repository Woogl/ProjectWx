---
title: "WxQuest — 퀘스트 실행과 저널"
category: topic
sources:
  - "raw/notes/2026-09-22-current-quests.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, quests]
aliases: ["WxQuest"]
confidence: medium
volatility: warm
summary: "WxQuest는 권위 측의 단일 StateTree 러너와 제목·목표 저널을 제공하며, 실제 진행 내용은 에셋이 구성한다."
---

# WxQuest — 퀘스트 실행과 저널

WxQuest는 권위 측의 단일 StateTree 러너와 제목·목표 저널을 제공하며, 실제 진행 내용은 에셋이 구성한다.

## 실행 모델

`UWxQuestComponent::BeginPlay`는 Owner의 권위를 확인한 뒤 자동 시작하지 않는 StateTreeComponent를 생성한다. `ActivateQuest`는 기존 로직을 정지하고 새 StateTree 참조를 설정한 뒤 시작한다. 따라서 현재 실행 모델은 동시에 여러 퀘스트를 유지하는 목록이 아니라 활성 퀘스트 하나를 교체하는 방식이다.

태스크 안에서 다음 퀘스트로 넘어갈 때는 `RequestActivateQuest`로 다음 틱을 예약한다. 현재 러너 콜스택 중 교체를 피하고, 예약 동안 에셋 수명이 달라져도 실행 시점에 소프트 참조를 다시 로드한다.

## 저널과 종료

제목 설정은 목표 목록을 초기화하고 활성 저널을 표시한다. 목표는 문자열 대신 증가하는 핸들로 추가·제거하므로 같은 문구가 여러 번 존재할 수 있다. 변경마다 `OnJournalChanged`를 발행한다. 러너 상태가 Running이 아니게 되면 저널을 정리해 완료·실패·교체 경로를 모은다.

## 조립과 범위

WxGame의 GameState가 퀘스트 컴포넌트를 소유하고 게임 ViewModel이 저널을 표시한다. StateTree에는 제목·목표·도달 대기·다음 퀘스트 태스크를 조합하며 보상·대화·장치 행동은 각 도메인 노드가 담당한다.

현재 컴포넌트에는 저널 복제·세이브 복원·복수 퀘스트 목록 구현이 없다. 권위 러너의 존재를 모든 클라이언트에서 동일한 저널을 볼 수 있다는 뜻으로 해석하지 않는다. 레벨 대상의 Locator가 있다고 해서 월드 파티션 로딩과 실제 도달 판정까지 검증된 것도 아니다.

[QuestComponent](../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp)에서 시작하고, [StateTree 태스크](../../../Plugins/WxQuest/Source/WxQuest/Private/Quest)와 실제 퀘스트 에셋을 함께 확인한다.

## 관련 문서

- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-quests.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
