# WxWorld — 코드 리뷰

> 건강한 모듈이다. 프로젝트 코딩·모듈 규칙 위반이 사실상 없고(타이머 콜백 이름 하나뿐), 서버 권위/클라 추종·초기 진입 스냅·스택 카운터 짝맞춤 같은 까다로운 축이 의도와 근거까지 주석에 남아 있다. 남은 지적은 "설계 문서는 맞는데 코드가 아직 못 따라간 자리"와 실질 중복에 몰려 있다. 커버리지: `WxWorld.Build.cs`·`.uplugin`과 Public/Private 전 헤더를 훑고, 기믹 StateTree 컴포넌트·상호작용 스캐너·스포너·대기 태스크군의 cpp를 라인 단위로 읽었다. 판정에 필요한 범위에서 WxCore 계약(`IWxInteractable`)과 소비처(`WxAbility_Interact`, `AWxEnemyCharacter`, `UWxViewModel_InteractionList`), 엔진 `UStateTreeComponent`·`FStateTreeExecutionContext`·`FStateTreeWeakExecutionContext` 원본까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 'Enable Player Input' 이 상호작용 당사자가 아니라 "이 머신의 첫 로컬 플레이어" 를 토글한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp:29`
- **범주**: 설계/구조
- **문제**: `GEngine->GetFirstLocalPlayerController(...)` 로 대상을 고른다. 기믹 ST 는 모든 피어에서 각자 도므로, 한 플레이어가 유발한 연출이 **모든 클라에서 각자의 로컬 플레이어 입력을 끈다** — 연출과 무관한 플레이어의 조작이 멈춘다. 스플릿스크린 2P 이상은 반대로 토글에서 누락된다. 헤더(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_EnablePlayerInput.h:43`)가 이 한계와 해법(오너 기믹의 `GetInteractingCharacter`)을 이미 적어 두었으나 코드는 미반영이다. 같은 폴더의 자매 태스크 `FWxStateTreeTask_MoveInteractorToTarget` 은 이미 `Gimmick->GetInteractingCharacter()`(`WxStateTreeTask_MoveInteractorToTarget.cpp:33`) + `IsLocallyControlled()`(같은 파일 47행)로 올바르게 좁힌다.
- **제안**: `MoveInteractorToTarget` 과 동일 패턴으로 오너 기믹의 `InteractingCharacter` 를 읽고, 그 캐릭터가 `IsLocallyControlled()` 인 피어에서만 토글한다. `DisabledPawn`/`DisabledController` 기록·해제 구조는 그대로 성립한다.
- **확신도**: 높음 (동작은 확실하며, 헤더에 이미 알려진 한계로 명시돼 있다)

### 2. 🟡 트리가 소화하지 않은 상호작용도 「재진입」으로 오판해 클라만 상태를 다시 연다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:149`, 같은 파일 `375-387`
- **범주**: 버그/정확성
- **문제**: `OnInteracted` 는 켜진 영역이기만 하면 `bPendingInteractResolve` 를 세우고 발행한다(149). `PublishAuthorityState` 는 다음 틱에 「활성 태그가 그대로 + 플래그가 서 있음」을 곧 *자기 자신으로 가는 전이* 로 단정하고 `Multicast_ReenterState` 를 쏜다(380-384). 그런데 **트리가 그 발행을 아무 전이로도 소화하지 않은 경우**(발행자를 지목한 전이가 그 상태에 없거나, 전이 조건이 실패한 경우)도 관측 모습이 정확히 같아 구분되지 않는다. 엔진 확인 결과 발행자는 리스너가 없어도 틱을 예약하므로(`FStateTreeWeakExecutionContext::BroadcastDelegate`) 플래그는 다음 틱에 그대로 소비된다. 결과는 서버는 아무 일도 하지 않았는데 클라만 `EnterReplicatedState`(416행)로 현재 상태를 다시 열어, 그 상태의 1회성 연출(`PlaySound`·`PlayLevelSequence`·`PlayInteractorMontage`·`SpawnNiagara`)이 **클라에서만 다시 도는** 서버/클라 발산이다. 예: 슬라이드 중인 문의 영역을 끄지 않은 조립에서 이동 중 한 번 더 누르면 그 상태의 컷신·사운드가 클라에서만 재생된다.
- **제안**: 재진입 통지를 태그 비교 추론이 아니라 「권위 트리가 실제로 그 상태를 (재)진입했다」는 트리 쪽 사실에 기대게 옮긴다. 당장 어렵다면 최소한 발행자에 전이를 잇지 않은 조립을 에디터 검증이나 권위 측 Warning 으로 드러내, 클라에서만 연출이 도는 상황을 조용히 넘기지 않게 한다.
- **확신도**: 중간 (경로는 코드로 확인했으나 실제 재현은 조립에 달려 있다)

