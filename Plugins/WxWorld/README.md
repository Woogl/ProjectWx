# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·체크포인트 같은 배치형 장치와 그 상호작용, 적 스포너, 그리고 이들을 데이터로 엮는 StateTree 태스크 묶음을 담는 도메인 플러그인이다. 장치의 상태는 서버 권위이고, 클라는 복제된 StateTag 로 수렴 추종한다.

## 책임
**담당**
- StateTree 로 자기 상태를 구동하는 월드 장치(`AWxDevice`)와 그 상태 실행·복제(`UWxDeviceStateTreeComponent`)
- 소유 클라 주변 상호작용 후보 스캔·선택·하이라이트(`UWxInteractionScannerComponent`)
- 배치형 스포너와 스폰 대상 계약(`AWxSpawner` / `IWxSpawnable`), 처치·리스폰 상태 관리
- 장치·상호작용·스포너·연출을 데이터로 엮는 StateTree 태스크군(이벤트 보내기·상호작용 켜기/대기·스포너 발동/처치대기·애니/사운드/시퀀스/나이아가라/이동 등)

**경계 (비담당)**
- 상호작용 어빌리티(권위 사거리·활성 검증, `Event.Interact` 처리)와 GAS — 콘텐츠/[[WxCombat]] 쪽. 스캐너는 폰 ASC 로 이벤트만 송출한다
- HUD 상호작용 리스트 뷰모델 표시 — [[WxUI]] (`WxViewModel_Interaction`)
- `IWxInteractable` 인터페이스와 공용 GameplayTag 정의 — [[WxCore]]
- 장치 컴포넌트 이름 드롭다운 등 에디터 커스터마이징 — [[WxToolset]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | StateTree 로 구동되는 월드 장치의 공통 호스트이자 상호작용 표면(`IWxInteractable`). 상호작용 신호를 받아 컴포넌트에 전달 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 상태(StateTag) 소유자. 서버 권위 상태를 클라가 수렴 추종 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController 에 붙어 소유 클라에서 주변 상호작용 액터를 주기 스캔·선택·하이라이트, `ServerInteract` 송출 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | `SpawnableActorClass` 를 스폰하고 처치·리스폰 상태를 자체 보유하는 레벨 배치 액터 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 구현하는 계약. `OnSpawnedBy` 로 FinishSpawning 이전 초기화 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_WaitForInteraction` | 지정 대상이 상호작용될 때까지 대기하는 퀘스트 게이트 태스크(폴링 없음) | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h` |
| `FWxStateTreeTask_SendEvent` | 라이브 전이 진입 시 다른 장치 트리에 이벤트를 보내 상태를 요청 | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SendEvent.h` |
| `TWxStateTreeWaitRegistry` | 통보가 올 때까지 Running 으로 머무는 태스크들의 공용 등록부(약한 실행 컨텍스트 기반) | `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 를 파생한 BP 로 몸통(루트)을 세운다. 상태 로직은 `UWxDeviceStateTreeComponent` 가 실행하는 ST 에셋에 저작하고, 상태 식별은 상태 디테일의 Tag 필드(에셋 내 유일)로 한다. 버튼·레버 같은 발동 장치도 같은 클래스이며 자기 트리에서 `SendEvent` 로 상대를 민다.
- **장치 상태 복제 규약**: 상태는 서버 권위. 권위 트리만 상태를 정하고 클라 트리는 복제된 `StateTag` 를 추종한다. 재진입·레이트조인·스트리밍 인도 모두 같은 수렴 경로로 처리된다 — 새 태스크는 이 전제(권위 구동 ST) 위에서 만든다.
- **새 상호작용 대상**: `IWxInteractable`([[WxCore]])을 구현하면 스캐너가 액터 단위로 후보에 넣는다. 대상 자격은 콜리전 프리셋이 아니라 인터페이스의 `CanInteract` 가 정한다(쿼리 콜리전만 켜져 있으면 된다).
- **새 대기형 태스크**: `TWxStateTreeWaitRegistry<Payload>` 를 정적 등록부로 두고 진입 시 `Add`, 통보 시 `FinishMatching`, `ExitState` 에서 핸들로 `Remove` — `WaitForInteraction`·`WaitSpawnersKilled` 가 이 골격을 따른다.
- **레벨 액터 지정**: 태스크가 배치 액터를 가리킬 땐 `FUniversalObjectLocator`(순수 구조체)로 지정해 ST 컴파일러의 레벨 참조 검증을 우회하고, 레벨 밖 호스트(퀘스트 ST)에서도 조립되게 한다.
- **스포너**: 스폰 대상 클래스는 `IWxSpawnable`(`MustImplement`) 이어야 한다. `Auto`/`Manual` 모드와 `bNeverRevive`(영구 처치) 로 리스폰 정책을 정하며, 일괄 리스폰은 `UWxSpawnerLibrary::TryRespawnAll`(서버 권위)로 트리거한다.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 이 모듈의 핵심인 서버 권위 상태 구동·수렴 추종 패턴이 doc-comment 에 전부 있다
2. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치 호스트가 상호작용 표면과 상태 구동을 어떻게 나눠 갖는지
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→`ServerInteract`→어빌리티로 이어지는 상호작용 입력 경로 전체
4. `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` — 폴링 없는 대기 태스크들의 공통 골격

## 관련
- 상위: [[WxCore]] (`IWxInteractable`·GameplayTag), 콘텐츠에서 상호작용 어빌리티([[WxCombat]] GAS)와 HUD 뷰모델([[WxUI]])이 이 모듈의 표면을 소비한다

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 51파일 — `/readme-writer`로 갱신*
