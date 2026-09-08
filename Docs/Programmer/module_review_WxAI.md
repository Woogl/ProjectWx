# WxAI — 코드 리뷰

> 여전히 이 저장소에서 상태 소유권·수명주기 처리가 가장 잘 정리된 모듈이다. 지난 리뷰의 🟡 1번(타겟 소실 후 재획득 불가)은 `FindPerceivedTarget` 도입으로 해소됐고, 규칙 위반은 이번에도 0건이다(`BlueprintCallable`·`FORCEINLINE`·인라인 정의·람다 전부 0건, 32파일 모두 저작권 첫 줄과 `Wx` 접두사, 델리게이트 콜백 6종 모두 `Handle` 접두사, 의존은 `WxCore` 하나). 커버리지: 소스 32파일 전부와 `Build.cs`/`uplugin`/`README`를 읽었고, 새로 들어온 `FindPerceivedTarget` 경로와 어빌리티 발동/중단 프로토콜은 엔진 소스(`UAIPerceptionComponent::GetCurrentlyPerceivedActors`·`ConfigureSense`·`UBehaviorTreeComponent::OnTaskFinished`·틱 대상 선정)를 직접 확인하며 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 타겟 승계가 "가장 가까운 적"이 아니라 해시 순서로 아무나 고른다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:233-250` (호출부는 `:119`, `:128`)
- **범주**: 버그/정확성
- **문제**: `FindPerceivedTarget` 은 `GetCurrentlyPerceivedActors(nullptr, ...)` 결과를 앞에서부터 훑어 **첫 번째** 유효 액터를 그대로 승계한다. 그 목록의 원본은 `TMap<TObjectKey<AActor>, FActorPerceptionInfo>` 라 순서가 해시 버킷 순서이며, 거리·최신성·감각 종류와 무관하다. 게다가 `SenseToUse == nullptr` 이면 엔진은 `HasAnyCurrentStimulus()` 로 판정하는데, 이는 **만료 전 성공 자극이 하나라도 있으면 참**이다. Hearing·Damage 의 `MaxAge` 가 5초(`:37-40`)이므로 "4초 전에 소리를 낸 뒤 사라진 액터" 도 후보에 들어간다.
  구체적 실패: 눈앞의 A와 5초 안에 소리를 냈던 먼 곳의 B가 모두 후보일 때, 타겟이던 C가 죽으면 해시 순서에 따라 **보이지도 않는 B** 를 승계할 수 있다. 그러면 `UWxBTService_LockOn` 은 벽 너머를 겨누고, `TargetDistance` 는 사거리 밖 값이 되어 전투 브랜치가 헛돈다. 나아가 같은 상황이 서버 실행마다(액터 스폰 순서/해시 배치에 따라) 다르게 나올 수 있어 재현도 어렵다.
- **제안**: 루프를 "첫 유효 액터 반환" 대신 "유효 후보 중 폰과의 거리제곱이 최소인 액터"로 바꾼다. 후보 수가 적어 정렬 없이 순회 중 최소값만 갱신하면 되고, 이미 가진 목록만 다시 읽는 것이라 추가 감지 비용이 없다. 시야를 우선하고 싶다면 `GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), ...)` 를 먼저 시도하고, 비면 전체 목록으로 폴백하는 2단계도 가능하다.
- **확신도**: 중간

### 2. 🟡 어빌리티 발동/중단/종료 프로토콜이 두 태스크에 통째로 복제돼 있다 (미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:16-95`, `:102-139`, `:141-192` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:46-125`, `:141-177`, `:249-299` (헤더 상태 필드도 `WxBTTask_ActivateAbility.h:38-63` ↔ `WxBTTask_MirrorAbility.h:56-89` 로 동일)
- **범주**: 중복/복잡도
- **문제**: 발동 대상 태그를 어디서 얻느냐(저작값 `AbilityTag` vs 대상 ASC 폴링 `MirroredTag`)만 다르고, `FScopedAbilityListLock` 후보 순회 → `ActivatedHandle` 선기록 → 재발동 판별(`FindAbilitySpecFromHandle`+`IsActive`) → `ActivationResult` 되감기 → `CanBeCanceled` 거부 시 즉시 마감 → `GetTaskStatus` 로 abort/완료를 가르는 종료 처리까지 약 150줄이 주석 문구를 빼면 문자 단위로 같다.
  이 프로토콜은 모듈에서 가장 미묘한 코드이고, 실제로 과거에 "`MirrorAbility` 쪽에만 `CanBeCanceled` 가드가 없다" 는 결함이 정확히 이 복제 구조에서 나왔다. 지금도 한쪽만 고칠 위험이 그대로 남아 있으며, 코드 어디에도 두 파일이 쌍이라는 표시가 없다.
- **제안**: 두 노드를 통합하지 말고(별도 클래스 유지는 선행 결정), 발동 대상 태그만 순수 가상으로 뽑은 공통 베이스(`UWxBTTask_AbilityBase` 등)로 발동·중단·종료 구간만 끌어올린다. 구체 BT Task 를 상속하는 것이 아니라 프로토콜 한 벌을 공유하는 것이라 기존 결정과 충돌하지 않는다. 그마저 원치 않으면 최소한 양쪽 헤더에 "이 프로토콜은 반대편 태스크와 쌍으로 유지한다" 는 주석을 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 두 태스크를 독립 클래스로 유지하기로 한 선행 결정이 있다)

### 3. 🟢 BT 노드 3종에 `NodeName` 이 없어 그래프에 클래스명이 그대로 노출된다 (미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:11-14`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:13-19`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-16`
- **범주**: 설계/구조
- **문제**: 나머지 노드는 생성자에서 `NodeName` 을 지정해 BT 에디터에 "Patrol", "Lock On", "Random Choice", "Mirror Ability", "Beyond Leash", "Return Home", "Random Weight", "Update Target Distance" 로 뜨는데 이 셋만 비어 있어 엔진 폴백인 타입명이 그대로 보인다. 이 모듈은 BT 저작 표면 그 자체라 일관성이 곧 사용성이다. 두 차례 리뷰에서 연속으로 남아 있다.
- **제안**: 각 생성자에 `NodeName = TEXT("Activate Ability")` / `TEXT("Wander")` / `TEXT("Attribute Ratio")` 한 줄씩 추가한다.
- **확신도**: 높음

