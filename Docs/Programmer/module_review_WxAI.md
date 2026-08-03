# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹힌 구조가 잘 지켜져 있고, 알려진 함정(GAS Spec 배열 재할당, latent 태스크 동기 종료, Composite 노드 메모리 레이아웃, 델리게이트 바인드/언바인드 대칭)은 이미 방어돼 있다. 프로젝트 규칙 위반은 0건이다. 남은 문제는 "파생 상태를 발행해 두고 무효화 경로가 없는" 패턴(회전 모드, `State.InCombat`, `MaxWalkSpeed`)과, 조건 Decorator의 실시간 재평가가 문서 안내와 어긋나는 지점에 몰려 있다. 커버리지: 소스 29파일 전부를 읽었고 `WxAIPerceptionComponent`·`WxBTComposite_RandomChoice`·`WxBTTask_ActivateAbility`·MoveTo 파생 태스크·리시 한 쌍은 cpp 로직까지 파고들었으며, UE 5.8 `AIModule`/`GameplayAbilities` 원본과 소비자(`AWxEnemyController`, `AWxCharacterBase`)에 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 avoid-repeat가 "지금 유효한 유일한 자식"까지 제외해 컴포지트를 실패시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:61`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:91`
- **범주**: 버그/정확성
- **문제**: `bAvoidRepeat`가 켜져 있으면 직전 선택 자식을 조건 검사 **전에** 무조건 `continue`로 제외한다(`:61`). 자식이 2개이고 직전에 child 0을 골랐는데 이번 진입에서 child 1의 조건 Decorator(예: `UWxBTDecorator_AttributeRatio`로 건 HP<0.5 발악기)가 false라면, child 1은 조건 필터로 빠지고 child 0은 avoid-repeat로 빠져 `Candidates.Num() == 0` → `ReturnToParent`(실패)가 된다(`:91`). child 0은 지금도 실행 가능한데 "직전에 썼다"는 이유만으로 공격 패턴 분기 전체를 포기하고 상위 Selector가 하위 브랜치(추격·배회 등)로 넘어간다. 자식 수가 적은 보스 패턴일수록 자주 걸린다.
- **제안**: avoid 때문에 후보가 비면 avoid를 한 단계 완화해 직전 자식을 다시 포함하는 폴백을 둔다. 엄격 avoid가 의도라면 이 실패 거동을 헤더 시멘틱 주석에 명시한다.
- **확신도**: 중간 (상위 Selector 폴백에 의존하는 의도된 설계일 수 있음)

### 2. 🟡 RandomChoice가 걸러낸 자식의 조건 Decorator를 옵저버로 등록하지 않아 FlowAbortMode가 무력화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:68`
- **범주**: 버그/정확성
- **문제**: 엔진 `UBTCompositeNode::FindChildToExecute`는 조건이 막은 자식마다 `NotifyDecoratorsOnFailedActivation`을 호출하고, 그 안에서 `LowerPriority`/`Both` FlowAbortMode를 가진 Decorator를 aux 노드(옵저버)로 등록한다. `GetNextChildHandler`의 사전 필터는 조건이 false인 자식을 `continue`로 건너뛸 뿐이라 이 등록이 전혀 일어나지 않는다. 결과: RandomChoice 아래 자식에 조건 데코를 달고 FlowAbortMode를 LowerPriority로 지정해도, 조건이 참으로 뒤집히는 순간 진행 중인 하위 패턴을 끊고 들어오지 못한다. 평범한 Selector였다면 동작했을 "조건 충족 즉시 선점" 구성이 이 컴포지트 아래에서만 조용히 죽는다. 7번(AttributeRatio가 재평가를 촉발하지 않음)과 짝을 이루는 문제로, 둘 다 고쳐야 그 구성이 성립한다.
- **제안**: 필터에서 제외한 인덱스마다 `NotifyDecoratorsOnFailedActivation(SearchData, Index, LastResult)`(`UBTCompositeNode`의 protected 멤버라 파생 클래스에서 호출 가능)를 불러준다. 그럴 의사가 없다면 헤더에 "RandomChoice 자식의 조건 데코는 FlowAbortMode가 동작하지 않는다"를 규약으로 못박는다.
- **확신도**: 중간

