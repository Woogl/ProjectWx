# WxAI — 코드 리뷰

> 모듈 건강도는 양호하다. 노드 메모리·수명주기·엔진 훅 계약을 정확히 다루고 있고, 플러그인 의존은 `WxCore` 하나뿐이며 프로젝트 코딩 규칙 위반은 한 건도 찾지 못했다. 이번 리뷰는 README → `WxAI.Build.cs`/`.uplugin` → Public 헤더 19개 전부 → 위험도가 높은 핵심 cpp(퍼셉션, 타겟 선정, 락온, 미러 이동, 어빌리티 태스크 2종, RandomChoice, 정찰·배회·복귀)까지 내려가 읽었고, 판정이 갈리는 지점은 UE 5.8 AIModule 원본과 소비자인 `AWxAIController` 로 교차 확인했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 MirrorAbility 의 TickTask 가 Abort 중에도 돌아 `Succeeded` 로 마감한다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:220`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:176`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:280`
- **범주**: 버그/정확성
- **문제**: 엔진은 `ActiveNodeType` 이 `ActiveTask` 일 때뿐 아니라 `AbortingTask` 일 때도 활성 태스크의 `TickTask` 를 돌린다(`BehaviorTreeComponent.cpp` 의 "tick active task" 블록이 두 상태를 함께 검사한다). 그런데 `AbortTask` 가 취소 요청 후 인스턴스가 아직 살아 있어 `InProgress` 를 반환하면(:176) 그 다음 프레임의 `TickTask` 가 그대로 실행되고, 대상이 그 사이 태그를 놓았으면 `FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded)`(:220) 로 마감한다. 같은 파일의 `HandleAbilityEnded` 는 `GetTaskStatus(this) == EBTTaskStatus::Aborting` 을 보고 `FinishLatentAbort` 를 부르도록 정확히 처리하고 있어(:280), 두 종료 경로의 계약이 어긋나 있다. 현재 트리 구성에서는 엔진이 `bWasAborting` 을 보고 재탐색을 억제하므로 멈추지는 않지만, 부모가 결과 통지를 쓰는 컴포지트(예: SimpleParallel)면 abort 가 성공으로 읽힌다.
- **제안**: `TickTask` 의 마감 지점에도 `HandleAbilityEnded` 와 같은 Aborting 분기를 두어 abort 중이면 `FinishLatentAbort` 로 마감한다.
- **확신도**: 중간

### 2. 🟡 어빌리티 발동·중단·종료 프로토콜이 두 태스크에 그대로 중복돼 있다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:41`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:102`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:72`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:249`
- **범주**: 중복/복잡도
- **문제**: 종료 델리게이트 선등록, 목록 잠금, 발동 핸들 선기록, 동기 종료·재발동 판별, 취소 요청 중 재진입 차단, 취소 거부 인스턴스 처리, 종료 시 구독 해제까지 약 80줄이 두 파일에 거의 문자 단위로 같다. 태그의 출처(고정 태그 vs 대상 소유 태그)와 대상 추적만 다르다. 실제로 이 중복이 위험하다는 증거가 발견 1이다 — 한쪽에만 있는 Aborting 가드가 다른 쪽 마감 경로에는 없다.
- **제안**: 두 태스크를 합치지는 말고, 발동·마감 프로토콜만 공통 베이스나 헬퍼로 올린다. 독립 구현을 유지한다면 "한쪽 수명주기를 고치면 반드시 다른 쪽도 같이 본다"를 두 헤더에 규약으로 박아 둔다.
- **확신도**: 중간

### 3. 🟢 RandomChoice 가 "가중치 전원 0" 저작을 진단 없이 삼킨다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:98`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:26`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 설계/구조
- **문제**: 모든 자식이 조건을 통과했는데 가중치가 전부 0 이면 후보도(:89 에서 전부 제외) 되돌려줄 차단 자식도 없어 `BTSpecialChild::ReturnToParent` 로 빠지고(:98), 부모는 직전 결과를 그대로 이어받는다. 헤더는 이 한계를 명시하지만, 가중치 속성은 0 을 정상적인 "추첨 제외" 값으로 안내하므로(`RandomWeight.h:31`) 디자이너가 공격 몇 개를 임시로 빼다가 이 조합에 도달했는지 알아챌 단서가 없다.
- **제안**: 런타임 선택 의미는 그대로 두고, 이 경로에 도달했을 때 (반복 억제한) 경고 로그나 에셋 검증을 붙인다.
- **확신도**: 중간

### 4. 🟢 같은 절차가 통째로 복제된 지점이 두 쌍 더 있다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:106`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:125`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:205`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:228`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:57`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:86`
- **범주**: 중복/복잡도
- **문제**: (가) 포커스 + CMC 회전 모드를 한 쌍으로 걸고 되돌리는 절차가 `UWxBTService_LockOn::ApplyLockOn`/`ReleaseLockOn` 과 `UWxBTService_MirrorMovement::ApplyFacing`/`ReleaseFacing` 에 있고, 해제 쪽은 아키타입 복원 로직까지 주석 문구 하나 다르지 않게 같다. (나) `MoveSpeedEffect` 의 SetByCaller 부여와 `OnTaskFinished` 제거가 Patrol/Wander 에 같은 모양으로 복제돼 있다. 둘 다 지금 동작에 결함은 없지만, 아키타입 복원이나 GE 제거 규칙을 고칠 때 한쪽만 고쳐질 여지가 있다.
- **제안**: 프로젝트 방침상 과한 구조 추출은 피하되, 최소한 (가)의 해제 절차만은 한곳으로 모으는 것을 검토한다. 유지한다면 짝 관계를 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위

- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`.
- **훑은 파일**: Public 헤더 19개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`. 계약 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` 를 함께 읽었다.
- **규칙 점검 결과**: 38개 소스 전부 첫 줄 Copyright 일치, `BlueprintCallable`·`FORCEINLINE`·헤더 내 인라인 정의·람다식 0건, 델리게이트 콜백 3개(`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded`) 모두 `Handle` 접두사, 모든 override 가 `Super::` 를 호출한다. Build.cs·uplugin 의 Wx 의존은 `WxCore` 뿐이고 소스 include 도 `WxGameplayTags.h`/`Minion/WxMinion.h` 두 개로 모두 WxCore 소유다.
- **기존 발견 재판정**: 어빌리티 프로토콜 중복과 RandomChoice 가중치 전원 0 진단 부족은 현재 코드에도 그대로 있어 유지했다. 이전 리뷰가 언급한 에디터 시야 드로우는 `WxAIBehaviorComponent.cpp` 로 자리를 옮겨 커밋됐고, 에디터 월드·선택 액터로 한정되며 `BeginPlay` 에서 틱을 끄므로 신규 결함이 없다.
- **검증해서 기각한 가설**: (1) `UWxBTTask_Patrol` 의 `PatrolCursor`/`bPatrolFinished` 가 노드 인스턴스에 남아 재빙의 시 승계되는 문제 — `AWxSpawner` 가 리스폰 때 폰을 Destroy 후 재생성하고 AutoPossessAI 가 새 컨트롤러를 만들므로 실제 경로가 없다. (2) `UWxAIPerceptionComponent::PostInitProperties` 의 `ConfigureSense` 가 아키타입에서 복사된 config 와 중복 등록되는 문제 — 엔진 `ConfigureSense` 가 같은 클래스 항목을 추가가 아니라 교체하고 `SensesConfig` 가 `Instanced` 라 중복이 생기지 않는다. (3) `GetPointLocation` 의 커서 범위 초과 — `USplineComponent` 가 인덱스를 가드한다.
- **미검토 / 한계**: 빌드·PIE·네트워크 실행 없이 정적으로만 봤다. BT/Blackboard 에셋의 실제 노드 배치(특히 LockOn 과 MirrorMovement 를 같은 트리에 두지 말라는 규약이 지켜지는지)는 확인하지 않았고, BP/WBP 내부 구조는 범위 밖이다. BT 노드 메모리의 정렬(4바이트 단위 오프셋에 `TArray`·`TWeakObjectPtr` 를 두는 것)은 엔진 자체 노드와 같은 관용이라 문제로 올리지 않았다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 38파일 — `/module-review`로 갱신*
