# WxDialogue — 코드 리뷰

> 9개 소스의 작고 응집도 높은 모듈이다. 코딩·모듈 의존 규칙 위반 0건, 심각(🔴) 0건이며, 실패 갈래마다 경고 로그와 근거 주석이 붙어 있어 읽기 좋다. 남은 지적은 전부 "세션이 자기 데이터를 끝까지 진행한다"는 단일 전제가 깨지는 자리(외부 종료·폰 교체·뷰 타겟 경합)와 대상 해석의 느슨함에 몰려 있다. 커버리지: `.Build.cs`·`.uplugin`·README 포함 소스 9개를 전부 읽었고 세션 컴포넌트(h/cpp)와 StateTree 노드 3종을 정독했으며, 발현 확인용으로 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`·`IWxInteractable`·`AWxNpc`·`UWxMetaHumanVisualComponent`·`UWxAnimNotifyState_CameraMove`·`WxIndicatorStateTreeNodes`)까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 세션을 밖에서 접을 수단도 수명 훅도 없다 — 대화 창이 닫혀도 세션은 열린 채 남는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:50-70`(공개 API), `:127`(`EndDialogue` private), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:199-216`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:213-219`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 경로는 `Advance()` 가 `NextRow == None` 인 행에 닿는 것 하나뿐이다. `EndDialogue()` 는 private(`h:127`)이고 공개 API(`h:50-70`)에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`·`UninitializeComponent`·빙의 변경 훅이 하나도 없다(헤더 전체에 해당 override 선언 0건). 세션은 "누군가 끝까지 넘겨 준다"만 상정한다.

  가장 확실한 발현은 대화 중 빙의 변경이다. UI 매니저가 `OnPossessedPawnChanged` 를 받아(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:267-269`) `WatchPawnTags` 에서 대화 창을 먼저 닫는데(`:296-297`), 이때 세션에는 아무 신호도 가지 않는다. 창이 사라지면 `Advance()` 를 부를 유일한 주체(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:44-50`)가 없어지므로 `CurrentRowName` 이 영원히 남고, `FWxStateTreeTask_PlayDialogue::Tick`(`Private/WxDialogueStateTreeNodes.cpp:218`)이 영구 `Running` 이 되어 그 퀘스트 단계가 완료되지 않는다.

  두 번째 발현은 대화 중 사망이다. 플레이어 폰은 사망해도 파괴되지 않고 래그돌로 남으므로(`Source/WxGame/Character/WxCharacterBase.cpp:207-213` 의 `HandleDeathTagChanged` → `HandleDeath`, 파일 전체에 액터 파괴 없음) 폰 ASC 의 `State.Dialogue`(`cpp:150`)가 1 로 고정되고, 그 태그를 차단 태그로 쓰는 상호작용 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:33`)가 그 폰에서 계속 막힌다. 다만 사망 갈래는 부활이 월드 리로드라 리로드가 상태를 함께 걷어내므로(같은 파일 `:321` 의 근거 주석) 실질 피해는 첫 번째 갈래에 몰린다. 부수적으로 대화 카메라도 `SetLifeSpan`(`cpp:284`)이 `EndDialogueCamera` 안에만 있어 컨트롤러가 사라질 때까지 남는다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged` 와 `EndPlay`/`UninitializeComponent` 에서 활성 세션을 접도록 한다. 빙의 변경 한 갈래만 막아도 위 하드행은 사라진다.
- **확신도**: 중간(코드 경로는 확정. 대화 중 빙의 변경·사망이 실제로 얼마나 나는지는 콘텐츠 배치에 달렸다)

### 2. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다 — 메타휴먼 얼굴에 얹히면 조용히 어긋난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:337`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로 액터에 스켈레탈 메시가 둘 이상이면 무엇이 나올지 계약이 없다(TSet 순회는 삽입 순서가 아니라 해시 순서다). 그런데 대표 대상인 `AWxNpc` 는 생성자에서 `UWxMetaHumanVisualComponent` 를 항상 붙이고(`Source/WxGame/Character/WxNpc.cpp:41`), 그 컴포넌트가 등록 시점에 Face·Outfit 스켈레탈 메시를 `NewObject` 로 만들어 몸통에 붙인다(`Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:45`, `:52`, `:66`, `:71`). Face 는 `SetAnimInstanceClass` 로 자기 애님 인스턴스를 갖고 있어(`:49`) 그쪽이 잡히면 `cpp:339-345` 의 "애님 인스턴스 없음" 경고조차 뜨지 않는다 — 몽타주는 얼굴에서 재생되고 몸은 그대로인 **무경고 오동작**이다.

  대상 액터는 몸통 메시를 이미 명시해 두었고(`AWxNpc::MeshComponent`, 대화 컴포넌트의 `AreaMesh` 로도 넘어간다 — `WxNpc.cpp:39`) 이 경로만 그것을 쓰지 않는다.
