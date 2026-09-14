# WxAI — 코드 리뷰

> 모듈 상태는 대체로 양호하다. 플러그인 의존은 `WxCore` 하나뿐이다(`Build.cs`·`.uplugin`·include 모두 확인). 소스 36개 모두 `CLAUDE.md` 규칙을 지킨다(첫 줄 Copyright, `Wx` 접두사, 람다·`FORCEINLINE`·헤더 본문 정의 0건). 남은 문제는 BT 엔진 계약과 어긋나는 세 곳(RandomChoice 관찰자 통지, MirrorMovement 노드 메모리, 모듈 밖 컨트롤러의 빙의 해제 순서)과 에디터 표기 누락 하나다. 커버리지: README → `Build.cs`/`.uplugin` → 헤더 18개·cpp 18개를 모두 읽었고, BT 탐색·롤백·퍼셉션 경로는 UE 5.8 엔진 소스와 대조했다. BT 에셋 3종에서 쓰는 노드는 클래스명으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 RandomChoice 가 조건이 막힌 자식을 가중치와 무관하게 관찰자로 올려, 조건이 풀리면 추첨 없이 그 자식을 실행한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:63`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:67`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:29`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 버그/정확성
- **문제**: 후보 수집 루프는 조건 평가(:63)에서 막힌 자식에 모두 `NotifyDecoratorsOnFailedActivation`(:67)을 보낸다. 가중치 0 제외(:89)는 그 뒤에 온다. 엔진은 이 통지로 LowerPriority·Both 데코의 Add 갱신을 쌓는다(엔진 `BTCompositeNode.cpp:307~311`). 다만 적용할 때 새 태스크보다 실행 인덱스가 뒤인 aux 노드의 Add 는 버린다(`BehaviorTreeComponent.cpp:1498~1499`). 그래서 실제로 관찰자가 되는 것은 선택된 자식보다 앞 인덱스의 막힌 자식이다. RandomChoice 가 실패해 부모가 뒤 형제로 넘어가면 막힌 자식 전부가 된다. 그 조건이 참으로 뒤집혀도 재추첨은 없다. 탐색 시작점이 그 자식에 고정되어, `UBTCompositeNode::GetNextChild` 가 `GetNextChildHandler` 를 건너뛰고 `GetMatchingChildIndex` 로 그 자식을 곧바로 고른다(`BTCompositeNode.cpp:598~602`). 가중치와 `bAvoidRepeat` 가 무시되고 `LastChosenChild` 도 갱신되지 않는다. 가중치 0 자식도 같은 경로를 타므로 "0 이면 추첨에서 제외된다"(`WxBTDecorator_RandomWeight.h:31`)는 약속이 깨진다. 예를 들어 [0] 근접 공격(거리 조건 aborts Both, 가중치 0 으로 잠시 뺌), [1] 원거리 공격 순서라면, 원거리 공격이 도는 중 대상이 다가올 때 근접 공격이 끼어들어 실행된다. 같은 자식을 [2] 에 두면 끼어들지 않으므로 자식 순서가 숨은 우선순위가 된다. 헤더 :29~:30 의 "인덱스와 무관하게 관찰자로 등록되고 재추첨이 일어날 수 있다"는 서술은 두 가지 모두 실제와 다르다.
- **제안**: 가중치 조회를 조건 평가 앞으로 옮기고, 가중치 0 자식은 통지 없이 건너뛴다. 헤더 :29~:30 은 실제 동작으로 고친다(앞 인덱스만 관찰, 뒤집히면 그 자식을 추첨 없이 실행). 인덱스 비대칭이 의도가 아니면 대안을 검토한다. 후보가 뽑힌 경우엔 통지하지 않고, 후보가 없어 실패할 때만 막힌 자식 전부에 통지하는 방식이다.
- **확신도**: 높음 (엔진 경로는 소스로 확인했다. BT 에셋 3종이 RandomChoice·RandomWeight 를 쓰지만, 가중치 0 과 Observer aborts 를 함께 쓰는지는 확인하지 못했다)

### 2. 🟡 AI 컨트롤러가 빙의를 풀 때 폰을 시야 자극원에서 영구히 빼 버린다
- **위치**: `Source/WxGame/Controller/WxAIController.cpp:135`, `Source/WxGame/Controller/WxAIController.cpp:138`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:18`
- **범주**: 버그/정확성
- **문제**: 모듈 밖 소비 측 코드지만 이 모듈의 계약을 깨므로 여기 적는다. 컴포넌트는 "캐릭터가 컨트롤러를 갈아타도(파티원 교체) 따라다닌다"(`WxAIBehaviorComponent.h:18`)고 전제한다. 그런데 `OnUnPossess` 는 `Super::OnUnPossess()`(:138)보다 먼저 `Perception->UnregisterComponent()`(:135)를 부른다. 엔진의 `OnUnregister → CleanUp` 은 리스너 해제와 함께 `GetMutableBodyActor()`, 즉 컨트롤러의 현재 폰을 `UAIPerceptionSystem::UnregisterSource` 로 모든 센스에서 뺀다(엔진 `AIPerceptionComponent.cpp:252~281`, `:460~474`). 이 시점엔 폰 참조가 살아 있어서 방금 놓은 캐릭터가 Sight 대상에서 사라진다. 폰을 자극원으로 올리는 경로는 폰 BeginPlay(`AISystem.cpp:111~126` → `OnNewPawn`)와 `StartPlay`(`AIPerceptionSystem.cpp:550~617`)뿐이라 다시 올라오지 않는다. 파티원 교체가 들어오면 플레이어가 넘겨받은 캐릭터를 적 AI 가 영영 보지 못한다. 청각·피격은 이벤트 보고라 영향이 없다. 지금 드러나지 않는 이유는 AI 폰의 빙의 해제가 폰 파괴 때만 일어나기 때문이다. 프로젝트의 `UnPossess` 호출은 플레이어 리스폰용 `Source/WxGame/Framework/WxRespawnLibrary.cpp:47` 하나뿐이다.
- **제안**: 언레지스터를 `Super::OnUnPossess()` 뒤로 옮긴다. 그때는 폰 참조가 끊겨 `GetBodyActor()` 가 null 이 되므로 자극원 해제를 건너뛴다. 교체 기능을 구현할 때 적 AI 가 교체된 캐릭터를 감지하는지 PIE 로 확인한다.
- **확신도**: 높음

