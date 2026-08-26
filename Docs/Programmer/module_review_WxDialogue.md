# WxDialogue — 코드 리뷰

> 작고 규율이 잡힌 모듈이다. 진입(서버 권위 → Client RPC → 로컬 세션)과 종료가 한 쌍으로 묶여 있고, 겹침 세션·테이블 재임포트·행 오타·빈 대사 같은 실패 경로가 모두 로그와 함께 닫힌다. 모듈 경계·코딩 규칙 위반은 0건이다(`WxCore` 외 Wx 참조 없음, `BlueprintCallable`·`FORCEINLINE` 없음, 람다 1건은 엔진 StateTree 규약, 전 파일 저작권 헤더 있음). 소스 11파일 전부를 읽고 세션 컴포넌트·ST 태스크는 로직 단위로 깊게 봤으며, 발견의 전제(`FinishTask` 의 동기/비동기 여부, Owner cascade 파괴 여부, 폰 교체 경로 존재 여부, UI 의 대화 창 닫기 시점)는 UE 5.8 엔진 소스와 `WxUI`·`WxGame` 호출부에서 직접 확인했다. 직전 리뷰(`807a9da8`) 이후 이 모듈의 C++ 변경은 없고 README 갱신뿐이라, 아래 1~4번은 그때와 같은 결함이 그대로 남아 있는 것이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 세션을 닫는 경로가 `Advance` 하나뿐이라 폰 교체·컴포넌트 소멸에서 세션이 고아가 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:197-215` (EndDialogue), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:53-86` (유일한 종료 트리거), `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:113-174` (수명주기 오버라이드 없음)
- **범주**: 설계/구조 (상태 관리)
- **문제**: `EndDialogue()` 호출부는 `Advance()` 가 끝 행에 닿거나 다음 행 해석에 실패했을 때, 그리고 다음 세션이 겹쳐 열릴 때(`:123-126`)뿐이다. 반면 UI 는 폰이 갈리는 순간 대화 창을 무조건 닫는다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:236-253`, `OnPossessedPawnChanged` → `WatchPawnTags` → `CloseDialogueScreen`). 그래서 대화 도중 폰이 교체되면 창은 사라지는데 세션은 `CurrentRowName` 을 쥔 채 살아남고, `Advance()` 를 부를 창이 없으니 스스로 풀리지 못한다 — 뷰 타겟은 대화 카메라에 남고, `OnDialogueEnded` 를 기다리는 `FWxStateTreeTask_PlayDialogue` 는 Running 에 영구히 매달려 퀘스트 진행이 멈춘다. 태그가 **폰** ASC 에 붙는다는 점(`:142-150`)이 여기에 겹친다: 새 폰에는 `State.Dialogue` 가 없어 `WxAbility_Interact` 의 차단(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)이 풀리고, 그 상태로 다른 NPC 에게 말을 걸면 겹침 경로가 stale 세션을 접으며 `OnDialogueEnded` 를 발화해 대기 중이던 ST 스텝이 **읽지도 않은 대사에 대해 Succeeded** 로 넘어간다. 정직하게 덧붙이면 지금 코드베이스에는 플레이어 폰을 갈아끼우는 경로가 없다 — `Possess` 오버라이드는 `AWxEnemyController` 뿐이고 `RestartPlayer` 는 폰이 없을 때만 불리며(`Source/WxGame/Framework/WxGameMode.cpp:112-115`) 부활은 월드 리로드다. 현재는 잠복 결함이고, 탈것·연출 폰이 들어오는 순간 실현된다. 컴포넌트가 먼저 사라지는 경우(Experience 전환으로 주입 컴포넌트가 걷힘)도 `EndPlay`·`OnUnregister` 훅이 없어 폰 ASC 의 `State.Dialogue` 가 1 로 남고 대화 카메라가 월드에 남는다. 참고로 `:245-246` 의 "컨트롤러가 사라지면 카메라도 함께 정리된다"는 사실이 아니다 — UE 5.8 `UWorld::DestroyActor` 는 파괴 대상 자신의 `SetOwner(NULL)` 만 하고 소유 액터를 cascade 파괴하지 않으며(`Actor.cpp` 의 `Children` 순회는 렌더 상태 dirty 용), 대화 카메라는 `EndDialogueCamera` 가 수명을 줄 때만 사라진다.
- **제안**: `BeginPlay` 에서 오너 PC 의 `OnPossessedPawnChanged` 를 구독해(`WxUI`·`WxSave` 가 이미 쓰는 델리게이트) `HasActiveDialogue()` 면 `EndDialogue()` 로 수렴시키고, `EndPlay` 에서도 활성 세션이면 같은 함수를 부른다. 태그·카메라·`OnDialogueEnded` 가 한 지점에서 함께 닫혀 UI 의 창 닫기와 세션 상태가 어긋나지 않는다. `:245` 주석은 "종료 시 수명을 줘 정리한다"로 정정한다.
- **확신도**: 중간 (트리거가 될 폰 교체 경로가 아직 없어 현재는 잠복이지만, 발생하면 자력 복구 수단이 없다)

### 2. 🟢 대상 없는 대사(나레이션)에 `TargetPose` 가 있으면 헛 스트리밍하고 원인과 어긋난 경고를 낸다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:285-315` (ApplyCurrentPose), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:334-343` (PlayPendingPose 경고), `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:40` (항상 `Target=nullptr`)
- **범주**: 성능/안전
- **문제**: `FWxStateTreeTask_PlayDialogue` 는 언제나 `Target=nullptr` 로 대화를 연다. 그 행에 `TargetPose` 가 채워져 있으면 `ApplyCurrentPose` 가 대상이 없는 줄 모른 채 몽타주를 비동기 로드하고, 완료 뒤 `PlayPendingPose` 가 "대상에 애님 인스턴스가 없어 포즈를 얹을 수 없다(대상 None)"를 찍는다. 실제 원인(대상 없는 대사에 포즈를 지정함)과 메시지(애님 BP 부재)가 달라 디버깅을 엉뚱한 곳으로 보낸다.
- **제안**: `ApplyCurrentPose` 초입에서 `CurrentTarget` 이 없으면 "대상 없는 대사에 포즈가 지정됨" 경고 한 줄만 남기고 반환한다 — 스트리밍도 걸지 않는다.
- **확신도**: 높음

### 3. 🟢 `StartDialogueWith` 가 세션 부재를 조용히 삼킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:24-27`
- **범주**: 버그/정확성 (미처리 실패 경로)
- **문제**: 주체가 폰이 아니거나 컨트롤러에 `UWxDialogueSessionComponent` 가 주입돼 있지 않으면 로그 없이 반환한다. 같은 모듈의 세션 컴포넌트는 시작 행 누락을 두고 "이 갈래가 조용하면 'F 를 눌러도 아무 일이 없다'만 남는다"(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:44-46`)며 경고를 찍고, ST 태스크도 같은 조립 실수를 경고로 잡는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:34-38`). Experience 가 세션 컴포넌트를 빠뜨린 조립 실수만 정확히 같은 증상을 내면서 단서가 없다 — 세 진입점의 처우가 어긋나 있다.
- **제안**: `Session` 이 없을 때 `LogWxDialogue` Warning 으로 주체·컨트롤러 이름을 남긴다.
- **확신도**: 높음

