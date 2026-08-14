# WxWorld — 코드 리뷰

> 건강한 모듈이다. 프로젝트 코딩·모듈 규칙 위반이 하나도 없고, 서버 권위/클라 추종·초기 진입 스냅·스택 카운터 짝맞춤 같은 까다로운 축이 의도와 근거까지 주석에 남아 있다. 남은 지적은 대부분 "설계는 맞는데 코드가 아직 못 따라간 자리"와 실질 중복이다. 커버리지: `WxWorld.Build.cs`·`.uplugin`과 Public/Private 전 헤더를 훑고, 기믹 StateTree 컴포넌트·상호작용 스캐너·스포너·대기 태스크군의 cpp를 라인 단위로 읽었다. 판정에 필요한 범위에서 WxCore 계약(`IWxInteractable`/`IWxSavable`)과 소비처(`WxAbility_Interact`, `AWxEnemyCharacter`, `UWxViewModel_InteractionList`), 엔진 `UStateTreeComponent`/`FStateTreeExecutionContext` 원본까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 'Enable Player Input' 이 상호작용 당사자가 아니라 "이 머신의 첫 로컬 플레이어" 를 토글한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp:29`
- **범주**: 설계/구조
- **문제**: `GEngine->GetFirstLocalPlayerController(...)` 로 대상을 고른다. 기믹 ST 는 모든 피어에서 각자 도므로, 한 플레이어가 유발한 연출이 **모든 클라에서 각자의 로컬 플레이어 입력을 끈다** — 연출과 무관한 플레이어의 조작이 멈춘다. 스플릿스크린 2P 이상은 반대로 토글에서 누락된다. 헤더(`Public/Gimmick/WxStateTreeTask_EnablePlayerInput.h:43`)가 이 한계와 해법(오너 기믹의 `GetInteractingCharacter`)을 이미 적어 두었으나 코드는 미반영이다. 같은 폴더의 자매 태스크 `FWxStateTreeTask_MoveInteractorToTarget` 은 이미 `Gimmick->GetInteractingCharacter()` + `IsLocallyControlled()` 로 올바르게 좁힌다.
- **제안**: `MoveInteractorToTarget` 과 동일 패턴으로 오너 기믹의 `InteractingCharacter` 를 읽고, 그 캐릭터가 `IsLocallyControlled()` 인 피어에서만 토글한다. `DisabledPawn`/`DisabledController` 기록·해제 구조는 그대로 성립한다.
- **확신도**: 높음 (동작은 확실하며, 헤더에 이미 알려진 한계로 명시돼 있다)

### 2. 🟡 권위 전용 대기 태스크에 권위 가드가 없어 비권위 실행 시 영구 Running 이 된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:80`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:63`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 헤더에 "권위에서 구동되는 ST 전용"으로 못박아 두었지만(`WxStateTreeTask_WaitSpawnersKilled.h:31`, `WxStateTreeTask_WaitForInteraction.h:34`) `EnterState` 에 권위 검사가 없다. 비권위 피어에서 진입하면 `AWxSpawner::bIsKilled` 는 복제되지 않아 영원히 false 이고 `MarkKilled`/`NotifyInteracted` 통보도 권위에서만 오므로, 그 상태에 **영구히 갇힌다**. 같은 모듈의 형제 태스크들(`WxStateTreeTask_TriggerSpawners.cpp:25`, `WxStateTreeTask_RespawnSpawners.cpp:24`, `WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp:30`)은 전부 `HasAuthority()` 로 가르고 클라는 `Succeeded` 로 흘려보내는 관례를 지키고 있어, 두 대기 태스크만 규약에서 벗어나 있다.
- **제안**: 형제 태스크와 같은 형태로 `EnterState` 앞에 `Owner->HasAuthority()` 가드를 두고 비권위는 `Succeeded` 반환. 기믹 축에서는 클라가 로컬 완료로 앞서가도 복제 `StateTag` 추종이 되돌리므로 안전하다.
- **확신도**: 높음

### 3. 🟡 전역 대기 배열을 순회하는 중 동기 완료가 일어나면 참조가 끊기고 순회가 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:60-77`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:43-60`
- **범주**: 버그/정확성
- **문제**: 엔진 문서상 `FStateTreeWeakExecutionContext::FinishTask` 는 *"틱 처리 중 호출되면 상태가 즉시 완료된다"* (`StateTreeAsyncExecutionContext.h`). 즉시 완료되면 `ExitState` 가 동기 실행되어 같은 전역 배열에서 자기 항목을 `RemoveAt` 한다(`WaitSpawnersKilled.cpp:121`, `WaitForInteraction.cpp:91`). 그 결과 (a) 순회가 잡아 둔 `const FWxSpawnersKilledWait& Wait` / `const FWxInteractionWait& Wait` 가 `FinishTask` 호출 **도중** 파괴되어 해제 후 사용이 되고, (b) 뒤 원소가 앞으로 당겨져 역방향 순회가 항목을 건너뛴다. 현재 호출 경로(`WxAbility_Interact::ExecuteInteract`, `AWxEnemyCharacter::HandleDeath`)는 ST 틱 밖이라 버퍼링 경로를 타지만, 기믹 ST 태스크가 GE 로 적을 처치하는 식으로 틱 안에서 통보가 발생하는 조립이 생기면 성립한다.
- **제안**: 순회에서는 완료 대상 핸들만 모으고(또는 항목을 값으로 복사하고) 배열 순회를 끝낸 뒤 `FinishTask` 를 호출한다.
- **확신도**: 중간 (동기 완료 경로에 도달하는 조립이 아직 없어 잠재적)

### 4. 🟡 `NotifyInteracted` 의 로케이터 해석 컨텍스트가 대기 노드가 아니라 통보자다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:55`
- **범주**: 버그/정확성
- **문제**: `Wait.Target.SyncFind(Target) == Target` — 해석 컨텍스트로 통보자 액터를 넘긴다. 형제 구현(`WxStateTreeTask_WaitSpawnersKilled.cpp:73`)은 대기 노드 **자신의** 오너(`Owner.Get()`)를 컨텍스트로 쓴다. 대기 목록이 프로세스 전역이므로 단일 프로세스 PIE(서버 월드 + 클라 월드 공존)에서는 두 월드의 등록이 한 배열에 섞이는데, 이 코드는 통보자의 월드로 모든 대기의 로케이터를 해석해 다른 월드의 대기까지 완료시킬 수 있다. 바로 위 48행에서 `Wait.Context.GetOwner()` 를 이미 확보해 놓고 쓰지 않는 것도 어색하다.
- **제안**: `Wait.Context.GetOwner()` 를 해석 컨텍스트로 넘겨 형제 구현과 통일한다(48행의 유효성 검사 결과를 그대로 재사용).
- **확신도**: 중간

