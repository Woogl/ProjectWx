# WxDialogue — 코드 리뷰

> 소스 9개의 작고 응집도 높은 모듈이다. 지난 리뷰 이후 `WxDialogueStateTreeNodes`(3노드)가 `WxStateTreeTask_PlayDialogue` 단일 태스크로 축소되면서 중복 해석·대상 캐시 관련 지적이 통째로 사라졌고, 코딩·모듈 의존 규칙 위반 0건에 실패 갈래마다 경고 로그와 근거 주석이 붙어 있어 여전히 읽기 좋다. 남은 지적은 "세션이 자기 데이터를 끝까지 진행한다"는 단일 전제가 깨지는 자리(외부 종료·빙의 변경·뷰 타겟 경합)와 대상 해석의 느슨함에 몰려 있다. 커버리지: `.Build.cs`·`.uplugin`·README 포함 소스 9개를 전부 읽고 세션 컴포넌트(h/cpp)와 ST 태스크를 정독했으며, 발현 확인용으로 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`·`AWxNpc`·`UWxMetaHumanVisualComponent`·`UWxAnimNotifyState_CameraMove`·`WxGameFeatureAction_AddComponents`·`WAS_CoreGameplay`)와 엔진 측 `FinishTask`·`CreateComponentOnInstance` 까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 세션을 밖에서 접을 수단도 수명 훅도 없다 — 대화 창이 닫혀도 세션은 열린 채 남는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:49-73`(공개 API), `:130`(`EndDialogue` private), `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:197-215`
- **범주**: 설계/구조
- **문제**: 세션이 끝나는 경로는 `Advance()` 가 `NextRow == None` 인 행에 닿는 것 하나뿐이다. `EndDialogue()` 는 private(`h:130`)이고 공개 API(`h:49-73`)에 취소·강제 종료가 없으며, 컴포넌트에 `EndPlay`·`UninitializeComponent`·빙의 변경 훅 선언이 하나도 없다.

  가장 확실한 발현은 대화 중 빙의 변경이다. UI 매니저가 `OnPossessedPawnChanged` 를 받아(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:262`, `:271`) `WatchPawnTags` 에서 대화 창을 무조건 먼저 닫는데(`:299`), 이때 세션에는 아무 신호도 가지 않는다. 창이 사라지면 `Advance()` 를 부를 유일한 주체(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:44-50`)가 없어져 `CurrentRowName` 이 영원히 남는다. 이번 리팩터링으로 `Play Dialogue` 는 폴링을 버리고 `OnDialogueEnded` 한 번에만 완료를 걸었으므로(`Private/WxStateTreeTask_PlayDialogue.cpp:51-56`, `bShouldCallTick = false`) 그 태스크는 영구 `Running` 이 되어 퀘스트 단계가 완료되지 않는다.

  같은 갈래에서 `TaggedAbilitySystem` 이 기억한 것은 **이전** 폰의 ASC 라 `State.Dialogue` 가 그 폰에 1 로 남고(`cpp:148-149`), 그 태그를 차단 태그로 쓰는 상호작용 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:35`)가 그 폰에서 계속 막힌다. 대화 카메라도 `SetLifeSpan` 이 `EndDialogueCamera` 안에만 있어(`cpp:282`) 컨트롤러가 사라질 때까지 남는다.
- **제안**: 세션에 외부 종료 진입점(`CancelDialogue()` 성격)을 열고, 컴포넌트가 오너 컨트롤러의 `OnPossessedPawnChanged` 와 `EndPlay`/`UninitializeComponent` 에서 활성 세션을 접도록 한다. 빙의 변경 한 갈래만 막아도 위 하드행은 사라진다.
- **확신도**: 중간(코드 경로는 확정. 대화 중 빙의 변경이 실제로 얼마나 나는지는 콘텐츠 배치에 달렸다)

### 2. 🟡 포즈 대상 메시를 `FindComponentByClass` 로 아무거나 하나 집는다 — 메타휴먼 얼굴에 얹히면 무경고로 어긋난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:335`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 `AActor::OwnedComponents`(TSet) 순회의 첫 항목을 돌려주므로, 액터에 스켈레탈 메시가 둘 이상이면 무엇이 나올지 계약이 없다(TSet 순회는 삽입 순서가 아니라 해시 순서다). 그런데 대표 대상인 `AWxNpc` 는 생성자에서 `UWxMetaHumanVisualComponent` 를 항상 붙이고(`Source/WxGame/Character/WxNpc.cpp:41`), 그 컴포넌트가 `OnRegister` 에서 Face·Outfit 스켈레탈 메시를 `NewObject` 로 만들어 몸통에 붙인다(`Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:45`, `:52`, `:66`, `:71`). Face 는 `SetAnimInstanceClass` 로 자기 애님 인스턴스를 갖고 있어(`:49`) 그쪽이 잡히면 `cpp:336-343` 의 "애님 인스턴스 없음" 경고조차 뜨지 않는다 — 몽타주가 얼굴에서 재생되고 몸은 그대로인 무경고 오동작이다.

  대상 액터는 몸통 메시를 이미 명시해 두었고(`AWxNpc::MeshComponent` — 대화 컴포넌트의 `AreaMesh` 로도 그대로 넘어간다, `WxNpc.cpp:39`), 같은 모듈의 상호작용 영역 판정은 그 명시 값만 본다(`Private/WxDialogueComponent.cpp:24`). 이 경로만 그 규약에서 벗어나 있다. `UWxDialogueComponent::AreaMesh` 의 주석(`Public/WxDialogueComponent.h:56-59`)이 "오너에서 자동으로 찾지 않는 이유는 메타휴먼 부착물처럼 등록 시점에 생기는 메시가 있으면 무엇이 잡힐지 정해지지 않기 때문"이라고 적은 바로 그 함정에 포즈 경로가 걸려 있다.