### 3. 🟡 SetTargetActor의 조기 반환이 회전 모드를 strafe에 고착시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:215`
- **범주**: 버그/정확성
- **문제**: `SetTargetActor`는 "BB의 현재 TargetActor == NewTarget"이면 즉시 반환하고, 뒤에 있는 파생 상태 복원(`AIC->ClearFocus`, `bUseControllerDesiredRotation=false`, `bOrientRotationToMovement=true` — 같은 파일 `:245`~`:250`)에 도달하지 못한다. 그런데 Blackboard의 Object 키는 엔진이 `FWeakObjectPtr`로 저장하므로(`UBlackboardKeyType_Object::GetValue`), 타겟 액터가 파괴되면 BB 값이 컴포넌트 모르게 스스로 nullptr이 된다. 이 상태에서 뒤늦게 `SetTargetActor(nullptr)`(억제 진입 `:160`, 사망 정리 `:203`)이 호출되면 "이미 nullptr"이라 조기 반환하고, 폰은 strafe 회전 모드에 갇힌다. 포커스 대상도 이미 소실돼 `AAIController::UpdateControlRotation`이 갱신을 멈추므로, 이후 정찰·복귀 이동에서 진행 방향을 보지 않고 미끄러지듯 이동한다. 새 타겟을 잡기 전까지 자력으로 복구되지 않는다.
- **제안**: "마지막으로 적용한 타겟"을 BB가 아니라 컴포넌트 자체 필드(`TWeakObjectPtr<AActor>`)로 들고 그것과 비교한다. BB 쓰기와 회전 모드 발행의 판단 기준을 분리하면 BB의 자체 무효화와 무관해진다.
- **확신도**: 중간

### 4. 🟡 타겟의 사망을 관측하는 경로가 없어 State.InCombat이 시체에 latch된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:88`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:91`
- **범주**: 설계/구조
- **문제**: `UpdateRecognition`은 오직 `HandleTargetPerceptionUpdated`에서만 호출된다(`:88`). 즉 인식 상태는 "퍼셉션 자극이 새로 올 때"만 재판정된다. 그런데 사망한 캐릭터는 파괴되지 않고(리포지토리 전체에 사망 시 `Destroy`/`SetLifeSpan` 호출 없음) 시야 안에 그대로 남아 새 자극을 만들지 않는다. 결과적으로 BB TargetActor는 시체를 가리킨 채 유지되고 `State.InCombat`도 켜진 채 남아, 복제 태그를 소비하는 네임플레이트가 계속 전투 표시를 하고 BT 전투 브랜치도 시체를 계속 공격한다. 자기 자신의 사망은 `BindOwnerDeath`/`HandleDeathTagChanged`(`:165`, `:194`)로 정확히 이 문제를 막고 있는데 타겟 쪽 대칭 처리만 비어 있다. 해제되는 유일한 경로는 리시 이탈 → `WxBTTask_ReturnHome`의 억제뿐이라, 홈 근처에서 죽인 경우엔 그 경로도 타지 않는다.
- **제안**: `SetTargetActor`에서 새 타겟 ASC의 `State.Dead` 태그 이벤트를 구독/해제해(자기 폰용 `BindOwnerDeath`와 같은 패턴) 사망 시 타겟을 비운다. 또는 BT 전투 브랜치 진입을 타겟 생존 Decorator로 게이팅한다.
- **확신도**: 중간 (BT 에셋 측에 생존 게이트가 이미 있을 수 있음 — C++만으로는 확인 불가)

### 5. 🟡 MaxWalkSpeed 저장·복원이 SPD 어트리뷰트의 소유권과 충돌한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:61`
- **범주**: 설계/구조
- **문제**: 두 태스크는 진입 시 `MaxWalkSpeed`를 캐시해 배율을 곱하고, 종료 시 캐시한 **절대값**으로 되돌린다(`WxBTTask_Patrol.cpp:85`, `WxBTTask_Wander.cpp:110`). 그러나 같은 필드를 `AWxCharacterBase::HandleSPDAttributeChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:238`)가 `MaxWalkSpeed = BaseWalkSpeed * SPD`로 어트리뷰트 변경마다 절대값 재계산한다. 소유자가 둘이라 (a) 정찰 중 SPD가 바뀌면 정찰 감속 배율이 통째로 사라지고, (b) 정찰 중 걸린 버프/디버프가 태스크 종료 시 "정찰 진입 시점"의 낡은 절대값으로 덮어써져 다음 SPD 이벤트까지 무효화된다. 예: Base 400 / SPD 1 → 정찰 진입(캐시 400, 실제 200) → 가속 버프 SPD 2(실제 800) → 정찰 종료 시 400으로 복원되어 버프가 남은 시간 동안 사라진다.
- **제안**: 감속을 CMC 필드 직접 쓰기가 아니라 SPD를 낮추는 GE(태스크 진입 시 부여·종료 시 제거)로 적용해 소유권을 한쪽으로 모은다. 최소 대응은 절대값 대신 배율을 되돌리는 것이지만, 배율 0을 허용하는 현 Clamp(`Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h:32`) 때문에 나눗셈 복원엔 별도 가드가 필요하다.
- **확신도**: 높음(메커니즘) / 중간(정찰·배회 중 SPD 변동 빈도에 따라 체감이 갈린다)

