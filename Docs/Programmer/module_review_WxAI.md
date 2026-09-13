# WxAI — 코드 리뷰

> 모듈 상태는 양호하다. 퍼셉션을 순정 컴포넌트로 되돌리면서 감각 수치와 피격 보고가 `UWxAIBehaviorComponent` 로 모였다. 이 컴포넌트의 빙의 순서 처리, 피아 필터, 자극 역추적은 UE 5.8 엔진 소스와 맞춰 봤을 때 의도대로 동작한다. 플러그인 의존은 `WxCore` 하나뿐이고, `CLAUDE.md` 규칙 위반도 없다(소스 36개 모두 첫 줄 Copyright, 람다·`FORCEINLINE`·`BlueprintCallable` 0건, `Wx` 접두사와 콜백의 `Handle` 접두사 준수). 커버리지: README → `Build.cs`/`.uplugin` → 헤더 18개와 cpp 18개를 전부 읽었다. 이번에 바뀐 `UWxAIBehaviorComponent`, 이를 쓰는 `AWxAIController` 의 빙의·해제 경로, BT 태스크 수명주기는 엔진 소스로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 AI 컨트롤러가 빙의를 풀 때 폰을 시야 자극원에서 영구히 빼 버린다
- **위치**: `Source/WxGame/Controller/WxAIController.cpp:136`, `Source/WxGame/Controller/WxAIController.cpp:139`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:18`
- **범주**: 버그/정확성
- **문제**: 모듈 밖 소비 측 코드지만, 이 모듈 컴포넌트가 전제로 삼은 "캐릭터가 컨트롤러를 갈아탄다" 는 계약을 깨므로 여기 적는다. `OnUnPossess` 는 `Super::OnUnPossess()`(:139)보다 먼저 `Perception->UnregisterComponent()`(:136)를 부른다. 엔진의 `UAIPerceptionComponent::OnUnregister → CleanUp` 은 리스너만 해제하지 않는다. `GetMutableBodyActor()`, 즉 컨트롤러의 현재 폰도 `UAIPerceptionSystem::UnregisterSource` 로 모든 센스의 자극원에서 뺀다. 이 시점에는 폰 참조가 아직 살아 있으므로 방금 놓은 캐릭터가 Sight 자극원 목록에서 사라진다. 폰을 자극원으로 자동 등록하는 것은 스폰(`OnNewPawn`)과 `StartPlay` 때뿐이라 다시 등록되지도 않는다. 헤더(:18)와 워크로그가 겨냥한 파티원 교체(AI → 플레이어)가 들어오면, 플레이어가 넘겨받은 캐릭터를 적 AI 가 영영 보지 못한다. 청각·피격은 이벤트 방식이라 영향이 없다. 지금 드러나지 않는 이유는 AI 폰의 빙의 해제가 폰 파괴 때만 일어나기 때문이다. 프로젝트의 `UnPossess` 호출은 플레이어용 `Source/WxGame/Framework/WxRespawnLibrary.cpp:47` 하나뿐이다.
- **제안**: 언레지스터를 `Super::OnUnPossess()` 뒤로 옮긴다. 그러면 폰 참조가 끊긴 뒤라 `GetBodyActor()` 가 null 을 돌려주고 자극원 해제를 건너뛴다. 교체 기능을 구현할 때 적 AI 가 교체된 캐릭터를 감지하는지 PIE 로 확인한다.
- **확신도**: 높음

### 2. 🟡 MirrorAbility 의 TickTask 가 Abort 도중에도 돌아 abort 를 `Succeeded` 로 마감한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:176`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:193`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:220`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:280`
- **범주**: 버그/정확성
- **문제**: 엔진 `UBehaviorTreeComponent` 는 활성 노드 타입이 `AbortingTask` 일 때도 그 태스크의 `TickTask` 를 돌린다(5.8 소스로 확인). `AbortTask` 가 취소를 요청했지만 스코프 락 때문에 종료가 미뤄져 `InProgress` 를 반환하면(:176), 다음 틱의 `TickTask` 가 그대로 실행된다. 그 사이 대상이 태그를 놓았다면 :220 에서 `FinishLatentTask(Succeeded)` 로, ASC 가 사라졌다면 :193 에서 `Failed` 로 마감한다. 엔진 `OnTaskFinished` 는 이 결과를 부모 컴포지트의 `ConditionalNotifyChildExecution` 과 태스크의 `OnTaskFinished` 에 그대로 넘기므로 abort 가 성공이나 실패로 읽힌다. 재탐색 요청은 `bWasAborting` 이 막아 트리가 멈추지는 않는다. 같은 파일의 `HandleAbilityEnded` 는 :280 에서 Aborting 상태를 확인하고 `FinishLatentAbort` 로 마감하지만, 틱 경로에는 이 분기가 없어 두 종료 경로의 계약이 어긋난다. 발생 창은 좁다 — abort 대기 중에 대상이 태그를 놓아야 한다.
- **제안**: `TickTask` 의 두 마감 지점(:193, :220) 앞에 `GetTaskStatus(this) == EBTTaskStatus::Aborting` 분기를 두어 `FinishLatentAbort` 로 마감한다.
- **확신도**: 중간

### 3. 🟡 타겟이 있으면 새 자극을 보지 않아, 등 뒤에서 때리는 적에게 어그로가 넘어가지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:38`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:56`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:61`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:48`
- **범주**: 설계/구조
- **문제**: `TickNode` 는 현재 `TargetActor` 가 유효하고 살아 있으면 곧바로 반환한다(:38). 그래서 재선정은 타겟이 죽거나, 사라지거나, 어그로 비허용으로 바뀔 때만 일어나고, 그 사이 들어온 자극은 모두 무시된다. `UWxAIBehaviorComponent` 는 "시야·청각이 놓치는 가해자도 잡는다"(`WxAIBehaviorComponent.h:48`)며 피격을 Damage 자극으로 보고하지만, 이미 교전 중인 폰은 뒤에서 맞아도 그 자극을 판단에 쓰지 못한다. 후보가 여럿일 때는 `GetCurrentlyPerceivedActors`(:56)가 채운 배열에서 조건을 통과한 첫 원소를 고른다(:61). 이 순서는 퍼셉션 내부 `PerceptualData` 맵의 순회 순서다. 대체로 먼저 감지한 순이지만 항목이 지워지면 빈 슬롯 재사용으로 뒤섞이며, 거리나 최근성과는 무관하다. 게다가 한번 고른 타겟은 무효가 될 때까지 고정된다. 소환물이나 동료가 끼는 전투에서 "붙어서 때리는 쪽을 두고 먼저 본 쪽만 쫓는" 상황이 재현될 수 있다.
- **제안**: 스티키 어그로는 유지하되 갱신 창을 연다. 예를 들어 최근 Damage 자극의 가해자를 현재 타겟보다 우선하거나, 일정 주기로 후보를 거리와 최근 자극 시각으로 점수화해 비교한다. 지금 동작이 의도라면 헤더의 "죽거나 사라지거나 어그로 비허용으로 바뀌면 갈린다" 옆에 "피격으로는 갈리지 않는다" 를 적어 둔다.
- **확신도**: 중간

### 4. 🟡 어빌리티 발동·중단·종료 프로토콜이 두 태스크에 그대로 중복돼 이미 한쪽만 고쳐졌다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:42`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:102`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:73`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:141`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:249`
- **범주**: 중복/복잡도
- **문제**: 약 80줄이 두 파일에 거의 글자 그대로 들어 있다. 종료 델리게이트 선등록, 어빌리티 목록 락, 발동 핸들 선기록, 동기 종료와 재발동 판별, 취소 요청 중 재진입 차단, 취소 거부 인스턴스 처리, 종료 시 구독 해제가 모두 같다. 다른 것은 태그의 출처(고정 태그냐 대상이 소유한 태그냐)와 대상 추적뿐이다. 같은 개념의 플래그 이름부터 `bIsRequestingAbort`/`bIsRequestingCancel` 로 갈렸다. 발견 2 의 Aborting 분기 누락이 이 중복에서 생긴 실제 어긋남이다.
- **제안**: 프로젝트가 과한 구조 추출을 피하므로 두 태스크를 합치지는 않는다. 발동·마감 프로토콜만 공통 베이스로 올리는 것을 검토한다. 독립 구현을 유지한다면 두 헤더에 짝 관계를 적어 "한쪽 수명주기를 고치면 다른 쪽도 본다" 를 규약으로 남긴다.
- **확신도**: 중간

