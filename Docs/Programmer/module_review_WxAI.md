# WxAI — 코드 리뷰

> 모듈은 대체로 건강하다. 플러그인 의존은 `WxCore` 하나뿐이다(`Build.cs`·`.uplugin`·include 모두 확인). 소스 36개 모두 첫 줄 Copyright와 `Wx` 접두사를 지키고, 람다·`FORCEINLINE`·헤더 본문 정의가 없다. 직전 리뷰의 빙의 해제 순서 문제는 컨트롤러 쪽에서 고쳐졌다. 남은 문제는 BT 엔진 계약과 어긋나는 두 곳(RandomChoice 관찰자 통지, MirrorMovement 노드 메모리)과 사소한 두 건이다. 사소한 두 건 중 하나는 컨트롤러의 피아 판정 변경 뒤 청각과 어긋난 전제다. 커버리지: README → `Build.cs`/`.uplugin` → 헤더 18개·cpp 18개를 모두 읽었다. BT 탐색·롤백·퍼셉션 피아 판정 경로는 UE 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 RandomChoice 가 가중치를 보기 전에 막힌 자식에 활성화 실패를 통지해, 조건이 풀리면 추첨 없이 그 자식이 끼어든다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:63`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:67`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:29`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 버그/정확성
- **문제**: 후보 수집 루프는 조건 평가(:63)에 걸린 자식마다 `NotifyDecoratorsOnFailedActivation`(:67)을 부른다. 가중치 0 제외(:89)는 그 뒤에야 본다. 엔진은 이 통지에서 LowerPriority·Both 데코를 관찰자 Add 로 쌓는다(엔진 `BTCompositeNode.cpp:307~311`). 적용할 때는 새 태스크보다 실행 인덱스가 큰 aux 노드의 Add 를 건너뛴다(`BehaviorTreeComponent.cpp:1498~1499`).
  - 그래서 실제 관찰자는 뽑힌 자식보다 앞 인덱스에 있는 막힌 자식이다. RandomChoice 가 실패해 뒤 형제로 넘어가면 막힌 자식 전부가 관찰자가 된다.
  - 그 조건이 참이 되면 탐색이 그 가지를 시작점으로 잡는다. `UBTCompositeNode::GetNextChild` 는 `GetNextChildHandler` 를 거치지 않고 `GetMatchingChildIndex` 로 그 자식을 곧장 고른다(`BTCompositeNode.cpp:598~602`).
  - 이때 가중치와 `bAvoidRepeat` 가 무시되고 `LastChosenChild` 도 갱신되지 않는다. 가중치 0 자식도 같은 경로로 실행되므로 "0 이면 추첨에서 제외된다"(`WxBTDecorator_RandomWeight.h:31`)는 약속이 깨진다.
  - 예: 자식이 [0] 근접 공격(거리 조건 aborts Lower Priority, Weight 0 으로 잠시 뺌), [1] 원거리 공격 순서다. 원거리 공격이 도는 중 대상이 붙으면 근접 공격이 끼어든다. 같은 자식을 [2] 에 두면 끼어들지 않으므로, 자식 순서가 숨은 우선순위가 된다.
  - 헤더 :29~30 은 "인덱스와 무관하게 관찰자로 등록되고, 뒤집히면 재추첨된다"고 적었다. 실제 동작은 앞 인덱스만 관찰하고, 뒤집히면 추첨 없이 그 자식을 실행한다.
- **제안**: 가중치 조회를 조건 평가 앞으로 옮기고, 가중치 0 자식은 통지 없이 건너뛴다. 인덱스에 따른 비대칭이 의도가 아니라면 방식을 바꾸는 것도 검토한다. 후보가 뽑혔을 때는 통지하지 않고, 후보가 없어 실패할 때만 막힌 자식 전부에 통지하는 방식이다. 어느 쪽이든 헤더 :29~30 을 실제 동작에 맞게 고친다.
- **확신도**: 높음. 엔진 경로는 소스로 확인했다. `BT_Minion`·`BT_Soldier`·`BT_Template` 가 RandomChoice·RandomWeight 를 쓰는 것도 확인했다. 다만 자식 데코의 aborts 설정과 가중치 값은 보지 못했다.

### 2. 🟡 MirrorMovement 가 힙을 소유하는 `TArray` 를 BT 노드 메모리에 두어, 지연 abort 중 롤백이 해제된 버퍼를 가리키는 헤더를 되살릴 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:36`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:106`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:160`
- **범주**: 버그/정확성
- **문제**: BT 컴포넌트는 탐색을 시작할 때마다 인스턴스 메모리 전체를 바이트 단위로 떠 두고, 롤백하면 그대로 되돌린다(엔진 `BehaviorTreeComponent.cpp:2010`, `:2960~2980`).
  - 현재 태스크의 abort 가 InProgress 로 지연되면 대기 중인 실행은 진행되지 않는다(`:2236~2240`). 그 사이 실행 요청이 들어오면 롤백이 돈다(`:1432~1449`).
  - 롤백에서 보존되는 것은 abort 중인 활성 태스크의 메모리뿐이다(`:2294~2312`, 플래그 설정은 `:2537`). 반면 aux 노드는 대기 중에도 매 프레임 틱을 받는다(`:1759~1774`).
  - 대기 중 `TickNode` 가 `Trail` 에 표본을 추가하다(:160) 용량을 넘기면 버퍼가 재할당된다. 그 뒤 요청이 오면 해제된 포인터를 든 옛 헤더가 복원된다.
  - 이후 `RemoveAt`(:106)과 `Add` 는 해제된 메모리를 읽고 쓴다. `CleanupNodeMemory`(:59)의 소멸자는 같은 포인터를 한 번 더 해제한다.
  - 엔진 노드 메모리 구조체(`FBTMoveToTaskMemory`·`FBTFocusMemory` 등)에 컨테이너가 없는 것도 이 전제 때문이다.
  - 터지려면 여러 틱에 걸친 지연 abort 와 재할당(기록 초반, 프레임률 상승)이 겹쳐야 한다. 지연 abort 는 예컨대 `WxBTTask_ActivateAbility.cpp:138` 의 InProgress 반환이다. 그래서 드물고, 현재 BT 에셋도 이 서비스를 쓰지 않는다. 하지만 한번 터지면 원인을 좇기 어려운 힙 손상이다.
