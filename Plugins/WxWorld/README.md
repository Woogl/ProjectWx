# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓이는 월드 장치(문·상자·체크포인트·엘리베이터·버튼)와 그 상태 구동, 플레이어의 상호작용 스캔·선택, 스포너, 그리고 이들을 엮는 StateTree 태스크 라이브러리를 책임진다.

## 책임
**담당**
- StateTree 로 자기 상태를 구동하는 월드 장치의 호스트·상태 실행·복제(StateTag 스냅샷 기반).
- 소유 클라 주변 상호작용 후보 스캔·선택·하이라이트, 서버로의 선택 전송(`ServerInteract`).
- 레벨 배치 스포너의 스폰·처치 상태 보유·리스폰, 스폰 대상 계약(`IWxSpawnable`).
- 월드 시퀀싱용 범용 StateTree 태스크군(연출·이동·이벤트·스폰·체크포인트 등)과 통보 대기 공용 등록부.
- 싱글플레이 부활 지점(체크포인트) 보관.

**경계 (비담당)**
- `IWxInteractable` 인터페이스 정의 자체 — [[WxCore]] (여기서는 구현·소비만).
- 상호작용 권위 검증·발동(GAS `Event.Interact` → `WxAbility_Interact`)과 플레이어 컨트롤러/폰 — [[WxGame]].
- 상호작용 리스트 HUD 렌더링·뷰모델 베이스 — [[WxUI]] (스캐너는 델리게이트·목록만 노출).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 액터 — 상호작용 표면(`IWxInteractable`)·프롬프트·배선(`LinkedDevices`)만 남기고 상태 구동은 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 상태 실행·소유·복제의 실체. 활성 상태를 태그로 관측해 권위→클라 수렴 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 플레이어 컨트롤러에 붙는 상호작용 스캐너·선택기 (소유 클라 전용) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 레벨 배치 스포너 — 대상 스폰·처치 상태 보유·리스폰 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `UWxSpawnerLibrary` | 서버 권위에서 스포너 일괄 리스폰(`TryRespawnAll`) 진입점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxCheckpointSubsystem` | 맵 재시작 동안 유지되는 부활 지점(액터 참조 없음) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |
| `TWxStateTreeWaitRegistry<T>` | 통보 올 때까지 Running 으로 대기하는 태스크들의 공용 등록부 (헤더 템플릿) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 를 상속한 BP 로 몸통을 세우고, StateTree 에셋의 상태를 GameplayTag 로 식별한다(태그가 유일한 상태 식별자, 하위 미태그 시퀀스는 상위 태그 진입 안에서 실행). 밖에서는 대상의 상태를 직접 쓰지 않고 이벤트로 상태를 요청만 한다.
- **새 월드 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT 로 `StateTreeTask/` 에 추가한다. 통보 대기형이면 `TWxStateTreeWaitRegistry` 를 재사용한다. `GetInstanceDataType()` 헤더 정의는 코딩 규칙 예외로 허용된다.
- **새 스폰 대상**: `IWxSpawnable` 을 구현하면 `OnSpawnedBy` 로 FinishSpawning 이전에 초기화 값을 받는다.
- **리플리케이션/권한**: 장치 상태는 복제하지 않고 각 피어에서 ST 를 실행해 스냅샷(StateTag·EntrySerial)으로 수렴시킨다. 일회성 연출은 실제 진입에서만, 상태 적용은 복원에서도 실행한다(`FWxDeviceExecutionPolicy`). 상호작용은 소유 클라에서만 스캔하고 권위 검증은 GAS 어빌리티가 한다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치의 계약 표면과 컴포넌트로의 위임 경계.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 상태 실행·복제·수렴의 핵심 패턴(모듈에서 가장 복잡).
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→`ServerInteract`→어빌리티로 이어지는 상호작용 흐름 전체가 doc-comment 에 정리됨.
4. `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` — 대기형 태스크(상호작용/스포너 처치 대기)가 공유하는 완료 통보 메커니즘.

## 관련
- 상위: 상호작용 권위 흐름과 플레이어는 [[WxGame]]의 `WxAbility_Interact`·`WxPlayerController` 가 소비하고, HUD 리스트는 [[WxUI]]가 표시한다. 상호작용 인터페이스 정의는 [[WxCore]]. 퀘스트 게이트(`상호작용 대기`)로 [[WxQuest]]와 맞물린다.

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 57파일 — `/readme-writer`로 갱신*
