# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹은 설계가 잘 지켜져 있고, 지난 리뷰에서 지적한 12건 중 10건(타겟 소실 미감지, `SetTargetActor` 조기 반환, 센스 등록 경로, avoid-repeat 실패, `MaxWalkSpeed` 소유권 이중화, 어빌리티 리스트 스코프 락, ReturnHome 인스턴싱, Wander 힙 할당, 스테일 주석 2건)이 실제로 해소됐다. 남은 문제는 모두 "관찰자(observer)를 등록하지 않아 상태 변화가 재판정으로 이어지지 않는" 한 갈래다. 커버리지: 소스 29파일 전부를 읽었고 `WxAIPerceptionComponent`·`WxBTComposite_RandomChoice`·`WxBTTask_ActivateAbility`·MoveTo 파생 태스크·리시 한 쌍은 cpp 로직까지 정독했으며, UE 5.8 `AIModule` 원본(`BTCompositeNode`·`BehaviorTreeComponent`·`AIPerceptionComponent`·`AISense_Sight`)과 소비자(`AWxEnemyController`·`UWxNameplateComponent`·`AWxEnemyCharacter`·`UWxEffect_MoveSpeedScale`)에 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 억제 해제·타겟 부활 뒤, 계속 보이는 대상을 영영 다시 잡지 못할 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:139-145`, `:150-154`, `:72-75`
- **범주**: 버그/정확성
- **문제**: 두 해제 경로가 모두 "다음 감지 자극에서 정상적으로 재획득한다"는 주석과 함께 상태를 그냥 비워 둔다(억제 해제 `:140`, 타겟 부활 `:150`). 그런데 `UAISense_Sight` 는 `NotifyType = EAISenseNotifyType::OnPerceptionChange` 로 등록돼 있어(UE 5.8 `AISense_Sight.cpp:159`, 소비는 `AIPerceptionComponent.cpp:545-546`), **가시성이 바뀌는 순간에만** `OnTargetPerceptionUpdated` 를 방송한다. 반면 억제 중에는 `HandleTargetPerceptionUpdated` 가 첫 줄에서 통째로 반환하므로(`:72-75`) 그 사이의 가시성 변화는 컴포넌트 입장에서 소실된다. 결과적으로 "억제 구간 내내 계속 보이던(=가시성이 한 번도 바뀌지 않은) 대상"은 억제가 풀려도 새 자극을 만들지 않아 재획득되지 않는다 — 폰은 집 앞에서 눈앞의 플레이어를 무시한 채 서 있게 되고, 플레이어가 시야에서 빠졌다 들어오거나 소리·피해를 줄 때까지 자력 복구되지 않는다. 부활 경로(`:151`)도 같은 구조라, 시야 안에서 되살아난 대상은 재획득 대상이 되지 못한다.
- **제안**: 억제 해제(`SetTargetingSuppressed(false)`)와 부활 처리에서 자극을 기다리지 말고 현재 지각 목록을 직접 되짚는다 — 엔진의 `GetCurrentlyPerceivedActors` / `GetPerceptualDataConstIterator` 로 살아 있는 적대 대상을 하나 골라 `SetTargetActor` 에 넘기고 `UpdateRecognition` 을 한 번 돌리면, 판정 단일 지점 구조를 유지한 채 자극 의존을 끊을 수 있다.
- **확신도**: 중간 (메커니즘은 엔진 원본으로 확정. 다만 복귀 중 폰이 홈 방향으로 회전하며 대개 시야가 한 번 끊기고, 발소리 `UWxAnimNotify_ReportNoise` 의 청각은 `OnEveryPerception` 이라 실전에서는 상당 부분 가려진다)

