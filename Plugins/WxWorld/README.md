# WxWorld — 월드 오브젝트 및 상호작용

> 문·엘리베이터·보물상자 같은 상호작용 가능한 월드 기믹과, 스폰 배치 액터, 플레이어 상호작용 스캐너를 담는 도메인 모듈. 기믹의 상태·연출은 공통 베이스(`AWxGimmick`)와 재사용 StateTree 노드 라이브러리로 데이터 주도 조립한다.

## 책임
**담당**
- 상호작용 가능한 월드 기믹의 공통 골격: 서버 권위 State 태그 → 복제/SaveGame → GimmickStateTree 진입 패턴 (`AWxGimmick`)과 그 자식 기믹 일체(문·엘리베이터·보물상자·컷신 트리거·콘솔·체크포인트)
- 기믹 StateTree 가 공유하는 범용 태스크/조건 노드(메시 이동·애니·시퀀스·사운드·Niagara·상호작용/입력 토글·스포너/액터 스폰) (`WxGimmickStateTreeNodes`)
- 플레이어 측 상호작용 스캔·선택·하이라이트와 서버 상호작용 RPC (`UWxInteractionScannerComponent`)
- 스폰 배치 액터와 처치/부활 상태 보존, 일괄 리스폰 (`AWxSpawner`, `UWxSpawnerLibrary`)

**경계 (비담당)**
- 상호작용 어빌리티(사거리·활성 검증, `Event.Interact` 처리)와 `IWxInteractable`/`IWxSavable` 인터페이스 정의는 [[WxCore]]/전투·세이브 도메인 소관 (스캐너는 선택만 서버로 전송)
- 보상 지급 로직(`Grant Reward` 태스크, `UWxRewardLibrary`)은 [[WxInventory]] — 보물상자는 `RewardRow` 데이터만 노출. 체크포인트의 충전형 아이템 리필(`Refill Item Charges` 태스크)도 같은 소관
- 세이브 슬롯 직렬화·복원 오케스트레이션은 [[WxSave]] (기믹/스포너는 `IWxSavable` 구현만)

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: StateTree/GameplayStateTree(기믹 상태머신), Niagara(FX), LevelSequence·MovieScene(컷신), GameplayAbilities(상호작용 게이팅), DeveloperSettings
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`WxWorld.Build.cs` 의 Wx 의존은 `WxCore` 뿐. 보물상자의 `WxInventory` 언급은 doc-comment 와 `RowType` 메타 문자열일 뿐 코드/빌드 의존 아님)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 상호작용 기믹의 Abstract 베이스. 권위 State 쓰기(`CommitGimmickState`)·복제/SaveGame·GimmickStateTree 구동을 소유. 자식은 컴포넌트·인터랙션 핸들러만 제공 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 StateTree 가 공유하는 태스크/조건 struct 모음(`Component Move`, `Play Level Sequence`, `Spawn Actor`, `Wx Gimmick State Is` 등). 초기 진입/라이브 전이를 `SourceStateID` 로 구분 | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionScannerComponent` | PlayerController 에 붙어 소유 클라에서 `IWxInteractable` 구현 액터를 주기 순회해 활성 영역을 모으고 선택·하이라이트하며 `ServerInteract` 로 전송 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 를 스폰하는 배치 액터. 처치/부활 상태(`bIsKilled`)를 GUID 키로 SaveGame 보존 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 계약(`OnSpawnedBy` 훅, 에디터 미리보기 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | 월드의 Auto 모드 스포너 일괄 리스폰 BP 진입점(`TryRespawnAll`) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 프로젝트 설정 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
- 이 모듈은 Native Tag 를 선언하지 않는다. 기믹 State 태그(`Gimmick.Door.*`, `Gimmick.Elevator.*`, `Gimmick.TreasureChest.*`, `Gimmick.CutsceneTrigger.*`, `Gimmick.AlarmConsole.*`, `Gimmick.SpawnConsole.*` 등)는 `WxCore` 의 `WxGameplayTags` 에 정의되어 있고, 각 기믹 생성자에서 초기 State 로 대입한다.

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 을 상속(Abstract), 생성자에서 컴포넌트 구성 + 영역 메시를 `ActiveInteractionMeshes` 에 담기 + 초기 `State` 태그 대입, `OnInteracted` override 에서 `SetInteractingCharacter` → `CommitGimmickState(NextState)` 로만 State 를 확정(항상 서버 권위). 자식은 State 를 직접 쓰지 않는다. 상호작용 영역이 여럿이면 `GetDefaultInteractionPrompt` 도 `OnInteracted` 와 같은 `Source` 분기로 override 해 영역별 프롬프트를 고른다(문구 자체는 BP 디폴트에서 author). 기존 자식(`AWxDoor`/`AWxElevator`/`AWxTreasureChest`/`AWxCutsceneTrigger`/`AWxAlarmConsole`/`AWxSpawnConsole`/`AWxCheckPoint`)이 참고 패턴.
- **데이터 주도 연출**: 비주얼·FX·상호작용 토글은 C++ 가 아니라 각 기믹 BP 에 할당한 ST 에셋(`ST_Door` 등)이 `WxGimmickStateTreeNodes` 의 노드를 조립해 author 한다. Root 에 재선택 전이 하나(`On Event: Gimmick → GotoState: Root`)만 두고, 각 상태의 `Required Event to Enter` = 그 State 태그. 기본(resting) 상태는 Required Event 없이 Root 자식 중 **마지막**에 둔다.
- **새 ST 노드 추가**: `WxGimmickStateTreeNodes.h` 에 `FStateTreeTaskCommonBase`/`FStateTreeConditionCommonBase` 파생 struct + InstanceData struct 로 추가. State 를 읽지 않는 순수 비주얼로 두면 어떤 기믹이든 재사용된다. 초기 진입/라이브 전이는 `Transition.SourceStateID` 유효성으로 구분.
- **새 스폰 대상**: `IWxSpawnableInterface` 를 구현하면 `AWxSpawner::SpawnableActorClass`(MustImplement 필터)에 지정 가능.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 전 기믹이 공유하는 「권위 State → 복제/OnRep → ST 이벤트」 패턴과 WxSave 통합의 정본. 헤더 주석에 배선 규약까지 상세.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 재사용 노드 카탈로그. 각 노드 책임과 복원 처리 규약(초기 진입 스냅·스킵)이 여기 모여 있다.
3. `Source/WxWorld/Public/Gimmick/WxDoor.h` (또는 `WxElevator.h`) — 위 두 패턴이 실제 기믹에서 어떻게 얇게 조립되는지 보여주는 최소 예.
4. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 스캔·선택·서버 전송의 로컬리티 설계.

## 관련
- 상위: [[WxCore]](인터페이스·태그·`AWxCharacter`)
- 연계: [[WxSave]](복원 오케스트레이션) · [[WxInventory]](보물상자 보상) · [[WxCombat]](상호작용 어빌리티)

---
*문서 기준 커밋 `c275320` · 생성일 2026-07-24 · 소스 27파일 — `/readme-writer`로 갱신*
