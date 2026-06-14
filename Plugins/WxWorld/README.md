# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문/엘리베이터/보물상자/콘솔/컷신 트리거)와, 폰이 다가가 입력으로 작동시키는 상호작용 파이프라인, 그리고 적 인스턴스를 배치·리스폰하는 스포너 시스템을 책임진다.

## 책임
**담당**
- 상호작용 컴포넌트: 오버랩 감지 → 프롬프트 위젯 토글 → 서버 권한 `TryInteract` → Multicast 알림(`UWxInteractionComponent`).
- 기믹 액터군: 1회성/상태머신 월드 오브젝트의 공통 베이스(`AWxGimmick`)와 구현(`AWxDoor`/`AWxElevator`/`AWxTreasureChest`/`AWxAlarmConsole`/`AWxSpawnConsole`/`AWxCutsceneTrigger`).
- 스포너: 레벨 배치 스포너(`AWxSpawner`)와 이를 레지스트리로 묶어 일괄 리스폰·처치 역조회를 처리하는 월드 서브시스템(`UWxSpawnerSubsystem`).
- 상태 보존: 기믹/스포너의 발동·처치 상태를 리플리케이션 + Level Streaming + WxSave 슬롯에 걸쳐 일관되게 복원(`ApplyState` 후크).

**경계 (비담당)**
- 상호작용 입력 처리(가장 가까운 컴포넌트 선택·`TryInteract` 호출)는 어빌리티(`WxAbility_Interact`)가 담당 — [[WxCombat]] 측 GAS 어빌리티.
- `IWxSavable`/`IWxInteractionSource` 계약 정의는 [[WxCore]], 세이브 슬롯 직렬화는 [[WxSave]]에 위임.
- 보물상자 보상 지급은 C++가 아닌 상속 BP에서 추가하는 보상 컴포넌트(WxRewardComponent)가 담당 — [[WxInventory]] (플러그인 간 참조 금지 회피).
- AI/AIController 빙의 로직은 [[WxAI]]. 스포너는 인스턴스를 만들고 `OnSpawnedBy` 훅만 제공한다.

## 의존성
- **주요 의존**: `WxCore` (`WxSavable`, `WxInteractionSource`), `GameplayTags`, `DeveloperSettings`; 엔진 서브시스템 — `GameplayAbilities`, `Niagara`, `LevelSequence`/`MovieScene`, `UMG`.
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInteractionComponent` | 상호작용 영역 = SphereComponent. `IWxInteractionSource` 구현. 모든 기믹이 영역 수만큼 보유 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `AWxGimmick` | 모든 기믹의 abstract 베이스. `bTriggered`/`ApplyState` 후크 + `IWxSavable` | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxElevator` | 가장 복잡한 기믹. 5-상태 머신 + 스플라인 이동 — 상태 복원 패턴의 레퍼런스 | `Source/WxWorld/Public/Gimmick/WxElevator.h` |
| `AWxSpawner` | 레벨 배치 스포너. 처치 상태(`bIsKilled`)·부활 정책 보유. `IWxSavable` | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `UWxSpawnerSubsystem` | 스포너 레지스트리. 일괄 리스폰·spawnable→spawner 역조회 | `Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `IWxSpawnableInterface` | 스폰 대상 계약. `OnSpawnedBy`(빙의 전 컨텍스트 주입) + 에디터 프리뷰 메시 | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `IWxInteractionWidgetInterface` | 프롬프트 위젯 BP 계약. 컴포넌트가 `InteractionText`를 전달 | `Source/WxWorld/Public/Interaction/WxInteractionWidgetInterface.h` |
| `UWxSpawnerLibrary` | `RespawnAutoSpawners` BP 진입점 (서브시스템 thin wrapper) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속 → 자식이 메시/`UWxInteractionComponent`를 직접 들고 `SceneRoot`에 `SetupAttachment`. `OnInteracted` 델리게이트에 `Handle...` 핸들러 바인딩. 발동 영구화가 필요하면 `MarkTriggered()` 호출, 상태 시각화는 `ApplyState()` 오버라이드.
- **상태 복원 규약**: 보존 필드는 `UPROPERTY(SaveGame)`. `ApplyState`는 (a)서버 `MarkTriggered` (b)`OnRep` (c)자식 `BeginPlay` 끝 (d)자식 State `OnRep`/전이 (e)`OnWxSaveRestored` 의 다섯 경로에서 단일하게 호출되어 리플리케이션·Level Streaming·WxSave를 한 지점으로 수렴시킨다.
- **권한 모델**: `TryInteract`/`Respawn`/`Mark*`는 서버 권위. 핸들러 내부는 `GetOwner()->HasAuthority()`로 분기. 클라 호출은 무시되거나 Multicast로만 전파. `WxSaveId`(FGuid)는 에디터에서 1회 부여되어 세션 간 불변 키로 쓰인다.
- **새 스폰 대상**: `IWxSpawnableInterface` 구현 후 스포너의 `SpawnableActorClass`에 지정(`MustImplement` 메타로 강제). 보스 등 영구 사망은 `bNeverRevive`, 콘솔 트리거 전용은 `SpawnMode=Manual`.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 흐름(오버랩→프롬프트→서버 TryInteract→Multicast)의 진입점, 헤더 주석에 전체 시퀀스 정리됨.
2. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — `ApplyState`/`bTriggered`/WxSave 통합 규약. 모든 기믹의 상태 복원이 여기서 파생된다.
3. `Source/WxWorld/Public/Spawnable/WxSpawner.h` + `System/WxSpawnerSubsystem.h` — 스폰·처치·리스폰 서버 권위 흐름과 레지스트리 위임.

## 관련
- 상위: 상호작용 입력은 [[WxCombat]] `WxAbility_Interact`가 구동. 세이브 복원은 [[WxSave]] 슬롯. 보상은 [[WxInventory]] BP 컴포넌트. 계약 타입은 [[WxCore]].

---
*문서 기준 커밋 `97dc8aa` · 생성일 2026-06-14 · 소스 26파일 — `/readme-writer`로 갱신*
