# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈로 책임 분리(호스트 액터 / 대화 정의 / PC 세션)가 선명하고 헤더 주석이 결정의 근거까지 남겨 두어 전반적으로 건강하다. `CLAUDE.md` 위반은 0건이다 — 저작권 첫 줄 11파일 전부 통과, `Wx` prefix 일관, `BlueprintCallable`·`FORCEINLINE` 0건, 헤더 본문은 `GetInstanceDataType()` 하나뿐이고 규칙 6 의 명시 예외로 사유 주석이 붙어 있으며, 유일한 람다(`Private/WxStateTreeTask_PlayDialogue.cpp:50`)에도 예외 사유가 달렸고, 델리게이트 콜백 `HandlePoseLoaded` 는 `Handle` prefix 를 지킨다. 플러그인 의존은 `WxCore` 뿐이다. 남은 문제는 전부 세션의 수명·종료 경로에 몰려 있고, 직전 리뷰(`1d91a915`) 이후 모듈 소스는 바뀌지 않아 7건이 그대로 서 있다. 이번 리뷰는 모듈 11파일을 전부 읽고 세션 컴포넌트 cpp 와 StateTree 태스크를 정독했으며, 소비자(`WxViewModel_Dialogue`·`WxUIManagerSubsystem`·`WxAbility_Interact`·`WxRespawnLibrary`·`AWxNpc`·`IWxInteractable`)는 계약 검증 목적으로만 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 세션을 닫는 길이 사용자 입력뿐 — 대화 중 폰이 사라지면 세션이 굳고 퀘스트가 정지한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:201`(`EndDialogue` 정의). 도달 경로는 `:64`·`:71`·`:80`(모두 `Advance` 안)과 `:125`(다음 대화의 시작)뿐이다
- **범주**: 버그/정확성
- **문제**: 컴포넌트에 `EndPlay`·`UninitializeComponent` 오버라이드도, 컨트롤러의 폰 교체를 듣는 지점도 없다(`Public/WxDialogueSessionComponent.h:113-173` private 섹션 전체에 그런 멤버가 없다). 세션을 끝낼 수 있는 주체는 뷰가 부르는 `Advance()`(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:51`)와 "다음 대화의 시작" 둘뿐이다. 그런데 대화 도중 사망·리스폰이 나면 `UWxRespawnLibrary::RequestRespawn` 이 폰을 언포제스하고(`Source/WxGame/Framework/WxRespawnLibrary.cpp:47`) 파괴하며(`:65`), 그 폰 교체를 받은 `UWxUIManagerSubsystem::HandlePossessedPawnChanged`(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:235`)가 `WatchPawnTags` 초입에서 대화 창을 닫아(`:252`) `Advance()` 를 부를 주체 자체가 사라진다. 대화 중 무적·정지 처리는 어디에도 없어(`State.Dialogue` 를 보는 곳은 상호작용 차단과 UI 뿐) 실제로 일어날 수 있는 상황이다. 세션은 `CurrentRowName` 을 든 채 영구히 "진행 중"으로 남고, 파급이 셋이다.
  - ① `Play Dialogue` 태스크가 `OnDialogueEnded` 를 영영 못 받아 `Running` 으로 멈춘다(`Private/WxStateTreeTask_PlayDialogue.cpp:50-53`). 그 대화를 기다리던 퀘스트 단계가 정지한다. 다른 대화를 열면 `:125` 가 앞 세션을 접어 풀리긴 하지만, 그 다른 대화의 진입점인 `AWxNpc::CanInteract` 는 대기 중인 퀘스트 스텝이 있어야 참을 답하므로(`Source/WxGame/Character/WxNpc.cpp:41-44`) 정지한 퀘스트 스스로는 탈출구를 내주지 못한다. 이것이 실질 피해의 전부다.
  - ② `EndDialogueCamera()` 가 돌지 않아 스폰한 대화 카메라 액터(`:252`)가 `SetLifeSpan`(`:285`)을 못 받고 월드에 남는다. 화면 자체는 `RequestRespawn` 의 `SetViewTarget`(`WxRespawnLibrary.cpp:72`)이 되돌리므로 액터 누수만 남는다.
  - ③ 파괴된 폰 ASC 에 올린 `State.Dialogue`(`:153`)를 되돌릴 주체도 사라진다 — 폰과 함께 없어져 실피해는 없지만, `TaggedAbilitySystem` 을 "도중 폰 교체 대비"로 든다는 헤더 주석(`Public/WxDialogueSessionComponent.h:149`)의 전제가 실제로는 서 있지 않다는 신호다.
