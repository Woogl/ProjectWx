# WxGame — 게임 조립 모듈

> 도메인 플러그인(Combat·Inventory·UI·AI·Dialogue·Quest·Save·World)을 하나의 플레이 가능한 게임으로 엮는 최상위 게임 모듈. GameMode/State, 캐릭터·컨트롤러 프레임워크, Experience 로 켜지는 데이터 주도 게임플레이 구성을 담당한다.

## 책임
**담당**
- Experience 로드 파이프라인 (Lyra 이식): GameMode 가 이 판의 Experience 를 확정 → GameState 의 매니저 컴포넌트가 참조를 복제 → 서버·클라 각자 GameFeature 활성 + 액션 실행.
- 게임 프레임워크 구체 클래스: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`/`AWxWorldSettings`.
- 캐릭터 계층: 공통 베이스부터 Player/Enemy/Boss/NPC 까지의 조립처. ASC·장비·MetaHuman·이동 컴포넌트를 얹는다.
- 플레이어 입력 바인딩(이동/시선/점프/크라우치)과 `UWxInputConfig` 매핑.
- WxUI 뷰모델과 게임 런타임 데이터를 잇는 MVVM 리졸버(양쪽에 동시 의존하는 경계 코드).
- 도메인 플러그인의 구체 조합이 필요한 액터(예: 대화+외형이 걸친 `AWxNpc`)와 구체 어빌리티(Interact/UseItem).

**경계 (비담당)**
- 전투 로직·어빌리티 프레임워크·어트리뷰트는 [[WxCombat]], 인벤토리는 [[WxInventory]], 위젯·뷰모델 정의는 [[WxUI]], AI 판단은 [[WxAI]], 대화·퀘스트·세이브·월드 상호작용은 각각 [[WxDialogue]]·[[WxQuest]]·[[WxSave]]·[[WxWorld]] 에 위임.
- 콘텐츠 단위 기능(예: WxFishing)은 `Plugins/GameFeatures/` 의 GameFeature 플러그인이 담당하며, Experience 의 `GameFeaturesToEnable` 이름 문자열로만 켜진다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정·폰 스폰 지연·시작 지급의 서버측 오케스트레이터 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceManagerComponent` | GameState 에 사는 로드 파이프라인 주체(복제+상태머신) | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성 프라이머리 데이터 에셋(폰·액션·GameFeature) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxGameFeatureAction_AddComponents` | Experience/GF 가 대상 액터에 컴포넌트를 주입하는 액션 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | Player/Enemy 공통 베이스(ASC 직접 소유, ModularGameplay receiver) | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력·카메라·락온을 소유한 플레이어 폰 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | Experience 가 주입하는 컨트롤러 컴포넌트의 receiver | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | WxUI 뷰모델을 폰 런타임 데이터로 채우는 경계 리졸버 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 게임플레이 구성 추가**: `UWxExperienceDefinition` 네이티브 인스턴스 에셋을 만들고 `DefaultPawnClass`·`Actions`·`GameFeaturesToEnable`·`ActionSets` 를 채운다. BP 서브클래스 인스턴스는 PrimaryAssetType 이 달라져 스캔에서 빠지므로 금지. 여러 Experience 가 공유할 묶음은 `UWxExperienceActionSet` 으로 분리.
- **Experience 선택 우선순위**(`AWxGameMode::ResolveExperienceId`): 진입 URL `?Experience=이름` → `AWxWorldSettings::GameplayExperience` → 자체 폴백.
- **액터에 컴포넌트 주입**: `UWxGameFeatureAction_AddComponents` 의 `ComponentList` 에 컴포넌트 클래스를 추가. 대상 액터는 컴포넌트 베이스가 상속한 프레임워크 receiver 로부터 도출되며(액터마다 별도 지정 불필요), 대상 액터는 ModularGameplay receiver 로 opt-in 돼 있어야 한다(`AWxPlayerController`/`AWxPlayerState`/`AWxCharacterBase`/`AWxGameState`).
- **리플리케이션 모델**: GameMode 는 서버 전용. Experience 참조는 GameState 서브오브젝트로 복제되어 클라가 `OnRep` 으로 같은 로드 파이프라인을 주행. 복제 컴포넌트는 엔진이 authority 로 제한하고, 비복제 컴포넌트의 사이드 제한은 컴포넌트 자신이 한다.
- **UI ↔ 런타임 경계**: WxUI 의 뷰모델은 게임 모듈을 참조할 수 없으므로, 양쪽에 의존하는 `MVVM/WxViewModelResolver_*` 가 위젯 생성 시 폰/ASC 에서 데이터를 끌어와 주입한다. 새 뷰모델 연동은 리졸버로 추가.
- **PIE 다중 세션**: `UWxExperienceManager`(엔진 서브시스템)가 GameFeature 활성 요청을 URL 별로 카운팅해, 한 세션 종료가 타 세션이 쓰는 플러그인을 끄지 않게 한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 판이 어떻게 시작되는지(Experience 확정 → 폰 스폰 지연 → 시작 지급)의 전모가 클래스 주석에 있다.
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 상태머신(`EWxExperienceLoadState`)과 서버/클라 파이프라인. 게임이 "켜지는" 지점.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 어떤 컴포넌트로 조립되는지, ASC 소유·팀·사망 흐름.
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — UI 와 게임 런타임을 잇는 경계가 왜/어떻게 존재하는지.

## 관련
- 상위: 없음 — 이 모듈이 DAG 최상단의 실행 조립처다. 콘텐츠 확장은 `Plugins/GameFeatures/` 의 GameFeature 플러그인이 Experience 를 통해 얹힌다.
- 함께: [[WxCombat]]·[[WxInventory]]·[[WxUI]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]]·[[WxSave]]·[[WxWorld]]·[[WxCore]] — 이 모듈이 엮는 도메인 플러그인들.

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 66파일 — `/readme-writer`로 갱신*
