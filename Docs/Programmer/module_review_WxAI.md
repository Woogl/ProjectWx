# WxAI — 코드 리뷰

> 28개 파일 약 2,100줄의 소형 모듈이고 상태가 매우 양호하다. 델리게이트 해제·노드 인스턴스 상태 격리·엔진 API 계약(`NavigationRaycast` 반환 의미, `GetTaskStatus` 의 인스턴스 노드 처리, `FBTCompositeMemory` 파생 패턴)이 모두 정확하며, 프로젝트 코딩·모듈 규칙 위반은 한 건도 없다. 이번 리뷰는 퍼셉션 컴포넌트와 BT 노드 cpp 전부를 읽고, 의심스러운 지점은 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 빙의 해제 경로에서 이전 폰의 회전 모드가 strafe 로 고착된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:169`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:272`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged` 는 `SetTargetActor(nullptr)` 로 상태를 되돌리지만, 회전 모드 원복 블록은 `AIC->GetPawn()` 을 본다. 엔진은 `SetPawn(nullptr)` 뒤에 `OnPossessedPawnChanged.Broadcast(OldPawn, nullptr)` 를 쏘므로(`AController::UnPossess`), 이 시점의 `GetPawn()` 은 이미 null 이고 `if (!Movement) return;`(`:274`)에서 빠져나간다. 결과적으로 방금 놓아준 폰은 `bOrientRotationToMovement=false` / `bUseControllerDesiredRotation=true` 인 채 남는다. 포커스는 컨트롤러 소유라 같은 커밋에서 폰 가드 밖으로 빼내 고쳤는데, 폰에 남는 회전 모드는 그 처리를 받지 못했다.
  그 폰이 다시 빙의되면 새 컨트롤러의 퍼셉션이 `HandlePossessedPawnChanged(nullptr, NewPawn)` 로 `SetTargetActor(nullptr)` 을 호출하지만, `AppliedTarget` 도 `NewTarget` 도 null 이라 `:246` 의 중복 가드에 걸려 그대로 반환한다 — 즉 재빙의로도 복구되지 않는다. `AWxAIController::OnPossess` 의 "재사용된 폰도 새 빙의에서는 타겟 없이 시작한다" 주석대로 폰 재사용이 전제된 코드베이스라 실제로 닿을 수 있는 경로다. 증상은 정찰·배회 중인 폰이 진행 방향이 아니라 컨트롤러 회전을 계속 바라보는 것이다.
- **제안**: `HandlePossessedPawnChanged` 가 이미 받고 있는 `OldPawn` 으로 회전 모드를 아키타입 기본값으로 되돌린 뒤 `SetTargetActor(nullptr)` 을 호출한다. 같은 이유로 `EndPlay`(`:81`)에서 폰이 살아 있는 채 컨트롤러만 사라지는 경로도 함께 본다.
- **확신도**: 중간

### 2. 🟡 Patrol·Wander 의 감속 GE 부여/제거가 통째로 중복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:87`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:73`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:125`
- **범주**: 중복/복잡도
- **문제**: 두 태스크가 `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 쌍(`WxBTTask_Patrol.h:38`, `WxBTTask_Wander.h:61`), `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller_MoveSpeedScale)` → `ApplyGameplayEffectSpecToSelf` 부여 블록, `OnTaskFinished` 의 제거 블록, `GetStaticDescription` 의 속도 표기까지 문자 단위로 같은 코드를 각자 들고 있다. 한쪽에서만 고친 수정(예: 핸들 유효성 처리, SetByCaller 태그 변경)이 다른 쪽에 전파되지 않고, 세 번째 "감속 이동" 노드가 생기면 또 복제된다.
- **제안**: `UBTTaskNode` 파생 중간 베이스를 새로 만드는 대신, GE 부여/제거 두 조각만 `WxAI` 내부 자유 함수(예: `Private` 의 이름 없는 네임스페이스가 아니라 공유 헤더)로 빼는 정도가 침습이 적다. 다만 프로젝트가 "중복 제거는 최소 인플레이스, 약간의 반복 용인" 을 택해 왔으므로, 현행 유지도 유효한 선택이다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 Wander: 이동 컴포넌트가 없으면 내비 검증이 무력화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:42`
- **범주**: 버그/정확성
- **문제**: `TravelDistance` 는 `Movement` 가 null 이면 `0.f` 가 된다. 그러면 `:55` 의 `NavigationRaycast(Pawn, NavStart, NavStart + Candidate * 0)` 는 시작점과 끝점이 같은 길이 0 레이라 막힘으로 판정되지 않고, 첫 후보 방향이 검증 없이 그대로 채택된다. 클래스 주석이 선언한 "내비메시가 없는 맵에서는 배회하지 않는다" 계약과 이 한 경로만 반대로 샌다. (`AddMovementInput` 도 무의미해지므로 실제 피해는 작다.)
- **제안**: `Movement` 가 없으면 방향 탐색에 들어가기 전에 `Failed` 로 마감한다.
- **확신도**: 중간