### 3. 🟡 권위 전용 대기 태스크에 권위 가드가 없어 비권위 실행 시 영구 Running 이 된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:80`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:63`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 헤더에 "권위에서 구동되는 ST 전용"으로 못박아 두었지만(`WxStateTreeTask_WaitSpawnersKilled.h:31`, `WxStateTreeTask_WaitForInteraction.h:34`) `EnterState` 에 권위 검사가 없다. 비권위 피어에서 진입하면 `AWxSpawner::bIsKilled` 는 복제되지 않아 영원히 false 이고 `MarkKilled`/`NotifyInteracted` 통보도 권위에서만 오므로 그 상태에 갇힌다. 기믹 ST 에 얹으면 복제 `StateTag` 추종이 클라를 끌어내 주지만, 그 순간까지 클라 트리는 어긋난 채로 남는다. 같은 모듈의 형제 태스크들(`WxStateTreeTask_TriggerSpawners.cpp:25`, `WxStateTreeTask_RespawnSpawners.cpp:24`, `WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp:30`)은 전부 `HasAuthority()` 로 가르고 클라는 `Succeeded` 로 흘려보내는 관례를 지키고 있어, 두 대기 태스크만 규약에서 벗어나 있다.
- **제안**: 형제 태스크와 같은 형태로 `EnterState` 앞에 `Owner->HasAuthority()` 가드를 두고 비권위는 `Succeeded` 반환.
- **확신도**: 높음

### 4. 🟡 늦게 도착한 `StateTag` 는 스냅이 아니라 라이브 전이로 수렴한다 — 헤더가 말하는 "레이트조인·스트리밍 인 재시작" 경로가 코드에 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:258-274`(OnRep), 같은 파일 `295-345`(스냅 경로)·`390-414`(추종), 헤더 주장은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h:69`
- **범주**: 설계/구조
- **문제**: 저장 상태 스냅(`StartTreeAtSavedState`)은 **트리 시작 시점의** `StateTag` 만 본다. 배치 액터의 클라 `BeginPlay` 는 보통 서버의 초기 프로퍼티 번치보다 먼저 돌므로, 그 시점 `StateTag` 는 비어 있어 스냅 경로가 걸리지 않는다. 이후 값이 도착하면 `OnRep_StateTag` 는 의도적으로 재시작하지 않고 틱만 깨우며(267-273), `FollowAuthorityState` → `EnterReplicatedState` 가 **라이브 전이**로 데려간다(주석이 "진입이 초기 진입으로 취급되지 않는다"고 명시). 즉 이미 열려 있는 문에 뒤늦게 합류·스트리밍 인 한 클라는 스냅이 아니라 문이 새로 열리는 연출·사운드·컷신을 한 번 본다. 헤더 69행의 "트리 재시작은 세이브 복원·레이트조인·스트리밍 인 전용이다" 는 서술과 실제 경로가 어긋난다(재시작을 부르는 곳은 `BeginPlay` 자동 시작과 `OnSaveRestored` 뿐이다).
- **제안**: 스냅이 의도라면 OnRep 에서 「첫 수렴」만 `RestartLogic()` 로 태우는 갈래가 필요하다. 반대로 라이브 전이가 의도라면 헤더 69행 서술을 코드에 맞게 고쳐, 다음 세션이 없는 경로를 있다고 읽지 않게 한다.
- **확신도**: 낮음 (복제 도착 순서에 달려 있어 의도된 절충일 수 있다 — 실측으로 가르는 것이 맞다)

