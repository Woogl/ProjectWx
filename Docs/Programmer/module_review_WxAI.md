# WxAI — 코드 리뷰

> 모듈 상태는 양호하다. 플러그인 의존은 `WxCore` 하나뿐이다(`Build.cs`·`.uplugin` 모두, 외부 include 도 `WxGameplayTags.h`·`Minion/WxMinion.h` 두 개로 `WxCore` 소속). `CLAUDE.md` 규칙 위반도 없다(소스 36개 모두 첫 줄 Copyright, 람다·`FORCEINLINE`·`BlueprintCallable` 0건, `Wx` 접두사와 콜백의 `Handle` 접두사 준수). 실질 문제는 `UWxBTComposite_RandomChoice` 의 관찰자 등록 한 곳과, 모듈 밖 컨트롤러가 이 모듈의 계약을 깨는 잠복 버그 하나다. 커버리지: README → `Build.cs`/`.uplugin` → 헤더 18개·cpp 18개를 전부 읽었다. BT 노드의 수명주기와 관찰자 경로는 UE 5.8 엔진 소스(`BehaviorTreeComponent.cpp`·`BTCompositeNode.cpp`·`AIPerceptionComponent.cpp` 등)와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 RandomChoice 가 뒤쪽·가중치 0 자식까지 관찰자로 올려, 조건이 뒤집히면 ensure 와 함께 추첨 없이 그 자식을 실행한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:63`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:67`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:30`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 버그/정확성
- **문제**: 후보 수집 루프는 조건 평가(:63)에서 막힌 모든 자식에 `NotifyDecoratorsOnFailedActivation`(:67)을 보낸다. 가중치 0 제외(:89)는 그 뒤에 오므로, LowerPriority·Both 데코가 붙은 자식은 실행 중인 자식보다 뒤에 있든 가중치가 0 이든 관찰자로 등록된다. 그 조건이 참으로 뒤집혔을 때 엔진이 하는 일은 헤더(:30)가 말하는 "재추첨"이 아니다. 첫째, `UBehaviorTreeComponent::EvaluateBranch` 는 요청한 데코의 실행 인덱스가 현재 태스크보다 뒤면 "decorator requesting restart has lower priority than Current Task" ensure 를 띄운다(엔진 `BehaviorTreeComponent.cpp:945~960`, Shipping 제외). 표준 `UBTDecorator_Blackboard`(`ActivateBranch` 경유)와 `RequestExecution(this)` 경유 데코 모두 이 함수를 지난다. 둘째, 요청은 막히지 않고 진행된다. 탐색 시작점이 그 자식에 고정되고, `UBTCompositeNode::GetNextChild` 는 `SearchStart` 가 있는 첫 진입에서 `GetNextChildHandler` 를 부르지 않고 `GetMatchingChildIndex` 로 그 자식을 곧바로 고른다(`BTCompositeNode.cpp:598~601`). 가중치와 `bAvoidRepeat` 가 모두 무시되고 `LastChosenChild` 도 갱신되지 않는다. 그래서 "0 이면 추첨에서 제외된다"(`RandomWeight.h:31`)는 약속이 Observer aborts 를 건 자식에서 깨진다. 예를 들어 거리 조건(aborts Both)을 붙인 근접 공격을 가중치 0 으로 잠시 빼 두면, 원거리 공격 도중 대상이 다가오는 순간 PIE 에서 ensure 가 뜨고 그 근접 공격이 실행된다. cpp :88 주석이 막으려던 "뽑힐 수 없는 자식의 관찰자 등록"이 조건 실패 쪽에서는 그대로 열려 있는 셈이다.
- **제안**: 가중치 조회를 조건 평가 앞으로 옮겨, 가중치 0 자식은 통지 없이 건너뛴다. 실패 통지는 추첨이 끝난 뒤 선택된 자식보다 앞 인덱스에만 보내 엔진의 우선순위 전제와 맞춘다. 후보가 없어 부모로 돌아갈 때는 전부 보낸다. 헤더 :29~:30 의 서술도 실제 동작("그 자식이 추첨 없이 실행된다")으로 고친다.
- **확신도**: 높음 (엔진 경로는 소스로 확인했다. 실제 BT 에셋이 이 조합을 쓰는지는 확인하지 못했다)

### 2. 🟡 AI 컨트롤러가 빙의를 풀 때 폰을 시야 자극원에서 영구히 빼 버린다
- **위치**: `Source/WxGame/Controller/WxAIController.cpp:135`, `Source/WxGame/Controller/WxAIController.cpp:138`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:18`
- **범주**: 버그/정확성
- **문제**: 모듈 밖 소비 측 코드지만, 이 모듈 컴포넌트가 전제한 "캐릭터가 컨트롤러를 갈아타도 따라다닌다"(`WxAIBehaviorComponent.h:18`)는 계약을 깨므로 여기 적는다. `OnUnPossess` 는 `Super::OnUnPossess()`(:138)보다 먼저 `Perception->UnregisterComponent()`(:135)를 부른다. 엔진의 `UAIPerceptionComponent::OnUnregister → CleanUp` 은 리스너 해제에 더해 `GetMutableBodyActor()`, 즉 컨트롤러의 현재 폰을 `UAIPerceptionSystem::UnregisterSource` 로 모든 센스의 자극원에서 뺀다(엔진 `AIPerceptionComponent.cpp:276~280`). 이 시점엔 폰 참조가 아직 살아 있어 방금 놓은 캐릭터가 Sight 자극원 목록에서 사라진다. 폰을 자극원으로 올리는 것은 스폰(`UAISystem::OnActorSpawned → OnNewPawn`)과 `StartPlay` 뿐이라 다시 등록되지도 않는다. 파티원 교체(AI → 플레이어)가 들어오면 플레이어가 넘겨받은 캐릭터를 적 AI 가 영영 보지 못한다. 청각·피격은 이벤트 보고라 영향이 없다. 지금 드러나지 않는 것은 AI 폰의 빙의 해제가 폰 파괴 때만 일어나기 때문이다. 프로젝트의 `UnPossess` 호출은 플레이어 리스폰용 `Source/WxGame/Framework/WxRespawnLibrary.cpp:47` 하나뿐이다.
- **제안**: 언레지스터를 `Super::OnUnPossess()` 뒤로 옮긴다. 폰 참조가 끊긴 뒤라 `GetBodyActor()` 가 null 을 돌려주어 자극원 해제를 건너뛴다. 교체 기능을 구현할 때 적 AI 가 교체된 캐릭터를 감지하는지 PIE 로 확인한다.
- **확신도**: 높음