### 4. 🟢 리시 복귀가 끝난 뒤에는, 복귀 내내 계속 보이던 적대 액터를 다시 물지 못한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:102-110`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:28-35`
- **범주**: 버그/정확성
- **문제**: `ForgetTargetActor` 는 **현재 타겟 하나만** `ForgetActor` 하고 `SetTargetActor(nullptr)` 로 끝낸다. 다른 적대 액터의 감지 기록은 그대로 남고, 엔진은 감지 상태가 뒤집힐 때만 `OnTargetPerceptionUpdated` 를 방송하므로 복귀 내내 계속 보이고 있던 액터는 새 통지를 만들지 않는다. 결과적으로 복귀가 끝나 `UWxBTDecorator_BeyondLeash` 가 거짓으로 돌아온 뒤에도 `TargetActor` 가 빈 채라, 홈 근처에 서 있는 적을 눈앞에 두고 전투 브랜치가 열리지 않는다.
  같은 컴포넌트의 빙의 변경 경로(`:176`)는 `ForgetAll()` 로 이 함정을 피하고 있어 처리도 비대칭이다. 대부분의 경우 복귀 이동으로 시야 반경(1500)을 벗어났다가 재진입하며 자연히 풀리므로 노출 빈도는 낮다.
- **제안**: `ForgetTargetActor` 를 `ForgetActor(Target)` 대신 `ForgetAll()` 로 바꾸면 다음 퍼셉션 갱신에서 보이는 모든 액터가 새 상태 변화로 다시 등록돼 자연 재획득된다. 단 이 경우 복귀 중에도 즉시 새 타겟이 잡히므로(브랜치는 `BeyondLeash` 가 잡고 있어 전투로 넘어가지는 않는다) "복귀 동안 타겟 없음"을 유지하려는 의도가 있었다면 그 의도가 사라진다는 점을 확인하고 결정한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 복귀는 디스인게이지이므로 타겟을 비운 채 두는 것이 목적일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 Public 헤더 16종, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, 소비자 확인용 `Source/WxGame/Controller/WxAIController.cpp`
- **검증했으나 문제 없음(오탐 배제 기록)**:
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출 — 엔진 `UAIPerceptionComponent::ConfigureSense` 가 같은 클래스의 기존 항목을 **교체**하므로, 아키타입에서 복사된 `SensesConfig` 위에 다시 등록해도 중복이 생기지 않는다(UE 5.8 소스 확인).
  - `ForgetActor`/`ForgetAll` 은 Shipping 에서도 컴파일되는 정식 API 라, 리시 복귀의 기록 정리가 빌드 구성에 따라 달라지지 않는다.
  - Damage 센스에 피아 필터가 없는 문제 — 저장소 전체에서 `ReportDamageEvent` 호출부는 `WxAIPerceptionComponent.cpp:204` 하나뿐이고 그 앞에 `FGenericTeamId::GetAttitude` 가드가 있다. 따라서 `FindPerceivedTarget` 이 아군을 후보로 받을 경로는 현재 없다(향후 BP/타 모듈이 직접 보고하면 깨지는 전제다).
  - `UWxBTTask_MirrorAbility::TickTask` 가 abort 진행 중에도 호출돼 `FinishLatentTask(Succeeded)` 로 마감할 수 있는 점 — 엔진 `UBehaviorTreeComponent::OnTaskFinished` 는 `bWasAborting` 이면 결과값을 무시하고 `RequestExecution` 을 걸지 않으므로(트리 상태는 정상 복구) 실질 영향이 디버거 표시에 그친다. 결함으로 올리지 않았다.
  - `UWxBTComposite_RandomChoice` 의 `FBTCompositeMemory` 확장(엔진 `FBTParallelMemory` 와 동일 패턴), 가중치 0 후보 제외와 회피 완화 순서, 부동소수 폴백(`:114`) 모두 정합적이다.
  - `UWxBTTask_Patrol` 완주 시 `InProgress` 상주, `UWxBTService_LockOn` 의 포커스/회전 모드 왕복(진입·재타겟·폰 교체·소실 4경로), 어빌리티 태스크의 `AddUObject` 약참조 수명 — 지난 리뷰와 동일 결론으로 재확인했다.
- **미검토 / 한계**: BT/Blackboard 애셋이 이 노드들을 어떤 트리 형태로 조립하는지는 범위 밖이라, 발견 1·4의 체감 강도(전투 브랜치가 `TargetActor` 를 어떤 게이트로 읽는지)는 코드 근거로만 적었다. 리플리케이션·데디케이티드 서버 경로는 `UWxAnimNotify_ReportNoise` 의 `HasAuthority` 가드와 "퍼셉션/BT 는 서버 전용" 전제 외에는 보지 않았다. `UWxBTTask_Patrol` 의 `PatrolCursor` 가 컨트롤러 재사용으로 더 짧은 경로에 남는 경우는 `USplineComponent` 가 입력 키를 클램프해 크래시가 없음만 확인하고, 실제 재사용 시나리오가 존재하는지는 확인하지 않았다.

---
*문서 기준 커밋 `262e4cca` · 리뷰일 2026-09-08 · 소스 32파일 — `/module-review`로 갱신*
