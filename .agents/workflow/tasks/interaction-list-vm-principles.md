# 상호작용 목록 뷰모델을 UI 설계 원칙에 맞추기

상태: 확인 대기 · 체크리스트 6/8 통과
다음 행동: 코드 리뷰와 상호작용 목록 표시를 확인한다.

- woogle 질문(2026-09-30): "뷰모델 설계 원칙에 의하면 InteractionList 뷰모델을 제거해야될까요?" → AI 답: 제거가 아니라 이동입니다. 원칙 2(뷰모델은 WxUI, 도메인 타입 금지)에만 어긋나고, 표시 값과 명령을 둘 곳으로 뷰모델은 여전히 필요합니다.
- woogle 질문(2026-09-30): "리졸버가 모델 전달 외에 구독까지 처리하는 구조가 문제 없는건가요?" → AI 답: 문제 없습니다. 원칙 7은 리졸버가 위젯용으로 새로 만든 뷰모델의 모델 연결·정리를 맡는다고 적고 있고, 대화·퀘스트·보스 리졸버가 같은 구조입니다. 모델 수명이 위젯 수명을 덮어야 하는데, 스캐너는 PC 생성자 컴포넌트라 이 조건을 만족합니다.
- woogle 질문(2026-09-30): "이 InteractionList VM도 일종의 Global VM Collection의 일종이죠?" → "Global Collection에 VM_InteractionList를 두고 이것을 리졸버를 써서 접근하는 것이 더 견고하고 찾기 쉽지 않나요?" → AI가 Q2로 정리했다.
- woogle 결정(2026-09-30): "리졸버 유지히고 작업 진행합시다" → Q2는 리졸버 유지, 구현 승인. Q1은 따로 답하지 않았고 구현 계획이 기준으로 둔 추천안(이번에는 WBP_Interaction을 두고 항목 WBP 5개를 별도 일감으로)으로 진행한다.

## 요청

- 요청 · woogle 2026-09-30

> 네, InteractionList ViewModel, Interaction ViewModel에 대한 개선 계획을 세워주세요.

## 질문

| ID | 질문 | 선택지 | 추천 | 답변 |
| --- | --- | --- | --- | --- |
| Q1 | WBP_Interaction이 행 VM을 받는 방법을 원칙 6(목록은 ListView 항목 뷰모델 확장)대로 바꿀까요? 지금은 OnListItemObjectSet → Cast → SetWxViewModel_Interaction BP 노드로 받습니다. 같은 BP 연결을 쓰는 항목 WBP가 4개 더 있고(WBP_AcquiredItemEntry·WBP_Effect·WBP_ItemSlot·WBP_QuestObjective), 확장을 쓰는 WBP는 없습니다. 엔진 확장은 항목 VM을 행이 만들어진 프레임이 그려진 뒤에 넣어서, 새 행이 한 프레임 동안 디자이너 기본값이나 재사용된 이전 행 값으로 그려집니다. 이 목록은 선택이 바뀔 때마다 행을 다시 만들므로 휠을 돌릴 때마다 생깁니다. 또 확장의 항목 VM 지정 필드가 private이라 WxMVVMToolset에 설정 함수를 새로 만들어야 합니다. | 이번에는 두고 항목 WBP 5개를 별도 일감으로 / WBP_Interaction만 이번에 확장으로 / 항목 WBP 5개 모두 이번에 확장으로 | 이번에는 두고 별도 일감으로. 하나만 바꾸면 항목 연결 방식이 두 가지로 갈립니다. 한 프레임 문제와 도구 함수는 5개에 공통이라 한 번에 판단하고 검증하는 편이 낫습니다. 이 일감은 원칙 2·9의 알려진 예외에 집중합니다. | (따로 답하지 않음, 추천안 기준 계획을 승인) "리졸버 유지히고 작업 진행합시다" · woogle 2026-09-30 |
| Q2 | VM_InteractionList를 로컬 PC가 만들어 Global Collection에 등록하고, WBP는 플레이어 리졸버로 받게 할까요? 그러면 InteractionList 리졸버가 없어지고 스캐너 구독은 PC가 한 번만 겁니다. 대신 원칙 8(공유가 필요할 때만 Global Collection)을 "플레이어 단위 모델을 보여 주는 VM은 뷰가 하나여도 PC가 등록한다"로 고쳐야 합니다. 같은 처지인 대화 VM(PC의 대화 세션 컴포넌트, 지금은 리졸버)을 따라 옮길지도 정해야 기준이 갈리지 않습니다. | 리졸버 유지(지금 계획) / Global Collection으로 옮기고 원칙 8을 고침, 대화는 나중에 / Global Collection으로 옮기고 원칙 8을 고침, 대화도 이번에 | 리졸버 유지. 이 경우 견고함 차이가 작습니다. 스캐너는 PC 생성자 컴포넌트라 위젯보다 먼저 있고, 해제를 놓쳐도 약참조 람다라 실행되지 않습니다. 코드 어디서나 조회할 수 있게 되는 점은 원칙 5가 일부러 막아 둔 것이고, 원칙을 고치면 대화까지 영향이 갑니다. | 리졸버 유지히고 작업 진행합시다 · woogle 2026-09-30 |

