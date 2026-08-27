# WxAI — 코드 리뷰

> 여전히 매우 깨끗한 모듈이다 — 소스 29파일 전부 Copyright 첫 줄·`Wx` prefix·델리게이트 콜백 `Handle` prefix를 지키고 `BlueprintCallable`·`FORCEINLINE`·람다가 하나도 없으며, 지난 리뷰의 🟡(타겟 부재 시 `TargetDistance` 가 0으로 기록되던 문제)은 `WxBlackboardKeys::NoTargetDistance` 센티널 도입으로 실제로 해소됐다. 이번 리뷰는 29파일(cpp 14 + h 15)을 모두 열고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite 의 cpp 를 깊게 봤으며, UE 5.8 엔진·GameplayAbilities 소스와 대조해 판단 근거를 라인 단위로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `ActivateAbility` 의 중단은 어빌리티가 실제로 취소됐는지 확인하지 않고 즉시 마감한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:96-110`
- **범주**: 버그/정확성
- **문제**: `AbortTask` 는 `CleanUp()` 으로 `OnAbilityEnded` 구독을 먼저 끊고 `CancelAbilityHandle` 을 호출한 뒤 `Super::AbortTask` 로 곧바로 `Aborted` 를 반환한다. 그런데 `CancelAbilityHandle` → `CancelAbilitySpec` → `UGameplayAbility::CancelAbility` 는 **`CanBeCanceled()` 가 거짓이면 아무 일도 하지 않고 반환**하며, 어빌리티의 `ScopeLockCount > 0` 이면 취소를 `WaitingToExecute` 로 미룬다(`Engine/Plugins/Runtime/GameplayAbilities/.../GameplayAbility.cpp` 의 `UGameplayAbility::CancelAbility`, `AbilitySystemComponent_Abilities.cpp:1316`).
  즉 `SetCanBeCanceled(false)` 로 잠근 어빌리티(무적/경직불가 콤보 공격 등 액션 RPG에서 흔한 구성)를 중단하면, BT 는 그 즉시 다른 브랜치로 넘어가는데 어빌리티는 계속 돌아 몽타주·히트박스·GE 가 유지된다. 구독은 이미 끊겼으므로 실제 종료가 와도 BT 를 되돌릴 방법이 없다. `AbilityTag` 가 가리키는 어빌리티는 디자이너가 BT 에디터에서 지정하므로, 코드 쪽에서 이 조합을 막을 수단도 없다.
- **제안**: `CancelAbilityHandle` 직후 `ASC->FindAbilitySpecFromHandle(HandleToCancel)` 로 여전히 `IsActive()` 인지 확인하고, 살아 있으면 구독을 유지한 채 `InProgress` 를 반환해 종료 통지에서 `FinishLatentAbort` 로 마감한다(엔진의 지연 중단 규약). 최소한 취소가 먹지 않은 경우를 `LogWxAI` 경고로 드러내 저작 실수를 잡을 수 있게 한다.
- **확신도**: 중간(취소 거부 경로는 엔진 소스로 확인했고, 실제 노출 여부는 어빌리티 에셋이 `bCanBeCanceled` 를 잠그는지에 달림)

### 2. 🟡 `AttributeRatio` 데코레이터는 FlowAbortMode 를 None 으로 못 박아 실행 중 재평가가 불가능하다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11-12`
- **범주**: 설계/구조
- **문제**: 생성자가 `bAllowAbortLowerPri = false; bAllowAbortChildNodes = false;` 로 두는데, 이 두 플래그는 BT 에디터의 FlowAbortMode 드롭다운을 걸러내는 용도다(`Engine/Source/Editor/BehaviorTreeEditor/Private/DetailCustomizations/BehaviorDecoratorDetails.cpp:96-100`). 둘 다 꺼지면 디자이너가 고를 수 있는 값이 `None` 뿐이고, `None` 인 데코레이터는 관찰자(aux node)로 등록되지 않으므로 조건이 **트리 재탐색 시점에만** 평가된다.
  결과적으로 "HP 30% 이하 → 광폭화/도주" 같은 브랜치는 전투 중 HP 가 떨어지는 순간이 아니라 현재 태스크가 끝난 뒤에야 열린다. 긴 콤보·피격 경직 태스크가 도는 동안에는 전환이 통째로 밀린다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 이 한계를 알고 폴링 + `RequestExecution` 으로 우회했는데(`WxBTDecorator_BeyondLeash.cpp:82-94`), AttributeRatio 에는 그 장치도 없고 헤더 주석(`WxBTDecorator_AttributeRatio.h:11-15`)에도 제약이 적혀 있지 않다. 같은 플래그를 쓰는 `UWxBTDecorator_RandomWeight`(`WxBTDecorator_RandomWeight.cpp:7-8`)는 순수 데이터 운반이라 올바른 설정이다.
