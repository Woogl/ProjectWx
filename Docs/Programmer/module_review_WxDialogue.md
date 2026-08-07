# WxDialogue — 코드 리뷰

> 11개 소스의 작고 응집도 높은 모듈이다. 지난 리뷰에서 지적한 행 포인터 캐시와 포즈 하드 참조는 각각 `FindCurrentRow()` 재조회와 `TSoftObjectPtr` 스트리밍으로 정리됐고, 코딩·모듈 의존 규칙 위반은 0건이며 심각(🔴) 등급도 없다 — 남은 지적은 세션 수명 관리와 카메라·메시 해석 방식에 몰려 있다. 커버리지: `.Build.cs`·`.uplugin` 포함 소스 11개를 모두 읽었고 세션 컴포넌트 cpp/h 와 StateTree 노드 3종을 정독했으며, 발현 확인용으로 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`·`UWxInteractionScannerComponent`·`UWxMetaHumanVisualComponent`·`UWxAnimNotifyState_CameraMove`)와 `DT_Dialogue`·`BP_Npc`·`WBP_DialogueScreen` 덤프, UE 5.8 `FStateTreeTaskBase` 플래그 기본값까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 세션을 밖에서 접을 방법도, 수명 훅도 없어 대화 중 사망하면 세션이 영구히 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:128`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:122-129`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 유일한 경로는 `Advance()` 가 `NextRow == None` 인 행에 닿는 것뿐이다. `EndDialogue()` 는 private 이고 공개 API 에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`/`UninitializeComponent`/빙의 변경 훅이 하나도 없다(헤더 `:41-176` 전체에 해당 override 선언 0건). 즉 세션은 자기 데이터가 끝까지 진행되는 것만 상정한다.

  그런데 대화 창은 `bPauseGame=false`·`inputMode=MENU` 다(`.claude/asset_dump/Widgets/WBP_DialogueScreen.json`). 월드는 계속 돌고 플레이어는 이동·회피 입력을 잃은 채 서 있으므로 대화 중 피격·사망이 실재하는 갈래다. 사망하면 `State.Dead` 로 사망 화면이 Menu 레이어에 올라오고(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:318`) 대화 창은 그 아래 남지만 입력이 위 레이어로 가 `Advance()` 를 부를 수단이 사라진다. 폰은 파괴되지 않으므로 `State.Dialogue` 는 폰 ASC 에 1 로 고정되고, 그 태그를 차단 태그로 쓰는 상호작용 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)가 영구 차단된다. 부수적으로 `FWxStateTreeTask_PlayDialogue::Tick`(`Private/WxDialogueStateTreeNodes.cpp:166-172`)은 영구 Running, `FWxStateTreeTask_WaitDialogueCompleted::Tick`(`:100-112`)은 `bObservedDialogue=true` 인 채 대기하다 **무관한 다음 대화**가 끝나는 순간 Succeeded 를 내며, 대화 카메라는 `SetLifeSpan` 을 못 받아(`Private/WxDialogueSessionComponent.cpp:285` 은 `EndDialogueCamera` 안에만 있다) 컨트롤러가 사라질 때까지 남는다.

  회복은 다음 대화가 열릴 때 `ClientStartDialogue` 의 선행 `EndDialogue()`(`:126-129`)가 우연히 치워 주는 것뿐인데, 상호작용이 태그로 막혀 있어 그 다음 대화를 플레이어가 열 수 없다. 실질 탈출구는 세이브 리로드(월드 트래블)뿐이다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged`·`EndPlay`/`UninitializeComponent` 와 폰 ASC 의 `State.Dead` 에서 활성 세션을 접도록 한다. 최소한 사망 한 갈래만이라도 세션을 접으면 영구 차단이 사라진다.
- **확신도**: 높음(경로는 코드로 확정. 사망 빈도는 콘텐츠 배치에 달렸다)

### 2. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:338`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로, 액터에 스켈레탈 메시가 둘 이상이면 어느 것이 나올지 보장되지 않는다. `AWxNpc` 는 몸통 메시를 `MeshComponent` 로 이미 명시해 두었는데(`Public/WxNpc.h:49-50`) 이 경로는 그것을 쓰지 않는다.

  현재 `BP_Npc` 는 스켈레탈 메시가 하나뿐이라(`.claude/asset_dump/Blueprints/BP_Npc.json` 의 컴포넌트 목록 = Capsule / MeshComponent / TextRender / DialogueComponent) 아직 드러나지 않지만, 이 프로젝트에는 캐릭터에 스켈레탈 메시를 런타임에 더 다는 컴포넌트가 이미 있다 — `UWxMetaHumanVisualComponent` 가 등록 시점에 Face·Outfit 메시를 생성해 몸통에 붙인다(`Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:45`, `:66`). 그 컴포넌트의 `ResolveBodyMesh` 는 오너가 `ACharacter` 가 아니면 같은 `FindComponentByClass` 폴백을 쓰므로(`:196`) `AWxNpc` 계열까지 겨냥한 물건이다. 붙는 순간 포즈가 Face 로 갈 수 있고, Face 는 `SetAnimInstanceClass` 로 애님 인스턴스를 갖고 있어(`:48`) `:340-346` 의 경고조차 뜨지 않는 **조용한 오동작**이 된다.
- **제안**: 대상이 `AWxNpc`(또는 `ACharacter`)면 그 액터가 지정한 몸통 메시를 쓰고, 그 밖의 일반 액터에서만 `FindComponentByClass` 로 폴백한다.
- **확신도**: 높음

### 3. 🟢 폰 ASC 를 못 찾으면 태그 없이 세션이 열려, 창도 안 뜨고 접히지도 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:145-153`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 폰 ASC 를 찾지 못하면 `State.Dialogue` 를 올리지 못한 채 그냥 지나간다 — 실패로 보지도, 로그를 남기지도 않는다. 그런데 대화 창을 띄우는 유일한 신호가 그 태그이므로(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:321-331`), 이 갈래에서는 세션만 열리고 창이 뜨지 않는다. `Advance()` 를 부를 뷰(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:44-50`)가 없으니 발견 1과 같은 고착 상태로 직행하고(`Play Dialogue` 는 영구 Running), 증상은 "퀘스트가 그냥 멈춰 있다" 하나로만 남는다. 폰이 없거나 ASC 가 없는 폰에 빙의한 순간 `Play Dialogue` 가 도는 조립에서 실재한다.
- **제안**: ASC 부재 갈래에 `LogWxDialogue` Warning 을 남기고, 세션을 열지 않고 되돌린다(창 없는 대화는 어차피 진행이 불가능하다).
- **확신도**: 중간(코드 경로는 확정, 발현 빈도는 콘텐츠 배치에 달렸다)

### 4. 🟢 `AWxNpc::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp:49-57`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다. 세션 주입은 Experience 의 Add Components 에 달려 있어 타이밍·구성에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리돼 있으므로(`Private/WxDialogueSessionComponent.cpp:34`, `:47`, `:80`, `:165`, `:173`, `:333`, `:343`) 이 모듈에서 여기만 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 5. 🟢 카메라 경로에서 엔진 포인터를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:240`, `:281`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()`(`:240`)은 카메라 매니저를 검사 없이 역참조한다. 로컬 PC 라면 사실상 항상 유효하지만, 같은 함수의 다른 입력(폰 `:223`, 대상 `:224`, 스폰 결과 `:253`)은 모두 검사를 거치고 있어 여기만 예외다. `EndDialogueCamera` 의 `PlayerController->GetPawn()`(`:281`)도 null 일 수 있고(대화 도중 폰 소멸), 그때 엔진이 뷰 타겟을 컨트롤러 자신으로 떨어뜨려 구도가 튄다.
- **제안**: 카메라 매니저 null 검사를 더하고, 복귀 뷰 타겟이 null 이면 뷰 전환을 건너뛴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 대화 카메라가 이전 뷰 타겟을 기억하지 않고 무조건 폰으로 되돌린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:266`, `:281`
- **범주**: 설계/구조
- **문제**: `BeginDialogueCamera` 는 진입 직전의 뷰 타겟을 남기지 않고, `EndDialogueCamera` 는 항상 `PlayerController->GetPawn()` 으로 복귀한다. 저장소에서 뷰 타겟을 바꾸는 다른 시스템은 `UWxAnimNotifyState_CameraMove` 하나인데(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:74`, `:193`) 그쪽도 같은 방식(복귀 대상 = `PC->GetPawn()`)이라, 둘이 겹치면 먼저 끝나는 쪽이 상대의 카메라를 폰으로 걷어낸다. 겹침은 가정이 아니라 이 모듈이 명시한 용례다 — `Play Dialogue` 의 대표 용도가 "처치 후 대사"이고(`Public/WxDialogueStateTreeNodes.h:26`), 피니셔·적 패턴 몽타주에 걸린 CameraMove ANS 가 아직 재생 중일 수 있다. 그때 ANS 의 `NotifyEnd` 가 대화 카메라에서 뷰를 뺏어 대화 내내 게임플레이 구도로 남는다.
- **제안**: 진입 시 `PlayerController->GetViewTarget()` 을 기억해 종료 시 그리로(사라졌으면 폰으로) 되돌린다. 근본적으로는 뷰 타겟 소유권을 중재하는 자리가 필요하지만, 최소 수정은 저장·복원이다.
- **확신도**: 중간(코드 경로는 확정, 두 연출이 실제로 겹치는 배치는 확인하지 못했다)

