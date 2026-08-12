# WxWorld — 월드 오브젝트 및 상호작용

> 레벨의 기믹(문·엘리베이터·트랩·체크포인트 등)과 플레이어 상호작용, 스포너를 담는 도메인 플러그인. 기믹은 StateTree 로 구동되고 상태는 서버 권위로 복제·영속된다.

## 책임
**담당**
- 기믹 구동: `UWxGimmickStateTreeComponent` 하나로 액터를 기믹화(StateTree 상태머신 + 상호작용 계약 + 상태 영속)
- 상호작용 감지·선택: 소유 클라에서 주변 상호작용 메시를 주기 스캔하고 선택·하이라이트, 서버로 실행 전달
- 기믹 저작 블록: 이동/애니메이션/사운드/스폰/GE 적용 등 StateTree Task 모음
- 스포너: 레벨 배치 `AWxSpawner` 의 스폰·처치·리스폰 및 영구 사망 처리

**경계 (비담당)**
- 상호작용 어빌리티의 권위 검증·실행(사거리·활성)은 [[WxCombat]] 계열 GAS 어빌리티가 담당 — 스캐너는 `Event.Interact` 만 송출
- 저장 슬롯 직렬화·복원 오케스트레이션은 [[WxSave]] (본 모듈은 `IWxSavable` 구현만 제공)
- HUD 리스트 표시·입력 바인딩은 [[WxUI]] 뷰모델/위젯
- 공용 인터페이스 정의(`IWxInteractable`, `IWxSavable`)는 [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]] (유일한 Wx 의존). 엔진: StateTree/GameplayStateTree(기믹 구동), GameplayAbilities(상호작용 이벤트·GE Task), ModularGameplay(컴포넌트 주입), Niagara·LevelSequence(연출 Task), UniversalObjectLocator(스포너 지목)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 기믹의 상태머신·상호작용(`IWxInteractable`)·영속(`IWxSavable`)을 한 몸에 담는 StateTree 컴포넌트. 붙이면 어떤 액터든 기믹이 됨 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController 부착, 소유 클라에서 주변 상호작용 메시 스캔·선택·하이라이트 후 `ServerInteract` 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 레벨 배치 스포너. 처치 상태(`bIsKilled`) 보유, 리스폰·영구 사망(`bNeverRevive`) 처리, `IWxSavable` | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스포너가 스폰 직후(빙의 전) 인스턴스에 컨텍스트를 주입하는 훅 인터페이스 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | BP 진입점. 월드의 Auto 모드 스포너 일괄 리스폰(`TryRespawnAll`) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `FWxStateTreeTask_*` (다수) | 기믹 저작용 StateTree Task 모음(이동·스플라인·애니·몽타주·사운드·시퀀스·스폰·나이아가라·GE·상호작용 토글·스포너 트리거) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/`, `.../Spawnable/` |

## 확장 포인트 / 규약
- **새 기믹**: 전용 C++ 액터 불필요 — 액터에 `UWxGimmickStateTreeComponent` 를 붙이고 StateTree 에셋으로 전이를 저작한다. 오너 액터의 `Replicates` 를 켜야 상태 복제·영속이 성립(꺼지면 BeginPlay Error 로그).
- **상태 영속**: 영속이 필요한 ST 상태에 엔진 순정 상태 Tag 를 달면 그 값이 곧 저장 키(`StateTag`, SaveGame). 태그 없는 상태는 저장되지 않는다.
- **새 Task**: `FStateTreeTaskCommonBase` 를 상속하고 `FWx...InstanceData` 를 짝으로 둔다(파일당 struct 쌍). `GetInstanceDataType()` 의 헤더 정의는 코딩 규칙 6의 명시적 예외.
- **상호작용 켜기**: `WxStateTreeTask_EnableInteraction` 로 상태 진입 시 대상 메시·프롬프트를 토글한다(가용성+문구를 한 자리에서 author). 목적지가 갈리는 상태는 전이에 이벤트 페이로드 `Source` 비교 조건을 단다.
- **새 스폰 대상**: `IWxSpawnable` 을 구현한 액터 클래스를 `AWxSpawner.SpawnableActorClass` 에 지정(에디터 `MustImplement` 강제).
- **스포너 아이콘**: `UWxWorldDeveloperSettings.SpawnerClassIcons` 로 클래스별 에디터 스프라이트 매핑.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 모듈의 심장. 상태 서버 권위·복제 추종·재진입·저장 키 전략이 헤더 주석에 정리됨
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용의 로컬 감지 → `ServerInteract` → `Event.Interact` 흐름과 로컬리티 설계
3. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_EnableInteraction.h` — Task 저작 패턴과 상호작용/전이 규약의 대표 예
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰·처치·리스폰·영구 사망과 GUID 기반 저장 키

## 관련
- 상위: [[WxCore]] (인터페이스 정의) · 상호작용 실행은 [[WxCombat]] GAS 어빌리티 · 영속은 [[WxSave]] · 표시는 [[WxUI]]

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 46파일 — `/readme-writer`로 갱신*
