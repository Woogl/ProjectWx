# WxWorld — 월드 오브젝트 · 상호작용 시스템

> 상호작용 가능한 월드 오브젝트(기믹)와 플레이어 상호작용 파이프라인, 스폰 배치 액터를 담당한다. 기믹의 상태 전이는 서버 권위 `State` 태그가 원천이고, 비주얼/연출은 StateTree 노드가 그 태그 이벤트로 진입해 처리한다.

## 책임
**담당**
- 상호작용 대상 컴포넌트(`UWxInteractionComponent`)와 로컬 수집·선택·강조 조율(`UWxInteractionRegistrySubsystem`).
- 기믹 베이스(`AWxGimmick`)와 파생 6종(Door/Elevator/CutsceneTrigger/TreasureChest/SpawnConsole/AlarmConsole): 권위 `State` 태그 소유·복제·SaveGame, `CommitGimmickState` 단일 쓰기 진입점.
- 기믹 공용 StateTree 노드 모음(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/Niagara/스폰/인터랙션·입력 토글.
- 레벨 배치 스폰 액터(`AWxSpawner`)와 일괄 리스폰(`UWxSpawnerLibrary`), 스폰 대상 규약(`IWxSpawnableInterface`).

**경계 (비담당)**
- 플레이어 측 상호작용 스캔·입력·타겟 전달(상호작용 어빌리티)과 HUD 프롬프트 리스트/뷰모델은 이 모듈 밖(전투/UI). 레지스트리는 로컬 표시용 후보 목록만 소유한다.
- 상호작용 소스 계약(`IWxInteractionSource`)·충돌 채널·`Gimmick.*` 태그 선언·`IWxSavable` 계약은 [[WxCore]] 정의.
- 세이브 슬롯 직렬화/복원 오케스트레이션은 [[WxSave]]. 보상 지급 태스크(`Wx Grant Reward`)와 `WxRewardTableRow`는 [[WxInventory]](TreasureChest 는 데이터 핸들만 보유).

## 의존성
- **주요 의존**: `WxCore`(공용 태그·인터페이스·충돌 채널). 엔진: StateTree/GameplayStateTree(기믹 상태머신), Niagara(FX), GameplayAbilities, LevelSequence/MovieScene(컷신), DeveloperSettings.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs·uplugin 상 Wx 모듈은 `WxCore` 뿐. WxInventory 는 `FDataTableRowHandle` 의 `RowType` 문자열 메타로만 참조되며 컴파일 의존이 아니다.)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 기믹 추상 베이스. `State` 태그 소유·복제·SaveGame, `CommitGimmickState`/`OnRep_GimmickState` 로 StateTree 진입 구동. 파생 6종의 부모 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 공용 ST 태스크/조건(ComponentMove·SplineMove·PlayAnimation·PlayLevelSequence·PlaySound·SpawnNiagara·TriggerSpawners·SpawnActor·EnableInteraction·EnablePlayerInput·GimmickStateIs) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 상호작용 상태·로직 보유 SceneComponent. `IWxInteractionSource` 구현, `TryInteract`(서버 권위) → `OnInteracted` fire | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer 서브시스템. 인-레인지 컴포넌트 수집·선택·외곽선 강조 조율(로컬 표시 전용) | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. 처치 상태(`bIsKilled`) GUID 보존, `Respawn`/`MarkKilled`, `IWxSavable` | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 규약. `OnSpawnedBy`(빙의 전 훅), 에디터 미리보기 메시 추출 | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | BP 진입점. 월드의 Auto 모드 Spawner 일괄 리스폰(`TryRespawnAll`) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | Config. 스포너 클래스별 에디터 아이콘 매핑 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
- 이 모듈은 `Gimmick.*` 태그를 소비만 하며 **선언은 [[WxCore]]** (`Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, 예: `Gimmick_Door_Open`, `Gimmick_Elevator_AtEnd`).
- 각 태그 = 해당 기믹의 권위 `State` 값이자 GimmickStateTree 의 Required Event to Enter. `Gimmick.Restore` 는 복원 진입 마커(일회성 노드가 라이브 발동과 구분).

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속 → 생성자에서 컴포넌트/기본 `State` 태그 지정 → 인터랙션 핸들러에서 `CommitGimmickState(NewTag)` 만 호출(State 쓰기는 서버 권위·C++ 전용). 비주얼/연출은 자식 BP 에 할당한 ST 에셋의 각 상태가 공용 노드로 author. 컴포넌트는 `AllowPrivateAccess` 로 노출해 ST 가 Context 액터 바인딩으로 참조.
- **새 상호작용 로직**: 액터에 `UWxInteractionComponent` 추가 → `OnInteracted` 델리게이트에 `Handle...` 핸들러 바인딩(서버 권위에서만 fire). 다중 영역은 컴포넌트를 영역 수만큼 추가.
- **새 스폰 대상**: 액터가 `IWxSpawnableInterface` 구현 → `AWxSpawner.SpawnableActorClass` 에 지정(`MustImplement` 강제).
- **리플리케이션**: 기믹 `State` 는 `ReplicatedUsing=OnRep_GimmickState` + `SaveGame`, 인터랙션 `bInteractionEnabled` 는 복제. FX/사운드/시퀀스는 모든 피어가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트 없음. State 재선택으로 클라 비주얼이 서버 권위에 수렴.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 「권위 State 태그 → StateTree 진입」 패턴의 중심. 이걸 이해하면 파생 6종이 전부 읽힌다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹 비주얼/연출이 실제로 어디서 일어나는지. 초기진입 vs 라이브전이 구분·복원 스냅 규약이 핵심.
3. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 3단계 흐름(스캔 → 서버 TryInteract → OnInteracted)과 볼륨/강조/레지스트리 경계.

## 관련
- 상위: [[WxGame]]
- 의존: [[WxCore]] · 협력: [[WxSave]] · [[WxInventory]]

---
*문서 기준 커밋 `83a7315` · 생성일 2026-07-11 · 소스 29파일 — `/readme-writer`로 갱신*
