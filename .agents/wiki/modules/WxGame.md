# WxGame — 게임 모듈 (조립 계층)

작업 단계: 구현

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> 플러그인으로 나눠 놓은 도메인들을 실제 게임으로 엮는 최상위 모듈이다. 캐릭터·컨트롤러·GameMode 같은 프레임워크 클래스에 각 도메인의 컴포넌트를 붙이고, 두 도메인이 서로를 모르는 탓에 어느 쪽에도 둘 수 없는 연결 코드를 떠맡는다.

## 책임

**담당**
- 게임 프레임워크 클래스 일체: `AWxGameMode` / `AWxGameState` / `AWxPlayerState` / `AWxPlayerController` / `AWxAIController`와 캐릭터 계층(`AWxCharacterBase` → `AWxPlayerCharacter` · `AWxEnemyCharacter`).
- 컴포넌트 조립 — 어느 도메인 컴포넌트를 어느 액터가 소유하는지를 여기서 정한다. 폰 리스폰에도 살아남아야 하는 것(인벤토리·상호작용 스캐너·대화 세션·HUD 레이아웃)은 PlayerController가, 캐릭터 수명과 같이 가는 것(ASC·락온·히트스톱·모션워핑)은 캐릭터가, 월드 단위인 것(퀘스트·스킬 컷신)은 GameState가 소유한다.
- 게임플레이 입력 소유와 GAS 배선: `UWxInputConfig`의 이동/시선/점프/웅크리기 바인딩, 그리고 ASC가 AbilitySet에서 파생시킨 어빌리티 입력 액션의 동적 바인딩.
- 팀/피아 판정의 근거 데이터(`EWxTeam`)와 그 해석을 캐릭터·AI 컨트롤러 양쪽에 내려주는 일.
- 도메인 사이를 잇는 접착 코드 — MVVM 뷰모델·리졸버(UI가 볼 수 없는 인벤토리·퀘스트·대화·상호작용 컴포넌트를 물어 위젯에 노출), AI 블랙보드 타겟을 락온 컴포넌트로 옮기는 통로, 퀘스트 대기 상태로 NPC 대화 가능 여부를 정하는 판정.
- 프론트엔드 → 전투 맵 전환(`UWxGameFlowSubsystem`)과 체크포인트 리스폰(`UWxRespawnLibrary`), 그리고 개발용 치트(`UWxCheatManager`).
- 메타휴먼 외형 조립(`UWxMetaHumanComponent`)과 비대칭 중력 점프 등 캐릭터 이동 감각(`UWxCharacterMovementComponent`).

