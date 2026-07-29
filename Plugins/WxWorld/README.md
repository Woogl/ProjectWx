# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹(문·보물상자·엘리베이터·체크포인트)과 스포너, 그리고 플레이어 측 상호작용 스캐너를 담는 도메인. 각 기믹은 복제·SaveGame 되는 State 태그를 권위 원천으로 두고, 비주얼·연출·사이드이펙트는 StateTree 공유 태스크가 그 태그로 진입해 처리한다.

## 책임
**담당**
- 상호작용 기믹 공통 프레임(`AWxGimmick`, 추상): 권위 State 태그(복제 + SaveGame) 쓰기 단일 진입점(`CommitGimmickState`), StateTree 재선택 구동, 상호작용 영역/프롬프트 토글, `IWxSavable` 통합.
- 구체 기믹: `AWxDoor`, `AWxTreasureChest`, `AWxElevator`, `AWxCheckPoint`.
- 기믹/스포너용 StateTree 공유 태스크 노드 — State 를 읽지 않는 순수 프리미티브(이동·애니·사운드·Niagara·시퀀스·상호작용자 이동/몽타주·액터 스폰·상호작용/입력 토글 등).
- 스포너(`AWxSpawner`): 배치형 스폰 액터, 처치 상태 보존(SaveGame), 리스폰/부활 금지, 일괄 리스폰(`UWxSpawnerLibrary`), 스폰 대상 훅(`IWxSpawnableInterface`).
- 플레이어 측 상호작용 스캐너(`UWxInteractionScannerComponent`): 소유 클라 반경 스캔·선택·외곽선 강조·`ServerInteract` 전송.

**경계 (비담당)**
- `IWxInteractable`·`IWxSavable` 인터페이스와 `WxGameplayTags`(Gimmick.*/Event.Interact/StateTree.Restore) 정의 — [[WxCore]].
- 상호작용 어빌리티의 권위 실행(사거리·활성 검증, `WxAbility_Interact`)과 ASC `Event.Interact` 처리 — [[WxCombat]]. 스캐너는 선택 메시를 서버로 넘길 뿐.
- 보상 지급('Grant Reward')·충전 아이템 리필('Refill Item Charges') ST 태스크 — [[WxInventory]] 제공. 기믹은 코드로 참조하지 않고 ST 에셋에서 조립.
- 세이브 슬롯 직렬화/복원 오케스트레이션 — [[WxSave]] (여기선 `IWxSavable` 구현만).
- HUD 프롬프트/선택 뷰모델 표시 — [[WxUI]].

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: StateTree/GameplayStateTree(상태머신), GameplayTags(State 태그), GameplayAbilities(상호작용/입력 차단), Niagara·LevelSequence·MovieScene(연출), ModularGameplay, UniversalObjectLocator(스포너 로케이터 지정). `Gimmick.*` State 태그는 WxCore 가 선언.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs 상 Wx 의존은 `WxCore` 하나. WxInventory 는 `RowType` 문자열 메타·주석뿐이고, WxCombat/WxUI 연계는 GAS 이벤트·ST 에셋 조립으로 느슨 결합).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 상호작용 월드 오브젝트 공통 부모(추상). State 태그 권위 쓰기·ST 구동·상호작용/프롬프트 토글·`IWxSavable` 의 단일 소유자 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxDoor` · `AWxTreasureChest` · `AWxElevator` · `AWxCheckPoint` | 구체 기믹. `OnInteracted` 에서 다음 State 확정만 하고 연출은 ST 에 위임 | `Source/WxWorld/Public/Gimmick/` |
| `FWxStateTreeTask_*` (기믹, 12종) | 기믹 ST 공유 태스크(EnableInteraction/ComponentMove/PlayLevelSequence/SpawnActor 등). 초기진입·복원·라이브 전이를 노드가 구분 | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionScannerComponent` | PlayerController 부착, 소유 클라 반경 스캔·선택·강조·`ServerInteract` | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` · `IWxSpawnableInterface` | 배치형 스폰 액터(처치 보존/리스폰/부활 금지)와 스폰 대상 훅(`OnSpawnedBy`) | `Source/WxWorld/Public/Spawnable/` |
| `FWxStateTreeTask_TriggerSpawnersByLocator` · `_WaitSpawnersKilled` | 로케이터로 배치 스포너 트리거/처치 대기 (컨텍스트 액터 무관, 퀘스트 ST 등에서도 조립) | `Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h` |
| `UWxSpawnerLibrary` · `UWxWorldDeveloperSettings` | 월드 내 Auto 스포너 일괄 리스폰 BP 진입점 · 스포너 클래스별 에디터 아이콘 설정 | `Source/WxWorld/Public/System/` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속(추상) → 생성자에서 컴포넌트 부착(`SceneRoot` 에 `SetupAttachment`)·초기 `State` 태그·기본 활성 영역 지정 → `OnInteracted` override 에서 `SetInteractingCharacter` 후 `CommitGimmickState`(State 쓰기는 **권위 전용, C++ 만**; 클라는 복제 State 추종). 비주얼/사이드이펙트는 자식 BP 가 ST 에셋을 `StateTree` 컴포넌트에 할당해 조립한다.
- **State 구동 규약**: State 태그가 곧 각 상태의 `Required Event to Enter`. Root 에 재선택 전이 1개(On Event: Gimmick → GotoState Root)만 두면 태그 계층 매칭으로 전 상태가 그 전이를 발화. resting(기본) 상태는 Required Event 없이 Root 자식 **마지막**에 둔다(위에 있으면 항상 선택됨). 초기 진입/복원과 라이브 전이는 노드가 `Transition.SourceStateID` 유효성 + `StateTree.Restore` 마커로 구분한다.
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 상속, `Context.GetOwner()` 를 기믹으로 캐스트해 얇은 프리미티브만 호출. 발동형(사운드·스폰 트리거)은 생성자 `bShouldStateChangeOnReselect = true`, 상태형(이동·토글)·머무는 태스크는 false.
- **새 스포너 대상**: 스폰 액터가 `IWxSpawnableInterface` 구현(`MustImplement` 메타로 강제). 콘솔/ST 로 개별 트리거하려면 대상 스포너의 `SpawnMode = Manual`(BeginPlay 자동 스폰·일괄 리스폰과 겹치지 않게).

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹 공통 패턴(권위 State → ST 진입, 복제/복원, 프롬프트/영역 토글)의 계약이 헤더 주석에 집약돼 있다. 여기를 잡으면 모든 구체 기믹이 얇아 보인다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹의 실제 동작이 코드가 아닌 이 12개 공유 태스크로 조립된다. 각 노드의 진입 경로 구분·재선택 규약이 핵심.
3. `Source/WxWorld/Public/Gimmick/WxElevator.h` — 다중 상호작용 영역·스플라인 이동·leaf 시퀀스 choreography 를 쓰는 가장 복합적인 기믹. 프레임 활용의 상한 예시.
4. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 클라 감지 → `ServerInteract` → 서버 GAS 이벤트로 이어지는 상호작용 입력 경로(로컬리티·RPC).

## 관련
- 상위/토대: [[WxCore]] (인터페이스·태그 정의)
- 연계: [[WxCombat]] (상호작용 어빌리티) · [[WxInventory]] (보상/리필 ST 태스크) · [[WxUI]] (프롬프트 표시) · [[WxSave]] (기믹·스포너 상태 영속)

---
*문서 기준 커밋 `a5b5f20` · 생성일 2026-07-29 · 소스 25파일 — `/readme-writer`로 갱신*