- **제안**: `UninitializeComponent()`(또는 `EndPlay`)에서 활성 세션을 접고, 컨트롤러의 `OnPossessedPawnChanged` 를 구독해 폰이 바뀌면 `EndDialogue()` 를 태운다. 카메라 액터에 스폰 시점부터 상한 수명을 주면 ②는 그것만으로 막힌다.
- **확신도**: 중간 (경로는 전부 코드로 확인했고, 체감 심각도는 "대화 중 사망"의 실제 빈도에 달렸다)

### 2. 🟡 중단된 대화와 데이터 오류로 끊긴 대화가 모두 `Succeeded` 로 보고된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:217`(사유 없는 단일 신호) ← 호출부 `:64`·`:80`·`:125` → `Private/WxStateTreeTask_PlayDialogue.cpp:50-53`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 의 `OnDialogueEnded.Broadcast()` 는 정상 완주와 비정상 종료를 구분하지 않고 신호에 사유도 싣지 않으므로, `Play Dialogue` 태스크의 리스너는 무조건 `FinishTask(EStateTreeFinishTaskType::Succeeded)` 를 낸다. 그런데 `EndDialogue()` 로 가는 길 넷 중 셋이 완주가 아니다.
  - `:125` — 세션이 겹치면 앞 세션을 접는다. 즉 대화 A 를 재생하던 퀘스트 단계가, 플레이어가 A 를 한 줄도 더 읽지 않고 대화 B 에 끊겨도 성공으로 넘어간다. 이 겹침 경로는 가상이 아니라 코드 주석이 실재한다고 못박은 것이다(`:121-122` — 퀘스트 트리의 Play Dialogue 는 `State.Dialogue` 차단 태그 게이트를 거치지 않는다).
  - `:80` — `NextRow` 가 가리키는 행을 찾지 못했거나 그 행의 대사가 비어 있는 경우다. 바로 위에서 "정상 종료가 아니다"라고 경고까지 찍고(`:77-79`) 성공으로 보고한다. 대화 테이블의 `NextRow` 오타 하나가 퀘스트를 그대로 통과시킨다.
  - `:64` — 세션 도중 테이블이 갈린 경우(에디터 재임포트).
  태스크 헤더는 첫 행 해석 실패만 `Failed` 로 규정하고(`Public/WxStateTreeTask_PlayDialogue.h:29`) 두 번째 행 이후의 같은 실패는 다루지 않아, 같은 오류가 진입 시점이냐 진행 중이냐에 따라 `Failed`/`Succeeded` 로 갈린다.
- **제안**: `OnDialogueEnded` 에 완주 여부(bool 또는 열거)를 실어 태스크가 중단·해석 실패 시 `Failed` 를 내게 한다. 유지한다면 최소한 헤더에 "중단과 진행 중 해석 실패도 성공으로 본다"를 명시해 퀘스트 저작 쪽이 알고 쓰게 한다.
- **확신도**: 높음 (경로는 코드로 확정. `Succeeded` 처리가 의도된 관용일 수는 있으나 `:77-79` 의 경고 문구, `Public/WxStateTreeTask_PlayDialogue.h:29` 의 규정과 어긋난다)