- **제안**: 즉시 반응이 필요하면 BeyondLeash 와 같은 폴링 방식(`INIT_DECORATOR_NODE_NOTIFY_FLAGS` + `TickNode` 에서 전이 감지 후 `RequestExecution`)을 채택하고 `bAllowAbortLowerPri` 를 열어 준다. 의도된 제약이라면 헤더에 "실행 중 재평가되지 않는다 — 태스크 경계에서만 전환" 을 명시해 저작자가 오해하지 않게 한다.
- **확신도**: 중간(메커니즘은 엔진 소스로 확인, 제약을 감수한 설계일 수 있음)

### 3. 🟢 `EWxTeam` 이 WxAI 에 정의돼 있으나 WxAI 안에서 전혀 쓰이지 않는다 *(지난 리뷰 미해결)*
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:121`, `WxCharacterBase.cpp:170`·`:182`, `WxEnemyCharacter.cpp:18`, `WxPlayerCharacter.cpp:22` 뿐이고 WxAI 코드는 한 번도 참조하지 않는다(피아 판정조차 엔진 타입 `FGenericTeamId::GetAttitude` 로 한다 — `WxAIPerceptionComponent.cpp:276`). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이므로, 다른 도메인 플러그인이 필요해지는 순간 "모든 Wx 플러그인은 WxCore 외 플러그인을 참조하면 안 된다" 규칙에 걸린다.
- **제안**: `WxTeamTypes.h` 를 `WxCore` 로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 4. 🟢 Patrol 과 Wander 의 감속 GE 부여·제거 코드가 통째로 중복이다 *(지난 리뷰 미해결)*
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58-68`·`:87-92`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:54-64`·`:106-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf`, 그리고 `OnTaskFinished` 의 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다(Wander 쪽 주석도 "Patrol 과 동일하다" 고 인정한다). 헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트(`WxBTTask_Patrol.h:39`·`:48`·`:62`, `WxBTTask_Wander.h:56`·`:65`·`:75`)와 `GetStaticDescription` 의 "감속 GE 미지정" 분기도 같다. SetByCaller 태그나 핸들 수명 규약이 바뀌면 두 곳을 함께 고쳐야 한다.
- **제안**: 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode` 라 공통 부모를 만들 수 없으므로(그리고 구체 BT Task 상속은 이 프로젝트가 피하는 패턴이므로), "부여/해제" 두 함수만 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, 경계 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **지난 리뷰 지적 중 이번에 수정을 확인한 것**: 타겟 부재 시 `TargetDistance` 를 Clear 해 0 이 기록되던 문제는 `WxBlackboardKeys::NoTargetDistance = TNumericLimits<float>::Max()`(`WxBlackboardKeys.cpp:84`)를 도입하고 서비스가 이를 기록하도록(`WxBTService_TargetDistance.cpp:36`) 고쳐 해소됐다. 헤더 주석(`WxBlackboardKeys.h:38`)도 새 계약에 맞게 갱신돼 있다.
- **엔진 대조로 확인 후 발견에서 제외한 것들**:
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출은 중복 등록을 만들지 않는다 — 엔진이 같은 클래스의 기존 config 를 교체한다(`AIPerceptionComponent.cpp:122-139`). 컴포넌트가 `AAIController::PerceptionComponent` 로 자동 등록되는 것도 `OnRegister` 에서 확인해, `UWxBTTask_ReturnHome` 의 `GetPerceptionComponent()` 조회가 유효함을 검증했다.
  - `bCreateNodeInstance = true` 인 `UWxBTTask_Patrol`(= `UBTTask_MoveTo` 파생)의 지연 완료 경로는 정상이다 — `FindTemplateNode`/`FindInstanceContainingNode` 가 인스턴스화된 노드를 실행 인덱스로 되짚어 준다(`BehaviorTreeComponent.cpp`).
  - `bPatrolFinished` 일 때 `Super::ExecuteTask` 없이 `InProgress` 를 반환해 남는 MoveTo 노드 메모리(`MoveRequestID`)는 해롭지 않다 — `AbortMove` 가 현재 요청 ID 와 일치할 때만 동작한다.
  - `UWxBTComposite_RandomChoice` 의 메모리 레이아웃은 안전하다 — `UBTCompositeNode::GetInstanceMemorySize()` 가 `sizeof(FBTCompositeMemory)` 를 돌려주고 Wx 가 파생 구조체 크기로 덮어쓴다(`BTCompositeNode.cpp:702`). 사전 필터의 `NotifyDecoratorsOnFailedActivation` 도 `SearchData.AddUniqueUpdate` 만 쓰므로 `FindChildToExecute` 밖에서 불려도 안전하다.
  - `UWxBTDecorator_BeyondLeash` 의 폴링은 낭비가 아니다 — FlowAbortMode 가 LowerPriority 인 데코레이터는 자기 브랜치가 활성화되는 순간 aux node 에서 제거되므로, 틱은 브랜치가 도는 동안이 아니라 **필요한 구간에만** 돈다.
  - `UWxPatrolComponent` 의 Loop 모드 인덱싱은 정상이다 — 5.8 의 `GetNumberOfSplinePoints()` 는 닫힌 루프에서도 제어점 개수를 그대로 돌려주고 `GetLocationAtSplinePoint` 는 인덱스를 클램프한다.
  - `WxBlackboardKeys` accessor 는 Blackboard 널 검사를 호출부에 맡기는데, 모듈 내 6개 호출부 전부 가드가 걸려 있다.
  - 플러그인 경계 위반은 없다 — WxAI 가 include 하는 Wx 헤더는 `WxGameplayTags.h`(WxCore) 하나뿐이다.
