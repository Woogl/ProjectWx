# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈이고, 널 검사·수명·로그 경로가 대체로 촘촘하며 설계 의도가 헤더 주석에 성실히 남아 있어 건강 상태는 양호하다. 이번 리뷰는 `Build.cs`·`uplugin`·전체 헤더를 읽고 `WxDialogueSessionComponent.cpp`(세션 진행·카메라·포즈 스트리밍)와 `WxStateTreeTask_PlayDialogue.cpp`를 줄 단위로 본 뒤, 모듈 밖 소비자(`WxUIManagerSubsystem`, `WxViewModel_Dialogue`, `WxAbility_Interact`)까지 따라가 계약이 실제로 맞물리는지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 세션에 중단(abort) 경로가 없어 사망·컴포넌트 해제 시 대화가 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:202` (`EndDialogue` 호출부는 65·72·81·126 뿐), `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:113`~`172`
- **범주**: 버그/정확성
- **문제**: `EndDialogue()`를 부르는 경로는 「`Advance()`가 끝에 닿음」과 「새 대화가 앞 대화를 덮음」 둘뿐이다. `EndPlay`/`OnUnregister`/`UninitializeComponent` 어느 것도 재정의하지 않고, 폰 교체·사망을 관찰하지도 않는다. 대화 중 플레이어가 죽으면(`State.Dialogue`는 어떤 어빌리티도 막지 않으므로 피격·사망이 그대로 가능하다) 시체 ASC 의 태그가 1 로 남아 대화 창이 사망 화면 위에 그대로 떠 있고, `BeginDialogueCamera`가 세운 대화 카메라 액터도 수명 지정 없이 뷰 타겟으로 남는다(`EndDialogueCamera`가 불리지 않으므로 `SetLifeSpan`도 걸리지 않는다). 리스폰 시 `AutoManageActiveCameraTarget`·`WatchPawnTags`가 화면과 UI 는 되돌려 주지만, 그 카메라 액터는 PC 가 죽을 때까지 남고 `CurrentRowName`도 남아 `HasActiveDialogue()`가 계속 true 다. 같은 이유로 Experience 해제·레벨 이동으로 컴포넌트가 사라지면 `OnDialogueEnded`가 영영 발화하지 않아 `Play Dialogue` 태스크가 `Running` 으로 멈춘다.
- **제안**: `UActorComponent::EndPlay`(또는 `UninitializeComponent`)를 재정의해 `HasActiveDialogue()`면 `EndDialogue()`를 부른다. 사망·폰 교체까지 덮으려면 컨트롤러의 `OnPossessedPawnChanged` 또는 `Ability_Death` 태그 이벤트를 세션 중에만 구독해 같은 지점으로 모은다.
- **확신도**: 중간

### 2. 🟡 대화→대화 전환에서 `State.Dialogue`가 1→0→1 로 튀어 대화 창이 닫혔다 다시 열린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:124`~`157`
- **범주**: 설계/구조
- **문제**: `ClientStartDialogue_Implementation`은 겹침을 `EndDialogue()`로 먼저 접은 뒤(154 줄 전에 태그가 0 으로 내려간다) 새 세션을 채우고 다시 태그를 1 로 올린다. 소비자인 `Plugins/WxUI/.../WxUIManagerSubsystem.cpp:268`의 `HandleDialogueTagChanged`는 0↔비0 전이만 듣고, 0 에서 `CloseDialogueScreen()`으로 위젯을 실제로 파괴한 뒤 1 에서 `UWxAsyncAction_PushWidgetToLayer`로 **비동기** 재푸시한다. 결과적으로 퀘스트 트리가 대화 도중 다른 대사를 여는(주석이 실재한다고 밝힌) 경로에서 대화 창이 최소 한 프레임 사라졌다 다시 뜬다. 카메라도 `EndDialogueCamera`의 폰 복귀 블렌드와 `BeginDialogueCamera`의 새 카메라 블렌드가 같은 프레임에 겹친다.
- **제안**: 겹침 전용 경로를 두어 태그 카운트를 내리지 않고(태그 발행/회수는 「세션이 하나도 없음↔있음」 전이에만) 카메라·포즈·행만 교체하도록 나눈다. 최소 조치로는 겹침일 때 `EndDialogueCamera` 호출과 태그 회수를 건너뛰는 갈래를 `EndDialogue`에 인자로 넣는 방법이 있다.
- **확신도**: 중간