### 3. 🟡 포즈를 받을 수 없는 대상에도 몽타주를 끝까지 스트리밍한 뒤에야 실패를 안다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:288-318`(요청), `:338-347`(판정)
- **범주**: 성능/안전
- **문제**: 포즈를 얹을 수 있는지는 대상이 `AWxDialogueActor` 이고 그 `GetPoseMesh()` 에 애님 인스턴스가 있느냐로 갈리는데(`:338-340`), 이 판정을 비동기 로드가 끝난 뒤에 한다. `Play Dialogue` 태스크는 대상 없이 들어오므로(`Private/WxStateTreeTask_PlayDialogue.cpp:39` 의 `nullptr`), 나레이션 테이블의 행이 `TargetPose` 를 채우고 있으면 대사마다 몽타주를 통째로 스트리밍했다가 경고만 찍고 버린다. `GetPoseMesh()` 가 기본값 `nullptr` 인 파생(`Public/WxDialogueActor.h:33`, `Private/WxDialogueActor.cpp:22-25` — 스켈레탈 없는 "말 거는 물체")도 같은 길로 떨어진다. 판정 재료(`PendingPoseTarget`)는 요청 시점에 이미 전부 손에 있다.
- **제안**: `ApplyCurrentPose()` 진입부에서 대상이 포즈를 받을 수 있는지 먼저 가르고, 아니면 `RequestAsyncLoad` 를 걸지 않는다.
- **확신도**: 높음

### 4. 🟡 호출자 없는 공개 접근자 두 개 — README 가 서술한 관찰 창구가 실제로는 비어 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`(`GetCurrentDialogueTarget`), `:69`(`GetCurrentRowHandle`) / 정의는 `Private/WxDialogueSessionComponent.cpp:93-105`
- **범주**: 중복/복잡도
- **문제**: 두 함수 모두 저장소 전체(`Source`, `Plugins`)에 호출자가 없다 — 모듈 안에서도 정의·주석 외 참조가 없고, `UFUNCTION` 이 아니라 BP 호출자가 있을 수도 없다. 실사용 소비자는 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp` 하나뿐이고 `GetCurrentSpeaker`/`GetCurrentLine`/`OnLineChanged`/`Advance` 만 쓴다(`WxQuest` 는 `WxDialogue` 를 아예 참조하지 않는다). 반면 `Plugins/WxDialogue/README.md` 의 책임·경계 절과 헤더 주석(`Public/WxDialogueSessionComponent.h:27`)은 "진행 중 행을 관찰하는 소비자(WxQuest 등)가 의미를 판정한다"를 현재 계약처럼 서술한다. 특히 `GetCurrentRowHandle()` 은 "대화 중이 아니면 미지정 인자와 구분되지 않으니 `HasActiveDialogue` 로 가리라"는 사용 규약까지 주석으로 지고 있는데(`:65-68`), 그 규약을 지킬 호출자가 없어 검증된 적도 없다. 이 미사용 API 를 지키느라 `ClientStartDialogue` 실패 경로에 `CurrentStartRow` 되돌리기(`Private/WxDialogueSessionComponent.cpp:141-143`)까지 붙어 있고, 그중 `CurrentRowName = NAME_None`(`:143`)은 `EnterRow` 가 실패 시 이름을 건드리지 않으므로 어느 진입 경로에서도 항상 이미 비어 있는 죽은 대입이다.
- **제안**: 관찰 소비자가 실제로 생길 때 되살리고 지금은 걷어내거나, 남긴다면 README·헤더에 "현재 미구현 확장 지점"임을 명시해 문서와 코드의 어긋남을 없앤다.
- **확신도**: 높음

### 5. 🟢 `PlayerCameraManager` 를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:240`
- **범주**: 성능/안전
- **문제**: 같은 함수가 폰·대상은 꼼꼼히 가리는데(`:225`) 카메라 매니저만 무검사다. 로컬 PC 라면 대개 유효하지만 스폰 직후·심리스 트래블처럼 아직 없을 수 있는 창이 존재하고, 이 값은 카메라가 어느 쪽으로 비껴설지(`Side`, `:241`)를 정하는 데만 쓰여 없을 때 한쪽으로 기본값을 잡아도 무해하다.
- **제안**: null 이면 `Side` 를 임의의 한쪽으로 두고 진행한다.
- **확신도**: 중간

