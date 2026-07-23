# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 기믹(문/엘리베이터/보물상자/콘솔/컷신 트리거)과 스폰 시스템, 그리고 플레이어 상호작용 스캔·선택·프롬프트 레지스트리를 담당하는 도메인 플러그인.

## 책임
**담당**
- 상호작용 기믹의 공통 프레임: `AWxGimmick` 베이스가 서버 권위 State 태그(복제 + SaveGame)와 GimmickStateTree 구동 패턴(권위 쓰기 → 복제 → ST 이벤트 진입)을 제공한다.
- 기믹 구현체: Door / Elevator / TreasureChest / AlarmConsole / SpawnConsole / CutsceneTrigger.
- 기믹 StateTree 공용 노드 라이브러리(`WxGimmickStateTreeNodes`): 이동/애니/시퀀스/사운드/Niagara/스폰/인터랙션·입력 토글 태스크 + State 비교 조건.
- 상호작용 컴포넌트(`UWxInteractionComponent`)와 PlayerController 부착 레지스트리(`UWxInteractionRegistryComponent`): 볼륨 쿼리 표식, 인-레인지 수집, 선택/외곽선 강조, `ServerInteract` 전송.
- 스폰 시스템: `AWxSpawner`(처치 상태 GUID 보존, 리스폰/부활 금지) + `IWxSpawnableInterface` + `UWxSpawnerLibrary`(일괄 리스폰 BP 진입점).

**경계 (비담당)**
- 상호작용 어빌리티(`Event.Interact` 수신 → 권위 사거리검증 → `TryInteract` 호출) — [[WxCombat]] 계열(GAS 어빌리티 측).
- HUD 프롬프트 리스트 위젯·뷰모델(WBP_InteractionList / VM, 입력 수신) — [[WxUI]].
- 보상 정의·지급 로직(`WxRewardTableRow`, `Wx Grant Reward` ST 태스크, `GrantReward`) — [[WxInventory]]. TreasureChest 는 보상 로우만 노출하고 지급은 위임한다.
- 공용 정의(`IWxSavable`, `IWxInteractable`, `WxGameplayTags`, `WxCollisionChannels`) — [[WxCore]].
- 세이브 슬롯 직렬화·복원 오케스트레이션 — [[WxSave]].

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: `StateTree` + `GameplayStateTree`(기믹 상태머신), `Niagara`(FX), `LevelSequence`/`MovieScene`(컷신), `GameplayAbilities`(상호작용 가용 판정·입력 게이트), `GameplayTags`, `DeveloperSettings`.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore` 만 참조. TreasureChest 의 `WxInventory.WxRewardTableRow` 는 `FDataTableRowHandle` 메타데이터 문자열 경로일 뿐 컴파일 의존이 아님).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 Abstract 베이스. 권위 State 커밋(`CommitGimmickState`)·복제·SaveGame·ST 이벤트 구동을 소유 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 공유 ST 태스크/조건 모음(이동·애니·시퀀스·사운드·Niagara·스폰·토글·State 조건) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 순수 감지 컴포넌트(볼륨·강조·활성 토글). 서버 권위 `TryInteract` → 소유자 `IWxInteractable::OnInteracted` 호출 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistryComponent` | PlayerController 부착 ActorComponent. 소유 클라에서 주변 볼륨 주기 스캔·선택·강조 후 `ServerInteract` 전송(HUD 리스트 소스) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistryComponent.h` |
| `AWxSpawner` | 레벨 배치 스포너. 처치 상태 GUID 보존, Auto/Manual·부활금지 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현. `OnSpawnedBy` 훅 + 에디터 프리뷰 메시 제공 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` BP 진입점(월드의 Auto 스포너 일괄 리스폰) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑(Config=Game) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
- 본 모듈은 Native Tag 를 **선언하지 않는다**. 기믹 State 태그(`Gimmick.*`)와 복원 마커 `Gimmick.Restore`, 상호작용 이벤트(`Event.Interact`) 는 [[WxCore]]의 `WxGameplayTags` 에서 선언되며 여기서는 사용만 한다.

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속(Abstract), 생성자에서 컴포넌트를 `SceneRoot` 에 부착하고 초기 `State` 태그 지정, `IWxInteractable::OnInteracted` override 에서 `CommitGimmickState(NewState)` 로만 State 를 쓴다(서버 권위 단일 진입점, 클라는 `OnRep_GimmickState` 로 복제 추종). 다중 영역이면 `Source` 로 분기한다. 프롬프트는 베이스 `InteractionPrompt`(다중 영역은 `GetInteractionPrompt` override). 비주얼/사이드이펙트는 자식 BP 에 할당한 ST 에셋이 State 태그 이벤트로 진입해 처리한다. State 쓰기는 C++ 만.
- **ST 노드 규약**: 태스크는 State 를 읽지 않는 순수 비주얼/사이드이펙트가 원칙(재사용). 초기 진입(복원/시작/레이트조인)과 라이브 전이는 `Transition.SourceStateID` 유효성으로 구분하고, 발동 FX/사운드/스폰은 복원 시 침묵(`bPlayOnRestore` 로 지속 FX 예외). 즉시완료 토글 태스크는 `bConsideredForCompletion=false` 로 재선택 루프를 피한다. 상호작용 이동/몽타주 연출은 두 태스크(`Wx Move Interactor To Target` → `Wx Play Interactor Montage`)를 상태로 나눠 조립한다.
- **복원 마커**: 저장 State 진입 시 `Gimmick.Restore` 이벤트를 함께 발행 → 일회성 노드가 스냅·스킵한다. `SaveId`(GUID) 는 에디터에서 1회 부여되는 세션 간 불변 키(기믹·스포너 공통).
- **새 스폰 대상**: `IWxSpawnableInterface` 구현. `AWxSpawner::SpawnableActorClass` 는 `MustImplement` 로 강제. `OnSpawnedBy` 는 Deferred Spawn 의 FinishSpawning 이전(빙의 전)에 호출된다. Manual 모드 스포너는 콘솔 등 외부 트리거로만 스폰한다.
- **리플리케이션 규약**: 상호작용 실행은 소유 클라 스캔 → `ServerInteract` RPC → 서버가 `Event.Interact` 를 폰 ASC 로 송출 → 권위 어빌리티가 `TryInteract` → 소유자 `IWxInteractable::OnInteracted`(서버 권위). 클라 비주얼은 복제 State 로 수렴한다.
- **데이터 주도**: 기믹 이동/애니/오프셋/보상 등 세부는 ST 에셋과 액터 UPROPERTY(디자이너 편집)로 author.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹 공통 State 구동/복제/Save 패턴. 헤더 doc-comment 가 핵심 계약을 설명한다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹 비주얼/사이드이펙트를 구성하는 ST 태스크·조건 카탈로그.
3. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h` — 가장 단순한 구현체로 State→ST 흐름을 구체 확인. 이후 Elevator/CutsceneTrigger 로 확장 패턴 비교.
4. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 볼륨/스캔/강조/서버 권위 발화 흐름 전체를 헤더 주석이 서술한다.

## 관련
- 상위/함께: 상호작용 어빌리티 [[WxCombat]], HUD 프롬프트 [[WxUI]], 보상 [[WxInventory]], 세이브/복원 [[WxSave]], 공용 정의 [[WxCore]].

---
*문서 기준 커밋 `b382b78` · 생성일 2026-07-22 · 소스 29파일 — `/readme-writer`로 갱신*
