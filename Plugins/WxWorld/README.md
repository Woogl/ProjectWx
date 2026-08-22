# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 월드 오브젝트(문·체크포인트·엘리베이터 같은 장치, 적 스포너)와 플레이어의 상호작용을 담당하는 도메인 플러그인. 장치는 StateTree로 상태를 구동하고, 상호작용은 스캐너 → 어빌리티 권위 검증 경로로 처리한다.

## 책임
**담당**
- **장치(Device)**: `AWxDevice` 하나가 상태 소유·복제·영속과 StateTree 실행을 함께 든다(엔진 `UStateTreeComponent` 를 쓰지 않고 실행 컨텍스트를 직접 연다). 상태는 서버 권위이며 복제된 상태 Tag를 클라가 추종한다.
- **상호작용(Interaction)**: `UWxInteractionScannerComponent`가 소유 클라에서 주변 상호작용 액터를 주기 스캔·선택·하이라이트하고, 선택을 서버로 보내 어빌리티가 권위 검증 후 대상 인터페이스를 호출한다. 대상은 액터 단위이며 액터 안의 특정 메시만 영역이 되는 개념은 없다.
- **스폰(Spawnable)**: `AWxSpawner`가 배치 액터로서 대상을 스폰하고 처치/부활 상태를 자체 보유하며 WxSave 슬롯으로 영속한다. `IWxSpawnable`은 스폰 직후 훅.
- **StateTree 태스크 스위트**: 장치/퀘스트 ST 에셋이 조립해 쓰는 재생·이동·상호작용·스포너 태스크 노드 모음(아래 확장 포인트).

