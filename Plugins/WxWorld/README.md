# WxWorld — 월드 오브젝트 및 상호작용

> 플레이어가 월드에서 다가가 상호작용하는 배치 오브젝트(문/엘리베이터/상자/콘솔/컷신 트리거)와, 적·오브젝트를 배치·리스폰하는 스포너를 담당한다. 상호작용 감지·HUD 후보 수집과 기믹 상태머신(StateTree) 구동의 공통 토대를 제공한다.

## 책임
**담당**
- 상호작용 감지/조율: 오버랩 기반 인터랙션 영역(`UWxInteractionComponent`), 로컬 플레이어별 후보 레지스트리와 선택/외곽선 강조(`UWxInteractionRegistrySubsystem`)
- 상호작용 가능한 월드 기믹: 권위 State enum을 소유하고 StateTree가 그 State를 추종해 비주얼/FX/인터랙션 토글을 적용하는 공통 패턴(`AWxGimmick` 및 Door/Elevator/TreasureChest/AlarmConsole/SpawnConsole/CutsceneTrigger)
- 기믹용 공통 StateTree 태스크 모음(이동/스플라인 이동/애니/FX/스포너 트리거/인터랙션 토글)
- 스폰: 레벨 배치 스포너의 스폰·처치(`bIsKilled`)·리스폰과 영구사망(보스), 월드 단위 스포너 레지스트리와 일괄 리스폰(`AWxSpawner`/`UWxSpawnerSubsystem`)

**경계 (비담당)**
- 상호작용 입력 어빌리티(`WxAbility_Interact`)와 프롬프트 위젯(`WBP_InteractionList`)/뷰모델은 여기 없다 — [[WxCombat]]/[[WxUI]] 쪽. 본 모듈은 후보 목록·선택만 노출한다.
- 상호작용/세이브의 공용 인터페이스(`IWxInteractionSource`, `IWxSavable`)는 [[WxCore]]에 정의된다. 본 모듈은 구현만 한다.
- 상자 보상 지급(`WxRewardComponent`)은 [[WxInventory]] 소유라 C++가 아니라 상속 BP에서 추가한다(플러그인 간 참조 금지 회피).
- 세이브 슬롯 읽기/쓰기 자체는 [[WxSave]]. 본 모듈은 `UPROPERTY(SaveGame)` State와 안정적 `WxSaveId`만 제공한다.

## 의존성
- **주요 의존**: [[WxCore]](유일한 Wx 의존; `IWxSavable`/`IWxInteractionSource`) · StateTree / GameplayStateTree(기믹 상태머신) · GameplayAbilities · Niagara(기믹 FX) · LevelSequence/MovieScene(컷신) · DeveloperSettings
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (WxInventory/WxUI/WxCombat 등은 BP·인터페이스 경유로만 협력)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInteractionComponent` | 오버랩 감지 + 레지스트리 등록 + 상호작용 Multicast를 담는 SphereComponent. 기믹이 영역마다 단다 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer별 인-레인지 후보 수집·선택 순환·강조 조율. HUD가 읽는 목록 소스 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxGimmick` | 모든 상호작용 월드 오브젝트의 추상 부모. SceneRoot + GimmickStateTree + 인터랙션 일괄 토글 + IWxSavable 제공 | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `FWxStateTreeTask_*` | 기믹 StateTree 공통 태스크(ComponentMove/SplineMove/PlaySkeletalAnim/PlayFx/TriggerSpawners/GimmickInteraction) | `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `AWxSpawner` | SpawnableActorClass를 스폰하는 배치 액터. 처치 상태(SaveGame)·리스폰·영구사망 보유 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 훅(`OnSpawnedBy`, 에디터 미리보기 메시) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerSubsystem` | 월드 내 스포너 레지스트리. 역조회 처치 마킹·Auto 일괄 리스폰 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `UWxSpawnerLibrary` | 서브시스템으로 위임하는 BP 진입점(thin wrapper) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`을 상속해 자체 State enum을 `Replicated, SaveGame`으로 소유한다. 권위 측이 인터랙션 시 최종 State를 확정·복제하고, 자식 BP에 할당한 StateTree 에셋이 Enum Compare 전이로 그 State를 추종해 비주얼/FX/인터랙션 토글을 전부 적용한다(서버/클라 동일, 이벤트 태그 없음). `StartLogic`은 컴포넌트 바인딩 순서를 위해 각 자식 `BeginPlay` 끝에서 호출한다.
- **새 기믹 동작**: `WxGimmickStateTreeNodes.h`의 공통 태스크를 ST 에셋에서 Context 액터의 컴포넌트/프로퍼티에 바인딩해 재사용한다. 태스크는 State를 읽지 않는 순수 비주얼이며, 초기 진입(시작/복원/레이트조인) vs 라이브 전이를 `Transition.SourceStateID` 유효성으로 구분한다(복원 시 FX/스폰은 침묵).
- **새 스폰 대상**: `IWxSpawnableInterface`를 구현하고 `AWxSpawner::SpawnableActorClass`에 지정(`MustImplement` 강제). `Manual` 모드 스포너는 콘솔 등 외부 트리거 전용, `bNeverRevive`로 보스 영구사망.
- **세이브**: 영속 키 `WxSaveId`는 에디터에서 1회 부여되어 에셋에 직렬화된다(런타임/세션 간 불변). `SaveGame` State 필드가 슬롯에 기록되고, 복원은 StateTree 폴링으로 자동 추종해 별도 후크가 없다.
- **권한 모델**: State 확정·스폰·`TriggerSpawners`는 서버 권위. 인터랙션 감지/선택/강조/FX는 로컬, 상호작용 알림은 `MulticastInteracted`로 전 피어 fire.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` — 기믹 공통 패턴(권위 State ↔ StateTree 추종)의 설계 의도가 클래스 주석에 정리돼 있다. 모든 기믹의 출발점.
2. `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxDoor.h` — 위 패턴이 가장 단순하게 구현된 구체 예. 다른 기믹은 변주.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 오버랩→레지스트리→어빌리티→Multicast로 이어지는 상호작용 흐름의 전 단계가 주석에 그려져 있다.
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/리스폰/영구사망 상태 모델.

## 관련
- 상위: 기믹/스포너는 레벨에 배치돼 [[WxGame]] 플레이 루프에서 동작하며, 상호작용은 [[WxCombat]]의 입력 어빌리티와 [[WxUI]]의 프롬프트 HUD가, 보상은 [[WxInventory]]가, 영속화는 [[WxSave]]가 함께 본다. 공용 인터페이스는 [[WxCore]].

---
*문서 기준 커밋 `ecc8da7` · 생성일 2026-06-19 · 소스 30파일 — `/readme-writer`로 갱신*
