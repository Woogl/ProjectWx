# StateTree 기반 퀘스트 시스템 (WxQuest)

## 계획

### 목표
퀘스트 1개 = StateTree 에셋 1개로 표현하는 퀘스트 시스템을 WxQuest 플러그인에 구축하고, WBP_GameHUD에 추적 위젯(제목+목표)을 추가한다. 상태별 태스크와 네이티브 전이(On State Completed / On Event)로 진행을 기술한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Wx.uproject` | Plugins 배열에 WxQuest 등록(현재 누락) | 수정 |
| `Plugins/WxQuest/WxQuest.uplugin` | GameplayStateTree·ModularGameplay 플러그인 의존 추가 | 수정 |
| `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs` | ModularGameplay(Public)·GameplayStateTreeModule(Private) 추가 | 수정 |
| `Plugins/WxCore/.../WxGameplayTags.h/.cpp` | Quest.Fail 태그 추가 | 수정 |
| `Plugins/WxQuest/.../Quest/WxQuestComponent.h/.cpp` | 퀘스트 실행·저널 컴포넌트(UGameStateComponent 파생, 내부 UStateTreeComponent 러너 소유) | 신규 |
| `Plugins/WxQuest/.../Quest/WxQuestStateTreeNodes.h/.cpp` | 퀘스트 ST 태스크 5종(RegisterJournal·SetQuestObjective·MarkIndicator·WaitMoveToTarget·StartNextQuest) | 신규 |
| `Plugins/WxQuest/.../Quest/WxQuestLibrary.h/.cpp` | BP 진입점(StartQuest·SendQuestEvent) | 신규 |
| `Plugins/WxWorld/.../Spawnable/WxSpawnerStateTreeNodes.h/.cpp` | 스포너 ST 태스크 2종(TriggerSpawnersByTag·WaitSpawnersKilled) | 신규 |
| `Source/WxGame/WxGame.Build.cs` | WxQuest 의존 추가 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Quest.h/.cpp` | HUD 추적용 VM+리졸버 | 신규 |