### 6. 🟡 Sight/Hearing 센스가 ApplySenseSettings 호출에만 의존해 등록된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:38`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:59`
- **범주**: 설계/구조
- **문제**: 생성자는 Sight/Hearing/Damage 세 config를 만들지만 `ConfigureSense`는 `PostInitProperties`에서 Damage에만 호출한다(`:40`). Sight·Hearing은 `ApplySenseSettings` 안에서야 `ConfigureSense`된다(`:59`, `:65`). 엔진 `UAIPerceptionComponent::OnRegister`는 `SensesConfig` 배열에 들어 있는 센스만 퍼셉션 시스템에 리스너로 올리므로, `ApplySenseSettings`가 호출되기 전까지 이 컴포넌트는 시각·청각이 전혀 없다. 유일한 호출부인 `AWxEnemyController::OnPossess`는 폰이 `AWxEnemyCharacter`로 캐스팅될 때만 호출한다(`Source/WxGame/Controller/WxEnemyController.cpp:30`). 즉 다른 폰 타입에 붙이면 경고 한 줄 없이 "피해만 감지하는" AI가 된다. `WxBlackboardKeys`가 키 오용을 진단 로그로 드러내는 것과 대비되는 비대칭이다.
- **제안**: `PostInitProperties`에서 Damage와 함께 Sight/Hearing도 기본값으로 `ConfigureSense`해 두고, `ApplySenseSettings`는 값 갱신 + `RequestStimuliListenerUpdate`만 하게 한다.
- **확신도**: 중간 (현 사용처에선 항상 호출되므로 잠재 결함)

### 7. 🟡 AttributeRatio는 FlowAbortMode를 켜도 실시간 재평가가 일어나지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16`
- **범주**: 설계/구조
- **문제**: 헤더가 "실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode를 LowerPriority/Self/Both로 설정한다"고 안내하지만, 이 Decorator는 `CalculateRawConditionValue`만 구현하고 `OnBecomeRelevant`/`TickNode`/어트리뷰트 변경 구독이 전혀 없다(cpp 전체가 생성자·`GetStaticDescription`·조건 계산 3개뿐). FlowAbortMode는 "abort를 허용하는 범위"만 정할 뿐 재평가를 촉발하지 않으므로, HP가 임계값을 넘어도 다른 원인으로 BT 재탐색이 일어나기 전까지 조건은 갱신되지 않는다. 같은 모듈의 `UWxBTDecorator_BeyondLeash`는 정확히 이 한계 때문에 `TickNode` 폴링 + `RequestExecution`을 직접 구현했다(`Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:51`). 디자이너가 헤더 안내대로 FlowAbortMode만 켜고 "HP 50%가 되면 즉시 발악기"를 기대하면 조용히 어긋난다.
- **제안**: BeyondLeash처럼 관찰자 경로를 구현한다 — `OnBecomeRelevant`에서 대상 ASC의 두 어트리뷰트 변경 델리게이트를 구독해 비율 판정이 뒤집힐 때만 `RequestExecution`을 호출하고 `OnCeaseRelevant`에서 해제한다(틱 폴링보다 저렴하다). 구현할 계획이 없다면 헤더 문구를 "재탐색 시점에만 재평가된다"로 정정한다.
- **확신도**: 중간

### 8. 🟢 ReturnHome의 bCreateNodeInstance와 주석이 실체와 어긋난다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:20`, `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h:14`
- **범주**: 중복/복잡도
- **문제**: "이동 속도 캐시를 폰별로 보관하기 위해 노드를 인스턴싱한다"는 주석과 함께 `bCreateNodeInstance = true`를 켜지만, `UWxBTTask_ReturnHome`에는 멤버 변수가 하나도 없다(헤더 `:22`~`:27` 전부 함수). 과거 속도 캐시가 제거되면서 주석과 플래그만 남은 것으로 보이며, 지금은 BT 컴포넌트마다 쓸모없는 노드 인스턴스 UObject를 하나 더 만들 뿐이다. 헤더 주석의 "현재 타겟/**마지막 인지 위치**를 비우고"도 실제 `SetTargetingSuppressed` 동작(`WxAIPerceptionComponent.cpp:147`~`:163`)에 없는 내용이다. 이 모듈은 주석을 사실상 규약 문서로 쓰고 있어 드리프트의 해악이 크다.
- **제안**: 플래그와 주석을 함께 제거하고(태스크는 무상태이므로 인스턴싱 없이 동작한다), 헤더 문구에서 "마지막 인지 위치"를 뺀다.
- **확신도**: 높음

