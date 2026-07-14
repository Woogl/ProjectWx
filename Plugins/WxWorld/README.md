# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문·엘리베이터·상자·콘솔·컷신 트리거 등 "기믹")와, 플레이어가 그것들을 조준·선택·발동하는 상호작용 파이프라인, 그리고 적/오브젝트를 배치·리스폰하는 스포너를 담당한다.

## 책임
**담당**
- 상호작용 파이프라인의 월드 측: 상호작용 볼륨 등록(`UWxInteractionComponent`)과 로컬 플레이어별 인-레인지/선택 관리(`UWxInteractionRegistrySubsystem`), 외곽선 강조 조율.
- 기믹 공통 뼈대: 서버 권위 State 태그(복제 + SaveGame), StateTree 구동, 전 기믹이 공유하는 StateTree 노드 라이브러리(이동/애니/사운드/Niagara/시퀀스/스폰 등).
- 구체 기믹 액터: 문, 엘리베이터, 보물상자, 경보/스폰 콘솔, 컷신 트리거.
- 레벨 배치 스포너(`AWxSpawner`)와 일괄 리스폰, 처치 상태의 세이브 보존.

**경계 (비담당)**
- 플레이어 측 스캐너(주변 볼륨 수집)와 상호작용 어빌리티·TargetData 전달은 어빌리티/전투 쪽([[WxCombat]])이 구동한다. 본 모듈은 볼륨을 등록하고 서버 `TryInteract` 진입점만 노출한다.
- 상호작용 프롬프트 UI(HUD 리스트/뷰모델)는 [[WxUI]]가 표시한다. 레지스트리는 텍스트/선택만 제공한다.
- Gameplay Tag 정의(`WxGameplayTags::Gimmick_*` 등)와 `IWxSavable`/`IWxInteractionSource` 인터페이스는 [[WxCore]]에 있다.
- 세이브 슬롯 직렬화·복원 구동은 [[WxSave]]가 담당한다. 본 모듈은 `IWxSavable` 구현과 `SaveGame` 필드만 제공한다.

## 의존성
- **주요 의존**: [[WxCore]] (Savable/InteractionSource 인터페이스, 공용 Gameplay Tag) · StateTree/GameplayStateTree (기믹 상태머신) · GameplayAbilities · Niagara · LevelSequence/MovieScene (컷신 재생) · DeveloperSettings.
- 규칙: WxCore 외 다른 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 추상 부모. State 태그·StateTree·세이브 공통 소유, `CommitGimmickState`가 유일한 서버 권위 쓰기 진입점 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `FWxStateTreeTask_*` / `FWxStateTreeCondition_GimmickStateIs` | 전 기믹이 공유하는 StateTree 노드 라이브러리(이동·애니·사운드·Niagara·시퀀스·스폰·인터랙션 토글) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 액터에 붙는 상호작용 영역. 볼륨 등록·`TryInteract`·`OnInteracted` 델리게이트·외곽선 강조 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 목록/선택 소유, HUD·강조 조율 | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. 처치 상태 보존·리스폰·`bNeverRevive`(보스) | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 훅(`OnSpawnedBy`, 에디터 미리보기 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` — 월드의 Auto 스포너 일괄 리스폰(BP 진입점) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑(프로젝트 설정) | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`을 상속한 `Abstract` C++ 클래스를 만들어 컴포넌트(메시/`UWxInteractionComponent`)를 들고, 생성자에서 초기 `State` 태그를 지정한다. 인터랙션 핸들러는 `CommitGimmickState`로만 State를 전이한다(항상 서버 권위). 비주얼·사이드이펙트는 C++에 두지 않고, 자식 BP에 할당한 StateTree 에셋이 State 태그를 `Required Event to Enter`로 받아 공유 노드로 처리한다. 기존 문/상자/콘솔 구현이 그 패턴의 최소 예시다.
- **리플리케이션/권한 모델**: State 쓰기는 무조건 서버 권위(단일 진입점 `CommitGimmickState`), 클라는 복제 State(`OnRep_GimmickState`)를 추종하며 서버·클라가 같은 ST 이벤트 로직을 공유한다. 상호작용도 `OnInteracted`는 서버에서만 fire된다.
- **초기 진입 vs 라이브 전이**: 모든 ST 노드가 `Transition.SourceStateID` 유효성으로 복원/시작/레이트조인(스냅·침묵)과 실제 발동(재생·스폰)을 구분한다. 세이브 복원 시엔 `Gimmick.Restore` 마커가 함께 발행되어 일회성 노드가 재실행되지 않는다.
- **새 스폰 대상**: `IWxSpawnableInterface`를 구현하면 `AWxSpawner`의 `SpawnableActorClass`에 지정 가능하다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 기믹의 State/권한/세이브/StateTree 구동 규약. 모듈의 중심 개념이 이 헤더 주석에 모여 있다.
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹이 실제로 "무엇을 하는지"는 전부 이 공유 노드들. 각 노드의 초기/라이브 구분과 완료 규약을 파악하면 기믹 저작 방식이 보인다.
3. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 흐름(스캐너→레지스트리→어빌리티→서버 TryInteract)의 3단계 주석이 모듈 경계를 설명한다.
4. `Source/WxWorld/Private/Gimmick/WxDoor.cpp` — 가장 단순한 구체 기믹. 생성자 초기 State + 인터랙션 핸들러 → `CommitGimmickState` 패턴의 정석.

## 관련
- 상위: 플레이어 상호작용 어빌리티·스캐너는 [[WxCombat]], 프롬프트 HUD는 [[WxUI]], 세이브 복원 구동은 [[WxSave]]가 담당하며 본 모듈과 맞물린다. 공용 인터페이스·태그는 [[WxCore]].

---
*문서 기준 커밋 `842f761` · 생성일 2026-07-14 · 소스 29파일 — `/readme-writer`로 갱신*
