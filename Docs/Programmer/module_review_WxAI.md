# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 프로젝트 규칙 준수도가 높고(`BlueprintCallable` 오용·람다·저작권 헤더 누락·`Handle` prefix 누락 0건, WxCore 외 Wx 의존 0건), GAS/BT 의 알려진 함정(어빌리티 Spec 배열 재할당, latent 태스크 동기 종료, Composite 노드 메모리 레이아웃, 인스턴스 메모리 시드)은 이미 방어돼 있다. 남은 문제는 "파생 상태를 발행해 두고 무효화 경로가 없는" 패턴(회전 모드, `State.InCombat`, `MaxWalkSpeed`)에 몰려 있다. 커버리지: 소스 29파일 전부를 읽었고 `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTComposite_RandomChoice`·MoveTo 파생 태스크·`WxBTDecorator_BeyondLeash` 는 cpp 로직까지 파고들었으며, 소비자(`AWxEnemyController`, `AWxCharacterBase`)와 대조해 검증했다. 참고로 WxAI 의 C++ 소스는 직전 리뷰(2026-07-25) 이후 한 줄도 바뀌지 않았고 README 만 갱신됐다 — 아래 발견 중 일부는 그때부터 남아 있는 항목이며, 소비자 측 변경(WxSound/BGM 플러그인 제거 등)으로 근거가 달라진 부분은 갱신했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 8 |

## 결과

### 1. 🟡 avoid-repeat 가 "지금 유효한 유일한 자식"까지 제외해 컴포지트를 실패시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:61`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:91`
- **범주**: 버그/정확성
- **문제**: `bAvoidRepeat` 가 켜져 있으면 직전 선택 자식을 조건 검사 전에 무조건 `continue` 로 제외한다(`:61`). 자식이 2개이고 직전에 child 0 을 골랐는데 이번 진입에서 child 1 의 조건 Decorator(예: `UWxBTDecorator_AttributeRatio` 로 건 HP<0.5 발악기)가 false 라면, child 1 은 조건 필터로 빠지고 child 0 은 avoid-repeat 로 빠져 `Candidates.Num() == 0` → `ReturnToParent`(실패)가 된다(`:91`). child 0 은 지금도 실행 가능한데 "직전에 썼다"는 이유만으로 공격 패턴 분기 전체를 포기하고 상위 Selector 가 하위 브랜치(추격·배회 등)로 넘어간다. 자식 수가 적은 보스 패턴일수록 자주 걸린다.
- **제안**: avoid 때문에 후보가 비면 avoid 를 한 단계 완화해 직전 자식을 다시 포함하는 폴백을 둔다. 엄격 avoid 가 의도라면 이 실패 거동을 헤더 시멘틱 주석에 명시한다.
- **확신도**: 중간 (상위 Selector 폴백에 의존하는 의도된 설계일 수 있음)

### 2. 🟡 SetTargetActor 의 조기 반환이 회전 모드를 strafe 에 영구 고착시킬 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:215`
- **범주**: 버그/정확성
- **문제**: `SetTargetActor` 는 "BB 의 현재 TargetActor == NewTarget" 이면 즉시 반환하고, 뒤에 있는 파생 상태 발행(`AIC->ClearFocus`, `bUseControllerDesiredRotation=false`, `bOrientRotationToMovement=true` — 같은 파일 `:245`~`:250`)에 도달하지 못한다. 그런데 Blackboard 의 Object 키는 엔진이 weak 포인터로 저장하므로, 타겟 액터가 파괴(디스폰·레벨 언로드)되면 BB 값이 컴포넌트 모르게 스스로 nullptr 이 된다. 이 상태에서 뒤늦게 `SetTargetActor(nullptr)`(억제 진입 `:160`, 사망 정리 `:203`)이 호출되면 "이미 nullptr" 이라 조기 반환하고, 폰은 strafe 회전 모드(`bOrientRotationToMovement=false`)에 갇힌 채 남는다. 포커스도 이미 소실돼 ControlRotation 이 갱신되지 않으므로, 이후 정찰·복귀 이동에서 진행 방향을 보지 않고 미끄러지듯 이동하며 다음 타겟을 잡기 전까지 되돌릴 경로가 없다.
- **제안**: "마지막으로 적용한 타겟"을 BB 가 아니라 컴포넌트 자체 필드(`TWeakObjectPtr<AActor>`)로 들고 그것과 비교한다. BB 쓰기와 회전 모드 발행의 판단 기준을 분리하면 BB 의 자체 무효화와 무관해진다.
- **확신도**: 중간

