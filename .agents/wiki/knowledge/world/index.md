# WxWorld — 월드 오브젝트 및 상호작용

## 한줄 요약

WxWorld는 월드 장치의 StateTree 실행·상태 복제와 상호작용 탐색·스폰·체크포인트를 관리합니다.


> 레벨에 배치되는 장치(문·상자·엘리베이터·체크포인트)와 스포너, 그리고 플레이어가 그것들을 집어 드는 상호작용 표면을 담당한다. 장치의 상태는 StateTree 에셋으로 저작하고, 이 모듈은 그 트리를 구동·복제하는 런타임과 트리에서 쓸 태스크 라이브러리를 제공한다.

## 책임

**담당**
- 월드 장치의 상태 구동: StateTree 실행, 상태 태그 기반 식별, 서버 상태의 클라 복원(`UWxDeviceStateTreeComponent`)
- 상호작용 후보 스캔·선택·하이라이트와 서버로의 상호작용 요청 송신(`UWxInteractionScannerComponent`)
- 배치형 스폰과 처치/리스폰 상태 보유(`AWxSpawner`), 그 상태를 읽고 쓰는 ST 태스크들
- 체크포인트(싱글플레이 부활 지점) 기록·조회 저장소
- 장치 트리 저작에 쓰는 ST 태스크 전반(이동·연출·이벤트 전달·대기)

**경계 (비담당)**
- 상호작용의 권위 판정과 실제 실행: 서버는 `Event.Interact` 로 어빌리티([Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp](../../../../Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp))를 띄우고, 그 어빌리티가 사거리·활성 검증 후 [WxCore](../foundation/index.md)의 `IWxInteractable` 을 호출한다. 이 모듈은 요청을 보내고 인터페이스를 구현할 뿐이다.
- 상호작용 목록 HUD: 프롬프트 배열·선택 인덱스를 델리게이트로 내보낼 뿐, 표시는 [Source/WxGame/MVVM/WxViewModel_InteractionList.h](../../../../Source/WxGame/MVVM/WxViewModel_InteractionList.h) 와 위젯 쪽([WxUI](../ui/index.md)) 몫이다.
- 장치 컴포넌트 이름 드롭다운 등 에디터 UI 커스터마이제이션: [Source/WxEditor/WxStateTreeComponentNameCustomization.h](../../../../Source/WxEditor/WxStateTreeComponentNameCustomization.h)
- 캐릭터 리스폰 정책: 체크포인트 값을 읽어 쓰는 쪽은 [Source/WxGame/Framework/WxRespawnLibrary.cpp](../../../../Source/WxGame/Framework/WxRespawnLibrary.cpp) 다.

## 핵심 타입 (진입점)

| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 계열의 루트. 상호작용 표면(`IWxInteractable`)과 배치 배선(`LinkedDevices`)만 들고 상태 실행은 아래 컴포넌트에 전부 위임한다 | [Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h) |
| `UWxDeviceStateTreeComponent` | 트리 실행 + 상태 태그 발행(서버) + 통지받은 상태로 진입(클라)을 쥔다 | [Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h) |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 액터의 컴포넌트를 지목하는 유일한 저장 형태. 이동·연출 태스크들이 공통으로 쓴다 | [Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h) |
| `UWxInteractionScannerComponent` | 상호작용 파이프라인의 클라 측 시작점. PlayerController 에 붙어 스캔→선택→`ServerInteract` 까지 간다 | [Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h) |
| `FWxStateTreeTask_WaitForInteraction` | 모듈 밖 권위 경로가 들어오는 문. `NotifyInteracted`/`IsAwaited` 정적 함수가 WxGame 쪽에서 불린다 | [Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h) |
| `TWxStateTreeWaitRegistry` | 폴링 없이 대기하는 태스크들이 공유하는 등록부 템플릿 | [Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h) |
| `AWxSpawner` | 스폰 대상의 수명과 처치 상태를 쥐는 배치 액터. 처치 판정은 대상이 하고 스포너는 그 통지를 받는다. 대상이 구현할 `IWxSpawnable` 계약은 [WxCombat](../combat/index.md) 소환 노티파이도 같은 필터로 쓰므로 [WxCore](../foundation/index.md)에 있다 | [Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h), [Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h](../../../../Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h) |
| `UWxCheckpointSubsystem` | 체크포인트 태스크가 쓰고 WxGame 리스폰 경로가 읽는 GameInstance 저장소 | [Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h) |

## 확장 포인트 / 규약