### 접근 방식
- **실행 주체는 GameState 전역 컴포넌트**: UGameStateComponent 파생 컴포넌트를 기존 GameMode의 프레임워크 컴포넌트 주입 인프라(GM_Combat 에셋 설정)로 부착한다. 리슨서버에서 PC별 이중 구동으로 월드 부수효과가 중복되는 구조를 원천 배제하고, WxGame 프레임워크 코드 수정이 없다.
- **컴포지션으로 ST 구동**: UGameStateComponent와 UStateTreeComponent는 다중상속이 불가하므로, 퀘스트 컴포넌트가 순정 UStateTreeComponent를 런타임 생성·소유한다. 퀘스트 시작은 러너 정지→에셋 교체→시작 순서(엔진이 Running 중 교체를 거부함을 확인). 트리 종료는 러너의 실행 상태 변경 델리게이트로 통지받아 저널을 자동 정리하므로 UnregisterJournal 태스크는 두지 않는다.
- **레벨 참조는 액터 태그 조회**: 호스트가 레벨 액터가 아니라 기믹식 소프트포인터 바인딩이 불가하다. 태스크 파라미터에 FName 태그를 적고 조회하며, 월드 파티션 언로드에 대비해 틱 태스크는 캐시가 빌 때만 재해석한다.
- **크로스 도메인은 ST 에셋 조립**: 보상은 기존 WxInventory Grant Reward 태스크 재사용, 스폰·처치 감시는 WxWorld 신규 태스크. WxQuest와 코드 의존 없음.
- **재진입 회피**: 태스크 EnterState 안에서 러너 재시작은 엔진 가드에 차단되므로 StartNextQuest는 next-tick 타이머로 위임한다.
- **HUD**: 퀘스트 태스크가 컴포넌트 저널(제목·목표)을 갱신하면 VM이 델리게이트로 받아 FieldNotify로 밀고, 위젯은 리졸버로 VM을 확보한다(기존 MVVM 관례).
- **세이브 연동은 보류**: 이후 활성 리프 상태 GUID 저장 방식으로 확장 가능함을 확인해 두었다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Wx.uproject` · `WxQuest.uplugin` · `WxQuest.Build.cs` | WxQuest 등록, GameplayStateTree·ModularGameplay 의존 추가 | 수정 |
| `WxCore .../WxGameplayTags.h/.cpp` | Quest.Fail 태그 추가 | 수정 |
| `WxQuest .../Quest/WxQuestComponent.h/.cpp` | 퀘스트 실행·저널 컴포넌트(UGameStateComponent 파생, 내부 UStateTreeComponent 러너 소유) | 신규 |
| `WxQuest .../Quest/WxQuestStateTreeNodes.h/.cpp` | 태스크 5종(RegisterJournal·SetQuestObjective·MarkIndicator·WaitMoveToTarget·StartNextQuest) | 신규 |
| `WxQuest .../Quest/WxQuestLibrary.h/.cpp` | BP 진입점(StartQuest·SendQuestEvent) | 신규 |
| `WxWorld .../Spawnable/WxSpawnerStateTreeNodes.h/.cpp` | 태그 해석형 스포너 태스크 2종(TriggerSpawnersByTag·WaitSpawnersKilled) | 신규 |
| `WxGame.Build.cs` · `Source/WxGame/MVVM/WxViewModel_Quest.h/.cpp` | WxQuest 의존, HUD 추적 VM+리졸버 | 수정·신규 |
| `/Game/Quest/` ST_Quest·ST_Quest2·BP_QuestMarker·BP_QuestComponent, `/Game/UI/Widget/WBP_QuestTracker`, WBP_GameHUD(TopRight 슬롯), GM_Combat(FrameworkComponents), LV_DevCombat(캠프 타겟+스포너 2기) | 에셋 일체(MCP 저작) | 신규·수정 |

### 구현·결정과 그 이유
- **컴포지션 러너**: UGameStateComponent와 UStateTreeComponent가 다중상속 불가라, 퀘스트 컴포넌트가 권위 측에서 순정 러너를 런타임 생성해 위임했다. 엔진 확인 결과 Running 중 에셋 교체가 거부되므로 시작은 정지→교체→시작 순서로 고정했다.
- **저널 자동 정리**: 러너의 실행 상태 변경 델리게이트를 자기 구독해 Running 이탈 시 정리한다. 완료·실패·교체 세 경로가 한 곳으로 수렴해 UnregisterJournal 태스크가 불필요해졌다.
- **재진입 회피**: 태스크 EnterState 안 러너 재시작은 엔진 가드에 차단됨을 확인, StartNextQuest는 소프트 참조를 다음 틱 타이머로 위임한다(GC 안전).
- **에셋 저작은 MCP 전면 활용**: ST는 기존 에셋 복제 후 인스턴스드 배열 성장(클래스 경로 전달)으로 상태 7개를 재구성했고, WBP는 WBP_Dialogue 복제 후 트리·그래프·MVVM 컨텍스트/바인딩을 재배선했다(신규 위젯에 MVVM 확장 객체를 새로 만들 수는 없어 복제가 유일 경로).
- **PIE 검증 완료 범위**: 자동 시작→저널 등록·목표 표시, 타겟 도달→Step2 전이·목표 갱신·스포너 트리거로 적 2기 스폰, 플레이어 사망에도 저널 유지(전역 컴포넌트), ST_Quest2 부팅·저널 재등록(퀘스트 교체 호환성). 스크린샷으로 확인.

### 계획 대비 달라진 점
- **ST 복제 함정 2건**: 복제된 에셋의 스키마 ContextActorClass가 BP_Door_C로 남아 트리 시작이 거부됐다(→ Actor로 수정). 트리 종료 링크(Succeeded/Failed)는 전체 전이 임포트 시 자기 GotoState로 뭉개져 부분 필드 갱신으로 재설정했다.
- **샘플 문구 영문화**: TextStyle_Small의 폰트(Roboto)가 한글 글리프 미지원이라 한글이 □로 표시돼, 데모 확인을 위해 영문으로 교체했다. 프로젝트 전반의 기존 상태다.
- **표시여부 바인딩 보류**: bHasActiveQuest→Visibility 컨버전 바인딩은 컨버전 함수 객체를 MCP로 신설할 수 없어 보류하고, 대신 보더를 제거해 빈 텍스트=비표시가 되게 했다.

### 후속 과제
- **실플레이 마무리 검증(사용자)**: 적 전멸→Success→골드 지급→다음 퀘스트 자동 전환의 실전투 확인. 자동화로는 적 처치가 불가했다(데미지존이 적 무피해, 속성 직접 조작 불가). 감지 경로는 기존 HandleDeath→MarkKilled에 연결돼 있다. Quest.Fail 이벤트 경로(UWxQuestLibrary::SendQuestEvent)도 실기동 미확인.
- **한글 UI 폰트**: 한글 지원 폰트 도입 후 TextStyle 계열 교체 및 퀘스트 문구 한글 복원.
- **트래커 표시여부·위치**: 에디터에서 bHasActiveQuest→Visibility 컨버전 바인딩 추가(Conv_BoolToSlateVisibility), TopRight 슬롯 내 정렬 조정.
- **v2 검토**: 픽업형 보상 스폰 기준 바인딩(GrantReward 오너=GameState라 원점 스폰), 세이브 연동(활성 리프 GUID), 리슨서버 원격 클라 저널 복제.
