# Quest 표시 VM의 WxUI 이전

상태: 완료

- 합의: Quest와 QuestObjective VM을 WxUI로 이전하고 순수 표시 데이터로 유지한다. WxGame의 QuestTracker가 저널 구독을 맡는다.
- 조사: WBP_QuestTracker는 UUserWidget 기반. Create Instance로 전환하고 NativeConstruct 이후 연결, NativeDestruct 이전 해제한다. Objective의 기존 행 바인딩은 유지한다.
- 제약: Wx 기능 모듈 간 의존성 및 WxCore 정의 추가 없음.
- 환경: 실행 중인 DebugGame 에디터의 파일 잠금은 사용자가 저장 후 종료하여 해결했다.

## 구현

- Quest·QuestObjective h/cpp를 WxUI Public/Private로 이전하고 WXUI_API 적용.
- Quest SetJournal은 표시 필드와 목표별 VM만 갱신. WxGame QuestTracker가 GameState 조회·구독·초기 저널 동기화 담당.
- Quest Resolver 제거, 기존 두 VM 클래스 경로 리다이렉트 추가. Build.cs와 WxCore 변경 없음.
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
