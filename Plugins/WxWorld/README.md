# WxWorld — 월드 오브젝트 & 상호작용

> 레벨에 배치되는 상호작용 가능한 월드 오브젝트(문/엘리베이터/보물상자/콘솔/컷신)와, 플레이어 상호작용 감지·선택·발동 파이프라인, 그리고 적/액터 스폰 시스템을 담당한다.

## 책임
**담당**
- 상호작용 감지·등록·발동: 오버랩 기반 인-레인지 수집, HUD 리스트용 레지스트리, 서버 권위 발동(Multicast)
- 기믹 액터: `AWxGimmick` 계열의 문/엘리베이터/보물상자/경보·스폰 콘솔/컷신 트리거 (StateTree 구동 포함)
- 스폰 시스템: 레벨 배치 `AWxSpawner`, 월드 레지스트리 서브시스템, 일괄/개별 리스폰과 처치(영구사망) 상태
- 월드 오브젝트의 WxSave/Level Streaming 영속(GUID 키 기반 상태 보존)

**경계 (비담당)**
- 상호작용 입력 어빌리티(`WxAbility_Interact`)·캐릭터 측 ASC — [[WxCombat]] 영역
- 상호작용 프롬프트 HUD/뷰모델 표시(`WBP_InteractionList`, `UWxViewModel_InteractionList`) — [[WxUI]] 영역
- 보물상자 보상 지급(`WxRewardComponent`) — [[WxInventory]] (플러그인 참조 금지로 상속 BP에서 부착)
- 스폰된 액터의 AI 빙의/행동 — [[WxAI]]
- `IWxSavable`/`IWxInteractionSource` 인터페이스 정의 자체 — [[WxCore]] (여기선 구현만)

## 의존성
- **주요 의존**: [[WxCore]] · GameplayTags · StateTreeModule / GameplayStateTreeModule(기믹 상태머신) · GameplayAbilities · LevelSequence/MovieScene(컷신) · Niagara · DeveloperSettings
- 규칙: 플러그인 의존은 WxCore뿐 — WxCore 외 Wx 참조 없음 ✅ (`IWxInteractionSource`/`IWxSavable`는 WxCore 정의 인터페이스이며, 보상·HUD 연동은 BP/델리게이트로 우회)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxInteractionComponent` | 상호작용 영역 단위. 오버랩 감지→레지스트리 등록, 서버 `TryInteract`→Multicast 발동 | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer별 인-레인지 목록·선택 소유. HUD 뷰모델이 구독 | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxGimmick` | 상호작용 월드 오브젝트 공통 베이스. `IWxSavable`, `ApplyState` 후크, GUID 영속 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `AWxDoor` / `AWxElevator` | StateTree로 상태·전이를 구동하는 기믹 (자체 State enum 권위) | `Source/WxWorld/Public/Gimmick/WxDoor.h`, `WxElevator.h` |
| `AWxSpawner` | 레벨 배치 스폰 액터. `bIsKilled`/`bNeverRevive` 처치 상태 보유 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상 액터 계약(`OnSpawnedBy` 훅, 에디터 미리보기 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerSubsystem` | 월드 내 Spawner 레지스트리. 일괄 리스폰·역조회 처치 마킹 | `Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `UWxSpawnerLibrary` | BP 진입점(thin wrapper) — `RespawnAutoSpawners` | `Source/WxWorld/Public/System/WxSpawnerLibrary.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속, 컴포넌트는 자식이 직접 보유해 `SceneRoot`에 부착. `ApplyState()` 오버라이드로 로컬 효과 적용. 1회성은 `MarkTriggered()`(점진 폐기 예정), 다단계는 자체 State enum 권위(Door/Elevator 패턴) 채택.
- **StateTree 기믹**: 상태/전이는 `ST_*` 에셋에서 author, C++는 노드용 프리미티브만 노출(`WxDoorStateTreeNodes`/`WxElevatorStateTreeNodes`).
- **새 스폰 대상**: `IWxSpawnableInterface` 구현 후 Spawner의 `SpawnableActorClass`에 지정(`MustImplement`로 강제). `OnSpawnedBy`에서 per-instance 컨텍스트 주입(FinishSpawning 이전).
- **영속**: 보존 필드는 `UPROPERTY(SaveGame)`. 안정 키 `WxSaveId`(GUID)는 에디터에서 1회 부여되어 세션/스트리밍 간 불변.
- **리플리케이션 권한**: 발동·상태 전이는 서버 권위. 클라는 복제된 State/`bTriggered` OnRep으로 따라간다. 상호작용 발동은 `TryInteract`(서버)→`MulticastInteracted`.

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 상호작용 파이프라인 전체 흐름(감지→레지스트리→어빌리티→Multicast)이 헤더 주석에 정리됨
2. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 모든 기믹의 공통 영속/`ApplyState` 규약. 기믹을 읽기 전 먼저 본다
3. `Source/WxWorld/Public/Spawnable/WxSpawner.h` + `Source/WxWorld/Public/System/WxSpawnerSubsystem.h` — 스폰/처치/리스폰 상태 모델

## 관련
- 상위: [[WxCombat]](상호작용 어빌리티·캐릭터 사망 처치 연동), [[WxUI]](프롬프트 HUD), [[WxInventory]](보물상자 보상), [[WxAI]](스폰 대상), [[WxSave]](슬롯 영속), [[WxCore]](`IWxInteractionSource`/`IWxSavable` 정의)

---
*문서 기준 커밋 `6e6d0ae` · 생성일 2026-06-18 · 소스 33파일 — `/readme-writer`로 갱신*