### 4. 🟢 `bAvoidRepeat` 의 `LastChosenChild` 가 엔진의 탐색 재개 경로에서 갱신되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:125`
- **범주**: 버그/정확성
- **문제**: `LastChosenChild` 는 `GetNextChildHandler` 안에서만 기록된다. 그런데 `UBTCompositeNode::GetNextChild` 는 `SearchData.SearchStart` 로 특정 노드에서 탐색을 재개하거나 `OverrideChild` 가 걸린 경우 핸들러를 거치지 않고 자식을 직접 고른다(UE 5.8 `BTCompositeNode.cpp`). 그 경로로 실행된 자식은 기록에 남지 않아, 다음 추첨에서 회피 기준이 실제 직전 자식과 어긋난다. 회피가 한 번 헛도는 정도라 영향은 작다.
- **제안**: 고칠 값어치가 크지 않다. 회피 정확도가 중요해지면 `NotifyChildExecution` 오버라이드에서 실제 실행된 자식 인덱스를 기록하는 쪽이 누락 없는 지점이다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전체, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`
- **교차 확인**: 모듈 경계(`WxCore` 외 Wx 의존 없음 — Build.cs·uplugin 모두), 코딩 규칙 6종(`Wx` prefix, 저작권 첫 줄, 람다, `Handle` prefix, `BlueprintCallable`, 인라인 정의) 전수 grep 결과 위반 0건. Blackboard accessor 호출부는 전부 null 가드를 거치며, `SetSelfActor`/`SetMaster`/`SetHomeLocation` 은 `Source/WxGame/Controller/WxAIController.cpp` 가 실제로 쓴다(데드 코드 없음). `Event.Hit` 페이로드가 `ContextHandle`·`EventMagnitude` 를 채운다는 전제는 `Plugins/WxCombat/.../WxCombatAttributeSet.cpp:310` 에서 확인했고, 대미지 없는 `Event.Hit.Parry` 를 자극에서 빼는 가드도 실제 발행부와 일치한다.
- **미검토 / 한계**: BT·Blackboard 에셋과 실제 트리 구성은 범위 밖이라, `WxBTComposite_RandomChoice` 가 문서에서 경고한 "LowerPriority 데코가 붙은 뒤쪽 자식이 추첨 결과를 끊는" 상황이 현재 트리에서 실제로 발생하는지는 확인하지 않았다. `UWxBTTask_Patrol` 이 갈 지점이 없을 때 `InProgress` 로 브랜치를 점유하는 설계는 헤더에 명시된 의도로 보고 검증하지 않았다. 멀티플레이 시나리오(서버-클라 권위)는 BT·퍼셉션이 서버 전용이라는 전제만 확인했고 실제 복제 동작은 검증하지 않았다.

---
*문서 기준 커밋 `79bab788` · 리뷰일 2026-09-03 · 소스 28파일 — `/module-review`로 갱신*
