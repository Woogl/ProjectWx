# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 프로젝트 규칙 준수(WxCore 외 의존 없음, `BlueprintCallable`·람다 미사용, `Handle` prefix, 저작권 헤더)는 위반 0건이고, GAS/BT 의 알려진 함정(어빌리티 Spec 배열 재할당, 동기 종료로 인한 latent 정지, 노드 인스턴싱, Composite 메모리 레이아웃)도 이미 방어돼 있다. 남은 문제는 대부분 "파생 상태를 발행해 두고 무효화 경로가 없는" 패턴에 몰려 있다. 커버리지: 소스 29파일 전부를 읽었고 Perception 컴포넌트·ActivateAbility·RandomChoice·MoveTo 파생 태스크·BeyondLeash 는 cpp 로직까지 파고들었으며, UE 5.8 엔진 소스와 소비자(`AWxEnemyController`, `AWxCharacterBase`)를 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 avoid-repeat 가 "지금 유효한 유일한 자식"까지 제외해 컴포지트를 실패시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:61`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:91`
- **범주**: 버그/정확성
- **문제**: `bAvoidRepeat` 가 켜져 있으면 직전 선택 자식을 무조건 `continue` 로 제외한다(`:61`). 자식이 2개이고 직전에 child 0 을 골랐는데 이번 진입에서 child 1 의 조건 Decorator(예: `AttributeRatio` HP<0.5 발악기)가 false 라면, child 1 은 조건 필터로 빠지고 child 0 은 avoid-repeat 로 빠져 `Candidates.Num() == 0` → `ReturnToParent`(실패)가 된다(`:91`). child 0 은 지금도 실행 가능한데 "직전에 썼다"는 이유만으로 공격 패턴 분기 전체를 포기하고 상위 Selector 가 하위 브랜치로 넘어간다. 자식이 적은 보스 패턴일수록 자주 걸린다.
- **제안**: avoid 때문에 후보가 비었을 때 avoid 를 한 단계 완화해 직전 자식을 다시 포함하는 폴백을 둔다. 엄격 avoid 가 의도라면 이 실패 거동을 헤더 시멘틱 주석에 명시한다.
- **확신도**: 중간 (상위 Selector 폴백에 의존하는 의도된 설계일 수 있음)

### 2. 🟡 SetTargetActor 의 조기 반환이 회전 모드·포커스를 영구 desync 시킬 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:215`
- **범주**: 버그/정확성
- **문제**: `SetTargetActor` 는 "BB 의 현재 TargetActor == NewTarget" 이면 즉시 반환하고, 뒤에 있는 파생 상태 발행(`AIC->ClearFocus`, `bUseControllerDesiredRotation=false`, `bOrientRotationToMovement=true` — 같은 파일 `:243`~`:251`)에 도달하지 못한다. 그런데 Blackboard Object 키는 엔진이 `FWeakObjectPtr` 로 저장하므로(`BlackboardKeyType_Object.cpp` 의 `SetValue`/`GetValue`) 타겟 액터가 파괴되면 BB 값이 컴포넌트 모르게 스스로 nullptr 이 된다. 이 상태에서 뒤늦게 `SetTargetActor(nullptr)`(억제 진입 `:160`, 사망 정리 `:203`)이 호출되면 "이미 nullptr" 이라 조기 반환하고, 폰은 strafe 회전 모드(`bOrientRotationToMovement=false`)에 갇힌 채 남는다. 이후 정찰·배회에서 진행 방향을 보지 않고 미끄러지듯 이동하며, 다음 타겟을 잡기 전까지 되돌릴 경로가 없다.
- **제안**: "마지막으로 적용한 타겟"을 BB 가 아니라 컴포넌트 자체 필드(`TWeakObjectPtr<AActor>`)로 들고 그것과 비교한다. BB 쓰기와 회전 모드 발행의 판단 기준을 분리하면 BB 의 자체 무효화와 무관해진다.
- **확신도**: 중간

### 3. 🟡 타겟의 사망을 관측하는 경로가 없어 State.InCombat 이 시체에 latch 된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:88`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:91`
- **범주**: 설계/구조
- **문제**: `UpdateRecognition` 은 오직 `HandleTargetPerceptionUpdated` 에서만 호출된다. 즉 인식 상태는 "퍼셉션 이벤트가 새로 올 때"만 재판정된다. 그런데 `AWxCharacterBase::HandleDeath`(`Source/WxGame/Character/WxCharacterBase.cpp:263`)는 `OnDeath` 브로드캐스트만 하고 액터를 파괴하지 않으므로, 타겟이 죽어도 BB TargetActor 는 유효하게 남고 시야도 유지돼 새 자극 이벤트가 오지 않는다. 결과적으로 `State.InCombat` 이 켜진 채 남아 네임플레이트·전투 BGM 이 시체 앞에서 유지되고, BT 전투 브랜치도 시체를 계속 공격한다. 자기 자신의 사망은 `BindOwnerDeath`/`HandleDeathTagChanged`(`:165`, `:194`)로 정확히 이 문제를 막고 있는데 타겟 쪽 대칭 처리만 비어 있다.
- **제안**: `SetTargetActor` 에서 새 타겟 ASC 의 `State.Dead` 태그 이벤트를 구독/해제해(자기 폰용 `BindOwnerDeath` 와 같은 패턴) 사망 시 타겟을 비운다. 또는 BT 전투 브랜치 진입을 타겟 생존 데코레이터로 게이팅한다.
- **확신도**: 중간