- **제안**: `bCreateNodeInstance = true` 로 서비스를 인스턴스화하고 `Trail` 을 노드 인스턴스 멤버로 옮긴다. `UWxBTTask_MirrorAbility` 가 실행 상태를 인스턴스에 두는 방식과 같다. 노드 메모리에는 트리비얼 복사가 가능한 값만 남긴다.
- **확신도**: 중간. 엔진 경로는 소스로 확인했다. 다만 모듈의 태스크는 보통 abort 를 동기로 마감하므로, 여러 틱짜리 지연 abort 가 실제 트리에서 생기는지는 실행으로 확인하지 못했다.

### 3. 🟢 데코레이터 3종의 `GetStaticDescription` 이 `Super` 를 부르지 않아 그래프에서 aborts·inversed 표기가 사라진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:46`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:18`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:13`
- **범주**: 버그/정확성
- **문제**: 엔진의 `UBTDecorator::GetStaticDescription` 은 "( aborts lower priority, inversed )" 줄과 타입명을 만든다(엔진 `BTDecorator.cpp:122~145`). 세 데코는 이 함수를 덮으면서 `Super` 를 부르지 않는다. 그래서 그래프만 봐서는 Observer aborts 와 Inverse Condition 을 알 수 없다.
  - BeyondLeash 는 aborts 가 None 이면 폴링이 꺼지는데(`WxBTDecorator_BeyondLeash.h:24`), 그 여부가 그래프에 보이지 않는다.
  - 반전된 RandomWeight 는 자식을 영구히 막는데도 그래프에는 "Weight = 1.00" 으로만 보인다.
  - 서비스 쪽은 같은 이유로 `GetStaticServiceDescription` 만 덮는다고 명시했다(`WxBTService_LockOn.h:44`). 데코에는 이 원칙이 적용되지 않았다.
