# WxWorld — 월드 오브젝트 및 상호작용

> 문·상자·체크포인트·스포너 같은 배치형 월드 장치를 StateTree로 구동하고, 플레이어가 그것들과 상호작용하는 감지·선택·발동 경로를 담는다. 장치 상태는 서버 권위로 정해지고 클라이언트는 복제된 상태 태그로 수렴한다.

## 책임
**담당**
- StateTree로 자기 상태를 구동하는 월드 장치의 공통 호스트(`AWxDevice`)와 상태 실행·복제·수렴(`UWxDeviceStateTreeComponent`)
- 소유 클라의 주변 상호작용 액터 스캔·선택·하이라이트와 서버로의 발동 전달(`UWxInteractionScannerComponent`)
- 배치형 스포너의 스폰·처치·리스폰 상태 보유(`AWxSpawner` / `IWxSpawnable`)
- 장치·연출·스폰을 조립하는 재사용 StateTree Task 카탈로그(`StateTreeTask/`, `Interaction/`, `Spawnable/`)

**경계 (비담당)**
- `IWxInteractable` 인터페이스 정의와 `Event.Interact`·`Ability.Interact` 태그, 상호작용 어빌리티 계약은 [[WxCore]]
- HUD 상호작용 리스트·프롬프트 표시(뷰모델)는 [[WxUI]]
- 아이템 줍기·대화 등 다른 도메인의 `IWxInteractable` 구현체는 [[WxInventory]]·[[WxDialogue]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | StateTree 구동 월드 장치의 공통 호스트. `IWxInteractable` 표면과 상호작용 신호 전달만 담당 | `Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxDeviceStateTreeComponent` | 장치 상태머신 실행기이자 상태(StateTag)의 소유자. 서버 권위 → 클라 수렴 패턴의 핵심 | `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` |
| `UWxInteractionScannerComponent` | PlayerController에 붙어 소유 클라에서 주변 상호작용 액터를 주기 스캔·선택·발동 | `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | SpawnableActorClass를 스폰하고 처치 상태를 보유하는 레벨 배치 액터 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 대상이 구현하는 인터페이스. FinishSpawning 이전 `OnSpawnedBy` 훅 | `Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `TWxStateTreeWaitRegistry` | 통보까지 Running으로 머무는 대기 태스크들의 공용 등록부 템플릿 | `Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h` |
| `UWxSpawnerLibrary` | 서버 권위에서 스포너 일괄 리스폰(`TryRespawnAll`)을 여는 BFL | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 등 모듈 설정 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice` 파생 BP를 만들고 몸통(메시 등)을 세운 뒤, 장치의 StateTree 에셋과 `InitialState`를 컴포넌트에 지정한다. 루트는 파생 BP가 만든다. 상태 식별은 엔진 순정 상태 Tag이며 에셋 안에서 유일해야 한다.
- **장치 상호작용**: '상호작용 켜기'(`FWxStateTreeTask_EnableInteraction`) Task가 상태 진입 시 프롬프트와 발행 자리(`OnInteracted` dispatcher)를 켠다. 눌렸을 때의 전이는 그 노드가 있는 상태나 하위 상태에 두어야 바인딩이 보인다.
- **장치 간 배선**: 배치에서 `AWxDevice::LinkedDevices`를 채우고 '이벤트 보내기'(`FWxStateTreeTask_SendEvent`)로 상대 트리에 상태를 요청한다. 남의 장치는 직접 여닫지 않는다.
- **새 스폰 대상**: 액터가 `IWxSpawnable`을 구현하면 `AWxSpawner::SpawnableActorClass` 픽커에 잡힌다. `OnSpawnedBy`에서 세팅한 값은 빙의/BeginPlay에서 쓸 수 있다. `bNeverRevive`로 보스형 영구 처치를 표현한다.
- **스폰 트리거**: '스포너 발동'(`FWxStateTreeTask_TriggerSpawners`)은 `FUniversalObjectLocator`로 배치 액터를 직접 가리키므로 레벨 밖 호스트(퀘스트 ST)에서도 조립된다. 초기 진입에서는 발동하지 않는다.
- **재사용 연출 Task**: `StateTreeTask/` 아래에 컴포넌트/스플라인 이동, 애니메이션·사운드·레벨 시퀀스·나이아가라 재생, GE 적용, 입력 토글 등이 장치·퀘스트 공용으로 준비돼 있다. 통보를 기다리는 유형은 `TWxStateTreeWaitRegistry`를 공유한다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h` — 서버 권위 상태 → 클라 수렴(추종/재진입/초기 상태 보류) 패턴의 doc-comment가 모듈 전체 설계의 뿌리다.
2. `Source/WxWorld/Public/Device/WxDevice.h` — 장치 액터가 상호작용 표면과 상태 구동을 어떻게 나눠 갖는지, 발행 자리(Binding) 개념.
3. `Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 감지→선택→ServerInteract→어빌리티로 이어지는 상호작용 입력 경로 전체.
4. `Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp` — `Event.Interact`/`Ability.Interact` 태그로 WxCore 어빌리티에 넘기는 실제 배선.

## 관련
- 상위: `Plugins/GameFeatures/` 콘텐츠 플러그인과 [[WxQuest]] StateTree 호스트가 재사용 Task와 스포너 트리거를 조립해 쓴다. 장치는 Experience 주입 설정으로 스캐너를 PlayerController에 붙인다.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 51파일 — `/readme-writer`로 갱신*
