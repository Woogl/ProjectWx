# WxGame — 기본 게임 모듈 (부트스트랩·조립)

> 도메인 플러그인(WxCombat·WxInventory·WxUI·WxAI·WxDialogue·WxQuest·WxSave·WxWorld)과 엔진 프레임워크를 하나의 플레이 가능한 게임으로 조립하는 최상위 게임 모듈. GameMode·GameState·Character·Controller 등 프레임워크 구현과 Lyra식 Experience/GameFeature 부트스트랩을 담는다.

## 책임
**담당**
- 게임 프레임워크 구현체: `AWxGameMode`, `AWxGameState`, `AWxPlayerController`, `AWxPlayerState`, `AWxWorldSettings`
- 캐릭터 계층: `AWxCharacterBase`(ASC 소유·팀·죽음) → `AWxPlayerCharacter`(카메라·입력)·`AWxEnemyCharacter`(BT·처형)·`AWxBossCharacter`·`AWxNpc`
- Experience/GameFeature 부트스트랩: 어느 Experience를 켤지 확정 → 비동기 로드 → GameFeature 플러그인 활성 → 액션 실행 → 폰 스폰·시작 지급
- 도메인 플러그인 간 접착: 양쪽에 의존해야 성립하는 조립 코드(MVVM 리졸버/뷰모델, Interact/UseItem 어빌리티, AnimNotify)를 이 모듈에 둔다

**경계 (비담당)**
- 전투/인벤토리/대화/퀘스트/세이브/AI/월드/UI의 실제 규칙과 데이터 — 각 도메인 플러그인 소유. 이 모듈은 진입점만 잇는다
- HUD·사망 화면·대화 창 표시 — `UWxUIManagerSubsystem`([[WxUI]])이 컨트롤러 빙의·폰 상태 태그를 따라가며 처리
- 플레이어 스폰 위치·스탯 복원 — `UWxPlayerSpawnComponent`([[WxSave]])가 StartSpot·빙의 시 담당

## 의존성
- **주요 의존**: `WxCore` · `WxCombat` · `WxInventory` · `WxUI` · `WxAI` · `WxDialogue` · `WxQuest` · `WxSave` · `WxWorld` (도메인 플러그인 전부). 엔진: `GameFeatures`·`ModularGameplay`·`GameplayAbilities`·`ModelViewViewModel`(MVVM)·`EnhancedInput`·`MotionWarping`·`MetaHumanSDKRuntime`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정·로드 완료까지 폰 스폰 보류·시작 인벤토리 지급 | `Source/WxGame/Framework/WxGameMode.h` |
| `AWxGameState` | ModularGameplay receiver, Experience 매니저 컴포넌트 거주처 | `Source/WxGame/Framework/WxGameState.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성(폰·GameFeature·액션) PrimaryDataAsset | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | 로드 파이프라인 주체(번들→GameFeature→액션→브로드캐스트), 서버 확정·클라 OnRep | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxExperienceManager` | PIE 다중 세션용 GameFeature 활성 참조 카운팅 엔진 서브시스템 | `Source/WxGame/Framework/WxExperienceManager.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션(스톡 대체) | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxWorldSettings` | 맵별 기본 Experience 지정처 | `Source/WxGame/Framework/WxWorldSettings.h` |
| `AWxCharacterBase` | ASC 직접 소유 공통 캐릭터, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerController` | ModularGameplay receiver(인벤토리·스캐너·대화·PlayerSpawn 주입 대상) | `Source/WxGame/Controller/WxPlayerController.h` |

## 확장 포인트 / 규약
- **Experience 확정 순서** (`AWxGameMode::ResolveExperienceId`): 진입 URL `?Experience=이름` → `AWxWorldSettings::GetDefaultGameplayExperience` → GameMode의 `DefaultExperience`. 실패 시 무효 ID
- **로드 파이프라인** (`UWxExperienceManagerComponent`): `Unloaded → Loading`(에셋 번들 비동기) `→ LoadingGameFeatures`(플러그인 URL 활성) `→ ExecutingActions`(자기 월드 한정 컨텍스트로 액션 실행) `→ Loaded`(브로드캐스트). GameMode가 `CallOrRegister_OnExperienceLoaded`로 대기했다가 폰 스폰·시작 지급
- **서버/클라 대칭**: GameMode는 서버 전용. `CurrentExperience`가 GameState 서브오브젝트로 복제돼 클라는 `OnRep`으로 같은 파이프라인을 독립 주행 — GameMode가 서버에만 있어도 클라 적용 성립
- **GameFeature 조립**: Experience/ActionSet의 `GameFeaturesToEnable`(이름 문자열)로 플러그인 활성. GameFeature 플러그인 → WxGame·도메인 참조는 허용, 역방향 금지(CLAUDE.md 규칙)
- **컴포넌트 주입**: `UWxGameFeatureAction_AddComponents`가 대상 액터를 컴포넌트 클래스의 프레임워크 베이스에서 도출. 대상은 ModularGameplay receiver로 opt-in돼야 함(`AWxGameState`·`AWxPlayerController`·`AWxPlayerState`·`AWxCharacterBase`). 사이드 제한은 컴포넌트 자신이 가드
- **ActionSet 합성**: `UWxExperienceActionSet`으로 여러 Experience가 액션·GameFeature·시작 아이템(`DefaultInventoryItems`)을 공유. Experience 본체 액션과 평탄화돼 함께 실행
- **에셋 규약**: Experience/ActionSet은 네이티브 클래스 인스턴스로만 생성(BP 서브클래스는 PrimaryAssetType이 달라져 스캔·URL 해석에서 누락)

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 전체 부트스트랩의 지휘자. Experience 확정·폰 스폰 보류·시작 지급의 시점 규약이 여기 doc-comment에 정리돼 있다
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 파이프라인의 실제 주체. 상태 머신(`EWxExperienceLoadState`)과 서버/클라 대칭 로드를 이해하는 핵심
3. `Source/WxGame/Framework/WxExperienceDefinition.h` — "한 판의 게임플레이 구성"이 무엇으로 이뤄지는지(폰·GameFeature·액션·ActionSet)의 데이터 스키마
4. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터 계층의 뿌리. ASC 소유 방식·팀·ModularGameplay 주입 대상 규약
5. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 어떤 도메인 플러그인·엔진 서브시스템을 조립하는지의 전체 의존 목록

## 관련
- 상위: 도메인 플러그인 전부 — [[WxCore]] · [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxSave]] · [[WxWorld]]
- GameFeature 콘텐츠 플러그인(`Plugins/GameFeatures/`)이 Experience의 `GameFeaturesToEnable`로 이 모듈의 부트스트랩에 얹힌다

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 66파일 — `/readme-writer`로 갱신*
