# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 장치(문·상자·엘리베이터·체크포인트)와 스포너, 그리고 플레이어가 그것들을 집어 드는 상호작용 표면을 담당한다. 장치의 상태는 StateTree 에셋으로 저작하고, 이 모듈은 그 트리를 구동·복제하는 런타임과 트리에서 쓸 태스크 라이브러리를 제공한다.

## 책임

**담당**
- 월드 장치의 상태 구동: StateTree 실행, 상태 태그 기반 식별, 서버 상태의 클라 복원(`UWxDeviceStateTreeComponent`)
- 상호작용 후보 스캔·선택·하이라이트와 서버로의 상호작용 요청 송신(`UWxInteractionScannerComponent`)
- 배치형 스폰과 처치/리스폰 상태 보유(`AWxSpawner`), 그 상태를 읽고 쓰는 ST 태스크들
- 체크포인트(싱글플레이 부활 지점) 기록·조회 저장소
- 장치 트리 저작에 쓰는 ST 태스크 전반(이동·연출·이벤트 전달·대기)

**경계 (비담당)**
- 상호작용의 권위 판정과 실제 실행: 서버는 `Event.Interact` 로 어빌리티(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`)를 띄우고, 그 어빌리티가 사거리·활성 검증 후 [[WxCore]]의 `IWxInteractable` 을 호출한다. 이 모듈은 요청을 보내고 인터페이스를 구현할 뿐이다.
- 상호작용 목록 HUD: 프롬프트 배열·선택 인덱스를 델리게이트로 내보낼 뿐, 표시는 `Source/WxGame/MVVM/WxViewModel_InteractionList.h` 와 위젯 쪽([[WxUI]]) 몫이다.
- 장치 컴포넌트 이름 드롭다운 등 에디터 UI 커스터마이제이션: `Source/WxEditor/WxStateTreeComponentNameCustomization.h`
- 캐릭터 리스폰 정책: 체크포인트 값을 읽어 쓰는 쪽은 `Source/WxGame/Framework/WxRespawnLibrary.cpp` 다.

## 핵심 타입 (진입점)

| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 계열의 루트. 상호작용 표면(`IWxInteractable`)과 배치 배선(`LinkedDevices`)만 들고 상태 실행은 아래 컴포넌트에 전부 위임한다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 모듈에서 가장 복잡한 지점. 트리 실행 + 상태 태그 복제 + 클라 복원을 혼자 쥔다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 액터의 컴포넌트를 지목하는 유일한 저장 형태. 이동·연출 태스크들이 공통으로 쓴다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h` |
| `UWxInteractionScannerComponent` | 상호작용 파이프라인의 클라 측 시작점. PlayerController 에 붙어 스캔→선택→`ServerInteract` 까지 간다 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `FWxStateTreeTask_WaitForInteraction` | 모듈 밖 권위 경로가 들어오는 문. `NotifyInteracted`/`IsAwaited` 정적 함수가 WxGame 쪽에서 불린다 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h` |
| `TWxStateTreeWaitRegistry` | 폴링 없이 대기하는 태스크들이 공유하는 등록부 템플릿 | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `AWxSpawner` | 스폰 대상의 수명과 처치 상태를 쥐는 배치 액터. 대상이 구현할 `IWxSpawnable` 계약은 [[WxCombat]] 소환 노티파이도 같은 필터로 쓰므로 [[WxCore]]에 있다 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h` |
| `UWxCheckpointSubsystem` | 체크포인트 태스크가 쓰고 WxGame 리스폰 경로가 읽는 GameInstance 저장소 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |

## 확장 포인트 / 규약

- **새 장치**: `AWxDevice` 파생 BP + StateTree 에셋. 루트 컴포넌트는 BP 가 세운다. 상태 식별자는 루트 에셋의 상태 태그이며, 컴포넌트의 `InitialState` 가 시작 상태를 정한다. 상호작용을 여는 것은 상태다 — `상태 상호작용`(`FWxStateTreeTask_EnableInteraction`) 태스크가 켜진 상태에서만 프롬프트와 발행 자리가 생긴다.
- **장치끼리 잇기**: 버튼→문처럼 다른 장치를 미는 경로는 `이벤트 보내기`(`FWxStateTreeTask_SendEvent`) 하나뿐이다. 대상은 레벨 배치가 정하는 `LinkedDevices` 또는 오너 BP 에 심긴 자식 장치(`FWxStateTreeComponentName`)로 고른다.
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT + `Category = "Wx"` 의 `DisplayName` 메타. 인스턴스 데이터를 별도 USTRUCT 로 두고 `GetInstanceDataType()` 만 헤더에 남긴다(코딩 규칙 3의 명시 예외).
- **레벨 액터 지목**: 태스크가 배치 액터를 가리킬 때는 직접 참조 대신 `FUniversalObjectLocator` 를 쓴다 — ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않아 레벨 밖 호스트(퀘스트 ST)에서도 조립된다. 스포너 지정은 `FWxSpawnerLocatorUtils`(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnerLocatorUtils.h`)로 해석·컴파일 검증을 통일한다.
- **스폰 대상**: `AWxSpawner::SpawnableActorClass` 는 `MustImplement` 로 `IWxSpawnable` 을 강제한다. `OnSpawnedBy` 는 Deferred Spawn 의 `FinishSpawning` 이전 타이밍이다.
- **복원 vs 라이브**: 진입 시 일회성 효과(이벤트 송신·스폰 트리거·체크포인트 기록)를 낼지는 `FWxDeviceExecutionPolicy::IsRestoring*` 로 가른다. 새 태스크가 부작용을 낸다면 이 구분을 반드시 통과시켜야 한다.
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(`Config = Game`, "Wx World Settings")가 스포너 클래스별 에디터 아이콘 매핑을 들고 있다.
- **리플리케이션/권한(최대 4인)**: 장치 상태는 `FWxDeviceStateSnapshot` 복제로 전파되고 클라는 그 스냅샷을 따라간다. 반면 스포너의 처치 상태(`bIsKilled`)는 서버 런타임 값이라 복제되지 않으므로, 스포너 계열 태스크와 `상호작용 대기`는 권위에서 구동되는 트리 전용이다. 스캐너는 반대로 소유 클라 전용(데디 서버 PC 는 스캔하지 않음)이다.

## 여기서부터 읽어라

1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치가 밖에 내보이는 계약이 전부 여기 있다. 무엇이 액터에 남고 무엇이 컴포넌트로 갔는지 여기서 갈린다.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 상태 관측(`ObserveActiveState`)→권위 발행(`PublishAuthorityState`)→클라 추종(`FollowAuthorityState`) 흐름. 멀티 동기화 문제는 대부분 이 파일과 `Private/Device/WxDeviceStateTreeComponent.cpp` 에서 끝난다.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔부터 서버 RPC 까지, 모듈 경계를 넘나드는 상호작용 흐름 전체가 doc-comment 에 정리돼 있다.
4. `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` — 대기형 태스크의 공통 골격. `WxStateTreeTask_WaitForInteraction.cpp` 와 함께 보면 "틱 없이 기다리다 통보로 완료" 패턴이 잡힌다.
5. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스포너 3종 태스크(발동·처치 대기·일괄 리스폰)가 모두 이 액터의 상태를 읽고 쓴다.

## 관련

- 상위: `Source/WxGame` — PlayerController 가 스캐너를 소유하고, `WxAbility_Interact` 가 권위 판정 후 이 모듈에 통보하며, 리스폰 경로가 체크포인트를 읽는다.
- 함께 보기: [[WxCore]] — `IWxInteractable` 상호작용 인터페이스의 정의처.
- 에디터 지원: `Source/WxEditor` — 장치 컴포넌트 이름 드롭다운 커스터마이제이션.

---
*문서 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 57파일 — `/readme-writer`로 갱신*
