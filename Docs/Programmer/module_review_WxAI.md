# WxAI — 코드 리뷰

> 모듈 건강도는 양호하다. 노드 인스턴스 메모리·aux 노드 수명주기·엔진 훅 계약을 정확히 다루고 실패 경로마다 명시적 분기와 주석이 붙어 있으며, 플러그인 의존은 `WxCore` 하나뿐이고 `.claude/CLAUDE.md` 코딩/모듈 규칙 위반은 한 건도 없다. 직전 리뷰(`04420d246`) 이후 C++ 소스는 한 줄도 바뀌지 않아(변경은 `Plugins/WxAI/README.md` 뿐) 이번은 재검증 패스이며, README → `.uplugin`/`Build.cs` → Public 헤더 19개 전부 → cpp 19개 전부를 다시 읽고 특히 StateTree 대신 이 모듈이 쓰는 BT 노드·Perception·AIController 의 수명주기와 실패 경로를 새로 파는 데 시간을 썼다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 MirrorAbility 의 TickTask 가 Abort 중에도 돌아 `Succeeded` 로 마감한다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:176`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:220`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:280`
- **범주**: 버그/정확성
- **문제**: 엔진은 `ActiveNodeType` 이 `ActiveTask` 일 때뿐 아니라 `AbortingTask` 일 때도 활성 태스크의 `TickTask` 를 돌린다. `AbortTask` 가 취소를 요청했지만 스코프 락 때문에 종료가 미뤄져 `InProgress` 를 반환하면(:176), 다음 프레임의 `TickTask` 가 그대로 실행된다. 이때 대상이 그 사이 태그를 놓았으면 :220 의 `FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded)` 로 마감해, abort 요청이 성공 결과로 바뀐다. 같은 파일의 `HandleAbilityEnded` 는 `GetTaskStatus(this) == EBTTaskStatus::Aborting` 을 보고 `FinishLatentAbort` 를 부르도록 정확히 처리하고 있어(:280) 두 종료 경로의 계약이 어긋나 있다. 창은 좁다 — 대상이 abort 대기 구간에 태그를 놓아야 한다. 현재 트리 구성에서는 엔진이 `bWasAborting` 을 보고 재탐색을 억제하므로 멈추지는 않지만, 부모가 결과 통지를 쓰는 컴포지트(예: SimpleParallel)면 abort 가 성공으로 읽힌다.
- **제안**: `TickTask` 의 마감 지점에도 `HandleAbilityEnded` 와 같은 Aborting 분기를 두어, abort 중이면 `FinishLatentAbort` 로 마감한다.
- **확신도**: 중간

### 2. 🟡 타겟이 이미 있으면 새 자극을 아예 보지 않아, 등 뒤에서 때리는 적으로 어그로가 넘어가지 않는다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:38`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:56`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:61`
- **범주**: 설계/구조
- **문제**: `TickNode` 은 현재 `TargetActor` 가 유효하고 살아 있으면 그 자리에서 반환한다(:38~:40). 그래서 재선정은 오직 "타겟이 죽거나 사라지거나 어그로 비허용으로 바뀔 때" 만 일어나고, 그 사이 들어온 자극은 전부 무시된다. `UWxAIPerceptionComponent` 가 Damage 센스를 직접 보고하는 경로(`WxAIPerceptionComponent.cpp:135`)를 굳이 만든 이유는 "시야·청각이 놓치는 가해자도 잡는다" 인데, 이미 교전 중인 폰은 뒤에서 맞아도 그 자극을 판단에 쓰지 못한다. 후보가 여럿일 때의 선택도 `GetCurrentlyPerceivedActors` 가 채운 배열의 첫 원소다(:56, :61) — 이 배열은 퍼셉션의 `PerceptualData` 맵 순회 순서라 거리·최근성과 무관하고, 그 임의의 첫 선택이 타겟이 무효해질 때까지 고정된다. 협동 플레이나 소환물이 끼는 상황에서 "가까이 붙어 때리는 쪽을 무시하고 멀리 있는 쪽만 본다" 가 재현될 수 있다.
- **제안**: 스티키 어그로를 유지하되 갱신 창을 하나 열어 둔다. 예: 최근 Damage 자극의 가해자는 현재 타겟보다 우선하거나, 유효 타겟이 있어도 일정 주기로 후보를 거리/최근 자극 시각으로 점수화해 비교한다. 지금 동작이 의도라면 헤더의 "타겟은 죽거나 사라지거나 어그로 비허용으로 바뀌면 갈린다" 옆에 "피격으로는 갈리지 않는다" 를 명시해 저작자가 오해하지 않게 한다.
- **확신도**: 중간