- **제안**: `UWxDialogueComponent` 가 이미 들고 있는 영역 메시를 getter 로 열어 쓰거나(또는 전용 포즈 메시 지정을 추가), 최소한 몽타주 스켈레톤과 맞는 메시만 고르도록 좁힌다.
- **확신도**: 높음(다중 메시 구성과 TSet 순회는 확인함. 현재 배치에서 항상 얼굴이 먼저 잡히는지는 런타임 미확인)

### 3. 🟡 주입 컴포넌트를 복제로 표시해 원격 클라에서 인스턴스가 둘이 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:25-26`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:25`
- **범주**: 설계/구조
- **문제**: 생성자 주석은 "주입으로 붙는 동적 컴포넌트라 안정된 이름이 없다 — 원격에서 이 객체를 해소하는 수단이 복제뿐"이라며 `SetIsReplicatedByDefault(true)` 를 건다. 그런데 복제는 **기존 주입 인스턴스에 결선해 주지 않고 별도 인스턴스를 새로 만든다**. 주입은 `WAS_CoreGameplay` 의 `WxGameFeatureAction_AddComponents` 가 Client·Server 번들 양쪽에 이 클래스를 올려 두어(에셋 확인) 클라에서도 자체 생성되고(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:152`), 그 생성 경로인 엔진 `UGameFrameworkComponentManager::CreateComponentOnInstance` 는 `SetNetAddressable()` 을 부르지 않는다(UE 5.8 `GameFrameworkComponentManager.cpp:544-553`). 즉 `IsNameStableForNetworking()` 이 false 라 서버 인스턴스는 액터 채널에서 동적 서브오브젝트로 다시 만들어지고, 원격 클라의 PC 에는 `UWxDialogueSessionComponent` 가 둘 남는다.

  그 뒤 `FindComponentByClass<UWxDialogueSessionComponent>()` 로 세션을 집는 소비처(`Private/WxDialogueComponent.cpp:42`, `Private/WxStateTreeTask_PlayDialogue.cpp:33`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:60`)가 RPC 를 받는 쪽이 아닌 인스턴스를 집으면 대화가 통째로 안 열린다. 리슨 호스트·로컬 플레이어에서는 복제본이 생기지 않아 드러나지 않으므로, v1 전제 안에서는 잠복 상태다.