### 3. 🟡 대상 포즈가 소유 클라에서만 재생되어 다른 머신에는 보이지 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:351`
- **범주**: 설계/구조
- **문제**: `PlayPendingPose`가 `AnimInstance->Montage_Play(Pose)`를 `ClientStartDialogue`가 실행된 머신에서만 부른다. 헤더가 밝힌 전제는 「세션(현재 노드·라인)은 **표시 전용 로컬 상태**」인데, 포즈는 플레이어 로컬 UI 가 아니라 공유 액터인 NPC 의 가시 상태를 바꾸는 것이라 그 전제에 정확히 포섭되지 않는다. 리슨 호스트에 2인 이상이 접속하면 대화에 참여하지 않은 플레이어는 NPC 가 대사 포즈를 취하는 것을 보지 못한다.
- **제안**: v1 전제를 유지한다면 헤더의 로컬 상태 문단에 「포즈도 로컬 재생이며 다른 머신에는 반영되지 않는다」를 명시해 나중에 서버로 옮길 목록에 넣는다. 실제로 공유가 필요해지면 포즈 재생만 NPC 측 GameplayCue 나 Multicast 로 분리한다.
- **확신도**: 중간(v1 싱글/리슨 호스트 전제 하에서는 의도된 설계일 수 있음)

### 4. 🟢 `UniversalObjectLocator` Build.cs 의존이 미사용으로 남아 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: 현재 소스 어디에서도 `FUniversalObjectLocator`를 쓰지 않는다(`Plugins/WxDialogue/Source` 전체 grep 결과 Build.cs 한 줄뿐). `Intermediate` 에만 남은 `WxStateTreeTask_EnableNpcInteraction.gen.cpp` 가 이 의존을 쓰던 파일의 잔재로, 클래스가 제거되면서 의존만 남았다.
- **제안**: `PublicDependencyModuleNames`에서 제거한다.
- **확신도**: 높음

### 5. 🟢 `PlayerCameraManager` 널 검사 누락
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:241`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()`을 널 검사 없이 역참조한다. `GetLocalPlayerController()`로 로컬 컨트롤러를 이미 걸렀으므로 실무상 항상 유효하지만, 같은 함수의 폰·`CurrentTarget`·`CameraActor`는 모두 검사하고 있어 여기만 비대칭이다.
- **제안**: `Pawn`·`CurrentTarget` 검사와 같은 조건문에 `PlayerController->PlayerCameraManager`를 얹는다(널이면 카메라 경로를 통째로 건너뛴다).
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **모듈 밖 대조**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxNpc.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, 엔진 `StateTreeAsyncExecutionContext.cpp`
- **규칙 점검 결과**: 위반 없음. 11개 파일 모두 `// Copyright Woogle. All Rights Reserved.`로 시작하고, `Wx` prefix·`Handle` prefix(`HandlePoseLoaded`)를 지키며, `BlueprintCallable`은 하나도 없고(`BlueprintAssignable`만 사용), 헤더의 인라인 정의는 규칙 6 의 예외인 `GetInstanceDataType()` 하나뿐이며 예외 사유 주석이 붙어 있다. Build.cs 의 Wx 의존은 `WxCore` 하나다. `WxStateTreeTask_PlayDialogue.cpp:50`의 람다는 `FStateTreeWeakExecutionContext`를 값 캡처해야 하는 USTRUCT const 노드라 `CreateUObject`/`CreateSP` 로 대체할 수 없어 규칙 3 의 「반드시 필요한 경우」에 해당한다.
- **미검토 / 한계**: `OnDialogueEnded.Broadcast()` 직후의 `Clear()`(`WxDialogueSessionComponent.cpp:218`~`219`)는 브로드캐스트 중에 새 대화가 열려 재등록되면 그 등록까지 지워버리는 구조라 위험해 보였으나, 엔진 `StateTreeAsyncExecutionContext.cpp:216`~`247`을 확인한 결과 `FinishTask`가 `bHasPendingCompletedState`만 세우고 다음 틱을 예약할 뿐 전이를 동기 실행하지 않아 현재 코드 경로에서는 재진입이 발생하지 않는다 — 발견으로 올리지 않았다. BP/WBP 자산과 데이터 테이블 실제 편성 내용은 범위 밖이며, 실제 플레이 검증(사망 중 대화, 대화 겹침 연출)은 하지 않았다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 11파일 — `/module-review`로 갱신*
