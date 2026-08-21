# WxWorld — 코드 리뷰

> 기믹 상태 복제·재진입·세이브 복원 같은 어려운 부분은 근거 주석까지 갖춘 채로 잘 짜여 있고 널 가드·권위 게이트도 대체로 촘촘하다. 남은 문제는 대부분 멀티플레이 경계(권위 전용 트리에서만 도는 태스크)와 같은 로직을 여러 파일에 복사한 중복이다. 이번 리뷰는 `WxGimmickStateTreeComponent`·`WxInteractionScannerComponent`·`WxSpawner` 세 축의 cpp 를 깊게 보고, ST 태스크 20종은 EnterState/Tick/ExitState 본문을 모두 읽되 에디터 전용 `GetDescription` 은 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 8 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 ST 태스크 두 개가 노드 피커에서 같은 이름 "스포너 발동" 으로 뜬다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_TriggerSpawners.h:30`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h:79`
- **범주**: 설계/구조
- **문제**: 두 `USTRUCT` 의 `meta` 가 `DisplayName = "스포너 발동", Category = "Wx"` 로 완전히 같다. 대상 지정 방식(바인딩형 `TSoftObjectPtr` vs 리터럴 `FUniversalObjectLocator`)이 달라 둘 다 필요한 노드인데, 디자이너가 ST 에디터 노드 목록에서 어느 쪽을 고르는지 구분할 수 없다. 헤더 doc-comment 는 서로를 이름으로 지목하고 있지만(`'Trigger Spawners By Locator'`) 그 이름이 UI 에 나타나지 않는다.
- **제안**: 로케이터 쪽을 `"스포너 발동 (지정)"` 처럼 갈라 준다. 두 doc-comment 의 상호 참조 문구도 새 표시명으로 맞춘다.
- **확신도**: 높음

### 2. 🟡 '플레이어 입력 켜기' 가 상호작용 당사자가 아니라 첫 로컬 플레이어를 토글한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp:34`
- **범주**: 버그/정확성
- **문제**: `GEngine->GetFirstLocalPlayerController(World)` 로 대상을 정한다. 기믹 ST 는 모든 피어에서 각자 도므로, 멀티플레이에서 A 가 문을 열면 그 기믹이 관련성 있는 모든 클라에서 같은 상태로 진입해 **연출과 무관한 플레이어 B 의 입력까지 잠긴다**. 스플릿스크린 2P 이상은 반대로 토글에서 누락된다. 헤더(`Public/Gimmick/WxStateTreeTask_EnablePlayerInput.h:170`)가 이 한계를 이미 「한계」로 적어 두었으나 코드는 그대로다.
- **제안**: 헤더가 스스로 제시한 방법대로, 오너 기믹의 복제 필드 `GetInteractingCharacter()` 로 당사자를 찾고 그 폰이 `IsLocallyControlled()` 일 때만 토글한다 — `WxStateTreeTask_MoveInteractorToTarget.cpp:28-51` 이 이미 쓰는 패턴이다. 당사자가 비어 있는 초기 진입/복원은 지금처럼 노옵.
- **확신도**: 높음

### 3. 🟡 '상호작용 켜기' 의 Target(액터) 갈래가 권위 전용 트리에서 클라에 반영되지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:81`
- **범주**: 설계/구조
- **문제**: `Interactable->SetInteractionEnabled(bEnable)` 는 이 트리가 도는 피어에서만 실행되고 값이 복제되지 않는다. 기믹 ST 는 모든 피어에서 돌아 수렴하지만, 이 모듈의 다른 대기 태스크들이 명시하듯(`WxStateTreeTask_WaitForInteraction.h:34`, `WxStateTreeTask_WaitSpawnersKilled.h:31`) 퀘스트 ST 는 **권위에서만** 돈다. 따라서 퀘스트 스텝이 NPC/레버의 상호작용을 여는 배선은 서버에서만 열리고 클라 스캐너는 그 대상을 후보에서 계속 탈락시켜 프롬프트가 뜨지 않는다. 헤더 `Public/Interaction/WxStateTreeTask_EnableInteraction.h:47` 이 "서버가 곧 클라인 싱글/리슨 호스트가 전제" 로 이 제약을 적어 두었다.
- **제안**: 대상 쪽 계약을 복제 가능한 형태로 만든다 — `IWxInteractable::SetInteractionEnabled` 구현체가 복제 필드를 세우게 하거나(`AWxLeverDevice` 는 지금 콜리전 상태를 로컬로만 바꾼다), 대상 액터에 멀티캐스트 진입점을 두고 태스크가 권위에서 그것을 부른다.
- **확신도**: 중간

