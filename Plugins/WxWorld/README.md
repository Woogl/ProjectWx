# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되어 플레이어와 맞물리는 것들을 담당한다. StateTree로 자기 상태를 굴리는 장치(문·상자·엘리베이터·체크포인트), 그 장치를 저작할 때 쓰는 ST 태스크 묶음, 적을 세우고 처치 상태를 들고 있는 스포너, 그리고 플레이어 쪽 상호작용 대상 스캔·선택이 여기에 있다.

## 책임
**담당**
- 장치 액터의 상태 구동과 서버→클라 상태 동기화(`AWxDevice` + `UWxDeviceStateTreeComponent`). 상태 태그·진입 시리얼 스냅샷을 복제하고, 클라는 그 스냅샷으로 수렴한다.
- 장치 ST 에셋을 조립하는 태스크 라이브러리(`Public/StateTreeTask/`, `Public/Interaction/`, `Public/Spawnable/`). 이동·연출·입력 토글·이벤트 전달·상호작용 게이트가 전부 노드로 나와 있다.
- 상호작용 "표면": 대상이 지금 상호작용 가능한지, 어떤 프롬프트를 띄울지, 눌렸을 때 어디로 흘리는지.
- 소유 클라의 상호작용 후보 수집·선택·하이라이트와 서버 RPC 송출(`UWxInteractionScannerComponent`).
- 스포너와 그 처치/리스폰 상태(`AWxSpawner`), 맵 재시작 간 유지되는 싱글플레이 부활 지점(`UWxCheckpointSubsystem`).

**경계 (비담당)**
- `IWxInteractable` 인터페이스 자체와 `WxGameplayTags`(`Event.Interact`, `Ability.Interact`)의 선언은 [[WxCore]]에 있다. 이 모듈은 구현·사용만 한다.
- 상호작용의 **권위 판정**(사거리·활성 검증 후 대상 인터페이스 호출, `FWxStateTreeTask_WaitForInteraction::NotifyInteracted` 호출)은 [[WxGame]]의 `WxAbility_Interact`가 한다. 스캐너는 선택만 보내고 판정하지 않는다.
- 프롬프트 목록을 실제로 그리는 HUD 위젯·뷰모델과 Enhanced Input 바인딩은 [[WxUI]] 쪽이다. 스캐너는 델리게이트로 목록/선택만 밀어낸다.
- 픽업·NPC·대화 액터는 각자 `IWxInteractable`을 구현한다([[WxInventory]]의 `AWxItemPickup`, [[WxDialogue]]의 `AWxDialogueActor`). 장치가 아닌 상호작용 대상을 여기에 추가하지 않는다.
- 스폰된 적의 전투·사망 처리는 [[WxGame]]의 `AWxEnemyCharacter`가 하고, 죽을 때 자기 스포너에 `MarkKilled()`를 되돌려 준다.
- ST 노드의 에디터 커스터마이제이션(`FWxStateTreeComponentName` 드롭다운 등)은 `Source/WxEditor`에 있다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 계보의 루트. 남는 것은 상호작용 표면과 배치 배선(`LinkedDevices`)뿐이고 상태 구동은 아래 컴포넌트에 넘긴다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 이 모듈에서 유일하게 어려운 곳. ST 실행 관측 + 상태 스냅샷 복제/추종이 전부 여기 모인다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 플레이어 쪽 절반. PlayerController에 붙어 후보 수집→선택→`ServerInteract`까지가 여기서 끝나고 판정은 밖으로 넘어간다 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `FWxStateTreeTask_WaitForInteraction` | 모듈 밖에서 들어오는 통보의 수신구. `NotifyInteracted`/`IsAwaited` 정적 함수가 WxGame의 어빌리티·NPC와 맞물린다 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h` |
| `TWxStateTreeWaitRegistry` | 폴링 없이 대기하는 태스크들의 공용 등록부. 대기형 노드를 새로 만들 때 베끼는 자리 | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 컴포넌트를 지목하는 유일한 저장 형태. 이동·연출 태스크가 대상 컴포넌트를 받는 통로 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h` |
| `AWxSpawner` | 스폰 대상과 처치 상태를 들고 있는 배치 액터. ST 태스크 3종과 `UWxSpawnerLibrary`가 모두 이 하나를 조작한다 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 만족해야 하는 계약. 스포너가 `SpawnableActorClass`에 `MustImplement`로 강제한다 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |

