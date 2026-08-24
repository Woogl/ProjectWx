# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·체크포인트·버튼 같은 배치형 월드 장치를 StateTree로 구동하고, 플레이어의 근접 상호작용 스캔·선택과 적/오브젝트 스폰·리스폰을 담당한다. 장치 상태는 서버 권위로 정해 복제·세이브된다.

## 책임
**담당**
- 장치 액터(`AWxDevice`)의 상호작용 표면과 세이브 신원, 그리고 상태머신 실행·소유(`UWxDeviceStateTreeComponent`) — 상태의 서버 권위 결정, 복제 추종, 세이브 복원 수렴.
- 장치 거동을 조립하는 StateTree Task 팔레트(`Device/`·`Interaction/`·`Spawnable/`의 `FWxStateTreeTask_*`).
- 플레이어 근접 상호작용 스캔·선택·하이라이트(`UWxInteractionScannerComponent`, PlayerController 부착).
- 스폰 지점의 스폰/처치/리스폰 상태 보유(`AWxSpawner`, `IWxSpawnable`).

**경계 (비담당)**
- 상호작용 권위 실행(사거리·활성 검증 후 인터페이스 호출)은 `Event.Interact`를 받는 ServerOnly 어빌리티 — [[WxCombat]] 쪽 GAS 자산이 맡는다. 스캐너는 이벤트 송출까지만.
- `IWxInteractable`·`IWxSavable` 인터페이스 정의는 [[WxCore]].
- 상호작용 프롬프트 리스트의 HUD 표시(뷰모델·위젯)는 [[WxUI]].
- 장치·스포너 상태의 슬롯 직렬화/복원 오케스트레이션은 [[WxSave]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 월드 장치 공통 호스트(추상). 상호작용 표면·세이브 신원·배선만 갖고, 상태 구동은 컴포넌트에 위임 | `Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 `StateTag`(복제·SaveGame)의 소유자. 권위/추종/복원 수렴의 핵심 | `Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라에서 주변 `IWxInteractable`을 주기 스캔·선택·하이라이트하고 `ServerInteract`로 서버에 전달 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 인스턴스를 스폰하고 처치/리스폰 상태를 자체 보유(SaveGame) | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 계약. Deferred Spawn의 `FinishSpawning` 이전 `OnSpawnedBy`로 초기값 주입 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_*` | 장치 거동 Task 팔레트(이벤트 보내기·컴포넌트/스플라인 이동·몽타주·시퀀스·나이아가라·스포너 트리거 등) | `Source/WxWorld/Public/{Device,Interaction,Spawnable}/` |
| `FWxStateTreeComponentName` | ST 에셋이 장치 액터의 컴포넌트를 이름으로 지목하는 저장 형태(에디터 드롭다운으로 선택) | `Source/WxWorld/Public/Device/WxStateTreeComponentName.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` 등 서버 권위 스포너 유틸(BP 라이브러리) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 몸통 컴포넌트를 세운 뒤, `UWxDeviceStateTreeComponent`의 State Tree 에셋으로 거동을 저작한다. 영속이 필요한 상태에는 반드시 상태 Tag를 달아야 저장된다(에셋 안에서 유일). 상태는 서버 권위 → 복제 → 클라 추종.
- **새 Task**: `FStateTreeTaskCommonBase`(또는 `FStateTreeTaskBase`)를 상속한 `FWxStateTreeTask_*` USTRUCT + `FWxStateTreeTask_*InstanceData` 쌍으로 추가. 인스턴스 데이터의 컴포넌트 지목은 리터럴 대신 `FWxStateTreeComponentName`이나 오너 프로퍼티 바인딩을 쓴다(공유 에셋을 여러 배치가 재사용하므로).
- **장치 간 연동**: `SendEvent` Task가 대상 장치의 트리에 이벤트를 발행한다. 대상은 배치 배선(`LinkedDevices` 바인딩) 또는 저작이 심은 내장 자식(`FWxStateTreeComponentName`)으로 지목. 대상의 상태는 밖에서 직접 쓰지 않고 요청만 한다.
- **새 스폰 대상**: `IWxSpawnable`을 구현하면 `AWxSpawner`의 `SpawnableActorClass`에 지정 가능(`MustImplement` 메타로 강제).
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(Project Settings → Wx World Settings)에서 스포너 클래스별 에디터 아이콘 매핑.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Device/WxDevice.h` — 장치가 무엇을 갖고 무엇을 컴포넌트에 넘기는지, 상호작용·세이브 계약의 진입점.
2. `Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` — 이 모듈에서 가장 미묘한 부분. 권위/추종/복원 수렴 패턴 전체가 헤더 doc-comment에 정리돼 있다.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→`ServerInteract`→어빌리티로 이어지는 상호작용 흐름의 로컬 절반.
4. `Source/WxWorld/Public/Device/WxStateTreeTask_SendEvent.h` — Task 작성 관례와 장치 간 배선의 대표 예시.

## 관련
- 상위: GameMode가 고른 Experience 에셋이 `UWxInteractionScannerComponent`를 PlayerController에 주입한다(컨트롤러는 본 클래스를 모름). 상호작용 실행 어빌리티는 [[WxCombat]], 프롬프트 HUD는 [[WxUI]], 상태·처치 영속은 [[WxSave]], 인터페이스 정의는 [[WxCore]].

---
*문서 기준 커밋 `e1999dc` · 생성일 2026-08-24 · 소스 50파일 — `/readme-writer`로 갱신*
