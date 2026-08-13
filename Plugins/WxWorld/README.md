# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓인 상호작용 가능한 기믹(문·엘리베이터·체크포인트 등)과 적 스포너를 담당하는 도메인 플러그인. 상태머신은 StateTree로, 상호작용은 IWxInteractable 계약과 어빌리티로, 영속은 IWxSavable로 구동한다.

## 책임
**담당**
- 기믹 상태머신: `UWxGimmickStateTreeComponent`를 붙인 어떤 액터든(순수 BP 포함) StateTree 기반 기믹으로 만든다. 상태·상호작용·영속을 한 컴포넌트에 담고, 상태는 서버 권위·클라 추종으로 복제한다.
- 상호작용 감지/실행: 소유 클라의 `UWxInteractionScannerComponent`가 주변 활성 메시를 스캔·하이라이트하고, 선택을 서버로 보내 상호작용 어빌리티가 권위에서 검증·실행하게 한다.
- 스포너: `AWxSpawner`가 `IWxSpawnable` 대상을 스폰·처치·리스폰하며 처치 상태를 WxSave 슬롯에 보존한다.
- 월드용 StateTree Task 라이브러리: 연출·상호작용 게이트·스포너 제어 태스크를 ST 에셋이 조립해 쓰도록 제공한다.

**경계 (비담당)**
- 상호작용 어빌리티(WxAbility_Interact)의 실제 정의와 ASC — [[WxCombat]] / GameplayAbilities. 본 모듈은 `Event.Interact` 송출과 인터페이스 호출만 안다.
- HUD 프롬프트·선택 리스트 표시 — [[WxUI]]. 스캐너는 델리게이트로 목록/선택만 내보낸다.
- 세이브 슬롯의 직렬화·복원 오케스트레이션 — [[WxSave]]. 본 모듈은 `IWxSavable` 계약만 구현한다.
- 상호작용·저장 인터페이스(`IWxInteractable`, `IWxSavable`)의 정의 자체 — [[WxCore]].

## 의존성
- **주요 의존**: `WxCore`(IWxInteractable·IWxSavable 계약). 엔진: StateTree / GameplayStateTree(기믹 상태머신·태스크), Niagara(스폰 이펙트), GameplayAbilities(상호작용 이벤트), UniversalObjectLocator(레벨 액터 지정), LevelSequence·MovieScene(시퀀스 재생), ModularGameplay·AIModule(컴포넌트 주입·BrainComponent 파생).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Wx 의존은 `WxCore` 단독)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 붙이면 액터가 기믹이 되는 상태머신·상호작용·영속 컴포넌트 | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라의 주변 스캔·하이라이트·서버 전송(PC에 부착) | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 처치/리스폰 상태를 가진 레벨 배치 스포너 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후 컨텍스트를 끌어가는 스폰 대상 계약 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | 월드 일괄 리스폰 BP 진입점 + 에디터 로케이터 표시 헬퍼 | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |
| ST Task — 연출/액션 | 컴포넌트/스플라인 이동·사운드·나이아가라·애님·몽타주·시퀀스·GE·입력토글 (`FWxStateTreeTask_*`) | `Source/WxWorld/Public/Gimmick/` |
| ST Task — 상호작용/스포너 게이트 | 상호작용 대기·활성화, 스포너 트리거·처치 대기 (`FWxStateTreeTask_*`) | `Source/WxWorld/Public/{Interaction,Spawnable}/` |

## 확장 포인트 / 규약
- **새 기믹**: 전용 C++ 액터를 만들지 말고 액터에 `UWxGimmickStateTreeComponent`와 ST 에셋을 붙인다. 상태는 상태 디테일의 Tag 필드로 식별하며(에셋 내 유일), 그 Tag가 곧 세이브 키다 — Tag 없는 상태는 저장되지 않는다. 상태 복제는 서버 권위, 클라는 복제된 StateTag를 추종한다(오너 액터의 `Replicates`가 켜져 있어야 한다).
- **새 월드 ST Task**: `FStateTreeTaskCommonBase`를 상속하고 `FWxStateTreeTask_*` 명명을 따른다. 인스턴스 데이터는 별도 `USTRUCT`로 분리하고 `using FInstanceDataType`로 연결한다. 레벨 액터 지정은 `TObjectPtr` 바인딩(같은 액터의 컴포넌트) 또는 `FUniversalObjectLocator`(임의 배치 액터 — 퀘스트 등 레벨 밖 호스트 ST에서 조립 가능)로 한다.
- **대기형 Task는 폴링하지 않는다**: 진입 시 자신을 대기 목록에 등록하고, 사건(상호작용 성립·스포너 처치)이 나는 순간 정적 통보(`NotifyInteracted`/`NotifySpawnerKilled`)로 완료된다. 새 게이트도 이 패턴을 따른다.
- **새 스폰 대상**: 대상 액터가 `IWxSpawnable`을 구현하면 `AWxSpawner`의 `SpawnableActorClass`로 지정할 수 있다(`OnSpawnedBy`에서 per-instance 컨텍스트 수령).
- **데이터 주도 부착**: 컴포넌트는 코드가 아니라 GameMode가 고른 Experience 에셋의 주입 설정으로 붙는다(ModularGameplay).

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 모듈의 심장. 상태 권위/복제/추종·상호작용 계약·영속이 한 헤더에 정리돼 있다.
2. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용이 감지→선택→서버→어빌리티로 흐르는 전체 경로 설명.
3. `Source/WxWorld/Public/Spawnable/WxSpawner.h` — 처치/리스폰/영구사망 상태 규칙.
4. `Source/WxWorld/Public/Gimmick/WxStateTreeTask_ComponentMove.h` — 전형적인 연출 태스크 한 예(새 태스크 작성 시 참고 틀).

## 관련
- 상위: GameFeature 콘텐츠 플러그인 및 [[WxGame]]이 Experience로 기믹·스포너를 배치한다. 상호작용 실행은 [[WxCombat]] 어빌리티, HUD는 [[WxUI]], 영속은 [[WxSave]]와 협력한다.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 46파일 — `/readme-writer`로 갱신*