### 5. 🟡 로케이터 표시명 헬퍼가 3중, UOL 클래스 검증(`Compile`)이 2중으로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43-65`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:113-135`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128` / `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:128-148`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:53-73`
- **범주**: 중복/복잡도
- **문제**: 표시명 헬퍼 세 벌은 빈 로케이터 반환 문자열(`"none"` vs `"unset"`)만 다르고 나머지 본문(액터 라벨 → `FActorLocatorFragment` 서브패스 끝 이름 → `"unresolved"`)이 완전히 동일하다. `Compile()` 의 UOL 클래스 검증 루프 두 벌은 주석까지 한 글자도 다르지 않다. 이미 `UWxSpawnerLibrary` 라는 공용 자리가 존재하는데도 각 태스크가 사본을 들고 있어, 표시 규칙이나 검증 정책이 바뀌면 다섯 곳을 따라다녀야 한다.
- **제안**: 표시명은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 하나로 모으고 빈 값 문자열만 인자로 받는다. `Compile` 검증도 같은 라이브러리의 `#if WITH_EDITOR` 헬퍼로 올려 두 태스크가 호출만 한다.
- **확신도**: 높음

### 6. 🟡 `StartTreeAtSavedState` 가 엔진의 재진입 가드를 우회하고 오너를 무검사 역참조한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:295-345` (특히 312행)
- **범주**: 설계/구조
- **문제**: 엔진 `UStateTreeComponent::StartTree()` 는 `CurrentlyRunningExecContext` 로 재진입을 **검사하고 설정**하는데, 이 멤버가 엔진 헤더에서 `private` 이라(`StateTreeComponent.h:196`) 재조립 경로는 검사도 설정도 못 한다. 그래서 `TickComponent` 가 그 가드를 세운 상태에서 재시작이 들어오면(예: 틱 중 `OnSaveRestored()` → `RestartLogic()`, 또는 340행 `OnStateTreeRunStatusChanged` 구독자가 재시작을 부르는 경우) 엔진이었다면 Error 로그로 막았을 호출이 그대로 통과해, `Context.Start()` 내부의 `InstanceData.Reset()` 이 바깥 틱이 순회 중인 인스턴스 데이터를 밀어 버린다. 312행 `FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData)` 는 `GetOwner()` 를 널 검사 없이 역참조하기도 한다(`*Asset` 은 `ResolveStateTag()` 통과가 보장한다).
- **제안**: 컴포넌트 자체 재진입 플래그를 두어 틱 처리 중 재시작을 막고, 312행 앞에 `GetOwner()` 널 가드를 세운다. 헤더 주석의 "엔진 업그레이드 시 확인 지점"에 이 가드 항목도 함께 적어 둔다.
- **확신도**: 중간 (현재 호출 경로에서 실제로 재진입하는 사례는 확인되지 않았다)

### 7. 🟢 두 스포너 발동 태스크가 에디터에서 같은 이름으로 뜬다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_TriggerSpawners.h:30`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h:34`
- **범주**: 중복/복잡도
- **문제**: 둘 다 `DisplayName = "스포너 발동"` 이라 노드 픽커에 동일한 이름이 두 개 뜬다. 헤더는 용도를 명확히 갈라 두었으나(바인딩형 = 공유 에셋용, 로케이터형 = 리터럴 지정용) 이름이 그 구분을 전혀 전달하지 못한다. 잘못 고르면 지정 칸이 비어 조용히 무동작(Warning 로그 한 줄)으로 끝난다.
- **제안**: 로케이터형을 "스포너 발동(지정)" 처럼 갈라 준다.
- **확신도**: 높음

