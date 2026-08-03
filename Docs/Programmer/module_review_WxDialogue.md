# WxDialogue — 코드 리뷰

> 11개 소스로 이루어진 작고 응집도 높은 모듈이다. 지난 리뷰의 🔴(대화 겹침 시 태그 카운트 누수)와 무음 실패 경로는 실제로 해소됐고, 모듈 의존 규칙·코딩 규칙 위반도 사실상 남아 있지 않다 — 이번에 남은 지적은 전부 수명 관리와 데이터 참조 방식에 몰려 있다. 커버리지: `*.Build.cs`·`.uplugin` 포함 소스 11개를 모두 읽었고 세션 컴포넌트 cpp/h 와 StateTree 노드를 정독했으며, 발현 확인용으로 소비처(`UWxViewModel_Dialogue`·`UWxUIManagerSubsystem`·`IWxInteractable`)와 `WBP_DialogueScreen` 덤프까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 세션을 밖에서 접을 방법이 없어, 대화 창이 다른 경로로 닫히면 세션이 영구 고착한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:48-76`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:53-78`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 유일한 경로는 `Advance()` 가 `NextDialogue == None` 인 행에 닿는 것뿐이다. `EndDialogue()` 는 private 이고(`Public/WxDialogueSessionComponent.h:120`) 공개 API 에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`/`OnUnregister`/빙의 변경 훅도 없다. 그런데 대화 창을 닫는 주체는 세션이 아니라 UI 매니저이고, 그쪽에는 세션과 무관하게 창을 닫는 경로가 있다 — `UWxUIManagerSubsystem::WatchPawnTags` 가 폰이 바뀔 때마다 `CloseDialogueScreen()` 을 무조건 부른다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:293`). 게다가 `WBP_DialogueScreen` 은 `bPauseGame=false` 라 대화 중에도 게임이 돌고 플레이어가 피해를 입을 수 있다.
  대화 도중 폰이 교체되면 창은 닫히는데 `CurrentRow` 는 남아 `HasActiveDialogue()` 가 영영 참이 된다 — `Advance()` 를 불러 줄 뷰가 사라졌으므로 스스로 풀리지 않는다. 결과로 `Wait Dialogue Completed`(대사 목격 후 세션이 닫히기를 기다림)와 `Play Dialogue`(세션 종료로 성공 판정)가 영구 Running 이 되어 퀘스트가 소프트락되고, 스폰한 대화 카메라 액터도 `SetLifeSpan` 을 못 받아 컨트롤러가 사라질 때까지 남는다. 다음 대화가 열릴 때 `ClientStartDialogue` 의 선행 `EndDialogue()`(`:114-117`)가 우연히 치워 주지만, 그 사이 걸린 퀘스트 게이트는 이미 지나간 뒤다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged` 와 `EndPlay`/`OnUnregister` 에서 활성 세션을 접도록 한다. 또는 반대로 UI 매니저가 창을 닫을 때 세션에 통보해 "창은 닫혔는데 세션은 열려 있다"는 상태 자체를 없앤다.
- **확신도**: 중간(비대칭 자체는 코드로 확정. 폰 교체 빈도는 콘텐츠에 달렸다)

### 2. 🟡 `CurrentRow` 가 DataTable 행 메모리를 원시 포인터로 캐시한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:140-148`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:150-163`
- **범주**: 버그/정확성
- **문제**: 헤더 주석은 `CurrentStartRow` 를 "세션 동안 행 메모리를 붙잡는 강참조"라고 적었지만, `FDataTableRowHandle::DataTable` 의 강참조가 지키는 것은 `UDataTable` 객체뿐이고 행 메모리 블록이 아니다. 행 실체는 `UDataTable::RowMap` 이 직접 잡은 버퍼이며 CSV 재임포트·로우 구조체 변경(`EmptyTable`/`CleanBeforeStructChange`)에서 통째로 해제·재할당된다. PIE 중 대화 테이블을 재임포트하면 `CurrentRow` 가 해제된 메모리를 가리킨 채 남고, 다음 `Advance()` 의 `CurrentRow->NextDialogue`(`:60`)에서 크래시한다. 쿠킹 빌드엔 이 경로가 없어 에디터 워크플로 한정이지만, 데이터 테이블을 PIE 중 만지는 것은 흔한 작업이다.
- **제안**: 포인터 대신 `CurrentRowName` 만 진실로 두고 읽는 자리마다 다시 조회한다(`CurrentStartRow.DataTable->FindRow<FWxDialogueTableRow>(CurrentRowName, ...)`). 대사 넘김이 프레임당 1회 미만이라 TMap 조회 비용은 사실상 없고, `HasActiveDialogue()` 는 `CurrentRowName.IsNone()` 으로 대체된다.
- **확신도**: 중간(런타임 실패 경로는 없고 에디터 편집 중에만 드러난다)

### 3. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:268-277`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로, 액터에 스켈레탈 메시가 둘 이상이면 어느 것이 나올지 보장되지 않는다. `AWxNpc` 는 몸통 메시를 `MeshComponent` 로 이미 명시해 두었는데(`Public/WxNpc.h:49-50`) 이 경로는 그것을 쓰지 않는다. NPC BP 가 무기·머리카락 같은 스켈레탈 메시를 하나만 더 붙여도 포즈가 엉뚱한 메시로 가고, 그쪽에 애님 인스턴스가 없으면 경고만 남긴 채 포즈가 사라진다.
- **제안**: 대상이 `AWxNpc`(또는 `ACharacter`)면 그 액터가 지정한 몸통 메시를 쓰고, 그 밖의 일반 액터에서만 `FindComponentByClass` 로 폴백한다.
- **확신도**: 높음

