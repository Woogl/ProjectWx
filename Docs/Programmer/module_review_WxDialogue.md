# WxDialogue — 코드 리뷰

> 9개 소스의 작고 응집도 높은 모듈이다. 코딩·모듈 의존 규칙 위반은 0건이고 심각(🔴) 등급도 없다 — 남은 지적은 지난 리뷰와 같은 자리(세션 수명 관리·카메라·메시 해석)에 그대로 몰려 있고, 그 사이 `AWxNpc` 가 게임 모듈로 옮겨가며 메타휴먼 부착물을 기본 탑재해 포즈 메시 해석 건은 발현 확률이 올라갔다. 커버리지: `.Build.cs`·`.uplugin`·README 포함 소스 9개를 모두 읽었고 세션 컴포넌트(cpp/h)와 StateTree 노드 3종을 정독했으며, 발현 확인용으로 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`·`UWxInteractionScannerComponent`·`AWxNpc`·`UWxMetaHumanVisualComponent`·`UWxAnimNotifyState_CameraMove`·`WxIndicatorStateTreeNodes`)까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 세션을 밖에서 접을 방법도 수명 훅도 없어, 대화 중 사망하면 State.Dialogue 가 고착된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:50-70`, `:127`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:125-128`, `:199-216`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 유일한 경로는 `Advance()` 가 `NextRow == None` 인 행에 닿는 것뿐이다. `EndDialogue()` 는 private(`h:127`)이고 공개 API(`h:50-70`)에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`·`UninitializeComponent`·빙의 변경 훅이 하나도 없다(헤더 전체에 해당 override 선언 0건). 즉 세션은 자기 데이터가 끝까지 진행되는 것만 상정한다.

  플레이어 폰은 사망해도 파괴되지 않고 래그돌로 남으므로(`Source/WxGame/Character/WxCharacterBase.cpp:207-213`, `:271-` 의 `HandleDeath` 는 액터를 파괴하지 않는다) 폰 ASC 에 올려 둔 `State.Dialogue`(`cpp:150`)가 1 로 고정되고, 그 태그를 차단 태그로 쓰는 상호작용 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:33`)가 영구 차단된다. 사망 화면이 Menu 레이어로 올라오면(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:322`) 대화 창은 그 아래 남아 `Advance()` 를 부를 수단이 사라진다. 부수적으로 `FWxStateTreeTask_PlayDialogue::Tick`(`Private/WxDialogueStateTreeNodes.cpp:213-219`)은 영구 Running 이 되고, 대화 카메라는 `SetLifeSpan`(`cpp:284`)이 `EndDialogueCamera` 안에만 있어 컨트롤러가 사라질 때까지 남는다.

  회복은 다음 대화가 열릴 때 `ClientStartDialogue` 의 선행 `EndDialogue()`(`cpp:125-128`)가 우연히 치워 주는 것뿐인데, 상호작용이 그 태그로 막혀 있어 플레이어가 그 다음 대화를 열 수 없다. 실질 탈출구는 세이브 리로드(월드 트래블)다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged`·`EndPlay`/`UninitializeComponent` 와 폰 ASC 의 `State.Dead` 에서 활성 세션을 접도록 한다. 최소한 사망 한 갈래만이라도 접으면 영구 차단이 사라진다.
- **확신도**: 높음(코드 경로는 확정. 대화 중 사망 빈도는 콘텐츠 배치와 대화 창의 입력·정지 설정에 달렸다)

### 2. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:337`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로, 액터에 스켈레탈 메시가 둘 이상이면 어느 것이 나올지 계약이 없다. 그런데 대표 대상인 `AWxNpc` 는 이제 생성자에서 `UWxMetaHumanVisualComponent` 를 항상 붙이고(`Source/WxGame/Character/WxNpc.cpp:41`), 그 컴포넌트가 등록 시점에 Face·Outfit 스켈레탈 메시를 만들어 몸통에 붙인다(`Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:45`, `:66`). Face 는 `SetAnimInstanceClass` 로 애님 인스턴스를 갖고 있어(`:49`) 그쪽이 잡히면 `cpp:339-345` 의 경고조차 뜨지 않는 **조용한 오동작**이 된다(몽타주는 얼굴 인스턴스에서 재생되고 몸은 그대로).

  현재는 몸통 메시가 기본 서브오브젝트라 먼저 삽입돼 사실상 첫 항목으로 나오지만, 그것은 컨테이너 구현에 기댄 우연이지 이 코드가 요구한 계약이 아니다. 대상 액터는 몸통 메시를 이미 명시해 두었고(`AWxNpc::MeshComponent`, 대화 컴포넌트의 `AreaMesh` 로도 넘겨진다 — `WxNpc.cpp:39`) 이 경로만 그것을 쓰지 않는다.
- **제안**: 대상의 `UWxDialogueComponent` 가 이미 들고 있는 영역 메시(또는 전용 포즈 메시 지정)를 getter 로 열어 그것을 쓰고, `ACharacter` 면 `GetMesh()`, 그 밖의 액터에서만 `FindComponentByClass` 로 폴백한다.
- **확신도**: 중간(오동작 경로는 확정, 현재 컴포넌트 삽입 순서에서는 아직 드러나지 않는다)

