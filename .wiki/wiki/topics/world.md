---
title: "WxWorld — 장치와 상호작용"
category: topic
sources:
  - "raw/notes/2026-09-22-current-world.md"
  - "raw/notes/2026-09-22-current-foundation.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, world]
aliases: ["WxWorld"]
confidence: medium
volatility: warm
summary: "WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다."
---

# WxWorld — 장치와 상호작용

WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다.

## 장치 상태의 복제

`AWxDevice`와 `UWxDeviceStateTreeComponent`가 장치 실행의 중심이다. 서버는 상태 태그명·진입 일련번호·상호작용자·선택지 값을 스냅샷으로 발행한다. 활성 태그가 없거나 이전 태그와 같으면 `PublishState`는 새 스냅샷을 만들지 않는다. 같은 태그의 재진입까지 별도 이벤트로 보존하는 복제 로그는 아니다.

클라이언트는 일련번호가 바로 다음이면 실시간 전이로, 초기 수신·번호 건너뜀은 복원으로 처리한다. 상호작용자 참조가 늦게 해소되어 같은 번호가 다시 통지되면 참조를 갱신하지만 전이를 반복하지 않는다. 태그로 루트 에셋의 상태를 찾아 Critical 전이를 요청하며, 종료된 트리는 먼저 재시작한다.

`FWxDeviceExecutionPolicy`는 초기 진입 또는 복원 플래그를 판별한다. 장치 태스크를 추가할 때 일회성 보상·효과를 복원 중 다시 실행할지, 위치·표시만 맞출지를 구분해야 한다. 스냅샷 하나로 모든 커스텀 태스크의 재생 안전성을 보장하지 않는다.

## 상호작용 계약

로컬 컨트롤러의 스캐너가 Pawn 주변 쿼리 콜리전을 검색하고 `IWxInteractable`의 자격·선택지를 읽는다. 기존 후보 순서를 보존하고 새 후보를 거리순으로 뒤에 붙여 목록이 매번 뒤섞이지 않게 한다. 선택 값의 의미는 대상 액터가 정의한다.

RPC는 선택 액터와 값을 GameplayEvent로 전달하고, WxGame의 Interact 어빌리티가 서버에서 현재 자격·거리·선택지 유효성을 재검사한다. 콜리전이 없는 액터는 인터페이스만 구현해도 감지·거리 검사에 걸리지 않는다. 클라이언트 UI 목록은 실행 권한의 근거가 아니다.

## 스폰과 체크포인트

스포너는 별도 Spawnable 경로와 공용 `IWxSpawnable` 처치 통지를 사용한다. 순찰 경로와 스폰/빙의 초기화는 WxAI·WxGame과 함께 확인한다.

CheckpointSubsystem은 Standalone에서만 레벨 패키지와 위치·회전을 보관한다. PIE 접두사를 제거해 같은 레벨인지 검사하고 다른 레벨에서는 반환하지 않는다. 디스크 세이브나 멀티플레이 체크포인트 저장 시스템이 아니다. 부활과 스포너 재생성 조립은 WxGame에 있다.

진입점: [장치 동기화](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp), [상호작용 스캐너](../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp), [체크포인트](../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp).

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat-finisher|그로기 피니시와 뒤잡]] ([그로기 피니시와 뒤잡](../concepts/combat-finisher.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer](../references/editor-tools.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-world.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
