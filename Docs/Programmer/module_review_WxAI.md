# WxAI — 코드 리뷰

> 규칙 면은 완전히 깨끗하고(29파일 전부 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix를 지키며 `BlueprintCallable`·`FORCEINLINE`·람다가 하나도 없다), 지난 리뷰의 발견 11건 중 8건이 실제로 고쳐졌다 — 촉각 자극의 적대 필터, 복귀 이동 성패 확인 후 억제, ActivateAbility 재발동 판별, RandomChoice 의 활성화 실패 알림, Once 정찰의 브랜치 점유, AttributeRatio·RandomWeight 의 abort 드롭다운 잠금, BeyondLeash 의 무효 앵커 가드가 모두 반영됐다. 이번 리뷰는 소스 29개(cpp 14 + h 15)를 모두 열고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite 의 cpp 를 깊게 봤으며, 지난번과 달리 이 환경에 UE 5.8 엔진 소스(`C:\Program Files\Epic Games\UE_5.8`)가 있어 엔진 내부 동작에 기대는 판단을 전부 라인 단위로 대조했다 — 그 대조 과정에서 리시 복귀를 무력화하는 새 결함 하나를 찾았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 억제 해제 시 재획득이 "만료되지 않는" 청각·촉각 자극을 집어 리시 복귀가 스스로 무효화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:157-169`, 원인 설정은 `:30-36`
- **범주**: 버그/정확성
- **문제**: 억제 해제 경로의 재획득 루프가 `It->Value.HasAnyCurrentStimulus()` 로 "지금 감지 중인" 액터를 고르는데, 이 판정은 **모든 센스 슬롯**을 훑는다. 그런데 이 컴포넌트의 `HearingConfig`(`:30-34`)와 `DamageConfig`(`:36`)는 `MaxAge` 를 지정하지 않고, 엔진의 `UAISenseConfig::MaxAge` 기본값은 0 이며 `GetMaxAge()` 는 0 을 `FAIStimulus::NeverHappenedAge`(= `FLT_MAX`)로 바꿔 돌려준다(`Engine/Source/Runtime/AIModule/Classes/Perception/AISenseConfig.h:49`). 그 값이 그대로 자극의 `ExpirationAge` 가 되어(`AIPerceptionComponent.cpp:479`) `AgeStimulus` 가 영원히 `true` 를 반환하므로(`AIPerceptionTypes.h:187-191`) 청각·촉각 자극은 **한 번 들어오면 절대 만료되지 않는다.**
  Sight 는 시야를 잃을 때 엔진이 `SensingFailed` 자극을 따로 방송해 슬롯이 정리되지만(`AISense_Sight.cpp:611`, `:764`), Hearing 과 Damage 는 성공 자극만 등록하는 일회성 센스라 해제 이벤트가 아예 없다(`AISense_Hearing.cpp:164-165`, `AISense_Damage.cpp:98`). 결과적으로 **한 번이라도 이 AI를 때렸거나 근처에서 소리를 낸 액터는 영구히 "현재 감지 중"으로 남는다.**
  실패 시나리오: 플레이어가 적을 리시 밖까지 끌고 감 → `UWxBTDecorator_BeyondLeash` 참 → `UWxBTTask_ReturnHome` 이 억제를 켜고 홈으로 복귀 → 도착 시 `OnTaskFinished` 가 `SetTargetingSuppressed(false)` → 재획득 루프가 플레이어를 다시 집는다(시야에는 없지만 촉각/청각 슬롯이 살아 있으므로). 타겟이 서면 전투 브랜치가 다시 추격 → 리시 이탈 → 복귀 → 재획득이 무한 반복된다. 전투를 한 번이라도 한 적은 예외 없이 이 경로를 밟으므로, 리시 기능 자체가 사실상 동작하지 않는다.
  같은 자리에 부차적 문제도 있다 — `FActorPerceptionContainer`(TMap) 순회의 "첫 항목"을 집으므로, 적대 대상이 둘 이상일 때 누가 뽑힐지 결정적이지 않다.
- **제안**: 재획득의 의도는 "Sight 가 엣지 트리거라 억제 중 계속 보이던 대상을 놓친다"를 메우는 것이므로, 판정을 Sight 로 한정하면 원인과 의도가 맞아떨어진다 — `HasAnyCurrentStimulus()` 대신 `HasActiveStimulus(*Target, UAISense::GetSenseID<UAISense_Sight>())`(`AIPerceptionComponent.cpp:771`)를 쓰거나 `GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), ...)` 로 바꾼다. 부차적으로 `HearingConfig`/`DamageConfig` 에 유한한 `MaxAge`(수 초)를 지정하면 퍼셉션 데이터가 무한히 "현재"로 남는 문제도 함께 없어진다.
- **확신도**: 높음 (엔진 소스로 자극 만료·해제 이벤트 유무를 모두 대조함)

### 2. 🟡 Patrol 의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:47-50`, `:64`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask` 는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation` 에 목표를 쓰지만(`:49`), 실제 이동은 `Super::ExecuteTask`(`:64`)가 `UBTTask_MoveTo::BlackboardKey` 를 읽어 수행한다(`Engine/.../BTTask_MoveTo.cpp:117-123` 의 `PerformMoveTask`). 그 필드는 엔진에서 `EditAnywhere` 이고 생성자 `:19` 는 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 키를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation` 에 쓰고 엉뚱한 키(대개 미설정)로 이동을 시도한다. 경고 하나 없이 정찰이 실패하거나 폰이 엉뚱한 곳으로 걸어가는 형태로만 드러난다. `UWxBTTask_ReturnHome` 은 읽기만 하므로 같은 위험이 없다.
- **제안**: 쓰기도 `BlackboardKey.SelectedKeyName` 을 경유해 읽는 키와 통일하거나, 반대로 디테일 패널에서 `BlackboardKey` 편집을 잠근다.
- **확신도**: 높음(메커니즘) / 중간(실제 에셋 설정에 달림)

### 3. 🟢 `EWxTeam` 이 WxAI 에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:121`, `WxCharacterBase.cpp:170`·`:182`, `WxEnemyCharacter.cpp:18`, `WxPlayerCharacter.cpp:22` 뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(피아 판정도 엔진 타입인 `FGenericTeamId::GetAttitude` 로 한다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인 플러그인이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h` 를 `WxCore` 로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 4. 🟢 Patrol 과 Wander 의 감속 GE 부여·제거 코드가 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:53-62`·`:82-86`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55-64`·`:107-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` → `OnTaskFinished` 에서 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다(헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트 — `WxBTTask_Patrol.h:34`·`:43`·`:57`, `WxBTTask_Wander.h:56`·`:65`·`:75` — 와 `GetStaticDescription` 의 미지정 분기도 동일하다). SetByCaller 태그나 핸들 수명 규약을 바꾸면 두 곳을 함께 고쳐야 한다.
- **제안**: 두 태스크의 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode` 라 공통 베이스를 만들 수 없으므로, "부여/해제" 두 함수만 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`·`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **엔진 대조로 확인 후 발견에서 제외한 것들**:
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출은 중복 등록을 만들지 않는다 — 엔진 `ConfigureSense` 가 같은 클래스의 기존 config 를 **교체**한다(`AIPerceptionComponent.cpp:122-138`).
  - `HandlePawnHit` 의 적대 필터(`:253-257`)는 위치가 옳고 충분하다 — Damage 센스는 피격 액터의 컨트롤러 리스너로만 자극을 보내므로(`AISense_Damage.cpp:87-99`) 보고 시점 필터가 유일한 관문이며, `ContextHandle.GetInstigator()` 는 `WxCombatLibrary.cpp:35` 의 `AddInstigator(SourceActor, SourceActor)` 로 채워진 공격자 캐릭터라 `IGenericTeamAgentInterface` 판정이 성립한다.
  - `RandomChoice` 의 사전 필터는 이제 `NotifyDecoratorsOnFailedActivation` 을 직접 보내 엔진 규약을 복원한다. 엔진이 그 앞에 두는 `bUseDecoratorsFailedActivationCheck` 게이트는 기본 false 라 건너뛰어도 동작이 같다(`BTCompositeNode.cpp:21`, `:56`).
  - `ReturnHome` 의 "이동 시작 확인 후 억제" 순서는 안전하다 — MoveTo 는 동기 완료를 `bObserverCanFinishTask` 로 걸러 `ExecuteTask` 반환값에 담으므로(`BTTask_MoveTo.cpp:133`·`:155-158`), 억제를 켜기 전에 완료가 새어 나가지 않는다.
  - `Patrol` 이 `bPatrolFinished` 때 `Super::ExecuteTask` 없이 `InProgress` 를 반환해도 MoveTo 노드 메모리의 잔여 상태는 해롭지 않다 — abort 시 `AbortMove` 에 실리는 낡은 `MoveRequestID` 는 PathFollowing 이 현재 요청과 대조해 무시한다.
  - BT 가 통째로 멈춰도 `ActivateAbility` 의 `OnAbilityEnded` 구독은 남지 않는다 — `StopTree` 가 활성 태스크에 `WrappedAbortTask` 를 부른다(`BehaviorTreeComponent.cpp:381-394`).
  - `WxAnimNotify_ReportNoise` 의 "실제 거리 = min(HearingDistance, 청취자 HearingRange)" 주석은 정확하다(`AISense_Hearing.cpp:147`·`:152`).
- **미검토 / 한계**:
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 2 는 "디자이너가 `BlackboardKey` 를 바꾸면"이 전제이므로, 실제 에셋에서 그 설정이 쓰이는지는 확인하지 못했다.
  - 발견 1 의 오실레이션은 정적 분석으로 도출한 것이며, 실제 BT 에셋의 전투 브랜치 구성(타겟이 서면 곧바로 추격하는지)에 따라 체감 양상이 달라질 수 있다. 인게임 재현은 하지 않았다.
  - `UWxBTComposite_RandomChoice` 가 엔진 Selector 와 달리 매 탐색마다 **모든** 자식의 조건 데코를 평가해, 선택되지 않은 하위 자식의 LowerPriority 데코까지 관찰자로 등록한다. 등록된 관찰자가 더 높은 우선순위의 실행 중 자식을 선점하지는 못해 무해하다고 판단했고 헤더에도 의도가 명시돼 있어 발견으로 세우지 않았으나, 이 컴포짓 아래 관찰자 abort 데코를 늘릴 계획이면 재검토가 필요하다.
  - 이번에 검토 후 발견에서 뺀 것들: `WxBTComposite_RandomChoice.cpp:24`·`:48` 이 `CastInstanceNodeMemory` 대신 `reinterpret_cast` 를 쓰는 점(크기는 맞음), `WxBTTask_ActivateAbility.h:6` 이 Public 헤더에서 `AbilitySystemComponent.h` 전체를 끌어오면서 바로 아래 `:12` 에 같은 타입의 전방 선언을 중복으로 두는 점, `WxBTDecorator_AttributeRatio` 의 UPROPERTY Category 가 다른 노드(`Wx|AI`)와 달리 `Wx` 인 점, `WxBlackboardKeys` 의 키 검증이 매 접근마다 도는 점(비-Shipping 한정)은 영향이 작아 세우지 않았다.

---
*문서 기준 커밋 `d91816be` · 리뷰일 2026-08-26 · 소스 29파일 — `/module-review`로 갱신*