### 3. 🟡 타겟의 사망을 관측하는 경로가 없어 State.InCombat 이 시체에 latch 된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:88`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:91`
- **범주**: 설계/구조
- **문제**: `UpdateRecognition` 은 오직 `HandleTargetPerceptionUpdated` 에서만 호출된다(`:88`). 즉 인식 상태는 "퍼셉션 자극이 새로 올 때"만 재판정된다. 그런데 `AWxCharacterBase::HandleDeath`(`Source/WxGame/Character/WxCharacterBase.cpp:279`)는 무기 판정 해제와 `OnDeath` 브로드캐스트만 하고 액터를 파괴하지 않으며(리포지토리 전체에 사망 시 `Destroy`/`SetLifeSpan` 호출이 없다), 시체는 시야 안에 그대로 남아 새 자극 이벤트를 만들지 않는다. 결과적으로 BB TargetActor 는 시체를 가리킨 채 유지되고 `State.InCombat` 도 켜진 채 남아, 복제 태그를 소비하는 네임플레이트가 계속 전투 표시를 하고 BT 전투 브랜치도 시체를 계속 공격한다. 자기 자신의 사망은 `BindOwnerDeath`/`HandleDeathTagChanged`(`:165`, `:194`)로 정확히 이 문제를 막고 있는데 타겟 쪽 대칭 처리만 비어 있다.
- **제안**: `SetTargetActor` 에서 새 타겟 ASC 의 `State.Dead` 태그 이벤트를 구독/해제해(자기 폰용 `BindOwnerDeath` 와 같은 패턴) 사망 시 타겟을 비운다. 또는 BT 전투 브랜치 진입을 타겟 생존 Decorator 로 게이팅한다.
- **확신도**: 중간 (BT 에셋 측에 생존 게이트가 이미 있을 수 있음 — C++ 만으로는 확인 불가)

### 4. 🟡 MaxWalkSpeed 저장·복원이 SPD 어트리뷰트의 소유권과 충돌한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:61`
- **범주**: 설계/구조
- **문제**: 두 태스크는 진입 시 `MaxWalkSpeed` 를 캐시해 배율을 곱하고, 종료 시 캐시한 **절대값**으로 되돌린다(`WxBTTask_Patrol.cpp:85`, `WxBTTask_Wander.cpp:110`). 그러나 같은 필드를 `AWxCharacterBase::HandleSPDAttributeChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:226`)가 `MaxWalkSpeed = BaseWalkSpeed * SPD` 로 어트리뷰트 변경마다 절대값 재계산한다. 소유자가 둘이라 (a) 정찰 중 SPD 가 바뀌면 정찰 감속 배율이 통째로 사라지고, (b) 정찰 중 걸린 버프/디버프가 태스크 종료 시 "정찰 진입 시점"의 낡은 절대값으로 덮어써져 다음 SPD 이벤트까지 무효화된다. 예: Base 400 / SPD 1 → 정찰 진입(캐시 400, 실제 200) → 가속 버프 SPD 2(실제 800) → 정찰 종료 시 400 으로 복원되어 버프가 남은 시간 동안 사라진다.
- **제안**: 감속을 CMC 필드 직접 쓰기가 아니라 SPD 를 낮추는 GE(태스크 진입 시 부여·종료 시 제거)로 적용해 소유권을 한쪽으로 모은다. 최소 대응은 절대값 대신 배율을 되돌리는 것이지만, 배율 0 을 허용하는 현 Clamp(`WxBTTask_Patrol.h:32`) 때문에 나눗셈 복원엔 별도 가드가 필요하다.
- **확신도**: 중간 (정찰·배회 중 SPD 변동 빈도에 따라 체감이 갈린다)