- **제안**: `UWxDialogueComponent` 가 이미 들고 있는 영역 메시를 getter 로 열어 쓰거나(또는 전용 포즈 메시 지정을 추가), `ACharacter` 면 `GetMesh()` 를 우선하고 그 밖의 액터에서만 `FindComponentByClass` 로 폴백한다.
- **확신도**: 중간(오동작 경로는 확정, 현재 삽입 순서에서 항상 드러나는지는 미확인)

### 3. 🟢 폰 ASC 를 못 찾으면 태그 없이 세션이 열려, 창도 안 뜨고 접히지도 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:144-152`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 폰 ASC 를 찾지 못하면 `State.Dialogue` 를 올리지 못한 채 그냥 지나간다 — 실패로 보지도, 로그를 남기지도 않는다. 대화 창을 띄우는 유일한 신호가 그 태그이므로(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:325-336`), 이 갈래에서는 세션만 열리고 창이 뜨지 않는다. `Advance()` 를 부를 뷰가 없으니 곧바로 발견 1 의 고착(`Play Dialogue` 영구 Running)으로 직행하고, 증상은 "퀘스트가 그냥 멈춰 있다" 하나로만 남는다. 폰이 아직 없거나 ASC 없는 폰에 빙의한 순간 `Play Dialogue` 가 도는 조립에서 실재한다.
- **제안**: ASC 부재 갈래에 `LogWxDialogue` Warning 을 남기고 세션을 열지 않은 채 되돌린다(창 없는 대화는 어차피 진행이 불가능하다).
- **확신도**: 중간(코드 경로는 확정, 발현 빈도는 콘텐츠 배치에 달렸다)