- **제안**: 멀티 확장 시 주입 컴포넌트를 net addressable 로 만들거나(주입 경로에서 `SetNetAddressable()` 후 등록), 세션을 복제하지 않고 대화 개시를 PC 자체의 Client RPC(또는 안정 이름을 갖는 컴포넌트)로 옮긴다. 지금 당장은 헤더 주석의 "복제면 원격에서 해소된다"는 전제를 정정해 두는 것만으로도 다음 세션의 오독을 막는다.
- **확신도**: 중간(엔진·에셋 근거는 확인함. v1 리슨 호스트 전제에서는 발현하지 않는다)

### 4. 🟡 대화 카메라가 진입 전 뷰 타겟을 기억하지 않고 무조건 폰으로 되돌린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:263`, `:278`
- **범주**: 설계/구조
- **문제**: `BeginDialogueCamera` 는 진입 직전의 뷰 타겟을 남기지 않고, `EndDialogueCamera` 는 항상 `PlayerController->GetPawn()` 으로 복귀한다. 저장소에서 뷰 타겟을 바꾸는 다른 시스템인 `UWxAnimNotifyState_CameraMove` 도 같은 방식이라(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:71`, `:182`), 둘이 겹치면 먼저 끝나는 쪽이 상대의 카메라를 폰으로 걷어낸다. 겹침은 가정이 아니라 이 모듈이 명시한 용례다 — `Play Dialogue` 의 대표 용도로 "처치 후 대사"를 들고 있어(`Public/WxStateTreeTask_PlayDialogue.h:26`) 피니셔 몽타주의 CameraMove ANS 가 아직 재생 중일 수 있고, 그때 ANS 의 `NotifyEnd` 가 대화 카메라에서 뷰를 뺏어 대화 내내 게임플레이 구도로 남는다.
- **제안**: 진입 시 `PlayerController->GetViewTarget()` 을 기억해 종료 시 그리로(사라졌으면 폰으로) 되돌린다. 근본적으로는 뷰 타겟 소유권을 중재하는 자리가 필요하지만 최소 수정은 저장·복원이다.
- **확신도**: 중간(코드 경로는 확정, 두 연출이 겹치는 실제 배치는 확인하지 못했다)

### 5. 🟢 폰 ASC 를 못 찾으면 태그 없이 세션이 열려, 창도 안 뜨고 접히지도 않는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:142-150`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 폰이 없거나 폰 ASC 를 찾지 못하면 `State.Dialogue` 를 올리지 못한 채 그냥 지나간다 — 실패로 보지도, 로그를 남기지도 않는다. 대화 창을 띄우는 유일한 신호가 그 태그이므로(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:327-337`), 이 갈래에서는 세션만 열리고 창이 뜨지 않는다. `Advance()` 를 부를 뷰가 없으니 곧바로 발견 1 의 고착(`Play Dialogue` 영구 Running)으로 직행하고, 증상은 "퀘스트가 그냥 멈춰 있다" 하나로만 남는다. 폰이 아직 없거나 ASC 없는 폰에 빙의한 순간 `Play Dialogue` 가 도는 조립에서 실재한다.
- **제안**: ASC 부재 갈래에 `LogWxDialogue` Warning 을 남기고 세션을 열지 않은 채 되돌린다(창 없는 대화는 어차피 진행이 불가능하다).
- **확신도**: 중간(코드 경로는 확정, 발현 빈도는 콘텐츠 배치에 달렸다)

### 6. 🟢 `UWxDialogueComponent::OnInteracted` 가 세션을 못 찾으면 로그 없이 무동작한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:38-49`
- **범주**: 버그/정확성
- **문제**: 상호작용자가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 아직 주입되지 않았으면 조용히 반환한다(`:43-46`). 세션 주입은 Experience 의 Add Components 에 달려 있어 타이밍·구성에 따라 없을 수 있는 값인데, 이 갈래가 침묵하면 증상은 "F 를 눌러도 아무 일이 없다" 하나로만 남는다. 세션 쪽 실패 갈래는 전부 Warning 을 남기도록 정리돼 있어(`Private/WxDialogueSessionComponent.cpp:33`, `:45`, `:78`, `:162`, `:170`, `:330`, `:340`) 이 모듈에서 여기만 예외다.
- **제안**: 세션 부재 갈래에 `LogWxDialogue` Warning 을 하나 남긴다.
- **확신도**: 높음

### 7. 🟢 대상 없는 대사(나레이션)에서도 포즈를 스트리밍한 뒤 버린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:289-314`
- **범주**: 성능/안전
- **문제**: `ApplyCurrentPose` 는 행의 `TargetPose` 만 보고 `PendingPoseTarget = CurrentTarget`(`:303`) 이 null 인지는 보지 않은 채 `RequestAsyncLoad`(`:312`)를 건다. `Play Dialogue` 는 항상 `Target=nullptr` 로 세션을 열므로(`Private/WxStateTreeTask_PlayDialogue.cpp:40`), 포즈가 지정된 테이블을 ST 로 재생하면 몽타주를 매 대사마다 로드했다가 `PlayPendingPose` 에서 "대상에 애님 인스턴스가 없다" 경고만 남기고 버린다(`:340`). 대상이 없으면 포즈를 얹을 곳이 애초에 없으므로 스트리밍 자체가 헛일이고, 로그도 오해를 부른다(진짜 문제는 애님 인스턴스가 아니라 대상 부재다).
- **제안**: `ApplyCurrentPose` 진입부에서 `CurrentTarget.IsValid()` 를 함께 가려 대상 없는 대사는 스트리밍을 걸지 않는다.
- **확신도**: 높음

### 8. 🟢 카메라 경로에서 엔진 포인터를 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:237`, `:278`
- **범주**: 성능/안전
- **문제**: `PlayerController->PlayerCameraManager->GetCameraLocation()`(`:237`)은 카메라 매니저를 검사 없이 역참조한다. 로컬 PC 라면 사실상 항상 유효하지만, 같은 함수의 다른 입력(폰 `:220`, 대상 `:221`, 스폰 결과 `:250`)은 모두 검사를 거치고 있어 여기만 예외다. `EndDialogueCamera` 의 `PlayerController->GetPawn()`(`:278`)도 null 일 수 있고(대화 도중 폰 소멸), 그때 엔진이 뷰 타겟을 컨트롤러 자신으로 떨어뜨려 구도가 튄다.
- **제안**: 카메라 매니저 null 검사를 더하고, 복귀 뷰 타겟이 null 이면 뷰 전환을 건너뛴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **규칙 점검 결과**: 모듈 전역 확인 — `Wx` prefix 준수, 소스 9개 + `.Build.cs` 전부 저작권 첫 줄 존재, `BlueprintCallable` 0건, `FORCEINLINE` 0건, 델리게이트 콜백 `HandlePoseLoaded` 의 `Handle` prefix 준수, `Super::` 호출이 필요한 override 는 생성자(`cpp:23`)뿐이며 준수. 람다는 `Private/WxStateTreeTask_PlayDialogue.cpp:51` 1건으로, 약한 실행 컨텍스트를 넘기는 엔진 제시 방식이라 "반드시 필요한 경우"에 해당하며 근거 주석(`:49-50`)도 붙어 있다. 모듈 의존은 `WxCore` 외 Wx 플러그인 없음(`WxDialogue.Build.cs`·`WxDialogue.uplugin` 모두) ✅.
- **지난 리뷰 대비 변화**: 모듈이 축소됐다. `WxDialogueStateTreeNodes.h/cpp`(`Enable Npc Interaction`·`Wait Dialogue Completed` 포함 3노드)가 사라지고 `WxStateTreeTask_PlayDialogue.h/cpp` 단일 태스크만 남았으며, 그 태스크도 틱 폴링 대신 `OnDialogueEnded` 일회성 신호로 완료를 받는 형태로 바뀌었다. 이로써 지난 리뷰의 발견 5(`FindTargetDialogue` 의 `IsValid` 가드)와 7(진입 시 3중 `SyncFind`)은 대상 코드가 없어져 소멸했다. 발견 1·2·4·6·8 은 근거를 다시 확인해 재현했고(발견 1 은 폴링 소멸로 하드행 경로가 오히려 단순·확실해졌다), 발견 3(폰 ASC 부재)은 유지하되 지난 리뷰에서 🟢 였던 판단을 그대로 뒀다. 발견 3(복제 중복 인스턴스)과 7(나레이션 헛 스트리밍)이 이번에 새로 추가됐다.
- **미검토 / 한계**:
  - 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Plugins/WxCombat/.../WxAnimNotifyState_CameraMove.cpp`, `Source/WxGame/...`, `Content/Framework/WAS_CoreGameplay.uasset`) — 리뷰 대상은 아니다.
  - `ClientStartDialogue` 가 세션 시작 시 `OnLineChanged` 를 발행하지 않고 관찰자의 pull 시드에 기대는 비대칭(`cpp:119-154` vs `Advance` 의 `:84`)을 따라갔다. 현재 유일한 소비자인 `UWxViewModel_Dialogue::Initialize`(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:20`)가 실제로 pull 시드하므로 지금은 동작하며, 구독을 유지한 채 재사용되는 뷰모델이 실재하는지는 위젯 수명(BP)에 달려 확인하지 못해 발견으로 올리지 않았다.
  - `EndDialogue` 가 `OnDialogueEnded.Broadcast()` 직후 `Clear()` 하는 구조(`cpp:213-214`)에서, 브로드캐스트 도중 새 대화가 열리면 그때 등록된 구독이 함께 지워지는 재진입 위험을 따라갔다. 현재 유일한 구독자인 `Play Dialogue` 의 `FinishTask` 가 엔진에서 즉시 전이하지 않고 다음 틱으로 미루는 것을 확인했으므로(UE 5.8 `StateTreeAsyncExecutionContext.cpp:243-247`, `bHasPendingCompletedState` + `ScheduleNextTick`) 현재 조립에서는 발현하지 않아 발견으로 올리지 않았다. 구독자가 늘면 재검토할 자리다.
  - 상태를 먼저 떠난 `Play Dialogue` 의 잔류 람다가 나중에 발화해 재진입한 같은 상태의 태스크를 잘못 완료시키는지 따라갔다. `FActiveStateID` 가 진입마다 새로 발급되는 런타임 고유 ID라(`StateTreeStatePath.h:44-68`) 재진입 후에는 `GetActivePathInfo()` 가 무효를 답해 no-op 이 되는 것을 확인했다 — 헤더 주석의 주장이 맞다.
  - 멀티플레이 동작은 코드 독해로만 판단했다. `Play Dialogue` 가 `StartDialogueRow` 직후 `HasActiveDialogue()` 로 성공을 가리는 것(`Private/WxStateTreeTask_PlayDialogue.cpp:43-47`)은 데디케이티드 서버에서 항상 실패로 읽혀 오해를 부르는 경고를 남기지만, 헤더·README 가 v1 싱글/리슨 호스트 전제를 명시하고 있어 의도된 한계로 보고 발견 3 안에 묶어 다뤘다.
  - `Public/WxStateTreeTask_PlayDialogue.h:43` 의 `GetInstanceDataType()` 헤더 정의는 코딩 규칙 6(인라인 정의 금지)과 형식상 충돌하나, 같은 파일 13행이 엔진 StateTree 관례를 근거로 예외임을 명시했고 저장소 내 다른 모듈의 ST 노드 20여 개가 전부 같은 형태라 발견으로 올리지 않았다.
  - `BeginDialogueCamera` 에서 두 액터의 XY 가 완전히 겹치면 `GetSafeNormal2D()` 가 영벡터를 답해 구도가 퇴화하는 경로(`cpp:233-242`)를 따라갔다. 크래시는 없고 상호작용 사거리상 발생 여지가 희박해 발견으로 올리지 않았다.
  - 카메라 구도 수식(`cpp:228-242`)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.
  - 에셋 내부(`DT_Dialogue` 행 링크, `BP_Npc` 컴포넌트 구성, `WBP_DialogueScreen` 의 입력 모드)는 범위 밖이다 — 발견 1 의 "대화 중 빙의 변경 가능" 전제는 코드로만 판단했다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 9파일 — `/module-review`로 갱신*