### 4. 🟡 클라 추종이 도달 불가능한 상태를 만나면 매 프레임 전이 요청을 되풀이한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:409-433`, `435-455`
- **범주**: 성능/안전
- **문제**: `FollowAuthorityState` 는 로컬 활성 태그가 `StateTag` 와 다르면 조건 없이 `EnterReplicatedState()` 를 부르고, 그것은 `RequestTransition(TargetState, Critical)` 로 끝난다. 엔진의 전이 요청은 처리 후 소비되고 `ScheduleNextTick(TransitionRequest)` 로 다음 틱을 예약하므로, 대상 상태가 해석은 되지만 선택되지 않는 경우(상태에 Enter Condition 이 걸려 있거나 자식 선택이 실패하는 그룹 상태)에는 **잠들어야 할 컴포넌트가 매 프레임 깨어나 같은 요청을 무한 반복**한다. 태그 자체를 못 찾는 경우만 `EnterReplicatedState` 초입(`:445-449`)에서 걸러진다.
- **제안**: 추종 요청을 낸 뒤 결과를 한 틱 확인해, 같은 목표에 대해 연속 실패가 이어지면 경고 1회 후 그 태그에 대한 재요청을 멈춘다(다음 `StateTag` 변화에서 다시 시도).
- **확신도**: 중간

### 5. 🟡 상호작용 반경이 스캐너와 어빌리티에 각각 하드코딩되어 조용히 어긋날 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h:78` (`ScanRadius = 150.f`), 짝: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h:35` (`ScanRadius = 150.f`)
- **범주**: 설계/구조
- **문제**: 클라 표시 반경과 서버 권위 사거리 검증 반경이 서로 다른 모듈의 독립된 `EditDefaultsOnly` 필드다. 스캐너 헤더 주석도 "일치시킨다" 고 요구하지만 강제할 장치가 없다. 어긋나면 증상이 「프롬프트는 뜨는데 눌러도 아무 일도 안 남」 이라 원인 추적이 어렵고, 두 값이 BP 디폴트로 각각 오버라이드되면 더 벌어진다.
- **제안**: 값을 한 곳(예: `WxCore` 의 developer settings 또는 공유 상수)에 두고 양쪽이 읽게 한다. 최소한 스캐너 `BeginPlay` 에서 폰 ASC 의 `Ability.Interact` 스펙 반경과 비교해 다르면 Warning 을 남긴다.
- **확신도**: 중간

### 6. 🟡 로케이터 표시명 헬퍼가 3벌, ST Compile 검증이 2벌 복사되어 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:107-129`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:74-96`
- **범주**: 중복/복잡도
- **문제**: `FUniversalObjectLocator` 표시명을 뽑는 로직(해석되면 `GetActorLabel`, 아니면 `FActorLocatorFragment` 의 SubPath 꼬리)이 세 파일에 글자 단위로 같게 들어 있다. 다른 것은 빈 값 반환 문자열(`"unset"` vs `"none"`)뿐이다. 그중 하나(`UWxSpawnerLibrary::GetSpawnerLocatorDisplayName`)는 이미 모듈 공용 에디터 헬퍼로 노출되어 있고 다른 두 태스크가 그것을 쓰지 않고 재구현했다. 같은 패턴으로 `Compile()` 의 "해석되는데 WxSpawner 가 아니면 에러" 본문도 `WxStateTreeTask_WaitSpawnersKilled.cpp:127-147` 과 `WxStateTreeTask_TriggerSpawnersByLocator.cpp:58-78` 에 그대로 두 벌 있다.
- **제안**: 표시명은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 하나로 모으고(스포너 전용이 아니게 되었으니 이름은 `GetLocatorDisplayName` 등으로), Compile 검증은 클래스 인자를 받는 공용 헬퍼로 빼 두 태스크가 호출만 하게 한다.
- **확신도**: 높음

