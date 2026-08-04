# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓이는 상호작용 오브젝트(기믹)와 스포너, 그리고 플레이어가 주변 대상을 감지·선택·실행하는 상호작용 파이프라인을 담는 도메인. 기믹의 상태·상호작용·영속은 StateTree 컴포넌트 한 몸에 담고, 플레이어 쪽 스캔·선택은 컨트롤러 컴포넌트가 맡는다.

## 책임
**담당**
- 상호작용 기믹 프레임워크: `UWxGimmickStateTreeComponent`(상태머신 + `IWxInteractable` + `IWxSavable`를 한 몸에)와 이를 구동하는 공유 StateTree 태스크 라이브러리(이동·애니·사운드·FX·컷신·스폰·입력/효과 토글)
- 플레이어 상호작용 스캔·선택·하이라이트(`UWxInteractionScannerComponent`, PlayerController 소유·소유 클라 구동)
- 스폰 오브젝트 배치·처치·리스폰(`AWxSpawner`, `IWxSpawnable`)와 스포너를 트리거·처치 대기시키는 StateTree 노드

**경계 (비담당)**
- 상호작용 어빌리티의 권위 실행(사거리·활성 검증, `Ability.Interact`)·GameplayEffect 적용 — [[WxCombat]]/GAS (스캐너는 폰 ASC로 `Event.Interact`만 송출)
- 보상 지급·아이템 충전 리필 등 콘텐츠 태스크(체크포인트·상자가 ST 에셋에서 조립해 호출) — [[WxInventory]]
- HUD 상호작용 리스트/선택 표시(뷰모델) — [[WxUI]] (스캐너는 델리게이트·데이터만 노출)
- 상호작용/세이브 인터페이스 정의(`IWxInteractable`·`IWxSavable`)와 세이브 슬롯 오케스트레이션 — [[WxCore]]/[[WxSave]]

## 의존성
- **주요 의존**: `WxCore`(`WxInteractable`·`WxSavable` 인터페이스). 엔진: StateTree·GameplayStateTree(기믹·스포너 노드), ModularGameplay(`UControllerComponent` 주입), GameplayAbilities(GE 적용·어빌리티 차단, private), Niagara·LevelSequence/MovieScene(연출 태스크, private), UniversalObjectLocator(레벨 밖 호스트에서 스포너 지정), DeveloperSettings(스포너 아이콘), UnrealEd(에디터 라벨만, private)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (WxCombat/WxInventory/WxUI/WxSave와의 연결은 전부 ST 에셋·Experience 배선과 에디터 메타 문자열뿐이며, `WxWorld.Build.cs`의 Wx 의존은 `WxCore`만 등재)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 기믹의 상태머신·상호작용 계약·상태 영속을 한 몸에 담는 ST 컴포넌트. 붙이면 어떤 액터든(순수 BP 포함) 기믹이 된다 | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `WxGimmickStateTreeNodes.h` | 기믹 종류 무관 공유 ST 태스크 모음(EnableInteraction·ComponentMove·ComponentSplineMove·MoveInteractorToTarget·PlayLevelSequence·SpawnActor 등) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionScannerComponent` | 소유 클라에서 주변 상호작용 메시를 주기 스캔·선택·하이라이트하고 `ServerInteract` 송출 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 인스턴스를 스폰·처치·리스폰하는 배치 액터(`IWxSavable`로 처치 상태 영속) | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후(빙의 전) per-instance 컨텍스트를 스포너에서 끌어오는 훅(`OnSpawnedBy`) | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `WxSpawnerStateTreeNodes.h` | 배치 스포너를 UOL로 직접 지정해 트리거(`TriggerSpawnersByLocator`)·처치 대기(`WaitSpawnersKilled`)하는 ST 노드 | `Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` — 월드의 Auto 스포너 일괄 리스폰(BP 진입점) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **신규 기믹**: 상속할 베이스 클래스가 없다 — 아무 액터(순수 BP 포함)에 `UWxGimmickStateTreeComponent`를 붙이고 ST 에셋을 할당하면 기믹이 된다. 상태 전이·연출·인터랙션 가용성은 전부 ST 에셋에서 조립하고, 태스크는 스키마의 Context Actor로 그 액터의 컴포넌트·변수에 바인딩한다.
- **상태·영속 규약**: ST 상태 디테일의 Tag 필드가 곧 저장 키다(에셋 안에서 유일). 권위 측이 틱마다 활성 상태 Tag를 `StateTag`(Replicated+SaveGame)에 반영하고, 클라·레이트조인·복원은 복제된 Tag로 그 상태에서 트리를 재시작해 수렴한다. Tag 없는 상태는 저장되지 않는다. `SaveId`는 에디터에서 오너 ActorGuid로 심어 에셋에 직렬화하므로, 배치 후 맵을 한 번 저장해야 저장/복원 대상이 된다(WP 스트리밍 경로를 키에 섞지 않으려는 설계).
- **초기 진입 vs 라이브 전이**: 모든 ST 태스크가 `Transition.SourceStateID` 유효성으로 구분한다 — 트리거형(사운드·GE·스폰·컷신)은 라이브 진입에서만, 상태형(메시 포즈·인터랙션/입력 토글)은 진입 경로 무관하게 적용. 재선택(Sustained) 재실행 여부는 각 노드 생성자의 `bShouldStateChangeOnReselect`가 선언한다.
- **새 상호작용 대상**: `IWxInteractable`(WxCore)을 구현해 활성 메시·프롬프트·`OnInteracted`를 답하면 스캐너 후보가 된다. 쿼리 콜리전만 켜져 있으면 되고 콜리전 프리셋·응답과 무관하다.
- **신규 스폰 대상**: `IWxSpawnable`를 구현하고 `AWxSpawner::SpawnableActorClass`에 지정(`MustImplement` 강제). `EWxSpawnerMode::Auto`는 일괄 리스폰(`TryRespawnAll`) 대상, `Manual`은 ST 노드 개별 트리거 전용. `bNeverRevive`로 보스류 영구 처치.
- **신규 ST 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT + InstanceData 쌍으로 추가. 종류 무관 공통 동작은 `WxGimmickStateTreeNodes.h`(Context 액터 전제), 스포너 대상은 `WxSpawnerStateTreeNodes.h`(UOL 지정, 액터 클래스 검증은 `Compile`)에 둔다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 기믹의 세 책임(상호작용·영속·ST 구동)과 복제·복원 수렴 모델이 전부 헤더 주석에 있다. 모듈의 심장.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹이 실제로 무엇을 할 수 있는지(연출·이동·인터랙션 토글·스폰)의 카탈로그. 파일 상단 주석이 노드별 계약과 초기/라이브 규약을 요약한다.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 입력이 어디서 시작돼 어떻게 서버 권위 실행으로 넘어가는지(스캔→선택→ServerInteract→어빌리티)와 다른 도메인과의 경계.
4. `Source/WxWorld/Public/Spawnable/WxSpawner.h` + `WxSpawnerStateTreeNodes.h` — 스폰·처치·리스폰·세이브 연동과 ST에서 스포너를 트리거/대기시키는 흐름.

## 관련
- 상위: [[WxGame]], GameFeature 플러그인(콘텐츠 배치)
- 인접: [[WxCore]](인터페이스), [[WxCombat]](상호작용 어빌리티·GAS), [[WxInventory]](보상·리필 태스크), [[WxUI]](프롬프트 HUD), [[WxSave]](세이브 슬롯)

---
*문서 기준 커밋 `28ee2c6` · 생성일 2026-08-03 · 소스 18파일 — `/readme-writer`로 갱신*