- **제안**: 엔진 데코처럼 세 함수 모두 `Super::GetStaticDescription()` 결과 뒤에 자체 설명을 붙인다.
- **확신도**: 높음

### 4. 🟢 UpdateTargetActor 의 "세 센스 모두 적대만 등록" 전제가 청각에서 성립하지 않는다 — 시야·촉각은 폰 규칙, 청각은 기본 팀 솔버를 쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h:29`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:60`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp:23`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:161`
- **범주**: 버그/정확성
- **문제**: `FindPerceivedTarget` 은 피아를 다시 가르지 않고, 조건에 맞는 첫 감지 액터를 문다(:60). 그 근거가 헤더 :29 의 "세 센스 모두 적대만 등록"이다.
  - 최근 `AWxAIController::GetTeamAttitudeTowards` 가 폰에 위임하게 바뀌었다(`Source/WxGame/Controller/WxAIController.cpp:49~56`). 이제 시야는 폰 규칙을 따른다. 촉각도 이 모듈의 필터(`WxAIBehaviorComponent.cpp:161`)가 폰 규칙으로 거른다. 폰 규칙은 어느 한쪽이 Neutral 이면 Neutral 을 돌려준다(`Source/WxGame/Character/WxCharacterBase.cpp:176~179`).
  - 청각은 다르다. 리스너 팀 ID 와 소음 팀 ID 를 팀 솔버로 비교한다(엔진 `AISense_Hearing.cpp:157`, `AIPerceptionTypes.h:231~235`). 기본 솔버는 두 ID 가 다르기만 하면 Hostile 이다(`AIInterfaces.cpp:30~33`).
  - `EWxTeam::Neutral` 은 255 로 `NoTeam` 과 값이 같다(`Source/WxGame/Character/WxTeamTypes.h:13`). 팀 인터페이스가 없는 소음원도 `NoTeam` 이 된다(`AISense_Hearing.cpp:29`, `AIInterfaces.cpp:14~23`).
  - 소음 노티파이는 공용 조깅 애니 8종에 박혀 있고, `ABP_Unarmed` 의 `BS_Idle_Walk_Run` 이 이를 쓴다. Neutral 팀 캐릭터나 `AWxNpc` 같은 비팀 액터가 이 애니로 적의 청취 반경을 지나가는 경우를 보자. 적대 자극으로 등록되고, 이 서비스가 그 액터를 TargetActor 로 확정한다.
  - 타겟은 사망하거나 리시로 복귀할 때까지 유지된다. 그동안 적은 중립 대상을 쫓아 락온하고 공격한다.
  - 지금은 Neutral 팀 에셋이 없고 `AWxNpc` 는 제자리에 서 있기만 해서 드러나지 않는다.
- **제안**: 피아 판정의 출처를 하나로 모은다. WxGame 에서 `FGenericTeamId::SetAttitudeSolver` 로 폰 규칙과 같은 솔버를 등록하면 청각까지 한 번에 맞는다. 모듈 안에서 막으려면 `FindPerceivedTarget` 에 `HandlePawnHit` 과 같은 `FGenericTeamId::GetAttitude(SelfActor, PerceivedActor) == ETeamAttitude::Hostile` 검사를 넣는다. 어느 쪽이든 헤더 :29 를 실제에 맞게 고친다.
- **확신도**: 중간. 엔진 경로는 소스로 확인했다. Neutral 팀이나 비팀 소음원이 실제로 쓰일지는 기획에 달렸다. 컨트롤러 헤더(`WxAIController.h:27`)가 청각의 솔버 경로를 이미 언급하고 있어, 지금의 불일치를 알고 둔 것일 수도 있다.

