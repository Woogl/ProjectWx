# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용형 월드 오브젝트(문/엘리베이터/콘솔/상자/컷신 트리거)와 플레이어 상호작용 파이프라인, 적/오브젝트 스폰을 담당한다. 발동·영속 상태는 서버 권위 + 복제 + SaveGame 로 다루고, 비주얼/사이드이펙트는 공용 StateTree 노드로 구동한다.

## 책임
**담당**
- 상호작용 기믹의 공통 베이스(`AWxGimmick`)와 서버 권위 State 확정 단일 진입점(`CommitGimmickState`)
- 구체 기믹 구현: 문/엘리베이터/보물상자/경보·스폰 콘솔/컷신 트리거 (각자 복제 State enum 소유)
- 플레이어 상호작용 감지·등록·선택: `UWxInteractionComponent`(오버랩+Multicast) + `UWxInteractionRegistrySubsystem`(로컬 인-레인지 목록/선택/외곽선)
- 모든 기믹이 공유하는 StateTree 태스크 노드 모음(이동/애니/시퀀스/사운드/Niagara/스포너 트리거/State 확정/레이저)
- 레벨 배치 스포너(`AWxSpawner`)와 일괄 리스폰(`UWxSpawnerLibrary::TryRespawnAll`), 처치 상태 영속

**경계 (비담당)**
- 상호작용 어빌리티(입력→TargetData→서버 TryInteract)는 GAS 측 `WxAbility_Interact`가 구동 — 본 모듈은 `TryInteract` 진입점만 노출
- 상호작용 프롬프트 표시는 플레이어 HUD 리스트 위젯이 담당 ([[WxUI]])
- 세이브 슬롯 기록/복원의 직렬화는 [[WxCore]]의 `IWxSavable` 계약 + [[WxSave]] 런타임
- 보상 지급 로직은 외부 RewardLibrary (보물상자 ST 태스크가 호출)

## 의존성
- **주요 의존**: [[WxCore]] (`IWxSavable`, `IWxInteractionSource` 인터페이스), GameplayAbilities, Niagara, StateTree/GameplayStateTree, LevelSequence/MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxSavable.h`·`WxInteractionSource.h`는 WxCore 제공)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 상호작용 기믹의 abstract 베이스. State 쓰기 권위 게이트 + GimmickStateTree 구동 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes.h` | 전 기믹이 공유하는 ST 태스크 노드 집합(이동/애니/시퀀스/FX/State 확정/레이저) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 폰 오버랩 감지·레지스트리 등록·Multicast 알림. 소유 액터가 `OnInteracted`에 바인딩 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | 로컬 플레이어별 인-레인지 목록/선택/외곽선 조율. HUD 뷰모델이 표시 | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | SpawnableActorClass 인스턴스를 스폰하는 레벨 액터. 처치 상태 영속(GUID 키) | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 훅(`OnSpawnedBy`, 에디터 프리뷰 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | `TryRespawnAll` — 월드의 Auto 스포너 일괄 리스폰 BP 진입점 | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |
| `UWxWorldDeveloperSettings` | 스포너 클래스별 에디터 아이콘 매핑 등 프로젝트 설정 | `Source/WxWorld/Public/System/WxWorldDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick`를 상속하고 자체 복제+`SaveGame` State enum 소유, `SetGimmickState(uint8)`로 enum 매핑 구현. 비주얼/사이드이펙트는 자식 BP에서 ST 에셋을 `StateTree` 컴포넌트에 할당해 author한다. State 쓰기는 항상 `CommitGimmickState`(서버 권위)로만.
- **State 구동 패턴**: 권위 State enum 복제 → 자식의 GimmickStateTree가 Enum Compare 전이로 추종(서버/클라 동일, 이벤트 태그 없음). 초기 진입/복원과 라이브 전이는 노드가 `Transition.SourceStateID` 유효성으로 구분(복원 시 트리거성 효과는 침묵). 예외: `AWxCutsceneTrigger`는 복제 enum 없이 ST 이벤트 태그(`PlayEventTag`)로 구동.
- **새 ST 태스크 추가**: `WxGimmickStateTreeNodes.h`의 `FStateTreeTaskCommonBase` 파생 패턴(Instance Data + `EnterState`/`Tick`/`ExitState`)을 따른다. 컨텍스트 액터는 `Context.GetOwner()`를 `AWxGimmick`로 캐스트해 얻는다.
- **새 스폰 대상**: `IWxSpawnableInterface` 구현. `AWxSpawner.SpawnableActorClass`가 `MustImplement`로 강제한다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 권위 State 쓰기/StateTree 추종 패턴의 헤더 주석이 모듈 전체의 설계 원칙
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 모든 기믹 비주얼/사이드이펙트가 조립되는 공용 노드 카탈로그
3. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 오버랩→레지스트리→어빌리티→Multicast 상호작용 흐름의 진입점
4. `Source/WxWorld/Public/Gimmick/WxDoor.h` 또는 `WxElevator.h` — 베이스+ST 패턴이 구체 기믹에서 어떻게 author되는지의 레퍼런스

## 관련
- 상위: [[WxGame]](레벨 배치/게임플레이), [[WxUI]](상호작용 프롬프트 HUD), [[WxSave]](기믹/스포너 State 영속)
- 동급 의존: [[WxCore]](`IWxSavable`, `IWxInteractionSource`)

---
*문서 기준 커밋 `1735fc7` · 생성일 2026-06-25 · 소스 27파일 — `/readme-writer`로 갱신*
