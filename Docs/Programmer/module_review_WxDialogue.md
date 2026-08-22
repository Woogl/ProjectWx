# WxDialogue — 코드 리뷰

> 작고 잘 정리된 모듈이다. 진입점(서버 권위 → Client RPC → 로컬 세션)과 종료 경로가 대칭이고, 겹침 세션·테이블 재임포트·빈 대사 같은 실패 경로가 모두 로그와 함께 닫힌다. 모듈 경계·코딩 규칙 위반은 없다. 남은 문제는 세션 수명이 "Advance 로 끝까지 읽는다"는 한 경로에만 매여 있어, 폰 교체·컴포넌트 소멸 같은 외부 전이에서 세션이 고아가 되는 것 하나다. 소스 11파일 전부를 읽었고, 세션 컴포넌트·ST 태스크는 엔진(UE 5.8 StateTree 약한 컨텍스트·PlayerCameraManager·액터 Owner 파괴) 소스까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 세션 종료가 Advance 한 경로에만 매여 있어 폰 교체·컴포넌트 소멸 시 세션이 고아가 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:41` (BeginPlay/EndPlay 오버라이드 없음), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:197-215` (EndDialogue), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:244-246` (카메라 Owner 주석)
- **범주**: 버그/정확성
- **문제**: `EndDialogue()` 는 `Advance()` 가 끝 행에 닿거나 다음 세션이 겹쳐 열릴 때만 불린다. 그런데 UI 쪽은 폰이 바뀌면 대화 창을 즉시 닫는다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:258-259`, "관찰을 놓는 순간 … 대화 창은 여기서 닫는다"). 그래서 대화 도중 폰만 갈아타는 흐름(같은 파일 `:238` 이 언급하는 탈것·연출 폰)이 끼어들면 창은 사라지는데 세션은 `CurrentRowName` 이 남은 채 살아 있고, 뷰 타겟은 대화 카메라에 고정되며, `OnDialogueEnded` 를 기다리는 `FWxStateTreeTask_PlayDialogue` 는 Running 에 머문다. `Advance()` 를 부를 창이 없으므로 다음 대화가 겹쳐 열릴 때까지 풀리지 않는다. 헤더 `:150-151` 이 "도중 폰 교체 대비"로 `TaggedAbilitySystem` 을 따로 기억하는 것을 보면 폰 교체를 의식했지만, 태그 되돌리기 외의 수렴은 없다. 같은 이유로 컴포넌트가 먼저 사라지는 경우(Experience 전환으로 주입 컴포넌트가 걷힘, 컨트롤러 파괴)에도 폰 ASC 의 `State.Dialogue` 가 1 로 남고 ST 대기가 영영 끝나지 않는다. 덧붙여 `:245` 의 "컨트롤러가 사라지면 카메라도 함께 정리된다"는 사실이 아니다 — `SpawnParams.Owner` 는 소유 관계만 맺을 뿐 엔진은 Owner 파괴 시 자식 액터를 파괴하지 않는다(`UWorld::DestroyActor` 는 자기 Owner 를 끊을 뿐이다). 대화 카메라는 `EndDialogueCamera` 가 수명을 줄 때만 사라진다.
- **제안**: `BeginPlay` 에서 오너 PC 의 `OnPossessedPawnChanged` 에 `HandlePossessedPawnChanged` 를 붙여 `HasActiveDialogue()` 면 `EndDialogue()` 로 수렴시키고(WxUI·WxSave 가 이미 같은 델리게이트를 쓰는 패턴), `EndPlay` 에서도 `HasActiveDialogue()` 면 `EndDialogue()` 를 부른다. 이렇게 하면 태그·카메라·`OnDialogueEnded` 가 한 지점에서 같이 닫히고 UI 의 창 닫기와 세션 상태가 일치한다. `:245` 주석은 "종료 시 수명을 줘 정리한다"로 고친다.
- **확신도**: 중간 (폰 교체 중 대화가 겹치는 빈도는 낮지만, 발생하면 복구 수단이 없다)