### 8. 🟢 `NotifyInteracted` 의 로케이터 해석 컨텍스트가 대기 노드가 아니라 통보자다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:55`
- **범주**: 버그/정확성
- **문제**: `Wait.Target.SyncFind(Target) == Target` — 해석 컨텍스트로 통보자 액터를 넘긴다. 형제 구현(`WxStateTreeTask_WaitSpawnersKilled.cpp:73`)은 대기 노드 **자신의** 오너를 컨텍스트로 쓴다. 대기 목록이 프로세스 전역이라, 한 프로세스에 권위 월드가 둘 이상 뜨면 다른 월드의 대기까지 통보자 월드 기준으로 해석해 완료시킬 수 있다. 현재는 퀘스트 ST 가 권위에서만 만들어져(`Plugins/WxQuest/.../WxQuestComponent.cpp:111`) 통상 PIE 에서는 권위 월드가 하나뿐이라 실제 영향이 없다. 바로 위 48행에서 `Wait.Context.GetOwner()` 를 이미 확보해 놓고 쓰지 않는 것도 어색하다.
- **제안**: `Wait.Context.GetOwner()` 를 해석 컨텍스트로 넘겨 형제 구현과 통일한다(48행의 유효성 검사 결과를 그대로 재사용).
- **확신도**: 낮음 (현 구성에서는 증상이 나지 않는다 — 일관성 정리에 가깝다)

### 9. 🟢 타이머 델리게이트에 바인딩되는 콜백에 `Handle` 접두사가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:42`(바인딩), `:133`(정의)
- **범주**: 규칙 위반
- **문제**: `SetTimer(..., &UWxInteractionScannerComponent::ScanAndPush, ...)` 는 `FTimerDelegate` 바인딩이므로 코딩 규칙 4(델리게이트 콜백은 `Handle` 접두사)의 대상이다. 같은 저장소의 선례도 전부 접두사를 지킨다 — `WxAbility_Groggy.cpp:74,76` 의 `HandleMontagePollTick`·`HandleGroggySafetyTimeout`. 모듈 전체에서 이 한 건뿐이다.
- **제안**: `HandleScanTimer` 등으로 개명한다.
- **확신도**: 중간 (타이머를 규칙 4 의 "델리게이트"로 볼지는 해석 여지가 있으나, 프로젝트 선례는 본다)

### 10. 🟢 `PreSave` 의 SaveId 확정 규칙이 두 곳에 통째로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:192-204`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:68-87`
- **범주**: 중복/복잡도
- **문제**: "ProceduralSave 는 건드리지 않는다 / `Modify()` 는 부르지 않는다 / ActorGuid 로 재확정한다" 라는 비자명한 규칙 세 가지가 두 구현에 각각(주석까지) 복제돼 있다. 세이브 키 규약은 WxSave 슬롯 정합성의 근간이라, 한쪽만 고쳐지면 조용히 어긋난다.
- **제안**: 두 곳 중 하나가 바뀌면 다른 하나도 따라가야 한다는 사실만이라도 주석으로 상호 참조를 걸어 둔다. 공용화한다면 자리는 WxCore 의 `IWxSavable` 옆 `#if WITH_EDITOR` 정적 헬퍼가 맞다(도메인 데이터가 아니라 계약 부속이라 WxCore 규칙에 어긋나지 않는다).
- **확신도**: 높음 (중복 자체는 확실, 공용화 여부는 취향 문제)