### 4. 🟢 `OnDialogueEnded` 의 Broadcast → Clear 순서가 콜백 안에서 다시 붙인 구독을 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:213-214`, 계약 주석 `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:78-82`
- **범주**: 버그/정확성
- **문제**: 헤더는 이 델리게이트를 "대화 한 번에 대한 일회성 약속"으로 선언하지만 구현은 `Broadcast()` 뒤에 `Clear()` 한다. 콜백 안에서 새 대화를 열고 다시 붙는 구독자는 곧이어 실행되는 `Clear()` 에 지워져 두 번째 대화의 종료를 영영 받지 못한다. 현재 유일한 구독자인 `FWxStateTreeTask_PlayDialogue`(`Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:51-54`)는 안전하다 — UE 5.8 `FStateTreeWeakExecutionContext::FinishTask` 가 `bHasPendingCompletedState` 만 세우고 `ScheduleNextTick` 으로 전이를 다음 틱에 미루는 것을 엔진 소스에서 확인했다. 잠복한 API 함정이다.
- **제안**: 발화 전에 옮겨 비운다 — `FSimpleMulticastDelegate Ended = MoveTemp(OnDialogueEnded); OnDialogueEnded.Clear(); Ended.Broadcast();`. 그러면 헤더의 "일회성" 약속이 재진입에서도 성립한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 README 가 약속한 관찰 API 에 소비자가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63` (GetCurrentDialogueTarget), `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:69` (GetCurrentRowHandle)
- **범주**: 중복/복잡도 (데드 코드)
- **문제**: README 와 헤더는 "진행 중인 대사의 신원(현재 행)과 대상만 노출하고 그 의미는 소비자([[WxQuest]] 등)가 관찰로 판정한다"를 이 모듈의 경계로 내세우는데, 저장소 전체에서 두 함수를 부르는 곳이 하나도 없다. `WxQuest` 는 `Dialogue` 라는 식별자조차 참조하지 않는다. `UFUNCTION` 이 아니라 BP 경로도 없으므로 실사용이 아니라 미구현 계약이다(같은 클래스의 `GetCurrentSpeaker`·`GetCurrentLine` 은 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:20` 이 쓰고, `HasActiveDialogue` 는 ST 태스크가 쓴다).
- **제안**: 대사-퀘스트 연동을 곧 붙일 계획이면 그대로 두되 README 의 서술을 "예정"으로 낮추고, 계획이 없으면 두 함수를 지워 공개 표면을 줄인다.
- **확신도**: 중간 (선행 배치한 확장 포인트일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **미검토 / 한계**:
  - 검증해서 발견에서 뺀 것: ST 태스크의 람다는 엔진이 제시하는 약한 실행 컨텍스트 전달 방식이라 코딩 규칙 3 의 "필요한 경우"에 해당한다. `GetInstanceDataType()` 의 헤더 인라인 정의는 `WxStateTreeTask_PlayDialogue.h:13` 이 명문화한 규칙 6 예외라 위반으로 세지 않았다. 포즈 스트리밍 완료 콜백은 `FStreamableDelegate::CreateUObject` 라 컴포넌트가 먼저 사라져도 안전하고, `EndDialogue` 가 스트리밍을 접지 않는 것·포즈를 되돌리지 않는 것은 헤더에 명시된 의도다. `EnterRow` 의 `FindRow<T>` 는 엔진이 RowStruct 타입을 검사하므로 잘못된 테이블을 물려도 안전하다. `WxAbility_Interact` 가 `ServerOnly` 라 `OnInteracted` 가 서버에서만 돌고, 따라서 진입이 이중 실행되지 않는 것도 확인했다.
  - 설계 전제라 발견으로 올리지 않은 것: 세션이 서버 검증 없는 로컬 표시 상태인 점, `bInteractionEnabled` 비복제, ST 태스크의 0번 컨트롤러 사용은 README·헤더가 v1(싱글/리슨 호스트) 전제로 명시한다. 다만 멀티로 확장할 때 세 지점이 걸린다 — (1) `State.Dialogue` 를 올리는 곳이 소유 클라에서 도는 `ClientStartDialogue_Implementation`(`:148`)이라 데디케이티드 서버의 폰 ASC 에는 태그가 서지 않아 `WxAbility_Interact` 의 서버 권위 차단이 성립하지 않는다. (2) 대상 포즈는 `AnimInstance->Montage_Play`(`:346`) 로컬 재생이라 월드 공유 액터인 NPC 의 자세가 다른 피어에 반영되지 않는다. (3) `FWxStateTreeTask_PlayDialogue` 는 `StartDialogueRow` 직후 `HasActiveDialogue()` 를 동기 검사하므로, Client RPC 가 실제로 지연되는 구성에서는 항상 Failed 로 떨어진다.
  - BP·Experience 에셋(세션 컴포넌트 주입 설정, 대화 위젯의 `Advance` 배선, DataTable 콘텐츠) 내부는 범위 밖이며 C++ 근거로만 판단했다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 11파일 — `/module-review`로 갱신*