### 2. 🟡 RandomChoice 의 사전 필터가 조건 Decorator 의 FlowAbortMode 를 무력화한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:61-64`, `:89-92`
- **범주**: 버그/정확성
- **문제**: 엔진 `UBTCompositeNode::FindChildToExecute` 는 조건이 막은 자식마다 `NotifyDecoratorsOnFailedActivation` 을 부르고, 그 안에서 `LowerPriority`/`Both` FlowAbortMode 를 가진 Decorator 를 aux 옵저버로 등록한다(UE 5.8 `BTCompositeNode.cpp:52-61`, `:307-311` 의 `AddUniqueUpdate(..., EBTNodeUpdateMode::Add)`). 반면 `GetNextChildHandler` 의 사전 필터는 조건 false 인 자식을 `continue` 로 건너뛸 뿐이고(`:61-64`), 선택된 자식이 반환되면 엔진 루프는 그 자식에서 즉시 `break` 하므로 걸러진 자식들에 대한 등록이 전혀 일어나지 않는다. 후보가 0개라 `ReturnToParent` 를 반환하는 경로(`:89-92`)도 마찬가지다. 결과: RandomChoice 아래 자식에 조건 데코를 달고 FlowAbortMode 를 Lower Priority 로 지정해도 "조건이 참으로 뒤집히는 순간 진행 중인 하위 브랜치를 선점"하는 구성이 이 컴포지트 아래에서만 조용히 죽는다. 평범한 Selector 였다면 동작한다. 3번과 짝을 이루며, 어트리뷰트 기반 선점을 만들려면 둘 다 고쳐야 한다.
- **제안**: 필터에서 제외한 인덱스마다 `NotifyDecoratorsOnFailedActivation(SearchData, Index, LastResult)` 를 불러준다(`UBTCompositeNode` 의 protected 멤버라 파생 클래스에서 호출 가능 — `BTCompositeNode.h:258`). 지원할 의사가 없다면 `WxBTComposite_RandomChoice.h` 시멘틱 주석에 "이 컴포지트 자식의 조건 데코는 FlowAbortMode 가 동작하지 않는다"를 규약으로 못박는다.
- **확신도**: 중간 (동작은 엔진 원본으로 확정. 현재 BT 에셋이 이 구성을 실제로 쓰는지는 미확인)