- **미검토 / 한계**:
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 2 의 실제 체감(전환 지연 폭)은 어떤 태스크가 AttributeRatio 아래에 놓이는지에 달려 있으며 확인하지 못했다.
  - 인게임 재현은 하지 않았다. 모든 판단은 소스와 UE 5.8 엔진 소스 대조로만 세웠다.
  - 영향이 작아 발견으로 세우지 않은 것들: `WxAI.Build.cs:11-22` 가 소스 어디서도 쓰지 않는 `NavigationSystem`·`GameplayTasks` 를 명시 의존으로 두고 `WxCore` 를 Public 에 두는 점(.cpp 전용이라 Private 로 충분), `WxBTTask_ActivateAbility.h:6` 이 Public 헤더에서 `AbilitySystemComponent.h` 전체를 끌어오면서 `:12` 에 같은 타입을 중복 전방 선언하는 점, `WxBTComposite_RandomChoice.cpp:24`·`:48` 이 `CastInstanceNodeMemory` 대신 `reinterpret_cast` 를 쓰는 점(크기·정렬은 맞음), `WxBTDecorator_AttributeRatio` 의 UPROPERTY Category 만 다른 노드(`Wx|AI`)와 달리 `Wx` 인 점, `UWxBTTask_Wander` 가 내비게이션 없이 `AddMovementInput` 으로만 이동해 절벽 낙하를 막지 못하는 점(전투 스트레이프 용도라 의도로 판단).

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 29파일 — `/module-review`로 갱신*
