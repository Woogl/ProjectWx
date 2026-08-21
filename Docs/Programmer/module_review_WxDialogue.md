# WxDialogue — 코드 리뷰

> 9파일 825줄의 작고 밀도 높은 모듈로, 실패 경로마다 로그를 남기고 수명·GC·네트워크 전제를 헤더 주석에 명시해 둔 편이라 전반적으로 건강하다. 이번 리뷰는 `Build.cs`·`.uplugin`·공개 헤더 전부와 cpp 4개 전량을 읽었고, 소비자 쪽(`AWxNpc`·`UWxViewModel_Dialogue`·`UWxUIManagerSubsystem`·`UWxAbility_Interact`)과 엔진 구현(`AActor::FindComponentByClass`, `FStateTreeWeakExecutionContext::FinishTask`, `StateTreeModule.Build.cs`)까지 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 세션을 닫는 길이 UI 의 `Advance()` 하나뿐 — 창이 사라지면 세션이 좀비가 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:142-150`, `:197`(`EndDialogue`)
- **범주**: 버그/정확성
- **문제**: `EndDialogue()` 는 private 이고 부르는 곳이 둘뿐이다 — `Advance()`(`:53-86`)와 다음 세션의 `ClientStartDialogue_Implementation`(`:123-126`). 그런데 `Advance()` 를 부르는 유일한 주체는 대화 창의 뷰모델이고(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:48`), 그 창은 `State.Dialogue` 태그 하나로만 뜬다. 즉 **창이 사라지는 모든 경로가 곧 끝나지 않는 세션**이고, 컴포넌트에 `EndPlay`/`OnUnregister` 정리 훅도 없다. 확인된 트리거 둘:
  - **태그 발행 실패**: 폰이나 ASC 가 없으면 `if (UAbilitySystemComponent* ASC = ...)`(`:144`)가 통째로 건너뛰어지는데 else 도 로그도 없다(이 파일의 다른 모든 실패 경로가 경고를 남기는 것과 대조적). 창이 안 뜨므로 세션이 열린 채 굳는다. Experience 지연 스폰으로 폰이 컨트롤러보다 늦게 서는 레벨 초입에, 퀘스트 ST 가 여는 나레이션 대사가 정확히 이 창에 걸린다.
  - **대화 도중 폰 교체**: `UWxUIManagerSubsystem::WatchPawnTags` 가 폰이 바뀔 때 무조건 `CloseDialogueScreen()` 을 부른다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:260`). 창이 닫히면 `Advance()` 호출자가 사라지고, `TaggedAbilitySystem` 은 옛 폰의 ASC 를 가리키며, 새 ASC 엔 태그가 안 붙어 창이 다시 뜨지도 않는다. 탈것·연출 폰 교체는 같은 파일 `:239` 주석이 실재하는 흐름으로 명시한 것이다.

  결과는 두 트리거 모두 같다 — `OnDialogueEnded` 가 발화하지 않아 대기 중인 `FWxStateTreeTask_PlayDialogue` 가 `Running` 으로 멈추고(퀘스트 스텝 정지), `EndDialogueCamera()` 를 못 타 스폰된 `ACameraActor` 가 `SetLifeSpan` 없이 컨트롤러 수명까지 남는다. 다음 대화가 열리면 그때서야 풀리므로 영구 락은 아니지만, 막힌 퀘스트가 그 다음 대화의 선행 조건이면 그대로 막다른 길이다.
- **제안**: (a) 태그 발행 실패에 경고를 남기고 시작 실패로 취급해 `:128-135` 와 같은 롤백을 타게 한다 — 그러면 ST 태스크의 `HasActiveDialogue()` 검사가 Failed 로 조립 오류를 드러낸다. (b) `EndPlay`/`OnUnregister` 와 폰 교체 지점에서 활성 세션을 접어, 어느 정리 경로로 끝나든 `OnDialogueEnded` 가 반드시 한 번 발화하게 한다.
- **확신도**: 중간

### 2. 🟡 포즈를 얹을 스켈레탈 메시를 `FindComponentByClass` 로 고른다 — 다중 메시 대상에서 비결정적
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:335`
- **범주**: 버그/정확성
- **문제**: `AActor::FindComponentByClass` 는 `TSet<TObjectPtr<UActorComponent>> OwnedComponents` 를 훑어 **처음 걸린 것**을 답한다(엔진 `Actor.cpp:4018`, 멤버 선언 `Actor.h:4331`) — TSet 이라 순서가 정해져 있지 않다. 대상이 메타휴먼 구성이면 후보가 여럿이다: `UWxMetaHumanComponent` 가 등록 시점에 Body·Face·Outfit 스켈레탈 메시를 오너에 추가로 생성해 붙인다(`Source/WxGame/Character/WxMetaHumanComponent.cpp:172`). 포즈는 실제로 포즈를 만드는 리더 메시에 얹혀야 하는데, Body 가 뽑히면 그쪽은 `SetLeaderPoseComponent` 로 구동되는 표시 전용이라 `PlayPendingPose` 의 경고만 남고 포즈가 사라지고, Face 가 뽑히면 `FaceAnimClass` 인스턴스가 있어 **경고조차 없이** 얼굴 리그에 몸 포즈 몽타주가 걸린다.
  같은 모듈이 `AreaMesh` 에서는 정확히 이 이유로 자동 탐색을 거부했다는 점이 근거를 보강한다(`Public/WxDialogueComponent.h:58`: "등록 시점에 생기는 메시가 있으면 무엇이 잡힐지 정해지지 않기 때문").
  현재 배치 NPC(`AWxNpc`)는 `AActor` 파생이고 `UWxMetaHumanComponent::OnRegister` 가 오너를 `ACharacter` 로 캐스팅해 실패하면 조기 반환하므로(`WxMetaHumanComponent.cpp:36`) 아직 후보가 하나뿐이지만, `AWxNpc` 가 메타휴먼 부착물을 갖게 되는 순간(헤더가 명시한 의도) 바로 성립한다. `StartDialogueRow(Row, Target)` 의 `Target` 이 임의 액터라는 점도 같은 위험을 넓힌다.
