# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·체크포인트·엘리베이터·버튼 같은 배치형 월드 장치를 StateTree로 구동하고, 플레이어의 근거리 상호작용 감지·선택과 레벨 스포너(적 배치·처치·리스폰)를 담당한다.

## 책임
**담당**
- 월드 장치(`AWxDevice`)의 상태 구동과 네트워크 수렴 — StateTree 실행, 상태(StateTag) 서버 권위 소유·복제, 상호작용 On/Off 바인딩
- 플레이어 상호작용 스캔 — 주변 후보 수집(액터 단위), 선택 사이클, 하이라이트(스텐실), `ServerInteract` RPC 발신
- 레벨 스포너(`AWxSpawner`) — 대상 스폰, 처치 상태 보유, 개별/일괄 리스폰
- 월드용 StateTree 태스크 모음 — 연출(애니메이션·사운드·나이아가라·레벨 시퀀스·컴포넌트/스플라인 이동), 상호작용 게이트, 스포너 발동·처치 대기·리스폰, 이벤트 송출

**경계 (비담당)**
- 상호작용 인터페이스 계약(`IWxInteractable`)과 `Event.Interact` 태그 정의 → [[WxCore]]
- 상호작용 어빌리티의 서버 권위 사거리·활성 검증 → GAS 어빌리티(에셋). 스캐너는 대상 인터페이스 호출을 어빌리티에 위임한다
- HUD 상호작용 리스트 표시 → [[WxUI]] (`WxViewModel_Interaction`)
- 퀘스트 스텝 게이트 호스트(레벨 밖 StateTree) → [[WxQuest]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 장치 공통 호스트. 상호작용 표면(IWxInteractable)과 배선(LinkedDevices)만 갖고 상태 구동은 컴포넌트에 위임 | `Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 복제 StateTag 소유자. 서버 권위→클라 추종 수렴 패턴의 중심 | `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController 부착, 소유 클라에서 근거리 상호작용 스캔·선택·하이라이트 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | SpawnableActorClass를 스폰하고 처치 상태를 보유하는 레벨 배치 액터 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 구현. FinishSpawning 이전 `OnSpawnedBy`로 스포너 주입을 받음 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_WaitForInteraction` | 대상이 상호작용될 때까지 대기하는 퀘스트 게이트. WaitRegistry로 무틱 대기 | `Source/WxWorld/Public/Interaction/WxStateTreeTask_WaitForInteraction.h` |
| `FWxStateTreeComponentName` | ST 에셋이 레벨 컴포넌트를 이름으로 지목하는 순수 구조체(연출 태스크 대상 지정) | `Source/WxWorld/Public/Device/WxDeviceComponentName.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` — Auto 모드 스포너 일괄 리스폰(BP 진입점) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 몸통 컴포넌트를 세운 뒤, `UWxDeviceStateTreeComponent`의 StateTree 에셋에 상태를 저작한다. 루트는 베이스가 만들지 않으므로 파생 BP가 세운다.
- **상태 식별**: 상태 디테일의 Tag 필드에 (에셋 내 유일한) GameplayTag를 달면 그 값이 복제·수렴 키가 된다. 태그 없는 상태는 추종 대상이 될 수 없어 시작 상태로도 못 쓴다.
- **새 StateTree 태스크**: `FStateTreeTaskCommonBase`를 상속하고 `FInstanceDataType`/`GetInstanceDataType()`을 둔다(헤더 정의는 코딩 규칙 6의 명시적 예외). 레벨 액터 지정은 `FUniversalObjectLocator`로 받아 ST 컴파일러의 레벨 참조 검증을 피한다.
- **통보형 대기**: `TWxStateTreeWaitRegistry<Payload>`로 진입 시 등록·통보 시 완료하는 무폴링 대기를 만든다(`WaitForInteraction`이 예시). 완료는 약한 실행 컨텍스트로 보내고 PIE 월드 경계로 좁힌다.
- **스포너 아이콘**: `UWxWorldDeveloperSettings`(Project Settings > Wx World Settings)에 스포너 클래스별 에디터 아이콘을 데이터로 매핑한다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 이 모듈의 핵심 난도. 서버 권위 상태를 StateTag 복제로 전 피어가 수렴시키는 패턴을 doc-comment가 전부 설명한다.
2. `Source/WxWorld/Public/Device/WxDevice.h` — 장치가 상호작용 신호를 받아 컴포넌트에 넘기고, 상태별 프롬프트·발행 자리(Binding)를 여닫는 흐름.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→`ServerInteract`→어빌리티 권위 검증으로 이어지는 상호작용 로컬리티/네트워크 경로.
4. `Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` — 통보형 대기 태스크의 공용 등록부(무틱 대기·월드 경계·오너 소멸 정리).

## 관련
- 상위: 장치·스포너 상호작용을 소비하는 [[WxQuest]](스텝 게이트 ST), 리스트 UI를 그리는 [[WxUI]]
- 계약: 상호작용 인터페이스·태그의 단일 소스 [[WxCore]]

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 51파일 — `/readme-writer`로 갱신*