**경계 (비담당)**
- 전투 규칙·대미지·락온·히트스톱·어빌리티 베이스 → [WxCombat](WxCombat.md). 이 모듈의 어빌리티는 `UWxAbilityBase`를 상속만 한다.
- 아이템 정의·인벤토리 저장·보상 테이블 → [WxInventory](WxInventory.md).
- 위젯·HUD 레이아웃·뷰모델 베이스(`UWxViewModel`) → [WxUI](WxUI.md). 여기 있는 뷰모델은 WxUI가 볼 수 없는 도메인을 물기 위한 파생일 뿐이다.
- 상호작용 대상·스캐너·스포너·체크포인트 → [WxWorld](WxWorld.md).
- 인지·행동트리 서비스·AI 행동 설정 → [WxAI](WxAI.md). `AWxAIController`는 감각의 모양만 잡고 수치는 폰의 `UWxAIBehaviorComponent`가 밀어 넣는다.
- 대화 데이터·진행(`AWxDialogueActor`, `UWxDialogueSessionComponent`) → [WxDialogue](WxDialogue.md), 퀘스트 저널·목표 판정 → [WxQuest](WxQuest.md).
- Native Gameplay Tag 선언과 공용 정의(`WxGameplayTags`, `IWxUIData`) → [WxCore](WxCore.md). 이 모듈은 태그를 선언하지 않고 참조만 한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 플레이어·적이 갈라지기 전 공통 지점. ASC를 PlayerState가 아니라 캐릭터가 직접 소유하는 구조가 여기서 결정되고, 사망·래그돌·팀 판정이 모두 이 클래스를 거친다 | [Source/WxGame/Character/WxCharacterBase.h](../../../Source/WxGame/Character/WxCharacterBase.h) |
| `AWxPlayerCharacter` | 게임플레이 입력의 소유자. `UWxInputConfig`의 고정 바인딩과 ASC가 내주는 어빌리티 입력 액션이 여기서 만난다 | [Source/WxGame/Character/WxPlayerCharacter.h](../../../Source/WxGame/Character/WxPlayerCharacter.h) |
| `AWxEnemyCharacter` | AI·상호작용(백스탭)·보상·보스 표시를 한 액터에 모은 조립점. 보스 교전 상태는 전역 델리게이트로 UI에 나간다 | [Source/WxGame/Character/WxEnemyCharacter.h](../../../Source/WxGame/Character/WxEnemyCharacter.h) |
| `AWxPlayerController` | 폰보다 오래 살아야 하는 플레이어 단위 컴포넌트들의 소유자. 인벤토리·상호작용·대화·HUD 레이아웃이 붙는 자리 | [Source/WxGame/Controller/WxPlayerController.h](../../../Source/WxGame/Controller/WxPlayerController.h) |
| `AWxAIController` | AI 폰 전부의 컨트롤러. 블랙보드 타겟 → 락온 컴포넌트 전달이 여기에만 있는 통로다 | [Source/WxGame/Controller/WxAIController.h](../../../Source/WxGame/Controller/WxAIController.h) |
| `AWxGameMode` | GameState·PlayerState 클래스를 고정하고, 폰 클래스를 프론트엔드 선택(`UWxGameFlowSubsystem`) → `DefaultPawnClass` 순으로 고른다 | [Source/WxGame/Framework/WxGameMode.h](../../../Source/WxGame/Framework/WxGameMode.h) |
| `UWxGameFlowSubsystem` | 프론트엔드 선택(폰+목적지)을 들고 맵을 여는 GameInstance 서브시스템. 선택은 목적지 월드에 있는 동안만 유지된다 | [Source/WxGame/FrontEnd/WxGameFlowSubsystem.h](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.h) |
| `UWxViewModel_Inventory` 외 | MVVM 폴더의 뷰모델·리졸버 묶음. WxUI와 도메인 플러그인 양쪽에 의존할 수 있는 유일한 자리라서 여기 있다 | [Source/WxGame/MVVM/](../../../Source/WxGame/MVVM/) |

## 확장 포인트 / 규약
- **새 캐릭터**: `AWxCharacterBase`(추상)를 상속하거나 `AWxPlayerCharacter` / `AWxEnemyCharacter`의 BP를 만든다. 스탯·어빌리티는 C++이 아니라 ASC에 지정한 AbilitySet에서 온다 — `InitAbilitySystem()`이 어트리뷰트 콜백을 먼저 걸고 `GiveAbilitySets()`를 호출하는 순서에 의존한다. 대화만 하는 캐릭터는 폰이 아니라 `AWxNpc`(`AWxDialogueActor` 파생)를 쓴다.
- **새 게임플레이 입력**: 이동류면 `UWxInputConfig`에 IA를 추가하고 `AWxPlayerCharacter::SetupPlayerInputComponent`에서 바인딩한다. 어빌리티 발동 입력은 여기 넣지 않는다 — IA는 어빌리티 CDO가 들고 있고 바인딩 목록은 AbilitySet에서 파생된다. 메뉴/UI 입력은 CommonUI 액션(WxUI) 쪽이다.
- **새 HUD 데이터**: `UWxViewModel`(WxUI) 파생 뷰모델과 짝이 되는 `UMVVMViewModelContextResolver` 파생을 `MVVM/`에 함께 둔다. 리졸버에 플레이어별 상태를 두지 않고 상태·구독은 뷰모델이 가진다. 도메인 컴포넌트의 늦은 도착·교체는 뷰모델의 관찰 경로에서 처리한다.
- **데이터 주도 설정**: `AWxEnemyCharacter::RewardRow`는 `WxRewardTableRow`(WxInventory) DataTable 행을, `UWxAbility_UseItem::ConsumableDef`는 Usable/Charges Fragment를 가진 `UWxItemDefinition`을 요구한다. 프론트엔드 목록은 `FWxFrontEndOption`(폰 클래스+레벨)을 WBP에서 채운다.
- **권한 모델**: 캐릭터 ASC는 서버 `PossessedBy`, 플레이어는 클라 `OnRep_PlayerState`에서 초기화한다. `UWxAbility_Interact`는 ServerOnly이고 클라 스캐너의 선택을 서버가 `CanInteract`와 쿼리 충돌 기반 사거리로 재검증한다. `UWxRespawnLibrary::RequestRespawn`과 `UWxGameFlowSubsystem::RequestNewGame`은 Standalone 전용이다.