- **새 장치**: `AWxDevice` 파생 BP + StateTree 에셋. 루트 컴포넌트는 BP 가 세운다. 상태 식별자는 루트 에셋의 상태 태그이며, 컴포넌트의 `InitialState` 가 시작 상태를 정한다.
- **작동 신호는 하나, 받는 방식은 대기 태스크**(2026-09-21 사용자 확정): 직접 눌리든 다른 장치가 밀든 장치는 `AWxDevice::NotifyDeviceInteracted` 하나로 작동되고, 그 상태에서 Running 으로 기다리던 `작동 대기`(`FWxStateTreeTask_WaitForTrigger`)가 Succeeded 로 완료된다. 어느 상태로 갈지는 그 상태의 「성공 시」(On State Succeeded) 전이가 정하며, 상황별 분기는 상태마다 다른 목적지 또는 조건을 단 전이를 위에서부터 두어 만든다. 기존 `상호작용 대기`(`FWxStateTreeTask_WaitForInteraction`)와 같은 엔진 순정 패턴이다 — 진입에 장치에 등록하고, 장치가 약한 실행 컨텍스트의 `FinishTask` 로 완료를 통보한다(틱·폴링 없음). ST 이벤트·델리게이트 전이, 상태 태그를 이벤트로 보내는 「그 상태로 가라」 명령, 컴파일된 트리를 틱 밖에서 조회하는 코드는 쓰지 않는다(사용자 선호: 엔진 순정 룰을 벗어나지 않는다 — 자체 장치는 트리 디버거에 보이지 않아 추적이 어렵다).
- **대기 노드는 `작동 대기` 하나**: 선택 설정 두 가지로 쓰임새를 가른다 — 「플레이어 상호작용」(프롬프트, 연결 장치가 받을 때만)을 켜면 직접 누를 수 있는 장치가 되고(다른 장치의 작동도 같은 전이로 받음), 「수락 규칙」(`TInstancedStruct<FWxDeviceTriggerRule>`)으로 누구에게서 무엇을 받을지를 정한다(비우면 누구든 받음). 노드를 종류별로 나누지 않는 이유는 다른 것이 동작이 아니라 설정뿐이어서다 — 별도 노드(상태 상호작용·엘리베이터 호출 대기)와 공유 베이스는 폐기했고, 장치 전용 설정(엘리베이터)은 범용 노드가 아니라 규칙 구조체 안에 둔다(2026-09-21 사용자 확정). 설정은 트리 틱 밖(스캔·수락 검증)에서도 읽히므로 전부 노드 프로퍼티다. 이 노드가 없는 상태에서는 작동도 상호작용도 받지 않는다. 활성 경로에 하나만 둔다(겹치면 경고를 남기고 나중에 진입한 노드만 받는다). 직접 눌리는 장치의 선택지도 자기 노드의 수락 규칙으로 계산하므로, 눌러도 받지 않을 조합(플레이어 상호작용 + 정차 지점 선택)은 프롬프트부터 뜨지 않는다. 신호를 받아들이면 그 자리에서 등록을 걷어 같은 프레임의 다음 신호는 받지 않는다(첫 신호가 이긴다). 받지 않은 신호는 `LogWxWorld` Verbose 로 남는다. 완료 판정에 포함돼야 하므로 같은 상태의 즉시 완료 태스크는 「완료 판정 제외」로 둔다(그러지 않으면 「성공 시」 전이가 곧바로 돈다).
- **잠금도 받는 쪽이 정한다**: 버튼·레버는 「플레이어 상호작용」의 「연결 장치가 받을 때만」을 켠다. 그러면 `LinkedDevices` 중 하나라도 `작동 대기` 가 활성일 때만 프롬프트가 뜬다(`AWxDevice::GetAcceptedOptions`) — 잠금 여부가 곧 「받는 장치의 대기 노드가 활성인가」라 트리 디버거에 그대로 보인다. 이동 중 잠금은 대기 노드를 정지 상태(Idle 리프)에만 두어, 일회용은 도착 상태에 대기 노드를 두지 않아 만든다. 클라에 아직 로드되지 않은 연결 장치(다른 월드 파티션 셀)는 상태를 알 수 없으니 프롬프트를 열어 둔다 — 받을지는 서버가 다시 검증한다(2026-09-21 사용자 확정). 다른 장치를 잠그는 노드·회신 신호·버튼의 Locked 상태는 두지 않는다. `Device.Locked` 는 스스로 잠기는 단독 장치용 공용 상태 태그다.
- **수락 규칙과 엘리베이터**: 규칙은 「이 보낸 장치에게서 무엇을 받아 줄지」를 선택지 목록(`FWxDeviceTriggerRule::GetAcceptedOptions`)으로 답하고, 같은 답이 미는 장치의 상호작용 목록과 서버의 수락 검증에 함께 쓰인다. `정차 지점 선택`(`FWxDeviceTriggerRule_SplineStops`)은 정차 지점을 스플라인 포인트로 본다 — 탑승칸 밖 버튼은 가장 가까운 지점 하나(이미 거기면 잠김), 탑승칸에 붙은 버튼은 현재 지점을 뺀 전부를 「N층」 행으로 내놓고, 「지금 층 포함」을 켠 규칙(Inactive 상태용)은 같은 층 호출도 받아 제자리에서 깨어난다. 고른 값은 `AWxDevice::SelectedOptionValue` 에 남고 `스플라인 이동` 이 `Actor.SelectedOptionValue` 바인딩으로 읽는다(음수면 제자리). `스플라인 이동` 은 재선택에도 다시 진입한다 — 클라가 이동을 보지 못한 채 같은 상태의 새 진입만 받는 경우(네트워크 컬 거리 밖에 있다 돌아옴)에 새 값으로 탑승칸을 다시 맞추기 위해서다. 늦은 접속·복원의 위치는 Idle 상태에 둔 즉시 스냅 `스플라인 이동`(완료 판정 제외)이 맞춘다. 값을 이벤트 페이로드가 아니라 액터 프로퍼티로 나르는 것은 클라가 이벤트 없이 스냅샷으로 상태에 진입하기 때문이며, 그래서 `FWxDeviceStateSnapshot::SelectedOptionValue` 로 복제된다. 새 규칙이 필요하면 `FWxDeviceTriggerRule` 파생 구조체를 추가한다 — 노드는 늘리지 않는다.
- **장치끼리 잇기**: 버튼→문처럼 다른 장치를 미는 경로는 `연결 장치 작동`(`FWxStateTreeTask_TriggerLinkedDevices`) 하나뿐이다. 저작할 값이 없는 노드로, 오너의 `LinkedDevices` 전부를 작동시키며 보낸 장치와 그 장치가 받은 선택지 값을 함께 넘긴다. 대상은 레벨 배치가 정하는 `LinkedDevices` 이고, 자식 장치는 BeginPlay 에 부모 장치를 자기 `LinkedDevices` 에 넣는다.
- **자식 액터 장치의 네트워크 주소**: `ChildActorComponent` 로 붙인 장치(엘리베이터 호출 버튼)는 클라에서 맵 로드 뒤에 다시 스폰되어 「맵에서 로드된 액터」 표시가 없는데, 서버는 같은 이름의 액터를 경로로 주소 지정한다. 그대로 두면 클라가 이 장치를 가리키는 RPC(`ServerInteract`)를 직렬화하다 엔진 assert(`NetGUID.IsDynamic()`, PackageMapClient.cpp)로 죽는다. 그래서 `AWxDevice::PostActorCreated` 가 부모 액터가 경로로 주소 지정되는 경우에 한해 엔진의 `AActor::SetNetAddressable()` 을 부른다(엔진 계약: 서버·클라 이름이 같아야 하고 `FinishSpawning` 전에 불러야 한다 — 자식 액터 이름은 컴포넌트 이름과 부모 UAID 에서 결정적으로 만들어진다). 런타임에 스폰된 부모의 자식은 표시하지 않는다 — 클라가 직접 만들지 않고 동적 액터로 복제받기 때문이다.
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT + `Category = "Wx"` 의 `DisplayName` 메타. 인스턴스 데이터를 별도 USTRUCT 로 두고 `GetInstanceDataType()` 만 헤더에 남긴다(코딩 규칙 3의 명시 예외).
- **레벨 액터 지목**: 태스크가 배치 액터를 가리킬 때는 직접 참조 대신 `FUniversalObjectLocator` 를 쓴다 — ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않아 레벨 밖 호스트(퀘스트 ST)에서도 조립된다. 스포너 지정은 `FWxSpawnerLocatorUtils`([Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.h](../../../../Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.h))로 해석·컴파일 검증을 통일한다.
- **스폰 대상**: `AWxSpawner::SpawnableActorClass` 는 `MustImplement` 로 `IWxSpawnable` 을 강제한다. 스포너는 Deferred Spawn 의 `FinishSpawning` 이전에 대상의 처치 통지(`GetOnKilledDelegate`)를 구독하고, 스폰 Owner 로 자신을 넘긴다 — 대상의 `UWxAIBehaviorComponent` 가 빙의 전 초기화에서 이것으로 정찰 경로를 찾는다.
- **장치 태스크 작성 규칙**: 장치 트리는 서버와 클라가 같은 에셋을 각자 돌리므로, 장치 트리에 놓이는 태스크는 전부 양쪽에서 실행된다(엔진 순정 Delay 등도 같다). 그래서 세 가지를 지킨다. ① 서버에서만 해야 하는 일(스폰·GE 적용·체크포인트 기록·연결 장치 작동)은 `HasAuthority` 로 막는다. ② 복원 진입(트리 시작·늦은 접속·건너뛴 통지)이면 일회성 효과를 건너뛰고 이동은 스냅한다 — `FWxDeviceExecutionPolicy::IsRestoring*` 로 가른다. ③ 서버만 아는 정보로 완료되는 태스크는 클라에서 먼저 완료되면 안 된다 — 클라에서는 Running 으로 남아 서버 통지로 넘어간다(`작동 대기` 가 이 모양이다). `애니메이션 재생` 은 의도적으로 ②를 따르지 않아 늦은 접속자도 연출을 한 번 본다.
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(`Config = Game`, "Wx World Settings")가 스포너 클래스별 에디터 아이콘 매핑을 들고 있다.
- **리플리케이션/권한(최대 4인)**: 서버는 활성 태그 상태가 바뀔 때마다 `FWxDeviceStateSnapshot`(태그·일련번호·당사자·선택지 값)을 발행하고, 클라는 그 통지를 받은 순간에만 판단한다(2026-09-22 사용자 확정) — 일련번호가 직전+1 이고 이미 그 태그면 무동작(제 타이머로 먼저 도착), 직전+1 인데 다른 태그면 라이브 전이, 첫 수신이거나 번호가 건너뛰었으면 복원 전이다. 통지 사이에 클라가 제 타이머로 앞서가도 되감지 않으며, 클라가 어긋나 있어도 다음 통지 전에는 스스로 고치지 않는다. 같은 태그로의 재진입과 한 틱 안에 지나간 태그 상태는 발행되지 않으므로, 클라에 보여야 할 연출은 잠깐이라도 머무는 별도 태그 상태에 둔다(체크포인트의 Resting). 서버만 트리를 돌리고 결과를 복제하는 안은 이동·프롬프트·연출마다 복제 통로가 필요해 기각했다. 반면 스포너의 처치 상태(`bIsKilled`)는 서버 런타임 값이라 복제되지 않으므로, 스포너 계열 태스크와 `상호작용 대기`는 권위에서 구동되는 트리 전용이다. 스캐너는 반대로 소유 클라 전용(데디 서버 PC 는 스캔하지 않음)이다.