### 4. 🟡 MaxWalkSpeed 저장·복원이 SPD 어트리뷰트의 소유권과 충돌한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:53`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:56`
- **범주**: 설계/구조
- **문제**: 두 태스크는 진입 시 `MaxWalkSpeed` 를 캐시해 배율을 곱하고, 종료 시 캐시한 **절대값**으로 되돌린다(`WxBTTask_Patrol.cpp:79`, `WxBTTask_Wander.cpp:102`). 그러나 같은 필드를 `AWxCharacterBase::HandleSPDAttributeChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:208`)가 `MaxWalkSpeed = BaseWalkSpeed * SPD` 로 어트리뷰트 변경마다 절대값 재계산한다. 소유자가 둘이라 (a) 정찰 중 SPD 가 바뀌면 정찰 감속 배율이 통째로 사라지고, (b) 정찰 중 걸린 버프/디버프가 태스크 종료 시 "정찰 진입 시점"의 낡은 절대값으로 덮어써져 다음 SPD 이벤트까지 무효화된다. 예: Base 400 / SPD 1 → 정찰 진입(캐시 400, 실제 200) → 가속 버프 SPD 2(실제 800) → 정찰 종료 시 400 으로 복원되어 버프가 남은 시간 동안 사라진다.
- **제안**: 감속을 CMC 필드 직접 쓰기가 아니라 SPD 를 낮추는 GE(태스크 진입/종료에 부여·제거)로 적용해 소유권을 한쪽으로 모은다. 최소 대응은 절대값 대신 배율을 되돌리는 것이지만, 배율 0 을 허용하는 현 Clamp(`WxBTTask_Patrol.h:32`) 때문에 나눗셈 복원은 별도 가드가 필요하다.
- **확신도**: 중간 (정찰·배회 중 SPD 변동 빈도에 따라 체감이 갈린다)

### 5. 🟡 Sight/Hearing 센스가 ApplySenseSettings 호출에만 의존해 등록된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:34`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:52`
- **범주**: 설계/구조
- **문제**: 생성자는 Sight/Hearing/Damage 세 config 를 만들지만 `ConfigureSense` 는 `PostInitProperties` 에서 Damage 에만 호출한다. 엔진은 `OnRegister` 시점의 `SensesConfig` 배열만 퍼셉션 시스템에 등록하므로(`AIPerceptionComponent.cpp` 의 `OnRegister`/`RegisterSenseConfig`), Sight·Hearing 은 `ApplySenseSettings` 가 호출되기 전까지 감지에 아예 참여하지 않는다. 유일한 호출부인 `AWxEnemyController::OnPossess` 는 폰이 `AWxEnemyCharacter` 일 때만 호출한다(`Source/WxGame/Controller/WxEnemyController.cpp:30`). 즉 다른 폰 타입에 이 컴포넌트를 붙이면 경고 한 줄 없이 시각·청각이 죽은 AI 가 된다.
- **제안**: `PostInitProperties` 에서 Damage 와 함께 Sight/Hearing 도 기본값으로 `ConfigureSense` 해 두고, `ApplySenseSettings` 는 값 갱신 + `RequestStimuliListenerUpdate` 만 하게 한다. 엔진 `ConfigureSense` 는 같은 클래스면 교체하는 동작이라 중복 등록되지 않는다.
- **확신도**: 중간 (현 사용처에선 항상 호출되므로 잠재 결함)

### 6. 🟢 Patrol/Wander 의 속도 캐시·복원 로직이 그대로 중복돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55`
- **범주**: 중복/복잡도
- **문제**: 진입 캐시 블록(약 10줄)과 종료 복원 블록(약 12줄)이 두 파일에 사실상 동일하게 복제돼 있고, 주석까지 "Patrol 과 동일"로 명시돼 있다. 4번을 고칠 때 두 곳을 각각 고쳐야 하고 한쪽만 고치면 조용히 갈라진다.
- **제안**: 4번의 GE 방식으로 전환하면서 공통화하거나, 최소한 WxAI 내부 공용 헬퍼로 뽑는다.
- **확신도**: 높음

### 7. 🟢 Wander 가 매 실행마다 방향 후보 배열을 힙 할당한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:29`
- **범주**: 성능/안전
- **문제**: 최대 8개로 고정된 후보를 기본 할당자 `TArray<int32>` 에 담아 배회 진입마다 힙 할당이 발생한다. 같은 모듈의 `UWxBTComposite_RandomChoice` 는 동일 상황에서 `TInlineAllocator<8>` 을 쓴다(`Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:53`). 다수의 배회 AI 가 짧은 Duration(기본 1s)으로 반복 진입하면 누적된다.
- **제안**: `TArray<int32, TInlineAllocator<8>>` 로 맞춘다.
- **확신도**: 높음

