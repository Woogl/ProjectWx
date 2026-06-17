# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문/엘리베이터/보물상자/콘솔/컷신 트리거)와, 플레이어가 그것들과 상호작용하는 공통 메커니즘(오버랩 감지·HUD 리스트·서버 권위 발동), 그리고 적/대상 액터를 스폰·리스폰하는 스포너 시스템을 담당한다.

## 책임
**담당**
- 상호작용 파이프라인: `UWxInteractionComponent`(오버랩 감지·서버 권위 `TryInteract`·Multicast 알림)와 로컬 플레이어별 `UWxInteractionRegistrySubsystem`(인-레인지 목록·선택·외곽선 강조 조율)
- 기믹 액터: `AWxGimmick` 베이스와 그 자식들(Door/Elevator/TreasureChest/AlarmConsole/SpawnConsole/CutsceneTrigger). 상태 복제·Level Streaming Persistence·WxSave 슬롯 보존 포함
- 문 상태머신을 StateTree로 구동하는 노드(`WxDoorStateTreeNodes`)
- 스포너: `AWxSpawner`(처치/부활 상태 보유), `UWxSpawnerSubsystem`(레지스트리·일괄 리스폰), `IWxSpawnableInterface`(스폰 대상 훅), `UWxSpawnerLibrary`(BP 진입점)

**경계 (비담당)**
- 상호작용 *입력*과 어빌리티(`WxAbility_Interact`)·외곽선 포스트프로세스 머티리얼·HUD 리스트 뷰모델/WBP는 [[WxCombat]]·[[WxUI]] 등 외부 담당. 본 모듈은 등록/선택/발동 데이터만 노출
- 보상 지급은 위임: 보물상자는 보상 컴포넌트(`WxRewardComponent`)를 C++로 들지 않고 [[WxInventory]]가 상속 BP에서 추가·자가 바인딩
- 상호작용 인터페이스(`IWxInteractionSource`)·세이브 인터페이스(`IWxSavable`) 정의는 [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]](`IWxInteractionSource`·`IWxSavable`), StateTree / GameplayStateTree(문 상태머신), GameplayAbilities·Niagara·LevelSequence/MovieScene(컷신), DeveloperSettings(스포너 아이콘 설정)
- 규칙: 플러그인 의존은 WxCore·엔진 플러그인뿐 — WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInteractionComponent` | 상호작용 영역. 폰 오버랩 감지 → 레지스트리 등록 → 서버 `TryInteract` → `OnInteracted` Multicast. 기믹들이 이 컴포넌트에 핸들러를 바인딩 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 목록·선택 인덱스 소유. HUD 리스트와 어빌리티가 읽는 단일 소스 | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxGimmick` | 모든 상호작용 월드 오브젝트의 추상 베이스. `ApplyState` 후크 + 1회성 `bTriggered` + WxSave 통합 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxDoor` | 자체 `EWxDoorState` 권위 상태 + StateTree 구동 개폐 문 | `Source/WxWorld/Public/Gimmick/WxDoor.h` |
| `AWxElevator` | 스플라인 경로 5상태 머신(정지 2 + 전이 3) 엘리베이터 | `Source/WxWorld/Public/Gimmick/WxElevator.h` |
| `AWxSpawner` | 스폰 대상 인스턴스를 들고 처치/부활(`bIsKilled`·`bNeverRevive`)을 자체 보유하는 레벨 배치 액터 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `UWxSpawnerSubsystem` | 월드 내 스포너 레지스트리. 역조회 처치 마킹·Auto 일괄 리스폰 위임 | `Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현. `OnSpawnedBy`(빙의 전 컨텍스트 주입) + 에디터 미리보기 메시 추출 | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`(Abstract) 상속 → 메시/`UWxInteractionComponent`를 직접 들고 핸들러를 `OnInteracted`에 바인딩. 다단계 상태가 필요하면 `bTriggered` 대신 자체 State enum + `ReplicatedUsing`/`SaveGame`을 두고 `ApplyState()` 오버라이드로 시각/인터랙션을 동기화한다(서버 즉시·OnRep·BeginPlay·WxSave 복원이 모두 `ApplyState` 한 경로로 수렴). 보존 필드는 `UPROPERTY(SaveGame)`, 안정 키 `WxSaveId`는 베이스가 에디터에서 부여
- **문 상태/전이 author**: C++는 얇은 프리미티브(포즈 보간·인터랙션 토글·State 조회)만 제공하고 상태·전이는 `ST_Door` StateTree 에셋에서 편집(`WxDoorStateTreeNodes`의 DoorPose/DoorInteraction/DoorStateIs)
- **새 스폰 대상**: 액터가 `IWxSpawnableInterface` 구현 → `AWxSpawner.SpawnableActorClass`에 지정. 트리거는 `EWxSpawnerMode`(Auto/Manual), 보스류는 `bNeverRevive`
- **상호작용 활성/텍스트/강조**는 `UWxInteractionComponent`의 `SetInteractionEnabled`/`SetInteractionText`/`SetHighlightEnabled`로 런타임 제어

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 전체 흐름(오버랩→등록→서버 발동→Multicast)이 헤더 주석에 정리되어 있어 모듈 진입에 최적
2. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — `ApplyState`/`bTriggered`/WxSave 규약. 모든 기믹의 상태 동기화 패턴이 여기서 출발
3. `Source/WxWorld/Public/Gimmick/WxDoor.h` + `WxDoorStateTreeNodes.h` — C++ 프리미티브와 StateTree 분담을 보여주는 대표 사례
4. `Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/부활 상태 모델

## 관련
- 상위: [[WxCombat]](`WxAbility_Interact`가 레지스트리 선택 대상을 TargetData로 서버에 전달), [[WxUI]](HUD 인터랙션 리스트가 레지스트리 목록 표시), [[WxInventory]](보물상자 보상 컴포넌트 BP 상속), [[WxCore]](인터페이스 정의)

---
*문서 기준 커밋 `a2ba2b5` · 생성일 2026-06-17 · 소스 30파일 — `/readme-writer`로 갱신*
