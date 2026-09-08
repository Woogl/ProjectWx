# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓이는 장치(문·상자·엘리베이터·발동 장치)와 스포너, 그리고 플레이어가 그것들과 상호작용하는 감지·선택 경로를 담당한다. 장치의 상태는 StateTree 로 구동하고, 그 결과만 네트워크로 복제한다.

## 책임
**담당**
- **월드 장치**: `AWxDevice` 를 호스트로, `UWxDeviceStateTreeComponent` 가 StateTree 로 장치 상태를 실행·소유하고 StateTag 스냅샷을 복제한다.
- **상호작용 감지·선택**: `UWxInteractionScannerComponent` 가 소유 클라에서 주변 후보를 스캔하고 선택 인덱스를 관리한다.
- **스폰 관리**: `AWxSpawner`/`IWxSpawnable` 이 스폰과 처치 상태를, 장치용 StateTree 태스크군이 저작 시 조립하는 동작(이벤트 보내기·스포너 발동·애니메이션·사운드·시퀀스 등)을 제공한다.
- **부가 시스템**: 싱글플레이 체크포인트(`UWxCheckpointSubsystem`), 스포너 리스폰 라이브러리, 에디터 프리뷰용 개발자 설정.

**경계 (비담당)**
- `IWxInteractable` 계약 자체와 `Event.Interact`/`Ability.Interact` 태그는 [[WxCore]] 소유. 이 모듈은 구현·발행만 한다.
- 상호작용의 권위 검증 어빌리티(사거리·활성 판정, ServerOnly)는 GAS 어빌리티로 [[WxCombat]]에 있다. 스캐너는 폰 ASC 로 이벤트만 송출한다.
- 상호작용 목록 표시(HUD 뷰모델)는 [[WxUI]]가 스캐너의 목록·선택을 구독해 담당한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 액터의 공통 호스트. 상호작용 표면(IWxInteractable)·프롬프트·배선(LinkedDevices)만 갖고 상태 구동은 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태의 실행·소유·복제 담당. StateTag 스냅샷 관측/복제로 클라 동기화 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController 에 붙어 소유 클라에서 후보 스캔·선택·하이라이트·ServerInteract 송신 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 레벨 배치 스폰 지점. 스폰 대상 클래스·처치 상태·리스폰 정책 보유 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 액터가 구현하는 콜백 계약(`OnSpawnedBy`, FinishSpawning 이전 호출) | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_SendEvent` | 장치 저작 태스크의 대표. 권위 측에서 Linked/Child 대상 장치 트리로 이벤트 발행 | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SendEvent.h` |
| `TWxStateTreeWaitRegistry` | 통보까지 Running 을 유지하는 대기형 태스크들의 공용 등록부(템플릿) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `UWxCheckpointSubsystem` | 맵 재시작 사이 유지되는 싱글플레이 부활 지점 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 를 상속한 BP 로 만든다(Abstract, 루트 미생성 — BP 가 몸통을 세운다). 상태·연출은 `UWxDeviceStateTreeComponent` 가 실행하는 StateTree 에셋에서 저작하고, 장치 사이 배선은 배치 인스턴스의 `LinkedDevices` 로 잇는다.
- **새 장치 동작**: `StateTreeTask/` 아래 `FWxStateTreeTask_*`(FStateTreeTaskCommonBase 파생) 를 추가한다. `GetInstanceDataType()` 인라인 정의와 대기형 태스크의 페이로드 템플릿은 코딩 규칙 6 의 명시된 예외다(각 파일 주석 참조).
- **새 스폰 대상**: 액터가 `IWxSpawnable` 을 구현하면 `AWxSpawner.SpawnableActorClass` 후보가 된다. 스포너는 UOL(`FUniversalObjectLocator`)로 지정해 레벨 밖 호스트(퀘스트 ST)에서도 발동할 수 있다.
- **리플리케이션/권한**: 장치 활성은 각 피어에서 실행되는 StateTree 가 결정하고, 권위 측이 StateTag 스냅샷만 복제하면 클라가 이를 따라간다(연출은 큐잉 없이 최신 진입으로 수렴). 스폰·처치·리스폰은 전부 서버 권위 호출이며 클라 호출은 내부에서 무시된다. 상호작용 감지·선택·하이라이트는 소유 클라 로컬 어포던스로, ServerInteract 수신만 서버에서 실행된다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치 호스트와 상호작용 표면·바인딩 구조. 모듈 전체의 중심.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 장치 상태의 실행·복제·클라 동기화 모델(스냅샷/시리얼).
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용의 전 경로(스캔→선택→ServerInteract→ASC 이벤트)를 doc-comment 로 개괄.
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치 상태 모델과 리스폰 정책.

## 관련
- 상위: [[WxCore]](공용 정의·`IWxInteractable`·태그)
- 협력: [[WxCombat]](권위 상호작용 어빌리티), [[WxUI]](상호작용 목록 HUD), [[WxQuest]](레벨 밖 StateTree 호스트)

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 57파일 — `/readme-writer`로 갱신*