### 8. 🟢 GetStaticDescription override 에서 Super:: 를 호출하지 않는다 (모듈 내 불일치)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:77`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:69`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:23`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:10`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:15`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:29`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5("override 시 `Super::` 호출") 위반이다. 베이스 구현이 무의미한 override(`CalculateRawConditionValue`, `GetInstanceMemorySize` 등)와 달리 `GetStaticDescription` 의 베이스는 노드 이름을 돌려주는 실동작이 있고, 실제로 `UWxBTTask_Patrol::GetStaticDescription`(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:68`)만 `Super::` 를 붙여 BT 에디터 노드 표기가 나머지 6개와 다르게 나온다.
- **제안**: 규칙대로 `Super::GetStaticDescription()` 결과를 앞에 붙여 통일한다.
- **확신도**: 낮음 (의도된 설계일 수 있음)

### 9. 🟢 성공한 자극마다 무조건 TargetActor 를 최신 감지 액터로 덮어쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82`
- **범주**: 설계/구조
- **문제**: `HandleTargetPerceptionUpdated` 는 성공 자극이면 우선순위·거리 비교 없이 그 액터를 타겟으로 확정한다. 감지 범위 안에 대상이 둘 이상이면(멀티플레이·소음원 포함) 센스 갱신마다 타겟이 "가장 최근 감지된 액터"로 뒤바뀌어 포커스·strafe 회전 모드가 대상 사이에서 진동한다. 특히 `UAISenseConfig_Damage` 는 affiliation 필터가 없어 아군 피해원까지 타겟이 될 수 있다.
- **제안**: 이미 유효 타겟이 있으면 유지하고 우선순위/거리/위협도 비교를 통과할 때만 스위치하는 규칙을 둔다.
- **확신도**: 낮음 (단일 타겟을 전제한 의도된 단순화일 수 있음)

### 10. 🟢 Wander 가 내비메시 검증 없이 raw 이동 입력을 가한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:94`
- **범주**: 성능/안전
- **문제**: 고른 월드 방향으로 `AddMovementInput` 만 넣고 내비메시 투영·장애물 회피가 없다. 벽 밀착은 무해하나 배치에 따라 절벽·내비메시 밖으로 걸어나갈 여지가 있다(오픈월드 QA 이슈 소지). 기본값이 짧은 시간(1s)·저속(0.3배)이라 노출은 제한적이다.
- **제안**: 목표 방향을 내비메시 위 지점으로 투영(`ProjectPointToNavigation`)하거나 이동 전 경계 체크를 둔다.
- **확신도**: 낮음 (단순 idle 배회용으로 의도된 거동일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/` 헤더 15개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`. 대조용으로 `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp` 및 UE 5.8 의 `AIPerceptionComponent.cpp`·`BTCompositeNode.cpp`·`BlackboardKeyType_Object.cpp`·`SplineComponent.cpp` 참조.
- **확인 결과 문제 없던 항목**: `UWxBTTask_ActivateAbility` 의 latent 수명주기(Spec 배열 재할당 대비 핸들 재조회, 동기 종료 시 `InProgress` 영구 정지 방지, Abort 경로에서 델리게이트 선해제 후 취소 — `FinishLatentTask` 누락·중복 없음). `UWxBTComposite_RandomChoice` 의 노드 메모리 레이아웃(`FBTCompositeMemory` 상속)과 `GetActiveInstanceIdx()` 사용은 엔진 `FindChildToExecute` 와 동일. `UWxBTDecorator_BeyondLeash` 의 인스턴스 메모리는 `OnBecomeRelevant` 가 항상 선행 시드. `UWxPatrolComponent::GetNextIndex` 의 세 MoveMode 경계값, 그리고 커서가 범위를 벗어나도 엔진 `GetLocationAtSplinePoint` 가 인덱스를 clamp 하므로 원점 이동 위험 없음. BB Object 키는 weak 저장이라 댕글링 없음. 규칙 스캔: `BlueprintCallable` 오용·람다·저작권 헤더 누락·`Handle` prefix 누락·WxCore 외 Wx 의존 — 위반 0건. 직전 리뷰가 지적한 `WxBTTask_Wander.cpp` 의 미사용 `NavigationSystem.h` include 는 이미 제거됨.
- **미검토 / 한계**: BehaviorTree/Blackboard `.uasset` 자체는 보지 않았다. 따라서 README 가 규정한 "BeyondLeash 의 FlowAbortMode = Lower Priority", "복귀 브랜치가 전투 브랜치보다 상위 우선순위", "Blackboard 에셋에 5개 키가 동명·동타입 등록" 같은 에셋 측 규약이 실제로 지켜지는지는 C++ 만으로 확인 불가다(코드는 기본값과 런타임 경고로만 방어한다). 멀티플레이 실행 검증(서버 전용 경로, MinimalReplication 태그 전파)과 리시 왕복·타겟 진동의 실제 플레이 확인도 정적 분석 범위 밖이다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 29파일 — `/module-review`로 갱신*
