# WxAI — 코드 리뷰

> 규칙 면은 여전히 깨끗하다 — 플러그인 의존은 `WxCore` 뿐이고, 29파일 어디에도 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix 누락이나 `BlueprintCallable`·`FORCEINLINE`·불필요한 람다가 없으며, 크래시급 결함도 없다. 남은 위험은 "엔진이 알아서 해 주는 줄 알았던 지점"에 몰려 있고, 이번 커밋에서 촉각 보고가 `UWxAIPerceptionComponent`로 들어오면서 그중 하나(무필터 Damage 자극)의 발생 빈도가 크게 올라갔다. 이번 리뷰는 소스 29개(cpp 14 + h 15)를 모두 열고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite 의 cpp 를 깊게 봤으며, 촉각 경로는 `WxCombat` 의 대미지 파이프라인(`WxCombatAttributeSet::ProcessDamageTaken`, `WxCombatLibrary::ApplyDamage`, `AWxWeaponBase::ProcessHit`)까지 따라가 실제 페이로드와 대조했다. 이 샌드박스에는 엔진 소스가 없어, 엔진 내부 동작에 기대는 발견은 확신도에 그 사실을 반영했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 촉각 자극에 적대 판정이 없어 적끼리 서로를 타겟으로 잡는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:35`, `:237-256`, `:96-99`, `:159-167`
- **범주**: 버그/정확성
- **문제**: Sight(`:20-22`)와 Hearing(`:30-32`)은 `DetectionByAffiliation` 으로 적만 감지하도록 막아 두었지만, `UAISenseConfig_Damage` 에는 애초에 그 필드가 없어 `DamageConfig`(`:35`)는 **가해자가 누구든** 자극을 통과시킨다. 그리고 채택 지점인 `HandleTargetPerceptionUpdated` 는 성공 자극이면 사망 여부만 확인하고 무조건 `SetTargetActor(Actor)` 를 부른다(`:96-99`). 이번 커밋으로 `HandlePawnHit` 가 폰이 받은 모든 `Event.Hit`(magnitude > 0)을 `UAISense_Damage::ReportDamageEvent` 로 그대로 실어 보내면서(`:255`) 이 구멍을 지나는 트래픽이 대폭 늘었다. 대미지 파이프라인 어디에도 피아 필터가 없다는 점이 결정적이다 — `AWxWeaponBase::ProcessHit` 는 무기 소유자 본인만 제외하고(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:255-268`), `UWxCombatLibrary::ApplyDamage` 도 팀을 보지 않는다(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:12-60`). 결과적으로 **적 A의 스윙 히트박스가 옆의 적 B에 스치면 B가 A를 TargetActor 로 확정하고 포커스·strafe·`State.InCombat` 까지 켠 채 아군을 쫓는다.** 밀집 스폰이나 광역 공격이 있는 구간에서 재현된다.
  같은 자리에 채택 규칙 자체가 없다는 문제도 겹친다 — 현재 타겟이 멀쩡해도 최근 자극이 무조건 이기므로, 적대 대상이 둘 이상이면 피격이 들어올 때마다 타겟이 오간다. 억제 해제 시의 재획득 루프(`:159-167`)도 같은 구멍을 공유하며, `FActorPerceptionContainer`(TMap) 순회 순서로 "첫 항목"을 집어 어느 액터가 뽑힐지 결정적이지 않다.
- **제안**: 채택 경로에 적대 게이트를 하나 세운다. `AWxCharacterBase` 가 이미 `IGenericTeamAgentInterface` 를 구현하므로(`Source/WxGame/Character/WxCharacterBase.cpp:170-182`), `HandlePawnHit` 에서 `FGenericTeamId::GetAttitude(Pawn, DamageInstigator) == ETeamAttitude::Hostile` 이 아니면 보고를 생략하는 것이 가장 싸다(자기 자신이 가해자인 경우도 함께 걸린다). 자극 콜백·재획득 루프 쪽까지 닫으려면 `HandleTargetPerceptionUpdated` 와 `:159-167` 에도 같은 판정을 넣고, "현재 타겟이 유효하면 유지" 규칙을 함께 두어 타겟 진동을 없앤다.
- **확신도**: 높음(메커니즘) / 중간(적 간 오사 허용이 의도된 설계일 수 있음)

### 2. 🟡 복귀 이동의 성패를 알기 전에 타겟을 비운다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-49`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask` 가 `Super::ExecuteTask`(MoveTo)의 결과를 받기 **전에** `SetTargetingSuppressed(true)` 를 호출한다(`:32`). `UBTTask_MoveTo::ExecuteTask` 는 경로가 없거나 목표가 내비메시 밖이면 동기 `Failed` 를 반환하는데, 그 헛발질 한 번이 이미 `SetTargetActor(nullptr)` → 포커스 해제 → CMC 회전 모드 원복 → `State.InCombat` 제거까지 끝낸 뒤다. 동기 결과에도 `OnTaskFinished` 가 곧바로 불려 억제는 즉시 풀리지만, 해제 시 재획득 루프(`WxAIPerceptionComponent.cpp:159-167`)는 **지금 감지 중인** 액터만 집는다. 이 모듈의 설계 전제는 "한 번 확보한 타겟은 시야를 잠시 잃어도 유지한다"(`WxAIPerceptionComponent.h:29`)이므로, 벽 뒤·등 뒤로 빠진 타겟은 이 한 번의 실패로 영구히 사라진다. 감지 중이어서 되살아나는 경우에도 실패할 때마다 포커스·회전 모드·복제 태그가 껐다 켜져 네임플레이트가 깜빡인다.
- **제안**: `Super::ExecuteTask` 결과를 먼저 받아 `InProgress` 일 때만 억제를 켠다. 다른 결과에서는 억제를 아예 건드리지 않는다.
- **확신도**: 높음(메커니즘) / 중간(HomeLocation 경로 실패 빈도에 달림)

### 3. 🟡 Patrol 의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:46-49`, `:63`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask` 는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation` 에 목표를 쓰지만(`:48`), 실제 이동은 `Super::ExecuteTask`(`:63`)가 `UBTTask_BlackboardBase::BlackboardKey` 를 읽어 수행한다. 그 필드는 엔진에서 `EditAnywhere` 이고 생성자 `:19` 는 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 키를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation` 에 쓰고 엉뚱한 키(대개 미설정)로 이동을 시도한다. 경고 하나 없이 정찰이 실패하거나 폰이 엉뚱한 곳으로 걸어가는 형태로만 드러난다. `UWxBTTask_ReturnHome` 은 읽기만 하므로 같은 위험이 없다.
- **제안**: 쓰기도 `BlackboardKey.SelectedKeyName` 을 경유해 읽는 키와 통일하거나, 반대로 디테일 패널에서 `BlackboardKey` 편집을 잠근다.
- **확신도**: 높음(메커니즘) / 중간(실제 에셋 설정에 달림)

### 4. 🟡 ActivateAbility: 재발동 경로에서 태스크가 어빌리티보다 먼저 끝난다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:53-59`, `:66-69`, `:110-117`
- **범주**: 버그/정확성
- **문제**: `ActivatedHandle` 을 `TryActivateAbility` **호출 전에** 세워 두고(`:54`), `HandleAbilityEnded` 는 스펙 핸들만으로 "내 실행의 종료"를 판별한다(`:112`). 그런데 엔진은 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성인 채로 재발동되면 기존 인스턴스를 `EndAbility(bWasCancelled=false)` 로 먼저 끝낸 뒤 재활성화한다. 이때 같은 핸들로 콜백이 들어와 `CleanUp()`(`:117`)이 구독을 끊고 `ActivatedHandle` 을 비우며 `ActivationResult = Succeeded` 가 채워진다. 이어 재활성화가 성공하면 `:66-69` 가 그 낡은 결과를 그대로 반환해 태스크를 즉시 종료한다 — 어빌리티는 계속 도는데 BT 는 다음 행동으로 넘어가고(공격 모션 위에 이동·배회가 겹침), `ActivatedHandle` 이 비어 있어 `AbortTask` 가 취소할 수도 없다.
- **제안**: `bIsActivating` 구간의 콜백은 결과만 기록하고 `CleanUp()` 은 `ExecuteTask` 반환 직전으로 미룬다. 또는 `:66-69` 의 `ActivationResult` 신뢰를 `:79-84` 의 "핸들 유효 + 스펙 활성" 확인 뒤로 미뤄, 실제로 도는 어빌리티가 있으면 InProgress 를 유지하게 한다.
- **확신도**: 중간(`bRetriggerInstancedAbility` 는 기본 false 라 디자이너가 켠 어빌리티에서만 발현하며, 이 환경에 엔진 소스가 없어 재확인은 못 했다)

### 5. 🟡 AttributeRatio 에는 재평가 트리거가 없어 FlowAbortMode 설정이 무효다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-13`, `:35-78`
- **범주**: 설계/구조
- **문제**: 이 데코레이터는 `CalculateRawConditionValue` 만 구현하고, 값 변화를 관찰할 장치(어트리뷰트 변경 델리게이트 구독이나 `TickNode` 폴링)가 없다. 관찰자 abort 는 노드가 스스로 `RequestExecution` 을 부를 때만 일어나므로, 디자이너가 디테일 패널에서 FlowAbortMode 를 지정해도 **아무 일도 일어나지 않는다** — "HP 가 30% 아래로 떨어지면 즉시 광폭화 브랜치로 전환" 같은 의도가 조용히 무시되고 다음 재탐색까지 미뤄진다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 정확히 이 문제 때문에 `TickNode` 폴링을 두었다(`WxBTDecorator_BeyondLeash.cpp:49-61`).
- **제안**: BeyondLeash 와 같은 폴링을 넣거나(`INIT_DECORATOR_NODE_NOTIFY_FLAGS()` + `TickNode` 에서 값 전이 시 `RequestExecution`), ASC 의 `GetGameplayAttributeValueChangeDelegate` 를 구독한다. 지원하지 않기로 한다면 생성자에서 `bAllowAbortLowerPri = false; bAllowAbortChildNodes = false;` 로 드롭다운을 잠가 오해를 없앤다.
- **확신도**: 중간

### 6. 🟡 RandomChoice 의 사전 필터가 형제 데코레이터의 abort 관찰자 등록을 건너뛴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-63`
- **범주**: 설계/구조
- **문제**: 후보 수집 루프가 `DoDecoratorsAllowExecution` 으로 조건 실패 자식을 **직접** 걸러내고(`:60`) 통과한 자식 인덱스만 반환한다. 그런데 엔진에서 "조건 실패 자식"의 처리는 `UBTCompositeNode::FindChildToExecute` 의 else 분기가 담당하며, 거기서 부르는 `NotifyDecoratorsOnFailedActivation` 이 `FlowAbortMode == LowerPriority | Both` 인 데코레이터를 aux 노드로 등록한다. 사전 필터로 걸러진 자식은 이 경로를 밟지 않으므로, RandomChoice **안에 있는** 형제의 LowerPriority 데코레이터(`UWxBTDecorator_BeyondLeash` 나 엔진 Blackboard 관찰자 등)는 관찰자로 등록되지 않고 실행 중인 자식을 선점하지 못한다. 같은 이유로 조건 데코레이터는 선택된 자식에 대해 두 번 평가된다.
- **제안**: RandomChoice 하위에 관찰자 abort 데코레이터를 두지 않는다는 제약을 클래스 주석과 `GetStaticDescription` 에 명시하거나, 필터에서 걸러낸 자식마다 `NotifyDecoratorsOnFailedActivation` 을 직접 호출해 엔진 규약을 복원한다.
- **확신도**: 중간(공격 패턴 추첨 용도로만 쓴다면 무해하지만, 규약 이탈이라 확장 시 조용히 깨진다)

### 7. 🟡 Once 정찰 완료 후 매 BT 틱 즉시 Succeeded 를 반환해 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:41-44`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished` 가 서면 `ExecuteTask` 가 이동도 지연도 없이 동기 `Succeeded` 를 반환한다. 즉시 끝난 태스크는 곧바로 `OnTaskFinished` → 실행 갱신을 부르므로, 이 폰은 살아 있는 내내 **BT 틱마다 트리 검색**을 반복한다. 형제 데코레이터 평가 비용이 전부 따라붙고, `OnTaskFinished` 도 매번 `FindPatrolComponent`(Owner/AttachParent 의 컴포넌트 선형 스캔)와 `GetNextIndex` 를 다시 돈다(`:93-107`). 프레임 내 무한 루프는 아니지만 Once 정찰 적이 많은 맵에서는 그대로 누적된다.
- **제안**: 완료 상태에서는 `InProgress` 를 반환해 실제로 브랜치를 점유하게 한다. 주석이 말하는 "그 자리에 머문다"의 정확한 표현이며, 상위 우선순위 abort 는 그대로 동작한다.
- **확신도**: 중간

### 8. 🟢 BeyondLeash 의 "Self/Both 금지"가 주석으로만 있고 에디터에서 막히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-19`, 경고 주석 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:21-22`
- **범주**: 설계/구조
- **문제**: 헤더는 Self/Both 를 고르면 "복귀가 경계에서 끊기고 경계 왕복이 난다"고 명시하는데, 생성자는 `FlowAbortMode` 에 기본값만 넣을 뿐(`:18`) 잘못된 선택지를 막지 않는다. 엔진은 바로 이 목적의 플래그(`bAllowAbortNone`·`bAllowAbortLowerPri`·`bAllowAbortChildNodes`)를 제공하고 디테일 패널이 그것으로 드롭다운을 제한하는데 쓰지 않았다. 잘못 고르면 재현·진단이 어려운 왕복 버그로만 드러난다.
- **제안**: 생성자에서 `bAllowAbortNone = false; bAllowAbortChildNodes = false;` 를 세워 LowerPriority 외 선택 자체를 없앤다.
- **확신도**: 높음(메커니즘) / 중간(의도적으로 여지를 남긴 것일 수 있음)

### 9. 🟢 BeyondLeash 가 HomeLocation 미설정을 "이탈"로 오판한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:36-37`
- **범주**: 버그/정확성
- **문제**: Blackboard Vector 키의 미설정 값은 `FAISystem::InvalidLocation`(매우 큰 값)이라, `HomeLocation` 이 비어 있으면 `DistSquared` 가 천문학적 값이 되어 항상 `true`(이탈)로 판정된다. 그 결과 AI 는 경고 하나 없이 영구 복귀 모드에 갇힌다. 현재는 `Source/WxGame/Controller/WxEnemyController.cpp` 가 `OnPossess` 에서 항상 채워 주므로 발현하지 않지만, 다른 컨트롤러나 스폰 경로가 생기면 진단이 어려운 형태로 터진다.
- **제안**: `FAISystem::IsValidLocation(Home)` 으로 가드하고, 무효면 `false` 를 반환하며 `LogWxAI` 경고를 남긴다(`WxBlackboardKeys.cpp:15-33` 의 키 검증 진단과 같은 결).
- **확신도**: 중간

### 10. 🟢 `EWxTeam` 이 WxAI 에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:121`, `WxCharacterBase.cpp:170`·`:182`, `WxEnemyCharacter.cpp:18`, `WxPlayerCharacter.cpp:22` 뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다. 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인 플러그인(예: 발견 1의 피아 필터를 WxCombat 쪽에 두는 경우)이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h` 를 `WxCore` 로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 11. 🟢 Patrol 과 Wander 의 감속 GE 부여·제거 코드가 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-61`·`:80-85`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55-64`·`:106-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` → `OnTaskFinished` 에서 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다(헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트와 `GetStaticDescription` 의 미지정 분기도 동일). SetByCaller 태그나 핸들 수명 규약을 바꾸면 두 곳을 함께 고쳐야 한다.
- **제안**: 두 태스크의 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode` 라 공통 베이스를 만들 수 없으므로, "부여/해제" 두 함수만 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`·`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`·`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`·`Source/WxGame/Character/WxEnemyCharacter.cpp`
- **미검토 / 한계**:
  - 이 샌드박스에는 언리얼 엔진 소스가 없다. 발견 2·4·5·6·7·9 는 엔진 내부 동작(동기 `Failed` 반환, `bRetriggerInstancedAbility` 재발동, 관찰자 등록 경로, 즉시 종료 태스크의 재탐색, `FAISystem::InvalidLocation`)에 기대므로 라인 단위 대조를 하지 못했다. UE 5.8 소스가 있는 환경에서 재확인이 필요하다.
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 3·5·6·8 은 "디자이너가 이 필드를 이렇게 설정하면"이 전제이므로, 실제 에셋에서 해당 설정이 쓰이는지는 확인하지 못했다.
  - `UWxAIPerceptionComponent` 가 생성자에서 `OnTargetPerceptionUpdated.AddDynamic` 을 하는 것(`WxAIPerceptionComponent.cpp:38`)이 CDO→인스턴스 프로퍼티 복사와 어떻게 맞물리는지는 정적으로 단정하지 못했다. 현재 타겟팅이 실제로 동작하므로 발견으로 올리지 않았다. 같은 이유로 `PostInitProperties` 의 `ConfigureSense` 3회 호출이 인스턴스에서 중복 등록되지 않는지도 미확인이다.
  - 이번에 검토 후 발견에서 뺀 것들: `HandlePawnHit` 의 `EventMagnitude <= 0` 가드는 실제로 필요한 필터였다 — 패리 반동(`WxCombatAttributeSet.cpp:338-346`)과 처형 짝 피격(`WxAbility_Finisher.cpp:85-92`)이 magnitude 0 으로 같은 `Event.Hit.*` 를 보내므로 주석대로 걸러진다. `HandlePawnHit` 에 `HasAuthority` 가드가 없는 점은 퍼셉션 컴포넌트가 서버 전용인 AIController 에만 붙으므로 무해하다. `UWxBTService_TargetDistance` 의 `RandomDeviation = 0.0f`, `WxBlackboardKeys` 의 키 검증이 매 접근마다 도는 점(비-Shipping 한정), `WxBTTask_ActivateAbility.h:6` 이 Public 헤더에서 `AbilitySystemComponent.h` 전체를 끌어오는 점, `WxBTDecorator_AttributeRatio` 의 UPROPERTY Category 가 다른 노드(`Wx|AI`)와 달리 `Wx` 인 점은 영향이 작아 발견으로 세우지 않았다.

---
*문서 기준 커밋 `cf3a7a0` · 리뷰일 2026-08-25 · 소스 29파일 — `/module-review`로 갱신*
