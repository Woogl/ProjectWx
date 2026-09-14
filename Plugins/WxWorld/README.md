# WxWorld — 월드 오브젝트 및 상호작용 시스템

> 문·상자·체크포인트·발동 장치 같은 월드 장치를 StateTree 로 구동하고, 플레이어의 상호작용 감지·선택과 스포너·부활을 담당하는 도메인 플러그인. 장치 상태 저작·실행·복제, 상호작용 표면(IWxInteractable), StateTree 태스크 팔레트가 한데 모여 있다.

## 책임
**담당**
- StateTree 로 자기 상태를 구동하는 월드 장치의 공통 호스트(`AWxDevice`)와 그 상태 실행·복제(`UWxDeviceStateTreeComponent`).
- 소유 클라 측 상호작용 감지·선택·하이라이트와 서버 상호작용 RPC(`UWxInteractionScannerComponent`).
- 스포너 배치 액터의 스폰·처치 상태 보유(`AWxSpawner`)와 스폰 대상 계약(`IWxSpawnable`).
- 싱글플레이 부활 지점 보관(`UWxCheckpointSubsystem`).
- 장치·상호작용·스포너·연출을 엮는 StateTree 태스크 팔레트(`Source/WxWorld/.../StateTreeTask`, `Interaction`, `Spawnable`).

**경계 (비담당)**
- 상호작용 권위 검증·사거리 판정은 폰 ASC 의 상호작용 어빌리티([[WxCombat]] 계열, `Ability.Interact`)가 맡고, 이 모듈은 `Event.Interact` 를 송출·수신만 한다.
- HUD 리스트 표시·입력 바인딩은 [[WxUI]] 뷰모델·위젯이 담당(스캐너는 목록·선택만 제공).
- 공용 정의(`IWxInteractable`, `WxGameplayTags`, `WxLocatorUtils`)는 [[WxCore]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | StateTree 구동 월드 장치의 공통 호스트. `IWxInteractable` 상호작용 표면과 `LinkedDevices` 배선만 보유, 상태는 컴포넌트에 위임 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태의 실행·소유(복제 StateTag 스냅샷)·ST 에셋 저작. 권위 진입을 관측해 클라에 동기화 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | `AWxPlayerController` 에 붙는 소유 클라 상호작용 스캐너. 반경 스캔·선택·하이라이트, `ServerInteract` 로 선택 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 스폰 대상 인스턴스를 세우고 처치 상태를 자체 보유하는 레벨 배치 액터. Auto/Manual 모드, `bNeverRevive` | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 계약. `OnSpawnedBy` 로 FinishSpawning 이전 초기화 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxCheckpointSubsystem` | 맵 재시작 동안 유지하는 싱글플레이 부활 지점(액터 참조 미보관) | `Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSubsystem.h` |
| `TWxStateTreeWaitRegistry` | Running 대기 태스크(상호작용 대기·스포너 처치 대기)의 공용 등록부 템플릿. 약한 실행 컨텍스트로 완료 통보 | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 컴포넌트를 이름으로 지목하는 순수 구조체(드롭다운 픽커) | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceComponentName.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 를 파생한 BP 로 만든다(Abstract, 루트 미생성 — 파생이 몸통을 세운다). 상태 흐름은 `UWxDeviceStateTreeComponent` 의 ST 에셋에 저작하고, 장치 간 연동은 `LinkedDevices`(배치) 또는 내장 자식 장치(저작)를 `FWxStateTreeTask_SendEvent` 로 민다.
- **새 스폰 대상**: `IWxSpawnable` 을 구현하면 `AWxSpawner.SpawnableActorClass` 후보가 된다(`MustImplement` 로 강제).
- **새 StateTree 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT 로 추가. `GetInstanceDataType()` 헤더 정의와 Running 대기용 템플릿은 코딩 규칙 4 의 예외(해당 지점 주석 참조). 레벨 배치 액터 지목은 `FUniversalObjectLocator`(순수 구조체라 ST 컴파일러 검증 우회, 레벨 밖 호스트에서도 조립 가능)로 한다.
- **데이터 주도**: 스포너 아이콘 매핑은 `UWxWorldDeveloperSettings`(Config=Game). `UWxSpawnerLibrary::TryRespawnAll` 로 일괄 리스폰(서버 권위, Manual 제외).
- **동기화 모델**: 장치 상태는 복제하지 않고 각 피어에서 ST 를 실행해 수렴시키며, 권위 상태 스냅샷만 복제한다. 일회성 효과와 상태 적용은 `FWxDeviceExecutionPolicy` 로 복구/실제 진입을 구분한다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치 액터가 무엇을 들고 무엇을 위임하는지. 상호작용 표면과 컴포넌트 분업의 출발점.
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 상태 실행·복제·동기화 모델의 핵심. 이 모듈에서 가장 정교한 부분.
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 상호작용 제어 흐름(스캔→선택→ServerInteract→어빌리티) 전체 경로가 doc-comment 에 정리돼 있다.

## 관련
- 상위: [[WxCore]]
- 협력: [[WxCombat]](상호작용 어빌리티), [[WxUI]](상호작용 HUD), [[WxQuest]](퀘스트 ST 에서 대기·스포너 태스크 조립)

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 57파일 — `/readme-writer`로 갱신*
