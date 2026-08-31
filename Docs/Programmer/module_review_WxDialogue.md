# WxDialogue — 코드 리뷰

> 11파일 규모의 작고 응집도 높은 모듈이다. 수명·소유권 근거가 헤더 주석에 잘 적혀 있고 데이터 오류 경고도 촘촘하며, CLAUDE.md 의 코딩·모듈 규칙 위반은 확인되지 않았다(WxCore 외 참조 없음, Copyright 첫 줄 전부 존재, 람다 1건은 사유 주석 있음, `GetInstanceDataType()` 인라인은 예외 주석 있음). 이번 리뷰는 README·Build.cs·uplugin 과 Public/Private 전체 `.h`·`.cpp` 를 읽고, 세션의 실제 소비 지점(`WxGame` 뷰모델, `WxUI` 매니저의 `State.Dialogue` 게이트, `WxGame` 상호작용 어빌리티)까지 따라가 계약이 실제로 맞물리는지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 대화 창이 뜨지 않거나 도중에 닫히면 세션이 영구히 열린 채 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:145`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:54`
- **범주**: 버그/정확성
- **문제**: 세션을 닫는 경로는 `Advance()` 와 새 세션의 `ClientStartDialogue` 둘뿐이고, `Advance()` 를 부르는 곳은 프로젝트 전체에서 대화 창의 뷰모델(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:48`) 하나다. 그런데 그 창을 여는 것은 `State.Dialogue` 태그이고, 태그는 시작 시점에 폰 ASC 를 찾았을 때만 올라간다 — `ClientStartDialogue_Implementation:145` 의 `if (ASC)` 에 else 갈래가 없어, 폰이 없거나(폰 없는 Experience 는 유효한 구성이다) ASC 가 없으면 세션만 열리고 창은 뜨지 않으며 로그도 남지 않는다. 대화 도중 폰이 교체돼도 같다: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:244` 가 관찰을 옮기며 창을 닫고, 새 폰 ASC 에는 태그가 없어 창이 다시 뜨지 않는다(태그는 `TaggedAbilitySystem` 이 붙잡은 옛 ASC 에 그대로 남는다). 두 경우 모두 `HasActiveDialogue()` 가 영영 참이라 `OnDialogueEnded` 가 발화하지 않고, 폴링 없이 그 신호만 기다리는 `FWxStateTreeTask_PlayDialogue`(`Private/WxStateTreeTask_PlayDialogue.cpp:50`)가 무한히 Running 에 머물러 퀘스트 단계가 멈춘다. 다른 대화를 새로 열기 전까지 복구 수단이 없다.
- **제안**: 세션에 외부 종료 경로를 하나 준다(예: 공개 `EndDialogue()` 또는 취소 진입점). 최소한 ASC 를 못 찾은 갈래에 Warning 을 남기고, 그 경우 세션을 열지 않고 실패로 되돌려 `Play Dialogue` 가 Failed 로 끝나게 한다. 폰 교체는 `APawn`/`Controller` 전이를 세션이 직접 듣고 태그를 새 ASC 로 옮기거나 세션을 접는 것으로 처리한다.
- **확신도**: 중간

### 2. 🟡 세션 교체가 앞선 `Play Dialogue` 태스크를 정상 완료로 오인시킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:124`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:213`
- **범주**: 버그/정확성
- **문제**: 새 세션을 열 때 활성 세션을 `EndDialogue()` 로 닫는데, 이 함수는 종료 사유와 무관하게 `OnDialogueEnded` 를 broadcast 한다. 앞 세션을 연 `FWxStateTreeTask_PlayDialogue` 는 이 신호를 `Succeeded` 로 받으므로, 대사를 끝까지 읽지 않고 다른 대화에 밀려난 경우에도 퀘스트 단계가 완료로 넘어간다. 덧붙여 `EndDialogue()` 는 broadcast 직후 `Clear()` 로 목록을 통째로 비우므로(`:214`), 종료 신호를 듣고 그 자리에서 다음 대화를 여는 소비자가 생기면 그쪽이 방금 건 등록까지 지워진다 — 지금은 StateTree 의 `FinishTask` 가 즉시 전이하지 않아 드러나지 않는 잠재 위험이다.
- **제안**: 완주와 중단을 구분한다. 교체로 인한 종료에는 신호를 발행하지 않거나 중단 사유를 함께 넘겨 태스크가 `Failed`/명시적 취소로 처리하게 한다. `Clear()` 는 broadcast 전에 목록을 지역 복사로 옮겨 놓고 비우는 순서로 바꾸면 재진입에 안전해진다.
- **확신도**: 중간

### 3. 🟡 권위 모델이 리슨 호스트 전제를 벗어나면 즉시 실패하거나 게이트가 무력화된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:39`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:42`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:148`
- **범주**: 설계/구조
- **문제**: StateTree 태스크는 `ClientStartDialogue` RPC 를 보낸 직후 서버 측 `HasActiveDialogue()` 를 검사한다. 데디케이티드 서버라면 세션 상태가 클라에만 생기므로 이 검사는 항상 거짓이 되어 태스크가 곧바로 Failed 로 끝난다. `State.Dialogue` 도 클라 ASC 에만 loose tag 로 올라가므로, 이 태그를 `ActivationBlockedTags` 로 쓰는 서버 실행 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:36`)의 차단이 서버에서는 작동하지 않는다. README·헤더에 v1 싱글/리슨 호스트 전제가 명시돼 있으나, 전제가 깨지는 순간 조용한 실패로만 드러난다.
- **제안**: 현 전제를 유지한다면 넷모드가 전제와 어긋날 때 경고를 남긴다. 멀티플레이로 확장할 때는 세션 상태와 차단 태그를 서버가 소유하고, 태스크 완료는 서버 확인 뒤 처리한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — README·헤더에 v1 전제가 명시돼 있다)

### 4. 🟢 세션 주입 누락과 잘못된 Interactor 가 조용히 실패한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:24`
- **범주**: 버그/정확성
- **문제**: `StartDialogueWith` 는 Interactor 가 Pawn 이 아니거나 컨트롤러에 `UWxDialogueSessionComponent` 가 없으면 로그 없이 반환한다. 시작 행 미지정·행 없음·대사 빔·포즈 대상 부재를 전부 Warning 으로 찍는 모듈이라, Experience 의 세션 컴포넌트 주입이 빠진 조립 오류만 "상호작용해도 아무 일 없음"으로 남는다.
- **제안**: 세션을 얻지 못한 갈래에 대상·Interactor 이름을 포함한 Warning 을 남긴다.
- **확신도**: 높음

### 5. 🟢 쓰지 않는 모듈 의존성이 Public 으로 전파된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:16`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: `UniversalObjectLocator` 는 모듈 소스 전체에서 참조가 하나도 없어 빌드 그래프에만 남아 있다. `GameplayAbilities`·`GameplayTags` 는 cpp 에서만 쓰이는데 Public 에 있어, WxDialogue 를 참조하는 모든 모듈로 불필요하게 전파된다.
- **제안**: `UniversalObjectLocator` 를 제거하고 `GameplayAbilities`·`GameplayTags` 를 `PrivateDependencyModuleNames` 로 내린다. Public 헤더가 실제로 요구하는 것은 `Core`/`CoreUObject`/`Engine`/`ModularGameplay`(`UControllerComponent`)/`StateTreeModule`(`FStateTreeTaskBase`)/`WxCore`(`IWxInteractable`) 뿐이다.
- **확신도**: 높음

### 6. 🟢 호출자 없는 공개 접근자가 남아 문서와 코드가 어긋난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:62`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:68`
- **범주**: 중복/복잡도
- **문제**: `GetCurrentDialogueTarget()`·`GetCurrentRowHandle()` 은 프로젝트 전체에 호출자가 없고 `UFUNCTION` 도 아니라 BP 에서도 닿지 않는다. 헤더와 README 는 "관찰자가 현재 행을 읽어 판정(예: WxQuest)" 이라는 계약을 전제로 서술하지만, WxQuest 소스에는 dialogue 참조가 하나도 없어 그 관찰자는 존재하지 않는다. 실제로 쓰이는 것은 뷰모델이 pull 하는 `GetCurrentSpeaker()`/`GetCurrentLine()` 뿐이다.
- **제안**: 소비자가 생길 때까지 두 접근자를 지우고(멤버 `CurrentTarget` 은 카메라·포즈가 쓰므로 유지), 헤더·README 의 관찰자 서술을 현재 상태에 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`
- **참고로 읽은 모듈 밖 파일**(발견의 근거 확인용): `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`
- **미검토 / 한계**: DataTable 에셋과 BP/WBP 그래프는 범위 밖이라 노드 그래프의 순환·미종료는 코드 경로로만 확인했다(`Advance()` 는 호출 1회당 1행만 나아가므로 순환 행이 있어도 코드가 무한 루프에 빠지지는 않는다). 원격 Client RPC 의 DataTable NetGUID 해소와 데디케이티드 서버 동작은 실행 검증하지 않아 발견 3 은 낮은 확신도로 남긴다. 포즈 스트리밍의 취소·늦은 도착 처리는 헤더 주석이 밝힌 의도(포즈는 되돌리지 않는다)와 일치해 문제로 잡지 않았다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 11파일 — `/module-review`로 갱신*
