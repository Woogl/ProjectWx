---
title: "WxGame — 게임 조립과 실행 흐름"
category: topic
sources:
  - "raw/notes/2026-09-22-current-game.md"
  - "raw/notes/2026-09-22-current-foundation.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, game]
aliases: ["WxGame"]
confidence: medium
volatility: warm
verified: 2026-09-22
summary: "WxGame은 캐릭터·컨트롤러·GameState에 도메인 기능을 배치하고 새 게임·부활·표시 연결을 조립한다."
---

# WxGame — 게임 조립과 실행 흐름

WxGame은 캐릭터·컨트롤러·GameState에 도메인 기능을 배치하고 새 게임·부활·표시 연결을 조립한다.

## 소유 구조

| 소유자 | 주요 구성 |
|---|---|
| `AWxCharacterBase` | ASC, CombatAttributeSet, MotionWarping, LockOn, HitStop, 무기 ChildActor, MetaHuman 구성 |
| `AWxPlayerController` | 인벤토리, 상호작용 스캐너, 대화 세션, PlayerLayout |
| `AWxGameState` | 퀘스트 컴포넌트, 스킬 컷신 컴포넌트 |
| GameInstance 서브시스템 | 새 게임 흐름, UI 레이아웃, 체크포인트 등 각 도메인의 장기 수명 |

ASC는 PlayerState가 아니라 캐릭터의 기본 서브오브젝트다. 사망·래그돌 태그 구독은 PostInitializeComponents에 있어 시뮬레이티드 프록시까지 포함한다. 보상처럼 권위에서만 처리할 동작은 해당 구독자에서 다시 권한을 확인해야 한다.

## 새 게임

`UWxGameFlowSubsystem::RequestNewGame`은 Standalone·유효한 캐릭터/레벨 선택·중복 요청 조건을 검사한다. 선택을 저장하고 체크포인트를 초기화한 뒤 레벨을 연다. GameMode는 목적지 월드에서 선택 Pawn 클래스를 사용한다. 다른 월드가 열리거나 이동이 실패하면 선택을 정리한다.

목적지 로드 뒤 카메라 위치를 갱신하고 레벨 스트리밍 완료를 기다려 첫 틱 전에 지형을 준비한다. 선택 목록은 `WxFrontEndDeveloperSettings`와 DefaultGame.ini에 있다. 파일 경로 등록만으로 모든 맵의 실제 로딩을 검증한 것은 아니다.

## 같은 월드 부활

`RequestRespawn`은 활성 사망 화면·로컬 컨트롤러·Standalone·사망 태그·기본 Pawn 클래스를 확인한다. 기존 Pawn을 비빙의한 뒤 체크포인트 또는 일반 RestartPlayer로 새 Pawn을 만든다. 생성 실패 시 기존 Pawn의 충돌과 빙의를 복구한다. 성공하면 기존 Pawn을 파괴하고 새 Pawn HP/MP를 최대값으로 채운 뒤 시점·스트리밍·스포너 재생성을 처리한다.

이 경로는 멀티플레이 부활이나 디스크 저장 복원으로 일반화하지 않는다. 인벤토리는 컨트롤러에 있으므로 Pawn 교체와 인벤토리 객체 수명이 다르지만, 게임 종료 후 저장을 의미하지는 않는다.

## 확장 진입점

[CharacterBase](../../../Source/WxGame/Character/WxCharacterBase.cpp), [PlayerController](../../../Source/WxGame/Controller/WxPlayerController.cpp), [GameFlow](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp), [RespawnLibrary](../../../Source/WxGame/Framework/WxRespawnLibrary.cpp)를 기준으로 기능 소유자를 정한다. 도메인 공통 규칙을 이 조립 계층에 중복 구현하지 않는다.

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-finisher|그로기 피니시와 뒤잡]] ([그로기 피니시와 뒤잡](../concepts/combat-finisher.md))
- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-game.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
