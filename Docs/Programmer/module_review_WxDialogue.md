# WxDialogue — 코드 리뷰

> 작고 규율이 잡힌 모듈이다. 진입(서버 권위 → Client RPC → 로컬 세션)과 종료가 대칭이고, 겹침 세션·테이블 재임포트·빈 대사·행 오타 같은 실패 경로가 모두 로그와 함께 닫힌다. 모듈 경계·코딩 규칙 위반은 없다(WxCore 외 참조 없음, `BlueprintCallable`·`FORCEINLINE` 0건, 람다 1건은 엔진 규약). 소스 11파일 전부를 다시 읽었고, 발견의 전제(Owner 파괴 cascade 여부, 폰 교체 시 UI 동작, 상호작용 계약 소비자)는 UE 5.8 엔진 소스와 `WxUI`·`WxWorld`·`WxGame` 호출부까지 대조해 재확인했다. 직전 리뷰(`bd689a19`) 이후 코드 변경은 `IsInteractionEnabled` → `CanInteract` 개명 2줄뿐이라 발견 4건이 모두 유효하게 남았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 세션 종료가 `Advance` 한 경로에만 매여 있어 폰 교체·컴포넌트 소멸 시 세션이 고아가 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:197-215` (EndDialogue), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:53-86` (유일한 종료 트리거), `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:113-174` (BeginPlay/EndPlay 오버라이드 없음)
- **범주**: 버그/정확성
- **문제**: `EndDialogue()` 를 부르는 곳은 `Advance()` 가 끝 행에 닿거나 해석에 실패했을 때, 그리고 다음 세션이 겹쳐 열릴 때(`:123-126`)뿐이다. 반면 UI 는 폰이 바뀌는 순간 대화 창을 무조건 닫는다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:252-253`, "관찰을 놓는 순간 … 대화 창은 여기서 닫는다"). 그래서 대화 도중 폰이 갈리면 창은 사라지는데 세션은 `CurrentRowName` 이 남은 채 살아 있고, 뷰 타겟은 대화 카메라에 고정되며, `OnDialogueEnded` 를 기다리는 `FWxStateTreeTask_PlayDialogue` 는 Running 에 머문다 — `Advance()` 를 부를 창이 없으니 다음 대화가 겹쳐 열릴 때까지 풀리지 않는다. 더 나쁜 것은 태그가 **폰** ASC 에 붙는다는 점이다(`:142-150`). 폰이 갈리면 새 폰에는 `State.Dialogue` 가 없어 `WxAbility_Interact` 의 차단(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)이 풀리고, 그 상태로 NPC 에게 말을 걸면 겹침 경로가 stale 세션을 접으면서 `OnDialogueEnded` 를 발화해 대기 중이던 퀘스트 ST 스텝이 **읽지도 않은 대사에 대해 Succeeded** 로 넘어간다. 단 정직하게 덧붙이면, 코드베이스에 플레이어 폰을 갈아끼우는 경로는 아직 없다(`Possess(` 호출은 `AWxEnemyController` 뿐이고 부활은 월드 리로드다) — 지금은 잠복 결함이고, 탈것·연출 폰이 들어오는 순간 실현된다. 같은 이유로 컴포넌트가 먼저 사라지는 경우(Experience 전환으로 주입 컴포넌트가 걷힘)에도 `EndPlay` 훅이 없어 폰 ASC 의 `State.Dialogue` 가 1 로 남는다. 덧붙여 `:245` 의 "컨트롤러가 사라지면 카메라도 함께 정리된다"는 사실이 아니다 — `SpawnParams.Owner` 는 소유 관계만 맺을 뿐, UE 5.8 `UWorld::DestroyActor` 는 소유 액터를 cascade 파괴하지 않는다(`Actor.cpp` 의 `Children` 순회는 렌더 상태 dirty 용이다). 대화 카메라는 `EndDialogueCamera` 가 수명을 줄 때만 사라진다.
- **제안**: `BeginPlay` 에서 오너 PC 의 `OnPossessedPawnChanged` 에 `HandlePossessedPawnChanged` 를 붙여 `HasActiveDialogue()` 면 `EndDialogue()` 로 수렴시키고(`WxUI`·`WxSave` 가 이미 쓰는 델리게이트), `EndPlay` 에서도 활성 세션이면 `EndDialogue()` 를 부른다. 그러면 태그·카메라·`OnDialogueEnded` 가 한 지점에서 함께 닫혀 UI 의 창 닫기와 세션 상태가 일치한다. `:245` 주석은 "종료 시 수명을 줘 정리한다"로 정정한다.
- **확신도**: 중간 (트리거가 될 폰 교체 경로가 아직 코드에 없어 현재는 잠복이지만, 발생하면 복구 수단이 없다)

### 2. 🟢 `OnDialogueEnded` 의 Broadcast → Clear 순서가 콜백 안에서 다시 붙인 구독을 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:213-214`, 계약 주석 `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:78-82`
- **범주**: 버그/정확성
- **문제**: 헤더는 이 델리게이트를 "대화 한 번에 대한 일회성 약속"으로 선언하지만, 구현은 `Broadcast()` 뒤에 `Clear()` 한다. 콜백 안에서 새 대화를 열고 다시 `OnDialogueEnded` 에 붙는 구독자는 곧바로 뒤따르는 `Clear()` 에 지워져 두 번째 대화의 종료를 영영 받지 못한다. 현재 유일한 구독자인 `FWxStateTreeTask_PlayDialogue` 는 `FinishTask` 가 전이를 다음 틱으로 미루므로(UE 5.8 약한 실행 컨텍스트는 `bHasPendingCompletedState` 만 세우고 `ScheduleNextTick`) 오늘은 터지지 않는다 — 잠복한 API 함정이다.
- **제안**: 발화 전에 옮겨 비운다: `FSimpleMulticastDelegate Ended = MoveTemp(OnDialogueEnded); OnDialogueEnded.Clear(); Ended.Broadcast();`. 헤더의 "일회성" 약속이 재진입에서도 성립한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `StartDialogueWith` 가 세션 부재를 조용히 삼킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:19-30` (특히 `:24-27`)
- **범주**: 버그/정확성
- **문제**: 주체가 폰이 아니거나 컨트롤러에 `UWxDialogueSessionComponent` 가 주입돼 있지 않으면 아무 로그 없이 반환한다. 같은 모듈의 세션 컴포넌트는 시작 행 누락을 두고 "이 갈래가 조용하면 'F 를 눌러도 아무 일이 없다'만 남는다"(`WxDialogueSessionComponent.cpp:44-47`)며 경고를 찍는데, Experience 가 세션 컴포넌트를 빠뜨린 조립 실수는 정확히 같은 증상을 내면서도 단서가 없다. 같은 조립 실수를 ST 태스크 쪽은 이미 경고로 잡는다(`WxStateTreeTask_PlayDialogue.cpp:34-38`) — 두 진입점의 처우가 어긋나 있다.
- **제안**: `Session` 이 없을 때 `LogWxDialogue` Warning 으로 주체·컨트롤러 이름을 남긴다.
- **확신도**: 높음

### 4. 🟢 대상 없는 대사(나레이션)에 `TargetPose` 가 있으면 쓸데없이 스트리밍하고 오해 소지 경고를 낸다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:285-315` (ApplyCurrentPose), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:334-343` (PlayPendingPose 경고), `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:40` (항상 Target=nullptr)
- **범주**: 성능/안전
- **문제**: `FWxStateTreeTask_PlayDialogue` 는 항상 `Target=nullptr` 로 대화를 연다. 그 행에 `TargetPose` 가 채워져 있으면 `ApplyCurrentPose` 가 대상이 없는데도 몽타주를 비동기 로드하고, 완료 후 `PlayPendingPose` 가 "대상에 애님 인스턴스가 없어 포즈를 얹을 수 없다(대상 None)"를 찍는다. 실제 원인(대상 없는 대사에 포즈를 지정함)과 메시지(애님 BP 부재)가 어긋나 디버깅을 엉뚱한 곳으로 보낸다.
- **제안**: `ApplyCurrentPose` 초입에서 `CurrentTarget` 이 없으면 "대상 없는 대사에 포즈가 지정됨" 경고 한 줄을 찍고 반환한다. 스트리밍도 걸지 않는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`
- **미검토 / 한계**:
  - 검증해서 발견에서 뺀 것: ST 태스크의 람다는 엔진이 제시하는 약한 컨텍스트 전달 방식이라 규칙 3 의 "필요한 경우"에 해당한다. `GetInstanceDataType()` 의 헤더 인라인 정의는 프로젝트 전 ST 태스크가 공유하는 명문화된 예외라(`WxStateTreeTask_PlayDialogue.h:13`) 규칙 6 위반으로 세지 않았다. 포즈 스트리밍 콜백은 `FStreamableDelegate::CreateUObject` 라 컴포넌트가 먼저 사라져도 안전하고, `EndDialogue` 가 스트리밍을 접지 않는 것·포즈를 되돌리지 않는 것은 헤더에 명시된 의도다. `GetTalkPrompt()` 가 스캐너 폴링마다 `FText::Format` 을 새로 만드는 것은 `AWxItemPickup::GetInteractionPrompt` 도 같은 모양이라 프로젝트 규범으로 보고 제외했다(스캔은 타이머 주기).
  - 설계 전제라 발견으로 올리지 않은 것: 세션이 서버 검증 없는 로컬 표시 상태인 점, `bInteractionEnabled` 비복제, ST 태스크의 0번 컨트롤러 사용은 README·헤더가 v1(싱글/리슨 호스트) 전제로 명시한다. 다만 멀티로 갈 때 두 지점이 걸린다 — (1) 세션 컴포넌트는 복제·Client RPC 를 갖췄는데 그 RPC 가 실어 나르는 `AWxDialogueActor` 는 `bReplicates` 를 켜지 않아(`WxDialogueActor.cpp:7-10`, 파생 `AWxNpc` 도 동일) 원격 클라에서 `Target` 이 항상 null 로 도착해 대화 카메라가 서지 않는다. (2) `FWxStateTreeTask_PlayDialogue` 는 카테고리 "Wx" 로 어느 StateTree 에서나 고를 수 있어, 전 피어에서 도는 장치(`AWxDevice`) ST 에 놓이면 각 피어가 자기 0번 PC 에 대화를 열고 순수 로컬로 완료한다.
  - BP·Experience 에셋(세션 컴포넌트 주입 설정, 대화 위젯의 `Advance` 호출 배선) 내부는 범위 밖이며 C++ 근거로만 판단했다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 11파일 — `/module-review`로 갱신*