## 여기서부터 읽어라

1. [Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h) — 장치가 밖에 내보이는 계약이 전부 여기 있다. 무엇이 액터에 남고 무엇이 컴포넌트로 갔는지 여기서 갈린다.
2. [Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h) — 서버 발행(`PublishState`)→클라 수신 판단(`OnRep_StateSnapshot`)→상태 진입 요청(`EnterState`) 흐름. 멀티 동기화 문제는 대부분 이 파일과 `Private/Device/WxDeviceStateTreeComponent.cpp` 에서 끝나고, `LogWxWorld` Verbose 의 publish/receive/request 세 줄로 따라간다.
3. [Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h) — 스캔부터 서버 RPC 까지, 모듈 경계를 넘나드는 상호작용 흐름 전체가 doc-comment 에 정리돼 있다.
4. [Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h) — 대기형 태스크의 공통 골격. `WxStateTreeTask_WaitForInteraction.cpp` 와 함께 보면 "틱 없이 기다리다 통보로 완료" 패턴이 잡힌다.
5. [Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h](../../../../Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h) — 스포너 3종 태스크(발동·처치 대기·일괄 리스폰)가 모두 이 액터의 상태를 읽고 쓴다.

## 관련

- 상위: [Source/WxGame](../../../../Source/WxGame) — PlayerController 가 스캐너를 소유하고, `WxAbility_Interact` 가 권위 판정 후 이 모듈에 통보하며, 리스폰 경로가 체크포인트를 읽는다.
- 함께 보기: [WxCore](../foundation/index.md) — `IWxInteractable` 상호작용 인터페이스의 정의처.
- 에디터 지원: [Source/WxEditor](../../../../Source/WxEditor) — 장치 컴포넌트 이름 드롭다운 커스터마이제이션.

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../../index.md) · [운영 절차](../../wiki-maintenance.md)