- **제안**: 포즈 대상 메시를 대상 쪽이 직접 지목하게 한다 — `UWxDialogueComponent` 가 `AreaMesh` 처럼 포즈 메시 프로퍼티를 들고 세션이 그것을 묻거나, 최소한 `ACharacter::GetMesh()` 를 먼저 시도한 뒤 `FindComponentByClass` 로 떨어지게 한다.
- **확신도**: 중간

### 3. 🟡 상호작용 잠금 상태를 메시 콜리전에 저장한다 — 근거가 사라졌고 프로파일을 덮어쓴다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:20-35`, 규약 서술은 `Public/WxDialogueComponent.h:36-41`
- **범주**: 설계/구조
- **문제**: 두 갈래로 나뉜다.
  - **근거 만료**: 헤더는 "별도의 상태 플래그를 두지 않는 이유는 영역 메시의 콜리전이 이미 그 상태이기 때문"이라고 적어 두었지만, 상호작용이 액터 단위로 바뀌면서 콜리전을 꺼도 감지가 갈리지 않게 됐고 cpp 주석(`:22-23`)이 그 사실을 직접 인정한다("잠금의 실질은 이 명시 판정이다"). 즉 콜리전 토글은 더 이상 아무것도 게이트하지 않고 bool 하나를 부작용 있는 방식으로 보관할 뿐이다.
  - **프로파일 클로버링**: `SetInteractionEnabled(true)` 가 `ECollisionEnabled::QueryOnly` 를 하드코딩해 넣는다(`:34`). 원래 값을 기억하지 않으므로, `AreaMesh` 를 `QueryAndPhysics` 로 저작한 호스트는 잠금→해제 한 번에 피직스 콜리전을 조용히 잃는다. "붙이면 어떤 액터든 대화 상대가 된다"는 이 컴포넌트의 약속과 정면으로 어긋난다(현재 유일한 호스트 `AWxNpc` 는 `QueryOnly` 라 실동작 영향은 없다).
  - 부수적으로, `AreaMesh` 미지정이나 콜리전 없는 메시를 골라 둔 조립 실수가 "의도된 잠금"과 구분되지 않아 `IsInteractionEnabled()` 가 조용히 false 만 답한다.
- **제안**: 잠금 상태를 컴포넌트의 `bool` 멤버로 옮기고 `AreaMesh` 는 감지·사거리용 형상이라는 원래 역할만 남긴다(`IWxInteractable` 은 이미 "켜고 끄는 수단은 구현체가 정한다"로 열어 두었다). 시작 잠금이 필요하면 `EditAnywhere` 기본값으로 표현한다.
- **확신도**: 중간(액터 단위 전환 이전 설계가 남아 있는 것으로 보이나, 의도적 유지일 수 있음)

### 4. 🟢 `GetCurrentRowHandle()` · `GetCurrentDialogueTarget()` 은 호출자가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`, `:69` (정의는 `Private/WxDialogueSessionComponent.cpp:93`, `:98`)
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 이 둘을 부르는 코드가 없다. `UFUNCTION` 도 아니라 BP·StateTree 바인딩에서도 닿지 않으므로 완전히 도달 불가다. 카메라가 세션 내부로 들어오면서 `GetCurrentDialogueTarget` 은 `CurrentTarget` 직접 참조로 대체됐고(`BeginDialogueCamera`, cpp:229), 행 신원을 관찰할 것으로 서술된 WxQuest 에는 Dialogue 참조가 0건이다. 헤더·README 의 "관찰 규약" 서술이 실재하지 않는 계약을 광고하고 있어 다음 세션을 오도한다.
- **제안**: 둘을 지우고 헤더·README 의 관찰 규약 서술도 `OnLineChanged`/`OnDialogueEnded`/`State.Dialogue` 세 가지로 줄인다. 행 신원 관찰이 다시 필요해지면 그때 되살린다.
- **확신도**: 높음

