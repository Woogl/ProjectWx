# WxGame — 게임 조립 모듈

> 프레임워크(GameMode·GameState·Controller·Character)를 구현하고, 각 `Wx` 플러그인이 제공하는 컴포넌트·어빌리티·뷰모델을 실제 액터 위에 조립하는 유일한 지점이다. 플러그인끼리 서로를 볼 수 없으므로, **둘 이상의 도메인이 만나는 배선은 전부 여기에 있다.**

## 책임

**담당**
- 액터 조립: 어떤 플러그인 컴포넌트를 어느 프레임워크 액터가 소유하는지 결정한다. 캐릭터(ASC·락온·히트스톱·모션워핑·메타휴먼), 플레이어 컨트롤러(인벤토리·상호작용 스캐너·대화 세션·UI 레이아웃), GameState(퀘스트·스킬 컷신).
- 프레임워크 클래스 계층: `AWxGameMode`/`AWxGameState`/`AWxPlayerState`와 캐릭터 3종(플레이어·적·NPC), 컨트롤러 2종(플레이어·AI).
- 도메인 횡단 배선: AI 블랙보드 타겟 → 락온 타겟, 백스탭 상호작용 → 피니셔 이벤트, 적 처치 → 보상 테이블, NPC 상호작용 가능 여부 → 퀘스트 대기 상태.
- MVVM 뷰모델·리졸버 중 **둘 이상의 도메인을 참조해야 하는 것들** (`MVVM/`). 예: 퀘스트 컴포넌트를 구독하는 HUD 뷰모델, ASC에서 스킬 슬롯을 찾는 리졸버.
- 게임플레이 입력 바인딩(`UWxInputConfig` + Enhanced Input)과 어빌리티 입력 액션 바인딩.
- 프론트엔드 → 전투 맵 전환(`UWxGameFlowSubsystem`)과 단일 플레이 리스폰(`UWxRespawnLibrary`).
- 치트(`UWxCheatManager`), 메타휴먼 외형 조립(`UWxMetaHumanComponent`), 비대칭 중력 이동(`UWxCharacterMovementComponent`).

