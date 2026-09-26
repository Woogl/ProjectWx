# Dialogue VM을 순수 표시 데이터로 분리

상태: 완료 · 후속 리졸버 전환·사용자 인게임 확인
다음 행동: 대화 수명이나 연결 구조를 바꿀 때 참고한다.

## 사용자 요구와 경계

- Dialogue VM 자체를 순수 표시 데이터로 만든다. 자식 VM을 추가하는 방식은 아니다.
- Wx 기능 모듈 간 신규 의존성은 WxCore를 제외하면 금지한다. WxCore에는 공용 정의를 넘어선 게임 로직이나 이관 목적의 과도한 추상화를 추가하지 않는다.
- 기존 WxGame 조립 계층이 WxUI와 WxDialogue를 연결한다.

## 1차 구현: 순수 표시 VM 분리

- WxUI의 `UWxViewModel_Dialogue`: Speaker, LineText, HasSpeaker와 SetLine 변경 알림만 남긴다. 세션 참조·진행 요청·구독·해제는 없다.
- WxGame의 기존 `UWxViewModelResolver_Dialogue`: 세션 OnLineChanged를 VM의 SetLine에 직접 연결하고 현재 대사로 초기화한다. 해제는 실제 구독한 세션(생성 VM의 Outer)에서 수행한다. Resolver 인스턴스에는 뷰별 상태를 저장하지 않는다. 세션이 없으면 빈 VM을 반환한다.
- WxGame의 `UWxDialogueScreen`: 위젯 소유 컨트롤러의 세션으로 RequestAdvance를 전달한다. WBP_DialogueScreen의 부모와 MVVM 진행 이벤트 목적지를 이 클래스로 전환한다.
- 새 연결 객체나 자식 VM은 없다. Build.cs, WxCore, WxDialogue의 게임 로직은 변경하지 않는다.
- 이동한 VM 클래스만 CoreRedirect로 WxGame → WxUI 경로를 유지한다. Resolver는 WxGame 경로 그대로다.
  - 2026-09-24 제거: 사용자 지시("오래된 리디렉터 제거 진행합시다")로 `DefaultEngine.ini`의 `[CoreRedirects]`를 모두 비웠다. 리다이렉트가 없어도 참조 WBP 6개(`WBP_Ability`·`WBP_PlayerSkills`·`WBP_ItemQuickSlot`·`WBP_QuestTracker`·`WBP_QuestObjective`·`WBP_DialogueScreen`)가 누락 클래스 경고 없이 로드되고 컴파일 오류·경고가 0이었다. 인게임 표시는 확인하지 않았다. 근거: [Nameplate 작업 자료](nameplate-manager.md)의 "오래된 CoreRedirects 제거" 절.
- 에디터 도구에 MVVM 이벤트 목적지를 위젯 함수로 변경하는 기능을 추가한다. 기존 MCP/Python 프로퍼티 쓰기로는 래퍼 그래프를 올바르게 갱신할 수 없어 MVVMEditorSubsystem을 사용한다.

## 1차 검증

- 실제 WBP EventGraph는 기본 위젯 이벤트만 있고, 진행 요청은 MVVM 이벤트에 저장되어 있음을 확인했다.
- WxEditor Win64 Development 빌드 성공. 로그: `Saved/Logs/BuildDoctor/build_2026-09-23_115815_979_4792.log`.
- WBP_DialogueScreen의 실제 이벤트는 `AdvanceButton.OnClicked` 하나다. 부모를 WxDialogueScreen으로 바꾸고 목적지를 `Self.RequestAdvance`로 바꿔 저장했다. 화자·대사·HasSpeaker 바인딩과 Resolver 클래스 경로는 유지했다.
- 이관 스크립트 자체와 최종 컴파일은 성공했으나 첫 저장 프로세스는 기존 SourceControl의 누락 디렉터리 경고를 Error로 기록해 종료 코드 1을 반환했다. 이관 전 구형 이벤트 로드 경고도 해당 로그에 남아 있다. 최종 검증에서는 SourceControl을 끄고 새 프로세스로 다시 로드·컴파일했으며 종료 코드 0, BP 오류·경고 없음이다. 로그: `Saved/Logs/VerifyDialogue.log`.
- 실제 OnLineChanged 델리게이트로 두 VM에 같은 대사를 전달하고, 하나의 구독만 해제한 뒤 나머지만 갱신되는 것을 확인했다. 화자 없음과 종료 시 빈 대사도 확인했다. 이는 신호·표시 데이터 검증이며 Resolver 생성·해제 전체나 PIE 검증을 대신하지 않는다.
- 인게임 화면 표시·클릭 진행·종료·재진입·빙의 변경은 아직 실행 확인하지 않았다.