## 검토 범위
- **깊게 본 파일**
  - WxAI 소스와 각 헤더: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`
  - 계약 확인용 WxGame 코드: `Source/WxGame/Controller/WxAIController.cpp`·`.h`, `Source/WxGame/Character/WxCharacterBase.cpp`
  - 대조한 UE 5.8 엔진 소스: `BehaviorTreeComponent.cpp`, `BehaviorTreeTypes.cpp`, `BTCompositeNode.cpp`, `BTDecorator.cpp`, `BTNode.cpp`, `AIPerceptionComponent.cpp`, `AISense_Hearing.cpp`, `AISense_Sight.cpp`, `AIPerceptionTypes.h`, `AIInterfaces.cpp`, `GenericTeamAgentInterface.h`, `Pawn.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`와 대응 헤더, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxTeamTypes.h`
- **미검토 / 한계**
  - 실행 확인: 빌드·PIE·네트워크 실행 없이 코드만 읽었다.
  - 에셋: 노드 클래스명과 애님 노티파이 참조만 문자열로 확인했다. RandomChoice·RandomWeight·LockOn·BeyondLeash·UpdateTargetDistance 는 BT 3종이 쓰고, UpdateTargetActor·ReturnHome 은 2종이 쓴다. MirrorAbility·MirrorMovement·AttributeRatio 는 어느 에셋에도 없다. 가중치·Observer aborts 같은 노드 속성 값은 보지 못했다.
  - 직전 리뷰(`a5f165c2`) 이후 달라진 점은 세 가지다.
    - 직전 2번(빙의 해제 때 폰이 시야 자극원에서 빠짐)은 해소돼 뺐다. `AWxAIController::OnUnPossess` 가 이제 `Super::OnUnPossess()` 뒤에 퍼셉션을 언레지스터한다(`Source/WxGame/Controller/WxAIController.cpp:141~148`). 이 시점엔 `GetBodyActor()` 가 null 이라 `CleanUp` 이 자극원을 빼지 않는다(엔진 `AIPerceptionComponent.cpp:266~281`, `:460~469`).
    - `9158384b` 의 감각 getter 제거는 남은 참조가 없음을 확인했다.
    - 4번은 컨트롤러의 피아 판정 위임 뒤에 새로 세운 발견이다.
  - 새로 세웠다가 기각한 가설
    - `ReturnHome` 이 TargetActor 가 빈 채 실행되면 `ForgetActor(nullptr)` 로 역참조가 난다 — 5.8 `ForgetActor` 가 null 을 거른다(`AIPerceptionComponent.cpp:679~697`).
    - 재빙의 때 `ApplySenseSettings` 가 언레지스터된 퍼셉션에 설정을 넣어 반영되지 않는다 — `ConfigureSense` 는 미등록이면 config 만 바꾸고(`AIPerceptionComponent.cpp:122~158`), 뒤따르는 `RegisterComponent` 의 `OnRegister` 가 그 값으로 리스너를 만든다(`:182~211`).
    - StopTree 에서 영속 스냅숏까지 소멸자가 돌아 `Trail` 이 이중 해제된다 — `FBehaviorTreeInstance::Cleanup(Destroy)` 는 스냅숏을 소멸 없이 비운다(`BehaviorTreeTypes.cpp:114~118`).
  - 판정을 유지한 기각 항목: 직전 리뷰가 기각한 항목은 해당 코드가 그대로라 판정을 유지한다. MirrorAbility 의 Aborting 중 마감, 발동 프로토콜·감속 GE·해제 절차 중복, 스티키 타겟, 가중치 전원 0 승계, Patrol 인스턴스 재빙의, 서비스 교체 순서, ASC 구독 수명, BeyondLeash 메모리 초기화, `Trail[0]` 접근, ReturnHome 실패 후 BeyondLeash 시드, LockOn·MirrorMovement 적용 기록 롤백, 완주 후 Patrol abort 가 여기에 해당한다.
  - 발견으로 올리지 않은 것: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:12` 의 익명 namespace 는 프로젝트의 코드 작성 선호와 어긋난다. 다만 CLAUDE.md 규칙이 아니고, 8곳이 공유하는 진단 헬퍼라 발견으로 올리지 않았다.

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 36파일 — `/module-review`로 갱신*
