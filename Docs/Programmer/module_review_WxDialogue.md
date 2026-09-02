# WxDialogue — 코드 리뷰

> 11파일 규모의 작고 응집도 높은 모듈이다. 코딩·모듈 규칙 위반은 없고, 수명주기(테이블 행 포인터 비캐시, 스트리밍 핸들 취소, `TWeakObjectPtr` 사용, `CreateUObject` 콜백) 처리도 대체로 견실하다. 남는 지적은 버그라기보다 세션 수명 경계와 v1 권위 전제에 걸린 것들이다. 이번 리뷰는 `.uplugin`/`Build.cs`, 6개 헤더 전부, 5개 cpp 전부를 읽었고 세션 컴포넌트·ST 태스크는 소비자(WxGame `WxViewModel_Dialogue`·`WxAbility_Interact`, WxUI `WxUIManagerSubsystem`)와 엔진 StateTree `FinishTask`/`GetActivePathInfo` 구현까지 대조해 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 세션을 중도에 접을 경로가 없다 — 고아 세션과 태그 잔류
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:202`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:113`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 는 private 이고, 도달하는 경로는 두 가지뿐이다 — `Advance()` 가 `NextRow=None` 에 닿거나, 다음 `ClientStartDialogue` 가 겹침을 접을 때. 컴포넌트에 `EndPlay`/`OnUnregister` 오버라이드도, 외부에서 부를 취소 진입점도 없다. 그래서 세션은 "대사를 끝까지 넘겨야만" 닫힌다.
  - 폰 교체(사망·리스폰) 시 `UWxUIManagerSubsystem::WatchPawnTags`(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:204`)가 관찰 대상을 옮기며 `CloseDialogueScreen()`(같은 파일 `:216`)로 대화 창을 닫지만, 세션에는 아무도 알리지 않는다. `CurrentRowName` 이 남아 `HasActiveDialogue()` 는 계속 true 이고, 대사를 넘길 UI 가 사라졌으므로 세션은 다음 대화가 열릴 때까지 영구히 열린 채로 남는다.
  - 그 사이 `FWxStateTreeTask_PlayDialogue` 가 붙여 둔 `OnDialogueEnded` 는 발화하지 않는다(`WxStateTreeTask_PlayDialogue.cpp:99`). 대화 도중 죽으면 '대화 재생' 스텝의 퀘스트 트리가 Running 에 영구 정지한다.
  - `BeginDialogueCamera` 가 세운 카메라 액터도 `SetLifeSpan` 을 받지 못해 PC 가 죽을 때까지 남는다(뷰 타겟 자체는 리스폰 시 엔진 `AutoManageActiveCameraTarget` 이 되돌리므로 화면은 정상 복귀한다).
  - 같은 폰을 유지한 채 대화 창만 사라지는 경로(CommonUI back action 등)가 생기면 더 나쁘다 — `State.Dialogue` 가 폰 ASC 에 남아 `UWxAbility_Interact` 의 `ActivationBlockedTags`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:36`)가 상호작용을 계속 막고, 상호작용이 막히면 세션을 접을 다음 대화도 열 수 없어 소프트락이 된다.
- **제안**: `EndDialogue()` 를 감싼 public 취소 진입점(`CancelDialogue()`)을 열고, `UActorComponent::EndPlay`/`OnUnregister` 에서 호출한다. 추가로 컨트롤러의 폰 교체(`APlayerController::OnPossessedPawnChanged`)를 구독해 세션을 접으면 사망·리스폰 경로가 함께 닫힌다 — 세션은 이미 `TaggedAbilitySystem` 으로 폰 교체를 의식하고 있으므로 정책상 이질적이지 않다.
- **확신도**: 중간 (폰 교체 시 창만 닫히고 세션이 남는 것은 코드로 확정. 소프트락 시나리오는 WBP 의 back action 처리에 달려 있어 C++ 만으로는 단정 불가)

### 2. 🟡 세션 상태와 `State.Dialogue` 가 소유 클라에만 존재한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:41`, `:154`, `:329`
- **범주**: 설계/구조
- **문제**: `StartDialogueRow` 는 곧바로 `ClientStartDialogue` 로 넘기고, 이후의 모든 상태(`CurrentRowName`·`CurrentTarget`·loose 태그·포즈 재생)는 그 클라에서만 만들어진다. 리슨 호스트/싱글에서는 소유 클라와 권위가 같은 머신이라 문제가 없지만, 데디케이티드 서버에서는 다음이 전부 무력화된다.
  - `UWxAbility_Interact` 는 `ServerOnly` 인데 `State.Dialogue` 는 클라 폰 ASC 의 loose 태그라 서버에서 보이지 않는다 → 대화 중에도 상호작용이 계속 발동한다.
  - `FWxStateTreeTask_PlayDialogue::EnterState` 가 `StartDialogueRow` 직후에 검사하는 `HasActiveDialogue()`(`WxStateTreeTask_PlayDialogue.cpp:91`)는 서버에서 항상 false → 태스크가 항상 Failed 를 반환하고 경고를 남긴다.
  - `PlayPendingPose` 의 `Montage_Play` 는 로컬 재생이라 다른 클라는 NPC 포즈를 보지 못한다.
  헤더(`WxDialogueSessionComponent.h:25,28`)와 README 가 "v1 싱글/리슨 호스트 전제"로 명시하고 있어 의도된 절충이지만, 데디케이티드로 넘어갈 때 손봐야 할 지점이 세 곳으로 흩어져 있다는 사실은 기록해 둘 값이 있다.