### 5. 🟢 `UWxAIBehaviorComponent` 의 감각 수치 getter 3개는 이관 뒤 호출부가 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:36`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:37`, `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h:38`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:111`
- **범주**: 중복/복잡도
- **문제**: `GetSightRadius`/`GetSightAngle`/`GetHearingRadius` 는 삭제된 `UWxAIPerceptionComponent` 가 수치를 가져가던 통로였다. 이관 뒤에는 같은 클래스의 `ApplySenseSettings` 가 멤버를 직접 읽어 컨트롤러로 넘긴다. `Source`·`Plugins` 전체에서 호출부가 0건이고, `UFUNCTION` 이 아니라 BP 에서 부를 수도 없다. 남겨 두면 밖에서 수치를 가져가는 경로가 아직 있는 것처럼 읽혀, "캐릭터가 수치를 넘긴다" 는 이번 이관의 방향과 어긋난다.
- **제안**: 세 함수의 선언과 정의를 지운다.
- **확신도**: 높음

### 6. 🟢 RandomChoice 가 "가중치 전원 0" 저작을 경고 없이 넘긴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:98`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:106`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:26`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 설계/구조
- **문제**: 모든 자식이 조건을 통과했는데 가중치가 전부 0 이면 후보가 하나도 없다(:89 에서 모두 제외). 엔진에 되돌려줄 차단 자식도 없어 `BTSpecialChild::ReturnToParent` 로 빠지고(:98~:106), 부모는 직전 결과를 그대로 이어받는다. 헤더(`RandomChoice.h:26`)가 이 한계를 적어 두기는 했다. 그런데 가중치 속성은 0 을 정상적인 "추첨 제외" 값으로 안내하므로(`RandomWeight.h:31`), 디자이너가 공격 몇 개를 임시로 빼다가 이 조합에 닿아도 런타임에 알아챌 단서가 없다.
- **제안**: 선택 의미는 그대로 두고, 이 경로에 닿으면 반복을 억제한 경고 로그를 남기거나 에셋 검증을 붙인다.
- **확신도**: 중간