### 2. 🟢 `OnDialogueEnded` 의 Broadcast → Clear 순서가 콜백 안에서 다시 붙인 구독을 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:213-214`, 계약 주석 `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:78-82`
- **범주**: 버그/정확성
- **문제**: 헤더는 이 델리게이트를 "대화 한 번에 대한 일회성 약속"으로 선언하지만, 구현은 `Broadcast()` 뒤에 `Clear()` 한다. 콜백 안에서 새 대화를 열고 다시 `OnDialogueEnded` 에 붙는 구독자는 곧바로 `Clear()` 에 지워져 두 번째 대화의 종료를 영영 받지 못한다. 현재 유일한 구독자인 `FWxStateTreeTask_PlayDialogue` 는 `FinishTask` 가 전이를 다음 틱으로 미루므로(UE 5.8 `TStateTreeStrongExecutionContext::FinishTask` 는 `bHasPendingCompletedState` 를 세우고 `ScheduleNextTick` 만 한다) 오늘은 터지지 않는다 — 잠복한 API 함정이다.
- **제안**: 발화 전에 옮겨 비운다: `FSimpleMulticastDelegate Ended = MoveTemp(OnDialogueEnded); OnDialogueEnded.Clear(); Ended.Broadcast();`. 헤더의 "일회성" 약속이 재진입에서도 성립한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `StartDialogueWith` 가 세션 부재를 조용히 삼킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:24-27`
- **범주**: 버그/정확성
- **문제**: 주체가 폰이 아니거나 컨트롤러에 `UWxDialogueSessionComponent` 가 주입돼 있지 않으면 아무 로그 없이 반환한다. 같은 모듈의 세션 컴포넌트는 시작 행 누락을 두고 "이 갈래가 조용하면 'F 를 눌러도 아무 일이 없다'만 남는다"(`WxDialogueSessionComponent.cpp:44-47`)며 경고를 찍는데, Experience 가 세션 컴포넌트를 빠뜨린 조립 실수는 정확히 같은 증상을 내면서도 단서가 없다.
- **제안**: `Session` 이 없을 때 `LogWxDialogue` Warning 으로 주체·컨트롤러 이름을 남긴다.
- **확신도**: 높음

### 4. 🟢 대상 없는 대사(나레이션)에 `TargetPose` 가 있으면 쓸데없이 스트리밍하고 오해 소지 경고를 낸다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:289-315` (ApplyCurrentPose), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:334-343` (PlayPendingPose 경고)
- **범주**: 성능/안전
- **문제**: `FWxStateTreeTask_PlayDialogue` 는 항상 `Target=nullptr` 로 연다(`WxStateTreeTask_PlayDialogue.cpp:40`). 그 행에 `TargetPose` 가 채워져 있으면 `ApplyCurrentPose` 가 대상이 없는데도 몽타주를 비동기 로드하고, 완료 후 `PlayPendingPose` 가 "대상에 애님 인스턴스가 없어 포즈를 얹을 수 없다(대상 None)"를 찍는다. 실제 원인(대상 없는 대사에 포즈를 지정함)과 메시지(애님 BP 부재)가 어긋난다.
- **제안**: `ApplyCurrentPose` 초입에서 `CurrentTarget` 이 없으면 "대상 없는 대사에 포즈가 지정됨" 경고 한 줄을 찍고 반환한다. 스트리밍도 걸지 않는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`
- **미검토 / 한계**:
  - 검증해서 발견에서 뺀 것: ST 태스크의 람다는 엔진이 제시하는 약한 컨텍스트 전달 방식이라 규칙 3 의 "필요한 경우"에 해당한다. 상태를 떠났다가 다시 들어온 뒤 옛 세션의 `OnDialogueEnded` 가 늦게 와도 `FStateTreeWeakExecutionContext` 가 활성 `FActiveStateID`(활성화마다 새 ID)로 검증하므로 새 태스크를 오완료시키지 않는다. `EndDialogueCamera` 가 폰 없는 PC 에 `SetViewTargetWithBlend(nullptr)` 를 불러도 `APlayerCameraManager::SetViewTarget` 이 PC 로 대체한다. `GetInstanceDataType()` 의 헤더 인라인 정의는 프로젝트 전 ST 태스크(27개)가 공유하는 명문화된 예외라 규칙 6 위반으로 세지 않았다.
  - 설계 전제라 발견으로 올리지 않은 것: 세션이 서버 검증 없는 로컬 표시 상태인 점, `bInteractionEnabled` 비복제, ST 태스크의 0번 컨트롤러 사용은 README·헤더가 v1(싱글/리슨 호스트) 전제로 명시한다. 다만 `FWxStateTreeTask_PlayDialogue` 는 카테고리 "Wx" 로 어느 StateTree 에서나 고를 수 있어, 전 피어에서 도는 장치(AWxDevice) ST 에 놓이면 각 피어가 자기 0번 PC 에 대화를 열고 순수 로컬 Completed 로 끝난다 — 멀티 전환 시 퀘스트 ST 전용임을 강제하거나 권위 게이트를 두어야 한다.
  - BP·Experience 에셋(세션 컴포넌트 주입 설정, 대화 위젯의 `Advance` 호출) 내부는 범위 밖이며 C++ 근거로만 판단했다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 11파일 — `/module-review`로 갱신*