## 여기서부터 읽어라
1. [Source/WxGame/WxGame.Build.cs](../../../Source/WxGame/WxGame.Build.cs) — 이 모듈이 어느 도메인 플러그인을 조립하는지가 의존 목록에 그대로 드러난다.
2. [Source/WxGame/Character/WxCharacterBase.h](../../../Source/WxGame/Character/WxCharacterBase.h) — 캐릭터가 어떤 컴포넌트를 소유하는지, ASC/사망/팀이 어디에 사는지가 한 화면에 있다.
3. [Source/WxGame/Controller/WxPlayerController.h](../../../Source/WxGame/Controller/WxPlayerController.h) — 캐릭터가 아니라 컨트롤러에 붙는 것이 무엇이고 왜인지(폰 수명 초과) 설명한다. 캐릭터 헤더와 짝으로 본다.
4. [Source/WxGame/MVVM/WxViewModel_InteractionList.h](../../../Source/WxGame/MVVM/WxViewModel_InteractionList.h) — 도메인↔UI 접착이 왜 이 모듈에 있는지 보여 주는 대표 사례.
5. [Source/WxGame/FrontEnd/WxGameFlowSubsystem.h](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.h) + [Source/WxGame/Framework/WxGameMode.cpp](../../../Source/WxGame/Framework/WxGameMode.cpp) — 프론트엔드에서 전투 맵까지의 흐름이 이 둘 사이에서 완결된다.

## 검증 범위와 근거

[Build.cs](../../../Source/WxGame/WxGame.Build.cs)·[uproject](../../../Wx.uproject), 캐릭터·컨트롤러·GameMode·GameState의 컴포넌트 조립과 다음 경로를 확인했다.

- [CharacterBase](../../../Source/WxGame/Character/WxCharacterBase.cpp)·[PlayerCharacter](../../../Source/WxGame/Character/WxPlayerCharacter.cpp): ASC 소유, 속성 콜백 연결 후 서버 AbilitySet 부여, EnhancedInput 배선. 실제 입력 에셋 값은 미검증이다.
- [PlayerController](../../../Source/WxGame/Controller/WxPlayerController.cpp)·[GameState](../../../Source/WxGame/Framework/WxGameState.cpp): 인벤토리·스캐너·대화·레이아웃 및 퀘스트·컷신의 네이티브 부착.
- [EnemyCharacter](../../../Source/WxGame/Character/WxEnemyCharacter.cpp): `UWxMinionComponent`·Nameplate의 네이티브 부착, 스포너 사망 통지와 보상 지급. 소환되지 않은 일반 적은 Instigator 주인이 없으므로 소환물로 등록하지 않는다.
- [AIController](../../../Source/WxGame/Controller/WxAIController.cpp): BT 실행과 블랙보드 초기화, `UWxMinionComponent::GetMaster`를 통한 Master 전달, TargetActor 변경의 락온 연결.
- [Interact](../../../Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp), [리스폰](../../../Source/WxGame/Framework/WxRespawnLibrary.cpp), [게임 진입](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp)의 권한·실패 분기.

MVVM은 모듈 간 연결 구조를 확인한 범위이며 모든 표시 필드·리졸버 수명, 메타휴먼 조립·이동·치트의 세부 동작 및 실제 멀티 플레이는 전수 검증하지 않았다.

## 관련 모듈
- 하위(위임 대상): [WxCore](WxCore.md) · [WxCombat](WxCombat.md) · [WxInventory](WxInventory.md) · [WxUI](WxUI.md) · [WxWorld](WxWorld.md) · [WxAI](WxAI.md) · [WxDialogue](WxDialogue.md) · [WxQuest](WxQuest.md)
- 상위: 없음 — 이 모듈이 최상위 게임 모듈(`IMPLEMENT_PRIMARY_GAME_MODULE`)이며, 맵·BP 에셋이 여기 클래스를 파생해 쓴다.

---
*이관 원문의 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 57파일 — 원문 출처 보존; 현재 확인 범위는 상단과 검증 절 참고*