### 7. 🟢 같은 절차가 통째로 복제된 짝이 두 쌍 더 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:106`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:125`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:205`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:228`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:57`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:97`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:86`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:138`
- **범주**: 중복/복잡도
- **문제**: (가) 포커스와 CMC 회전 모드를 한 쌍으로 걸고 되돌리는 절차가 `UWxBTService_LockOn` 의 `ApplyLockOn`/`ReleaseLockOn` 과 `UWxBTService_MirrorMovement` 의 `ApplyFacing`/`ReleaseFacing` 에 똑같이 있다. 특히 해제 쪽의 아키타입 복원 블록(`LockOn.cpp:148~:153`, `MirrorMovement.cpp:251~:256`)은 주석 한 글자까지 같다. (나) `MoveSpeedEffect` 를 SetByCaller 로 부여하고 `OnTaskFinished` 에서 제거하는 코드가 Patrol 과 Wander 에 같은 모양으로 들어 있다. 지금 동작에는 결함이 없지만, 복원 규칙이나 GE 제거 규칙을 바꿀 때 한쪽만 고칠 여지가 있다.
- **제안**: 프로젝트 방침상 구조 추출은 최소로 한다. 적어도 (가)의 해제 절차는 한곳으로 모으는 것을 검토한다. 그대로 둔다면 짝 관계를 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`. 계약 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp` 와 UE 5.8 엔진의 `AIPerceptionComponent.cpp`·`AIPerceptionSystem.cpp`·`AISense_Sight.cpp`·`AISense_Hearing.cpp`·`AISense_Damage.cpp`·`BehaviorTreeComponent.cpp`·`AIController.cpp`·`Pawn.cpp` 를 대조했다.
- **훑은 파일**: 나머지 Public 헤더 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`.
- **미검토 / 한계**: 빌드·PIE·네트워크 실행 없이 코드만 읽고 판단했다. BT/Blackboard 에셋의 실제 노드 배치(LockOn 과 MirrorMovement 를 한 트리에 두지 말라는 규약, RandomChoice 아래 데코 구성)는 확인하지 않았다. 검증한 뒤 기각한 가설: (1) `UWxAIBehaviorComponent` 가 첫 컨트롤러를 놓친다 — `PlacedInWorldOrSpawned` 폰은 `PostInitializeComponents` 에서 빙의가 먼저 끝나지만 `BeginPlay` 의 즉시 적용(`WxAIBehaviorComponent.cpp:47`)이 따라잡는다. 재사용 컨트롤러는 미등록 상태에서 config 만 갱신되고, 이어지는 `OnRegister` 가 그 값으로 리스너를 만든다. (2) 플레이어가 조종 중일 때 피격 보고가 오작동한다 — `UAISense_Damage` 는 폰→컨트롤러 순으로 리스너를 찾고 리스너 ID 까지 검사하므로 자극이 조용히 버려진다. (3) Neutral 팀 판정이 어긋난다 — `EWxTeam::Neutral` 이 255(`NoTeam`)라 엔진 기본 태도 판정과 `AWxCharacterBase::GetTeamAttitudeTowards` 가 같은 답을 낸다. (4) `UWxBTTask_ReturnHome` 이 `ForgetActor` 한 대상을 다시 잡지 못한다 — 엔진이 해당 시야 쿼리의 이전 결과를 지워 다음 갱신 때 다시 통지한다. (5) `FindPatrolComponent` 가 금지된 정적 FindComponent 래퍼다 — 널 검사 한 줄짜리 래퍼가 아니라 "경로는 부착 부모의 것" 이라는 조회 규칙을 담은 함수다. (6) 직전 리뷰에서 기각한 BeyondLeash 메모리 초기화, MirrorMovement `Trail[0]`, Patrol 커서 범위 항목은 소스가 그대로라 판정을 유지한다.

---
*문서 기준 커밋 `6bde8a033` · 리뷰일 2026-09-13 · 소스 36파일 — `/module-review`로 갱신*
