# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹(문·엘리베이터·체크포인트·상자·콘솔·컷신)과 적 스포너, 그리고 플레이어 측 상호작용 스캐너를 담는 도메인. 각 기믹은 복제·SaveGame 되는 State 태그를 권위 원천으로 두고, 비주얼·연출·사이드이펙트는 StateTree 공유 태스크가 그 태그로 진입해 처리한다.

## 책임
**담당**
- 상호작용 기믹 공통 프레임(`AWxGimmick`): 권위 State 태그(복제 + SaveGame) 쓰기 단일 진입점(`CommitGimmickState`), StateTree 구동, 상호작용 영역/프롬프트 토글, WxSave 통합.
- 구체 기믹: `AWxDoor`, `AWxElevator`, `AWxCheckPoint`, `AWxTreasureChest`, `AWxCutsceneTrigger`, `AWxAlarmConsole`, `AWxSpawnConsole`.
- 기믹/스포너용 StateTree 공유 태스크 노드(이동·애니·사운드·Niagara·시퀀스·스폰·상호작용 토글 등).
- 스포너(`AWxSpawner`): 배치형 스폰 액터, 처치 상태 보존(SaveGame), 리스폰, 일괄 리스폰(`UWxSpawnerLibrary`).
- 플레이어 측 상호작용 스캐너 컴포넌트(`UWxInteractionScannerComponent`): 주변 후보 스캔·선택·하이라이트, 서버로 상호작용 요청 전달.

**경계 (비담당)**
- `IWxInteractable`·`IWxSavable` 인터페이스 정의, `WxGameplayTags`(Gimmick.*/Event.Interact/StateTree.Restore) — [[WxCore]].
- 상호작용 어빌리티(권위 사거리·활성 검증, `WxAbility_Interact`)와 ASC 이벤트 처리 — [[WxCombat]].
- 보상 지급/충전형 아이템 리필 ST 태스크(`Grant Reward`·`Refill Item Charges`) — [[WxInventory]]. 기믹이 직접 참조하지 않고 ST 에셋에서 조립.
- HUD 상호작용 리스트/선택 뷰모델 표시 — [[WxUI]].

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 의존). 엔진: StateTree/GameplayStateTree(상태머신), GameplayTags(State 태그), GameplayAbilities(상호작용/입력 차단), Niagara·LevelSequence·MovieScene(연출), UniversalObjectLocator(스포너 로케이터 지정).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs 상 Wx 의존은 `WxCore` 하나뿐. WxInventory/WxUI/WxCombat 연계는 모두 ST 에셋 조립·GAS 이벤트로 느슨 결합).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 상호작용 월드 오브젝트 공통 부모(추상). State 태그·ST·WxSave 소유 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `FWxStateTreeTask_*` | 기믹 ST 공유 태스크 모음(EnableInteraction/ComponentMove/PlayLevelSequence/SpawnActor 등 12종) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `AWxSpawner` | 배치형 스폰 액터. 처치 상태·리스폰·부활 금지 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 계약(`OnSpawnedBy` 훅). SpawnableActorClass 가 구현 필수 | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `FWxStateTreeTask_TriggerSpawnersByLocator` / `_WaitSpawnersKilled` | 로케이터 기반 스포너 ST 태스크(레벨 밖 호스트에서 조립) | `Source/WxWorld/Public/Spawnable/WxSpawnerStateTreeNodes.h` |
| `UWxInteractionScannerComponent` | PlayerController 부착 상호작용 스캐너(스캔·선택·하이라이트·ServerInteract) | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `UWxSpawnerLibrary` | 월드 내 Auto 스포너 일괄 리스폰 BP 진입점 | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 설정 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속(추상) → 생성자에서 컴포넌트 부착(`SceneRoot`에 `SetupAttachment`)·초기 `State` 태그 지정 → `OnInteracted` override 에서 `SetInteractingCharacter` 후 `CommitGimmickState`(State 쓰기는 **권위 전용, C++ 만**). 비주얼/사이드이펙트는 자식 BP 가 ST 에셋을 `StateTree` 컴포넌트에 할당해 처리한다.
- **State 구동 규약**: State 태그가 곧 각 상태의 `Required Event to Enter`. Root 에 재선택 전이 1개(On Event: Gimmick → Root)만 두면 태그 계층 매칭으로 전 상태가 그 전이를 발화. resting(기본) 상태는 Required Event 없이 Root 자식 **마지막**에 둔다. 초기 진입/복원과 라이브 전이는 노드가 `Transition.SourceStateID` 유효성 + `StateTree.Restore` 마커로 구분한다.
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 상속, `Context.GetOwner()`를 기믹으로 캐스트해 얇은 프리미티브만 호출. 발동형(사운드·스폰 트리거)은 생성자 `bShouldStateChangeOnReselect = true`, 상태형(이동·토글)·머무는 태스크는 false.
- **새 스포너 대상**: 스폰 액터가 `IWxSpawnableInterface` 구현(`MustImplement` 메타로 강제). 콘솔/ST 로 트리거하려면 대상 스포너의 `SpawnMode = Manual`(BeginPlay 자동 스폰·일괄 리스폰과 겹치지 않게).

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹 공통 패턴(권위 State → ST 진입, WxSave, 프롬프트/영역 토글)의 계약이 헤더 주석에 정리돼 있다. 여기를 이해하면 모든 구체 기믹이 얇아 보인다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹의 실제 동작(연출·이동·스폰)이 전부 이 공유 태스크에 있다. 각 노드의 진입 경로 구분·재선택 규약이 핵심.
3. `Source/WxWorld/Public/Gimmick/WxElevator.h` — 다중 상호작용 영역·스플라인 이동·leaf 시퀀스 choreography 를 쓰는 가장 복합적인 기믹. 프레임 활용의 상한 예시.
4. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 클라 감지 → 서버 실행으로 이어지는 상호작용 입력 경로(로컬리티·RPC·GAS 이벤트).

## 관련
- 상위/토대: [[WxCore]] (인터페이스·태그 정의)
- 연계: [[WxCombat]] (상호작용 어빌리티), [[WxInventory]] (보상/리필 ST 태스크), [[WxUI]] (프롬프트 표시), [[WxSave]] (기믹·스포너 상태 영속)

---
*문서 기준 커밋 `21e2e76` · 생성일 2026-07-27 · 소스 31파일 — `/readme-writer`로 갱신*