### 9. 🟢 인식 해제 주석이 이미 제거된 BGMSourceComponent를 근거로 든다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:107`
- **범주**: 중복/복잡도
- **문제**: "`SetRecognized(false)`가 곧 `State.InCombat` 제거이며, 이 태그를 감시하는 BGMSourceComponent가 시체 위에서 계속 재생되는 것을 막는다"고 적혀 있으나, 리포지토리 전체에서 `BGMSourceComponent`를 언급하는 곳은 이 주석 한 줄뿐이다(플러그인은 제거됨). 4번의 심각도를 판단할 때 이 주석 때문에 소비자를 잘못 가정하게 된다 — 현재 `State.InCombat`의 실소비자는 네임플레이트(WxUI)다.
- **제안**: 주석에서 BGM 근거를 지우고 현재 소비자로 갱신한다.
- **확신도**: 높음

### 10. 🟢 Wander가 매 실행마다 방향 후보 배열을 힙 할당한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:29`
- **범주**: 성능/안전
- **문제**: 최대 8개로 고정된 후보를 기본 할당자 `TArray<int32>`에 담아 배회 진입마다 힙 할당이 발생한다. 같은 모듈의 `UWxBTComposite_RandomChoice`는 동일 상황에서 `TInlineAllocator<8>`을 쓴다(`Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:53`). 다수의 배회 AI가 짧은 Duration(기본 1s)으로 반복 진입하면 누적된다.
- **제안**: `TArray<int32, TInlineAllocator<8>>`로 맞춘다.
- **확신도**: 높음

### 11. 🟢 리시 데코가 평상시 매 프레임 폴링하며 그때마다 BB 키 이름 조회 진단을 돈다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:51`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:17`
- **범주**: 성능/안전
- **문제**: FlowAbortMode가 LowerPriority면 엔진은 브랜치가 **비활성일 때**(= 리시 안, 즉 대부분의 시간) 이 데코를 aux 노드로 등록하므로 `TickNode`가 매 프레임 돈다. 거리 계산 자체는 `DistSquared` 한 번이라 싸지만, 그 안의 `GetHomeLocation`이 호출마다 `VerifyBlackboardKey`(→ `GetKeyID` 이름 선형 탐색 + `GetKeyType`)를 수행한다. AI 수 × 프레임에 비례하며 Shipping에서만 사라진다. `HomeLocation`은 `OnPossess`에서 한 번 쓰이고 이후 바뀌지 않는 값이라 매 프레임 다시 읽을 이유도 없다.
- **제안**: `OnBecomeRelevant`에서 `HomeLocation`을 노드 메모리에 1회 캐시하고 틱에서는 거리만 계산한다. 폴링 간격(누적 DeltaSeconds)을 두는 것도 함께 검토.
- **확신도**: 중간

### 12. 🟢 ActivatableAbilities 순회 중 TryActivateAbility를 호출하면서 스코프 락을 걸지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:36`
- **범주**: 성능/안전
- **문제**: `ASC->GetActivatableAbilities()`를 range-for로 돌면서 루프 안에서 `TryActivateAbility`를 호출한다. 성공 시엔 즉시 `break`하고 이후 `IterSpec`을 건드리지 않아 안전하지만, **실패 시엔 계속 순회한다**. `:48`의 주석은 "실패는 ActivatableAbilities를 바꾸지 않는다"고 단정하는데 이는 GAS가 보장하는 계약이 아니다 — 실패 경로에서도 `NotifyAbilityFailed` 브로드캐스트로 게임 코드가 실행될 수 있고, 그 안에서 `GiveAbility`/`ClearAbility`가 일어나면 락이 없는 동안 배열이 재할당·`RemoveAtSwap`되어 순회 중인 참조가 무효화된다.
- **제안**: 루프를 `FScopedAbilityListLock`(UE 5.8 `GameplayAbilitySpec.h`의 공개 타입, `FScopedAbilityListLock Lock(*ASC);`)으로 감싸 순회 중 배열 변경을 지연시킨다. 한 줄 추가로 가정을 보장으로 바꿀 수 있다.
- **확신도**: 낮음 (현재 어빌리티 구성에선 실제로 발생하지 않을 수 있음)