## 검증 범위와 근거

[Build.cs](../../../../Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs)·[descriptor](../../../../Plugins/WxWorld/WxWorld.uplugin), Device·DeviceStateTreeComponent 공개 계약과 [상태 동기화 구현](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp), [장치 상호작용](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp)을 확인했다.

- 스냅샷은 상태 태그명·일련번호·상호작용 캐릭터·선택지 값을 담는다. 클라이언트는 통지를 받은 순간에만 상태 전이를 요청한다(엔진의 `FStateTreeExecutionContext::RequestTransition` 외부 요청 — 트리 디버거에 출처가 ExternalRequest 로 남는다). 초기/복원 진입과 라이브 전이는 구분되며 개별 태스크가 그 정책을 올바르게 사용하는지는 실제 조합별 확인이 필요하다.
- [Spawner](../../../../Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp): 서버 지연 스폰, 기존 인스턴스·처치 상태 검사, 처치 통지 구독 이후 `FinishSpawning`, `bNeverRevive` 복원 분기. 통지 발행 측은 [Source/WxGame/Character/WxEnemyCharacter.cpp](../../../../Source/WxGame/Character/WxEnemyCharacter.cpp)의 서버 사망 처리다.
- [Scanner](../../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp)·[WaitForInteraction](../../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp): 소유 로컬 컨트롤러의 스캔과 RPC 요청, 권위 어빌리티의 상호작용 실행 후 대기 통지.
- [Checkpoint](../../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp): Standalone에서 월드 패키지별 부활 위치를 보관한다. 디스크 영속 저장을 제공한다는 뜻은 아니다.