### 11. 🟢 `GetPrompts()` 가 죽은 항목을 건너뛰어 선택 인덱스와 어긋날 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:77-86`
- **범주**: 버그/정확성
- **문제**: 82행 주석은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 하지만, 자리를 채우는 것은 `IWxInteractable` 을 못 찾은 경우뿐이고 **약참조가 이미 끊긴 항목은 통째로 건너뛴다**(79행). `UpdateInRange` 안에서 부를 때는 직전에 끊긴 항목을 걷어내 문제가 없지만, 공개 진입점이라 뷰모델이 초기 시드로 직접 부르는 경로(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`)에서는 스캔 사이에 파괴된 메시가 있으면 프롬프트 배열만 한 칸 줄어 `GetSelectedIndex()` 와 어긋난다 — 엉뚱한 행이 선택 표시된다.
- **제안**: 끊긴 항목도 빈 텍스트로 자리를 채워 주석이 말하는 불변식을 코드가 지키게 한다.
- **확신도**: 중간 (창이 좁아 실제 목격은 어렵다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayLevelSequence.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/README.md`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentSplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp`, Public 폴더 전 헤더
- **참고로 함께 읽은 모듈 밖 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, 엔진 `StateTreeComponent.cpp/h`·`StateTreeAsyncExecutionContext.cpp/h`·`StateTreeExecutionContext.cpp`
- **규칙 준수 확인 결과**: 전 파일(46개) 첫 줄 저작권 표기 정상, `Wx` prefix 누락 없음, `FORCEINLINE`·인라인 정의 없음(헤더의 `GetInstanceDataType()` 은 각 파일과 README 가 명시한 엔진 StateTree 관례 예외로 인정), `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll`(Blueprint Function Library) 한 곳뿐, `WxCore` 외 `Wx` 플러그인 의존 없음(역방향도 `WxGame` 하나뿐), override 의 `Super::` 호출 누락 없음(`StartLogic`/`RestartLogic` 은 엔진 `StartTree` 재조립이 목적인 의도적 대체라 위반으로 보지 않았다). 람다는 `WxInteractionScannerComponent.cpp:196` 의 정렬 술어 하나로 용도상 필요한 사용이다. 유일한 지적은 발견 9(타이머 콜백 이름).
- **지난 리뷰에서 뺀 항목**: 2026-08-15 리뷰의 "전역 대기 배열 순회 중 동기 완료로 참조가 끊긴다"(당시 🟡)는 이번에 엔진 구현을 직접 확인해 **성립하지 않음**을 확인했다. `FStateTreeWeakExecutionContext::FinishTask` 는 헤더 문구와 달리 완료 상태만 기록하고 `bHasPendingCompletedState` 를 세운 뒤 다음 틱을 예약할 뿐이라(`StateTreeAsyncExecutionContext.cpp:176-254`), `ExitState` 를 그 자리에서 동기 호출하지 않는다. 틱 안에서 통보가 발생해도 완료 처리는 통보 루프가 반환된 뒤 상위 틱 루프에서 돈다. 재도입 제안 시 이 확인을 먼저 뒤집어야 한다.
- **미검토 / 한계**:
  - StateTree 에셋·기믹 BP·`UWxWorldDeveloperSettings` 의 실제 설정 값은 보지 않았다. 발견 1·2·3 의 실제 영향 범위는 어떤 ST 에셋이 그 노드를 쓰는지에 달려 있다.
  - 발견 2·4 는 멀티 피어 실행 순서에 걸린 판정이라 코드 정황으로만 세웠다. PIE 2인(리슨 호스트 + 클라)에서 기믹 하나로 재현 확인이 필요하다.
  - 스캐너의 `OverlapMultiByObjectType(AllObjects)` 주기 스캔과 `Examined`/`InCandidates` 선형 탐색의 실측 비용은 프로파일하지 않았다. 반경 150cm·주기 0.1초·소유 클라 전용이라 지적으로 올리지 않았다.
  - 스캐너의 `ScanRadius`(150cm 기본)와 `UWxAbility_Interact::ScanRadius` 는 코드 주석상 일치가 전제인데, 양쪽 모두 `EditDefaultsOnly` 라 BP 디폴트 실제 값은 확인하지 않았다.
  - `AWxSpawner` 의 World Partition 셀 언로드 시 `EndPlay` → 스폰 인스턴스 `Destroy()` 가 추격 중인 적에게 어떤 체감을 주는지는 런타임 검증이 필요해 판단을 보류했다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 46파일 — `/module-review`로 갱신*
