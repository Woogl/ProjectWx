# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·엘리베이터 같은 월드 장치, 플레이어의 상호작용 감지·선택, 적 스폰/리스폰, 체크포인트를 담당한다. 장치의 상태는 StateTree 로 구동하고 멀티플레이에서 복제한다.

## 책임
**담당**
- `AWxDevice` 계열 월드 장치: StateTree 기반 상태 구동, 상태의 복제·동기화, 장치 간 이벤트 배선(LinkedDevices/Child)
- 소유 클라이언트의 주변 상호작용 후보 스캔·선택·하이라이트, 서버로의 상호작용 요청 전달
- 배치 액터의 적/오브젝트 스폰과 처치·리스폰 상태 보유, 그리고 이를 몰아주는 StateTree 태스크군
- 싱글플레이 부활 지점(체크포인트) 보관
- 장치 트리에서 쓰는 연출/제어 StateTree 태스크군(애니메이션·사운드·나이아가라·시퀀스·이동·이벤트 등)

**경계 (비담당)**
- `IWxInteractable` 인터페이스와 상호작용 어빌리티 계약 자체의 정의는 [[WxCore]] (스캐너·장치가 이 계약을 구현/소비만 한다)
- 상호작용 목록 HUD 표시는 뷰모델을 통해 [[WxUI]] 로 위임(스캐너는 목록·선택만 발행)
- 사거리·활성 검증 후 대상 인터페이스를 호출하는 권위 상호작용 어빌리티(`WxAbility_Interact`)는 [[WxCombat]] 쪽

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 월드 장치의 공통 호스트. `IWxInteractable` 표면·프롬프트·당사자·배선만 남기고 상태 구동은 아래 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태의 실행·소유. 복제 StateTag 스냅샷으로 클라를 권위 상태에 수렴시킨다 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라 PlayerController 에 붙어 주변 상호작용 후보를 스캔·선택하고 `ServerInteract` 로 전달 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 를 스폰하고 처치/리스폰 상태를 서버 권위로 보유하는 배치 액터 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 스포너로부터 초기화를 받는 계약(`OnSpawnedBy`) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxCheckpointSubsystem` | 맵 재시작 사이 유지되는 싱글플레이 부활 지점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |
| `TWxStateTreeWaitRegistry` | 통보가 올 때까지 Running 으로 머무는 대기 태스크들의 공용 등록부(템플릿) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 를 상속한 BP 로 몸통을 세운다(베이스는 루트를 만들지 않는다). 상태 그래프는 파생 BP 가 지정하는 StateTree 에셋에 두며, 상태 식별자는 태그 하나다. 상호작용 신호는 항상 액터가 받아 컴포넌트로 전달한다 — 스캐너·어빌리티·발동 장치가 보는 계약 상대는 액터 하나.
- **장치 상태 연출**: `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/` 의 `FWxStateTreeTask_*` 군을 상태 노드로 붙인다. 일회성 효과와 상태 적용을 구분하려면 `FWxDeviceExecutionPolicy::IsRestoring*` 을 본다(복원 진입에선 일회성 연출을 재생하지 않는다).
- **장치 간 연동**: `FWxStateTreeTask_SendEvent` 로 `LinkedDevices`(배치) 또는 `ChildDevice`(내장) 를 겨눠 권위 측에서 대상 트리에 이벤트를 보낸다. 대상 상태는 그 이벤트를 듣는 대상 에셋의 전이가 정한다.
- **대기 태스크**: 통보 도착까지 Running 으로 머무는 태스크는 `TWxStateTreeWaitRegistry` 에 등록하고 ExitState 에서 핸들로 되돌린다(상호작용 대기·스포너 처치 대기 등). 페이로드 타입이 태스크마다 달라 헤더 템플릿으로 둔다.
- **스폰 대상**: `SpawnableActorClass` 는 `IWxSpawnable` 구현이 강제된다(`MustImplement`). `OnSpawnedBy` 는 FinishSpawning 이전에 불려 빙의/BeginPlay 이전 세팅에 쓴다. 일괄 리스폰은 `UWxSpawnerLibrary::TryRespawnAll`(서버 권위, Manual 모드 제외).
- **리플리케이션 모델**: 장치는 복제 `FWxDeviceStateSnapshot` 만 흘리고 트리는 각 피어에서 실행되어 같은 상태에 수렴한다(상호작용 바인딩은 비복제). 스캔·선택·하이라이트는 소유 클라 전용 로컬 어포던스.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치 계약 표면과 상호작용 바인딩의 밑그림
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 상태 구동·복제·클라 수렴이라는 이 모듈의 핵심 패턴
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→서버 상호작용 요청의 전체 흐름(doc-comment가 상세)
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/리스폰 권위 규약

## 관련
- 상위: 상호작용 계약·인터페이스는 [[WxCore]], HUD 목록 표시는 [[WxUI]], 권위 상호작용 어빌리티는 [[WxCombat]], 장치·스포너 트리거를 상위에서 조립하는 곳은 [[WxQuest]]

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 57파일 — `/readme-writer`로 갱신*
