# WxDialogue — 코드 리뷰

> 11파일 규모의 작고 응집도 높은 모듈이다. 실패 갈래마다 경고를 남기고 수명·소유권 근거를 주석으로 못 박아 둔 편이라 전반적으로 건강하며, 프로젝트 코딩·모듈 규칙 위반은 없다. 이번 리뷰는 모듈의 모든 `.h`/`.cpp` 를 읽고 세션 컴포넌트·ST 태스크의 실행 흐름을 소비자(`WxGame` 의 뷰모델·상호작용 어빌리티, `WxUI` 의 매니저)와 엔진 StateTree 구현까지 따라가 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 겹쳐 열리는 세션이 앞선 `Play Dialogue` 태스크를 Succeeded 로 오완료시킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:123`, `:213`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 은 새 세션을 열기 전에 `EndDialogue()` 로 앞 세션을 접는데, `EndDialogue()` 는 종료 사유를 가리지 않고 `OnDialogueEnded` 를 broadcast 한다. 앞 세션이 `FWxStateTreeTask_PlayDialogue` 가 연 것이었다면 그 태스크는 대사를 한 줄도 넘기지 못한 채 `Succeeded` 로 완료된다(`WxStateTreeTask_PlayDialogue.cpp:51`). 코드 주석(`:121`) 스스로 "겹쳐 열리는 경로가 실재한다"고 적고 있으며, 퀘스트 트리 둘이 동시에 진행되는 오픈월드에서 두 `Play Dialogue` 가 겹치면 앞 퀘스트가 대사를 건너뛴 채 다음 단계로 넘어간다.
- **제안**: 종료 사유를 나눈다 — 교체 경로에서는 `Clear()` 만 하고 broadcast 를 생략하거나, 신호를 정상 종료/중단 2종으로 갈라 태스크가 중단이면 `Failed` 를 내게 한다.
- **확신도**: 중간

### 2. 🟡 세션에 강제 종료 경로가 없어 사망·언포제스 시 대화 상태와 ST 태스크가 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:197`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:41`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 는 `Advance()` 로 대사를 끝까지 넘기거나 다음 세션이 열릴 때만 불린다. 클래스에 `EndPlay`/`OnUnregister`/폰 해제 훅이 하나도 없어, 대화 도중 플레이어가 죽거나 컨트롤러가 폰을 놓으면 `CurrentRowName` 이 남아 `HasActiveDialogue()` 가 계속 true 이고, `OnDialogueEnded` 를 기다리던 `Play Dialogue` 태스크는 Running 에 굳어 해당 퀘스트 스텝이 다음 대화가 열릴 때까지 진행하지 않는다(다음 대화의 `EndDialogue()` 가 뒤늦게 신호를 쏴 자가 회복하지만, 그때까지는 멈춘 채다). `TaggedAbilitySystem` 이 사라진 폰 ASC 를 가리키게 되는 것도 같은 원인이다.
- **제안**: `EndPlay`(또는 컨트롤러의 폰 해제 지점)에서 `EndDialogue()` 를 부른다.
- **확신도**: 중간

### 3. 🟡 포즈를 얹을 메시를 "첫 번째 스켈레탈 메시"로 고른다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:335`
- **범주**: 버그/정확성
- **문제**: `Target->FindComponentByClass<USkeletalMeshComponent>()` 는 대상이 스켈레탈 메시를 둘 이상 들면 컴포넌트 배열 순서에 따라 아무거나 답한다. 지금 `AWxNpc`(`Source/WxGame/Character/WxNpc.cpp:26`)가 우연히 단일 메시인 것은 `UWxMetaHumanComponent::OnRegister` 가 오너를 `ACharacter` 로 캐스트해(`Source/WxGame/Character/WxMetaHumanComponent.cpp:35`) 액터인 `AWxNpc` 에서 조립을 건너뛰기 때문이다. 그 캐스트가 고쳐지거나 BP 가 무기·아웃핏 메시를 하나 붙이는 순간, 몽타주가 조용히 엉뚱한 메시로 간다 — 메타휴먼 구성에서는 구동용 리더 메시가 숨겨진 쪽이라 "포즈는 재생됐는데 화면은 그대로"가 된다.
- **제안**: 포즈 대상 메시를 대상 액터가 답하게 한다 — `AWxDialogueActor` 에 가상 접근자를 두거나 ComponentTag 규약(프로젝트에 이미 있는 `VisualOverride` 류)으로 지목하고, 세션은 그 결과만 쓴다.
- **확신도**: 중간

### 4. 🟡 v1 싱글/리슨 호스트 전제가 세 지점에 하드 결선돼 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:32`, `:43`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:148`
- **범주**: 설계/구조
- **문제**: (a) ST 태스크가 `StartDialogueRow()`(=Client RPC 발사) 직후 같은 프레임에 `HasActiveDialogue()` 를 동기 검사한다 — RPC 가 실제로 원격으로 나가는 데디케이티드 서버에서는 항상 false 라 진입이 무조건 `Failed` 다. (b) 컨트롤러 인덱스 0 하드코딩이라 스플릿스크린·멀티에서 엉뚱한 플레이어에게 대사가 열린다. (c) `State.Dialogue` 를 소유 클라의 폰 ASC 에만 loose 로 올리므로, 서버 권위로 도는 `UWxAbility_Interact` 의 `ActivationBlockedTags` 게이트(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)가 데디케이티드에서 걸리지 않아 대화 중 재상호작용이 열린다. 세 가지 모두 README·헤더에 "v1 전제"로 명시돼 있으나, 전제가 깨질 때 드러나는 증상이 조용하다(경고 없이 Failed, 또는 그냥 동작 차이).
- **제안**: 지금 고칠 필요는 없다. 다만 (a)에는 "리슨 호스트 전제" 가 깨졌음을 알리는 별도 경고 문구를, 멀티 확장 시점에는 (a)를 서버측 세션 상태 조회로, (c)를 서버측 태그 발행으로 옮기는 것을 체크리스트에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 문서화된 v1 전제다)