### 3. 🟡 MirrorMovement 가 BT 노드 메모리에 힙을 소유하는 `TArray` 를 두어, 탐색 롤백이 해제된 버퍼를 가리키는 헤더를 되살릴 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:36`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:106`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:160`
- **범주**: 버그/정확성
- **문제**: BT 컴포넌트는 탐색을 시작할 때 인스턴스 메모리 전체를 바이트 복사로 떠 두고, 롤백 때 그대로 되돌린다(엔진 `BehaviorTreeComponent.cpp:2010`, `:2960~2980`, `BehaviorTreeTypes.cpp:348~351`). 이 스냅숏은 현재 태스크의 abort 가 지연되는 동안(`AbortTask` 가 InProgress)에도 남는다(`BehaviorTreeComponent.cpp:2236~2240`). 그 사이 실행 요청이 하나라도 오면 롤백이 돈다(`:1432~1449`). 이때 보존되는 것은 abort 중인 태스크의 메모리뿐이다(`:2294~2312`). 반면 aux 노드는 대기 중에도 매 틱 돈다(`:1759~1774`). 대기 중 `TickNode` 가 `Trail` 에 표본을 쌓다(:160) 용량을 넘겨 재할당한 뒤 요청이 오면, 해제된 버퍼를 가리키는 옛 헤더가 복원된다. 이후 `Add`/`RemoveAt`(:106)이 해제된 메모리에 쓰거나 재할당하고, `CleanupNodeMemory` 의 소멸자가 같은 포인터를 한 번 더 해제한다. 엔진 노드 메모리 구조체(`FBTMoveToTaskMemory`, `FBTFocusMemory` 등)에 컨테이너가 없는 것도 이 전제 때문이다. 터지려면 여러 틱에 걸친 지연 abort(늦게 마감하는 태스크)와 재할당(기록 초반, 프레임률 상승)이 겹쳐야 해서 드물다. 현재 BT 에셋도 이 서비스를 쓰지 않는다. 하지만 한번 터지면 원인을 좇기 어려운 힙 손상이다.
- **제안**: `bCreateNodeInstance = true` 로 서비스를 인스턴스화하고 `Trail` 을 노드 인스턴스 멤버로 옮긴다. `UWxBTTask_MirrorAbility` 가 실행 상태를 인스턴스에 두는 방식과 같다. 노드 메모리에는 트리비얼 복사가 가능한 값만 남긴다.
- **확신도**: 중간 (엔진 경로는 소스로 확인했다. 모듈의 C++ 태스크는 보통 동기로 abort 를 마감하므로, 여러 틱짜리 지연 abort 가 실제 트리에서 생기는지는 실행으로 확인하지 못했다)