**경계 (비담당)**
- 상호작용의 권위 실행·사거리/차단 검증(`WxAbility_Interact`), GameplayEffect·GameplayEvent — [[WxCombat]] / GAS.
- 상호작용 HUD 리스트 표시·선택 뷰모델(`UWxViewModel_InteractionList`) — [[WxUI]].
- 저장 슬롯·복원 오케스트레이션(`IWxSavable` 계약의 소비자) — [[WxSave]].
- 상호작용 계약·저장 인터페이스·`WxGameplayTags` 정의 — [[WxCore]].

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존 — `IWxInteractable`·`IWxSavable`·`WxGameplayTags`). 엔진 서브시스템: `StateTree`(장치 상태머신·태스크), `GameplayStateTree`(순정 `StateTreeComponentSchema` — 장치 cpp 한정), `GameplayAbilities`(상호작용 어빌리티 연동), `Niagara`·`LevelSequence`/`MovieScene`(연출 태스크), `ModularGameplay`(스캐너의 `UControllerComponent`), `UniversalObjectLocator`(레벨 밖 호스트에서 배치 액터 지정), `DeveloperSettings`. 에디터에서만 `UnrealEd`(스포너 라벨·프리뷰).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDevice` | StateTree로 상태를 구동하는 월드 장치의 공통 호스트(Abstract). 상태 Tag·당사자·세이브 키를 소유하고 `IWxInteractable`+`IWxSavable` 구현 | `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` |
| `UWxInteractionScannerComponent` | 플레이어 컨트롤러에 붙는 주변 상호작용 스캐너·선택·서버 전송 | `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` |
| `AWxSpawner` | 스폰 대상 인스턴스를 스폰하고 처치/부활 상태를 영속하는 배치 액터 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnable` | 스폰 직후(빙의 전) per-instance 컨텍스트 주입 훅 | `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` 등 스포너 일괄 조작 BP 라이브러리 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 설정 | `Plugins/WxWorld/Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## Gameplay Tags
Native Tag 선언은 이 모듈에 없다. 상호작용 경로는 `WxCore`가 정의한 `WxGameplayTags::Event_Interact`·`Ability_Interact`를 소비만 한다(스캐너가 어빌리티를 애셋 태그로 조회·이벤트 송출). 장치의 상태 식별은 별도 태그 네임스페이스가 아니라 ST 에셋의 상태 디테일 Tag 필드(에셋 안에서 유일)를 저장 키로 쓴다.

## 확장 포인트 / 규약
- **새 장치**: `AWxDevice`를 상속한 BP를 만들고(몸통 메시는 BP가 세운다) 액터 디테일의 `State Tree`에 ST 에셋을 지정해 전이·연출을 정의한다. C++ 클래스를 새로 만들 일은 없다. 영속이 필요한 상태에는 상태 디테일에 Tag를 달아야 저장된다(그 Tag가 곧 액터의 `StateTag`이자 저장 값). `Replicates`는 `AWxDevice`가 켜 두지만 BP가 되돌리면 상태 복제가 죽는다(꺼지면 BeginPlay가 Error 로그).
- **새 ST 태스크**: `FStateTreeTaskCommonBase` 파생 `USTRUCT`로 만든다. 인스턴스 데이터를 짝 구조체로 두고 `using FInstanceDataType`·`GetInstanceDataType()`을 헤더에 표기(코딩 규칙 6의 유일 예외, 각 헤더 주석 참조). 태스크 분류: `Device/`(연출·이동 — 재생/컴포넌트 이동/이펙트 적용/이벤트 보내기/스포너 발동·리스폰), `Interaction/`(상호작용 켜기·대기), `Spawnable/`(로케이터 지정 스포너 발동·처치 대기).
- **레벨 밖 호스트에서 배치 액터 지정**: 퀘스트 ST 등에서 특정 배치 스포너/대상을 겨눌 땐 `FUniversalObjectLocator`(순수 구조체)를 쓴다 — ST 컴파일러의 레벨 액터 참조 검증을 우회하고 WP/PIE 해석이 엔진에 내장돼 있다.
- **발동 장치 연결**: 레버·버튼 같은 발동 장치도 `AWxDevice`다 — 누른 상태를 자기 ST 에셋으로 몰고 그 상태의 '이벤트 보내기' 태스크가 지목한 장치를 민다. **대상은 유추하지 않는다 — 채운 칸에만 나가고 비운 칸은 그냥 비어 있다.** 지목 수단이 둘인 것은 대상이 정해지는 자리가 둘이기 때문이다. ① **배치가 정하는 대상**: 태스크의 `TargetDevices`를 Context Actor의 `LinkedDevices`(레벨 인스턴스 저작)에 **바인딩**한다. 공유 ST 에셋을 여러 배치가 쓰므로 리터럴을 못 박는다(`ST_Button`·`ST_Piston`). ② **저작이 정하는 대상**: `ChildDevice`(컴포넌트 드롭다운)로 오너 BP의 내장 장치 하나를 이름 지목한다(`ST_Door`·`ST_Elevator`). 배선은 하나가 여럿을(1:N), 한 장치가 여러 발동 장치에(N:1) 걸린다. 장치 BP의 ChildActorComponent로 심긴 발동 장치만은 자기를 품은 장치를 `BeginPlay`가 `LinkedDevices`에 넣어 준다 — ChildActor 템플릿은 부모 액터 참조를 저작으로 담을 수 없어서이고, 대상 지목 중 런타임이 채우는 유일한 자리다. 눌리면 각 장치 트리에 버튼이 보내는 태그가 나가고, ST 에셋은 `On Event`로 받아 전이를 정한다. **보낼 태그는 `ST_Button`의 루트 파라미터 `TriggerEvent`가 정한다** — 에셋 기본값이 `Event.Device.Triggered`라 대부분의 버튼은 손댈 것이 없고, 달라야 하는 배치만 그 버튼 StateTree 컴포넌트의 파라미터 오버라이드로 바꾼다(엘리베이터 1F·2F 버튼, `BP_Elevator` 안에 저장). 태스크의 `Event` 칸을 액터 프로퍼티에 바로 바인딩하는 길은 없다 — StateTree 컴파일러가 Context Actor를 소스로 한 그 바인딩을 "Malformed target property path"로 거부한다(`LinkedDevices` 바인딩은 통과하므로 소스 프로퍼티에 따라 갈린다). 태그 규칙 — ① 기본 `Event.Device.Triggered`("눌렸다"): 받는 쪽이 보낸 이를 가를 필요가 없으면 이것만 쓴다(문·상자·체크포인트). ② 갈래가 필요하면 **요청하는 상태의 태그**를 보낸다(엘리베이터 버튼 = `Device.Elevator.1F`/`2F`, 받는 ST는 `On Event(Device.Elevator.1F) → 1F`). 이 용법에서 상태 태그의 뜻은 "그 상태로 가 달라" 하나뿐이며, 그 외 용도로 상태 태그를 이벤트에 쓰지 않는다. ③ 상태가 아닌 동작 요청이 생기면 그때 `Event.Device.<장치>.<동사>`를 만든다(미리 만들지 않는다). 전이는 끝 태그까지 정확히 듣는다 — 이벤트 매칭이 계층이라 부모 태그를 들으면 모든 장치 이벤트가 잡힌다. 상태별로 잠가야 하면 **잠글 상태를 이벤트로 요청**한다 — 같은 '이벤트 보내기' 태스크에 `ChildDevice`와 `Event`를 채우면 오너 BP의 내장 버튼 하나를 지목한다(엘리베이터가 층 버튼·내부 버튼을 `Device.Button.Locked`/`.Idle` 로 옮긴다). 필요하면 `Payload`로 값을 실어 보낸다. **되돌림**: 동작을 마친 장치가 자기를 민 버튼을 푸는 것도 같은 배선이다 — 밀린 쪽의 `LinkedDevices`에 민 쪽을 넣어 저작을 양방향으로 놓는다(레벨에 따로 놓인 버튼이 미는 피스톤). 그래서 문·피스톤은 동작이 끝나는 상태에서 `Device.Button.Idle`을 보내 자기를 민 버튼을 푼다 — 버튼의 쿨다운이 시간이 아니라 그 완료로 풀리는 것이 이 경로다. 장치의 활성은 그 장치의 트리만 쓰므로, 방금 눌려 쿨다운 중인 버튼도 다투지 않고 잠긴다. 밖에서 활성을 직접 끄고 켜는 '상호작용 켜기' Target(UOL) 갈래는 자기 트리로 켜고 끄지 않는 대상에만 쓴다(나중에 쓴 쪽이 이긴다).
- **발동 연출·재조작 차단**: 눌린 상태(`ST_Button`의 Pressed)가 곧 연출 구간이자 쿨다운이다 — 그 상태에서 상호작용을 끄고 연출 태스크와 엔진 `Delay Task`를 돌린 뒤 완료 전이로 대기 상태에 돌아온다. 연출은 각 피어의 ST가 상태 Tag 복제를 추종해 재생하므로 별도 멀티캐스트가 없다.
- **스포너 부활 정책**: `EWxSpawnerMode`(Auto/Manual)와 `bNeverRevive`(보스). 일괄 리스폰은 `UWxSpawnerLibrary::TryRespawnAll`(Auto만), 지정 트리거는 로케이터 태스크.

## 여기서부터 읽어라
1. `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h` — 모듈의 심장. 클래스 doc-comment에 서버 권위 상태 구동·클라 추종·재진입·세이브 복원 진입의 전체 패턴이 정리돼 있다. cpp의 `StartTree`/`PublishAuthorityState`/`FollowAuthorityState`가 그 구현.
2. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h` — 스캔 → 선택 → `ServerInteract` → 폰 ASC 이벤트 → 어빌리티 권위 검증으로 이어지는 상호작용 흐름 전체가 doc-comment에 있다.
3. `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` — 처치/부활·WxSave `SaveId`(에디터 저장 시 ActorGuid 확정) 계약.
4. `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxStateTreeTask_EnableInteraction.h` + `WaitForInteraction.h` — 장치가 상호작용 영역을 여닫고 그 발행을 전이로 잇는 규약, 퀘스트 게이트로서의 상호작용 대기.

## 관련
- 상위: [[WxGame]] (Experience가 스캐너 컴포넌트 주입, GameFeature가 콘텐츠 배치)
- 인접: [[WxCore]] · [[WxCombat]] · [[WxSave]] · [[WxUI]] · [[WxQuest]]

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 46파일 — `/readme-writer`로 갱신*
