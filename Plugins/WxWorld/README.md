# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 가능 오브젝트(기믹)와 그 상호작용·스폰 인프라를 담는 도메인 플러그인. 기믹의 상태는 서버 권위 State 태그가 원천이고, 비주얼·사이드이펙트는 StateTree 노드로 위임된다.

## 책임
**담당**
- 상호작용 파이프라인: 쿼리 볼륨(`UWxInteractionComponent`) → 로컬 레지스트리(`UWxInteractionRegistrySubsystem`) → 서버 권위 발동(`TryInteract` → `OnInteracted`).
- 기믹 베이스 골격(`AWxGimmick`): 복제+SaveGame State 태그, 단일 권위 쓰기 진입점(`CommitGimmickState`), StateTree 구동, 라이브/복원 진입 구분.
- 기믹 종류별 인터랙션 핸들러와 기본 State(Door/Elevator/AlarmConsole/CutsceneTrigger/SpawnConsole/TreasureChest).
- 기믹 StateTree 공용 노드 모음(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/Niagara/스폰/인터랙션 토글 — 전부 State 비의존 순수 비주얼·사이드이펙트.
- 레벨 배치 스포너(`AWxSpawner`)와 일괄 리스폰 진입점(`UWxSpawnerLibrary`), 스폰 대상 훅 인터페이스(`IWxSpawnableInterface`).

**경계 (비담당)**
- 상호작용 입력·스캔 어빌리티, HUD 프롬프트 위젯/뷰모델(레지스트리를 소비) → [[WxUI]] / 어빌리티 측.
- 보상 지급("Wx Grant Reward" 태스크·`WxRewardTableRow`·`UWxRewardLibrary`)과 인벤토리 → [[WxInventory]]. (이 모듈은 보상 로우 핸들만 보유, 지급 태스크는 외부.)
- State 태그 정의·저장 슬롯·`IWxSavable`·`IWxInteractionSource` 등 공용 정의 → [[WxCore]].
- 스폰되는 적/AI 본체 로직 → [[WxAI]] / [[WxCombat]].

## 의존성
- **주요 의존**: `WxCore`(State 태그 `WxGameplayTags`, `IWxSavable`, `IWxInteractionSource`), 엔진 `StateTree`/`GameplayStateTree`(기믹 상태머신), `LevelSequence`/`MovieScene`(컷신), `Niagara`(FX), `GameplayAbilities`.
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxInventory` 언급은 메타데이터 `RowType` 문자열과 외부 태스크 참조일 뿐, Build.cs 의존 아님)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 전 기믹의 추상 베이스. State 권위 쓰기·StateTree 구동·Save 통합의 중심 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 모든 기믹 ST 가 공유하는 태스크/조건 struct 모음 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 상호작용 쿼리 볼륨 + `OnInteracted` 델리게이트 소스 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 목록·선택·강조 조율 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 처치 상태(GUID/Save) 보유 레벨 배치 스포너 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 `OnSpawnedBy` 훅 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | 월드 Auto 스포너 일괄 리스폰 BP 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 파생 → 생성자에서 컴포넌트 구성·기본 `State` 지정 → 인터랙션 핸들러에서 `CommitGimmickState`(권위 전용)만 호출. State 쓰기는 항상 C++ 액터 측, ST 는 진입·비주얼만. 자식 컴포넌트는 `SceneRoot` 에 `SetupAttachment`.
- **새 ST 노드 추가**: `FStateTreeTaskCommonBase`/`FStateTreeConditionCommonBase` 파생 struct + InstanceData. State 비의존 순수 동작으로 유지하고, 라이브/복원 구분은 `Transition.SourceStateID` 유효성과 `Gimmick.Restore` 이벤트로 판정(복원 시 1회성 효과 침묵·스냅).
- **새 스폰 대상**: `IWxSpawnableInterface` 구현. `AWxSpawner::SpawnableActorClass` 에 `MustImplement` 로 강제됨.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 클래스 헤더 주석이 「권위 State → ST 진입 → 비주얼/복원」 전체 패턴을 설명. 모듈 이해의 척추.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹이 위임하는 공용 동작 카탈로그. 각 노드의 라이브/복원 규약이 한 파일에 집약.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 3단계 흐름(스캔→레지스트리→서버 발동)의 출발점.
4. 개별 기믹(`WxDoor.h` 등)은 베이스 패턴의 인스턴스로, 차이만 빠르게 훑으면 됨.

## 관련
- 상위: 상호작용 어빌리티/HUD([[WxUI]])가 레지스트리·컴포넌트를 소비, 보상 지급은 [[WxInventory]] 태스크가 기믹 ST 안에서 수행, State 태그·Save 는 [[WxCore]].

---
*문서 기준 커밋 `6f04ccd` · 생성일 2026-06-30 · 소스 29파일 — `/readme-writer`로 갱신*
