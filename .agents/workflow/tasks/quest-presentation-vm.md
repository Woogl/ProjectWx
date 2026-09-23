# Quest 표시 VM의 WxUI 이전

상태: 완료 · 후속(화면 클래스 제거) 인게임 확인 통과

- 합의: Quest와 QuestObjective VM을 WxUI로 이전하고 순수 표시 데이터로 유지한다. WxGame의 QuestTracker가 저널 구독을 맡는다.
- 조사: WBP_QuestTracker는 UUserWidget 기반. Create Instance로 전환하고 NativeConstruct 이후 연결, NativeDestruct 이전 해제한다. Objective의 기존 행 바인딩은 유지한다.
- 제약: Wx 기능 모듈 간 의존성 및 WxCore 정의 추가 없음.
- 환경: 실행 중인 DebugGame 에디터의 파일 잠금은 사용자가 저장 후 종료하여 해결했다.

## 구현

- Quest·QuestObjective h/cpp를 WxUI Public/Private로 이전하고 WXUI_API 적용.
- Quest SetJournal은 표시 필드와 목표별 VM만 갱신. WxGame QuestTracker가 GameState 조회·구독·초기 저널 동기화 담당.
- Quest Resolver 제거, 기존 두 VM 클래스 경로 리다이렉트 추가. Build.cs와 WxCore 변경 없음.
  - 2026-09-24 제거: 사용자 지시("오래된 리디렉터 제거 진행합시다")로 `DefaultEngine.ini`의 `[CoreRedirects]`를 모두 비웠다. 리다이렉트가 없어도 참조 WBP 6개(`WBP_Ability`·`WBP_PlayerSkills`·`WBP_ItemQuickSlot`·`WBP_QuestTracker`·`WBP_QuestObjective`·`WBP_DialogueScreen`)가 누락 클래스 경고 없이 로드되고 컴파일 오류·경고가 0이었다. 인게임 표시는 확인하지 않았다. 근거: [Nameplate 작업 자료](nameplate-manager.md)의 "오래된 CoreRedirects 제거" 절.
- 에디터 종료 후 기존 Resolver→Create Instance 에셋 전환 저장 성공(QuestCreateInstance3.log).
- Wx.UI.Quest.TrackerLifecycle: 실제 WBP, 초기 시드·중복 목표·목표 제거·복수 추적기·해제·재생성·소스 부재 검증 추가.
- 첫 빌드에서 Quest 관련 소스는 통과. 별도 변경의 WxCombat 전방 선언 누락 및 WxDataTableRowRename 테스트 AnimNotifyState 헤더 누락 확인. 누락 헤더를 보완했으며 동시 작업이 추가한 중복 전방 선언은 한 개만 유지.


## 검증 결과

- Editor Win64 Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-23_123828_665_3844.log` (Result: Succeeded, exit 0).
- `Saved/Logs/QuestMigrate.log`: WBP_QuestTracker 부모를 WxGame.WxQuestTracker로 전환, Quest/Create Instance·QuestObjective/Manual 유지 및 두 VM 클래스가 WxUI임을 확인. QuestTracker·QuestObjective·GameLayout 3개 WBP를 경고를 오류로 취급하여 컴파일하고 저장 성공.
- `Saved/Logs/QuestLifecycle.log` 및 `Saved/Automation/QuestLifecycle/index.json`: Wx.UI.Quest.TrackerLifecycle 성공 1, 실패 0, 경고 0. 별도 프로세스에서 저장된 실제 WBP로 검증했다.
- Wiki 링크 검사 오류 0, 변경 파일 공백 검사 통과. 새로운 Wx 모듈 의존성과 WxCore 변경 없음.
- 인게임 렌더링·StateTree 퀘스트 진행과 멀티플레이는 이번 검증 범위 밖이다. GameState/QuestComponent의 지연 생성 자동 연결은 기존과 동일하게 제공하지 않는다.
- 2026-09-23 사용자 요청으로 검증 완료한 WxQuestTrackerTest.cpp와 화면의 테스트 전용 friend 선언을 제거했다. 위 테스트 결과는 제거 전 검증 기록이다.

## 화면 클래스 제거 · 리졸버 복귀 · 2026-09-23

상태: 완료 · 인게임 확인 통과

- 사용자 요청: `UWxQuestTracker`를 제거하고 WBP가 뷰모델로 구동되게 한다. 이유: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없다."
- 구조: 보스 바와 같은 세 층(모델 WxQuest 컴포넌트 / 연결 WxGame 리졸버 / 표시 WxUI VM).
- WxQuest: `FWxOnQuestJournalChanged`를 동적에서 네이티브 멀티캐스트로 바꾸고 `BlueprintAssignable`을 없앴다. 인자 없는 동적 델리게이트는 VM을 소유자로 한 람다를 걸 수 없기 때문이다. 이 델리게이트를 BP에서 바인딩한 에셋은 없다(Content 바이너리 검색 0건).
- WxGame `UWxViewModelResolver_Quest` 신규:
  - CreateInstance: 위젯을 Outer로 VM을 만들고, GameState 퀘스트 컴포넌트에 VM을 소유자로 한 약한 람다(CreateWeakLambda)를 건다. 현재 저널을 한 번 반영한다. 컴포넌트가 없으면 빈 VM(`bHasActiveQuest=false`)이다.
  - DestroyInstance: `RemoveAll(VM)`로 그 VM의 구독만 끊는다.
- `WBP_QuestTracker`: 부모 `WxQuestTracker` → `UserWidget`, VM 생성 방식 Create Instance → Resolver. 바인딩 2개와 목표 행 구성은 그대로다.
- `WxQuestTracker.h/.cpp` 삭제. Build.cs·WxCore 변경 없음.
- 기존 제약 유지: GameState·퀘스트 컴포넌트보다 위젯이 먼저 생기면 나중에 연결하지 않는다(클라 복제 지연).

### 검증

- WxEditor Win64 Development 빌드 성공(클래스 삭제 후 최종): `Saved/Logs/BuildDoctor/build_2026-09-23_212120_128_3604.log`, 종료 코드 0.
- 이관: `Saved/Logs/ScreensToResolver.log` (종료 코드 0). 두 WBP 모두 부모 변경·리졸버 전환 후 경고를 오류로 취급한 컴파일을 통과하고 저장했다. `WBP_GameLayout`·`WBP_QuestObjective`도 컴파일을 통과했다.
- 클래스 삭제 후 새 프로세스 재검증: `Saved/Logs/ScreensResolverVerify.log` (종료 코드 0). 부모·리졸버 클래스·VM 클래스·이벤트 목적지를 확인했고, 4개 WBP가 경고를 오류로 취급한 컴파일을 통과했다. 삭제한 두 클래스는 로드되지 않았다.
- Content에서 삭제한 클래스 이름의 바이너리 참조 0건.
- 미실행(인간 확인 필요): PIE에서 대화 표시·클릭 진행·종료 후 재대화, 퀘스트 수주·목표 갱신·완료 시 추적기 표시. 런타임 구독·해제는 코드와 엔진 수명(아래)으로만 확인했다.
- 인게임(사용자 확인, 2026-09-23): 통과. "테스트 문제 없습니다." 인간 코드 리뷰의 별도 승인 기록은 없다.
- 사용자 요청으로 Wiki에 반영하고 푸시했다(2026-09-23).
