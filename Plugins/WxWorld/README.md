# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 장치(문·상자·체크포인트·엘리베이터·버튼)를 StateTree로 구동하고, 플레이어 주변 상호작용 대상을 스캔·선택하며, 적/오브젝트 스폰과 처치/리스폰 상태를 관리한다. 상태는 서버 권위 → 복제 → 클라 추종으로 수렴한다.

## 책임
**담당**
- 월드 장치(`AWxDevice`)의 공통 호스트와 StateTree 기반 상태 구동·복제·복원 수렴
- 소유 클라의 주변 상호작용 스캔·선택·하이라이트와 `ServerInteract` 전달
- 장치 거동 StateTree Task 팔레트(이벤트 보내기·이동·애니·시퀀스·사운드·나이아가라·스포너 트리거 등)
- 스포너(`AWxSpawner`)의 스폰·처치/리스폰 상태 보유와 스폰 대상 계약(`IWxSpawnable`)

**경계 (비담당)**
- 상호작용 권위 실행(사거리·활성 검증 후 인터페이스 호출)은 `Event.Interact`를 받는 ServerOnly 어빌리티 — [[WxCombat]]의 GAS 자산. 스캐너는 이벤트 송출까지만.
- `IWxInteractable`·`IWxSavable` 인터페이스 정의와 `Event.Interact`·`Ability.Interact` 태그 — [[WxCore]].
- 상호작용 프롬프트 리스트의 HUD 표시(뷰모델·위젯) — [[WxUI]].
- 장치·스포너 상태의 슬롯 직렬화/복원 오케스트레이션 — [[WxSave]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 월드 장치 공통 호스트(추상). 상호작용 표면·세이브 신원·배선(`LinkedDevices`)만 갖고 상태 구동은 컴포넌트에 위임 | `Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 `StateTag`(복제·SaveGame)의 소유자. 권위/추종/복원 수렴의 핵심 | `Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | 소유 클라(PC 부착)에서 주변 `IWxInteractable`을 주기 스캔·선택·하이라이트하고 `ServerInteract`로 서버에 전달 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 인스턴스를 스폰하고 처치/리스폰 상태를 자체 보유(SaveGame) | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상 계약. Deferred Spawn의 `FinishSpawning` 이전 `OnSpawnedBy`로 초기값 주입 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_*` | 장치·퀘스트 거동 Task 팔레트(이벤트 보내기·상호작용 켜기/대기·컴포넌트/스플라인 이동·몽타주·시퀀스·사운드·나이아가라·스포너 트리거/처치 대기 등) | `Source/WxWorld/Public/{Device,Interaction,Spawnable}/` |
| `FWxStateTreeComponentName` | ST 에셋이 장치 액터의 컴포넌트를 이름으로 지목하는 저장 형태(에디터 드롭다운으로 선택) | `Source/WxWorld/Public/Device/WxStateTreeComponentName.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` 등 서버 권위 스포너 유틸(BP 라이브러리) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 몸통 컴포넌트를 세운 뒤, `UWxDeviceStateTreeComponent`의 State Tree 에셋으로 거동을 저작한다. 영속이 필요한 상태에는 반드시 상태 Tag를 달아야 저장된다(에셋 안에서 유일). 상태는 서버 권위 → 복제(`StateTag`) → 클라 추종으로 수렴하며, 세이브·레이트조인·스트리밍 인 모두 같은 수렴 경로를 탄다.
- **새 Task**: `FStateTreeTaskCommonBase`를 상속한 `FWxStateTreeTask_*` USTRUCT + `FWxStateTreeTask_*InstanceData` 쌍으로 추가. `GetInstanceDataType()`은 코딩 규칙 6의 예외로 헤더에 정의(이유 주석 동반). 컴포넌트 지목은 리터럴 대신 `FWxStateTreeComponentName`이나 오너 프로퍼티 바인딩을 쓴다(공유 에셋을 여러 배치가 재사용하므로).
- **장치 간 연동**: `SendEvent` Task가 대상 장치 트리에 이벤트를 발행한다. 대상은 배치 배선(`LinkedDevices` 바인딩) 또는 저작이 심은 내장 자식(`FWxStateTreeComponentName`)으로 지목. 대상의 상태는 밖에서 직접 쓰지 않고 요청만 한다(자기 트리만 자기 활성을 쓴다).
- **레벨 밖 호스트(퀘스트 ST)**: `WaitForInteraction`·`WaitSpawnersKilled`는 `FUniversalObjectLocator`로 배치 액터를 지목해 폴링 없이 통보로만 완료한다 — 퀘스트 스텝 게이트로 재사용된다.
- **새 스폰 대상**: `IWxSpawnable`을 구현하면 `AWxSpawner`의 `SpawnableActorClass`에 지정 가능(`MustImplement` 메타로 강제).
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(Project Settings → Wx World Settings)에서 스포너 클래스별 에디터 아이콘 매핑.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Device/WxDevice.h` — 장치가 무엇을 갖고 무엇을 컴포넌트에 넘기는지, 상호작용·세이브 계약의 진입점.
2. `Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` — 이 모듈에서 가장 미묘한 부분. 권위/추종/복원 수렴 패턴 전체가 헤더 doc-comment에 정리돼 있다.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→`ServerInteract`→어빌리티로 이어지는 상호작용 흐름의 로컬 절반.

## 관련
- 상위: 상호작용 스캐너·장치는 [[WxGame]]의 Experience/GameMode 주입 설정으로 배선(컨트롤러가 클래스를 모름). 상호작용 게이트·스포너 처치 대기 Task는 [[WxQuest]] ST가, 권위 어빌리티는 [[WxCombat]]가 사용.

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 50파일 — `/readme-writer`로 갱신*