### 3. 🟡 어빌리티 발동·중단·종료 프로토콜이 두 태스크에 그대로 중복돼 있다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:41`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:102`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:72`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:249`
- **범주**: 중복/복잡도
- **문제**: 종료 델리게이트 선등록, 어빌리티 목록 잠금, 발동 핸들 선기록, 동기 종료·재발동 판별, 취소 요청 중 재진입 차단, 취소 거부 인스턴스 처리, 종료 시 구독 해제까지 약 80줄이 두 파일에 거의 문자 단위로 같다. 태그의 출처(고정 태그 vs 대상 소유 태그)와 대상 추적만 다르다. 이 중복이 실제로 위험하다는 증거가 발견 1이다 — 한쪽에만 있는 Aborting 가드가 다른 쪽 마감 경로에는 없다.
- **제안**: 두 태스크를 합치지는 말고, 발동·마감 프로토콜만 공통 베이스나 헬퍼로 올린다. 독립 구현을 유지한다면 "한쪽 수명주기를 고치면 반드시 다른 쪽도 같이 본다" 를 두 헤더에 규약으로 박아 둔다.
- **확신도**: 중간

### 4. 🟢 RandomChoice 가 "가중치 전원 0" 저작을 진단 없이 삼킨다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:98`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:106`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:26`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 설계/구조
- **문제**: 모든 자식이 조건을 통과했는데 가중치가 전부 0 이면 후보도 없고(:89 에서 전부 제외) 엔진에 되돌려줄 차단 자식도 없어 `BTSpecialChild::ReturnToParent` 로 빠지며(:98~:106), 부모는 직전 결과를 그대로 이어받는다. 헤더는 이 한계를 명시하지만(`RandomChoice.h:26`), 가중치 속성은 0 을 정상적인 "추첨 제외" 값으로 안내하므로(`RandomWeight.h:31`) 디자이너가 공격 몇 개를 임시로 빼다가 이 조합에 도달했는지 알아챌 단서가 런타임에 없다.
- **제안**: 런타임 선택 의미는 그대로 두고, 이 경로에 도달했을 때 (반복 억제한) 경고 로그나 에셋 검증을 붙인다.
- **확신도**: 중간

### 5. 🟢 같은 절차가 통째로 복제된 지점이 두 쌍 더 있다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:106`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:125`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:205`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:228`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:57`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:97`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:86`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:138`
- **범주**: 중복/복잡도
- **문제**: (가) 포커스 + CMC 회전 모드를 한 쌍으로 걸고 되돌리는 절차가 `UWxBTService_LockOn::ApplyLockOn`/`ReleaseLockOn` 과 `UWxBTService_MirrorMovement::ApplyFacing`/`ReleaseFacing` 에 있고, 해제 쪽(`LockOn.cpp:125` / `MirrorMovement.cpp:228`)은 아키타입 복원 로직까지 주석 문구 하나 다르지 않게 같다. (나) `MoveSpeedEffect` 의 SetByCaller 부여와 `OnTaskFinished` 제거가 Patrol/Wander 에 같은 모양으로 복제돼 있다. 둘 다 지금 동작에 결함은 없지만, 아키타입 복원이나 GE 제거 규칙을 고칠 때 한쪽만 고쳐질 여지가 있다.
- **제안**: 프로젝트 방침상 과한 구조 추출은 피하되, 최소한 (가)의 해제 절차만은 한곳으로 모으는 것을 검토한다. 유지한다면 짝 관계를 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 퍼셉션 센스 config 의 널 가능성을 같은 클래스 안에서 두 가지로 다룬다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:44`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:93`
- **범주**: 성능/안전
- **문제**: `PostInitProperties` 는 `SightConfig`/`HearingConfig`/`DamageConfig` 세 개를 모두 널 검사한 뒤 `ConfigureSense` 를 부르는데(:44~:57), 같은 클래스의 `ApplySenseSettings` 는 검사 없이 바로 역참조한다(:93~:97). 셋은 생성자의 `CreateDefaultSubobject` 가 보장하므로 실제로는 `PostInitProperties` 의 가드가 잉여지만, 코드만 보면 어느 쪽이 참인 불변식인지 판단할 수 없다. 만약 방어적 쪽이 옳다면 빙의 시점(`BeginPlay`/`HandlePossessedPawnChanged`)에 크래시가 나는 경로가 된다.
- **제안**: 불변식을 한 번만 선언한다 — `PostInitProperties` 의 널 검사를 걷어내거나(생성자 보장에 의존), 반대로 유지할 이유가 있다면 `ApplySenseSettings` 앞에 `check` 나 이른 반환을 붙여 같은 전제를 쓰게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위

- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`.
- **훑은 파일**: Public 헤더 19개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`. 계약 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp` 를 함께 읽었다.
- **모듈 경계 점검**: `WxAI.Build.cs` 의 Wx 의존은 `WxCore` 뿐이고(그 밖은 엔진 `AIModule`·`GameplayAbilities`·`GameplayTags`·`NavigationSystem`), `WxAI.uplugin` 의 `Plugins` 도 `GameplayAbilities`·`WxCore` 뿐이다. 소스 include 중 Wx 로 시작하는 외부 헤더는 `WxGameplayTags.h` 와 `Minion/WxMinion.h` 두 개이며 둘 다 `WxCore` 소유다. 다른 Wx 플러그인 참조는 없다.
- **규칙 점검 결과**: 38개 소스 전부 첫 줄이 `// Copyright Woogle. All Rights Reserved.` 와 일치한다. `FORCEINLINE`·헤더 내 인라인 함수 정의 0건, 람다식 0건, `BlueprintCallable`/`BlueprintPure` 0건(`UFUNCTION` 은 델리게이트 바인딩용 `HandlePossessedPawnChanged` 하나뿐), 모든 UCLASS/USTRUCT/UENUM 이 `Wx` 접두사를 지킨다(`UWxBTTask_*`, `FWxLockOnMemory`, `EWxPatrolMoveMode` 등). 델리게이트 콜백 3개(`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded`)는 모두 `Handle` 접두사다. 이 모듈에 StateTree 노드는 없으므로(BT 노드만 사용) `GetInstanceDataType()` 인라인 예외 대상도 없고, 실제로 그 심볼은 모듈 전체에 0건이다.
- **기존 발견 재판정**: 직전 리뷰의 네 항목 중 세 개(MirrorAbility Abort 마감, 어빌리티 프로토콜 중복, RandomChoice 가중치 전원 0, 절차 복제)는 소스가 그대로라 라인까지 동일하게 재확인해 유지했다. 발견 2·6 은 이번에 새로 올린 것이다.
- **검증해서 기각한 가설**: (1) `FWxBeyondLeashMemory::bWasBeyond` 가 in-class 초기화도 `InitializeMemory` 오버라이드도 없이 쓰인다 — `OnBecomeRelevant` 가 어떤 `TickNode` 보다 먼저 값을 시드하고 `CalculateRawConditionValue` 는 이 메모리를 읽지 않으므로, 초기화 전 읽기 경로가 없다. (2) `UWxBTService_MirrorMovement::TickNode:110` 의 `Trail[0]` 이 빈 배열을 인덱싱한다 — 직전 :98 의 `RecordSample` 이 무조건 하나를 넣고 폐기 루프(:103)가 `Num() > 1` 을 지키므로 항상 1개 이상이다. (3) `UWxBTTask_Patrol` 의 `PatrolCursor` 가 스플라인 포인트 수를 넘겨 원점으로 걸어간다 — 커서 갱신은 `GetNextIndex` 가 범위를 보장하고, 폰 교체 경로(리스폰)는 폰과 컨트롤러를 함께 새로 만들어 인스턴스 상태가 승계되지 않는다. (4) `UWxBTTask_Patrol::ExecuteTask` 가 `bPatrolFinished` 에서 `Super::ExecuteTask` 없이 `InProgress` 를 반환해 `UBTTask_MoveTo` 의 노드 메모리가 반쯤 초기화된 채 남는다 — 그 상태의 `MoveRequestID`/`bWaitingForPath` 가 비어 있어 베이스의 `TickTask`·`AbortTask`·`OnTaskFinished` 가 모두 무동작으로 지나가며, 브랜치를 점유한 채 머무는 것 자체는 헤더가 명시한 의도다. (5) `UWxBTTask_ActivateAbility::ExecuteTask` 의 range-for 가 `TryActivateAbility` 중 재할당된 배열을 순회한다 — `FScopedAbilityListLock` 이 루프 전체를 감싸 부여/제거가 지연되고, 락 해제 후에는 핸들로 다시 조회한다. (6) `UWxBTTask_ReturnHome` 이 `ForgetActor(nullptr)` 을 부를 수 있다 — 엔진이 널을 안전하게 처리하고, `ForgetActor` 로 지운 시야 대상은 Sight 센스가 다음 갱신에서 다시 등록하므로 헤더가 말한 "재획득 가능" 이 성립한다.
- **미검토 / 한계**: 이 환경에는 언리얼 엔진 소스가 없어(엔진 트리 탐색 결과 0건) `BehaviorTreeComponent`·`AIPerceptionComponent`·`AISense_Sight`·`AbilitySystemComponent` 의 내부 동작에 기대는 판정(발견 1 의 "Aborting 중에도 TickTask 가 돈다", 발견 2 의 "`GetCurrentlyPerceivedActors` 순서가 거리와 무관하다" 포함)은 소스 대조가 아니라 엔진 계약에 대한 지식으로 세웠다 — 고치기 전에 UE 5.8 원본으로 한 번 확인하는 편이 좋다. 빌드·PIE·네트워크 실행 없이 정적으로만 봤고, BT/Blackboard 에셋의 실제 노드 배치(특히 LockOn 과 MirrorMovement 를 같은 트리에 두지 말라는 규약, RandomChoice 아래 데코 구성)는 확인하지 않았다. BP/WBP 내부 구조는 범위 밖이다. BT 노드 메모리 구조체를 4바이트 오프셋에 `TArray`·`TWeakObjectPtr` 와 함께 두는 정렬 관용은 엔진 자체 노드와 같으므로 문제로 올리지 않았다.

---
*문서 기준 커밋 `231068b` · 리뷰일 2026-09-13 · 소스 38파일 — `/module-review`로 갱신*
