# WxDialogue — 코드 리뷰

> 11개 소스의 작고 응집도 높은 모듈이다. 책임 분리(정의/세션/NPC/ST 노드)가 명확하고 실패 갈래마다 로그가 붙어 있으며, 모듈 의존 규칙·코딩 규칙 위반은 남아 있지 않다 — 남은 지적은 전부 세션 수명 관리와 대상 해석 방식에 몰려 있고 심각(🔴) 등급은 없다. 커버리지: `*.Build.cs`·`.uplugin` 포함 소스 11개를 모두 읽었고 세션 컴포넌트 cpp/h 와 StateTree 노드 3종을 정독했으며, 발현 확인용으로 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxQuestComponent`·`UWxMetaHumanVisualComponent`)와 `DT_Dialogue`·`BP_Npc`·`BP_Player` 덤프까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 세션을 밖에서 접을 방법이 없어, 대화 창이 다른 경로로 닫히면 세션이 고착한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:120`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:53-78`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 유일한 경로는 `Advance()` 가 `NextRow == None` 인 행에 닿는 것뿐이다. `EndDialogue()` 는 private 이고 공개 API 에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`/`UninitializeComponent`/빙의 변경 훅이 하나도 없다(헤더 전체에 해당 override 선언 0건). 그런데 대화 창을 닫는 주체는 세션이 아니라 UI 매니저이고, 그쪽에는 세션과 무관하게 창을 닫는 경로가 있다 — `UWxUIManagerSubsystem::WatchPawnTags` 가 폰이 바뀔 때마다 `CloseDialogueScreen()` 을 무조건 부른다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:293`). 게다가 `WBP_DialogueScreen` 은 `bPauseGame=false` 라 대화 중에도 월드가 돌고, `State.Dialogue` 는 상호작용만 막을 뿐 피격을 막지 않는다.
  대화 도중 폰이 죽거나 교체되면 창은 닫히는데 `CurrentRow` 는 남아 `HasActiveDialogue()` 가 계속 참이 된다 — `Advance()` 를 불러 줄 뷰(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:48`)가 사라졌으므로 스스로 풀리지 않는다. 결과는 셋이다.
  1. `FWxStateTreeTask_PlayDialogue::Tick`(`Private/WxDialogueStateTreeNodes.cpp:166-172`)이 영구 Running 이 되어 퀘스트 단계가 정지한다.
  2. `FWxStateTreeTask_WaitDialogueCompleted::Tick`(같은 파일 `:89-115`)은 이미 `bObservedDialogue=true` 였다면 완주 신호를 못 받고 대기하다가, 한참 뒤 **무관한 다음 대화**가 끝나는 순간에야 Succeeded 를 낸다 — 단계 전이가 엉뚱한 시점으로 밀린다.
  3. 스폰한 대화 카메라 액터가 `SetLifeSpan` 을 못 받아 컨트롤러가 사라질 때까지 남는다(`Private/WxDialogueSessionComponent.cpp:256` 은 `EndDialogueCamera` 안에만 있다).

  회복은 다음 대화가 열릴 때 `ClientStartDialogue` 의 선행 `EndDialogue()`(`:114-117`)가 우연히 치워 주는 것뿐이라, 잠긴 NPC·독백처럼 다음 대화가 보장되지 않는 배치에서는 풀리지 않는다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged` 와 `EndPlay`/`UninitializeComponent` 에서 활성 세션을 접도록 한다. 또는 반대로 UI 매니저가 창을 닫을 때 세션에 통보해 "창은 닫혔는데 세션은 열려 있다"는 상태 자체를 없앤다.
- **확신도**: 중간(비대칭 자체는 코드로 확정. 폰 교체 빈도는 콘텐츠에 달렸다)

### 2. 🟡 `CurrentRow` 가 DataTable 행 메모리를 원시 포인터로 캐시한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:140-148`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:148-164`
- **범주**: 버그/정확성
- **문제**: 헤더 주석(`:140`)은 `CurrentStartRow` 를 "세션 동안 행 메모리를 붙잡는 강참조"라고 적었지만, `FDataTableRowHandle::DataTable` 의 강참조가 지키는 것은 `UDataTable` 객체뿐이고 행 메모리 블록이 아니다. 행 실체는 `UDataTable::RowMap` 이 직접 잡은 버퍼이며 CSV 재임포트·로우 구조체 변경(`EmptyTable`/`CleanBeforeStructChange`)에서 통째로 해제·재할당된다. PIE 중 대화 테이블을 재임포트하면 `CurrentRow` 가 해제된 메모리를 가리킨 채 남고, 다음 `Advance()` 의 `CurrentRow->NextRow`(`:60`)에서 크래시한다. 쿠킹 빌드엔 이 경로가 없어 에디터 워크플로 한정이지만, 데이터 테이블을 PIE 중 만지는 것은 흔한 작업이다.
- **제안**: 포인터 대신 `CurrentRowName` 만 진실로 두고 읽는 자리마다 다시 조회한다(`CurrentStartRow.DataTable->FindRow<FWxDialogueTableRow>(CurrentRowName, ...)`). 대사 넘김이 프레임당 1회 미만이라 TMap 조회 비용은 사실상 없고, `HasActiveDialogue()` 는 `CurrentRowName.IsNone()` 으로 대체된다.
- **확신도**: 중간(런타임 실패 경로는 없고 에디터 편집 중에만 드러난다)

### 3. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:269`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로, 액터에 스켈레탈 메시가 둘 이상이면 어느 것이 나올지 보장되지 않는다. `AWxNpc` 는 몸통 메시를 `MeshComponent` 로 이미 명시해 두었는데(`Public/WxNpc.h:49-50`) 이 경로는 그것을 쓰지 않는다. 저장소에서 캐릭터 메시를 이렇게 푸는 곳은 여기와 `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:196`(폴백) 둘뿐이고 나머지는 전부 `ACharacter::GetMesh()` 를 쓴다(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:133`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:674`).
  현재 `BP_Npc` 는 스켈레탈 메시가 하나뿐이라 아직 드러나지 않지만, 이 프로젝트에는 캐릭터에 스켈레탈 메시를 더 다는 패턴이 이미 둘 있다 — `BP_Player` 의 `VisualOverride_GEN_VARIABLE`(외형 교체용 두 번째 메시), 그리고 `UWxMetaHumanVisualComponent` 가 등록 시점에 런타임 생성하는 Face·Outfit 메시(`WxMetaHumanVisualComponent.cpp:45`, `:66`). 후자는 헤더 주석이 명시하듯 오너가 `ACharacter` 가 아닌 계열(즉 `AWxNpc`)까지 겨냥한 컴포넌트다. 붙는 순간 포즈가 Face·Outfit 로 갈 수 있고, 그쪽에는 애님 인스턴스가 있으므로 `:274` 의 경고조차 뜨지 않는 **조용한 오동작**이 된다.