### 4. 🟢 데코레이터 3종의 `GetStaticDescription` 이 `Super` 를 부르지 않아 그래프에서 aborts·inversed 표기가 사라진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:46`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:18`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:13`
- **범주**: 버그/정확성
- **문제**: 엔진의 `UBTDecorator::GetStaticDescription` 은 "( aborts lower priority, inversed )" 줄과 타입명을 만든다(엔진 `BTDecorator.cpp:122~145`). BT 그래프 노드는 이 문자열을 그대로 설명으로 쓴다(에디터 `BehaviorTreeGraphNode.cpp:70~76`). 세 데코는 이 함수를 덮으면서 `Super` 를 부르지 않아, 그래프만 봐서는 Observer aborts 와 Inverse Condition 을 알 수 없다. BeyondLeash 는 aborts 가 None 이면 폴링이 꺼지는데(`WxBTDecorator_BeyondLeash.h:24`) 그 여부가 보이지 않는다. AttributeRatio 와 RandomWeight 는 반전돼도 "HP / MaxHP <= 0.50", "Weight = 1.00" 으로만 보인다. 반전된 RandomWeight 는 자식을 영구히 막는다. 서비스 쪽은 같은 이유로 `GetStaticServiceDescription` 만 덮는다고 명시했지만(`WxBTService_LockOn.h:44`), 데코에는 이 원칙이 적용되지 않았다.
- **제안**: 엔진 데코처럼 세 함수 모두 `Super::GetStaticDescription()` 뒤에 자체 설명을 붙인다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp` 와 각 헤더. 계약 확인용으로 `Source/WxGame/Controller/WxAIController.cpp` 를 봤다. UE 5.8 엔진의 `BehaviorTreeComponent.cpp`·`BTCompositeNode.cpp`·`BTDecorator.cpp`·`BehaviorTreeTypes.cpp`·`BTNode.h`·`BTTask_MoveTo.cpp`·`AIPerceptionComponent.cpp`·`AIPerceptionSystem.cpp`·`AISystem.cpp`·`AISense_Sight.cpp`·`AISense_Hearing.cpp` 와 에디터 `BehaviorTreeGraphNode.cpp` 도 대조했다.
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp` 와 대응 헤더, `Source/WxGame/Character/WxEnemyCharacter.cpp`.
- **미검토 / 한계**: 빌드·PIE·네트워크 실행 없이 코드만 읽었다. BT 에셋은 노드 클래스명만 확인했다. RandomChoice·RandomWeight·LockOn·BeyondLeash·ActivateAbility·Patrol·ReturnHome·Wander 는 쓰이고, MirrorAbility·MirrorMovement 는 어느 에셋에도 없다. 가중치·Observer aborts 같은 노드 속성 값은 보지 못했다. 직전 리뷰(`9d8cb2dd`) 대비 변경은 두 가지다. (1) 3번 🟢(감각 getter 3개 미사용)은 워킹 트리 변경으로 해소돼 뺐다. (2) 직전 1번의 "뒤쪽 자식도 관찰자가 되어 'lower priority than Current Task' ensure 가 뜬다"는 서술은 틀렸다. 엔진이 새 태스크보다 뒤인 aux 노드의 Add 를 버리므로(`BehaviorTreeComponent.cpp:1498~1499`) 현재 1번으로 고쳐 썼다. 새로 세운 뒤 기각한 가설은 다음과 같다. (3) `ApplySenseSettings` 의 config 재전달이 반영되지 않는다 — 5.8 `ConfigureSense` 는 기존 config 에도 `OnListenerConfigUpdated` 를 부른다(`AIPerceptionComponent.cpp:141~155`). 이 호출이 Sight 다이제스트(`AISense_Sight.cpp:978~997`)와 Hearing 다이제스트(`AISense_Hearing.cpp:100~116`)를 갱신한다. (4) ReturnHome 이 실패한 뒤 BeyondLeash 가 옛 활성 노드 기준(`IsExecutingBranch` 참)으로 `bWasBeyond` 를 시드한다 — 그 순간의 실제 거리 판정도 이탈이라 값이 어긋나지 않는다. (5) LockOn·MirrorMovement 의 적용 기록(`TWeakObjectPtr`)이 롤백으로 되돌려진다 — 엔진 `FBTFocusMemory` 와 같은 패턴이고 여러 틱짜리 지연 abort 가 필요해 적지 않았다. (6) 완주 후 대기 중인 Patrol 을 abort 할 때 낡은 MoveTo 메모리가 문제를 일으킨다 — 5.8 `UBTTask_MoveTo::AbortTask`(`BTTask_MoveTo.cpp:235~257`)는 요청 ID 가 맞을 때만 이동을 끊어 무해하다. 직전 리뷰가 기각한 항목들은 해당 코드가 그대로라 판정을 유지한다. MirrorAbility 의 Aborting 중 마감, 발동 프로토콜·감속 GE·해제 절차 중복, 스티키 타겟, 가중치 전원 0 승계, Patrol 인스턴스 재빙의, 서비스 교체 순서, ASC 구독 수명, BeyondLeash 메모리 초기화, `Trail[0]` 접근이 여기에 해당한다.

---
*문서 기준 커밋 `a5f165c2` · 리뷰일 2026-09-14 · 소스 36파일 — `/module-review`로 갱신*
