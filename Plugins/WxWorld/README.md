# WxWorld — 월드 오브젝트 / 상호작용

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문·엘리베이터·콘솔·상자 등 기믹), 폰-범위 기반 상호작용 등록/선택, 적/오브젝트 스포너를 담는 도메인 플러그인이다.

## 책임
**담당**
- 기믹(`AWxGimmick` 파생) 베이스와 StateTree 구동 패턴: C++ 가 권위 State enum만 소유하고, 공용 ST 노드가 복제 State를 추종해 비주얼·FX·인터랙션 토글을 적용
- 상호작용 컴포넌트/레지스트리: 폰 오버랩 감지 → 로컬 레지스트리 등록 → 선택/외곽선 강조 → 서버 권위 `TryInteract` → Multicast 알림
- 스포너: 레벨 배치 `AWxSpawner` 의 스폰/처치 상태(`bIsKilled`) 보유, `UWxSpawnerLibrary` 일괄 리스폰

**경계 (비담당)**
- 상호작용 입력→서버 전달 어빌리티(`WxAbility_Interact`), 상호작용 텍스트를 그리는 HUD 리스트(WBP/뷰모델) — [[WxCombat]] / [[WxUI]]
- `IWxSavable`·`IWxInteractionSource` 인터페이스 정의, 슬롯 직렬화 — [[WxCore]] / [[WxSave]]

## 의존성
- **주요 의존**: [[WxCore]] (`IWxSavable`, `IWxInteractionSource`), StateTree/GameplayStateTree, GameplayAbilities, Niagara, LevelSequence/MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 추상 베이스. `SceneRoot`+`StateTree` 제공, `IWxSavable` 구현, `SetGimmickState` 훅. 자식이 State enum 소유 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxDoor` | 기믹 자식 예시. `EWxDoorState`(Replicated+SaveGame) 권위 State, Enum Compare 전이로 슬라이드 | `Source/WxWorld/Public/Gimmick/WxDoor.h` |
| `WxGimmickStateTreeNodes` | 전 기믹 공용 ST 태스크 모음(Move/SplineMove/PlayAnimation/PlaySound/SpawnNiagara/SetState/TriggerSpawners/Laser 등) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | `USphereComponent`+`IWxInteractionSource`. 오버랩 감지·레지스트리 등록·`TryInteract`·Multicast | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer 서브시스템. 범위 내 컴포넌트 목록·선택 소유, HUD 리스트에 공급 | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. `bIsKilled`(SaveGame) 처치 상태, `bNeverRevive`, Auto/Manual 모드 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 인터페이스. `OnSpawnedBy` per-instance 훅(스포너 백레퍼런스 주입 등) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerLibrary` | BP 진입점. `TryRespawnAll` 이 월드의 Auto 스포너를 TActorIterator 로 일괄 리스폰(collect-first) | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속 → 자식이 메시/InteractionComponent와 State enum(`UPROPERTY` Replicated+SaveGame)을 직접 보유 → `SetGimmickState` 오버라이드로 원시 uint8을 자기 enum으로 캐스트. 비주얼·FX·인터랙션 토글은 모두 자식 BP에 할당한 ST 에셋의 공용 노드가 처리한다(C++ 비주얼 코드 없음).
- **리플리케이션 모델**: 권위 측이 State enum을 확정·복제하고, ST가 Enum Compare 전이로 복제 State를 폴링해 서버/클라 동일하게 추종한다(이벤트 태그·RepNotify 불필요). 1회성 사이드이펙트(사운드/FX/스폰/SetState)는 "라이브 전이 vs 초기 진입(복원/레이트조인)"을 `Transition.SourceStateID` 유효성으로 구분해 복원 시 침묵한다.
- **새 ST 태스크**: `WxGimmickStateTreeNodes.h` 에 `FStateTreeTaskCommonBase` 파생 + InstanceData 추가. 순수 비주얼 태스크는 State를 읽지 않고 `Context.GetOwner()` 캐스트로 컴포넌트만 만진다.
- **상호작용 추가**: 액터에 `UWxInteractionComponent` 추가(영역 수만큼) → `OnInteracted` 델리게이트에 핸들러 바인딩, 핸들러 내부에서 `HasAuthority()` 분기.
- **WxSave 통합**: 기믹/스포너는 `IWxSavable` 의 `WxSaveId`(에디터 부여, 세션 간 불변)로 슬롯 키를 갖고, `UPROPERTY(SaveGame)` State 필드가 직렬화된다.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 기믹 베이스와 "권위 State + ST 추종" 패턴의 전체 설명
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 모든 기믹이 공유하는 ST 태스크 카탈로그와 라이브/복원 구분 규약
3. `Source/WxWorld/Public/Gimmick/WxDoor.h` — 베이스 패턴을 따르는 가장 단순한 구체 기믹
4. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 오버랩→레지스트리→어빌리티→Multicast 상호작용 흐름
5. `Source/WxWorld/Public/Spawnable/WxSpawner.h` — 스폰/처치/리스폰 모델과 WxSave 보존

## 관련
- 상위: [[WxGame]]
- 인접: [[WxCore]] (인터페이스), [[WxSave]] (슬롯), [[WxCombat]] (상호작용 어빌리티), [[WxUI]] (HUD 리스트)

---
*문서 기준 커밋 `c451acb` · 생성일 2026-06-23 · 소스 30파일 — `/readme-writer`로 갱신*