- **제안**: 대상이 `AWxNpc`(또는 `ACharacter`)면 그 액터가 지정한 몸통 메시를 쓰고, 그 밖의 일반 액터에서만 `FindComponentByClass` 로 폴백한다.
- **확신도**: 높음

### 4. 🟡 대화 행이 포즈 몽타주를 하드 참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h:31-32`
- **범주**: 성능/안전
- **문제**: `TObjectPtr<UAnimMontage> TargetPose` 는 하드 참조라, 대화 테이블이 로드되는 순간 그 테이블 모든 행의 몽타주(와 그것이 끌고 오는 스켈레톤·애님 시퀀스)가 함께 로드된다. 테이블은 배치된 NPC 의 `UWxDialogueComponent::StartRow` 핸들이 하드로 붙잡고 있으므로(`Public/WxDialogueComponent.h:25-26`) 레벨 로드와 함께 상주한다. 즉 "말을 걸기 전부터 그 테이블의 모든 대화 연출 애셋이 메모리에 있다"가 되며, 대화 편수와 포즈 종류에 비례해 커지는 종류의 비용이다(`DT_Dialogue` 는 이미 `AM_Death` 를 이 경로로 물고 있다).
- **제안**: `TSoftObjectPtr<UAnimMontage>` 로 바꾸고 `ApplyCurrentPose` 에서 스트리밍한다. 최소한 대화 테이블을 레벨 상주 하드 참조에서 떼어낸다.
- **확신도**: 중간(v1 규모에서는 실측 문제가 아닐 수 있다)

### 5. 🟢 폰 ASC 를 못 찾으면 태그 없이 세션이 열려, 창도 안 뜨고 접히지도 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:134-142`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 폰 ASC 를 찾지 못하면 `State.Dialogue` 를 올리지 못한 채 그냥 지나간다 — 실패로 보지도, 로그를 남기지도 않는다. 그런데 대화 창을 띄우는 유일한 신호가 그 태그이므로(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:321-331`), 이 갈래에서는 세션만 열리고 창이 뜨지 않는다. `Advance()` 를 부를 뷰가 없으니 발견 1과 같은 고착 상태로 직행하고(`Play Dialogue` 는 영구 Running), 증상은 "퀘스트가 그냥 멈춰 있다" 하나로만 남는다. 폰이 없거나(사망 후 소멸·리스폰 대기) ASC 가 없는 폰에 빙의한 순간 `Play Dialogue` 가 도는 조립에서 실재한다.
- **제안**: ASC 부재 갈래에 `LogWxDialogue` Warning 을 남기고, 세션을 열지 않고 되돌린다(창 없는 대화는 어차피 진행이 불가능하다).
- **확신도**: 중간(코드 경로는 확정, 발현 빈도는 콘텐츠 배치에 달렸다)

