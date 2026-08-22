# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓이는 장치(문·상자·체크포인트·버튼)와 그 상호작용, 그리고 적/오브젝트를 스폰·관리하는 스포너를 담당한다. 장치 상태는 StateTree로 구동되고, StateTree 태스크 라이브러리로 연출·전파·게이트를 조립한다.

## 책임
**담당**
- 월드 장치(`AWxDevice`)의 상태 구동. 상태는 서버 권위이며 `UWxDeviceStateTreeComponent`가 StateTree 실행·상태(StateTag) 소유·복제·SaveGame 복원 수렴을 맡는다.
- 플레이어 주변 상호작용 대상 스캔·선택·하이라이트(`UWxInteractionScannerComponent`, PlayerController 부착). 서버로 선택을 전송해 권위 실행을 트리거한다.
- 스포너(`AWxSpawner`)의 스폰·처치 상태 보유·리스폰. 세이브 슬롯에 처치 상태 영속.
- 장치 트리에서 쓰는 StateTree 태스크 라이브러리(연출: 애니·사운드·Niagara·LevelSequence·컴포넌트/스플라인 이동, 전파: SendEvent, 게이트: 상호작용/스포너 대기, 스포너 제어: 트리거·리스폰).

**경계 (비담당)**
- 상호작용의 권위 실행(사거리·활성 검증, 대상 인터페이스 호출)은 스캐너가 폰 ASC로 `Event.Interact`를 송출하면 상호작용 어빌리티(WxAbility_Interact, [[WxCombat]] 계열)가 수행한다. 이 모듈은 후보 수집과 선택 전송까지만.
- 상호작용/세이브 계약 인터페이스(`IWxInteractable`·`IWxSavable`) 자체 정의는 [[WxCore]].
- 세이브 슬롯 직렬화·복원 트리거는 [[WxSave]]. 이 모듈은 SaveId 제공과 `OnSaveRestored` 처리만.
- HUD 목록 표시는 [[WxUI]] 뷰모델(UWxViewModel_InteractionList)이 스캐너 델리게이트를 구독해 담당.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 액터 — 상호작용 표면(IWxInteractable)·세이브 신원(IWxSavable)·배선(LinkedDevices)만 갖고 상태는 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기·StateTag 소유자. 서버 권위 상태 → 복제/복원 수렴의 핵심 | `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라 상호작용 스캐너. PlayerController 부착, ServerInteract RPC 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 배치 스포너 — SpawnableActorClass 인스턴스 스폰·처치 상태 보유·리스폰 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 계약. FinishSpawning 이전 `OnSpawnedBy`로 스포너 주입 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | 스포너 일괄 리스폰 등 BP 진입 함수 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `FWxComponentName` | ST 에셋이 장치의 컴포넌트를 이름으로 지목하는 픽커 구조체 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxComponentName.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 몸통(루트 컴포넌트)과 StateTree 에셋을 붙인다. AWxDevice는 루트를 만들지 않는다(파생 BP가 세운다). 버튼·레버 같은 발동 장치도 같은 클래스로, 누른 상태를 자기 트리로 몰아 SendEvent 태스크로 상대를 민다.
- **상태 구동 규약**: 상태 식별은 엔진 순정 상태 Tag. 영속이 필요한 상태에는 반드시 상태 디테일에 Tag를 달아야 세이브 슬롯에 담긴다(에셋 안에서 유일). 권위만 상태를 정하고 클라·복원은 StateTag 수렴 추종 — 자세한 패턴은 `WxDeviceStateTreeComponent.h` doc-comment.
- **새 StateTree 태스크**: `FStateTreeTaskCommonBase`를 상속하고 별도 `...InstanceData` USTRUCT + `using FInstanceDataType`를 둔다(`Public/Device/·Interaction/·Spawnable/`의 `WxStateTreeTask_*` 참조). 배치별로 달라지는 대상은 에셋 리터럴 대신 LinkedDevices 바인딩·`FWxComponentName`·`FUniversalObjectLocator`로 지목한다(공유 ST 에셋 전제).
- **게이트 태스크의 통보 규약**: 대기형 태스크(WaitForInteraction·WaitSpawnersKilled)는 폴링하지 않고 정적 `Notify...`로 완료를 통보받는다 — 각각 상호작용 어빌리티 경로와 `AWxSpawner::MarkKilled`가 호출한다. 서버 권위 ST 전용.
- **스포너 대상**: `SpawnableActorClass`는 `IWxSpawnable` 구현 액터여야 한다(MustImplement 메타). `EWxSpawnerMode::Manual`은 일괄 리스폰 제외, `bNeverRevive`는 영구 처치.
- **리플리케이션 모델**: 장치 상태는 StateTag 복제 + 재진입 멀티캐스트로 전파(태스크 값 자체는 대체로 복제 안 함, 각 피어 트리가 같은 값에 수렴). EnableInteraction 등 일부 토글 태스크는 값을 복제하지 않아 싱글/리슨 호스트 전제.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치의 계약 표면(상호작용·세이브·배선)이 무엇이고 무엇을 컴포넌트에 위임하는지.
2. `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` — 서버 권위 상태 → 복제·세이브 복원 수렴 패턴. 이 모듈의 가장 미묘한 부분.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 감지→선택→ServerInteract→어빌리티로 이어지는 흐름과 로컬리티 근거.
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰·처치·리스폰과 SaveId 확정(에디터 PreSave) 규약.

## 관련
- 상위: 상호작용 권위 실행은 [[WxCombat]]의 상호작용 어빌리티(GAS)와 짝을 이룬다. 계약 인터페이스는 [[WxCore]], 세이브 영속은 [[WxSave]], HUD 표시는 [[WxUI]]. 스캐너·컴포넌트 부착은 GameFeature/Experience 주입 설정으로 이뤄진다.

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 50파일 — `/readme-writer`로 갱신*
