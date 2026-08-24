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
| `FWxStateTreeComponentName` | ST 에셋이 장치의 컴포넌트를 이름으로 지목하는 픽커 구조체 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeComponentName.h` |

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고 몸통(루트 컴포넌트)과 StateTree 에셋을 붙인다. AWxDevice는 루트를 만들지 않는다(파생 BP가 세운다). C++ 클래스를 새로 만들 일은 없다. 버튼·레버 같은 발동 장치도 같은 클래스로, 누른 상태를 자기 트리로 몰아 SendEvent 태스크로 상대를 민다. `Replicates`는 `AWxDevice`가 켜 두지만 BP가 되돌리면 상태 복제가 죽는다(꺼지면 BeginPlay가 Error 로그).
- **상태 구동 규약**: 상태 식별은 엔진 순정 상태 Tag. 영속이 필요한 상태에는 반드시 상태 디테일에 Tag를 달아야 세이브 슬롯에 담긴다(에셋 안에서 유일, 그 Tag가 곧 액터의 `StateTag`이자 저장 값). 권위만 상태를 정하고 클라·복원은 StateTag 수렴 추종 — 자세한 패턴은 `WxDeviceStateTreeComponent.h` doc-comment.
- **새 StateTree 태스크**: `FStateTreeTaskCommonBase` 파생 `USTRUCT`로 만든다. 인스턴스 데이터를 짝 구조체로 두고 `using FInstanceDataType`·`GetInstanceDataType()`을 헤더에 표기(코딩 규칙 6의 유일 예외, 각 헤더 주석 참조). 태스크 분류: `Device/`(연출·이동 — 재생/컴포넌트 이동/이펙트 적용/이벤트 보내기/스포너 발동·리스폰), `Interaction/`(상호작용 켜기·대기), `Spawnable/`(로케이터 지정 스포너 발동·처치 대기). 배치별로 달라지는 대상은 에셋 리터럴 대신 LinkedDevices 바인딩·`FWxStateTreeComponentName`·`FUniversalObjectLocator`로 지목한다(공유 ST 에셋 전제).
- **레벨 밖 호스트에서 배치 액터 지정**: 퀘스트 ST 등에서 특정 배치 스포너/대상을 겨눌 땐 `FUniversalObjectLocator`(순수 구조체)를 쓴다 — ST 컴파일러의 레벨 액터 참조 검증을 우회하고 WP/PIE 해석이 엔진에 내장돼 있다.
- **발동 장치 연결**: 레버·버튼 같은 발동 장치도 `AWxDevice`다 — 누른 상태를 자기 ST 에셋으로 몰고 그 상태의 '이벤트 보내기' 태스크가 지목한 장치를 민다. **대상은 유추하지 않는다 — 태스크의 `TargetKind`가 갈래를 선언하고, 고르지 않은 쪽 칸은 디테일 패널에서 감춰진다.** 지목 수단이 둘인 것은 대상이 정해지는 자리가 둘이기 때문이다. ① **배치가 정하는 대상**(`TargetKind = Linked`, 기본값): 태스크의 `LinkedDevices`를 Context Actor의 같은 이름 프로퍼티(레벨 인스턴스 저작)에 **바인딩**한다 — 태스크가 고르는 값이 아니라 배선의 거울이라 이름을 원본에 맞췄다. 공유 ST 에셋을 여러 배치가 쓰므로 리터럴을 못 박는다(`ST_Button`·`ST_Piston`). ② **저작이 정하는 대상**(`TargetKind = Child`): `ChildDevice`(컴포넌트 드롭다운)로 오너 BP의 내장 장치 하나를 이름 지목한다(`ST_Door`·`ST_Elevator`). 배선은 하나가 여럿을(1:N), 한 장치가 여러 발동 장치에(N:1) 걸린다. 장치 BP의 ChildActorComponent로 심긴 발동 장치만은 자기를 품은 장치를 `BeginPlay`가 `LinkedDevices`에 넣어 준다 — ChildActor 템플릿은 부모 액터 참조를 저작으로 담을 수 없어서이고, 대상 지목 중 런타임이 채우는 유일한 자리다. 눌리면 각 장치 트리에 버튼이 보내는 태그가 나가고, ST 에셋은 `On Event`로 받아 전이를 정한다. **보낼 태그는 `ST_Button`의 루트 파라미터 `TriggerEvent`가 정한다** — 에셋 기본값이 `Event.Device.Triggered`라 대부분의 버튼은 손댈 것이 없고, 달라야 하는 배치만 그 버튼 StateTree 컴포넌트의 파라미터 오버라이드로 바꾼다(엘리베이터 1F·2F 버튼, `BP_Elevator` 안에 저장). 태스크의 `Event` 칸을 액터 프로퍼티에 바로 바인딩하는 길은 없다 — StateTree 컴파일러가 Context Actor를 소스로 한 그 바인딩을 "Malformed target property path"로 거부한다(`LinkedDevices` 바인딩은 통과하므로 소스 프로퍼티에 따라 갈린다). 태그 규칙 — ① 기본 `Event.Device.Triggered`("눌렸다"): 받는 쪽이 보낸 이를 가를 필요가 없으면 이것만 쓴다(문·상자·체크포인트). ② 갈래가 필요하면 **요청하는 상태의 태그**를 보낸다(엘리베이터 버튼 = `Device.Elevator.1F`/`2F`, 받는 ST는 `On Event(Device.Elevator.1F) → 1F`). 이 용법에서 상태 태그의 뜻은 "그 상태로 가 달라" 하나뿐이며, 그 외 용도로 상태 태그를 이벤트에 쓰지 않는다. ③ 상태가 아닌 동작 요청이 생기면 그때 `Event.Device.<장치>.<동사>`를 만든다(미리 만들지 않는다). 전이는 끝 태그까지 정확히 듣는다 — 이벤트 매칭이 계층이라 부모 태그를 들으면 모든 장치 이벤트가 잡힌다. 상태별로 잠가야 하면 **잠글 상태를 이벤트로 요청**한다 — 같은 '이벤트 보내기' 태스크를 `Child` 갈래로 두고 `ChildDevice`와 `Event`를 채우면 오너 BP의 내장 버튼 하나를 지목한다(엘리베이터가 층 버튼·내부 버튼을 `Device.Button.Locked`/`.Idle` 로 옮긴다). 필요하면 `Payload`로 값을 실어 보낸다. **되돌림**: 동작을 마친 장치가 자기를 민 버튼을 푸는 것도 같은 배선이다 — 밀린 쪽의 `LinkedDevices`에 민 쪽을 넣어 저작을 양방향으로 놓는다(레벨에 따로 놓인 버튼이 미는 피스톤). 그래서 문·피스톤은 동작이 끝나는 상태에서 `Device.Button.Idle`을 보내 자기를 민 버튼을 푼다 — 버튼의 쿨다운이 시간이 아니라 그 완료로 풀리는 것이 이 경로다. 장치의 활성은 그 장치의 트리만 쓰므로, 방금 눌려 쿨다운 중인 버튼도 다투지 않고 잠긴다. 밖에서 활성을 직접 끄고 켜는 '상호작용 켜기'의 `Actor` 갈래는 자기 트리로 켜고 끄지 않는 대상에만 쓴다(나중에 쓴 쪽이 이긴다). 그 태스크도 같은 규칙이다 — `TargetKind`가 오너 장치 자신(`Self`, 기본값)인지 로케이터로 지목한 배치 액터(`Actor`)인지를 선언하고, 프롬프트·발행 자리는 앞 갈래에만 있다.
- **발동 연출·재조작 차단**: 눌린 상태(`ST_Button`의 Pressed)가 곧 연출 구간이자 쿨다운이다 — 그 상태에서 상호작용을 끄고 연출 태스크와 엔진 `Delay Task`를 돌린 뒤 완료 전이로 대기 상태에 돌아온다. 연출은 각 피어의 ST가 상태 Tag 복제를 추종해 재생하므로 별도 멀티캐스트가 없다.
- **게이트 태스크의 통보 규약**: 대기형 태스크(WaitForInteraction·WaitSpawnersKilled)는 폴링하지 않고 정적 `Notify...`로 완료를 통보받는다 — 각각 상호작용 어빌리티 경로와 `AWxSpawner::MarkKilled`가 호출한다. 서버 권위 ST 전용.
- **스포너 대상·부활 정책**: `SpawnableActorClass`는 `IWxSpawnable` 구현 액터여야 한다(MustImplement 메타). `EWxSpawnerMode::Manual`은 일괄 리스폰 제외, `bNeverRevive`는 영구 처치. 일괄 리스폰은 `UWxSpawnerLibrary::TryRespawnAll`(Auto만), 지정 트리거는 로케이터 태스크.
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