## 최종 구조: Resolver 제거 · 사용자 승인

- 사용자 요청: Resolver를 제거하고 기존 DialogueScreen에 연결 책임을 모아 단순화한다.
- WBP의 VM 생성 방식을 Create Instance로 바꾸고 Resolver 객체 참조를 제거했다. VM 이름·바인딩 ID·표시 바인딩과 Self.RequestAdvance 입력 연결은 유지한다.
- WxGame의 Dialogue Resolver 소스 두 파일을 제거했다. WxUI의 순수 표시 VM은 그대로다.
- DialogueScreen의 활성화에서 세션 구독·현재 대사 동기화, 비활성화·파괴에서 구독 해제를 수행한다. 구독한 세션과 VM은 화면의 약한 참조로 추적한다. VM의 Outer에는 더 이상 세션을 넣지 않는다.
- 엔진 근거: UUserWidget::NativeConstruct는 MVVM 확장 Construct를 실행하고, CommonActivatableWidget은 Super::NativeConstruct 이후 자동 활성화한다. 조기 활성화는 NativeConstruct 완료 후 다시 연결한다. 비활성화 뒤 다시 활성화하면 같은 VM에 세션의 현재 대사를 다시 채운다.
- 진행 입력은 활성 화면이 관찰 중인 세션에만 전달한다.
- 수명 회귀 테스트: 실제 WBP에서 조기/일반 활성화, 두 화면 독립 구독, 비활성 해제, 재활성 시 현재 값 복구, 위젯 재생성 시 새 VM 연결과 구형 VM 해제를 검사한다. 실행 결과는 아래에 기록한다.
- 에셋 이관 스크립트는 성공했으나 첫 프로세스는 다른 서버가 쓰는 MCP 8000 포트 충돌로 종료 코드 1을 반환했다. 후속 검증은 별도 포트를 사용한다.

### 최종 검증

