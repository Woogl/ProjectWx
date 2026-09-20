# WxDialogue — 코드 리뷰

> 844줄 11파일의 작고 응집도 높은 모듈이다. 설계 근거가 주석으로 충실히 남아 있고 모듈 경계(`WxCore` 외 Wx 의존 없음)·코딩 규칙 위반은 한 건도 없다. 남은 문제는 전부 "세션이 정상 완주하지 못했을 때"의 종료 경로에 몰려 있다. 이번 리뷰는 `Build.cs`·공개 헤더 전부와 `WxDialogueSessionComponent.cpp`·`WxStateTreeTask_PlayDialogue.cpp` 전문을 읽고, 소비자(`UWxViewModel_Dialogue`·`UWxUIManagerSubsystem`·`UWxAbility_Interact`)와 엔진 측 `FStateTreeWeakExecutionContext::FinishTask`·`AController::OnPossessedPawnChanged` 동작까지 대조해 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 중단된 대화도 `OnDialogueEnded` 하나로 뭉쳐져 ST 태스크가 언제나 Succeeded 를 낸다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:230`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:50`
- **범주**: 버그/정확성
- **문제**: `EndDialogue()` 는 종료 사유를 구분하지 않고 `OnDialogueEnded` 를 발화하며, `FWxStateTreeTask_PlayDialogue` 의 리스너는 그것을 무조건 `EStateTreeFinishTaskType::Succeeded` 로 변환한다. `EndDialogue()` 에 이르는 5개 경로 중 정상 완주는 `WxDialogueSessionComponent.cpp:91`(`NextRow=None`) 하나뿐이고 나머지는 전부 중단이다 — 재임포트로 테이블이 갈림(`:84`), 다음 행 해석 실패(`:100`, 오타·행 삭제), 빙의 전이(`:169`, 대화 중 사망·리스폰), 겹쳐 열린 새 대화가 앞 세션을 강제 종료(`:131`). 따라서 플레이어가 대화 중 죽거나, 테이블에 오타가 있거나, 다른 대화가 끼어들기만 해도 "대화 재생" 태스크는 Succeeded 로 완료되고 그 전이를 탄 퀘스트가 읽지 않은 대사에 전진한다. 모듈 README 가 "대화는 기록을 남기지 않고 종료를 기다린 쪽이 의미를 판정한다"고 못 박았는데, 정작 소비자가 완주와 중단을 구분할 신호가 없다.
- **제안**: `OnDialogueEnded` 에 완주 여부 1비트를 실어(예: `DECLARE_MULTICAST_DELEGATE_OneParam(..., bool /*bCompleted*/)`) 정상 완주 경로에서만 true 를 넘기고, 태스크는 false 일 때 `Failed` 로 마감한다. 5개 호출부 각각이 자기 사유를 이미 알고 있으므로 `EndDialogue(bool bCompleted)` 인자 추가만으로 끝난다.
- **확신도**: 높음

### 2. 🟡 대화 대상이 파괴돼도 세션이 끝까지 진행된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:239`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:351`
- **범주**: 버그/정확성
- **문제**: `CurrentTarget` 의 유효성은 `BeginDialogueCamera()` 진입 시 한 번만 본다. 세션 도중 대상이 파괴되면 `Advance()`(`:73`)는 아무것도 눈치채지 못하고 남은 대사를 끝까지 흘린다. 대화 카메라는 스폰 시점 트랜스폼에 고정된 `ACameraActor` 라 아무도 없는 지점을 계속 겨누고, 포즈가 지정된 행마다 `PlayPendingPose()` 가 "대상에 애님 인스턴스가 없다" 경고만 반복해 찍는다. 플레이어에게는 사라진 NPC 와 대화가 이어지는 모습으로 보인다. 크래시는 나지 않는다(`CurrentTarget`·`PendingPoseTarget` 모두 약참조).
- **제안**: `Advance()` 초입에서 "대상이 있었는데 지금 무효"인 경우(`CurrentTarget` 을 세션 시작 시 지정받았는지 여부와 함께) 세션을 접는다. 나레이션 경로는 애초에 대상이 없으므로 이 판정에 걸리지 않게 구분이 필요하다.
- **확신도**: 중간

