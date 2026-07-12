# WxWorld — 월드 오브젝트 · 상호작용 시스템

> 레벨에 배치되는 상호작용 기믹(문·엘리베이터·상자·콘솔·컷신)과 스폰 액터, 그리고 플레이어가 대상을 고르는 상호작용 파이프라인을 담는 도메인 모듈. 기믹의 상태 전이는 서버 권위 `State` 태그가 원천이고, 비주얼/연출은 공용 StateTree 노드가 그 태그 이벤트로 진입해 처리한다.

## 책임
**담당**
- 기믹 추상 베이스(`AWxGimmick`)와 파생 6종(Door/Elevator/CutsceneTrigger/TreasureChest/SpawnConsole/AlarmConsole): 권위 `State` 태그 소유·복제·SaveGame, 단일 쓰기 진입점 `CommitGimmickState`, StateTree 구동·복원 진입.
- 전 기믹 공용 StateTree 노드 라이브러리(`WxGimmickStateTreeNodes`): 컴포넌트 이동/스플라인/애니/시퀀스/사운드/Niagara/스폰/인터랙션·입력 토글 등 `State` 무관 재사용 태스크·조건.
- 상호작용 컴포넌트(`UWxInteractionComponent`)와 로컬 플레이어별 수집·선택·강조 조율(`UWxInteractionRegistrySubsystem`).
- 스폰 시스템: 레벨 배치 `AWxSpawner`(처치/부활 상태 GUID SaveGame 보존), 스폰 대상 규약 `IWxSpawnableInterface`, 일괄 리스폰 `UWxSpawnerLibrary`.

**경계 (비담당)**
- 플레이어 측 상호작용 스캔·입력 어빌리티·서버 타겟 전달 → [[WxCombat]] (레지스트리는 스캐너가 push 한 로컬 후보 목록만 소유).
- 상호작용 프롬프트 HUD 리스트·뷰모델 표시 → [[WxUI]].
- 세이브 슬롯 직렬화/복원 오케스트레이션 → [[WxSave]] (`IWxSavable` 계약 자체는 WxCore 정의).
- 보상 지급 태스크(`Wx Grant Reward`)·보상 테이블 타입(`WxRewardTableRow`) → [[WxInventory]] (TreasureChest 는 데이터 핸들만 보유).

## 의존성
- **주요 의존**: [[WxCore]] (공용 `WxGameplayTags`·`WxInteractionSource`·`WxSavable`·`WxCollisionChannels`). 엔진: StateTree/GameplayStateTree(기믹 상태머신), Niagara(FX), GameplayAbilities, LevelSequence/MovieScene(컷신), DeveloperSettings.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs·uplugin 상 Wx 모듈은 `WxCore` 뿐. WxInventory 는 `WxTreasureChest.h` 의 `RowType="/Script/WxInventory.WxRewardTableRow"` 메타 문자열로만 참조되며 컴파일/링크 의존이 아니다.)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 기믹 추상 베이스. `State` 태그 소유·복제·SaveGame, `CommitGimmickState`(권위 쓰기)/`OnRep_GimmickState` 로 StateTree 진입 구동. 파생 6종의 부모 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 공용 ST 태스크/조건(ComponentMove·SplineMove·PlayAnimation·PlayLevelSequence·PlaySound·SpawnNiagara·TriggerSpawners·SpawnActor·EnableInteraction·EnablePlayerInput·GimmickStateIs). `State` 무관 순수 비주얼/사이드이펙트 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 쿼리 볼륨을 재사용하는 상호작용 SceneComponent. `IWxInteractionSource` 구현, `TryInteract`(서버 권위) → `OnInteracted` fire, 외곽선 강조 토글 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer 서브시스템. 인-레인지 컴포넌트 수집·선택·강조 조율(로컬 표시 전용). 스캐너가 `UpdateInRange` 로 push | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. 처치 상태(`bIsKilled`) GUID 보존, `Respawn`/`MarkKilled`, `IWxSavable` | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 규약. `OnSpawnedBy`(빙의 전 per-instance 훅), 에디터 미리보기 메시 추출. Spawner 의 `MustImplement` | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | BP 진입점. 월드의 Auto 모드 Spawner 일괄 리스폰(`TryRespawnAll`, 서버 권위) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | Config 설정. 스포너 클래스별 에디터 미리보기 아이콘 매핑 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`(Abstract) 상속 → 생성자에서 컴포넌트를 `SceneRoot` 에 부착하고 기본 `State` 태그 지정 → `UWxInteractionComponent::OnInteracted` 에 `Handle...` 핸들러를 바인딩해 그 안에서 `CommitGimmickState(NewTag)` 만 호출(State 쓰기는 서버 권위·C++ 전용). 비주얼/연출은 자식 BP 에 할당한 ST 에셋이 공용 노드로 author. ST 바인딩 대상 컴포넌트는 `AllowPrivateAccess` 로 노출.
- **상태 구동 패턴**: 복제 `State` 태그 = 각 상태의 *Required Event to Enter*. State 가 바뀌면 권위(`CommitGimmickState`)·클라(`OnRep_GimmickState`) 양쪽이 태그를 ST 이벤트로 발행해 서버/클라가 같은 상태로 진입. 복원 진입은 `Gimmick.Restore` 마커를 함께 보내 1회성 노드(사운드/FX/스폰)가 라이브 발동과 구분해 스냅/스킵.
- **비주얼은 노드 재사용**: `WxGimmickStateTreeNodes` 태스크는 `State` 를 읽지 않는 범용 노드다. 이동/애니/FX/스폰/토글이 필요하면 새 C++ 대신 ST 에셋에서 상태별로 조합한다. 머무는(완료 전이 없는) 태스크는 상태 완료 판정에서 제외해 루트 재선택 thrash 를 피한다.
- **새 스폰 대상**: `IWxSpawnableInterface` 구현(미리보기 메시 제공, `OnSpawnedBy` per-instance 컨텍스트) → `AWxSpawner.SpawnableActorClass` 지정. 콘솔 트리거 대상 Spawner 는 `Manual` 모드로 두어 자동 스폰을 막는다.
- **다중 상호작용 영역**: 한 액터에 `UWxInteractionComponent` 를 영역 수만큼 추가하고 각기 다른 쿼리 볼륨(프리미티브)에 부착.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 「권위 State 태그 → StateTree 진입」 패턴의 중심. 이걸 이해하면 파생 6종이 전부 읽힌다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹 비주얼/연출이 실제로 어디서 일어나는지. 초기진입 vs 라이브전이 구분·복원 스냅 규약이 핵심.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 3단계 흐름(스캔 → 서버 TryInteract → OnInteracted)과 볼륨/강조/레지스트리 경계.

## 관련
- 상위: [[WxGame]]
- 위임: [[WxCombat]] · [[WxUI]] · [[WxSave]] · [[WxInventory]]
---
*문서 기준 커밋 `d0c804a` · 생성일 2026-07-12 · 소스 29파일 — `/readme-writer`로 갱신*