### 5. 🟡 로케이터 표시명 헬퍼가 3중, UOL 클래스 검증(`Compile`)이 2중으로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43-65`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:113-135`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128` / `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:128-148`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:53-73`
- **범주**: 중복/복잡도
- **문제**: 표시명 헬퍼 세 벌은 빈 로케이터 반환 문자열(`"none"` vs `"unset"`)만 다르고 나머지 본문(액터 라벨 → `FActorLocatorFragment` 서브패스 끝 이름 → `"unresolved"`)이 완전히 동일하다. `Compile()` 의 UOL 클래스 검증 루프 두 벌은 주석까지 한 글자도 다르지 않다. 이미 `UWxSpawnerLibrary` 라는 공용 자리가 존재하는데도 각 태스크가 사본을 들고 있어, 표시 규칙이나 검증 정책이 바뀌면 다섯 곳을 따라다녀야 한다.
- **제안**: 표시명은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 하나로 모으고 빈 값 문자열만 인자로 받는다. `Compile` 검증도 같은 라이브러리의 `#if WITH_EDITOR` 헬퍼로 올려 두 태스크가 호출만 한다.
- **확신도**: 높음

### 6. 🟡 `StartTreeAtSavedState` 가 엔진의 재진입 가드를 우회하고 오너를 무검사 역참조한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:295-345` (특히 312행)
- **범주**: 설계/구조
- **문제**: 엔진 `UStateTreeComponent::StartTree()` 는 `CurrentlyRunningExecContext` 로 재진입을 **검사하고 설정**하는데, 이 멤버가 엔진 헤더에서 `private` 이라 재조립 경로는 검사도 설정도 못 한다. 그래서 `TickComponent` 가 그 가드를 세운 상태에서 `RestartLogic()` 이 들어오면(예: `OnSaveRestored()` → `RestartLogic()`, 또는 340행 `OnStateTreeRunStatusChanged` 구독자가 재시작을 부르는 경우) 엔진이었다면 Error 로그로 막았을 호출이 그대로 통과해, `Context.Start()` 내부의 `InstanceData.Reset()` 이 바깥 틱이 순회 중인 인스턴스 데이터를 밀어 버린다. 312행 `FStateTreeExecutionContext Context(*GetOwner(), *Asset, InstanceData)` 는 `GetOwner()` 를 널 검사 없이 역참조하기도 한다(`*Asset` 은 `ResolveStateTag()` 통과가 보장한다).
- **제안**: 컴포넌트 자체 재진입 플래그(`bIsStartingTree` 등)를 두어 시작 경로와 틱 처리 중 재시작을 막고, 312행 앞에 `GetOwner()` 널 가드를 세운다. 헤더 주석의 "엔진 업그레이드 시 확인 지점"에 이 가드 항목도 함께 적어 둔다.
- **확신도**: 중간 (현재 호출 경로에서 실제로 재진입하는 사례는 확인되지 않았다)