### 3. 🟡 AttributeRatio 는 FlowAbortMode 를 켜도 실시간 재평가가 일어나지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16`
- **범주**: 설계/구조
- **문제**: 헤더가 "실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다"고 안내하지만, 이 Decorator 는 `CalculateRawConditionValue` 만 구현하고 `OnBecomeRelevant`/`TickNode`/어트리뷰트 변경 구독이 전혀 없다(cpp 전체가 생성자·`GetStaticDescription`·조건 계산 3개뿐). FlowAbortMode 는 "abort 를 허용하는 범위"만 정할 뿐 재평가를 촉발하지 않으므로, HP 가 임계값을 넘어도 다른 원인으로 BT 재탐색이 일어나기 전까지 조건은 갱신되지 않는다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 정확히 이 한계 때문에 `TickNode` 폴링 + `RequestExecution` 을 직접 구현했다(`Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:51-63`). 디자이너가 헤더 안내대로 FlowAbortMode 만 켜고 "HP 50% 가 되면 즉시 발악기"를 기대하면 조용히 어긋난다.
- **제안**: BeyondLeash 처럼 관찰자 경로를 구현한다 — `OnBecomeRelevant` 에서 대상 ASC 의 두 어트리뷰트 변경 델리게이트를 구독해 비율 판정이 뒤집힐 때만 `RequestExecution` 을 호출하고 `OnCeaseRelevant` 에서 해제한다(틱 폴링보다 저렴하다). 구현할 계획이 없다면 헤더 문구를 "재탐색 시점에만 재평가된다"로 정정한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp` (+ 대응 헤더)
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/` 헤더 15개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`. 대조용으로 `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_MoveSpeedScale.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`.
- **확인 결과 문제 없던 항목**:
  - 타겟 소실 처리(`BindTargetLoss`/`UnbindTargetLoss`)의 대칭성. `OnEndPlay` 브로드캐스트 도중 `RemoveDynamic`, ASC 태그 델리게이트 브로드캐스트 도중 `UnregisterGameplayTagEvent` 둘 다 엔진 멀티캐스트가 락/사본으로 방어하는 경로라 안전하다. `AppliedTarget` 을 `TObjectKey` 로 든 것도 BB 약참조 자체 무효화를 정확히 회피한다.
  - `HandleTargetPerceptionUpdated` 의 `Actor` 널 가능성 — 엔진이 `SourceActor == nullptr` 이면 방송 자체를 하지 않는다(`AIPerceptionComponent.cpp:584-595`).
  - 자기 폰 사망 시 `State.InCombat` 잔존은 의도된 설계이며 실제로 관측되지 않는다. 소비자 두 곳이 각자 사망을 먼저 걸러낸다 — 네임플레이트는 `IgnoreTags` 에 `State.Dead`(`WxNameplateComponent.cpp:24`), 처형/뒤잡 자격은 `IsAlive()` 선검사(`WxEnemyCharacter.cpp:85`).
  - 감속 GE 수명. `MoveSpeedEffect` 는 `OnTaskFinished` 에서 제거되고, 엔진은 즉시 실패·중단·`StopTree`(사망 시 `AWxEnemyController::HandlePawnDeath`) 어느 경로에서도 `OnTaskFinished` 를 보장한다(UE 5.8 `BehaviorTreeComponent.cpp:379-400`). 억제 플래그 해제도 같은 경로를 탄다.
  - `UWxBTTask_ActivateAbility` 의 latent 수명주기: 순회 스코프 락, 활성화 후 핸들 재조회, 동기 종료 시 `InProgress` 영구 정지 방지, Abort 경로에서 델리게이트 선해제 후 취소 — 누락·중복 없음.
  - `UWxBTComposite_RandomChoice` 의 노드 메모리 레이아웃(`FBTCompositeMemory` 상속 + `GetInstanceMemorySize` 일치), 룰렛 부동소수 폴백(`FRand()` 가 1.0 을 낼 수 있어 `Candidates.Last()` 폴백이 필요하다), 회피/가중치 0 필터의 인덱스 정합, `UBTComposite_Selector` 상속이 `GetNextChildHandler` 오버라이드를 가리지 않음.
  - `UWxBTDecorator_BeyondLeash` 의 옵저버 수명: Lower Priority 는 브랜치 활성 시 엔진이 aux 에서 **제거**하므로(`NotifyDecoratorsOnActivation`) 복귀 중 자기중단이 구조적으로 불가능하다 — 헤더가 금지한 Self/Both 왕복 문제가 코드 차원에서도 맞물린다.
  - `UWxPatrolComponent::GetNextIndex` 의 세 MoveMode 경계값(닫힌 스플라인에서 `GetNumberOfSplinePoints` 가 중복 끝점을 제외한다는 엔진 계약 포함), `GetPointLocation` 의 인덱스 클램프, `FindPatrolComponent` 의 Owner-우선 조회.
  - 권한 모델: 퍼셉션·BT·소음 보고 모두 서버 전용이고 `UWxAnimNotify_ReportNoise` 는 `HasAuthority` 게이팅, 인식은 MinimalReplication 태그로만 클라에 전파 — 권위 위반 없음.
- **규칙 준수 확인(위반 0건)**: 29개 소스 전부 첫 줄 저작권 표기 정상 / `Wx` prefix 전면 준수 / `BlueprintCallable` 0건 / `FORCEINLINE`·헤더 인라인 정의 0건 / 람다 0건 / 델리게이트 콜백 4종(`HandleTargetPerceptionUpdated`, `HandleTargetDeathTagChanged`, `HandleTargetEndPlay`, `HandleAbilityEnded`) 모두 `Handle` prefix / 라이프사이클 override 의 `Super::` 호출 누락 0건 / Wx 플러그인 의존은 `WxCore` 하나(`.Build.cs`·`.uplugin` 동일)
- **미검토 / 한계**:
  - BehaviorTree/Blackboard `.uasset` 자체는 보지 않았다. README 가 규정한 "BeyondLeash 의 FlowAbortMode = Lower Priority", "복귀 브랜치가 전투 브랜치보다 상위 우선순위", "Blackboard 에셋에 5개 키가 동명·동타입 등록" 같은 에셋 측 규약이 실제로 지켜지는지는 C++ 만으로 확인 불가다. 2·3번의 실제 체감 크기도 현재 BT 가 조건 데코에 FlowAbortMode 를 걸어 쓰는지에 달려 있다.
  - `WxBlackboardKeys` 의 비-Shipping 진단(`VerifyBlackboardKey`)이 매 accessor 호출마다 도는 `GetKeyID` FName 선형 탐색 비용(`UWxBTDecorator_BeyondLeash::TickNode` 가 매 프레임 호출)은 인지했으나, 개발 빌드 한정이고 실측하지 않아 지적에 넣지 않았다. 줄일 여지는 `OnBecomeRelevant` 에서 `HomeLocation` 을 노드 메모리에 1회 캐시하는 것이다.
  - `MoveSpeedMultiplier` 의 `ClampMin = 0.0` 은 0 을 허용해 SPD 를 0 으로 만들 수 있으나, 엔진 `UPathFollowingComponent` 의 block detection 이 MoveTo 를 Blocked 로 종료시켜 영구 정지로는 이어지지 않는다 — 명백한 오설정이라 지적에 넣지 않았다.
  - `Once` 로 정찰을 마친 `UWxBTTask_Patrol` 이 이동 없이 즉시 `Succeeded` 를 반환하는 거동(`WxBTTask_Patrol.cpp:42-45`)은 상위 배치가 `[Patrol -> Wait]` 시퀀스가 아닐 경우 매 틱 BT 재탐색을 유발한다. 에셋 배치에 달린 문제라 판단을 보류했다.
  - `NavigationSystem`·`GameplayTasks` 모듈 의존은 WxAI 소스에서 직접 쓰이지 않으나 `AIModule` 의 전이 의존이라 무해해 지적하지 않았다.
  - 멀티플레이 실환경(전용 서버) 실행 검증, 리시 왕복·타겟 진동의 실제 플레이 확인은 정적 분석 범위 밖이다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 29파일 — `/module-review`로 갱신*