### 5. 🟢 `GetCurrentRowHandle()`·`GetCurrentDialogueTarget()` 은 호출자가 없다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`, `:69`
- **범주**: 중복/복잡도
- **문제**: README 는 "대화의 의미 해석(퀘스트 수주 등)은 소비자가 현재 행 신원을 관찰로 판정 — WxQuest" 라고 계약을 선언하지만, 저장소 전체(`Source`·`Plugins`)에 두 함수의 호출자가 없다. 소비자 없는 공개 표면이라, 이 계약이 실제로 성립하는지 검증된 적이 없다(특히 "관찰자가 권위 측에서 이 로컬 상태를 직접 읽는다"는 전제).
- **제안**: 소비자가 생길 때까지 제거하거나, 계획 단계임을 README 서술에 반영한다.
- **확신도**: 높음

### 6. 🟢 `UniversalObjectLocator` 의존은 삭제된 노드의 잔재다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs:20`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 어디에서도 UOL 을 쓰지 않는다. 유일한 사용처였던 `WxStateTreeTask_EnableNpcInteraction` 은 소스에서 사라지고 `Intermediate/` 산출물로만 남아 있으며, 기능은 `Plugins/WxWorld/.../WxStateTreeTask_EnableInteraction` 이 대체했다. 같은 맥락에서 `GameplayAbilities`(`:16`)는 cpp 에서만 쓰이는데 `PublicDependencyModuleNames` 에 올라가 있어 이 모듈을 참조하는 쪽으로 불필요하게 전파된다.
- **제안**: `UniversalObjectLocator` 제거, `GameplayAbilities` 는 Private 으로 내린다.
- **확신도**: 높음

### 7. 🟢 세션을 찾지 못한 상호작용 경로만 조용히 실패한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:24`
- **범주**: 버그/정확성
- **문제**: `StartDialogueWith` 는 Interactor 가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 주입되지 않았을 때 로그 없이 return 한다. 같은 모듈의 다른 실패 갈래(`StartDialogueRow`, `EnterRow`, ST 태스크)는 전부 경고를 찍으므로 여기만 규약에서 벗어나 있고, Experience 에 세션 컴포넌트를 넣지 않은 조립 실수가 "F 를 눌러도 아무 일이 없다"로만 보인다 — 저자가 `WxDialogueSessionComponent.cpp:44` 주석에서 스스로 경계한 바로 그 증상이다.
- **제안**: 세션 부재 시 경고 한 줄을 남긴다.
- **확신도**: 높음

### 8. 🟢 `PlayerCameraManager` 만 널 검사 없이 역참조한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:237`
- **범주**: 성능/안전
- **문제**: `BeginDialogueCamera` 는 PC·Pawn·`CurrentTarget`·`SpawnActor` 결과를 모두 가리는데 `PlayerController->PlayerCameraManager` 만 무검사다. 로컬 PC 면 대개 존재하지만 트래블·PC 파괴 진행 중처럼 카메라 매니저가 없는 순간에 세션이 열리면 크래시다.
- **제안**: 다른 가드와 같은 줄에서 함께 검사한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`
- **교차 확인한 모듈 밖 파일**(발견 근거로만): `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`
- **미검토 / 한계**:
  - 규칙 위반은 0건이다 — 저작권 첫 줄·`Wx` prefix·`Handle` prefix·`BlueprintCallable` 미사용·`WxCore` 외 플러그인 무참조를 전부 확인했고, 유일한 인라인 정의(`GetInstanceDataType`)와 유일한 람다(ST 태스크의 약한 실행 컨텍스트 전달)는 규칙이 허용하는 예외이며 예외 사유 주석도 붙어 있다.
  - `FDataTableRowHandle` 을 Client RPC 인자로 넘기는 경로(`WxDialogueSessionComponent.cpp:115`)는 리슨 호스트에서 로컬 실행돼 직렬화를 타지 않으므로, 실제 원격 직렬화(DataTable 에셋의 NetGUID 해소)는 검증하지 못했다.
  - 대화 창 위젯의 열림/닫힘 타이밍(`PushSoftContentToLayer` 의 비동기 로드와 세션 교체가 겹칠 때)은 `WxUI`/BP 영역이라 이번 범위 밖이다.
  - 리뷰 중 인접 모듈에서 발견한 사실 하나: `UWxMetaHumanComponent::OnRegister` 의 `Cast<ACharacter>(GetOwner())` 는 액터인 `AWxNpc` 에서 실패해 메타휴먼 부착물이 조립되지 않는다(`Source/WxGame/Character/WxMetaHumanComponent.cpp:35`). WxDialogue 소관이 아니라 발견 항목으로 세우지 않았고, 발견 3의 전제로만 인용했다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 11파일 — `/module-review`로 갱신*