- **제안**: 지금 고칠 필요는 없다. 데디케이티드 전환 시 (a) 세션 진행을 서버로 옮기고 표시만 클라로 내리거나, 최소한 (b) `State.Dialogue` 를 서버에서도 올리고 (c) 포즈를 Multicast 로 돌리는 세 항목을 한 묶음으로 처리한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 헤더·README 에 전제로 명시돼 있다)

### 3. 🟢 상호작용 진입점의 실패가 조용하다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:121`
- **범주**: 버그/정확성
- **문제**: `StartDialogueWith` 는 `Interactor` 가 폰이 아니거나 컨트롤러에 `UWxDialogueSessionComponent` 가 없으면 로그 없이 반환한다. 세션 컴포넌트는 Experience 주입으로 붙으므로(`Content/Framework/WAS_CoreGameplay.uasset`) 주입이 빠지거나 실패하면 정확히 이 갈래로 떨어지는데, 그때 남는 단서가 없다. 같은 모듈의 다른 실패 경로는 전부 경고를 남기며(`WxDialogueSessionComponent.cpp:34`, `:46` — "이 갈래가 조용하면 'F 를 눌러도 아무 일이 없다'만 남는다"), 여기만 그 원칙에서 벗어나 있다.
- **제안**: `LogWxDialogue` 경고 한 줄을 추가한다(대상 액터·Interactor 이름 포함).
- **확신도**: 높음

### 4. 🟢 `PlayerCameraManager` 를 검증 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:241`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()` 을 널 검사 없이 부른다. 같은 함수의 다른 참조(`Pawn`, `CurrentTarget`, 스폰된 `CameraActor`)는 모두 가드가 있어 이 한 줄만 예외다. 로컬 컨트롤러로 이미 걸렀으므로 실제 재현 가능성은 낮지만(`PlayerCameraManager` 는 `InitPlayerState` 시점에 스폰된다), 스폰 전에 대화가 열리는 조립이 생기면 크래시다.
- **제안**: `PlayerCameraManager` 가 없으면 `Side` 를 임의로 고정(예: `1.f`)하거나 카메라 경로 전체를 건너뛴다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`
- **훑은 파일**: `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **교차 확인한 소비자**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **검증했으나 문제 없음으로 판정한 것들**
  - 규칙 준수: 전 파일 Copyright 첫 줄 ✔, `Wx` prefix ✔, `WxCore` 외 Wx 플러그인 의존 없음 ✔, `BlueprintCallable` 미사용 ✔, 델리게이트 콜백 `HandlePoseLoaded` ✔, 인라인 정의는 `GetInstanceDataType()` 하나뿐이며 예외 사유 주석이 달려 있음(`WxStateTreeTask_PlayDialogue.h:13`) ✔, 람다는 ST 약한 컨텍스트 1건으로 사유 주석 동반 ✔.
  - 포즈 몽타주 GC: `PoseLoadHandle.Reset()` 이후 하드 참조가 없어도 `UAnimInstance::AddReferencedObjects` → `FAnimMontageInstance::AddReferencedObjects` 가 재생 중 몽타주를 붙잡는다(엔진 `AnimInstance.cpp:4551`, `AnimMontage.cpp:1735`). 주석의 주장이 맞다.
  - ST 태스크의 미해제 `OnDialogueEnded` 바인딩: 상태를 먼저 떠난 뒤 발화해도 `FStateTreeWeakExecutionContext` 가 활성 경로 ID(`FActiveStateID` — 진입마다 새로 발급)로 걸러 무시된다(엔진 `StateTreeAsyncExecutionContext.cpp:377`). 같은 상태를 재진입해도 ID 가 달라 오작동하지 않는다.
  - `EndDialogue` 의 `Broadcast()` → `Clear()` 순서: `FinishTask` 는 즉시 전이하지 않고 `bHasPendingCompletedState` 만 세운 뒤 다음 틱을 예약하므로(엔진 `StateTreeAsyncExecutionContext.cpp:243`) 재진입으로 새 바인딩이 지워지는 일은 없다.
  - 겹침 대화(`ClientStartDialogue` 의 선행 `EndDialogue`): 태그 1→0→1 전이 시점에 `CurrentRowName` 이 이미 채워져 있어 `UWxViewModel_Dialogue::Initialize` 의 pull 시드가 올바른 대사를 얻는다.
- **미검토 / 한계**: 대화 위젯(WBP)의 back action·입력 처리는 BP 영역이라 보지 않았다 — 발견 1의 소프트락 시나리오가 실제로 성립하는지는 그쪽 확인이 필요하다. `Build.cs` 의 `UniversalObjectLocator` 는 이 모듈 코드에서 쓰이지 않지만 저장소 6개 모듈이 동일하게 들고 있어 프로젝트 전역 관례로 보고 발견에서 제외했다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 11파일 — `/module-review`로 갱신*