### 7. 🟡 '대기' 태스크 두 개가 같은 레지스트리 패턴을 따로 구현하고, 그 저장소가 월드 스코프가 아니다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:11-25`·`36-95`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:12-46`·`57-124`
- **범주**: 중복/복잡도
- **문제**: 두 파일이 「익명 namespace 의 전역 `TArray<Wait>` + 증가만 하는 핸들 + EnterState 등록 / ExitState 선형 탐색 제거 / Notify 에서 역순 순회하며 오너 소멸분 청소」 라는 같은 구조를 각각 복사해 갖고 있다. 이 배열이 **모듈 전역 static** 이라 월드 단위가 아니다 — 한 프로세스 안 PIE 다중 인스턴스(서버 월드 + 클라 월드)에서 등록이 뒤섞이고, `NotifySpawnerKilled`(`:57`)/`NotifyInteracted`(`:36`)는 매 통보마다 무관한 월드의 등록까지 전부 훑는다. 또한 `ExitState` 를 거치지 못한 등록은 「다음 통보가 올 때」만 지워지므로, 통보가 더 오지 않으면 세션이 끝날 때까지 남는다.
- **제안**: 공통 레지스트리를 템플릿/작은 클래스 하나로 뽑고, 그 인스턴스를 `UWorldSubsystem` 이 소유하게 해 월드 스코프와 자동 정리를 함께 얻는다. 통보 진입점은 지금처럼 static 파사드로 두면 호출부(`WxAbility_Interact`, `AWxSpawner::MarkKilled`)는 그대로 둘 수 있다.
- **확신도**: 중간

### 8. 🟡 상호작용자 GE 의 instigator 가 기믹이 아니라 피격자 자신이 된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp:46-52`
- **범주**: 버그/정확성
- **문제**: `ASC` 는 상호작용자(대상)의 ASC 이고, 그것으로 `MakeEffectContext()` 를 만들어 `ApplyGameplayEffectSpecToSelf` 한다. `UAbilitySystemComponent::MakeEffectContext` 는 컨텍스트의 instigator/effect causer 를 **자기 오너**로 채우므로, 이 GE 의 instigator 는 기믹이 아니라 효과를 받는 플레이어 자신이 된다. 회복·버프에는 무해하지만, 함정 기믹처럼 데미지·디버프를 주는 GE 를 붙이면 킬 크레딧·아군 판정·`ExecCalc` 의 소스 어트리뷰트가 전부 자기 자신을 가리킨다. 소스 오브젝트만 `AddSourceObject(Owner)`(`:47`)로 기믹을 담고 있다.
- **제안**: 기믹 오너의 ASC 가 있으면 그쪽 `MakeEffectContext` + `ApplyGameplayEffectSpecToTarget` 로 보내고, 없으면 최소한 `EffectContext.AddInstigator(Owner, Owner)` 로 instigator 를 기믹으로 덮는다.
- **확신도**: 중간 (회복/휴식류 자가 GE 만 쓰는 전제라면 의도된 설계일 수 있음)

### 9. 🟢 하이라이트 해제가 보이지 않는 프리미티브를 건너뛰어 잔여 외곽선이 남을 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:290-303`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted` 가 켜기·끄기 양쪽 모두에서 `!Primitive->IsVisible()` 인 컴포넌트를 건너뛴다. 켤 때의 필터는 타당하지만(안 보이는 형상은 외곽선에 기여하지 않음) 끌 때도 같은 필터가 걸린다. 하이라이트가 켜진 뒤 그 메시가 숨겨졌다가 대상이 사거리를 벗어나면 `bRenderCustomDepth` 가 켜진 채로 남고, 스캐너는 그 액터를 이미 잊었으므로 다시 보이는 순간부터 외곽선이 영구히 남는다.
- **제안**: 끄는 경로(`bHighlighted == false`)에서는 가시성 필터를 걸지 않고 모든 `UPrimitiveComponent` 에 `SetRenderCustomDepth(false)` 를 건다.
- **확신도**: 중간