### 7. 🟢 두 스포너 발동 태스크가 에디터에서 같은 이름으로 뜬다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxStateTreeTask_TriggerSpawners.h:30`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h:34`
- **범주**: 중복/복잡도
- **문제**: 둘 다 `DisplayName = "스포너 발동"` 이라 노드 픽커에 동일한 이름이 두 개 뜬다. 헤더는 용도를 명확히 갈라 두었으나(바인딩형 = 공유 에셋용, 로케이터형 = 리터럴 지정용) 이름이 그 구분을 전혀 전달하지 못해 디자이너가 어느 쪽을 골랐는지 알 수 없다.
- **제안**: 로케이터형을 "스포너 발동(지정)" 처럼 갈라 준다.
- **확신도**: 높음

### 8. 🟢 `PreSave` 의 SaveId 확정 규칙이 두 곳에 통째로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:192-204`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:68-87`
- **범주**: 중복/복잡도
- **문제**: "ProceduralSave 는 건드리지 않는다 / `Modify()` 는 부르지 않는다 / ActorGuid 로 재확정한다" 라는 비자명한 규칙 세 가지가 두 구현에 각각(주석까지) 복제돼 있다. 세이브 키 규약은 WxSave 슬롯 정합성의 근간이라, 한쪽만 고쳐지면 조용히 어긋난다.
- **제안**: WxCore 의 `IWxSavable` 옆에 `#if WITH_EDITOR` 정적 헬퍼(예: `StampSaveIdFromActorGuid(FObjectPreSaveContext&, const AActor*, FGuid&)`)를 두고 두 `PreSave` 가 호출만 하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_MoveInteractorToTarget.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ComponentSplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp`, Public 폴더 전 헤더
- **참고로 함께 읽은 모듈 밖 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`
- **규칙 준수 확인 결과**: 전 파일 첫 줄 저작권 표기 정상, `Wx` prefix 누락 없음, `FORCEINLINE` 없음, `BlueprintCallable` 은 `UWxSpawnerLibrary::TryRespawnAll`(Blueprint Function Library) 한 곳뿐, `WxCore` 외 `Wx` 플러그인 의존 없음, 델리게이트 바인딩 자체가 없어 `Handle` prefix 대상 없음, override 의 `Super::` 호출 누락 없음. 람다는 `WxInteractionScannerComponent.cpp:196` 의 정렬 술어 하나로 용도상 필요한 사용이라 위반으로 보지 않았다. 헤더의 `GetInstanceDataType()` 인라인 정의는 각 파일이 명시한 대로 엔진 StateTree 관례의 예외로 인정했다.
- **미검토 / 한계**:
  - StateTree 에셋·기믹 BP·`UWxWorldDeveloperSettings` 의 실제 설정 값은 보지 않았다. 발견 1·2 의 실제 영향 범위는 어떤 ST 에셋이 그 노드를 쓰는지에 달려 있다.
  - 발견 3 의 동기 `FinishTask` 경로는 엔진 헤더 문서와 호출부 정황으로만 판단했고, 실제로 그 경로를 타는 조립을 재현하지는 않았다.
  - 스캐너의 `OverlapMultiByObjectType(AllObjects)` 주기 스캔과 `Examined`/`InCandidates` 선형 탐색의 실측 비용은 프로파일하지 않았다. 반경 150cm·주기 0.1초·소유 클라 전용이라 지적으로 올리지 않았으나, 밀집 지역에서의 실측은 별도로 볼 가치가 있다.
  - `AWxSpawner` 의 World Partition 셀 언로드 시 `EndPlay` → 스폰 인스턴스 `Destroy()` 동작이 추격 중인 적에게 어떤 체감을 주는지는 런타임 검증이 필요해 판단을 보류했다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 46파일 — `/module-review`로 갱신*
