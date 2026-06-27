# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 가능한 기믹(문/엘리베이터/상자/콘솔/컷신 트리거)과 적/오브젝트 스포너를 책임진다. 「권위 State 태그 → 복제/SaveGame → StateTree 진입」 단일 패턴으로 동작·연출·세이브를 통일하고, 플레이어가 무엇과 상호작용할 수 있는지 수집·선택하는 로컬 레지스트리를 제공한다.

## 책임
**담당**
- 기믹 공통 골격(`AWxGimmick`): 서버 권위 State 쓰기 단일 진입점(`CommitGimmickState`), State 복제(`OnRep`)·SaveGame, GimmickStateTree 구동·복원 마커 발행
- 구체 기믹 액터: `AWxDoor` / `AWxElevator` / `AWxTreasureChest` / `AWxAlarmConsole` / `AWxSpawnConsole` / `AWxCutsceneTrigger`
- 기믹 ST가 공유하는 범용 노드 모음(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/FX/스폰/인터랙션 토글 태스크 + State 비교 조건
- 상호작용 쿼리 볼륨(`UWxInteractionComponent`)과 로컬 플레이어 인-레인지/선택 레지스트리(`UWxInteractionRegistrySubsystem`)
- 스포너(`AWxSpawner`): 처치 상태(GUID 키 SaveGame) 보존·리스폰, 일괄 리스폰 라이브러리(`UWxSpawnerLibrary`)

**경계 (비담당)**
- 상호작용 입력·스캔·서버 전달 어빌리티(`WxAbility_Interact`)와 HUD 프롬프트 리스트(`WBP_InteractionList`/VM) — 외부(어빌리티/[[WxUI]])가 본 모듈의 컴포넌트·레지스트리를 소비
- State 태그·`IWxSavable`·`IWxInteractionSource` 정의 → [[WxCore]]
- 보물상자 보상 지급(`Wx Grant Reward` 태스크/`GrantReward`)·보상 로우 타입 → [[WxInventory]] (본 모듈은 RowType 메타 문자열로만 참조, 코드/빌드 의존 없음)

## 의존성
- **주요 의존**: WxCore / StateTree·GameplayStateTree(기믹 상태머신) / GameplayAbilities·GameplayTags / Niagara(FX 태스크) / LevelSequence·MovieScene(컷신 태스크)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`WxWorld.Build.cs`의 Wx 의존은 `WxCore` 뿐. `WxInventory.WxRewardTableRow`는 `meta = (RowType=...)` 문자열일 뿐 빌드 의존이 아님)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 추상 베이스. State 권위 쓰기·복제·SaveGame·ST 구동의 공통 골격 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 기믹 ST가 공유하는 범용 태스크/조건 struct 모음(이동·애니·시퀀스·사운드·FX·스폰·토글) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `AWxDoor` / `AWxElevator` | 대표 구체 기믹. State 확정 + ST 비주얼(ComponentMove / ComponentSplineMove) 분리 패턴 예시 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h`, `WxElevator.h` |
| `UWxInteractionComponent` | 상호작용 쿼리 볼륨 + `OnInteracted` 델리게이트(소유 액터가 핸들러 바인딩) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 수집·선택·하이라이트 조율(HUD 소스) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스포너. 처치 상태 GUID 보존·리스폰, `IWxSpawnableInterface` 대상 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 훅(`OnSpawnedBy`, 에디터 프리뷰 메시) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` — 월드의 Auto 스포너 일괄 리스폰 BP 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## Gameplay Tags
C++ Native Tag 선언은 본 모듈에 없다. 기믹 State·복원 마커 태그(`Gimmick.Door.*`, `Gimmick.Elevator.*`, `Gimmick.TreasureChest.*`, `Gimmick.AlarmConsole.*`, `Gimmick.SpawnConsole.*`, `Gimmick.CutsceneTrigger.*`, `Gimmick.Restore`)는 [[WxCore]]의 `WxGameplayTags`에서 선언된 것을 사용한다.

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`를 상속해 ① 생성자에서 컴포넌트(메시/`UWxInteractionComponent`)와 초기 `State` 태그 지정 ② `BeginPlay`에서 인터랙션 `OnInteracted`에 `Handle*` 핸들러 바인딩 ③ 핸들러는 `CommitGimmickState`로만 State 확정(서버 권위). 비주얼/FX/사이드이펙트는 C++가 아니라 ST 에셋이 State 태그(Required Event to Enter)로 진입한 상태에서 공유 노드로 author 한다.
- **리플리케이션·세이브 모델**: State 쓰기는 무조건 서버 권위(`CommitGimmickState`). `State`는 `ReplicatedUsing=OnRep_GimmickState`+`SaveGame`이며, 서버/클라 양쪽이 State 태그를 ST 이벤트로 발행해 같은 진입 로직으로 수렴한다. 복원(BeginPlay/스트리밍 인)은 `Gimmick.Restore` 마커를 함께 보내 일회성 노드(사운드/FX/스폰/시퀀스)가 발동 대신 스냅·스킵하게 한다. 일시 상태 기믹(CutsceneTrigger)은 SaveGame 없이 복원 시 Idle 리셋.
- **공유 ST 노드 규약**: 노드는 State를 읽지 않는 순수 비주얼/사이드이펙트이며 `Context.GetOwner()` 캐스트로 동작. `Transition.SourceStateID` 유효성으로 초기진입(스냅) vs 라이브전이(연출)를 구분. 작업 종료 시 `Succeeded` 반환으로 상태 완료를 구동하되, 머무는 태스크(SpawnActor 등)는 상태 완료 판정에서 제외해야 재선택 thrash를 피한다.
- **상호작용 추가**: 액터에 `UWxInteractionComponent`를 영역 수만큼 붙이고 `OnInteracted`에 바인딩. 핸들러 내부에서 `HasAuthority()`로 권위 분기. 프롬프트는 컴포넌트가 아니라 레지스트리→HUD가 표시.
- **스폰 대상 규약**: `IWxSpawnableInterface` 구현 필요(`AWxSpawner::SpawnableActorClass`가 `MustImplement`로 강제). 보스 등은 `bNeverRevive`, 콘솔 트리거 전용 스포너는 `SpawnMode=Manual`로 일괄 리스폰에서 제외.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 모든 기믹이 공유하는 State 권위/복제/세이브/ST 구동 패턴. 이걸 알아야 자식 기믹이 읽힌다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 자식 기믹의 "동작"이 실제로 사는 곳(C++ 아닌 ST 노드). 파일 상단 주석이 전 노드 한눈 개요.
3. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h` — 가장 단순한 구체 기믹. State 확정과 ST 비주얼 분리의 최소 예시.
4. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 흐름(스캐너→레지스트리→어빌리티→Multicast) 주석이 모듈 경계를 설명.

## 관련
- 상위: [[WxGame]] 레벨이 기믹/스포너를 배치, 플레이어 [[WxCombat]]의 상호작용 어빌리티가 컴포넌트/레지스트리를 소비, [[WxUI]]가 프롬프트 표시, [[WxInventory]]가 상자 보상 지급, [[WxSave]]가 State/처치 슬롯 보존. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `9e49a09` · 생성일 2026-06-27 · 소스 28파일 — `/readme-writer`로 갱신*