### 13. 🟢 성공한 자극마다 무조건 TargetActor를 최신 감지 액터로 덮어쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82`
- **범주**: 설계/구조
- **문제**: `HandleTargetPerceptionUpdated`는 성공 자극이면 우선순위·거리 비교 없이 그 액터를 타겟으로 확정한다. 감지 범위 안에 대상이 둘 이상이면(멀티플레이·소음원 포함) 센스 갱신마다 타겟이 "가장 최근 감지된 액터"로 뒤바뀌어 포커스·strafe 회전 모드가 대상 사이에서 진동한다. 특히 `UAISenseConfig_Damage`는 Sight/Hearing과 달리 `DetectionByAffiliation` 자체가 없는 클래스라(엔진 확인) 아군 피해원도 그대로 타겟이 된다 — 생성자에서 Damage만 affiliation 설정 없이 만드는 것(`:28`)은 실수가 아니라 엔진 제약이지만, 그만큼 필터링을 코드가 떠안아야 한다.
- **제안**: 이미 유효 타겟이 있으면 유지하고, 우선순위/거리/팀 판정을 통과할 때만 스위치하는 규칙을 둔다.
- **확신도**: 낮음 (단일 타겟을 전제한 의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp` (+ 대응 헤더)
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/` 헤더 15개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`. 대조용으로 `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`.
- **확인 결과 문제 없던 항목**: `UWxBTTask_ActivateAbility`의 latent 수명주기(활성화 후 핸들 재조회, 동기 종료 시 `InProgress` 영구 정지 방지, Abort 경로에서 델리게이트 선해제 후 취소 — `FinishLatentTask` 누락·중복 없음). `UWxBTComposite_RandomChoice`의 노드 메모리 레이아웃(`FBTCompositeMemory` 상속 + `GetInstanceMemorySize` 일치로 베이스 영역 침범 없음)과 룰렛 부동소수 폴백. `UWxBTDecorator_BeyondLeash`의 인스턴스 메모리는 `OnBecomeRelevant`가 항상 선행 시드하며, LowerPriority 강제는 엔진의 aux 등록 규칙과 정확히 맞물린다. `UWxAIPerceptionComponent`의 사망 델리게이트 바인드/언바인드 대칭(`EndPlay` 백업 포함). `UWxPatrolComponent::GetNextIndex`의 세 MoveMode 경계값과 `GetPointLocation`의 엔진 측 인덱스 클램프. 권한 모델: 퍼셉션·BT·소음 보고 모두 서버 전용이고 `UWxAnimNotify_ReportNoise`는 `HasAuthority` 게이팅, 인식은 MinimalReplication 태그로만 클라에 전파 — 권위 위반 없음.
- **규칙 준수 확인(위반 0건)**: 29개 소스 전부 첫 줄 저작권 표기 정상 / `Wx` prefix 전면 준수 / `BlueprintCallable` 0건 / `FORCEINLINE`·헤더 인라인 정의 0건(직전 리뷰에서 지적된 `UWxBTDecorator_RandomWeight::GetWeight` 인라인 정의는 cpp로 이관되어 해소됨) / 람다 0건 / 델리게이트 콜백 3종(`HandleTargetPerceptionUpdated`, `HandleDeathTagChanged`, `HandleAbilityEnded`) 모두 `Handle` prefix / Wx 플러그인 의존은 `WxCore` 하나(`.Build.cs`·`.uplugin` 동일)
- **미검토 / 한계**:
  - BehaviorTree/Blackboard `.uasset` 자체는 보지 않았다. README가 규정한 "BeyondLeash의 FlowAbortMode = Lower Priority", "복귀 브랜치가 전투 브랜치보다 상위 우선순위", "Blackboard 에셋에 5개 키가 동명·동타입 등록" 같은 에셋 측 규약이 실제로 지켜지는지는 C++만으로 확인 불가다. 특히 1·2·7번의 실제 체감 크기는 현재 BT가 조건 데코에 FlowAbortMode를 걸어 쓰는지에 달려 있다.
  - `WxBTTask_Patrol`/`WxBTTask_Wander`의 속도 캐시·복원 블록은 사실상 동일한 코드가 두 파일에 있으나, 프로젝트가 구조적 추출보다 인플레이스 반복을 선호하므로 중복으로 지적하지 않았다(단 5번을 고칠 때 두 곳을 함께 고쳐야 한다).
  - `NavigationSystem`·`GameplayTasks` 모듈 의존은 WxAI 소스에서 직접 쓰이지 않으나 `AIModule`의 전이 의존이라 무해해 지적하지 않았다.
  - 멀티플레이 실환경(전용 서버) 실행 검증, 리시 왕복·타겟 진동의 실제 플레이 확인은 정적 분석 범위 밖이다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 29파일 — `/module-review`로 갱신*