- WxEditor Win64 Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-23_122525_055_9048.log` (종료 코드 0).
- Resolver 없는 빌드에서 저장한 WBP 재로드·컴파일 성공: `Saved/Logs/VerifyDialogueCreateInstance.log` (종료 코드 0). Create Instance·Resolver null·기존 VM 이름·새 화면 부모를 검사했다. Blueprint 경고를 실패로 처리한 컴파일을 통과했다.
- `Wx.UI.Dialogue.ScreenLifecycle` 자동화 성공: `Saved/Logs/DialogueLifecycle-final.log`, `Saved/Automation/DialogueLifecycle/index.json` (종료 코드 0). 실제 WBP 인스턴스로 생성 전/후 활성화, 독립 VM, 세션 신호 전달, 비활성 해제, 재활성 초기화, 재생성 시 새 VM 연결·구형 VM 구독 해제, 화자·빈 대사를 검증했다.
- 테스트 준비의 월드 중복 초기화와 BeginPlay를 생략한 월드의 컨트롤러 목록 누락을 수정한 뒤 통과했다. 런타임 구현은 이 테스트 환경 수정 과정에서 변경하지 않았다.
- Content/UI에서 제거한 Resolver 이름의 바이너리 문자열 참조 0건. 문서 링크 검사 오류 0건.
- 실제 게임에서 화면 외관·버튼 클릭에 따른 세션 진행·빙의 변경은 아직 인간 확인 대상이다. 자동화의 대사 신호는 테스트가 직접 발행하며 실제 대화 테이블 진행 검증은 아니다.

- 2026-09-23 사용자 요청으로 검증 완료한 WxDialogueScreenTest.cpp와 화면의 테스트 전용 friend 선언을 제거했다. 위 테스트 결과는 제거 전 검증 기록이다.

## 화면 클래스 제거 · 리졸버 복귀 · 2026-09-23

- 사용자 요청: `UWxDialogueScreen`을 제거하고 WBP가 뷰모델로 구동되게 한다. 이유: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없다."
- 구조: 보스 바와 같은 세 층(모델 WxDialogue 세션 / 연결 WxGame 리졸버 / 표시 WxUI VM).
- WxUI `UWxViewModel_Dialogue`: 진행 명령 `RequestAdvance()`(BlueprintCallable)와 네이티브 단일 델리게이트 `OnAdvanceRequested`를 추가했다. VM은 세션을 모른다.
- WxGame `UWxViewModelResolver_Dialogue` 신규:
  - CreateInstance: 위젯을 Outer로 VM을 만들고, 소유 PC의 세션 `OnLineChanged`에 VM의 `SetLine`을 건 뒤 현재 대사를 채운다. `OnAdvanceRequested`를 세션 `Advance`에 약한 바인딩(BindUObject)으로 잇는다. 세션이 없으면 빈 VM이다.
  - DestroyInstance: 그 VM의 구독만 `RemoveAll(VM)`로 끊는다. 리졸버는 상태가 없다.
- `WBP_DialogueScreen`: 부모 `WxDialogueScreen` → `WxActivatableWidget`, VM 생성 방식 Create Instance → Resolver, 진행 이벤트 `AdvanceButton.OnClicked`의 목적지 `Self.RequestAdvance` → `WxViewModel_Dialogue.RequestAdvance`.
- `WxDialogueScreen.h/.cpp` 삭제. `WxPlayerLayoutComponent.cpp`의 주석 한 줄("활성화될 때" → "생성될 때")을 정정했다.
- 수명 근거(엔진): CommonUI 스택은 창을 닫을 때 `GeneratedWidgetsPool.Release(Widget, true)`로 Slate까지 해제한다(`CommonActivatableWidgetContainer.cpp:244`). 따라서 `NativeDestruct` → MVVM `Destruct` → `DestroyInstance`가 호출되고, 다시 띄우면 `Construct` → `CreateInstance`로 새 VM이 현재 대사를 채운다(`MVVMView.cpp:93`, `:146`).
- 동작 차이: 이전 화면 클래스는 활성화 여부로 구독과 진행 입력을 제한했다. 이제는 위젯이 생성되어 있는 동안 구독한다. 비활성 화면은 보이지 않아 클릭될 수 없고, `Advance`는 활성 대화가 없으면 무시한다.
- WxToolset: `SetEventDestinationWidgetFunction`을 `SetEventDestination(WidgetBlueprint, EventIndex, "Self.함수" | "뷰모델이름.함수")`로 일반화했다. 경로는 기존 `ResolvePropertyPath`로 해석하고, 끝이 BlueprintCallable 함수인지 검사한다.

### 검증

- WxEditor Win64 Development 빌드 성공(클래스 삭제 후 최종): `Saved/Logs/BuildDoctor/build_2026-09-23_212120_128_3604.log`, 종료 코드 0.
- 이관: `Saved/Logs/ScreensToResolver.log` (종료 코드 0). 두 WBP 모두 부모 변경·리졸버 전환 후 경고를 오류로 취급한 컴파일을 통과하고 저장했다. `WBP_GameLayout`·`WBP_QuestObjective`도 컴파일을 통과했다.
- 클래스 삭제 후 새 프로세스 재검증: `Saved/Logs/ScreensResolverVerify.log` (종료 코드 0). 부모·리졸버 클래스·VM 클래스·이벤트 목적지를 확인했고, 4개 WBP가 경고를 오류로 취급한 컴파일을 통과했다. 삭제한 두 클래스는 로드되지 않았다.
- Content에서 삭제한 클래스 이름의 바이너리 참조 0건.
- 미실행(인간 확인 필요): PIE에서 대화 표시·클릭 진행·종료 후 재대화, 퀘스트 수주·목표 갱신·완료 시 추적기 표시. 런타임 구독·해제는 코드와 엔진 수명(아래)으로만 확인했다.
- 인게임(사용자 확인, 2026-09-23): 통과. "테스트 문제 없습니다." 인간 코드 리뷰의 별도 승인 기록은 없다.
- 사용자 요청으로 Wiki에 반영하고 푸시했다(2026-09-23).