### 5. 🟡 Sight/Hearing 센스가 ApplySenseSettings 호출에만 의존해 등록된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:38`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:59`
- **범주**: 설계/구조
- **문제**: 생성자는 Sight/Hearing/Damage 세 config 를 만들지만 `ConfigureSense` 는 `PostInitProperties` 에서 Damage 에만 호출한다(`:40`). Sight·Hearing 은 `ApplySenseSettings` 안에서야 `ConfigureSense` 된다(`:59`, `:65`). 엔진은 `SensesConfig` 배열에 등록된 센스만 퍼셉션 시스템에 리스너로 올리므로, `ApplySenseSettings` 가 호출되기 전까지 이 컴포넌트는 시각·청각이 전혀 없는 상태다. 유일한 호출부인 `AWxEnemyController::OnPossess` 는 폰이 `AWxEnemyCharacter` 로 캐스팅될 때만 호출한다(`Source/WxGame/Controller/WxEnemyController.cpp:30`). 즉 다른 폰 타입에 이 컴포넌트를 붙이면 경고 한 줄 없이 "피해만 감지하는" AI 가 된다.
- **제안**: `PostInitProperties` 에서 Damage 와 함께 Sight/Hearing 도 기본값으로 `ConfigureSense` 해 두고, `ApplySenseSettings` 는 값 갱신 + `RequestStimuliListenerUpdate` 만 하게 한다.
- **확신도**: 중간 (현 사용처에선 항상 호출되므로 잠재 결함)

### 6. 🟡 AttributeRatio 는 FlowAbortMode 를 켜도 실시간 재평가가 일어나지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16`
- **범주**: 설계/구조
- **문제**: 헤더가 "실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다"고 안내하지만, 이 Decorator 는 `CalculateRawConditionValue` 만 구현하고 `OnBecomeRelevant`/`TickNode`/어트리뷰트 변경 구독이 전혀 없다(cpp 전체가 생성자·`GetStaticDescription`·조건 계산 3개뿐). FlowAbortMode 는 "abort 를 허용하는 범위"만 정할 뿐 재평가를 촉발하지 않으므로, HP 가 임계값을 넘어도 다른 원인으로 BT 재탐색이 일어나기 전까지 조건은 갱신되지 않는다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 정확히 이 한계 때문에 `TickNode` 폴링 + `RequestExecution` 을 직접 구현했다(`Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:51`). 디자이너가 헤더 안내대로 FlowAbortMode 만 켜고 "HP 50% 되면 즉시 발악기" 를 기대하면 조용히 어긋난다.
- **제안**: BeyondLeash 처럼 관찰자 경로를 구현한다 — `OnBecomeRelevant` 에서 대상 ASC 의 두 어트리뷰트 변경 델리게이트를 구독해 비율 판정이 뒤집힐 때만 `RequestExecution` 을 호출하고 `OnCeaseRelevant` 에서 해제한다(틱 폴링보다 저렴하다). 구현할 계획이 없다면 헤더 문구를 "재탐색 시점에만 재평가된다"로 정정한다.
- **확신도**: 중간

### 7. 🟢 Patrol/Wander 의 속도 캐시·복원 로직이 그대로 중복돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55`
- **범주**: 중복/복잡도
- **문제**: 진입 캐시 블록(약 10줄)과 종료 복원 블록(약 12줄)이 두 파일에 사실상 동일하게 복제돼 있고, 주석까지 "Patrol 과 동일"로 명시돼 있다. 4번을 고칠 때 두 곳을 각각 고쳐야 하고 한쪽만 고치면 조용히 갈라진다.
- **제안**: 4번의 GE 방식으로 전환하면서 공통화하거나, 최소한 WxAI 내부 공용 헬퍼로 뽑는다.
- **확신도**: 높음

