# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓인 기믹·스포너·상호작용 대상을 다루는 도메인 플러그인. StateTree로 기믹의 상태머신·연출을 구동하고, 플레이어 주변 상호작용 대상을 스캔하며, 처치·상태를 WxSave 슬롯으로 영속한다.

## 책임
**담당**
- 기믹(문·플랫폼·체크포인트 등)의 상태머신·상호작용·상태 영속을 한 컴포넌트(`UWxGimmickStateTreeComponent`)로 통합 — 전용 C++ 액터 없이 임의 액터(순수 BP 포함)를 기믹화.
- 기믹 StateTree가 공유하는 범용 연출 노드(메시 이동·스플라인 이동·애니·몽타주·사운드·Niagara·LevelSequence·GE 적용·입력 토글·상호작용 토글·액터/스포너 스폰).
- 스포너(`AWxSpawner`) 배치·리스폰·영구 처치 상태 관리와 스포너 지향 StateTree 노드.
- 소유 클라 상호작용 스캐너(`UWxInteractionScannerComponent`): 반경 오버랩으로 in-range 대상 수집·선택·하이라이트, 서버로 상호작용 전달.

**경계 (비담당)**
- 상호작용 계약(`IWxInteractable`)·저장 계약(`IWxSavable`)·Native Gameplay Tag 정의(`WxGameplayTags`) — [[WxCore]].
- 상호작용의 권위 실행(사거리·활성 검증 후 대상 호출)은 `WxAbility_Interact`(ServerOnly GAS 어빌리티) — 전투/GAS 도메인. 스캐너는 폰 ASC로 `Event.Interact`만 송출한다.
- in-range 목록의 HUD 표시(`UWxViewModel_InteractionList`) — [[WxUI]].
- 세이브 슬롯 직렬화·복원 오케스트레이션 — [[WxSave]]. 본 모듈은 `IWxSavable` 구현만 제공.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: `StateTree`/`GameplayStateTreeModule`(기믹·스포너 노드), `GameplayAbilities`(상호작용 이벤트·GE·어빌리티 차단), `Niagara`·`LevelSequence`/`MovieScene`(연출 노드), `ModularGameplay`(`UControllerComponent` 주입), `UniversalObjectLocator`(스포너 로케이터 참조), `AIModule`(`UStateTreeComponent` = `UBrainComponent` 파생이라 공개 헤더로 노출).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 기믹의 상태머신·상호작용(`IWxInteractable`)·영속(`IWxSavable`)을 담는 StateTree 컴포넌트 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `FWxStateTreeTask_*` (기믹 노드) | 기믹 ST 공유 연출 태스크 모음(이동·스플라인·애니·몽타주·사운드·FX·GE·입력·상호작용·스폰 등) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `AWxSpawner` | 레벨 배치 스포너. `SpawnableActorClass` 인스턴스 생성·리스폰·처치 상태(`bIsKilled`) 영속 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후(빙의 전) per-instance 컨텍스트 주입 훅(`OnSpawnedBy`) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_TriggerSpawnersByLocator` / `_WaitSpawnersKilled` | UOL로 배치 스포너를 지정해 트리거·처치 대기하는 ST 노드(레벨 밖 호스트용) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h` |
| `UWxInteractionScannerComponent` | PC 부착 상호작용 스캐너. in-range 수집·선택·하이라이트·`ServerInteract` | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` BP 진입점(월드의 Auto 스포너 일괄 리스폰) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹**: 전용 액터 불필요. 임의 액터에 `UWxGimmickStateTreeComponent`를 붙이고 StateTree 에셋을 지정하면 기믹이 된다. 상태 식별은 엔진 순정 상태 Tag이며, 상태 디테일의 Tag 필드에 단 값이 곧 세이브 키다(에셋 내 유일, Tag 없는 상태는 저장 안 됨). 상태 전이는 전부 ST 에셋이 정하고 컴포넌트는 목적지를 모른다. **오너 액터의 Replicates를 반드시 켠다** — 컴포넌트는 자기 몫만 켤 수 있어 꺼져 있으면 상태 복제·상호작용 멀티캐스트가 죽는다(로컬 플레이에선 정상으로 보이므로 `BeginPlay`가 Error 로그로 알린다).
- **새 연출 노드**: `FStateTreeTaskCommonBase` 파생 `FWxStateTreeTask_*`를 추가한다. 노드는 소유 액터의 얇은 프리미티브만 호출하고 기믹 종류를 모른다. 초기 진입(시작/복원/레이트조인)과 라이브 전이는 `Transition.SourceStateID` 유효성으로 구분 — 발동형 액션은 라이브에서만, 상태형 포즈는 진입 경로 무관하게 목표로 수렴, 작업 완료 시 `Succeeded` 반환. 재선택 재발동 여부는 생성자의 `bShouldStateChangeOnReselect`로 선언.
- **새 스폰 대상**: `AActor`에 `IWxSpawnable`을 구현하고 `AWxSpawner::SpawnableActorClass`(MustImplement로 강제)에 지정한다. `OnSpawnedBy`에서 스폰 컨텍스트를 끌어간다. 보스 등 부활 금지는 `bNeverRevive`, 외부 트리거 전용은 `SpawnMode=Manual`.
- **리플리케이션**: 기믹은 권위 측이 활성 상태 Tag를 `StateTag`(Replicated+SaveGame)에 폴링 기록하고, 클라는 멀티캐스트 이벤트로 같은 전이를 밟되 어긋난 피어는 `OnRep_StateTag`로 그 상태에서 재시작해 수렴한다. 상호작용 스캐너는 소유 클라 전용(복제 안 됨)이고 선택은 `ServerInteract`로 메시 포인터를 원자 전송한다.
- **세이브 키**: 기믹·스포너의 `SaveId`는 에디터에서 부여돼 에셋에 직렬화된 GUID다(오너 경로 파생 금지 — World Partition 셀 좌표 때문). 배치 후 맵을 저장해야 키가 남으며, 저장 전이거나 런타임 스폰된 것은 저장/복원에서 제외된다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 기믹의 세 책임(상호작용·영속·ST 구동)과 상태 구동/복제 패턴이 모듈의 중심.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹 ST가 조립하는 연출 노드 카탈로그. 초기 진입 vs 라이브 전이 규약이 상단 주석에 정리됨.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 스캔→선택→서버 전달→GAS 실행의 전체 흐름과 로컬리티 근거.
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스포너의 처치 상태·리스폰·영구 사망 모델.

## 관련
- 상위: [[WxGame]] / 기믹·스포너 배치와 ST 에셋은 GameFeature·레벨 콘텐츠에서 조립
- 기반: [[WxCore]] (상호작용/저장 계약·Gameplay Tag)
- 협력: [[WxCombat]] (상호작용 어빌리티·GE), [[WxUI]] (상호작용 HUD), [[WxSave]] (슬롯 영속)

---
*문서 기준 커밋 `bb8ee6b` · 생성일 2026-08-07 · 소스 18파일 — `/readme-writer`로 갱신*
