# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 놓인 상호작용 장치(문·상자·체크포인트·버튼·엘리베이터)와 스포너를 StateTree 로 구동하고, 플레이어의 근접 상호작용 스캔·선택·발동을 잇는 도메인 플러그인.

## 책임
**담당**
- 월드 장치(`AWxDevice`)의 공통 호스트와, 그 상태를 서버 권위로 구동·복제·저장하는 StateTree 실행기(`UWxDeviceStateTreeComponent`)
- 장치 저작용 StateTree 태스크 팩(연출 재생·이동·이벤트 전달·상호작용 토글/대기·스포너 발동/대기 등)
- 근접 상호작용 스캐너: 소유 클라에서 후보 수집·선택·하이라이트, 서버로 발동 전달(`UWxInteractionScannerComponent`)
- 스폰 대상을 레벨에 배치·처치·리스폰하고 그 처치 상태를 자체 보존(`AWxSpawner`/`IWxSpawnable`/`UWxSpawnerLibrary`)

**경계 (비담당)**
- `IWxInteractable`·`IWxSavable` 인터페이스, Native Gameplay Tag, 로케이터 유틸 원본 정의 — [[WxCore]]
- 상호작용 어빌리티(`WxAbility_Interact`)의 권위 사거리·활성 검증과 GE 적용 — [[WxCombat]] (ASC/GAS)
- 세이브 슬롯 직렬화·복원 오케스트레이션 — [[WxSave]] (본 모듈은 `SaveGame` 프로퍼티만 노출)
- HUD 상호작용 리스트 위젯·뷰모델과 Enhanced Input 수신 — [[WxUI]]
- 컴포넌트 이름 드롭다운·로케이터 디테일 커스터마이징 — WxEditor(에디터 도구)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | 상호작용·세이브 표면을 가진 월드 장치의 공통 호스트(Abstract) | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태(StateTag)의 소유·복제·저장·수렴 추종 실행기 | `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PC 부착 근접 상호작용 스캔·선택·하이라이트·발동 전달 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 스폰 대상 배치·처치·리스폰 및 처치 상태 보존 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후(FinishSpawning 전) 초기화 훅 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `FWxStateTreeTask_SendEvent` | 라이브 전이 시 다른 장치 트리로 이벤트 전달 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeTask_SendEvent.h` |
| `FWxStateTreeTask_EnableInteraction` | 오너 장치 자신의 상호작용 토글 + 프롬프트·발행 자리 등록 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h` |
| `FWxStateTreeComponentName` | ST 에셋이 장치 컴포넌트를 이름으로 지목하는 구조체 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeComponentName.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 파생 BP 를 만들고 몸통 컴포넌트를 세운다(베이스는 루트를 만들지 않음). 상태는 ST 에셋으로 저작하며, 영속이 필요한 상태에는 상태 디테일의 Tag 를 달아야 저장된다. 저장 없는 첫 시작 상태는 컴포넌트의 `InitialState` 로 정한다.
- **새 장치 태스크**: `FStateTreeTaskCommonBase` 파생 USTRUCT + `FWxStateTreeTask_*InstanceData` 쌍으로 만든다(`Device/` 참고). `GetInstanceDataType()` 인라인 정의는 코딩 규칙 6 의 명시적 예외. 통보로 완료되는 대기형 태스크는 `TWxStateTreeWaitRegistry<Payload>`(`Private/WxStateTreeWaitRegistry.h`)로 등록/해제한다.
- **상태 소유 패턴**: 상태는 서버 권위 단일 소스. 권위가 틱 말미에 활성 Tag 를 `StateTag` 에 기록하고, 클라·복원은 같은 자리에서 라이브 전이로 그 값에 수렴 추종한다(복제 카운터 대신 값 수렴). 밖에서 남의 장치 상태를 직접 쓰지 않고 '이벤트 보내기' 로 요청만 한다.
- **장치 배선**: 배치가 정하는 상대는 오너 `LinkedDevices`(바인딩), 저작이 정하는 내장 장치는 `FWxStateTreeComponentName`(이름), 외부 배치 대상은 `FUniversalObjectLocator` 로 지목한다.
- **스포너 아이콘**: `UWxWorldDeveloperSettings.SpawnerClassIcons`(Project Settings > Wx World Settings)로 클래스별 에디터 아이콘을 데이터 주도로 지정.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 장치가 상호작용·세이브·배선을 어떻게 받아 컴포넌트에 위임하는지, 계약 상대가 액터 하나인 이유
2. `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h` — 이 모듈의 핵심인 상태 권위·복제·복원 수렴 패턴(태스크·복원·초기 상태가 모두 여기로 귀결)
3. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔→선택→ServerInteract→어빌리티로 이어지는 상호작용 로컬리티/권위 경계
4. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 처치 상태 보존·리스폰·에디터 프리뷰의 스포너 수명

## 관련
- 상위: [[WxGame]] (Experience 주입으로 스캐너 부착·장치 배치)
- 의존: [[WxCore]] (인터페이스·태그·로케이터 유틸)

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 51파일 — `/readme-writer`로 갱신*