### 8. 🟢 GetWeight 가 헤더에 인라인으로 정의돼 있다 (코딩 규칙 6 위반)
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:27`
- **범주**: 규칙 위반
- **문제**: `float GetWeight() const { return Weight; }` 는 클래스 본문 내 정의라 암묵적 인라인이며, CLAUDE.md 코딩 규칙 6("인라인 함수 정의를 금지한다")에 어긋난다. 모듈 전체를 스캔했을 때 이 한 곳이 유일한 위반이다.
- **제안**: 선언만 남기고 정의를 `WxBTDecorator_RandomWeight.cpp` 로 옮긴다.
- **확신도**: 높음

### 9. 🟢 ReturnHome 의 bCreateNodeInstance 는 보관할 상태가 없는데 켜져 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:20`
- **범주**: 중복/복잡도
- **문제**: "이동 속도 캐시를 폰별로 보관하기 위해 노드를 인스턴싱한다"는 주석과 함께 `bCreateNodeInstance = true` 를 켜지만, `UWxBTTask_ReturnHome` 에는 멤버 변수가 하나도 없다(`Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h:22`~`:27` 전부 함수). 과거에 있던 속도 캐시가 제거되면서 주석과 플래그만 남은 것으로 보인다. 지금은 BT 컴포넌트마다 쓸모없는 노드 인스턴스 UObject 를 하나씩 더 만들 뿐이고, 주석은 존재하지 않는 필드를 근거로 대고 있어 다음 수정자를 오도한다.
- **제안**: 플래그와 주석을 함께 제거한다(태스크는 무상태이므로 인스턴싱 없이 동작한다).
- **확신도**: 높음

### 10. 🟢 인식 해제 주석이 이미 제거된 BGMSourceComponent 를 근거로 든다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:107`
- **범주**: 중복/복잡도
- **문제**: "`SetRecognized(false)` 가 곧 `State.InCombat` 제거이며, 이 태그를 감시하는 BGMSourceComponent 가 시체 위에서 계속 재생되는 것을 막는다"고 적혀 있으나, WxSound(BGM) 플러그인은 이미 제거됐고 리포지토리 전체에서 `BGMSourceComponent` 를 참조하는 곳은 이 주석 한 줄뿐이다. 3번(타겟 사망 latch)의 심각도를 판단할 때 이 주석 때문에 소비자를 잘못 가정하게 된다 — 현재 `State.InCombat` 의 실소비자는 네임플레이트(WxUI)다.
- **제안**: 주석에서 BGM 근거를 지우고 현재 소비자(네임플레이트)로 갱신한다.
- **확신도**: 높음