### 5. 🟢 대상 없는 대사(나레이션)에서도 포즈를 스트리밍한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:285-315`
- **범주**: 성능/안전
- **문제**: `ApplyCurrentPose` 가 `CurrentTarget` 유효성을 보지 않고 `RequestAsyncLoad` 를 건다. `FWxStateTreeTask_PlayDialogue` 는 항상 `Target=nullptr` 로 진입하므로(`WxStateTreeTask_PlayDialogue.cpp:40`), NPC 대화와 ST 나레이션이 같은 테이블을 공유하면 대사마다 몽타주를 로드했다가 `PlayPendingPose` 에서 "대상에 애님 인스턴스가 없어"(`:340`) 경고만 남긴다. 실패도 아닌 상황에 몽타주 IO 와 경고가 붙는다.
- **제안**: `ApplyCurrentPose` 진입부에서 `CurrentTarget.IsValid()` 를 게이트해 대상 없는 세션은 포즈 경로를 통째로 건너뛴다.
- **확신도**: 높음

### 6. 🟢 `Build.cs` 의 `UniversalObjectLocator` 는 쓰이지 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: 모듈 어디에서도 참조하지 않는다(소스 전체 grep 0건). `StateTreeModule` 이 공개 의존으로 끌어오는 것도 아니다(엔진 `StateTreeModule.Build.cs` 의 `PublicDependencyModuleNames` 에 없음) — `WxQuest.Build.cs` 와 목록이 같은 것으로 보아 복사에서 온 잔재다. README 의존성 목록에도 그대로 실려 있어 오해를 부른다. 겸사겸사, `GameplayAbilities` 는 cpp 에서만 쓰이므로 `PrivateDependencyModuleNames` 가 맞다(현재 전부 Public).
- **제안**: `UniversalObjectLocator` 를 지우고 README 의존성 줄도 함께 정리한다.
- **확신도**: 높음

### 7. 🟢 `PlayerCameraManager` 만 널 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:237`
- **범주**: 성능/안전
- **문제**: `BeginDialogueCamera` 는 Pawn·CurrentTarget·스폰된 CameraActor 를 모두 가드하는데 `PlayerController->PlayerCameraManager` 만 그대로 역참조한다. 로컬 PC 라면 `PostInitializeComponents` 에서 이미 스폰돼 있어 실사용에선 널이 아닐 것이므로 지금 터지는 문제는 아니고, 이 함수 안의 일관성 차이로만 눈에 띈다.
- **제안**: 널이면 다른 조기 반환들과 같이 카메라 경로를 건너뛴다. 실익이 낮다고 보면 그대로 두어도 무방하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **대조한 모듈 밖 근거**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, 엔진 `Actor.cpp`·`StateTreeAsyncExecutionContext.cpp`·`StateTreeExecutionContext.cpp`
- **미검토 / 한계**:
  - 규칙 검사(`Wx` prefix, 저작권 첫 줄, `Handle` prefix, `Super::` 호출, `BlueprintCallable` 오용, WxCore 외 Wx 플러그인 참조)는 전 파일 통과. 헤더의 `GetInstanceDataType()` 인라인 정의(`WxStateTreeTask_PlayDialogue.h:43`)와 ST 태스크의 `AddLambda`(`.cpp:51`)는 각각 코드에 근거가 적힌 프로젝트 전역 관례와 엔진 약한 실행 컨텍스트 규약이라 위반으로 보지 않았다.
  - `EndDialogue()` 의 `Broadcast(); Clear();`(`:213-214`) 재진입 위험은 검토했으나 문제 없음으로 판단했다 — `FStateTreeWeakExecutionContext::FinishTask` 는 완료 플래그만 세우고 전이는 다음 틱에 돌므로(엔진 `StateTreeExecutionContext.cpp:2246`) Broadcast 콜스택 안에서 새 바인딩이 붙는 경로가 없다.
  - 리플리케이션은 v1 싱글/리슨 호스트 전제(루즈 태그 비복제, 소유 클라 진행)가 헤더 3곳에 명시돼 있어 그 전제 안에서만 검증했다. 데디케이티드 서버 구성(ST 태스크의 `HasActiveDialogue()` 즉시 검사, 서버 측 `State.Dialogue` 차단 부재)은 리뷰하지 않았다.
  - `.uasset`(대화 DataTable, WBP, ST 에셋) 내부 구조는 범위 밖.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 9파일 — `/module-review`로 갱신*
