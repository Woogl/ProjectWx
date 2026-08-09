# WxGame — 기본 게임 모듈 (조립부)

> 도메인 플러그인들을 하나의 플레이 가능한 게임으로 조립하는 최상위 게임 모듈. Lyra 식 Experience/GameFeature 파이프라인으로 "이 판이 무엇인지"를 데이터로 정의하고, ModularGameplay로 각 프레임워크 액터에 도메인 컴포넌트를 주입하며, 플레이어·NPC·적 액터와 MVVM 뷰모델 브리지를 제공한다.

## 책임
**담당**
- Experience 로드 파이프라인: `AWxGameMode`가 진입 URL/WorldSettings/폴백 순으로 Experience를 확정 → `AWxGameState`의 `UWxExperienceManagerComponent`가 복제·주행(에셋 번들 로드 → GameFeature 플러그인 활성 → 액션 실행)
- ModularGameplay 조립: `UWxGameFeatureAction_AddComponents`로 Pawn/Controller/PlayerState/GameState에 도메인 컴포넌트를 사이드 플래그 없이 주입
- 프레임워크 액터: GameMode/GameState/PlayerController/PlayerState/WorldSettings 및 캐릭터 계층(`AWxCharacterBase` → Player/Enemy/Boss), 대화 NPC(`AWxNpc`)
- 플레이어 입력 결선: Enhanced Input 게임플레이 입력(`AWxPlayerCharacter` + `UWxInputConfig`), MVVM 뷰모델과 도메인 데이터를 잇는 리졸버·뷰모델(`MVVM/`)
- 시작 지급: Experience(ActionSet 합산)가 정의한 시작 아이템을 GameMode가 접속 컨트롤러 인벤토리에 지급

**경계 (비담당)**
- 전투/GAS 실제 로직·어트리뷰트는 [[WxCombat]] (본 모듈은 ASC를 소유·초기화하고 부여만 한다)
- 인벤토리 상태·아이템 정의는 [[WxInventory]], 대화 계약·세션은 [[WxDialogue]], 퀘스트는 [[WxQuest]]
- 화면(HUD/사망/대화창) 표시와 뷰모델 클래스는 [[WxUI]] (본 모듈의 리졸버가 데이터만 주입)
- 세이브/로드·재개 지점·스탯 복원은 [[WxSave]], AI 행동은 [[WxAI]], 월드 상호작용 오브젝트는 [[WxWorld]]

## 의존성
- **주요 의존**: `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxDialogue`, `WxQuest`, `WxSave` (도메인 플러그인 전부) + 엔진: `GameFeatures`, `ModularGameplay`, `GameplayAbilities`, `ModelViewViewModel`(MVVM), `CommonUI`, `MotionWarping`, `EnhancedInput`, `AIModule`, `MetaHumanSDKRuntime`/`HairStrandsCore`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 서버 전용. Experience 확정 → 매니저에 위임, 로드 완료까지 스폰 유예 + 시작 아이템 지급 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceManagerComponent` | GameState에 거주. Experience 참조를 복제해 서버·클라 각자 로드 파이프라인 주행 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성 프라이머리 데이터 에셋(폰 클래스·GameFeature·액션·ActionSet) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxGameFeatureAction_AddComponents` | 프레임워크 액터에 도메인 컴포넌트를 주입하는 GameFeature 액션(사이드 플래그 없음) | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | ASC를 직접 소유하는 공통 베이스 캐릭터(팀·GAS·장비·래그돌), ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 3인칭 카메라·게임플레이 입력 소유. `UWxInputConfig`로 입력 주입, 락온 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | ModularGameplay receiver. 주입 컴포넌트를 중개하지 않고 쓰는 쪽이 직접 조회 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | Pawn 데이터를 WxUI 뷰모델에 주입하는 MVVM 브리지(양쪽 의존을 본 모듈이 흡수) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 게임플레이 구성은 `UWxExperienceDefinition`(및 공유 묶음 `UWxExperienceActionSet`) 에셋으로 정의한다 — 네이티브 클래스 인스턴스로만 만들어야 스캔·URL 해석에 잡힌다. 맵 기본값은 `AWxWorldSettings.DefaultGameplayExperience`.
- 도메인 컴포넌트를 액터에 붙이려면 Experience 액션에 `UWxGameFeatureAction_AddComponents` 엔트리를 추가한다. 대상 액터는 컴포넌트가 상속한 ModularGameplay 베이스에서 도출되며(엔트리에 사이드 플래그·대상 지정 없음), 복제 컴포넌트는 매니저가 authority에서만 만들고 그 외 사이드 제한은 컴포넌트 자신이 한다.
- 리플리케이션 모델: GameMode는 서버 전용이고 Experience 참조만 복제되어 클라가 동일 파이프라인을 재주행한다 — GameMode가 서버에만 있어도 클라 적용이 성립. 로드 완료 전 폰 스폰·시작 지급은 `CallOrRegister_OnExperienceLoaded`로 유예.
- 새 캐릭터는 `AWxCharacterBase` 파생으로 만들고 `InitAbilitySystem`을 통해 ASC를 초기화한다(서버는 `PossessedBy`, 클라는 OnRep 경로). ASC는 PlayerState가 아닌 캐릭터가 직접 소유한다(리스폰 시 스탯 재초기화).
- WxUI 뷰모델에 게임 데이터를 잇는 위젯은 `MVVM/`에 리졸버/뷰모델을 추가한다 — WxUI가 게임 모듈을 참조할 수 없으므로 브리지는 양쪽에 의존하는 이 모듈이 담당한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — Experience 로드 파이프라인 전체(상태 머신·복제·GameFeature 활성)가 한 파일에 요약됨. 게임 부팅 흐름의 심장.
2. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정·스폰 유예·시작 지급의 결선. 부팅이 어디서 시작하는지.
3. `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` — 도메인 컴포넌트가 어떻게 액터에 붙는지(조립 규약).
4. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 ASC·팀·장비·래그돌을 어떻게 엮는지.

## 관련
- 상위: 이 모듈이 [[WxCombat]]·[[WxInventory]]·[[WxUI]]·[[WxWorld]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]]·[[WxSave]]·[[WxCore]]를 조립한다. 콘텐츠는 `Plugins/GameFeatures/`의 GameFeature 플러그인이 Experience의 `GameFeaturesToEnable`로 얹힌다.

---
*문서 기준 커밋 `81dd309` · 생성일 2026-08-09 · 소스 64파일 — `/readme-writer`로 갱신*
