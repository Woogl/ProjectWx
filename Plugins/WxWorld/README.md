# WxWorld — 월드 오브젝트 및 상호작용

> 레벨에 배치되는 상호작용형 월드 오브젝트(문/엘리베이터/콘솔/상자/컷신 트리거 등 "기믹")와 플레이어 상호작용 파이프라인, 적 스폰/리스폰을 담당한다. 기믹 비주얼·연출은 공통 StateTree 노드 모음으로 데이터 주도화되어 있다.

## 책임
**담당**
- 상호작용 파이프라인: 오버랩 감지 → 로컬 레지스트리 등록/선택/외곽선 강조 → 서버 권위 `TryInteract` → Multicast `OnInteracted` 발화 (`Interaction/`)
- 기믹 액터 계층: `AWxGimmick` 베이스 + 권위 State enum(복제 + SaveGame) + 자식별 구현(Door/Elevator/콘솔류/상자/컷신)
- 기믹 공통 StateTree 태스크 라이브러리: 이동/애니/시퀀스/사운드/Niagara/스폰 트리거/State 확정 등 재사용 노드 (`Gimmick/WxGimmickStateTreeNodes.h`)
- 스폰 시스템: 레벨 배치 `AWxSpawner`, 처치/리스폰 위임 서브시스템, BP 진입점 라이브러리 (`Spawnable/`, `System/`)

**경계 (비담당)**
- 상호작용 입력·어빌리티(`WxAbility_Interact`)와 HUD 프롬프트 리스트(WBP/뷰모델)는 외부([[WxCombat]]/[[WxUI]]). 본 모듈은 레지스트리 목록만 제공
- 세이브 슬롯 직렬화 자체는 [[WxSave]]. 본 모듈은 `IWxSavable` 구현 + `UPROPERTY(SaveGame)` 노출만
- 보상/픽업 드랍은 [[WxInventory]]의 `WxRewardComponent` — 플러그인 간 참조 금지로 상자 C++ 가 아니라 상속 BP 에서 부착
- 인터페이스/세이브 계약 정의는 [[WxCore]] (`IWxInteractionSource`, `IWxSavable`)

## 의존성
- **주요 의존**: `WxCore` (인터페이스 `IWxInteractionSource`/`IWxSavable`), StateTree(`StateTreeModule`/`GameplayStateTreeModule`), GameplayTags, GameplayAbilities, Niagara, LevelSequence/MovieScene, DeveloperSettings
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (보상/UI/세이브 결합은 모두 BP·인터페이스로 우회)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGimmick` | 모든 기믹의 추상 베이스. 공통 `StateTree` 컴포넌트 + `IWxSavable`, 자식이 State enum 소유 | `Source/WxWorld/Public/Gimmick/WxGimmick.h` |
| `WxGimmickStateTreeNodes.h` | 기믹 비주얼·연출을 구성하는 공통 ST 태스크 모음(이동/애니/시퀀스/FX/스폰/SetState/레이저) | `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` |
| `UWxInteractionComponent` | 오버랩 감지 + 레지스트리 등록 + 서버 `TryInteract` + Multicast `OnInteracted` | `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` |
| `UWxInteractionRegistrySubsystem` | LocalPlayer 단위 인-레인지 목록/선택/강조 조율(HUD 리스트 소스) | `Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h` |
| `AWxSpawner` | 스폰 대상 인스턴스를 들고 처치/리스폰 상태(`bIsKilled`, SaveGame) 소유 | `Source/WxWorld/Public/Spawnable/WxSpawner.h` |
| `IWxSpawnableInterface` | 스폰 대상이 구현하는 계약(`OnSpawnedBy` 후크 + 에디터 프리뷰 메시) | `Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` |
| `UWxSpawnerSubsystem` | 스포너 레지스트리. 처치 마킹/일괄 리스폰 위임(전수 순회 회피) | `Source/WxWorld/Public/System/WxSpawnerSubsystem.h` |
| `AWxDoor` / `AWxElevator` | State+StateTree 패턴의 대표 구현(직선 슬라이드 / 스플라인 끝점 왕복) | `Source/WxWorld/Public/Gimmick/WxDoor.h`, `WxElevator.h` |

## 확장 포인트 / 규약
- **새 기믹 추가**: `AWxGimmick` 상속 → 권위 `State` enum(`Replicated, SaveGame`) 선언 → 인터랙션 핸들러(`Handle...Interacted`)에서 권위 측 최종 State 확정 → `SetGimmickState(uint8)` 오버라이드로 ST의 `Wx Set State` 태스크가 자기 enum으로 캐스트해 쓰게 함. 자식 BP가 실행할 ST 에셋을 `StateTree` 컴포넌트에 할당
- **상태 구동 모델**: C++는 권위 State만 소유, ST 에셋이 복제 State를 Enum Compare 전이로 폴링·추종하며 비주얼/인터랙션 토글/사이드이펙트를 서버·클라 동일하게 적용. 초기 진입(시작/복원/레이트조인)은 `Transition.SourceStateID` 유효성으로 라이브 전이와 구분 — 발동성 FX/사운드/스폰은 복원 시 침묵. 예외: `AWxCutsceneTrigger`는 State enum 없이 ST 이벤트 + OnComplete 전이로 구동
- **새 ST 태스크 추가**: `FStateTreeTaskCommonBase` 상속, State를 읽지 않는 순수 비주얼 노드는 어떤 기믹이든 재사용. 머무는(완료 없는) 태스크는 에셋에서 상태 완료 판정에서 제외해야 thrash 방지
- **데이터 주도 설정**: `UWxWorldDeveloperSettings`(에디터 스포너 아이콘 맵), 스포너 모드(`EWxSpawnerMode::Auto/Manual`)·`bNeverRevive`(보스 영구사망)
- **리플리케이션/권한**: State는 서버 권위에서 확정·복제, 상호작용 발화는 `MulticastInteracted`. WxSave 키는 에디터에서 부여되는 불변 `FGuid`(`GetWxSaveId`)

## 여기서부터 읽어라
1. `Source/WxWorld/Public/Gimmick/WxGimmick.h` — 베이스의 상태 구동 패턴 doc-comment가 전 기믹 공통 규약을 설명한다
2. `Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` — 기믹이 조합하는 ST 태스크 카탈로그(이게 비주얼/연출의 실제 구현)
3. `Source/WxWorld/Public/Interaction/WxInteractionComponent.h` — 오버랩→레지스트리→서버→Multicast 상호작용 흐름의 진입점
4. `Source/WxWorld/Public/Gimmick/WxDoor.h` / `WxElevator.h` — State+ST 패턴이 실제 기믹에서 어떻게 author 되는지의 예시

## 관련
- 상위: 상호작용 입력/타겟팅은 [[WxCombat]]의 `WxAbility_Interact`, HUD 프롬프트는 [[WxUI]]. 스폰 대상 캐릭터/AI는 [[WxAI]]. 인터페이스·세이브 계약은 [[WxCore]], 직렬화는 [[WxSave]], 보상은 [[WxInventory]]

---
*문서 기준 커밋 `75a730d` · 생성일 2026-06-21 · 소스 30파일 — `/readme-writer`로 갱신*
