# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹(문·엘리베이터·상자·콘솔·컷신 트리거)과 플레이어의 근접 상호작용 감지·선택·실행, 그리고 적/오브젝트 스폰을 담당한다. 기믹의 상태·연출은 StateTree 로 데이터 주도 구동한다.

## 책임
**담당**
- 상호작용 기믹의 공통 프레임: `AWxGimmick` 베이스가 서버 권위 State 태그(복제 + SaveGame)와 GimmickStateTree 구동 패턴(권위 쓰기 → 복제 → ST 이벤트 진입)을 제공한다.
- 기믹 구현체: Door / Elevator / TreasureChest / AlarmConsole / SpawnConsole / CutsceneTrigger.
- 기믹 StateTree 공용 노드 라이브러리(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/Niagara/스폰/인터랙션·입력 토글 태스크 + State 비교 조건 — 기믹 종류와 무관한 재사용 연출 프리미티브.
- PlayerController 부착 상호작용 레지스트리(`UWxInteractionRegistryComponent`): 채널 쿼리로 주변 상호작용 메시 수집, 선택/외곽선 강조, `ServerInteract` 전송. 상호작용 영역은 별도 컴포넌트가 아니라 대상 액터의 메시 자체다(WxInteractable 채널 응답으로 표식).
- 스폰 시스템: `AWxSpawner`(처치 상태 GUID 보존, 리스폰/부활 금지) + `IWxSpawnableInterface` + `UWxSpawnerLibrary`(일괄 리스폰 BP 진입점).

**경계 (비담당)**
- `IWxInteractable`·`IWxSavable` 인터페이스 자체 정의와 게임플레이 태그(`Gimmick.*`) 선언 → [[WxCore]].
- 상호작용 실행의 권위 어빌리티(`WxAbility_Interact`)와 폰 ASC 이벤트 발행 → [[WxCombat]]/GAS 측(레지스트리는 `Event.Interact` 송출까지만).
- 세이브 슬롯 직렬화·복원 오케스트레이션 → [[WxSave]](기믹/스포너는 `IWxSavable` 구현만).
- HUD 프롬프트·선택 표시 위젯/뷰모델 → [[WxUI]].

## 의존성
- **주요 의존**: [[WxCore]](`IWxInteractable`, `IWxSavable`, `Gimmick.*` 태그), 엔진 서브시스템 StateTree/GameplayStateTree, GameplayAbilities(상호작용 어빌리티 활성 판정·입력 차단), LevelSequence/MovieScene(컷신), Niagara(FX).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 상호작용 기믹의 추상 베이스. State 태그 소유·복제·SaveGame, `CommitGimmickState` 단일 권위 쓰기, StateTree 구동 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `FWxStateTreeTask_*` / `FWxStateTreeCondition_GimmickStateIs` | 전 기믹 공유 StateTree 노드 모음(이동·애니·시퀀스·사운드·Niagara·스폰·토글·State 검사) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionRegistryComponent` | PlayerController 부착. 소유 클라 주기 스캔·선택·하이라이트, `ServerInteract` RPC(HUD 리스트 소스) | `Source/WxWorld/Public/Interaction/WxInteractionRegistryComponent.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. 처치 상태(`bIsKilled`) SaveGame 영속, `Respawn`/`bNeverRevive` | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 계약(`OnSpawnedBy` 훅, 에디터 프리뷰 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | 월드 내 Auto 스포너 일괄 리스폰 BP 진입점 | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑(Config=Game) | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |
| `AWxDoor` / `AWxElevator` / `AWxTreasureChest` / `AWxAlarmConsole` / `AWxSpawnConsole` / `AWxCutsceneTrigger` | 구체 기믹. 생성자에서 컴포넌트·초기 State 지정, `OnInteracted` 에서 목표 State 확정 | `Source/WxWorld/Public/Gimmick/*.h` |

## Gameplay Tags
- 본 모듈은 Native Tag 를 **선언하지 않는다**. 기믹 State 태그(`Gimmick.*`)와 그 부모 태그 `Gimmick`(ST Root 재선택 전이가 받는다), 복원 마커 `StateTree.Restore`, 상호작용 이벤트(`Event.Interact`) 는 [[WxCore]]의 `WxGameplayTags` 에서 선언되며 여기서는 사용만 한다.

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 을 상속해 컴포넌트(메시·`UWxInteractionComponent`)를 생성자에서 구성하고 초기 `State` 태그를 지정, `OnInteracted` 에서 `SetInteractingCharacter` → `CommitGimmickState(목표태그)` 로 상태를 전이한다. State 쓰기는 항상 서버 권위(`CommitGimmickState`)만 사용하고 클라는 복제 State 를 추종한다. 비주얼/연출은 C++ 가 아니라 자식 BP 에 할당한 StateTree 에셋이 State 태그를 `Required Event to Enter` 로 받아 구동한다. ST 에셋 배선은 Root 에 재선택 전이 하나(`On Event: Gimmick` → `GotoState: Root`)만 두고 각 상태가 자기 태그를 `Required Event to Enter` 로 선언하는 형태이며, 조건 없는 resting 상태는 Root 자식 중 **마지막**에 둔다(위에 있으면 항상 그것이 선택된다). 상태별 HUD 프롬프트(예: `Call Elevator`/`Open`)는 그 상태의 `Wx Enable Interaction` 태스크 `Prompt` 필드로 지정하며, 미지정 시 기믹의 `InteractionPrompt` 기본값으로 폴백한다.
- **새 연출 프리미티브**: `WxGimmickStateTreeNodes.h` 에 `FStateTreeTaskCommonBase` 파생 태스크를 추가한다. 초기 진입(시작/복원/레이트조인)은 `Transition.SourceStateID` 무효로 판별해 스냅·침묵하고, 라이브 전이에서만 재생/사이드이펙트를 낸다. 순간 side-effect 태스크는 `bConsideredForCompletion=false` 로 재선택 루프를 피한다.
- **새 상호작용 대상**: 액터가 `IWxInteractable`(WxCore)을 구현하고 감지 영역마다 `UWxInteractionComponent` 를 붙인다(볼륨은 부착 부모 프리미티브 자동 채택, 아니면 `SetCollisionVolume`). 응답·프롬프트는 컴포넌트가 아닌 소유 액터가 낸다.
- **새 스폰 대상**: `IWxSpawnableInterface` 를 구현하면 `AWxSpawner`/`FWxStateTreeTask_TriggerSpawners`/`FWxStateTreeTask_SpawnActor` 로 스폰된다.
- **데이터 주도**: 기믹 상태·연출은 자식 BP 의 StateTree 에셋이, 스포너 아이콘 등 에디터 편의는 `UWxWorldDeveloperSettings`(Config=Game)가 구동한다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹 공통의 State 구동/복제/SaveGame 패턴. 이 헤더 doc-comment 가 모듈의 설계 계약이다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 연출이 어떻게 데이터 주도로 조립되는지, 초기 진입 vs 라이브 전이 규약.
3. `Source/WxWorld/Public/Interaction/WxInteractionRegistryComponent.h` — 감지→선택→ServerInteract→어빌리티로 이어지는 상호작용 실행 경로를 헤더 주석이 서술한다.
4. `Source/WxWorld/Private/Gimmick/WxDoor.cpp` 등 구체 기믹 — 베이스 위에 새 기믹을 얹는 최소 구현 예시. 이후 Elevator/CutsceneTrigger 로 확장 패턴을 비교한다.

## 관련
- 상위: 레벨 디자인이 배치·구동하는 월드 콘텐츠 계층. 상호작용 실행은 GAS 어빌리티([[WxCombat]]), 영속은 [[WxSave]], HUD 표시는 [[WxUI]], 공용 인터페이스·태그는 [[WxCore]]와 맞물린다.

---
*문서 기준 커밋 `10f1722` · 생성일 2026-07-23 · 소스 29파일 — `/readme-writer`로 갱신*