### 4. 🟡 대화 행이 포즈 몽타주를 하드 참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h:31-32`
- **범주**: 성능/안전
- **문제**: `TObjectPtr<UAnimMontage> TargetPose` 는 하드 참조라, 대화 테이블이 로드되는 순간 그 테이블 모든 행의 몽타주(와 그것이 끌고 오는 스켈레톤·애님 시퀀스)가 함께 로드된다. 테이블은 배치된 NPC 의 `UWxDialogueComponent::StartRow` 핸들이 하드로 붙잡고 있으므로(`Public/WxDialogueComponent.h:25-26`) 레벨 로드와 함께 상주한다. 즉 "말을 걸기 전부터 그 테이블의 모든 대화 연출 애셋이 메모리에 있다"가 되며, 대화 편수와 포즈 종류에 비례해 커지는 종류의 비용이다.
- **제안**: `TSoftObjectPtr<UAnimMontage>` 로 바꾸고 `ApplyCurrentPose` 에서 스트리밍한다(어빌리티 아이콘이 이미 쓰는 방식). 최소한 대화 테이블을 레벨 상주 하드 참조에서 떼어낸다.
- **확신도**: 중간(v1 규모에서는 실측 문제가 아닐 수 있다)

### 5. 🟢 "빈 대사 = 정상 종료"라는 문서와 "빈 대사 = 경고·실패"라는 코드가 어긋난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h:24-26` vs `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:152-157`
- **범주**: 중복/복잡도
- **문제**: 행 구조체 주석과 README(`Plugins/WxDialogue/README.md:33`)는 "대사가 비면 대화가 종료된다"를 정상 관용구로 안내한다. 그런데 `EnterRow` 는 빈 대사를 실패로 보고 Warning 을 남기며, 그 실패는 `Advance` 에서 "다음 행을 해석하지 못해"라는 두 번째 Warning 을 부른다(`:67-74`). 시작 행이 그런 행이면 `Play Dialogue` 태스크가 아예 Failed 를 낸다(`Private/WxDialogueStateTreeNodes.cpp:134-138`). 동작은 "종료"로 수렴하므로 버그는 아니지만, 안내대로 만든 데이터가 매번 경고 두 줄을 찍는다.
- **제안**: 빈 대사를 정상 종료로 인정해 경고에서 빼거나, 반대로 문서에서 그 관용구를 지우고 `NextDialogue=None` 하나로 종료를 통일한다.
- **확신도**: 높음

### 6. 🟢 `AWxNpc::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp:49-60`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다. 세션 주입은 Experience 비동기 상태머신에 달려 있어 타이밍에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 지난 리뷰 이후 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리됐으므로(`Private/WxDialogueSessionComponent.cpp:32`, `:45`, `:70`, `:155`) 여기만 남은 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 7. 🟢 카메라 경로에서 엔진 포인터를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:211`, `:252`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()` 은 카메라 매니저를 검사 없이 역참조한다. 로컬 PC 라면 사실상 항상 유효하지만, 같은 함수의 다른 입력(폰·대상)은 모두 검사를 거치고 있어 여기만 예외다. `EndDialogueCamera` 의 `PlayerController->GetPawn()` 도 null 일 수 있고(대화 중 사망 후 폰 소멸), 그때 뷰 타겟이 컨트롤러 자신으로 떨어져 구도가 튄다.
- **제안**: 카메라 매니저 null 검사를 더하고, 복귀 뷰 타겟이 null 이면 뷰 전환을 건너뛴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`·`WxActorTarget.h`) — 리뷰 대상은 아니다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `ClientStartDialogue` 가 동기 실행되는 것에 기대는 `Play Dialogue`(`Private/WxDialogueStateTreeNodes.cpp:131-138`)와 0번 컨트롤러 폴링은 데디케이티드·원격 클라에서 깨지지만, 세 곳의 주석이 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견에서 뺐다. 실측은 하지 않았다.
  - `Public/WxDialogueStateTreeNodes.h:69`, `:107`, `:148` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지) 위반이나, 같은 파일 14행이 엔진 StateTree 관례를 근거로 예외임을 명시하고 있어 발견으로 올리지 않았다(지난 리뷰가 지적한 사소 게터 두 건은 cpp 로 이관돼 해소).
  - 에셋 내부는 범위 밖이라 `.claude/asset_dump` 로 `WBP_DialogueScreen` 의 `bPauseGame=false`·`bIsBackHandler=false` 만 확인했고 바인딩 구성은 보지 않았다. `DT_Dialogue` 의 실제 행 조립도 검토하지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - `AWxNpc` 가 영역 메시의 응답을 전부 Ignore 로 두는 구성(`Private/WxNpc.cpp:34-36`)이 상호작용 스캐너의 오버랩 쿼리 방식과 맞물리는지는 확인하지 않았다 — WxCore/스캐너 리뷰의 몫이다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 11파일 — `/module-review`로 갱신*