### 11. 🟢 GetStaticDescription override 에서 Super:: 를 호출하지 않는다 (모듈 내 불일치)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:77`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:69`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:23`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:10`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:15`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:29`
- **범주**: 규칙 위반
- **문제**: 베이스 `UBTNode::GetStaticDescription()` 은 노드의 짧은 타입명을 돌려주는 실동작이 있는데 6개 노드가 이를 버리고 자체 문자열만 반환한다. 반면 `UWxBTTask_Patrol::GetStaticDescription`(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:68`)만 `Super::` 결과를 앞에 붙여, BT 에디터의 노드 표기가 모듈 안에서 두 가지로 갈린다.
- **제안**: Patrol 방식으로 통일해 `Super::GetStaticDescription()` 을 앞에 붙인다.
- **확신도**: 낮음 (표기를 간결하게 하려는 의도일 수 있음)

### 12. 🟢 Wander 가 매 실행마다 방향 후보 배열을 힙 할당한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:29`
- **범주**: 성능/안전
- **문제**: 최대 8개로 고정된 후보를 기본 할당자 `TArray<int32>` 에 담아 배회 진입마다 힙 할당이 발생한다. 같은 모듈의 `UWxBTComposite_RandomChoice` 는 동일 상황에서 `TInlineAllocator<8>` 을 쓴다(`Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:53`). 다수의 배회 AI 가 짧은 Duration(기본 1s)으로 반복 진입하면 누적된다.
- **제안**: `TArray<int32, TInlineAllocator<8>>` 로 맞춘다.
- **확신도**: 높음

### 13. 🟢 ActivatableAbilities 순회 중 TryActivateAbility 를 호출하면서 스코프 락을 걸지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:36`
- **범주**: 성능/안전
- **문제**: `ASC->GetActivatableAbilities()` 를 range-for 로 돌면서 루프 안에서 `TryActivateAbility` 를 호출한다. 성공 시엔 즉시 `break` 하고 이후 `IterSpec` 을 건드리지 않아 안전하지만, **실패 시엔 계속 순회한다**. `:48` 의 주석은 "실패는 ActivatableAbilities 를 바꾸지 않는다"고 단정하는데 이는 GAS 가 보장하는 계약이 아니다 — 실패 경로에서도 `NotifyAbilityFailed` 브로드캐스트 등으로 게임 코드가 실행될 수 있고, 그 안에서 `GiveAbility`/`ClearAbility` 가 일어나면 배열이 재할당·RemoveAtSwap 되어 순회 중인 참조가 무효화된다(스코프 락이 없으면 제거가 지연되지 않는다).
- **제안**: 루프를 `FScopedAbilityListLock`(ASC 내부의 `ABILITYLIST_SCOPE_LOCK` 과 같은 것) 으로 감싸 순회 중 배열 변경을 지연시킨다. 한 줄 추가로 가정을 보장으로 바꿀 수 있다.
- **확신도**: 낮음 (현재 프로젝트 어빌리티 구성에선 실제로 발생하지 않을 수 있음)

### 14. 🟢 성공한 자극마다 무조건 TargetActor 를 최신 감지 액터로 덮어쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82`
- **범주**: 설계/구조
- **문제**: `HandleTargetPerceptionUpdated` 는 성공 자극이면 우선순위·거리 비교 없이 그 액터를 타겟으로 확정한다. 감지 범위 안에 대상이 둘 이상이면(멀티플레이·소음원 포함) 센스 갱신마다 타겟이 "가장 최근 감지된 액터"로 뒤바뀌어 포커스·strafe 회전 모드가 대상 사이에서 진동한다. 특히 `UAISenseConfig_Damage` 는 Sight/Hearing 과 달리 affiliation 필터가 없어(`:28`) 아군 피해원까지 타겟이 될 수 있다.
- **제안**: 이미 유효 타겟이 있으면 유지하고, 우선순위/거리/위협도 비교를 통과할 때만 스위치하는 규칙을 둔다.
- **확신도**: 낮음 (단일 타겟을 전제한 의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/` 헤더 15개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`. 대조용으로 `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`.
- **확인 결과 문제 없던 항목**: `UWxBTTask_ActivateAbility` 의 latent 수명주기(활성화 후 핸들 재조회, 동기 종료 시 `InProgress` 영구 정지 방지, Abort 경로에서 델리게이트 선해제 후 취소 — `FinishLatentTask` 누락·중복 없음). `UWxBTComposite_RandomChoice` 의 노드 메모리 레이아웃(`FBTCompositeMemory` 상속으로 베이스 영역 침범 없음)과 룰렛 폴백. `UWxBTDecorator_BeyondLeash` 의 인스턴스 메모리는 `OnBecomeRelevant` 가 항상 선행 시드. `UWxAIPerceptionComponent` 의 사망 델리게이트 바인드/언바인드 대칭(`EndPlay` 백업 포함). `UWxPatrolComponent::GetNextIndex` 의 세 MoveMode 경계값. 권한 모델: 퍼셉션·BT·소음 보고 모두 서버 전용이고 `UWxAnimNotify_ReportNoise` 는 `HasAuthority` 게이팅, 인식은 MinimalReplication 태그로만 클라에 전파 — 권위 위반 없음. 규칙 스캔: `BlueprintCallable` 오용·람다·저작권 헤더 누락·`Handle` prefix 누락 0건, `WxAI.Build.cs`/`.uplugin` 의 Wx 의존은 `WxCore` 뿐.
- **미검토 / 한계**: BehaviorTree/Blackboard `.uasset` 자체는 보지 않았다. 따라서 README 가 규정한 "BeyondLeash 의 FlowAbortMode = Lower Priority", "복귀 브랜치가 전투 브랜치보다 상위 우선순위", "Blackboard 에셋에 5개 키가 동명·동타입 등록" 같은 에셋 측 규약이 실제로 지켜지는지는 C++ 만으로 확인 불가다(코드는 기본값과 런타임 경고로만 방어한다). 이번 세션에선 UE 5.8 엔진 소스 트리에 접근할 수 없어 엔진 내부 동작(BB Object 키의 weak 저장, `SensesConfig` 등록 시점, `GetStaticDescription` 베이스 구현 등)은 공개 API 계약 기준으로만 판단했고 해당 발견의 확신도에 반영했다. 멀티플레이 실행 검증과 리시 왕복·타겟 진동의 실제 플레이 확인도 정적 분석 범위 밖이다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 29파일 — `/module-review`로 갱신*