### 6. 🟢 "빈 대사 = 정상 종료"라는 문서와 "빈 대사 = 경고·실패"라는 코드가 어긋난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h:24-26` vs `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:152-158`
- **범주**: 중복/복잡도
- **문제**: 행 구조체 주석과 README(`Plugins/WxDialogue/README.md:34`)는 "대사가 비면 대화가 종료된다"를 정상 관용구로 안내한다. 그런데 `EnterRow` 는 빈 대사를 실패로 보고 Warning 을 남기며, 그 실패는 `Advance` 에서 "다음 행을 해석하지 못해"라는 두 번째 Warning 을 부른다(`:67-74`). 시작 행이 그런 행이면 `Play Dialogue` 태스크가 아예 Failed 를 낸다(`Private/WxDialogueStateTreeNodes.cpp:157-161`). 동작은 "종료"로 수렴하므로 지금 데이터(`DT_Dialogue` 는 전부 `NextRow=None` 으로 끝낸다)가 깨진 것은 없지만, 안내대로 만드는 다음 대화가 매번 경고 두 줄을 찍고 그 문구가 진짜 오타(존재하지 않는 행 이름)와 구분되지 않는다.
- **제안**: 빈 대사를 정상 종료로 인정해 경고에서 빼거나, 반대로 문서에서 그 관용구를 지우고 `NextRow=None` 하나로 종료를 통일한다.
- **확신도**: 높음

### 7. 🟢 `AWxNpc::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp:54-57`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다. 세션 주입은 Experience(`WAS_CoreGameplay` 의 Add Components) 에 달려 있어 타이밍·구성에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리돼 있으므로(`Private/WxDialogueSessionComponent.cpp:32`, `:45`, `:70`, `:155`, `:274`) 이 모듈에서 여기만 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 8. 🟢 카메라 경로에서 엔진 포인터를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:211`, `:252`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()` 은 카메라 매니저를 검사 없이 역참조한다. 로컬 PC 라면 사실상 항상 유효하지만, 같은 함수의 다른 입력(폰 `:194`, 대상 `:195`, 스폰 결과 `:224`)은 모두 검사를 거치고 있어 여기만 예외다. `EndDialogueCamera` 의 `PlayerController->GetPawn()` 도 null 일 수 있고(대화 중 사망 후 폰 소멸), 그때 엔진이 뷰 타겟을 컨트롤러 자신으로 떨어뜨려 구도가 튄다.
- **제안**: 카메라 매니저 null 검사를 더하고, 복귀 뷰 타겟이 null 이면 뷰 전환을 건너뛴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **규칙 점검 결과**: `Wx` prefix·저작권 첫 줄·람다 부재·`BlueprintCallable` 부재·`Handle` prefix 는 모두 준수(모듈 전역 grep 확인). 모듈 의존은 `WxCore` 외 Wx 플러그인 없음(`WxDialogue.Build.cs`, `WxDialogue.uplugin`) ✅.
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp/.h`, `Plugins/WxQuest/.../WxQuestComponent.cpp`) — 리뷰 대상은 아니다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `ClientStartDialogue` 가 동기 실행되는 것에 기대는 `Play Dialogue`(`Private/WxDialogueStateTreeNodes.cpp:154-161`)와 0번 컨트롤러 폴링(`:18`)은 데디케이티드·원격 클라에서 깨지지만, 헤더·README·세 노드 주석이 모두 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견에서 뺐다. 멀티 확장 시 제일 먼저 손댈 지점이다.
  - `Public/WxDialogueStateTreeNodes.h:70`, `:108`, `:156` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지)과 형식상 충돌하나, 같은 파일 15행이 엔진 StateTree 관례를 근거로 예외임을 명시했고 저장소 내 다른 모듈의 ST 노드가 모두 같은 형태라 발견으로 올리지 않았다. `Private/WxDialogueStateTreeNodes.cpp:13-63` 의 익명 namespace 헬퍼도 다른 도메인 ST 노드 cpp 와 동일한 관례라 같은 이유로 제외했다.
  - `FWxStateTreeTask_EnableNpcInteraction::Tick`(`Private/WxDialogueStateTreeNodes.cpp:221-227`)이 매 틱 `Locator.SyncFind` 로 대상을 재해석하는 비용은 실측하지 않았다. 헤더(`:142-143`)가 스트리밍 재로드 대응을 근거로 명시한 의도이고 인스턴스 수가 적어 발견으로 올리지 않았다.
  - 대화가 겹쳐 열릴 때(`Private/WxDialogueSessionComponent.cpp:114`) 한 프레임에 `SetViewTargetWithBlend` 가 두 번 불리고 `State.Dialogue` 가 1→0→1 로 튀는 경로를 따라갔으나(위젯이 닫히고 다시 push 된다), 최종 뷰 타겟과 뷰모델 시드가 모두 의도대로 수렴해 발견으로 올리지 않았다.
  - 에셋 내부는 범위 밖이라 `.claude/asset_dump` 로 `DT_Dialogue` 의 행 링크·`AM_Death` 참조, `BP_Npc`·`BP_Player` 의 컴포넌트 목록만 확인했고 바인딩 구성은 보지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정, `:200-237`)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - `AWxNpc` 가 영역 메시의 응답을 전부 Ignore 로 두는 구성(`Private/WxNpc.cpp:34-36`)이 상호작용 스캐너의 오버랩 쿼리 방식과 맞물리는지는 확인하지 않았다 — WxWorld 스캐너 리뷰의 몫이다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 11파일 — `/module-review`로 갱신*