## 구현 계획

구현 승인: woogle 2026-09-30

아래는 Q1 추천안(이번에는 WBP_Interaction을 두는 안) 기준입니다. 다른 답이면 맨 아래 「Q1 답에 따른 추가」를 더합니다.

**목표 구조**

- 모델: `UWxInteractionScannerComponent`(WxWorld). 행과 선택을 소유하고 바뀌면 신호를 냅니다.
- 조립: `UWxViewModelResolver_InteractionList`(WxGame). 위젯마다 목록 VM을 만들고, 스캐너 구독·값 넣기·명령 연결·해제를 맡습니다(원칙 3·7).
- 뷰모델: `UWxViewModel_InteractionList`·`UWxViewModel_Interaction`(둘 다 WxUI). 표시 값과 명령 델리게이트만 둡니다(원칙 2·9).
- 뷰: WBP_InteractionList(Enhanced Input → `Request~`), WBP_Interaction(항목).

**코드 변경**

1. WxWorld `UWxInteractionScannerComponent`
   - `OnRowsChanged`를 네이티브 멀티캐스트(`DECLARE_MULTICAST_DELEGATE`)로 바꾸고 `UPROPERTY(BlueprintAssignable)`를 뗍니다. 인자 없는 동적 델리게이트에는 리졸버가 VM 소유 람다를 걸 수 없어서입니다. 퀘스트 저널(`FWxOnQuestJournalChanged`)과 같은 방식입니다. 이 신호를 쓰는 에셋은 0건이고, 발행하는 두 곳은 그대로 둡니다.
   - 헤더 주석에서 뷰모델이 스캐너를 직접 부른다는 서술 두 줄을 리졸버가 잇는다는 내용으로 고칩니다.
2. `UWxViewModel_InteractionList`를 WxGame에서 WxUI로 옮깁니다(`git mv`로 `Plugins/WxUI/Source/WxUI/Public/MVVM/`·`Private/MVVM/`에).
   - 남김: `Entries`(FieldNotify), `RequestInteract()`, `RequestCycle(int32)`.
   - 추가: `SetRows(const TArray<FText>& Prompts, int32 SelectedIndex)`. 지금 `HandleRowsChanged`의 행 재생성 로직을 옮깁니다. 명령 델리게이트 `FSimpleDelegate OnInteractRequested`와 `OnCycleRequested`(int32 한 개짜리 네이티브 델리게이트)를 두고, `Request~`는 이것을 실행하기만 합니다(`UWxViewModel_Dialogue::OnAdvanceRequested`와 같은 방식).
   - 제거: `Initialize`, `Deinitialize`, `HandleRowsChanged`, `CachedScanner`, 스캐너 include.
3. `UWxViewModelResolver_InteractionList`를 WxGame의 자기 파일(`Source/WxGame/MVVM/WxViewModelResolver_InteractionList.h/.cpp`)로 분리합니다. 모듈이 그대로라 클래스 경로가 바뀌지 않아 리다이렉트가 필요 없습니다.
   - `CreateInstance`: 위젯을 Outer로 VM을 만듭니다(지금은 PC, 대화·퀘스트는 위젯). 소유 PC의 스캐너를 찾으면 VM 소유 약참조 람다로 `SetRows(GetPrompts(), GetSelectedIndex())`를 한 번 실행하고 `OnRowsChanged`에 겁니다. 두 명령 델리게이트는 스캐너의 `TryInteractSelected`·`CycleSelection`에 `BindUObject`로 잇습니다. PC나 스캐너가 없어도 nullptr 대신 빈 VM을 돌려줍니다(원칙 6, 대화·퀘스트와 같음).
   - `DestroyInstance`: VM의 Outer 위젯 → 소유 PC → 스캐너 순으로 찾아 `OnRowsChanged.RemoveAll(VM)`.
   - 헤더 주석에는 "스캐너는 PC 생성자 컴포넌트라 위젯보다 먼저 있다"는 전제를 옮겨 둡니다. 리졸버는 위젯이 뜰 때 한 번만 불리므로 이 전제가 필요합니다.
