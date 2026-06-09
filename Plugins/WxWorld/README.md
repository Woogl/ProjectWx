# WxWorld — 월드 시스템

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문/엘리베이터/콘솔/컷신 트리거)와 몬스터 스포너를 구현한다. 폰 오버랩 기반 상호작용, 1회성/상태머신 기믹, WxSave 연동 상태 보존을 담당한다.

## 책임
**담당**
- 레벨 배치 상호작용 오브젝트(`AWxGimmick` 계층: 문/엘리베이터/경보·스폰 콘솔/컷신 트리거)
- 상호작용 컴포넌트(`UWxInteractionComponent`): 폰 오버랩 감지, 프롬프트 위젯 토글, Multicast 알림
- 몬스터 스폰 액터(`AWxSpawner`)와 레지스트리 서브시스템(`UWxSpawnerSubsystem`), 처치/리스폰/영구사망 관리
- 기믹·스포너의 상태 리플리케이션과 WxSave 슬롯 보존(`bTriggered`/`bIsKilled`/상태 enum)

**경계 (비담당)**
- 상호작용 입력→`TryInteract` 발동(WxAbility_Interact)은 GAS 어빌리티 쪽 책임 — [[WxCombat]]
- 저장/복원 슬롯 직렬화 메커니즘 자체는 [[WxSave]] (본 모듈은 `IWxSavable` 구현체만 제공)
- 프롬프트 위젯의 비주얼/MVVM은 [[WxUI]] (본 모듈은 `IWxInteractionWidgetInterface` 계약만 정의)

## 의존성
- **주요 의존**: [[WxCore]] (`WxSavable`), GameplayTags, GameplayAbilities, Niagara, LevelSequence/MovieScene, UMG, DeveloperSettings
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 상호작용 월드 오브젝트 공통 부모(Abstract). `bTriggered`+`ApplyState()` 후크, `IWxSavable` | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `UWxInteractionComponent` | 오버랩 감지·프롬프트·Multicast 상호작용 알림(`SphereComponent` 파생) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `IWxInteractionWidgetInterface` | 프롬프트 위젯 계약(텍스트 전달) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionWidgetInterface.h` |
| `AWxDoor` | 1회성 개폐 문(상태머신 Closed→Opening→Open) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h` |
| `AWxElevator` | 스플라인 경로 엘리베이터(5상태 머신) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h` |
| `AWxAlarmConsole` | 1회성 경보 콘솔(Niagara/사운드) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxAlarmConsole.h` |
| `AWxSpawnConsole` | 1회성 스폰 콘솔(외부 Spawner Respawn 트리거) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxSpawnConsole.h` |
| `AWxCutsceneTrigger` | Level Sequence 재생 트리거(로컬 입력 잠금) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h` |
| `AWxSpawner` | SpawnableActorClass 스폰 액터, 처치/부활/영구사망 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 계약(에디터 미리보기 메시 제공) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerSubsystem` | Spawner 레지스트리, 처치 마킹/일괄 리스폰 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `UWxSpawnerLibrary` | BP 진입점, Subsystem 위임 thin wrapper | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 폴더 구성
- `Gimmick/` — `AWxGimmick` 베이스와 구체 기믹(문/엘리베이터/콘솔/컷신 트리거)
- `Interaction/` — 상호작용 컴포넌트와 프롬프트 위젯 인터페이스
- `Spawnable/` — 스폰 액터와 스폰 대상 인터페이스
- `System/` — Spawner 서브시스템/BP 라이브러리/DeveloperSettings

## 확장 포인트 / 규약
- **새 상호작용 기믹**: `AWxGimmick`를 상속한다. 컴포넌트(메시/`UWxInteractionComponent`)는 자식이 직접 들고 `SceneRoot`에 `SetupAttachment`한다(`SetRootComponent` 호출 금지). 상태 적용은 `ApplyState()` 오버라이드로 통일하고, 1회성 발동은 `MarkTriggered()`를 쓴다. 보존 필드는 `UPROPERTY(SaveGame)`로 표시.
- **새 스폰 대상**: 스폰될 액터 클래스가 `IWxSpawnableInterface`를 구현해야 `AWxSpawner.SpawnableActorClass`에 지정 가능(`MustImplement` 제약).
- **상호작용 동작 연결**: 소유 액터가 `UWxInteractionComponent::OnInteracted`(Multicast)에 `Handle*` 핸들러를 바인딩하고, 핸들러 내 권위 로직은 `GetOwner()->HasAuthority()`로 분기.
- **리플리케이션/권한(최대 4인 멀티)**: `TryInteract`는 서버 권한 진입점이며 `MulticastInteracted`(NetMulticast Unreliable)로 전원 fire. 기믹 상태는 `ReplicatedUsing` OnRep에서 `ApplyState`를 호출하고, 서버는 `MarkTriggered`에서 명시 호출해 경로를 통일한다. WxSave 키는 에디터에서 부여된 불변 `FGuid WxSaveId`.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 기믹 계층의 상태/세이브/ApplyState 규약. 모듈 전체의 토대.
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 흐름(오버랩→프롬프트→TryInteract→Multicast)의 진입점.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` + `System/WxSpawnerSubsystem.h` — 스폰/처치/리스폰 모델.

## 관련
- 상위: [[WxGame]]가 구체 기믹/스포너 BP를 레벨에 배치해 사용. 상호작용 발동은 [[WxCombat]]의 GAS 어빌리티, 상태 보존은 [[WxSave]], 프롬프트 위젯은 [[WxUI]]와 함께 본다.

---
*문서 기준 커밋 `59bfe3f` · 생성일 2026-06-08 · 소스 24파일 — `/readme-writer`로 갱신*
