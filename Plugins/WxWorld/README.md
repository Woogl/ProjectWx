# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 오브젝트(기믹), 플레이어 상호작용 파이프라인(오버랩→레지스트리→선택→발동), 적/오브젝트 스포너를 담당하는 도메인 플러그인.

## 책임
**담당**
- 상호작용 컴포넌트와 로컬 플레이어 레지스트리: 오버랩 감지, 인-레인지 목록 수집, 선택 순환, 외곽선 강조, 서버 권위 발동(Multicast).
- 상호작용 기믹 액터(문/엘리베이터/보물상자/경보·스폰 콘솔/컷신 트리거)의 공통 베이스(`AWxGimmick`)와 상태/세이브 통합.
- 레벨 배치 스포너(`AWxSpawner`)와 월드 단위 스포너 레지스트리/일괄 리스폰.

**경계 (비담당)**
- 상호작용 입력→발동 트리거(`WxAbility_Interact`)와 사망 처리는 [[WxCombat]]/GAS 측 어빌리티가 호출(본 모듈은 `TryInteract` 진입점만 제공).
- HUD 상호작용 리스트 위젯·뷰모델 표시는 [[WxUI]].
- 세이브 슬롯 직렬화 메커니즘 자체는 [[WxSave]](본 모듈은 `IWxSavable` 구현만 제공).
- 상호작용 인터페이스/세이브 인터페이스 정의는 [[WxCore]](`IWxInteractionSource`, `IWxSavable`).

## 의존성
- **주요 의존**: WxCore (`IWxInteractionSource`, `IWxSavable`), GameplayAbilities, Niagara, LevelSequence/MovieScene(컷신 트리거), GameplayTags, DeveloperSettings.
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (모든 `Wx*` include 는 WxCore 헤더 또는 모듈 내부 헤더로 해소됨)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInteractionComponent` | SphereComponent 기반 상호작용 영역. 오버랩→레지스트리 등록, 서버 `TryInteract`→`OnInteracted` Multicast | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 컴포넌트 목록·선택 소유, 강조 조율(로컬 표시 전용) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxGimmick` | 상호작용 월드 오브젝트 공통 부모(Abstract). `bTriggered`/`ApplyState` 후크 + `IWxSavable` | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxSpawner` | SpawnableActorClass 인스턴스를 스폰하는 배치 액터. 처치/부활 상태(`bIsKilled`) 보유 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 액터가 구현하는 훅(`OnSpawnedBy`, 에디터 프리뷰 메시) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerSubsystem` | 월드 내 Spawner 레지스트리. 일괄 리스폰/처치 역조회 위임 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `UWxSpawnerLibrary` | BP 진입점. 서브시스템으로 위임하는 thin wrapper | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | Spawner 클래스별 에디터 아이콘 매핑(Config=Game) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 를 상속. 컴포넌트(메시/`UWxInteractionComponent`)는 자식이 직접 보유·바인딩하고 `SceneRoot` 에 SetupAttachment 한다. 1회성 발동은 `MarkTriggered()`(서버) + `ApplyState()` 오버라이드로 구현(예: `AWxTreasureChest`). 다단계 상태는 자식 자체 enum + `ReplicatedUsing` OnRep 에서 `ApplyState()` 호출(예: `AWxDoor::EWxDoorState`).
- **새 스폰 대상 추가**: 스폰될 액터가 `IWxSpawnableInterface` 구현(`SpawnableActorClass` 는 `MustImplement` 로 강제). 스폰 직후 빙의 전 `OnSpawnedBy` 로 per-instance 컨텍스트를 받는다(Deferred Spawn).
- **데이터 주도**: `UWxWorldDeveloperSettings` 의 `SpawnerClassIcons` 로 스포너 에디터 아이콘 지정. 스폰 모드는 `EWxSpawnerMode`(Auto/Manual), 보스 등은 `bNeverRevive`.
- **리플리케이션/권한**: 발동은 서버 권한(`TryInteract`→`MulticastInteracted`). 상태 필드는 `Replicated`+`SaveGame`. 레지스트리/선택/강조는 로컬 클라이언트 전용. 상태 복원은 Level Streaming Persistence + WxSave(`OnWxSaveRestored`→`ApplyState`), 키는 에디터 부여 `WxSaveId`(FGuid).

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 파이프라인 전체 흐름(오버랩→발동)이 헤더 주석에 정리됨.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 기믹 베이스의 `ApplyState`/`bTriggered`/세이브 호출 경로.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/부활 모델, `IWxSpawnableInterface` 와의 관계.

## 관련
- 상위: [[WxCore]] (`IWxInteractionSource`, `IWxSavable` 정의)
- 협력: [[WxCombat]] (상호작용 어빌리티·사망 트리거), [[WxUI]] (상호작용 리스트 HUD), [[WxSave]] (세이브 슬롯)

---
*문서 기준 커밋 `8fb8b93` · 생성일 2026-06-16 · 소스 28파일 — `/readme-writer`로 갱신*