### 3. 🟢 폰 ASC 를 못 찾으면 태그 없이 세션이 열려, 창도 안 뜨고 접히지도 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:144-152`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 폰 ASC 를 찾지 못하면 `State.Dialogue` 를 올리지 못한 채 그냥 지나간다 — 실패로 보지도, 로그를 남기지도 않는다. 대화 창을 띄우는 유일한 신호가 그 태그이므로(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:325-336`), 이 갈래에서는 세션만 열리고 창이 뜨지 않는다. `Advance()` 를 부를 뷰(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:44-50`)가 없으니 발견 1 과 같은 고착으로 직행하고(`Play Dialogue` 는 영구 Running), 증상은 "퀘스트가 그냥 멈춰 있다" 하나로만 남는다. 폰이 없거나 ASC 없는 폰에 빙의한 순간 `Play Dialogue` 가 도는 조립에서 실재한다.
- **제안**: ASC 부재 갈래에 `LogWxDialogue` Warning 을 남기고 세션을 열지 않고 되돌린다(창 없는 대화는 어차피 진행이 불가능하다).
- **확신도**: 중간(코드 경로는 확정, 발현 빈도는 콘텐츠 배치에 달렸다)

### 4. 🟢 대화 카메라가 이전 뷰 타겟을 기억하지 않고 무조건 폰으로 되돌린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:265`, `:280`
- **범주**: 설계/구조
- **문제**: `BeginDialogueCamera` 는 진입 직전의 뷰 타겟을 남기지 않고, `EndDialogueCamera` 는 항상 `PlayerController->GetPawn()` 으로 복귀한다. 저장소에서 뷰 타겟을 바꾸는 다른 시스템인 `UWxAnimNotifyState_CameraMove` 도 같은 방식이라(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:71`, `:184`), 둘이 겹치면 먼저 끝나는 쪽이 상대의 카메라를 폰으로 걷어낸다. 겹침은 가정이 아니라 이 모듈이 명시한 용례다 — `Play Dialogue` 의 대표 용도가 "처치 후 대사"이고(`Public/WxDialogueStateTreeNodes.h:26`), 피니셔·적 패턴 몽타주에 걸린 CameraMove ANS 가 아직 재생 중일 수 있다. 그때 ANS 의 `NotifyEnd` 가 대화 카메라에서 뷰를 뺏어 대화 내내 게임플레이 구도로 남는다.
- **제안**: 진입 시 `PlayerController->GetViewTarget()` 을 기억해 종료 시 그리로(사라졌으면 폰으로) 되돌린다. 근본적으로는 뷰 타겟 소유권을 중재하는 자리가 필요하지만, 최소 수정은 저장·복원이다.
- **확신도**: 중간(코드 경로는 확정, 두 연출이 실제로 겹치는 배치는 확인하지 못했다)

### 5. 🟢 `FindTargetDialogue` 의 대상 해석에 `IsValid` 가드가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:23-27`
- **범주**: 버그/정확성
- **문제**: `Cast<AActor>(Locator.SyncFind(Owner))` 는 파괴 대기(pending kill) 액터를 걸러 내지 않는다. UOL 액터 프래그먼트는 소프트 경로 조회라 `Destroy()` 와 GC 사이의 액터를 여전히 답할 수 있고, WP 스트리밍 아웃에 그 창이 존재한다. `AppliedTargets` 는 `TWeakObjectPtr` 라 파괴 대기 객체에 null 을 답하므로 `:46` 의 동일성 비교가 항상 어긋나, 그 창 동안 매 틱 파괴 대기 컴포넌트에 `SetInteractionEnabled` 를 호출한다. 같은 패턴의 형제 노드는 이 갈래를 명시적으로 막아 두었다 — `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorStateTreeNodes.cpp:18-22` 의 `ResolveTargetActor` 는 `IsValid(Target) ? Target : nullptr` 다.
- **제안**: 해석 결과에 `IsValid` 를 씌워 형제 노드와 동작을 맞춘다.
- **확신도**: 중간(크래시로 이어지진 않지만 무의미한 반복 호출이고, 저장소 선례와 어긋난다)

### 6. 🟢 `UWxDialogueComponent::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:27-38`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다. 세션 주입은 Experience 의 Add Components 에 달려 있어 타이밍·구성에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리돼 있으므로(`Private/WxDialogueSessionComponent.cpp:34`, `:47`, `:80`, `:164`, `:172`, `:332`, `:342`) 이 모듈에서 여기만 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 7. 🟢 `Enable Npc Interaction` 진입 시 대상 하나를 세 번 해석한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp:262-269`, `:273`
- **범주**: 중복/복잡도
- **문제**: 경고 루프가 대상마다 `Locator.SyncFind`(`:264`)를 한 번, 이어서 `FindTargetDialogue`(`:265`)가 같은 로케이터를 다시 한 번 해석하고, 그 직후 `RefreshNpcInteraction`(`:273`)이 또 한 번 해석한다. 즉 진입 1회에 대상당 `SyncFind` 3회다. 매 틱 재해석은 스트리밍 재로드 대응이라는 근거가 헤더에 있지만(`Public/WxDialogueStateTreeNodes.h:147`), 진입 경로의 이 3중 해석은 순전히 경고 로직이 해석 결과를 버리는 데서 온다.
- **제안**: 경고 루프에서 `FindTargetDialogue` 결과를 한 번만 구해 판정하고, 그 결과를 `RefreshNpcInteraction` 의 첫 적용과 공유한다(또는 경고를 `RefreshNpcInteraction` 안에서 진입 1회 플래그로 낸다).
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
- **규칙 점검 결과**: 모듈 전역 grep 으로 확인 — `Wx` prefix 준수, 소스 9개 + `.Build.cs` 전부 저작권 첫 줄 존재, 람다 0건, `BlueprintCallable` 0건, `FORCEINLINE` 0건, 델리게이트 콜백 `HandlePoseLoaded` 의 `Handle` prefix 준수. 모듈 의존은 `WxCore` 외 Wx 플러그인 없음(`WxDialogue.Build.cs`·`WxDialogue.uplugin` 모두) ✅.
- **지난 리뷰 대비 변화**: 이전 리뷰 시점의 `AWxNpc`(당시 `Private/WxNpc.cpp`)는 게임 모듈(`Source/WxGame/Character/WxNpc.cpp`)로 옮겨가 모듈 소스가 11 → 9 파일이 됐고, 그 자리의 지적(세션 부재 무로그)은 `UWxDialogueComponent::OnInteracted` 로 이동해 그대로 남아 있다(발견 6). `Enable Npc Interaction` 은 단일 대상에서 `TArray<FUniversalObjectLocator>` 로 바뀌었으나 `IsValid` 가드 누락은 그대로다(발견 5). 발견 1·2·3·4·8 은 지난 리뷰에서 지적된 뒤 코드가 바뀌지 않아 재확인만 했다.
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Plugins/WxUI/.../WxIndicatorStateTreeNodes.cpp`, `Plugins/WxCombat/.../WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`, `Plugins/WxCore/.../WxInteractable.cpp`, `Source/WxGame/...`) — 리뷰 대상은 아니다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `ClientStartDialogue` 가 동기 실행되는 것에 기대는 `Play Dialogue`(`Private/WxDialogueStateTreeNodes.cpp:201-208`)와 0번 컨트롤러 폴링(`:18`)은 데디케이티드·원격 클라에서 깨지지만, 헤더·README·세 노드 주석이 모두 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견에서 뺐다. 멀티 확장 시 제일 먼저 손댈 지점이다.
  - `Public/WxDialogueStateTreeNodes.h:71`, `:109`, `:162` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지)과 형식상 충돌하나, 같은 파일 15행이 엔진 StateTree 관례를 근거로 예외임을 명시했고 저장소 내 다른 모듈의 ST 노드가 모두 같은 형태라 발견으로 올리지 않았다. `Private/WxDialogueStateTreeNodes.cpp:14-110` 의 익명 namespace 헬퍼도 다른 도메인 ST 노드 cpp 13개와 동일한 관례라 같은 이유로 제외했다.
  - `FWxStateTreeTask_EnableNpcInteraction::Tick` 이 매 틱 `Locator.SyncFind` 로 대상을 재해석하는 비용은 실측하지 않았다. 헤더(`:147-149`)가 스트리밍 재로드 대응을 근거로 명시한 의도이고 인스턴스 수가 적어 발견으로 올리지 않았다(진입 경로의 중복 해석만 발견 7 로 올렸다).
  - 포즈 스트리밍 중 새 대화가 열려 앞 대사의 포즈 요청이 `ApplyCurrentPose`(`Private/WxDialogueSessionComponent.cpp:298-302`)의 `CancelHandle` 에 걷히는 경로를 따라갔다. `EndDialogue`(`:213-214`)의 "마지막 포즈는 늦게 도착해도 얹힌다"는 보증과 어긋나지만, 몽타주 한 장의 스트리밍 창 안에 다음 대화가 들어와야 하는 매우 좁은 레이스라 발견으로 올리지 않았다.
  - 대화가 겹쳐 열릴 때(`:125-128`) 한 프레임에 `State.Dialogue` 가 1→0→1 로 튀며 위젯이 닫히고 다시 push 되는 경로도 따라갔다. `PushSoftContentToLayer` 가 동기 경로라 뷰모델 시드와 최종 뷰 타겟이 의도대로 수렴해 발견으로 올리지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정, `:228-244`)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - 에셋 내부(`DT_Dialogue` 행 링크, `BP_Npc` 컴포넌트 구성, `WBP_DialogueScreen` 의 정지·입력 모드)는 범위 밖이고, 이전 리뷰가 참조하던 에셋 덤프 디렉터리가 현재 저장소에 없어 확인하지 못했다 — 발견 1 의 "대화 중 피격 가능" 전제는 코드로 확정하지 못한 부분이다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 9파일 — `/module-review`로 갱신*