4. `UWxViewModel_Interaction`(WxUI): 클래스와 필드는 그대로 둡니다. 원칙에 어긋나는 곳이 없습니다. 헤더 주석의 "스캐너의 행(선택지) 하나당"만 "목록 VM이 행 하나당"으로 고칩니다.
5. `Config/DefaultEngine.ini`에 `[CoreRedirects]`의 `+ClassRedirects=(OldName="/Script/WxGame.WxViewModel_InteractionList",NewName="/Script/WxUI.WxViewModel_InteractionList")`를 넣습니다. WBP_InteractionList를 헤드리스로 리세이브한 뒤 `BatchFiles/CheckRedirects.bat`에서 SAFE 판정을 받으면 지웁니다.

**에셋 변경**

- WBP_InteractionList: 리세이브만 합니다(뷰모델 클래스 경로만 WxUI로 바뀜). 그래프·바인딩·리졸버 설정은 그대로입니다.
- WBP_Interaction: 바꾸지 않습니다(Q1 추천안).

**바꾸지 않는 것**

- 선택이 바뀔 때마다 행 VM을 전부 다시 만드는 방식(2026-09-23 결정).
- WBP_InteractionList의 입력 처리(Enhanced Input 이벤트 → `RequestInteract`·`RequestCycle`), 스캐너의 선택 소유와 `ServerInteract` RPC.

**검증**

- 빌드: WxEditor DebugGame·Development. 리다이렉트를 넣은 뒤, 그리고 리다이렉트 제거·임시 테스트 삭제 뒤 각각 빌드합니다.
- 위젯 BP 헤드리스 컴파일(`CompileAllBlueprints -BlueprintBaseClass=WidgetBlueprint`)과 `CheckRedirects.bat`.
- 헤드리스 임시 자동화 테스트(`Source/WxGame/Tests/`, LV_DevCombat, ListView 항목 확인에는 RenderOffscreen). 결과를 기록한 뒤 지우고 다시 빌드합니다.
  - 연결: HUD의 WBP_InteractionList가 받은 VM이 WxUI 클래스이고 Outer가 그 위젯입니다.
  - 값: 플레이어 옆에 상호작용 대상 둘을 두면 `Entries`의 수·Prompt·bSelected가 스캐너 `GetPrompts`·`GetSelectedIndex`와 같고, ListView 항목(WBP_Interaction)이 각 행 VM을 받습니다.
  - 명령: `RequestCycle(1)`을 부르면 스캐너 선택과 `bSelected`가 다음 행으로 옮겨 갑니다. `RequestInteract`를 부르면 선택 대상으로 폰 ASC에 `Event.Interact`가 도착합니다.
  - 범위 이탈: 플레이어를 멀리 옮기면 `Entries`가 0개입니다.
  - 해제: 위젯을 제거하면 스캐너 `OnRowsChanged`에 그 VM의 구독이 남지 않습니다. 빙의 교체로 HUD가 새로 생기면 새 VM이 연결되고 옛 VM 구독은 남지 않습니다.
- 코드 확인(grep): WxUI에 WxWorld include와 스캐너 타입이 0건, Content에 옛 클래스 경로 참조가 0건입니다.

**테스트 체크리스트 초안**