## 확장 포인트 / 규약
- **새 장치를 추가**하려면 `AWxDevice`를 상속한 BP를 만들고(루트·몸통은 BP가 세운다), `UWxDeviceStateTreeComponent`에 ST 에셋과 `InitialState`를 물린다. 상태 식별은 루트 에셋의 **상태 태그**가 담당하며, 태그 없는 하위 시퀀스는 태그 달린 상위 상태의 진입 안에서 돈다.
- **새 ST 태스크를 추가**할 때는 `FStateTreeTaskCommonBase`를 상속하고 `Public/StateTreeTask/` 규약을 따른다: 인스턴스 데이터 구조체 분리, `GetInstanceDataType()` 헤더 인라인(코딩 규칙 4의 명시 예외), `#if WITH_EDITOR GetDescription()`, `DisplayName`은 한글. 대기형이면 `TWxStateTreeWaitRegistry`, 레벨 액터 지정이면 `FUniversalObjectLocator`(ST 컴파일러의 레벨 액터 참조 검증을 피하는 이유), 장치 컴포넌트 지정이면 `FWxStateTreeComponentName`을 쓴다.
- **진입 경로 구분이 이 모듈의 핵심 규약이다.** `FWxDeviceExecutionPolicy`(`Public/Device/WxDeviceExecutionPolicy.h`)로 "복원/초기 진입"과 "라이브 전이"를 가른다 — 상태 적용(문이 열린 자세)은 복원에서도 하고, 일회성 효과(컷신·몽타주·GE·리스폰·이벤트 송출)는 라이브 전이에서만 한다. 새 태스크를 만들 때 먼저 결정할 것.
- **리플리케이션(최대 4인)**: 장치 상태는 서버 권위 → `FWxDeviceStateSnapshot` 복제 → 클라 추종. 연출(애니메이션·시퀀스·사운드·나이아가라)은 복제하지 않고 각 피어가 진입 시 로컬 재생한다. 상호작용 바인딩·프롬프트도 복제하지 않는다(각 피어의 ST가 같은 값에 수렴). 반면 스포너의 `bIsKilled`는 복제되지 않는 서버 런타임 상태라 그것을 보는 태스크는 권위 ST 전용이다.
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(Config=Game)의 `SpawnerClassIcons`가 스폰 대상 클래스별 에디터 아이콘을 정한다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치의 계약이 얼마나 얇은지, 무엇이 컴포넌트로 넘어갔는지가 한 번에 보인다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 이 모듈에서 유일하게 상태 기계가 복잡한 곳. 스냅샷 필드와 `Synchronize`/`FollowAuthorityState` 계열 함수 이름만 훑어도 동기화 모델이 잡힌다.
3. `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp` — 상호작용이 액터에서 트리로 넘어가는 실제 경로(`OnInteracted` → `BroadcastInteractionDelegate`).
4. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 플레이어 쪽 절반과 모듈 경계(어디까지 로컬, 어디부터 서버 어빌리티인지).
5. `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SendEvent.h` — 장치끼리 미는 법(버튼→문). 여기까지 보면 태스크 묶음의 나머지는 같은 틀의 반복이다.
6. `Plugins/WxWorld/Source/WxWorld/Private/Tests/WxDeviceInteractorSyncTest.cpp` — 실제 문 BP로 동기화 규약을 검증하는 자동화 테스트. 기대 동작의 실행 가능한 명세다.

## 관련
- 상위: [[WxGame]] (상호작용 어빌리티 `WxAbility_Interact`, 스폰 대상 `AWxEnemyCharacter`, NPC), `Source/WxEditor` (ST 노드 커스터마이제이션)
- 함께 보는 모듈: [[WxCore]] (`IWxInteractable`·`WxGameplayTags`), [[WxUI]] (상호작용 HUD 리스트), [[WxInventory]]·[[WxDialogue]] (다른 `IWxInteractable` 구현체), [[WxQuest]] (퀘스트 ST가 레벨 밖 호스트로서 상호작용 대기·스포너 태스크를 쓰는 것을 전제로 태스크가 `FUniversalObjectLocator`를 쓴다)

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 58파일 — `/readme-writer`로 갱신*