### 6. 🟢 `OnDialogueEnded` 는 Broadcast 도중 붙은 바인딩까지 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:217-218`
- **범주**: 버그/정확성
- **문제**: `Broadcast()` 직후 `Clear()` 라, 종료 통보를 받은 리스너가 그 자리에서 새 대화를 열고 새 약속을 붙이면 그 약속까지 함께 지워진다 — 그 대화는 종료 통보를 영영 못 받는다. 지금은 유일한 구독자인 `Play Dialogue` 태스크가 완료를 뒤로 미루므로(`Private/WxStateTreeTask_PlayDialogue.cpp:50-53` 의 `MakeWeakExecutionContext().FinishTask` 는 완료 상태만 세우고 전이는 예약된 다음 Tick 에 돈다) 이 재진입이 실현되지 않는다.
- **제안**: 당장 고칠 필요는 없다. 동기 구독자가 붙는 순간 깨지는 구조이므로, `Broadcast` 전에 델리게이트를 지역으로 옮겨 멤버를 먼저 비우는 관용구가 안전하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 쓰지 않는 모듈 의존성
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:19`
- **범주**: 중복/복잡도
- **문제**: `UniversalObjectLocator` 를 `PublicDependencyModuleNames` 에 넣었지만 모듈 소스 어디에서도 UOL 타입을 쓰지 않는다(모듈 전 소스 검색 히트 0건 — Build.cs 자신의 한 줄이 유일하다). 나머지 의존은 모두 실사용된다(`GameplayAbilities`·`GameplayTags` = 폰 ASC 루즈 태그, `StateTreeModule` = 태스크, `WxCore` = `IWxInteractable`·`WxGameplayTags`).
- **제안**: 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp` — 그리고 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **미검토 / 한계**: 복제 경로는 코드 독해로만 판단했고, 데디케이티드 서버/원격 클라 환경에서 `FDataTableRowHandle` RPC 인자 해소, 배치 NPC(`AWxNpc`, 비복제)의 `Target` 참조 해소, `SetLooseGameplayTagCount` 가 클라 로컬에만 서는 탓에 서버측 `UWxAbility_Interact::ActivationBlockedTags`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:36`) 게이트가 비게 되는 문제, 포즈 몽타주가 소유 클라에서만 재생돼 다른 클라에 안 보이는 문제를 실측하지 않았다 — 모듈 스스로 v1 싱글/리슨 호스트 전제를 문서화하고 있어(`Public/WxDialogueSessionComponent.h:28`) 그 전제 안에서만 검증했다. 그 전제를 벗어나면 `Private/WxStateTreeTask_PlayDialogue.cpp:42` 의 동기 `HasActiveDialogue()` 판정도 항상 실패로 떨어진다(발견으로 세지 않음 — 명시된 v1 한계). 카메라 구도 수식(`BeginDialogueCamera`)은 좌우 판정까지 손으로 따라갔으나 실제 화면 결과는 확인하지 않았다. `FStreamableManager::RequestAsyncLoad` 가 동기 완료해 `PoseLoadHandle` 대입 순서가 뒤집히는 경계(`:315`)는 `PendingPose.Get()` 선검사(`:309`)에 가려 실현되기 어렵고 결과도 핸들 1개 잔존뿐이라 발견으로 세지 않았고, `FindCurrentRow()` 가 한 대사에 세 번 도는 것(`:198` 에서 2회 + `:292`)과 `ANSI_TO_TCHAR(__FUNCTION__)` 가 매 호출 임시를 만드는 것도 대사 단위 빈도라 세지 않았다. 대화 테이블 에셋의 데이터 정합(순환 `NextRow` 등)과 BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 11파일 — `/module-review`로 갱신*
