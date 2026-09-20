# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 노드 인스턴스화(`bCreateNodeInstance`)와 노드 메모리 사용 구분, 델리게이트 구독/해제 짝, 취소 거부 어빌리티·빙의 해제 같은 엔진 함정에 대한 방어가 일관되게 들어가 있고, 모듈 경계(`WxCore` 외 Wx 플러그인 무참조)도 `.Build.cs`와 실제 include 양쪽에서 지켜진다. 이번 리뷰는 `Public/`·`Private/` 전 파일 헤더와 BT 노드 14종·컴포넌트 2종·AnimNotify·테스트 2종의 cpp 구현까지 읽었고, MirrorMovement/MirrorAbility/ActivateAbility/RandomChoice 등 복잡도가 높은 파일은 엔진 원본(UE 5.8 `AIModule`)과 대조해 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 MirrorMovement 가 MaxWalkSpeed 를 직접 쓰고 복원하지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:140-141` (쓰기), `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:37-56` (`Release`)
- **범주**: 설계/구조
- **문제**: 이 프로젝트에서 `MaxWalkSpeed`/`MaxWalkSpeedCrouched` 의 주인은 SPD 어트리뷰트 콜백이다 — `Source/WxGame/Character/WxCharacterBase.cpp:198-212` 가 `클래스 기본값 × SPD` 로 매번 다시 계산한다. `UWxBTTask_Patrol`·`UWxBTTask_Wander` 는 바로 이 충돌을 피하려고 GE(`MoveSpeedEffect`)로 우회했는데(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:55` 주석), 이 서비스만 매 틱 `Master 의 MaxWalkSpeed × 1.25` 를 직접 덮어쓴다. 게다가 `Release()` 가 이 값을 되돌리지 않아, 브랜치를 벗어난 뒤에도 분신은 다음 SPD 변경이 올 때까지 마스터 기준 1.25배 속도로 남는다(분신이 전투 브랜치로 넘어간 뒤에도 유지된다).
- **제안**: Patrol/Wander 와 같은 방식으로 이동 속도 배율 GE(`SetByCaller` 배율)를 부여/제거하거나, 최소한 `Release()` 에서 컴포넌트 아키타입 값으로 복원한다(`UWxBTService_LockOn::ReleaseLockOn` 이 회전 모드에 쓰는 패턴과 동일).
- **확신도**: 높음

### 2. 🟡 Master 반응 분기 사전 판정이 데코레이터의 Inverse Condition 을 무시한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:128`
- **범주**: 버그/정확성
- **문제**: `Reaction->CalculateRawConditionValue(*BT, nullptr)` 를 직접 부른다. 엔진이 실제 실행 가부를 정할 때 쓰는 `UBTDecorator::WrappedCanExecute` 는 `IsInversed() != CalculateRawConditionValue(...)` 로 반전을 반영한다(엔진 `BTDecorator.cpp:46-50`). 따라서 저작자가 `UWxBTDecorator_MasterAbility` 에 "Inverse Condition" 을 켜면 이 사전 판정과 엔진의 최종 판정이 정반대가 된다 — 반응해야 할 분기에 `RequestExecution` 이 나가지 않거나, 반대로 실행되지 못할 분기 때문에 트리를 재탐색한다.
- **제안**: `Reaction->WrappedCanExecute(*BT, nullptr)` 로 바꾼다(public API이고, 이 데코레이터는 비인스턴스라 `NodeMemory` 가 nullptr 이어도 안전하다).
- **확신도**: 중간 (반전을 쓰지 않는 저작 관례를 전제했을 수 있음)

### 3. 🟡 플러그인 자동화 테스트가 `/Game` 콘텐츠 에셋과 private 멤버에 묶여 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/Tests/WxMirrorMovementAbilityTeleportTest.cpp:25-27`, `Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:42`
- **범주**: 설계/구조
- **문제**: 테스트가 특정 캐릭터의 스킬 BP(`/Game/Character/HGTest/Abilities/Skill_3/GA_HGTest_Skill_3`)를 `LoadClass` 로 직접 물고 있다. 검증 대상은 "`FaceMasterAbilityTags` 와 겹치는 AssetTags 를 가진 어빌리티가 발동하면 전방 순간이동" 이라는 로직뿐이라 실제 스킬 에셋이 필요 없는데, 콘텐츠가 이동·개명되면 플러그인 테스트가 깨진다(플러그인 → 게임 콘텐츠 역방향 의존). 또한 런타임 헤더가 테스트 클래스를 `friend` 로 알고 있어, 테스트 이름을 바꾸면 런타임 헤더를 고쳐야 한다.
- **제안**: 테스트 파일 안에 `AbilityTags` 를 생성자에서 채우는 최소 `UGameplayAbility` 파생 테스트 클래스를 두고 그것으로 검증한다. `friend` 는 유지하더라도 테스트 대역을 콘텐츠에서 떼는 것이 우선이다.
- **확신도**: 높음

### 4. 🟡 `Ability` 루트 태그를 문자열로 조회한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:114`
- **범주**: 규칙 위반
- **문제**: `FGameplayTag::RequestGameplayTag(TEXT("Ability"))` 로 매 Master 어빌리티 발동마다 문자열 조회를 한다. 같은 태그가 이미 `WxGameplayTags::Ability` 로 선언돼 있고(`Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:188`, 정의는 `WxGameplayTags.cpp:85`), 이 모듈의 다른 파일들은 모두 그 상수를 쓴다. 태그 이름이 바뀌면 여기만 런타임 에러로 떨어진다.
- **제안**: `WxGameplayTags::Ability` 로 교체하고 지역 변수도 제거한다.
- **확신도**: 높음

### 5. 🟢 데코레이터 4종이 `GetStaticDescription` 에서 `Super::` 를 호출하지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:18`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:13`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:46`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_MasterAbility.cpp:30`
- **범주**: 규칙 위반
- **문제**: `UBTDecorator::GetStaticDescription` 은 `aborts lower priority` / `inversed` 표기를 만들어 붙인다(엔진 `BTDecorator.cpp:122-130`). Super 를 빼면 BT 그래프에서 그 표기가 사라지는데, `UWxBTDecorator_BeyondLeash`·`UWxBTDecorator_MasterAbility` 는 생성자에서 `FlowAbortMode = LowerPriority` 를 켜고 있어 저작자가 가장 알아야 할 정보가 안 보인다. 같은 함정을 `UWxBTService_LockOn` 헤더 주석(`Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h:44`)이 이미 경고하고 있고, `UWxBTDecorator_ObserveAbility`(`WxBTDecorator_ObserveAbility.cpp:30`)만 제대로 Super 를 붙인다.
- **제안**: `ObserveAbility` 처럼 `Super::GetStaticDescription()` 을 앞에 붙인다.
- **확신도**: 높음

### 6. 🟢 Blackboard 키 이름 `"Master"` 가 노드 생성자에 리터럴로 중복돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:20`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:21`
- **범주**: 중복/복잡도
- **문제**: `WxBlackboardKeys` 가 키 이름의 단일 계약인데(README 와 `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:11-17`), 두 노드의 기본 키만 `TEXT("Master")` 리터럴이다. 키 이름을 바꾸면 컴파일은 통과하고 기본값만 조용히 어긋난다. 같은 자리에서 `UWxBTDecorator_BeyondLeash`(`WxBTDecorator_BeyondLeash.cpp:22`)·`UWxBTTask_ReturnHome`(`WxBTTask_ReturnHome.cpp:15`)·`UWxBTTask_Patrol`(`WxBTTask_Patrol.cpp:27`)은 상수를 쓴다.
- **제안**: `MirrorTarget.SelectedKeyName = WxBlackboardKeys::Master;` 로 통일한다.
- **확신도**: 높음

### 7. 🟢 Master 키 진단이 WxBlackboardKeys 의 진단과 겹치고 원인을 알려주지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:32-38`
- **범주**: 중복/복잡도
- **문제**: 키 부재와 타입 불일치를 한 조건으로 묶어 `Error` 로그 한 줄에 이름들만 찍는다 — 무엇이 잘못됐는지(키가 없는지, 타입이 다른지) 읽는 쪽에서 판단해야 한다. `WxBlackboardKeys.cpp:15-33` 의 `VerifyBlackboardKey` 가 이미 두 경우를 나눠 사람이 읽을 수 있는 경고를 내고 있어 진단이 두 벌이다.
- **제안**: 값 조회를 `WxBlackboardKeys::GetMaster(BB)` 로 바꿔 진단을 accessor 한 곳으로 모으고, 이 블록은 관찰자 등록에 필요한 KeyID 유효성 확인만 남긴다.
- **확신도**: 중간

### 8. 🟢 `ForgetActor` 주석의 근거가 엔진 동작과 다르다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:48-49`
- **범주**: 버그/정확성
- **문제**: 주석은 "청각·촉각 자극은 MaxAge 안에 남아 있어, 자격을 되찾는 순간 그대로 어그로가 된다" 고 적었지만, `UAIPerceptionComponent::ForgetActor` 는 `PerceptualData.Remove(ActorToForget)` 로 그 액터의 인식 정보(누적 자극 포함)를 통째로 지운다(엔진 `AIPerceptionComponent.cpp:679-697`). 시야는 `UAISense_Sight::OnListenerForgetsActor` 가 쿼리 결과를 리셋해 곧 재감지되지만(엔진 `AISense_Sight.cpp:1074-1086`), 소리·피격만으로 잡았던 대상은 `Effect_IgnoreAggro` 가 풀려도 새 자극이 오기 전까지 다시 잡히지 않는다. 실제 동작 자체는 의도일 수 있으나 주석의 전제가 틀려 다음 수정자가 오판하기 쉽다.
- **제안**: 주석을 실제 동작(감지 기록 전체 삭제 · 시야는 재감지, 청각/촉각은 새 자극 필요)으로 정정한다. 청각 전용 어그로 복귀가 필요하면 어그로 시스템 작업에서 함께 다룬다.
- **확신도**: 중간 (동작은 의도일 수 있고, 주석만 틀렸을 가능성이 큼)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_FollowMasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Public/` 전 헤더(22개), `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_MasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Private/Tests/*.cpp`
- **확인한 규칙 점검**: 소스 46개 전부 첫 줄 저작권 표기 통과 · `FORCEINLINE`/인라인 정의 0건 · `BlueprintCallable` 남용 0건(`UFUNCTION`은 `ReceiveControllerChangedDelegate` 바인딩용 1건) · 델리게이트 콜백 `Handle` prefix 전부 준수 · `.Build.cs`·include 모두 `WxCore` 외 Wx 플러그인 참조 없음
- **미검토 / 한계**:
  - BT/BB 에셋과 캐릭터 BP 는 범위 밖이라, "저작 누락 시 조용히 기본 동작으로 떨어지는" 지점(`MoveSpeedEffect` 미지정, `AttributeRatio` 의 Attribute 미지정, 최상위 Composite 에 `ObserveMasterAbility` 배치 여부)이 실제 에셋에서 지켜지는지는 확인하지 못했다.
  - `UWxBTComposite_RandomChoice` 가 후보를 사전 필터한 뒤 `BlockedChild` 를 반환하면 그 자식에 대해 `NotifyDecoratorsOnFailedActivation` 이 엔진 경로에서 한 번 더 불린다. `SearchData.AddUniqueUpdate` 라 무해하다고 판단해 발견으로 올리지 않았으나, 관찰자 등록이 중요한 트리에서 실기기 검증은 못 했다.
  - `UWxBTDecorator_BeyondLeash`·`UWxBTService_MirrorMovement` 의 매 프레임 틱 비용은 코드상 과하지 않다고 봤을 뿐, 다수 AI 동시 구동 프로파일링은 하지 않았다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 46파일 — `/module-review`로 갱신*