### 7. 🟢 `RefreshNpcInteraction` 의 대상 해석에 `IsValid` 가드가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:28`
- **범주**: 버그/정확성
- **문제**: `Cast<AWxNpc>(Instance.Target.Locator.SyncFind(...))` 는 파괴 대기(pending kill) 액터를 걸러 내지 않는다. UOL 액터 프래그먼트는 소프트 경로 조회라 `Destroy()` 와 GC 사이의 액터를 여전히 답할 수 있고, WP 스트리밍 아웃에는 그 창이 존재한다. `Instance.AppliedNpc` 는 `TWeakObjectPtr` 라 파괴 대기 객체에 null 을 답하므로 `:36` 의 동일성 비교가 항상 어긋나, 그 창 동안 매 틱 파괴 대기 NPC 에 `SetInteractionEnabled` 를 호출한다. 같은 패턴의 형제 노드는 이 갈래를 명시적으로 막아 두었다 — `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:19-20` 의 `ResolveTargetActor` 는 `IsValid(Target) ? Target : nullptr` 다.
- **제안**: 해석 결과에 `IsValid` 를 씌워 형제 노드와 동작을 맞춘다.
- **확신도**: 중간(크래시로 이어지진 않지만 무의미한 반복 호출이고, 사내 선례와 어긋난다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **규칙 점검 결과**: 모듈 전역 grep 으로 확인 — `Wx` prefix 준수, 11개 소스 + `.Build.cs` 전부 저작권 첫 줄 존재, 람다 0건, `BlueprintCallable` 0건, `FORCEINLINE` 0건, 델리게이트 콜백 `HandlePoseLoaded` 의 `Handle` prefix 준수. 모듈 의존은 `WxCore` 외 Wx 플러그인 없음(`WxDialogue.Build.cs`·`WxDialogue.uplugin` 모두) ✅.
- **지난 리뷰 대비 해소 확인**: 행 메모리 원시 포인터 캐시 → `CurrentRowName` + `FindCurrentRow()` 매번 재조회로 대체(`Private/WxDialogueSessionComponent.cpp:183-193`). `TargetPose` 하드 참조 → `TSoftObjectPtr` + `RequestAsyncLoad` 스트리밍(`Public/WxDialogueTableRow.h:35`, `Private/WxDialogueSessionComponent.cpp:288-326`). "빈 대사 = 정상 종료" 문서/코드 불일치 → 문서를 코드에 맞춰 정정(`Public/WxDialogueTableRow.h:24`, `README.md:34`).
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Plugins/WxUI/.../WxIndicatorStateTreeNodes.cpp`, `Plugins/WxCombat/.../WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`) — 리뷰 대상은 아니다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `ClientStartDialogue` 가 동기 실행되는 것에 기대는 `Play Dialogue`(`Private/WxDialogueStateTreeNodes.cpp:154-161`)와 0번 컨트롤러 폴링(`:18`)은 데디케이티드·원격 클라에서 깨지지만, 헤더·README·세 노드 주석이 모두 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견에서 뺐다. 멀티 확장 시 제일 먼저 손댈 지점이다.
  - `Public/WxDialogueStateTreeNodes.h:70`, `:108`, `:156` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지)과 형식상 충돌하나, 같은 파일 15행이 엔진 StateTree 관례를 근거로 예외임을 명시했고 저장소 내 다른 모듈의 ST 노드가 모두 같은 형태라 발견으로 올리지 않았다. `Private/WxDialogueStateTreeNodes.cpp:13-63` 의 익명 namespace 헬퍼도 다른 도메인 ST 노드 cpp 와 동일한 관례라 같은 이유로 제외했다.
  - `FWxStateTreeTask_EnableNpcInteraction::Tick` 이 매 틱 `Locator.SyncFind` 로 대상을 재해석하는 비용은 실측하지 않았다. 헤더(`:142-143`)가 스트리밍 재로드 대응을 근거로 명시한 의도이고 인스턴스 수가 적어 발견으로 올리지 않았다.
  - 포즈 스트리밍 중 새 대화가 열려 앞 대사의 포즈 요청이 `ApplyCurrentPose`(`Private/WxDialogueSessionComponent.cpp:299-303`)의 `CancelHandle` 에 걷히는 경로를 따라갔다. `EndDialogue`(`:214-215`)의 "마지막 포즈는 늦게 도착해도 얹힌다"는 보증과 어긋나지만, 몽타주 한 장의 스트리밍 창 안에 다음 대화가 들어와야 하는 매우 좁은 레이스라 발견으로 올리지 않았다.
  - `FWxStateTreeTask_WaitDialogueCompleted::EnterState`(`:73-87`)가 행 미지정 시 경고만 남기고 Running 으로 남는 것은 같은 오조립에 Failed 를 내는 `Play Dialogue`(`:140-144`)와 다르지만, 헤더가 명시한 의도라 발견에서 뺐다.
  - 대화가 겹쳐 열릴 때(`Private/WxDialogueSessionComponent.cpp:126`) 한 프레임에 `State.Dialogue` 가 1→0→1 로 튀며 위젯이 닫히고 다시 push 되는 경로를 따라갔으나, `PushSoftContentToLayer` 가 `LoadSynchronous` 동기 경로라 뷰모델 시드와 최종 뷰 타겟이 모두 의도대로 수렴해 발견으로 올리지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정, `:229-245`)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - 에셋 내부는 범위 밖이라 `.claude/asset_dump` 로 `DT_Dialogue` 의 행 링크·`AM_Death` 참조, `BP_Npc` 컴포넌트 목록, `WBP_DialogueScreen` 의 `bPauseGame`·`inputMode` 만 확인했고 위젯 바인딩 구성은 보지 않았다.

---
*문서 기준 커밋 `18f580a2` · 리뷰일 2026-08-07 · 소스 11파일 — `/module-review`로 갱신*