| 항목 | 확인 방법 | 담당 |
| --- | --- | --- |
| 빌드 | WxEditor DebugGame·Development(리다이렉트 추가 뒤, 리다이렉트 제거·임시 테스트 삭제 뒤) | AI |
| 위젯 BP 컴파일·리다이렉트 | 헤드리스 CompileAllBlueprints(WidgetBlueprint), CheckRedirects.bat SAFE 판정 뒤 제거 | AI |
| 연결·값 | 임시 자동화 테스트: VM 클래스·Outer, 대상 둘일 때 Entries와 스캐너 값 일치, 항목 위젯의 행 VM | AI |
| 명령 | 같은 테스트: RequestCycle 선택 이동, RequestInteract가 Event.Interact로 도착 | AI |
| 범위 이탈·해제·재생성 | 같은 테스트: 범위 이탈 Entries 0, 위젯 제거 뒤 구독 없음, 빙의 교체 뒤 새 VM 연결 | AI |
| 코드 확인 | grep: WxUI의 WxWorld 의존 0건, 옛 클래스 경로 참조 0건 | AI |
| 코드 리뷰 | 변경 파일: WxInteractionScannerComponent(신호 종류·주석), WxViewModel_InteractionList(WxUI로 이동·SetRows·명령 델리게이트), WxViewModelResolver_InteractionList(신규 파일·구독과 해제·Outer), WxViewModel_Interaction(주석), DefaultEngine.ini(리다이렉트 추가 뒤 제거). 에셋: WBP_InteractionList 리세이브. 볼 점: 원칙 2·3·7·9와 맞는지, 대화·퀘스트 리졸버와 모양이 같은지 | 사람 |
| 상호작용 목록 표시 | 게임: 상호작용 대상이 여럿 겹친 곳에서 목록을 보고, 휠로 선택을 바꾸고, 상호작용한 뒤 범위를 벗어납니다. 전과 같이 보이고 선택 표시가 따라가야 합니다. | 사람 |

**Q1 답에 따른 추가**

- 「WBP_Interaction만」 또는 「5개 모두」면 다음을 더합니다.
  - WxMVVMToolset에 ListView 위젯에 항목 뷰모델 확장을 붙이고 항목 VM을 지정하는 함수를 만듭니다(엔진 `CreateBlueprintWidgetExtension` + 리플렉션으로 private 필드 설정).
  - 대상 목록 WBP에 확장을 붙이고, 항목 WBP의 OnListItemObjectSet 연결 노드를 지웁니다.
  - 체크리스트: AI 항목으로 항목 위젯이 행 VM을 받는지 확인하고, 사람 항목으로 휠을 돌릴 때 한 프레임 깜빡임이 보이는지 확인합니다.