### 3. 🟢 `UWxAIBehaviorComponent` 의 감각 수치 getter 3개는 호출부가 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:36`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:37`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:38`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:111`
- **범주**: 중복/복잡도
- **문제**: `GetSightRadius`/`GetSightAngle`/`GetHearingRadius` 는 `Source`·`Plugins` 전체에서 호출부가 0건이고 `UFUNCTION` 도 아니다. 수치는 같은 클래스의 `ApplySenseSettings` 와 에디터 `TickComponent` 가 멤버를 직접 읽는다. 남겨 두면 밖에서 수치를 가져가는 경로가 있는 것처럼 읽혀, "컴포넌트가 컨트롤러에 밀어 넣는다"는 소유 방향과 어긋난다.
- **제안**: 세 함수의 선언과 정의를 지운다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` 와 각 헤더. 계약 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp` 와 UE 5.8 엔진의 `BehaviorTreeComponent.cpp`·`BTCompositeNode.cpp`·`BTTaskNode.cpp`·`BehaviorTreeTypes.cpp`·`BTDecorator.cpp`·`BTDecorator_Blackboard(Base).cpp`·`AIPerceptionComponent.cpp`·`AIPerceptionSystem.cpp`·`AISystem.cpp`·`GameplayAbility.cpp` 를 대조했다.
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp` 와 대응 헤더.
- **미검토 / 한계**: 빌드·PIE·네트워크 실행 없이 코드만 읽었고, BT/Blackboard 에셋의 실제 노드 배치(RandomChoice 자식의 Observer aborts 사용 여부, LockOn 과 MirrorMovement 를 한 트리에 두지 않는 규약)는 확인하지 않았다. 이전 리뷰 항목 중 다음은 현재 코드로 재검증한 뒤 뺐다. (1) MirrorAbility `TickTask` 가 Aborting 중에 `Succeeded`/`Failed` 로 마감하는 문제 — 코드는 그대로지만 영향이 사실상 없다. `ConditionalNotifyChildExecution` 은 `bUseChildExecutionNotify` 가 꺼진 Selector·Sequence·RandomChoice 에서 아무 일도 하지 않고, 태스크는 `OnTaskFinished` 를 덮지 않으며, `bWasAborting` 이 재탐색 요청을 막는다. 오히려 지연된 취소가 끝내 오지 않을 때 트리를 Aborting 에서 풀어 주는 안전망 역할을 한다. (2) ActivateAbility/MirrorAbility 발동 프로토콜 중복, Patrol/Wander 감속 GE 중복, LockOn/MirrorMovement 해제 절차 중복 — 자기완결 재구현 방침에 따른 의도로 본다. (3) `UWxBTService_UpdateTargetActor` 의 스티키 타겟·후보 순서 — 보류된 어그로 시스템의 범위라 결함으로 적지 않는다. (4) RandomChoice 가중치 전원 0 시 직전 결과 승계 — 헤더(`WxBTComposite_RandomChoice.h:25`)에 명시된 한계다. 새로 세운 뒤 기각한 가설: (5) Patrol 노드 인스턴스의 커서·완주 상태가 재빙의로 새 폰에 넘어간다 — `StartTree → ProcessPendingInitialize → RemoveAllInstances` 가 `NodeInstances` 를 비워 인스턴스가 새로 만들어진다. (6) 브랜치 전환 때 새 LockOn 서비스의 적용을 옛 서비스의 해제가 지운다 — 서비스 Add 는 post-update(`BehaviorTreeTypes.cpp:516`)라 옛 `OnCeaseRelevant` 뒤에 새 `OnBecomeRelevant` 가 온다. (7) `UWxAIBehaviorComponent` 가 ASC 이벤트 구독을 해제하지 않는다 — ASC 가 같은 캐릭터(`AWxCharacterBase`)에 있어 수명이 같고 `CreateUObject` 바인딩이라 댕글링이 없다. 에디터 전용 틱은 게임 월드에서 `BeginPlay` 가 끄고 `EWorldType::Editor` 에서만 그린다. (8) 직전 리뷰에서 기각한 BeyondLeash 메모리 초기화, MirrorMovement `Trail[0]`, Patrol 커서 범위, `FindPatrolComponent` 래퍼 항목은 소스가 그대로라 판정을 유지한다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 36파일 — `/module-review`로 갱신*
