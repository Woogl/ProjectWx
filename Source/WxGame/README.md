# WxGame — 게임 조립 모듈

> 도메인 플러그인(Combat·Inventory·AI·Dialogue·Quest·World·UI)을 하나의 플레이 가능한 게임으로 합치는 최상위 모듈. 프레임워크 골격(GameMode·GameState·Controller·PlayerState), Lyra식 Experience 로드 파이프라인, 구체 캐릭터/컨트롤러, 게임↔UI 뷰모델 브리지를 소유한다.

## 책임
**담당**
- 게임 프레임워크 골격: `AWxGameMode`·`AWxGameState`·`AWxPlayerController`·`AWxAIController`·`AWxPlayerState`·`AWxWorldSettings`.
- Experience 시스템(`Framework/`): 어떤 GameFeature·컴포넌트·시작 지급으로 이 판을 구성할지 데이터로 정의하고, 서버가 고른 정의를 복제해 서버·클라가 각자 비동기 로드·적용한다.
- 구체 캐릭터/폰 조립: `AWxCharacterBase`가 도메인 플러그인의 전투·장비·이동 컴포넌트를 한 액터에 모으고, `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxMinion`/`AWxNpc`가 파생한다.
- 게임플레이 입력(이동/시선/점프/크라우치)과 그 구성 애셋(`UWxInputConfig`).
- 게임↔UI 브리지(`MVVM/`): 게임 모듈의 Pawn·PlayerController·ASC 데이터를 WxUI 소속 뷰모델에 주입하는 리졸버·뷰모델.

**경계 (비담당)**
- 전투·인벤토리·AI·대화·퀘스트·월드 상호작용의 규칙과 컴포넌트 구현은 각 도메인 플러그인([[WxCombat]]·[[WxInventory]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]]·[[WxWorld]])에 있다. 이 모듈은 그 컴포넌트를 캐릭터에 조립·주입만 한다.
- 뷰모델 클래스 정의와 위젯은 [[WxUI]]. 이 모듈은 게임 데이터를 그 뷰모델에 밀어넣는 리졸버만 가진다.
- 콘텐츠 묶음(어떤 GameFeature를 켤지, 시작 아이템)은 `Plugins/GameFeatures/`의 GameFeature 플러그인과 Experience 애셋 데이터로 정의된다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정 → 매니저에 위임, 로드 완료까지 폰 스폰·시작 지급 게이팅 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 한 판의 구성(폰 클래스·GameFeature·액션·액션셋)을 담는 프라이머리 데이터 에셋 | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState에 상주해 로드 파이프라인(번들→GameFeature→액션)을 주행하는 주체 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 넷모드 무관하게 receiver 액터에 컴포넌트를 주입하는 Experience 액션 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 도메인 컴포넌트(ASC·전투·장비·이동)를 조립하는 공통 베이스, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 플레이어 입력·카메라·락온 소유, `UWxInputConfig`로 입력 주입 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | Spawnable·Interactable(피니셔) 구현, 락온·네임플레이트·보상 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 Pawn의 ASC·표시 데이터를 WxUI 뷰모델에 주입하는 브리지 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 판(모드) 추가**: `UWxExperienceDefinition` 인스턴스를 네이티브 클래스로 만들고(BP 서브클래스 금지 — PrimaryAssetType 이탈) 폰 클래스·`GameFeaturesToEnable`·`Actions`·`ActionSets`를 채운다. 여러 판이 공유하는 묶음은 `UWxExperienceActionSet`으로 뽑는다. 확정은 진입 URL `?Experience=이름` → `AWxWorldSettings` 순.
- **캐릭터에 기능 추가**: 도메인 컴포넌트를 만들고 Experience의 `AddComponents` 액션으로 주입한다. 대상 액터는 ModularGameplay receiver(캐릭터·PlayerController·PlayerState·GameState)로 opt-in돼 있어야 한다.
- **시작 지급**: `UWxExperienceActionSet::DefaultInventoryItems`(`FWxItemRewardEntry`)·`GameHUDClass`가 구동한다. GameMode가 로드 완료 시점에 컨트롤러 인벤토리에 넣는다.
- **리플리케이션 모델**: GameMode는 서버 전용. Experience 참조를 매니저가 복제(`OnRep_CurrentExperience`)해 서버·클라가 같은 로드 파이프라인을 각자 주행한다. 복제 컴포넌트는 엔진이 authority에서만 생성한다.
- **입력**: 게임플레이 입력은 `AWxPlayerCharacter`가 `UWxInputConfig`로 받고, 어빌리티 발동 IA는 어빌리티 CDO/AbilitySet에서, 메뉴 입력은 CommonUI(`WxHUDLayout`)에서 나온다 — 세 경로가 분리돼 있다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — Experience 로드 파이프라인의 상태 머신. 게임이 어떻게 구성되는지의 핵심.
2. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정과 폰 스폰 게이팅의 전 흐름이 헤더 주석에 정리돼 있다.
3. `Source/WxGame/Character/WxCharacterBase.h` — 도메인 플러그인 컴포넌트가 한 캐릭터에 어떻게 조립되는지.
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 게임 데이터가 WxUI로 건너가는 브리지 패턴.

## 관련
- 상위: 최상위 조립 모듈이라 상위 코드 참조는 없다. 콘텐츠는 [[WxFishing]] 등 `Plugins/GameFeatures/`의 GameFeature 플러그인이 Experience를 통해 얹는다.
- 함께 보기: 조립 대상인 [[WxCombat]]·[[WxInventory]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]]·[[WxWorld]]·[[WxUI]], 공용 정의 [[WxCore]].

---
*문서 기준 커밋 `f0aad4c` · 생성일 2026-09-03 · 소스 71파일 — `/readme-writer`로 갱신*
