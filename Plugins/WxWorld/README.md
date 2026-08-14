# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치된 오브젝트를 살아 움직이게 하는 도메인. StateTree로 구동되는 기믹(문·엘리베이터·체크포인트 등), 플레이어의 근접 상호작용 감지·선택, 적/오브젝트 스포너와 그 처치·부활 영속을 책임진다.

## 책임
**담당**
- 기믹 구동: `UWxGimmickStateTreeComponent` 하나로 어떤 액터든(순수 BP 포함) StateTree 상태머신 + 상호작용 + 상태 영속을 갖춘 기믹이 된다. 상태는 서버 권위, 클라는 복제된 상태 태그를 추종.
- 상호작용 감지: `UWxInteractionScannerComponent`가 소유 클라에서 주변 상호작용 메시를 주기 스캔, in-range 목록·선택·하이라이트를 관리하고 선택을 서버로 원자 전송.
- 스폰·영속: `AWxSpawner`가 레벨 배치 스폰과 처치 상태(`bIsKilled`)를 GUID 키로 WxSave 슬롯에 보존, 부활/영구사망 정책 관리.
- 월드 연출 StateTree 태스크군: 컴포넌트 이동/스플라인 이동, 애니메이션·몽타주·LevelSequence·사운드·Niagara 재생, 플레이어 입력 토글, 상호작용 대상에 GameplayEffect 적용 등.

**경계 (비담당)**
- 상호작용의 실제 권위 실행(사거리·활성 검증, 대상 인터페이스 호출)은 스캐너가 송출한 `Event.Interact`를 받는 [[WxCombat]]의 어빌리티(WxAbility_Interact) 몫. 스캐너는 감지·선택만 한다.
- 세이브 슬롯 자체의 직렬화·복원 오케스트레이션은 [[WxSave]]. 이 모듈은 `IWxSavable` 계약을 구현하기만 한다.
- HUD 프롬프트 목록 표시는 [[WxUI]]의 뷰모델(UWxViewModel_InteractionList)이 스캐너 델리게이트를 구독해 처리.
- 기믹/스포너의 레벨 배치·주입 설정·구체 콘텐츠는 Experience 에셋과 GameFeature 콘텐츠 플러그인 몫.

## 의존성
- **주요 의존**: `WxCore`(`IWxSavable`·`IWxInteractable` 계약, `WxSavable.h`/`WxInteractable.h`). 엔진: StateTree·GameplayStateTree(기믹 구동), GameplayAbilities(상호작용 이벤트/GE 적용), Niagara·LevelSequence·MovieScene(연출 태스크), ModularGameplay(스캐너의 ControllerComponent 주입), UniversalObjectLocator(로케이터 대상 지정), AIModule(StateTreeComponent가 BrainComponent 파생).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 모듈의 심장 — Gimmick 축. 상태머신·상호작용·영속 3책임을 한 컴포넌트에 담는다 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | Interaction 축. PC에 붙는 로컬 스캐너, 서버 상호작용 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | Spawnable 축. 레벨 배치 스포너 + 처치/부활 영속 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 인스턴스가 스포너 컨텍스트를 끌어가는 훅(`OnSpawnedBy`) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | System 축. `TryRespawnAll` 등 서버 권위 BP 라이브러리 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 등 프로젝트 설정 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |
| `FWxStateTreeTask_EnableInteraction` | 상호작용 영역/대상 토글 — 태스크군의 대표 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h` |

## 확장 포인트 / 규약
- **새 기믹 연출 태스크**: `FStateTreeTaskCommonBase`를 상속한 `USTRUCT`를 만들고, InstanceData `USTRUCT` + `using FInstanceDataType` + `GetInstanceDataType()`/`EnterState`(·`Tick`) override로 구현한다. `Gimmick/`(연출)·`Interaction/`(상호작용)·`Spawnable/`(스폰) 폴더 축에 맞춰 배치. `GetInstanceDataType()`의 헤더 인라인 정의는 코딩 규칙 6의 명시적 예외(엔진 관례).
- **새 상호작용 대상**: `UWxGimmickStateTreeComponent`를 붙이면 전용 액터 없이 기믹화된다. 그 밖의 커스텀 대상은 WxCore의 `IWxInteractable`을 구현해 스캐너 스캔·프롬프트·활성 검증에 참여한다.
- **새 스폰 대상**: 스폰될 액터가 `IWxSpawnable::OnSpawnedBy`를 구현하면 Deferred Spawn의 FinishSpawning 이전에 스포너 컨텍스트를 받는다. `AWxSpawner::SpawnableActorClass`에 지정(`MustImplement`).
- **데이터 주도**: 기믹 거동은 StateTree 에셋이 전이·태스크로 정의하며, 상태 디테일의 **Tag 필드**가 곧 세이브 키(에셋 내 유일 필요). 스포너 아이콘은 `UWxWorldDeveloperSettings::SpawnerClassIcons`(TMap) 설정으로 구동.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 모듈의 심장. 상태 서버권위/클라추종 패턴, 상호작용 영역, 세이브 키 규약이 클래스 주석에 응축돼 있다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 입력→선택→ServerInteract→WxCombat 어빌리티로 이어지는 제어 흐름 전체.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/부활과 World Partition 셀 리로드를 견디는 GUID 세이브 키 처리.

## 관련
- 상위: Experience 에셋이 `UWxInteractionScannerComponent`를 PlayerController에 주입, [[WxCombat]]의 WxAbility_Interact가 실제 상호작용을 권위 실행, [[WxUI]] 뷰모델이 스캐너 목록을 표시, [[WxSave]]가 기믹·스포너의 `IWxSavable` 상태를 슬롯에 보존. 기믹/스포너 배치는 GameFeature 콘텐츠 플러그인 몫.

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 46파일 — `/readme-writer`로 갱신*