### 10. 🟢 `bNeverRevive` 가 Auto 모드에서만 편집 가능해 Manual 보스 스포너에는 걸 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h:66`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:66`
- **범주**: 설계/구조
- **문제**: `bNeverRevive` 에 `EditCondition = "SpawnMode == EWxSpawnerMode::Auto", EditConditionHides` 가 걸려 있어 Manual 스포너에서는 디테일 패널에 뜨지도 않고 항상 기본값 false 로 남는다. 그런데 필드의 용도(주석: "보스 등")와 맞는 스포너는 보통 로케이터 태스크로 개별 발동하는 Manual 쪽이고, `Respawn()` 의 영구 처치 가드(`:66`)는 `bIsKilled && bNeverRevive` 를 본다 — 결과적으로 Manual 보스는 트리거가 다시 걸릴 때마다 부활한다.
- **제안**: Manual 이 일괄 리스폰 대상이 아니라는 이유로 가린 것이라면 그 근거를 필드 주석에 남기고, 그렇지 않다면 `EditCondition` 을 걷는다.
- **확신도**: 낮음 (의도된 설계일 수 있음)

### 11. 🟢 엔진 `StartTree` 재조립본이 재진입 가드를 빠뜨렸다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:311-364`
- **범주**: 설계/구조
- **문제**: `StartTreeAtSavedState` 는 시작 상태를 넣기 위해 엔진 `UStateTreeComponent::StartTree` 를 손으로 다시 조립한 것인데(헤더가 「엔진 업그레이드 시 확인 지점」으로 명시), 엔진 원본에 있는 `CurrentlyRunningExecContext` 재진입 가드(`Engine/Plugins/Runtime/GameplayStateTree/.../StateTreeComponent.cpp:181-188`)가 빠져 있다. 그 멤버는 엔진 클래스에서 `private` 이라 파생 클래스가 세울 수 없다. 결과적으로 `Context.Start()` 가 도는 도중 무언가가 `StopLogic`/`RestartLogic` 을 되부르면 엔진 쪽 분기가 그 사실을 모른 채 같은 `InstanceData` 위에 두 번째 실행 컨텍스트를 만든다. 현재 이 모듈의 태스크들 중 그 경로를 타는 것은 없어 실질 위험은 낮다.
- **제안**: 고칠 수는 없으므로, 헤더의 "엔진 업그레이드 시 확인 지점" 주석에 **어떤** 가드를 못 가져왔는지를 적어 다음 세션이 같은 조사를 반복하지 않게 한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxLeverDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp` (+ 대조용으로 엔진 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeAsyncExecutionContext.cpp`, 호출부 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`)
- **훑은 파일**: 나머지 ST 태스크 전부(`WxStateTreeTask_ComponentMove` · `ComponentSplineMove` · `PlayAnimation` · `PlayInteractorMontage` · `PlayLevelSequence` · `PlaySound` · `SpawnNiagara` · `EnablePlayerInput` · `ApplyGameplayEffectToInteractor` · `TriggerSpawners` · `RespawnSpawners` · `TriggerSpawnersByLocator`), `System/WxSpawnerLibrary.*`, `System/WxWorldDeveloperSettings.*`, `Spawnable/WxSpawnable.*`, `WxWorldModule.*`, `WxWorld.Build.cs`, `WxWorld.uplugin`
- **규칙 점검(전수)**: 소스 첫 줄 저작권 48/48 통과 · `WxCore` 외 Wx 플러그인 의존 없음 · `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll` 1건(BP Function Library 라 허용) · `FORCEINLINE` 0건 · 람다 1건(`WxInteractionScannerComponent.cpp:192` 의 `Sort` 술어 — 필요) · override 의 `Super::` 호출 누락 없음. `GetInstanceDataType()` 의 헤더 인라인 정의는 각 헤더가 코딩 규칙 6 의 예외로 명시하고 README 에도 규약으로 적혀 있어 위반으로 세지 않았다.
- **미검토 / 한계**: (1) 기믹 상태 수렴의 실제 타이밍(권위 `bPendingInteractResolve` 소비 시점이 항상 트리가 발행을 소화한 뒤인지)은 코드·엔진 소스 독해로만 확인했고 실제 네트워크 세션으로 재현하지 않았다. (2) 기믹 동작은 ST 에셋의 저작에 크게 의존하는데 에셋(.uasset) 내부는 리뷰 범위 밖이라, 태스크가 실제로 어떻게 조립되어 있는지는 확인하지 않았다. (3) `AWxSpawner` 의 World Partition 셀 스트리밍 인/아웃 시 `bIsKilled` 복원 순서는 `WxSave` 쪽 오케스트레이션에 달려 있어 이번 리뷰에서 추적하지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 48파일 — `/module-review`로 갱신*