### 4. 🟢 대화 카메라가 이전 뷰 타겟을 기억하지 않고 무조건 폰으로 되돌린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:265`, `:280`
- **범주**: 설계/구조
- **문제**: `BeginDialogueCamera` 는 진입 직전의 뷰 타겟을 남기지 않고, `EndDialogueCamera` 는 항상 `PlayerController->GetPawn()` 으로 복귀한다. 저장소에서 뷰 타겟을 바꾸는 다른 시스템인 `UWxAnimNotifyState_CameraMove` 도 같은 방식이라(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:71`, `:184`), 둘이 겹치면 먼저 끝나는 쪽이 상대의 카메라를 폰으로 걷어낸다. 겹침은 가정이 아니라 이 모듈이 명시한 용례다 — `Play Dialogue` 의 대표 용도로 "처치 후 대사"를 들고 있고(`Public/WxDialogueStateTreeNodes.h:26`), 피니셔·적 패턴 몽타주의 CameraMove ANS 가 아직 재생 중일 수 있다. 그때 ANS 의 `NotifyEnd` 가 대화 카메라에서 뷰를 뺏어 대화 내내 게임플레이 구도로 남는다.
- **제안**: 진입 시 `PlayerController->GetViewTarget()` 을 기억해 종료 시 그리로(사라졌으면 폰으로) 되돌린다. 근본적으로는 뷰 타겟 소유권을 중재하는 자리가 필요하지만 최소 수정은 저장·복원이다.
- **확신도**: 중간(코드 경로는 확정, 두 연출이 겹치는 실제 배치는 확인하지 못했다)

### 5. 🟢 `FindTargetDialogue` 의 대상 해석에 `IsValid` 가드가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:23-27`, 비교 지점 `:46`
- **범주**: 버그/정확성
- **문제**: `Cast<AActor>(Locator.SyncFind(Owner))` 는 파괴 대기(pending kill) 액터를 걸러 내지 않는다. UOL 액터 프래그먼트는 소프트 경로 조회라 `Destroy()` 와 GC 사이의 액터를 여전히 답할 수 있고, WP 스트리밍 아웃에 그 창이 존재한다. `AppliedTargets` 는 `TWeakObjectPtr` 라 파괴 대기 객체에 null 을 답하므로 `:46` 의 동일성 비교가 항상 어긋나, 그 창 동안 매 틱 파괴 대기 컴포넌트에 `SetInteractionEnabled` 를 호출한다. 같은 패턴의 형제 노드는 이 갈래를 명시적으로 막아 두었다 — `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:18-22` 의 `ResolveTargetActor` 는 `IsValid(Target) ? Target : nullptr` 다.
- **제안**: 해석 결과에 `IsValid` 를 씌워 형제 노드와 동작을 맞춘다.
- **확신도**: 중간(크래시로 이어지진 않지만 무의미한 반복 호출이고, 저장소 선례와 어긋난다)

### 6. 🟢 `UWxDialogueComponent::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:27-38`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다. 세션 주입은 Experience 의 Add Components 에 달려 있어 타이밍·구성에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리돼 있어(`Private/WxDialogueSessionComponent.cpp:34`, `:47`, `:80`, `:164`, `:172`, `:332`, `:342`) 이 모듈에서 여기만 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 7. 🟢 `Enable Npc Interaction` 진입 시 대상 하나를 세 번 해석한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:262-269`, `:273`
- **범주**: 중복/복잡도
- **문제**: 경고 루프가 대상마다 `Locator.SyncFind`(`:264`)를 한 번, 이어서 `FindTargetDialogue`(`:265`)가 같은 로케이터를 다시 한 번 해석하고, 그 직후 `RefreshNpcInteraction`(`:273`)이 또 한 번 해석한다. 진입 1회에 대상당 `SyncFind` 3회다. 매 틱 재해석은 스트리밍 재로드 대응이라는 근거가 헤더에 있지만(`Public/WxDialogueStateTreeNodes.h:147`), 진입 경로의 이 3중 해석은 순전히 경고 로직이 해석 결과를 버리는 데서 온다.
- **제안**: 경고 루프에서 `FindTargetDialogue` 결과를 한 번만 구해 판정하고 그 결과를 `RefreshNpcInteraction` 의 첫 적용과 공유한다(또는 경고를 `RefreshNpcInteraction` 안에서 진입 1회 플래그로 낸다).
- **확신도**: 높음

### 8. 🟢 카메라 경로에서 엔진 포인터를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:239`, `:280`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()`(`:239`)은 카메라 매니저를 검사 없이 역참조한다. 로컬 PC 라면 사실상 항상 유효하지만, 같은 함수의 다른 입력(폰 `:222`, 대상 `:223`, 스폰 결과 `:252`)은 모두 검사를 거치고 있어 여기만 예외다. `EndDialogueCamera` 의 `PlayerController->GetPawn()`(`:280`)도 null 일 수 있고(대화 도중 폰 소멸), 그때 엔진이 뷰 타겟을 컨트롤러 자신으로 떨어뜨려 구도가 튄다.
- **제안**: 카메라 매니저 null 검사를 더하고, 복귀 뷰 타겟이 null 이면 뷰 전환을 건너뛴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **규칙 점검 결과**: 모듈 전역 확인 — `Wx` prefix 준수, 소스 9개 + `.Build.cs` 전부 저작권 첫 줄 존재, 람다 0건, `BlueprintCallable` 0건, `FORCEINLINE` 0건, 델리게이트 콜백 `HandlePoseLoaded` 의 `Handle` prefix 준수, `Super::` 호출이 필요한 override 는 생성자(`cpp:23`)뿐이며 준수. 모듈 의존은 `WxCore` 외 Wx 플러그인 없음(`WxDialogue.Build.cs`·`WxDialogue.uplugin` 모두) ✅.
- **지난 리뷰 대비 변화**: 없음. `Plugins/WxDialogue` 는 지난 리뷰 기준 커밋(`f7620119`)과 이번 기준 커밋 사이에 한 줄도 바뀌지 않았고(그 구간 변경은 `Source/WxGame` 의 캐릭터 무브먼트·플레이어 캐릭터와 콘텐츠·문서뿐), 발견 1~8 은 이번에 근거를 처음부터 다시 확인해 모두 재현했다. 발견 1 은 이번에 빙의 변경 갈래(`WxUIManagerSubsystem` 의 `HandlePossessedPawnChanged` → `CloseDialogueScreen`)를 확인해 발현 경로를 사망 위주에서 그쪽으로 옮겨 적었고, 사망 갈래는 월드 리로드로 회복된다는 점을 반영해 확신도를 낮췄다.
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Plugins/WxUI/.../WxIndicatorStateTreeNodes.cpp`, `Plugins/WxCombat/.../WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCore/.../WxInteractable.cpp`, `Source/WxGame/...`) — 리뷰 대상은 아니다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `ClientStartDialogue` 가 동기 실행되는 것에 기대는 `Play Dialogue`(`Private/WxDialogueStateTreeNodes.cpp:201-208`)와 0번 컨트롤러 폴링(`:18`)은 데디케이티드·원격 클라에서 깨지지만, 헤더·README·세 노드 주석이 모두 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견에서 뺐다. 멀티 확장 시 제일 먼저 손댈 지점이다.
  - `Public/WxDialogueStateTreeNodes.h:71`, `:109`, `:162` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지)과 형식상 충돌하나, 같은 파일 15행이 엔진 StateTree 관례를 근거로 예외임을 명시했고 저장소 내 다른 모듈의 ST 노드가 전부 같은 형태라 발견으로 올리지 않았다. `Private/WxDialogueStateTreeNodes.cpp:14-110` 의 익명 namespace 헬퍼도 다른 플러그인 cpp 12개와 동일한 관례라 같은 이유로 제외했다.
  - `FWxStateTreeTask_EnableNpcInteraction::Tick` 이 매 틱 대상마다 `SyncFind` + `FindComponentByClass` 를 도는 비용은 실측하지 않았다. 헤더(`:147-149`)가 스트리밍 재로드 대응을 근거로 명시한 의도이고 대상 수가 적어 발견으로 올리지 않았다(진입 경로의 중복 해석만 발견 7 로 올렸다).
  - 같은 NPC 를 서로 다른 상태의 `Enable Npc Interaction` 이 반대 값으로 잡을 때 기록(`AppliedTargets`)이 컴포넌트 동일성 기준이라 나중에 덮인 값을 되찾지 않는 점을 따라갔다. 다만 `UWxQuestComponent` 가 `bHasActiveQuest` 로 단일 활성 퀘스트를 전제하고 있어(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:107`) 동시 충돌 조립이 실재하는지 확인하지 못해 발견으로 올리지 않았다.
  - `Wait Dialogue Completed` 가 진입 시점에 이미 지정 행이 표시 중이면 그 대화로 게이트를 통과하는 경로를 따라갔다. 헤더(`:56`)의 "즉시 통과 없음" 서술과 형식상 어긋나지만, 대화가 열려 있는 동안 그 상태로 전이시키는 조립을 찾지 못해 발견으로 올리지 않았다.
  - 포즈 스트리밍 중 새 대화가 열려 앞 대사의 포즈 요청이 `ApplyCurrentPose`(`Private/WxDialogueSessionComponent.cpp:298-302`)의 `CancelHandle` 에 걷히는 경로도 따라갔다. `EndDialogue`(`:213-214`)의 "마지막 포즈는 늦게 도착해도 얹힌다"는 보증과 어긋나지만, 몽타주 한 장의 스트리밍 창 안에 다음 대화가 들어와야 하는 매우 좁은 레이스라 발견으로 올리지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정, `:228-244`)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - 에셋 내부(`DT_Dialogue` 행 링크, `BP_Npc` 컴포넌트 구성, `WBP_DialogueScreen` 의 정지·입력 모드)는 범위 밖이다 — 발견 1 의 "대화 중 피격·빙의 변경 가능" 전제는 코드로만 판단했다.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 9파일 — `/module-review`로 갱신*