**경계 (비담당)**
- 전투 규칙·ASC 구현·어빌리티 베이스·락온·피니셔·히트스톱 → [[WxCombat]]
- 인벤토리 데이터·아이템 정의·사용 처리 → [[WxInventory]]
- 위젯·HUD 레이아웃·`UWxViewModel` 베이스·네임플레이트 → [[WxUI]]
- 상호작용 스캔·스포너·체크포인트 → [[WxWorld]]
- 퍼셉션 설정값·BT/블랙보드 자산·행동 컴포넌트 → [[WxAI]]
- 대화 세션·대사 진행 → [[WxDialogue]]
- 퀘스트 저널·목표 판정 → [[WxQuest]]
- Gameplay Tag 선언·충돌 채널·공용 인터페이스 → [[WxCore]] (이 모듈은 선언하지 않고 쓰기만 한다)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 모든 캐릭터의 조립 지점. ASC를 PlayerState가 아닌 캐릭터가 직접 소유하고, 사망·래그돌 태그 구독과 `GiveAbilitySets()` 호출이 여기서 일어난다 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 카메라·Enhanced Input·아이템 사용·입력 버퍼를 더한 플레이어 쪽 조립 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | AI·네임플레이트·락온 포인트를 더한 적 쪽 조립. 백스탭 상호작용과 보상 지급, 보스 교전 상태 방송을 겸한다 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 폰 리스폰에도 살아남아야 하는 플레이어 단위 컴포넌트(인벤토리·스캐너·대화·레이아웃)의 소유자 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxAIController` | 퍼셉션의 모양을 잡고, BT가 고른 타겟을 `UWxLockOnComponent`로 옮기는 AI↔전투 통로 | `Source/WxGame/Controller/WxAIController.h` |
| `AWxGameMode` | 맵별 BP로 갈리며, 폰 클래스를 프론트엔드 선택값 → `DefaultPawnClass` 순으로 고른다 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | 월드 단위 컴포넌트(퀘스트·스킬 컷신)의 소유자 | `Source/WxGame/Framework/WxGameState.h` |
| `UWxGameFlowSubsystem` | 프론트엔드에서 고른 폰·목적지를 들고 맵을 여는 전환 상태 보관소. GameMode가 이 선택을 읽는다 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |

## 확장 포인트 / 규약
- **새 캐릭터**: `AWxCharacterBase` 파생 BP를 만든다. 무기는 `WeaponActor`(ChildActorComponent)의 클래스로, 외형은 `MetaHumanComponent`의 에셋 슬롯으로, 어빌리티·어트리뷰트는 ASC의 `AbilitySets`(`UWxAbilitySet`, WxCombat)로 지정한다 — C++ 수정 없이 데이터로 끝난다.
- **새 적**: `AWxEnemyCharacter` 파생 BP에서 `bIsBoss`, `RewardRow`(`FWxRewardTableRow`, WxInventory), `AIBehaviorComponent`의 감지 설정을 채운다.
- **새 게임플레이 입력**: 이동·시선·점프·앉기만 `UWxInputConfig`에 둔다. 어빌리티 발동 IA는 어빌리티 CDO가 들고 AbilitySet에서 파생되며, 메뉴/UI 입력은 CommonUI 액션이 받는다 — `UWxInputConfig`를 늘리기 전에 어느 쪽인지 먼저 가른다.
- **새 뷰모델**: 한 도메인 안에서 끝나면 그 플러그인에 두고, 두 도메인을 동시에 봐야 할 때만 `MVVM/`에 둔다. 위젯과의 결합은 `UMVVMViewModelContextResolver` 파생(리졸버)이 담당하며, 리졸버는 위젯 클래스가 공유하므로 상태를 갖지 않고 뷰모델만 만들어 돌려준다.
- **리플리케이션/권한 (최대 4인 멀티)**: ASC는 캐릭터 소유이고 플레이어는 Mixed 복제 모드다. `PossessedBy`(서버)와 `OnRep_PlayerState`(클라)가 같은 `InitAbilitySystem()`으로 모이며, `GiveAbilitySets()`는 권위에서만 돈다. 반면 사망·래그돌 구독은 시뮬 프록시를 포함한 전 머신에서 필요하므로 `PostInitializeComponents`에 있다. `Team`은 복제되고 피아 판정의 단일 출처다.
- **리스폰**: `UWxRespawnLibrary::RequestRespawn`은 Standalone 전용이며 사망 위젯이 호출한다. 체크포인트(WxWorld) → `RestartPlayer` → 어트리뷰트 재초기화 → 스포너 리셋 순으로 진행한다.

## 여기서부터 읽어라
1. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터에 무엇이 붙는지가 곧 이 게임의 시스템 목록이다. 생성자와 `InitAbilitySystem`을 같이 본다.
2. `Source/WxGame/Controller/WxPlayerController.cpp` — 20줄짜리 생성자 하나가 플레이어 단위 시스템 4개의 소속을 정한다.
3. `Source/WxGame/Character/WxEnemyCharacter.cpp` — AI·전투·인벤토리·UI가 한 액터에서 만나는 가장 밀도 높은 배선 예시(백스탭 → 피니셔 이벤트, 사망 → 보상, 보스 교전 방송).
4. `Source/WxGame/MVVM/` — HUD가 어느 컴포넌트를 구독해 어떤 값을 끌어오는지. 한 파일 안에 뷰모델과 그 리졸버가 같이 있다.
5. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` + `Source/WxGame/Framework/WxGameMode.cpp` — 타이틀에서 전투 맵까지의 흐름.

## 관련
- 상위: 이 모듈이 최종 소비자다. 조립 대상은 [[WxCore]] · [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]].

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 57파일 — `/readme-writer`로 갱신*