- 장치 단일 신호/수신측 판단 재설계(2026-09-21, 작업 트리): [WxDevice.cpp](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp)·[작동 대기](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_WaitForTrigger.cpp)·[수락 규칙](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp)·[스캐너](../../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp)를 작성하며 확인했고, WxEditor Development 빌드 성공, `Content/WorldObject/Gimmick` 의 ST 6종(Button·Door·Piston·TreasureChest·CheckPoint·Elevator)을 에디터 MCP 로 이관·컴파일했다. `LV_DevCombat` PIE 에서 장치 13기가 오류 없이 기동해 초기 상태를 발행하는 것까지 확인했다. 실제 상호작용 입력(버튼 누름·층 선택·이동 중 잠금·멀티 클라 표시)은 미검증이다.

- 장치 동기화 단순화(2026-09-22, 작업 트리): [동기화 구현](../../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp)을 「서버는 태그가 바뀔 때 발행, 클라는 통지받은 순간에만 판단」으로 다시 썼다 — 실행 확장 교체·진입 관측·매 틱 대조·재시도·실행 상태 복제를 걷어냈다. `ST_CheckPoint` 는 에디터 MCP 로 Unlit → (Activated: 불꽃 나이아가라) Resting[`Device.CheckPoint.Resting`, 쉬기 효과 + Delay 1초] → Lit 구조로 바꿨다 — 같은 태그 재진입은 발행되지 않고, 이전의 Lit→Lit 강제 재진입은 인스턴스 데이터를 새로 만들어 쉴 때마다 불꽃을 하나씩 더 띄웠기 때문이다(전이 대상보다 앞선 부모 상태는 전이의 영향을 받지 않아 불꽃이 한 번만 뜬다). WxEditor Development 빌드 성공, `LV_DevCombat` 단일 PIE 와 리슨 서버+클라 2인 PIE 에서 장치 13기의 스냅샷이 서버·클라 월드에서 일치하고 `InitialState` 가 On 인 피스톤 2기가 클라에서도 On 위치로 복원되는 것까지 확인했다. 작동 신호 이후의 라이브 전이(버튼·엘리베이터·체크포인트 쉬기), 컬 거리 복귀, 늦은 접속은 미검증이다.

장치별 BP, 모든 이동·연출 태스크의 실행 결과와 에디터 UI는 이번 검증 범위 밖이다.

*이관 원문의 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 57파일 — 원문 출처 보존; 현재 확인 범위는 이 참고 정보 참고*

</details>
