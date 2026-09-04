# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈이고 책임 분리(호스트 액터 / 대화 정의 / PC 세션)와 헤더 주석이 잘 잡혀 있어 전반적으로 건강하다. 다만 세션의 수명 관리에 구멍이 하나 있다. 이번 리뷰는 `Build.cs`·전 헤더와 세션 컴포넌트·StateTree 태스크 cpp 를 정독했고, 소비자 쪽(`WxViewModel_Dialogue`, `WxUIManagerSubsystem`)은 계약 확인 목적으로만 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 세션을 끝낼 길이 사용자 입력뿐 — 폰이 사라지면 대화가 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:202` (정의), 호출처는 `:65`, `:72`, `:81`(Advance) 과 `:126`(ClientStartDialogue) 넷뿐
- **범주**: 버그/정확성
- **문제**: `EndDialogue()` 에 닿는 경로는 (a) 뷰가 부르는 `Advance()` 와 (b) 다음 대화의 시작뿐이다. 컴포넌트에는 `EndPlay`·`UninitializeComponent` 오버라이드도, 컨트롤러의 폰 교체를 듣는 지점도 없다(`WxDialogueSessionComponent.h:113-173`). 그런데 대화 중 사망 같은 폰 교체가 일어나면 `WxUIManagerSubsystem::WatchPawnTags`(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:262`)가 대화 창을 닫아 버리고, 창과 함께 뷰모델이 사라지면 `Advance()` 를 부를 주체가 없어진다. 결과로 세션은 `CurrentRowName` 을 든 채 영구히 "진행 중"으로 남고, 다음 대화를 열 때까지 아무도 정리하지 않는다. 파급이 셋이다 — ① `Play Dialogue` 태스크가 `OnDialogueEnded` 를 영영 못 받아 `Running` 으로 멈춘다(퀘스트 단계 정지), ② `EndDialogueCamera()` 가 돌지 않아 대화 카메라 액터가 `SetLifeSpan` 도 못 받고 PC 수명까지 남는다(`:286` 은 정상 종료 경로에만 있다), ③ 파괴된 폰 ASC 에 올린 `State.Dialogue` 를 되돌릴 주체도 사라진다(폰과 함께 없어져 실피해는 작다).
- **제안**: `UninitializeComponent()`(또는 `EndPlay`)에서 활성 세션을 접고, 컨트롤러의 `OnPossessedPawnChanged` 를 구독해 폰이 바뀌면 세션을 종료한다. 최소한 카메라 액터만이라도 스폰 시 수명 상한을 갖게 하면 ②는 막힌다.
- **확신도**: 중간 (경로 자체는 코드로 확인했고, 사망 중 대화가 실제로 얼마나 자주 나는지에 따라 체감 심각도는 달라진다)

### 2. 🟡 대상 없는 대사도 포즈를 끝까지 스트리밍한 뒤에야 실패를 안다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:289-319`(요청), `:339-348`(판정)
- **범주**: 성능/안전
- **문제**: 포즈를 얹을 수 있는지는 대상이 `AWxDialogueActor` 이고 `GetPoseMesh()` 에 애님 인스턴스가 있느냐로 갈리는데(`:339-341`), 이 판정을 비동기 로드가 끝난 뒤에 한다. `Play Dialogue` 태스크는 대상 없이 들어오므로(`WxStateTreeTask_PlayDialogue.cpp:39` 이 `Target=nullptr`), 나레이션 테이블의 행이 `TargetPose` 를 채우고 있으면 대사마다 몽타주를 통째로 스트리밍했다가 경고만 찍고 버린다. 판정 재료(`PendingPoseTarget`)는 요청 시점에 이미 다 있다.
- **제안**: `ApplyCurrentPose()` 진입부에서 대상이 포즈를 받을 수 있는 상태인지 먼저 가르고, 아니면 로드를 걸지 않는다.
- **확신도**: 높음

### 3. 🟡 호출자 없는 공개 접근자 두 개
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`(`GetCurrentDialogueTarget`), `:69`(`GetCurrentRowHandle`) / 정의는 `Private/WxDialogueSessionComponent.cpp:94-106`
- **범주**: 중복/복잡도
- **문제**: 둘 다 모듈 안팎 어디에도 호출자가 없다. README 와 헤더 주석은 "관찰자(WxQuest 등)가 대사의 의미를 판정하는 창구"라고 서술하지만, `Plugins/WxQuest` 에는 대화라는 낱말조차 없고 실사용 소비자는 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp` 하나로 `GetCurrentSpeaker`/`GetCurrentLine`/`OnLineChanged`/`Advance` 만 쓴다. `UFUNCTION` 도 아니라 BP 호출자도 있을 수 없다. 특히 `GetCurrentRowHandle()` 은 "대화 중이 아니면 반쪽 핸들과 구분이 안 되니 `HasActiveDialogue` 로 가리라"는 사용 규약까지 주석으로 지고 있는데, 그 규약을 지킬 호출자가 아직 없다.
- **제안**: 관찰 소비자가 실제로 생길 때 되살리고 지금은 걷어내거나, 남긴다면 README 의 "소비자가 관찰로 판정한다"가 현재는 미구현 계획임을 명시한다.
- **확신도**: 높음

### 4. 🟢 `PlayerCameraManager` 를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:241`
- **범주**: 성능/안전
- **문제**: 같은 함수가 폰·대상은 꼼꼼히 가리는데(`:226`) 카메라 매니저만 무검사다. 로컬 PC 라면 대개 유효하지만 스폰 직후·심리스 트래블 구간처럼 아직 없을 수 있는 창이 존재하고, 이 값은 구도의 좌우를 정하는 데만 쓰여 없을 때 기본값으로 떨어져도 무해하다.
- **제안**: null 이면 `Side` 를 임의의 한쪽으로 두고 진행한다.
- **확신도**: 중간

### 5. 🟢 `OnDialogueEnded` 는 Broadcast 도중 붙은 바인딩까지 지운다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:218-219`
- **범주**: 버그/정확성
- **문제**: `Broadcast()` 직후 `Clear()` 라, 종료 통보를 받은 리스너가 그 자리에서 새 대화를 열고 새 약속을 붙이면 그 약속까지 함께 지워진다(그 대화는 종료 통보를 영영 못 받는다). 지금은 유일한 구독자인 `Play Dialogue` 태스크가 `FStateTreeWeakExecutionContext::FinishTask` 로 완료를 다음 틱에 미루므로 이 재진입이 실현되지 않는다.
- **제안**: 지금 고칠 필요는 없다. 다만 동기 구독자가 생기면 깨지는 구조이므로, `Broadcast` 전에 델리게이트를 지역으로 옮겨 비우는 관용구가 안전하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 쓰지 않는 모듈 의존성
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: `UniversalObjectLocator` 를 `PublicDependencyModuleNames` 에 넣었지만 모듈 소스 어디에서도 UOL 타입을 쓰지 않는다.
- **제안**: 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Private/WxDialogueActor.cpp`, `Public/WxDialogueComponent.h`, `Private/WxDialogueComponent.cpp`, `Public/WxDialogueTableRow.h`, `Public/WxDialogueModule.h`, `Private/WxDialogueModule.cpp` — 그리고 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **규칙 점검 결과**: `CLAUDE.md` 위반은 찾지 못했다 — 저작권 첫 줄 11파일 모두 있음, `Wx` prefix 일관, `BlueprintCallable`·`FORCEINLINE` 사용 없음, 델리게이트 콜백은 `HandlePoseLoaded` 로 `Handle` prefix 준수, 유일한 람다(`WxStateTreeTask_PlayDialogue.cpp:50`)와 유일한 헤더 인라인 정의(`WxStateTreeTask_PlayDialogue.h:43`)는 모두 예외 사유 주석을 달고 있다. 플러그인 의존도 `WxCore` 외 `Wx` 플러그인 참조가 없다.
- **미검토 / 한계**: 복제 경로는 코드 독해로만 판단했고 실제 데디케이티드 서버/클라 분리 환경에서 `FDataTableRowHandle` RPC 인자 해소와 동적 주입 컴포넌트의 Client RPC 라우팅을 실측하지 않았다(모듈 자체가 v1 싱글/리슨 호스트 전제라고 문서화하고 있다). 카메라 구도 수식(`BeginDialogueCamera`)은 좌우 판정까지 손으로 따라가 검증했으나 실제 화면 결과는 확인하지 않았다. 대화 테이블 에셋의 데이터 정합(순환 `NextRow` 등)은 범위 밖이다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 11파일 — `/module-review`로 갱신*