- 「별도 일감」이면 이 일감의 코드 리뷰 요약에 항목 WBP 5개의 후속 일감이 필요하다고 한 줄 남깁니다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 빌드 | WxEditor DebugGame·Development(리다이렉트 추가 뒤, 리다이렉트 제거·임시 테스트 삭제 뒤) | AI | 통과 | 리다이렉트를 넣은 뒤 두 구성 모두 Result: Succeeded. 임시 테스트를 지운 상태로 두 구성 다시 Succeeded |
| 위젯 BP 컴파일·리다이렉트 | 헤드리스 CompileAllBlueprints(WidgetBlueprint), CheckRedirects.bat SAFE 판정 뒤 제거 | AI | 통과 | WBP_InteractionList를 헤드리스 ResavePackages로 저장(1/1) → CheckRedirects SAFE 1건 → 리다이렉트 제거(DefaultEngine.ini는 HEAD와 같음). 제거 뒤 위젯 BP 88개 모두 successful, 0 errors·0 warnings(WBP_InteractionList·WBP_Interaction·WBP_GameLayout 포함) |
| 연결·값 | 임시 자동화 테스트: VM 클래스·Outer, 대상 둘일 때 Entries와 스캐너 값 일치, 항목 위젯의 행 VM | AI | 통과 | 헤드리스 게임(LV_DevCombat -game -RenderOffscreen) 임시 테스트 Wx.Temp.InteractionList 성공. VM 클래스 패키지 /Script/WxUI, Outer WBP_InteractionList_C, 스캐너 신호와 두 명령이 연결됨. 픽업 둘(Potion x2·Potion)을 두면 스캐너 2행과 VM 2행의 문구·선택이 같고, ListView 항목 위젯 2개가 각 행 VM을 받아 같은 문구를 표시 |
| 명령 | 같은 테스트: RequestCycle 선택 이동, RequestInteract가 Event.Interact로 도착 | AI | 통과 | RequestCycle(1)로 스캐너 선택 0→1, VM 선택 표시도 따라감. RequestInteract로 Event.Interact 1회, OptionalObject가 선택한 픽업. 픽업이 파괴된 뒤 스캐너·VM 모두 1행 |
| 범위 이탈·해제·재생성 | 같은 테스트: 범위 이탈 Entries 0, 위젯 제거 뒤 구독 없음, 빙의 교체 뒤 새 VM 연결 | AI | 통과 | 폰을 50m 위로 옮기면 스캐너·VM 0행. UnPossess·폰 파괴·RestartPlayer 뒤 새 HUD의 새 VM이 연결되고 옛 VM(아직 살아 있음)의 구독은 없음. 행 1개가 있는 채로 같은 폰을 다시 빙의하면 스캐너 신호 0회인데 새 VM이 1행(생성 때 처음 값 넣기 확인), 옛 VM 구독 없음. 결과 기록 뒤 임시 테스트를 지움 |
| 코드 확인 | grep: WxUI의 WxWorld 의존 0건, 옛 클래스 경로 참조 0건 | AI | 통과 | WxUI 소스에 WxWorld·스캐너 참조 0건, Source/WxGame/MVVM에 옛 VM 파일 0개, Config에 리다이렉트 0건. OnRowsChanged 사용처는 리졸버의 구독·해제 두 줄과 스캐너 발행 두 곳. WBP_InteractionList는 /Script/WxUI(VM)와 /Script/WxGame(리졸버)만 참조 |
| 코드 리뷰 | 변경 파일: WxInteractionScannerComponent.h/.cpp(신호를 네이티브 멀티캐스트로·주석 세 줄), WxViewModel_InteractionList.h/.cpp(WxGame에서 WxUI로 git mv·SetRows·명령 델리게이트), WxViewModelResolver_InteractionList.h/.cpp(WxGame 신규 파일·구독과 해제·Outer를 위젯으로), WxViewModel_Interaction.h(주석 한 줄), module_review_WxGame.md(맡은 일감 링크). 에셋: WBP_InteractionList 리세이브(VM 클래스 경로만 WxGame→WxUI, 그래프·바인딩은 그대로). DefaultEngine.ini는 리다이렉트를 넣었다가 지워 변경 없음. 볼 점: 원칙 2·3·7·9와 맞는지, 대화·퀘스트 리졸버와 모양이 같은지. 후속: 항목 WBP 5개의 ListView 항목 뷰모델 확장 전환(Q1)은 별도 일감 list-entry-viewmodel-extension.md | 사람 | 대기 |  |
| 상호작용 목록 표시 | 게임: 상호작용 대상이 여럿 겹친 곳에서 목록을 보고, 휠로 선택을 바꾸고, 상호작용한 뒤 범위를 벗어납니다. 전과 같이 보이고 선택 표시가 따라가야 합니다. | 사람 | 대기 |  |

## 구현 진행 · 2026-09-30

- 계획대로 구현했다. 계획에 없던 변경은 스캐너 `EndPlay` 주석 한 줄뿐이다("선택 VM을 정리한다"를 "구독자에게 빈 목록을 알린다"로, 모델이 VM을 아는 서술이라서).
- VM 파일은 `git mv`로 옮겨 이름 바꾸기가 스테이징돼 있다. 리졸버 새 파일과 이 기록은 추적되지 않은 상태다.
- 헤드리스 확인 중 본 무관한 기존 경고: BP_ItemPickup이 지워진 `/Script/WxWorld.WxInteractionComponent` 컴포넌트(`WxInteraction`)를 아직 들고 있어 로드 때 "Component class is not set" 경고가 난다(에셋 마지막 수정 2026-06-13). AcquiredItemList의 "Cannot add null item into ListView"도 이전 기록과 같은 기존 경고다.
- Windows PowerShell 5.1에서 `-BlueprintBaseClass=/Script/UMGEditor.WidgetBlueprint`를 따옴표 없이 넘기면 점에서 인자가 잘려 위젯 BP가 하나도 컴파일되지 않는다. 인자를 작은따옴표로 감싸 다시 돌렸다.

## 조사 · 2026-09-30