### 3. 🟡 데디케이티드 서버에서는 ST "대화 재생"이 항상 Failed 이고 서버측 대화 게이팅이 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:39`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:158`
- **범주**: 설계/구조
- **문제**: 세션 상태는 `ClientStartDialogue` Client RPC 안에서만 세워지므로, 서버에서 도는 StateTree 는 `StartDialogueRow()` 직후 `HasActiveDialogue()` 가 false 인 것을 보고 경고와 함께 `Failed` 를 낸다 — 대사는 클라에서 정상 재생되는데 퀘스트 트리만 실패 분기로 빠진다. 같은 이유로 `State.Dialogue` loose 태그도 클라 ASC 에만 올라가, 서버 권위로 도는 `UWxAbility_Interact`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:36`)의 `ActivationBlockedTags` 가 서버에서는 걸리지 않는다(현재는 클라 예측이 먼저 막아 주는 것에 의존). 헤더·README 가 "v1 싱글/리슨 호스트 전제"로 명시한 사안이지만, 데디케이티드로 넘어가는 순간 가장 먼저 깨지는 지점이므로 기록해 둔다.
- **제안**: 멀티 확장 시 진행 상태를 서버 권위로 올리고(현재 행 이름을 복제, `Advance` 는 Server RPC), 태그도 권위 측에서 발행한다. 그 전까지는 데디케이티드 타겟을 지원 대상에서 제외한다는 것을 릴리스 노트급으로 남겨 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 `EndDialogue()` 의 `Broadcast()` → `Clear()` 순서가 재진입에 취약하다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:230`
- **범주**: 버그/정확성
- **문제**: 브로드캐스트 도중 어떤 리스너가 새 대화를 열어 `OnDialogueEnded` 에 새 바인딩을 걸면, 직후의 `Clear()` 가 그 새 바인딩까지 지운다(그 대화의 종료를 기다리는 쪽이 영영 깨어나지 못한다). 나아가 `ClientStartDialogue` 의 겹침 처리(`:131`)는 `EndDialogue()` 반환 후 계속 진행하므로, 그 사이에 열린 세션의 카메라 액터가 `DialogueCamera` 에서 밀려나 수명도 못 받고 월드에 남는다. 다만 현재 유일한 리스너인 ST 태스크의 `FStateTreeWeakExecutionContext::FinishTask` 는 완료 상태만 기록하고 다음 틱을 예약할 뿐 전이를 동기 실행하지 않음을 엔진 소스에서 확인했으므로(UE 5.8 `StateTreeAsyncExecutionContext.cpp`), 지금은 발현하지 않는 잠복 함정이다.
- **제안**: 로컬 델리게이트로 옮겨 담고(`FSimpleMulticastDelegate Ended = MoveTemp(OnDialogueEnded);`) 멤버를 먼저 비운 뒤 브로드캐스트한다.
- **확신도**: 중간

### 5. 🟢 `EndPlay` 가 진행 중 세션을 접지 않아 대화 카메라가 수명 없이 남는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:39`
- **범주**: 버그/정확성
- **문제**: `EndPlay` 는 빙의 델리게이트만 해제하고 세션은 그대로 둔다. 대화 도중 컨트롤러가 파괴되면 `EndDialogueCamera()` 가 돌지 않아 스폰해 둔 `ACameraActor` 가 `SetLifeSpan`(`:299`)을 못 받고 월드에 남으며, `OnDialogueEnded` 도 발화하지 않아 기다리던 태스크가 Running 에 굳는다. 실제로는 월드 정리와 함께 걷히는 경우가 대부분이라 영향은 작다.
- **제안**: `EndPlay` 에서 `Super::EndPlay` 앞에 `if (HasActiveDialogue()) { EndDialogue(); }` 한 줄.
- **확신도**: 중간

### 6. 🟢 `UWxDialogueComponent` 의 `BlueprintSpawnableComponent` 가 클래스 주석이 금한 사용을 유도한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:16`
- **범주**: 설계/구조
- **문제**: 클래스 주석과 README 가 "이 컴포넌트를 아무 액터에 붙여도 말을 걸 수 있게 되지는 않는다(상호작용 계약은 `AWxDialogueActor` 전용)"고 명시하는데, `meta = (BlueprintSpawnableComponent)` 는 Add Component 메뉴에 이 컴포넌트를 노출해 기획자에게 정확히 그 오용을 권한다. 붙여 봐야 무반응인 채로 남아 디버깅 시간을 먹는다.
- **제안**: `BlueprintSpawnableComponent` 메타를 제거한다. `AWxDialogueActor` 가 네이티브 서브오브젝트로 만들므로 저작에 필요 없다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`
- **대조한 모듈 밖 파일**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **확인했고 문제 없던 항목**: 모듈 의존성(`WxCore` 외 Wx 플러그인 참조 0건, `.Build.cs`·실제 include 양쪽 확인), 11개 파일 전부의 Copyright 첫 줄, `Wx` Prefix, 인라인 정의 금지(`GetInstanceDataType()` 의 예외 사유 주석 존재), `Handle` prefix 콜백, `Super::` 호출, `BlueprintCallable` 오용 없음, `CurrentStartRow` 를 `UPROPERTY` 로 잡아 테이블 GC 를 막는 처리, 행 포인터 비캐시 규약, 포즈 스트리밍 핸들의 취소·수명 처리
- **미검토 / 한계**: 대화 DataTable 에셋과 대화 위젯 WBP 의 실제 값(리뷰 범위 밖). 카메라 구도 수식(`BeginDialogueCamera` 의 off-axis 계산)은 논리적 타당성만 읽었고 실제 화면으로 검증하지 않았다. `FDataTableRowHandle` 을 Client RPC 인자로 넘길 때의 네트워크 직렬화는 표준 프로퍼티 복제에 의존한다고 보고 실제 패킷을 확인하지 않았다(데디케이티드 전제 자체가 발견 3 에 걸려 있다).

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 11파일 — `/module-review`로 갱신*
