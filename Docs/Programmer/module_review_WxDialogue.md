# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈로 책임 분리(호스트 액터 / 대화 정의 / PC 세션)가 명확하고 헤더 주석이 결정의 근거까지 남겨 놓아 전반적으로 건강하다. `CLAUDE.md` 위반은 하나도 없다 — 저작권 첫 줄 11파일 전부, `Wx` prefix 일관, `BlueprintCallable`·`FORCEINLINE` 없음, 델리게이트 콜백은 `HandlePoseLoaded`, 유일한 람다와 유일한 헤더 인라인 정의는 각각 예외 사유 주석을 달고 있고, 플러그인 의존은 `WxCore` 뿐이다. 다만 세션의 수명·종료 처리에 구멍이 남아 있다. 이번 리뷰는 `Build.cs`·`uplugin`·전 헤더를 읽고 세션 컴포넌트와 StateTree 태스크 cpp 를 정독했으며, 소비자(`WxViewModel_Dialogue`, `WxUIManagerSubsystem`, `WxAbility_Interact`, `WxRespawnLibrary`)는 계약 확인 목적으로만 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 세션을 닫는 길이 사용자 입력뿐 — 대화 중 폰이 사라지면 세션이 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:202` (`EndDialogue` 정의), 도달 경로는 `:65`·`:72`·`:81`(모두 `Advance` 안)과 `:126`(다음 대화 시작)뿐
- **범주**: 버그/정확성
- **문제**: 컴포넌트에 `EndPlay`·`UninitializeComponent` 오버라이드도, 컨트롤러의 폰 교체를 듣는 지점도 없다(`Public/WxDialogueSessionComponent.h:113-173` 에 그런 멤버가 없음). 그래서 세션을 끝낼 수 있는 주체는 뷰가 부르는 `Advance()`(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:48`)와 "다음 대화의 시작" 둘뿐이다. 그런데 대화 도중 사망·리스폰이 일어나면 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:47`, `:65`)이 폰을 언포제스하고 파괴하고, 이를 본 `UWxUIManagerSubsystem::WatchPawnTags`(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:262`)가 대화 창을 닫아 버려 `Advance()` 를 부를 주체가 사라진다. 결과로 세션은 `CurrentRowName` 을 든 채 영구히 "진행 중"으로 남는다. 파급이 셋이다.
  - ① `Play Dialogue` 태스크가 `OnDialogueEnded` 를 영영 못 받아 `Running` 으로 멈춘다(`Private/WxStateTreeTask_PlayDialogue.cpp:50-55`) — 그 대화를 기다리던 퀘스트 단계가 복구 불가능하게 정지한다.
  - ② `EndDialogueCamera()` 가 돌지 않아 스폰된 대화 카메라 액터(`:253`)가 `SetLifeSpan`(`:286`)을 못 받고 PC 수명까지 남는다.
  - ③ 파괴된 폰 ASC 에 올린 `State.Dialogue`(`:154`)를 되돌릴 주체도 사라진다 — 폰과 함께 없어져 실피해는 작지만, `TaggedAbilitySystem` 을 "도중 폰 교체 대비"로 든다는 헤더 주석(`Public/WxDialogueSessionComponent.h:149`)의 전제가 실제로는 서 있지 않다는 신호다.
- **제안**: `UninitializeComponent()`(또는 `EndPlay`)에서 활성 세션을 접고, 컨트롤러의 `OnPossessedPawnChanged` 를 구독해 폰이 바뀌면 `EndDialogue()` 를 태운다. 카메라 액터만이라도 스폰 시 상한 수명을 주면 ②는 즉시 막힌다.
- **확신도**: 중간 (경로는 코드로 확인했고, 체감 심각도는 대화 중 사망 빈도에 달렸다)

### 2. 🟡 중단된 앞 대화가 `Succeeded` 로 보고된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:124-127`, `:218` → `Private/WxStateTreeTask_PlayDialogue.cpp:50-53`
- **범주**: 설계/구조
- **문제**: `ClientStartDialogue` 는 세션이 겹치면 앞 세션을 `EndDialogue()` 로 접는데, 그 안의 `OnDialogueEnded.Broadcast()` 는 정상 완주와 중단을 구분하지 않는다. 신호에 사유가 실리지 않으므로 `Play Dialogue` 태스크의 리스너는 무조건 `FinishTask(EStateTreeFinishTaskType::Succeeded)` 를 낸다. 즉 대화 A 를 재생하던 퀘스트 단계가, 플레이어가 A 를 한 줄도 더 읽지 않고 대화 B 에 끊겨도 성공으로 넘어간다. 이 겹침 경로는 가상의 상황이 아니라 코드 주석이 직접 실재한다고 못박은 것이다(`:122` — 퀘스트 트리의 Play Dialogue 는 `State.Dialogue` 차단 태그 게이트를 거치지 않는다). 태스크 헤더는 반대 방향("상태를 먼저 떠나도 대화를 끊지 않는다", `Public/WxStateTreeTask_PlayDialogue.h:30`)만 다루고 이 방향은 다루지 않는다.
- **제안**: `OnDialogueEnded` 에 완주 여부(bool 또는 열거)를 실어 태스크가 중단 시 `Failed` 를 내게 하거나, 최소한 헤더에 "중단도 성공으로 본다"를 명시해 퀘스트 저작 쪽이 알고 쓰게 한다.
- **확신도**: 중간 (성공 처리가 의도된 관용일 수 있다)

### 3. 🟡 대상 없는 대사도 포즈를 끝까지 스트리밍한 뒤에야 실패를 안다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:289-319`(요청), `:339-348`(판정)
- **범주**: 성능/안전
- **문제**: 포즈를 얹을 수 있는지는 대상이 `AWxDialogueActor` 이고 그 `GetPoseMesh()` 에 애님 인스턴스가 있느냐로 갈리는데(`:339-341`), 이 판정을 비동기 로드 완료 후에 한다. `Play Dialogue` 태스크는 대상 없이 들어오므로(`Private/WxStateTreeTask_PlayDialogue.cpp:39` 의 `Target=nullptr`), 나레이션 테이블의 행이 `TargetPose` 를 채우고 있으면 대사마다 몽타주를 통째로 스트리밍했다가 경고만 찍고 버린다. 판정 재료(`PendingPoseTarget`)는 요청 시점에 이미 전부 손에 있다.
- **제안**: `ApplyCurrentPose()` 진입부에서 대상이 포즈를 받을 수 있는지 먼저 가르고, 아니면 `RequestAsyncLoad` 를 걸지 않는다.
- **확신도**: 높음

### 4. 🟡 호출자 없는 공개 접근자 두 개 — README 가 서술한 관찰 창구가 실제로는 비어 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`(`GetCurrentDialogueTarget`), `:69`(`GetCurrentRowHandle`) / 정의는 `Private/WxDialogueSessionComponent.cpp:94-106`
- **범주**: 중복/복잡도
- **문제**: 두 함수 모두 저장소 전체(`Source`, `Plugins`)에 호출자가 없다. `UFUNCTION` 도 아니라 BP 호출자도 있을 수 없다. 실사용 소비자는 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp` 하나뿐이고 `GetCurrentSpeaker`/`GetCurrentLine`/`OnLineChanged`/`Advance` 만 쓴다. 반면 `Plugins/WxDialogue/README.md:15` 와 헤더 주석(`:27-28`)은 "진행 중 행을 관찰하는 소비자(WxQuest 등)가 의미를 판정한다"를 현재 계약처럼 서술한다. 특히 `GetCurrentRowHandle()` 은 "대화 중이 아니면 미지정 인자와 구분되지 않으니 `HasActiveDialogue` 로 가리라"는 사용 규약까지 주석으로 지고 있는데(`Public/WxDialogueSessionComponent.h:66-68`), 그 규약을 지킬 호출자가 아직 없어 검증된 적도 없다.
- **제안**: 관찰 소비자가 실제로 생길 때 되살리고 지금은 걷어내거나, 남긴다면 README·헤더에 "현재 미구현 확장 지점"임을 명시해 문서와 코드의 어긋남을 없앤다.
- **확신도**: 높음

### 5. 🟢 `PlayerCameraManager` 를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:241`
- **범주**: 성능/안전
- **문제**: 같은 함수가 폰·대상은 꼼꼼히 가리는데(`:226`) 카메라 매니저만 무검사다. 로컬 PC 라면 대개 유효하지만 스폰 직후·심리스 트래블처럼 아직 없을 수 있는 창이 존재하고, 이 값은 카메라가 어느 쪽으로 비껴설지(`Side`)를 정하는 데만 쓰여 없을 때 한쪽으로 기본값을 잡아도 무해하다.
- **제안**: null 이면 `Side` 를 임의의 한쪽으로 두고 진행한다.
- **확신도**: 중간

### 6. 🟢 `OnDialogueEnded` 는 Broadcast 도중 붙은 바인딩까지 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:218-219`
- **범주**: 버그/정확성
- **문제**: `Broadcast()` 직후 `Clear()` 라, 종료 통보를 받은 리스너가 그 자리에서 새 대화를 열고 새 약속을 붙이면 그 약속까지 함께 지워진다 — 그 대화는 종료 통보를 영영 못 받는다. 지금은 유일한 구독자인 `Play Dialogue` 태스크가 `FStateTreeWeakExecutionContext::FinishTask` 로 완료를 뒤로 미루므로 이 재진입이 실현되지 않는다.
- **제안**: 당장 고칠 필요는 없다. 동기 구독자가 붙는 순간 깨지는 구조이므로, `Broadcast` 전에 델리게이트를 지역으로 옮겨 멤버를 먼저 비우는 관용구가 안전하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 쓰지 않는 모듈 의존성
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: `UniversalObjectLocator` 를 `PublicDependencyModuleNames` 에 넣었지만 모듈 소스 어디에서도 UOL 타입을 쓰지 않는다(전 소스 검색 0건). 나머지 의존은 모두 실사용된다(`ModularGameplay` = `UControllerComponent`, `GameplayAbilities` = ASC 루즈 태그, `StateTreeModule` = 태스크).
- **제안**: 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp` — 그리고 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **미검토 / 한계**: 복제 경로는 코드 독해로만 판단했고 데디케이티드 서버/원격 클라 환경에서 `FDataTableRowHandle` RPC 인자 해소, 배치 NPC(`AWxNpc`, 비복제)의 `Target` 참조 해소, 동적 주입 컴포넌트의 Client RPC 라우팅을 실측하지 않았다 — 모듈 스스로 v1 싱글/리슨 호스트 전제를 문서화하고 있어 그 전제 안에서만 검증했다. 그 전제를 벗어나면 `Private/WxStateTreeTask_PlayDialogue.cpp:42` 의 동기 `HasActiveDialogue()` 판정이 항상 실패로 떨어진다는 점만 지적해 둔다(발견으로 세지 않음 — 의도된 v1 한계). 카메라 구도 수식(`BeginDialogueCamera`)은 좌우 판정까지 손으로 따라갔으나 실제 화면 결과는 확인하지 않았다. 몽타주 포즈의 타 클라이언트 복제, 대화 테이블 에셋의 데이터 정합(순환 `NextRow` 등), BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 11파일 — `/module-review`로 갱신*