- 원칙 정본: [MVVM 원칙에 맞춘 뷰모델 재설계](viewmodel-mvvm-redesign.md) 구현 계획 절. 알려진 예외: "InteractionList 뷰모델: WxGame에 있고 도메인 타입을 써 2번에 어긋납니다. 뷰모델은 WxUI로, 값 넣기는 조립 층으로 옮깁니다." [WxGame 모듈 리뷰](module_review_WxGame.md) 1번도 같은 지적입니다.
- 현재 코드
  - 목록 VM(`Source/WxGame/MVVM/WxViewModel_InteractionList.h/.cpp`)이 스캐너를 약참조로 들고(h:45) `OnRowsChanged`를 직접 구독하며(cpp:17), `RequestInteract`·`RequestCycle`이 스캐너를 바로 부릅니다(cpp:55·63). 리졸버는 같은 파일에 있고 VM Outer를 PC로 잡으며, PC가 없으면 nullptr을 돌려줍니다(cpp:69-77).
  - 스캐너 `OnRowsChanged`는 인자 없는 동적 멀티캐스트 + BlueprintAssignable이고(WxInteractionScannerComponent.h:14·55), 발행하는 곳은 cpp:246·259 두 곳입니다. 에셋 문자열 검색 결과 이 신호를 쓰는 에셋은 없습니다.
  - 행 VM `UWxViewModel_Interaction`(WxUI)은 `Prompt`·`bSelected`만 가진 불변 VM입니다.
- 에셋(uasset 문자열 검색, unreal-mcp 미연결이라 에디터로는 보지 않음)
  - WBP_InteractionList: VM 생성 방식은 Resolver(`WxViewModelResolver_InteractionList`)입니다. `Entries` → CommonListView `BP_SetListItems` 바인딩과 IA_Interact·IA_Interact_Next·IA_Interact_Prev 입력 이벤트에서 `RequestInteract`·`RequestCycle`을 부릅니다.
  - WBP_Interaction: `UserObjectListEntry`를 구현하고 VM 생성 방식은 Manual입니다. `OnListItemObjectSet` → Cast → `SetWxViewModel_Interaction`으로 행 VM을 받고, `Prompt`·`bSelected`(Conv_BoolToSlateVisibility)를 바인딩합니다.
  - `OnListItemObjectSet` 연결을 쓰는 항목 WBP: WBP_AcquiredItemEntry, WBP_Effect, WBP_Interaction, WBP_ItemSlot, WBP_QuestObjective. ListView 항목 뷰모델 확장을 쓰는 에셋은 0건입니다.
- 참고 구현
  - 퀘스트 리졸버(`WxViewModelResolver_Quest.cpp`): 인자 없는 네이티브 신호에 VM 소유 약참조 람다를 걸고, 처음 값을 한 번 넣은 뒤 `RemoveAll(VM)`으로 해제합니다. 자식 VM 목록을 통째로 다시 만드는 것(`SetJournal`)도 이번 `SetRows`와 같습니다.
  - 대화 VM·리졸버: 명령은 VM의 `FSimpleDelegate OnAdvanceRequested`를 리졸버가 `BindUObject`로 모델 함수에 잇습니다.
- 엔진 사실(UE 5.8 소스)
  - 리졸버 해제 훅: 위젯이 소스를 해제할 때 수동으로 넣은 소스가 아니면 `Resolver->DestroyInstance`를 부릅니다(`MVVMView.cpp:320-325`, `MVVMViewClass.cpp:199-203`).
  - ListView 항목 뷰모델 확장(`MVVMViewListViewBaseExtension.cpp`)은 `OnEntryWidgetGenerated`에서 항목 위젯에 `SetViewModel`을 합니다. 이 신호는 행을 만든 뒤 코어 티커로 다음 틱에 알립니다(`ListViewBase.cpp:320-345`). 한 프레임 안에서 코어 티커는 Slate 그리기 뒤에 돕니다(`LaunchEngineLoop.cpp:5991`·`6103`). 반면 `OnListItemObjectSet`은 행을 만들 때 바로 불립니다(`SObjectTableRow.h:856-861`).
  - 확장의 항목 VM 지정 필드 `EntryViewModelId`는 편집 지정자 없는 private `UPROPERTY()`입니다(`MVVMViewBlueprintListViewBaseExtension.h:52-53`). 확장을 붙이는 공개 함수는 `UMVVMWidgetBlueprintExtension_View::CreateBlueprintWidgetExtension`입니다.
- 기록에서 확인한 것: 목록 VM 자체를 없앨 수 있다는 보고는 일감 기록·git 이력·Wiki 원자료에서 찾지 못했습니다. 2026-09-23 [상호작용 목록 VM 단순화](interaction-list-vm-simplification.md)는 제거 요청을 검토한 뒤 유지로 판단했습니다. 2026-09-29 뷰모델 품질 정리 조사 7번은 "대화·퀘스트처럼 WxUI VM과 WxGame 리졸버로 나눈다"는 이동안이었습니다.
