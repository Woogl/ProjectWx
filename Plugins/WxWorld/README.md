# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 월드 오브젝트(문·체크포인트·엘리베이터 같은 기믹, 적 스포너)와 플레이어의 상호작용을 담당하는 도메인 플러그인. 기믹은 StateTree로 상태를 구동하고, 상호작용은 스캐너 → 어빌리티 권위 검증 경로로 처리한다.

## 책임
**담당**
- **기믹(Gimmick)**: `UWxGimmickStateTreeComponent` 하나로 어떤 액터든(순수 BP 포함) StateTree 구동·상호작용·상태 영속을 가진 기믹으로 만든다. 상태는 서버 권위이며 복제된 상태 Tag를 클라가 추종한다.
- **상호작용(Interaction)**: `UWxInteractionScannerComponent`가 소유 클라에서 주변 상호작용 액터를 주기 스캔·선택·하이라이트하고, 선택을 서버로 보내 어빌리티가 권위 검증 후 대상 인터페이스를 호출한다. 대상은 액터 단위이며 액터 안의 특정 메시만 영역이 되는 개념은 없다.
- **스폰(Spawnable)**: `AWxSpawner`가 배치 액터로서 대상을 스폰하고 처치/부활 상태를 자체 보유하며 WxSave 슬롯으로 영속한다. `IWxSpawnable`은 스폰 직후 훅.
- **StateTree 태스크 스위트**: 기믹/퀘스트 ST 에셋이 조립해 쓰는 재생·이동·상호작용·스포너 태스크 노드 모음(아래 확장 포인트).

**경계 (비담당)**
- 상호작용의 권위 실행·사거리/차단 검증(`WxAbility_Interact`), GameplayEffect·GameplayEvent — [[WxCombat]] / GAS.
- 상호작용 HUD 리스트 표시·선택 뷰모델(`UWxViewModel_InteractionList`) — [[WxUI]].
- 저장 슬롯·복원 오케스트레이션(`IWxSavable` 계약의 소비자) — [[WxSave]].
- 상호작용 계약·저장 인터페이스·`WxGameplayTags` 정의 — [[WxCore]].

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존 — `IWxInteractable`·`IWxSavable`·`WxGameplayTags`). 엔진 서브시스템: `StateTree`/`GameplayStateTree`(기믹 상태머신·태스크), `GameplayAbilities`(상호작용 어빌리티 연동), `Niagara`·`LevelSequence`/`MovieScene`(연출 태스크), `ModularGameplay`·`AIModule`(`UStateTreeComponent`=BrainComponent 기반), `UniversalObjectLocator`(레벨 밖 호스트에서 배치 액터 지정), `DeveloperSettings`. 에디터에서만 `UnrealEd`(스포너 라벨·프리뷰).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxGimmickStateTreeComponent` | 기믹의 상태머신·상호작용·영속을 한 몸에 담는 StateTree 컴포넌트 (`IWxInteractable`+`IWxSavable`) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 플레이어 컨트롤러에 붙는 주변 상호작용 스캐너·선택·서버 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 스폰 대상 인스턴스를 스폰하고 처치/부활 상태를 영속하는 배치 액터 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후(빙의 전) per-instance 컨텍스트 주입 훅 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` 등 스포너 일괄 조작 BP 라이브러리 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 설정 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
Native Tag 선언은 이 모듈에 없다. 상호작용 경로는 `WxCore`가 정의한 `WxGameplayTags::Event_Interact`·`Ability_Interact`를 소비만 한다(스캐너가 어빌리티를 애셋 태그로 조회·이벤트 송출). 기믹의 상태 식별은 별도 태그 네임스페이스가 아니라 ST 에셋의 상태 디테일 Tag 필드(에셋 안에서 유일)를 저장 키로 쓴다.

## 확장 포인트 / 규약
- **새 기믹**: C++ 액터를 만들지 않는다. 아무 액터에 `UWxGimmickStateTreeComponent`를 붙이고 ST 에셋으로 전이·연출을 정의한다. 영속이 필요한 상태에는 상태 디테일에 Tag를 달아야 저장된다. 오너 액터의 `Replicates`가 켜져 있어야 상태 복제가 성립한다(꺼지면 BeginPlay가 Error 로그).
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 파생 `USTRUCT`로 만든다. 인스턴스 데이터를 짝 구조체로 두고 `using FInstanceDataType`·`GetInstanceDataType()`을 헤더에 표기(코딩 규칙 6의 유일 예외, 각 헤더 주석 참조). 태스크 분류: `Gimmick/`(연출·이동 — 재생/컴포넌트 이동/이펙트 적용/스포너 발동·리스폰), `Interaction/`(상호작용 켜기·대기), `Spawnable/`(로케이터 지정 스포너 발동·처치 대기).
- **레벨 밖 호스트에서 배치 액터 지정**: 퀘스트 ST 등에서 특정 배치 스포너/대상을 겨눌 땐 `FUniversalObjectLocator`(순수 구조체)를 쓴다 — ST 컴파일러의 레벨 액터 참조 검증을 우회하고 WP/PIE 해석이 엔진에 내장돼 있다.
- **장치(레버) 연결**: 상호작용 장치는 `AWxLeverDevice`를 배치하고 **레버 쪽** `Gimmicks` 배열(레벨 인스턴스 저작)로 움직일 기믹 액터를 지목한다 — 한 레버가 여럿을(1:N), 한 기믹이 여러 레버에(N:1) 걸린다. 눌리면 각 기믹 트리에 `Event.Interact`가 나가고, ST 에셋은 `On Event`로 받아 전이를 정한다. 레버는 상시 활성이며 상태를 들지 않는다 — 상태별로 잠가야 하면 '상호작용 켜기' 태스크의 Target(액터) 갈래로 여닫는다.
- **스포너 부활 정책**: `EWxSpawnerMode`(Auto/Manual)와 `bNeverRevive`(보스). 일괄 리스폰은 `UWxSpawnerLibrary::TryRespawnAll`(Auto만), 지정 트리거는 로케이터 태스크.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h` — 모듈의 심장. 클래스 doc-comment에 서버 권위 상태 구동·클라 추종·재진입·세이브 복원 진입의 전체 패턴이 정리돼 있다. cpp의 `PublishAuthorityState`/`FollowAuthorityState`/`StartTreeAtSavedState`가 그 구현.
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔 → 선택 → `ServerInteract` → 폰 ASC 이벤트 → 어빌리티 권위 검증으로 이어지는 상호작용 흐름 전체가 doc-comment에 있다.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 처치/부활·WxSave `SaveId`(에디터 저장 시 ActorGuid 확정) 계약.
4. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h` + `WaitForInteraction.h` — 기믹이 상호작용 영역을 여닫고 그 발행을 전이로 잇는 규약, 퀘스트 게이트로서의 상호작용 대기.

## 관련
- 상위: [[WxGame]] (Experience가 스캐너 컴포넌트 주입, GameFeature가 콘텐츠 배치)
- 인접: [[WxCore]] · [[WxCombat]] · [[WxSave]] · [[WxUI]] · [[WxQuest]]

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 46파일 — `/readme-writer`로 갱신*
