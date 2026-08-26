# WxAI — 코드 리뷰

> 모듈이 매우 깨끗하다 — 29파일 전부 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix를 지키고, `BlueprintCallable`·`FORCEINLINE`·람다가 하나도 없으며, 지난 리뷰의 🔴/🟡 두 건(억제 해제 시 청각·촉각 자극 재획득, Patrol 의 이동 목표 키 이원화)이 모두 실제로 고쳐졌다. 이번 리뷰는 소스 29개(cpp 14 + h 15)를 전부 열고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite 의 cpp 를 깊게 봤으며, 엔진 소스(`C:\Program Files\Epic Games\UE_5.8`)와 대조해 지난 지적의 수정 여부와 새 발견을 라인 단위로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 타겟이 없을 때 `TargetDistance` 를 Clear 하면 "거리 0" 이 기록되어 근거리 게이팅이 무조건 통과한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp:36`, 구현은 `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:90-94`, 근거 주석은 `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:38`
- **범주**: 버그/정확성
- **문제**: 헤더 주석은 "Float 키는 값 없음을 Set 으로 표현할 수 없어 Clear 를 별도로 둔다" 고 설명하지만, 엔진의 Clear 역시 값 없음을 표현하지 못한다 — `UBlackboardComponent::ClearValue` 는 `UBlackboardKeyType::Clear` 로 메모리를 0 으로 밀 뿐이고(`Engine/.../BlackboardKeyType.cpp:128-131`), Float 키에는 Clear 전용 오버라이드가 없다. 즉 타겟이 사라지면 `TargetDistance` 에 **0.0** 이 들어간다.
  Float 키의 `SupportedOp` 은 `Arithmetic` 하나뿐이라(`BlackboardKeyType_Float.cpp:12`) BT 에서 이 키를 읽는 방법은 `Less/LessOrEqual/...` 비교밖에 없고, 그 비교는 저장된 값을 그대로 쓴다(`BlackboardKeyType_Float.cpp:46-58`). 결과적으로 "타겟 없음" 이 **모든 근거리 조건에서 가장 관대한 값**(거리 0 = 코앞)으로 읽힌다. `TargetDistance <= AttackRange` 로 근접 공격 브랜치를 여는 트리에서, 타겟을 놓친 직후 그 조건이 참으로 남는다. 지금은 상위에 `TargetActor Is Set` 이 함께 걸려 가려질 가능성이 크지만, 계약 자체가 뒤집혀 있어 데코 하나만 재배치해도 드러난다.
- **제안**: Clear 대신 큰 센티널(예: `TNumericLimits<float>::Max()` 또는 `BIG_NUMBER`)을 써서 "타겟 없음 = 무한히 멀다" 로 읽히게 한다. Clear 를 유지한다면 `WxBlackboardKeys.h` 의 주석을 "0 이 기록되며, 소비자는 반드시 `TargetActor` 를 먼저 검사해야 한다" 로 고쳐 계약을 명시한다.
- **확신도**: 높음(메커니즘은 엔진 소스로 확인) / 중간(실제 피해는 BT 에셋의 데코 배치에 달림)

### 2. 🟢 `EWxTeam` 이 WxAI 에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:121`, `WxCharacterBase.cpp:170`·`:182`, `WxEnemyCharacter.cpp:18`, `WxPlayerCharacter.cpp:22` 뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(피아 판정도 엔진 타입인 `FGenericTeamId::GetAttitude` 로 한다 — `WxAIPerceptionComponent.cpp:276`). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인 플러그인이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h` 를 `WxCore` 로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 3. 🟢 Patrol 과 Wander 의 감속 GE 부여·제거 코드가 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58-68`·`:87-92`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:54-64`·`:106-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` → `OnTaskFinished` 에서 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다. 헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트(`WxBTTask_Patrol.h:39`·`:48`·`:62`, `WxBTTask_Wander.h:56`·`:65`·`:75`)와 `GetStaticDescription` 의 미지정 분기도 동일하다. SetByCaller 태그나 핸들 수명 규약을 바꾸면 두 곳을 함께 고쳐야 한다.
- **제안**: 두 태스크의 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode` 라 공통 부모를 만들 수 없으므로, "부여/해제" 두 함수만 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`·`Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`
- **지난 리뷰 지적 중 이번에 수정을 확인한 것**:
  - 억제 해제 시 재획득이 Sight 로 한정됐고(`WxAIPerceptionComponent.cpp:167`·`:176`) 여러 후보 중 최근접을 확정한다(`:181-187`). 부가로 `HearingConfig`/`DamageConfig` 에 유한 `MaxAge` 5초를 줘(`:41-42`) 자극이 영원히 "감지 중" 으로 남던 원인도 제거됐다 — `RegisterSenseConfig` 가 이 값을 `MaxActiveAge` 로 반영하는 것을 엔진에서 확인(`AIPerceptionComponent.cpp:248`).
  - Patrol 의 이동 목표 키 이원화는 `InitializeFromAsset` 에서 `BlackboardKey.SelectedKeyName` 을 고정하고(`WxBTTask_Patrol.cpp:28`) `HideCategories = (Blackboard)`(`WxBTTask_Patrol.h:20`) 로 저작을 잠가 해소됐다. `HideCategories` 가 함께 가리는 것은 `ObservedBlackboardValueTolerance` 뿐이고 `AcceptableRadius` 등 실사용 프로퍼티는 `Category = Node` 라 그대로 노출된다.
- **엔진 대조로 확인 후 발견에서 제외한 것들**:
  - `WxBTTask_ActivateAbility.cpp:46` 의 `FScopedAbilityListLock` 은 순회 중 배열 재할당을 실제로 막는다 — 락 중 `GiveAbility` 는 대기 목록으로 미뤄진다(`AbilitySystemComponent_Abilities.cpp:311`).
  - `UWxBTComposite_RandomChoice` 의 `GetNextChildHandler` 오버라이드는 정상 동작한다 — 5.8 의 `UBTCompositeNode::GetNextChild` 가 가상 함수를 직접 호출한다(`BTCompositeNode.cpp:611`). 사전 필터의 `NotifyDecoratorsOnFailedActivation` 도 `SearchData.AddUniqueUpdate` 만 쓰므로 `FindChildToExecute` 밖에서 불려도 안전하다(`BTCompositeNode.cpp:297-315`).
  - BT 가 통째로 멈춰도 `UWxBTTask_ReturnHome` 의 억제가 남지 않는다 — `StopTree` 가 활성 태스크에 `WrappedAbortTask` 를 부른 뒤 `OnTaskFinished` 까지 통지한다(`BehaviorTreeComponent.cpp:395-401`).
  - `UWxBTTask_Patrol` 이 `bPatrolFinished` 때 `Super::ExecuteTask` 없이 `InProgress` 를 반환해도 MoveTo 노드 메모리의 잔여 상태는 해롭지 않다 — 5.8 의 MoveTo 는 AITask 경로라 `MoveRequestID` 가 무효로 남고, `AbortTask` 의 else 분기가 이미 `Reset` 된 Task 를 보고 아무 일도 하지 않는다(`BTTask_MoveTo.cpp:235-254`).
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출은 중복 등록을 만들지 않는다 — 엔진이 같은 클래스의 기존 config 를 교체한다(`AIPerceptionComponent.cpp:122-139`).
  - `State.InCombat` 은 이 컴포넌트만 발행하고(`WxAIPerceptionComponent.cpp:142`·`:146`) 소비자는 `WxNameplateComponent.cpp:27`·`WxEnemyCharacter.cpp:140` 뿐이라, 다른 발행자와 겹쳐 `SetRecognized` 의 전환 판정이 어긋날 여지가 없다.
  - 노드 인스턴스가 BTComponent 에 캐시돼 트리 재시작 시 `PatrolCursor`/`bPatrolFinished` 가 살아남는 엔진 특성은 확인했으나, 적 리스폰은 새 폰 → 새 기본 컨트롤러라 실제로 재사용되지 않아 제외했다.
- **미검토 / 한계**:
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 1 의 실피해 여부는 실제 트리에서 `TargetDistance` 데코가 `TargetActor Is Set` 아래에 있는지에 달려 있으며, 그 배치는 확인하지 못했다.
  - 인게임 재현은 하지 않았다. 모든 판단은 소스와 UE 5.8 엔진 소스 대조로만 세웠다.
  - 이번에 검토 후 발견에서 뺀 것들: `WxBTComposite_RandomChoice.cpp:24`·`:48` 이 `CastInstanceNodeMemory` 대신 `reinterpret_cast` 를 쓰는 점(크기·정렬은 맞음), `WxBTTask_ActivateAbility.h:6` 이 Public 헤더에서 `AbilitySystemComponent.h` 전체를 끌어오면서 `:12` 에 같은 타입을 중복 전방 선언하는 점, `WxBTDecorator_AttributeRatio` 의 UPROPERTY Category 가 다른 노드(`Wx|AI`)와 달리 `Wx` 인 점, `WxAI.Build.cs` 가 소스 어디서도 쓰지 않는 `NavigationSystem`·`GameplayTasks` 를 명시 의존으로 두는 점(AIModule 이 이미 공개 의존), `WxBlackboardKeys` 의 키 검증이 매 접근마다 도는 점(비-Shipping 한정)은 영향이 작아 세우지 않았다.

---
*문서 기준 커밋 `d459af72` · 리뷰일 2026-08-26 · 소스 29파일 — `/module-review`로 갱신*
