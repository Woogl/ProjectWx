---
title: "WxGame — 게임 조립과 실행 흐름"
category: topic
sources:
  - "raw/notes/2026-09-25-checkpoint-single-slot.md"
  - "raw/notes/2026-09-25-checkpoint-savegame.md"
  - "raw/notes/2026-09-22-current-game.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-boss-battle-three-layer.md"
  - "raw/notes/2026-09-24-nameplate-manager.md"
  - "raw/notes/2026-09-24-nameplate-manager-wxgame.md"
  - "raw/notes/2026-09-24-ragdoll-physics-asset.md"
  - "raw/notes/2026-09-25-ability-montage-section-model.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, game]
aliases: ["WxGame"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "WxGame은 캐릭터·컨트롤러·GameState에 도메인 기능을 배치하고 새 게임·부활·표시 연결을 조립한다."
---

# WxGame — 게임 조립과 실행 흐름

WxGame은 캐릭터·컨트롤러·GameState에 도메인 기능을 배치하고 새 게임·부활·표시 연결을 조립한다.

## 소유 구조

| 소유자 | 주요 구성 |
|---|---|
| `AWxCharacterBase` | ASC, CombatAttributeSet, MotionWarping, LockOn, HitStop, 무기 ChildActor, MetaHuman 구성 |
| `AWxPlayerController` | 인벤토리, 상호작용 스캐너, 대화 세션, PlayerLayout, NameplateManager |
| `AWxEnemyCharacter` | 교전 태그(`State.Engaged`) 갱신 |
| `AWxGameState` | 퀘스트 컴포넌트, 스킬 컷신 컴포넌트 |
| GameInstance 서브시스템 | 새 게임 흐름, UI 레이아웃 등 각 도메인의 장기 수명 |
| 월드 서브시스템 | `UWxBattleSubsystem`: 교전 중인 보스와 현재 보스 |

`AWxPlayerController`에 붙은 WxGame `UWxNameplateManagerComponent`가 빙의 캐릭터의 락온 대상과 적의 교전 태그를 직접 읽어 머리 위 Nameplate·Reticle을 붙인다. 락온(WxCombat)과 위젯·VM(WxUI)을 함께 알아야 하는 연결 코드라 조립 계층에 있다. 표시 규칙은 [UI](ui.md)의 머리 위 Nameplate 절에 있다.

ASC는 PlayerState가 아니라 캐릭터의 기본 서브오브젝트다. 사망·래그돌 태그 구독은 PostInitializeComponents에 있어 시뮬레이티드 프록시까지 포함한다. 보상처럼 권위에서만 처리할 동작은 해당 구독자에서 다시 권한을 확인해야 한다.

시체 수명은 사망 어빌리티가 아니라 `AWxCharacterBase::CorpseLifeSpan`이 정한다. `HandleDeath`가 이 값이 0보다 크면 `SetLifeSpan`을 걸고, 0 이하면 시체를 남긴다. 클라이언트에서의 호출은 `SetLifeSpan`의 권위 검사가 거른다. 원자료 기록상 0보다 큰 값을 둔 BP는 분신 `BP_Minion`(0.1초)뿐이다.

캐릭터의 이동 컴포넌트는 `UWxCharacterMovementComponent`다. 낙하가 끝나면 재생 중인 몽타주 인스턴스 전부에서 `UWxAbilityBase::LandingSectionName`(`Grounded`) 섹션을 찾아 그 섹션으로 옮긴다. 가장 최근 몽타주만 보면 가산 일반 피격이 넉업 위에 겹친 채 착지할 때 넉업이 `Loop` 섹션에 갇히기 때문이다.

래그돌은 캐릭터 메시의 PhysicsAsset으로 시뮬레이션한다. 마네킹 메시 네 개(`SKM_Manny`·`SKM_Manny_Simple`·`SKM_Quinn`·`SKM_Quinn_Simple`)는 모두 `/Game/NiagaraExamples/Gallery/SkeletalMesh/Mannequins/Rigs/PA_Mannequin`을 쓴다. `/Game/Mannequins/Rigs/PA_Mannequin`은 쓰지 않는다. 기본 자세부터 겹치는 바디 쌍(pelvis–spine_03, spine_05–upperarm_l)의 충돌이 켜져 있어 래그돌이 떨리기 때문이다. 정본 PA가 예제 폴더에 있으므로 예제 콘텐츠를 지우면 PA 참조가 끊긴다.

**알려진 미수정 문제 (2026-09-23 리뷰)**: 레벨 스트리밍으로 숨겼다 다시 보이면 `PostInitializeComponents`가 다시 실행된다(UE 5.8 `Actor.cpp` RouteEndPlay·`Level.cpp` 재초기화). 그러면 사망·래그돌 구독이 중복되어, 사망 시 `OnDeath`와 보상 지급이 두 번 일어날 수 있다.

## 보스전 상태

캐릭터 종류는 `AWxCharacterBase::IdentityTags`(`Character.*`)로 식별하고, 보스는 BP에서 `Character.Boss`를 지정한다. bool 플래그나 종류별 C++ 클래스는 두지 않는다. 이 태그는 `PostInitializeComponents`에서 전 머신의 ASC에 loose 태그로 올라가므로 GAS 태그 조건에서도 보인다. 재실행되어도 개수를 1로 맞춘다.

`UWxBattleSubsystem`(월드 서브시스템)은 보스전 상태의 주인이다.
- 적은 `RefreshEngagement`·`EndPlay`에서 교전 태그를 갱신한 뒤 서브시스템에 직접 알린다.
- 서브시스템은 `Character.Boss`를 가진 캐릭터만 교전 순서대로 모은다. 보스 판정은 목록에 추가할 때만 한다.
- 맨 앞을 현재 보스로 삼고, 바뀔 때만 `OnCurrentBossChanged`를 발행한다. 새 보스가 합류해도 먼저 교전한 보스를 유지하고, 그 보스가 빠지면 다음 순서로 넘어간다.
- 교전 상태는 머신마다 계산되므로 복제 없이 각 머신이 자기 월드에서 모은다.
- VM·MVVM을 모르는 순수 모델이다. 표시 연결은 [UI](ui.md)의 세 층 규칙을 따른다.

게임 월드는 GameInstance를 받은 뒤 월드 서브시스템을 초기화한다(UE 5.8 LoadMap·PIE 모두).

기획의 보스룸 입장·컷신·재도전·페이즈 흐름은 아직 없다. 서버 권위 상태가 필요해지면 복제 액터나 GameState 컴포넌트가 그 상태를 들고, 서브시스템은 모으는 역할로 둔다. GameState에 얹는 대안은 클라이언트 복제 전에 들어오는 통지가 유실될 수 있어 기각했다. 보스 콘텐츠가 없어 인게임 동작은 검증하지 않았다.

## 새 게임

`UWxGameFlowSubsystem::RequestNewGame`은 Standalone·유효한 캐릭터/레벨 선택·중복 요청 조건을 검사한다. 체크포인트 SaveGame 슬롯을 삭제하고, 성공하면 선택을 저장한 뒤 레벨을 연다. 삭제 실패 시 상태 문구를 표시하고 이동하지 않는다. GameMode는 목적지 월드에서 선택 Pawn 클래스를 사용한다. 다른 월드가 열리거나 이동이 실패하면 선택을 정리한다.

목적지 로드 뒤 카메라 위치를 갱신하고 레벨 스트리밍 완료를 기다려 첫 틱 전에 지형을 준비한다. 선택 목록은 `WxFrontEndDeveloperSettings`와 DefaultGame.ini에 있다. 파일 경로 등록만으로 모든 맵의 실제 로딩을 검증한 것은 아니다.

## 같은 월드 부활

`RequestRespawn`은 활성 사망 화면·로컬 컨트롤러·Standalone·사망 태그·기본 Pawn 클래스를 확인한다. 체크포인트 SaveGame을 로드해 현재 맵과 Transform을 검사하고, 기존 Pawn을 비빙의한 뒤 유효한 체크포인트 또는 일반 RestartPlayer로 새 Pawn을 만든다. 생성 실패 시 기존 Pawn의 충돌과 빙의를 복구한다. 성공하면 기존 Pawn을 파괴하고 새 Pawn HP/MP를 최대값으로 채운 뒤 시점·스트리밍·스포너 재생성을 처리한다.

체크포인트 한 개의 레벨 패키지·위치·회전은 디스크에 유지되며, 같은 맵에서 부활할 때 읽는다. PIE와 일반 플레이는 `WxCheckpoint` 슬롯과 사용자 인덱스 0을 공유한다. 캐릭터·인벤토리·장치 상태 전체를 복원하거나 멀티플레이 부활을 제공하는 경로는 아니다. 인벤토리는 컨트롤러에 있으므로 Pawn 교체와 인벤토리 객체 수명이 다르지만, 게임 종료 후 저장을 의미하지는 않는다. 체크포인트의 사용자 확인 범위와 남은 검증은 [[world|WxWorld의 체크포인트 검증 범위]] ([WxWorld의 체크포인트 검증 범위](world.md))를 따른다.

## 확장 진입점

[CharacterBase](../../../Source/WxGame/Character/WxCharacterBase.cpp), [PlayerController](../../../Source/WxGame/Controller/WxPlayerController.cpp), [GameFlow](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp), [RespawnLibrary](../../../Source/WxGame/Framework/WxRespawnLibrary.cpp), [BattleSubsystem](../../../Source/WxGame/Battle/WxBattleSubsystem.cpp)을 기준으로 기능 소유자를 정한다. 다른 서브시스템을 Initialize에서 `GetSubsystem`으로 부르면 엔진이 그 자리에서 초기화하므로 `InitializeDependency`를 따로 두지 않는다(UE 5.8). 도메인 공통 규칙을 이 조립 계층에 중복 구현하지 않는다.

## 관련 문서

- [[modules|모듈 경계와 배치 원칙]] ([모듈 경계와 배치 원칙](../topics/modules.md))
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

- [체크포인트 SaveGame 전환](../../raw/notes/2026-09-25-checkpoint-savegame.md) — 부활 조회·새 게임 초기화와 저장 범위
- [체크포인트 단일 슬롯 결정](../../raw/notes/2026-09-25-checkpoint-single-slot.md) — PIE·일반 플레이의 공용 슬롯
- [근거 1](../../raw/notes/2026-09-22-current-game.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [보스 표시 세 층 구조](../../raw/notes/2026-09-23-boss-battle-three-layer.md)
- [Nameplate·Reticle을 로컬 NameplateManager로](../../raw/notes/2026-09-24-nameplate-manager.md)
- [NameplateManager를 WxGame으로](../../raw/notes/2026-09-24-nameplate-manager-wxgame.md)
- [래그돌 떨림과 PhysicsAsset 통일](../../raw/notes/2026-09-24-ragdoll-physics-asset.md)
- [어빌리티 규칙 변경과 몽타주 섹션 모델](../../raw/notes/2026-09-25-ability-montage-section-model.md) — 시체 수명을 캐릭터로 이동, 착지 섹션을 재생 중인 모든 몽타주에서 탐색

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 2026-09-23에 보스전 상태·식별 태그·재초기화 문제(커밋 `4352e9100`, UE 5.8 소스 확인)를 편찬해 추가했다. 2026-09-24에 NameplateManager 부착과 `LockOnTargetQuery` 연결(커밋 `aaf557a09`)을 HEAD `ca84c9aac` 코드와 대조해 추가했다. 같은 날 래그돌 PhysicsAsset 절을 에셋 값(메시의 PhysicsAsset, PA 바디·컨스트레인트·충돌 비활성 표)과 대조해 추가했다(인게임 미검증). 2026-09-25에 시체 수명(`CorpseLifeSpan`·`HandleDeath`)과 착지 섹션 탐색(`JumpToLandingSection`)을 HEAD `d63ce0630` 코드와 대조해 추가했고, `BP_Minion` 값과 착지·시체 제거의 인게임 동작은 원자료 기록에만 기대며 직접 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
