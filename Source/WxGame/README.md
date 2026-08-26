# WxGame — 게임 조립 모듈

> 도메인 플러그인(WxCombat·WxInventory·WxUI 등)을 하나의 플레이 세션으로 엮는 최상위 게임 모듈. GameMode·GameState·Controller·Character 등 프레임워크 골격과 Lyra식 Experience/GameFeature 부트스트랩을 제공한다.

## 책임
**담당**
- 프레임워크 골격: GameMode·GameState·PlayerController·PlayerState·Character 계층 및 팀/입력 배선.
- Experience 부트스트랩: 이 판의 Experience 확정 → 비동기 로드 → GameFeature 플러그인 활성 → 액션 실행 → 폰 스폰·시작 지급까지의 파이프라인.
- 도메인 시스템의 조립점: ModularGameplay receiver로 각 도메인이 요청한 컴포넌트를 폰/컨트롤러/스테이트에 주입받게 하고, MVVM ViewModel/Resolver로 도메인 상태를 UI에 연결.

**경계 (비담당)**
- 전투/스탯 로직 — [[WxCombat]] (ASC·AttributeSet·무기·락온).
- 인벤토리·상호작용 — [[WxInventory]] · [[WxWorld]].
- 위젯·HUD 레이아웃 — [[WxUI]] (CommonUI 액션).
- 대화·퀘스트·세이브·AI — [[WxDialogue]] · [[WxQuest]] · [[WxSave]] · [[WxAI]].
- 콘텐츠 활성 단위 — `Plugins/GameFeatures/` (Experience가 이름으로 지목).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 서버 전용. Experience 확정·로드 대기·폰 스폰·시작 인벤토리 지급의 조율자 | `Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성을 정의하는 프라이머리 데이터 에셋(폰 클래스·GameFeature·액션) | `Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState에 붙어 Experience를 복제·비동기 로드·적용하는 실제 주체 | `Framework/WxExperienceManagerComponent.h` |
| `AWxGameState` | ModularGameplay receiver이자 Experience 매니저 컴포넌트의 거주처 | `Framework/WxGameState.h` |
| `AWxWorldSettings` | 맵이 자기 기본 Experience를 지정하는 자리(URL 다음, 확정의 마지막 단계) | `Framework/WxWorldSettings.h` |
| `AWxPlayerController` | Experience가 요청한 컨트롤러 컴포넌트(인벤토리·상호작용·대화·PlayerSpawn 등) 주입 대상 | `Controller/WxPlayerController.h` |
| `AWxCharacterBase` | ASC를 직접 소유하는 플레이어/에너미 공통 베이스, 폰 대상 컴포넌트 receiver | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력(이동/시선/어빌리티) 소유. `UWxInputConfig`로 입력 주입 | `Character/WxPlayerCharacter.h` |

## 확장 포인트 / 규약
- 게임플레이 구성은 코드가 아니라 **Experience 에셋**이 정한다: 폰 클래스(`DefaultPawnClass`), 켤 GameFeature 이름 목록(`GameFeaturesToEnable`), 실행 액션(`Actions`/`ActionSets`). GameMode의 상속 `DefaultPawnClass`는 읽지 않으며, 폰 클래스가 비면 프론트엔드로 보아 스펙테이터 폰으로 빙의한다.
- Experience 확정 우선순위: 진입 URL `?Experience=이름` → `AWxWorldSettings.GameplayExperience`. 그 뒤의 폴백은 없다 — 둘 다 비면 무효 ID가 매니저에서 `Error` 로그로 드러나고 폰이 스폰되지 않는다.
- 여러 Experience가 공유하는 액션·GameFeature·시작 지급·HUD는 `UWxExperienceActionSet`으로 묶어 `ActionSets`에 합성한다. Experience/ActionSet 에셋은 네이티브 클래스 인스턴스로만 만든다(BP 서브클래스는 스캔에서 빠짐).
- 컴포넌트 주입은 `UWxGameFeatureAction_AddComponents`(사이드 플래그 없는 스톡 대체)로 요청하며, 대상 액터는 ModularGameplay receiver로 opt-in돼 있어야 한다(GameState·PlayerController·PlayerState·Character 계열).
- 도메인 상태의 UI 연결은 `MVVM/` ViewModel + Resolver로 한다. Resolver는 게임 모듈과 WxUI 양쪽에 의존해, WxUI 소속 ViewModel이 게임 모듈을 참조하지 못하는 경계를 메운다(예: `WxViewModelResolver_PlayerCharacter`, `WxViewModelResolver_Ability`).
- 로드는 비동기라 접속(PostLogin)보다 늦을 수 있다 — GameMode가 로드 완료까지 폰 스폰을 미루고 대기 접속자를 일괄 처리한다.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 세션 부트스트랩의 전체 흐름(확정→대기→스폰→지급)이 클래스 주석에 요약돼 있다.
2. `Framework/WxExperienceManagerComponent.h` — Experience 로드 상태 머신(`EWxExperienceLoadState`)과 서버/클라 복제 파이프라인.
3. `Character/WxCharacterBase.h` — 캐릭터가 ASC·장비·팀·주입 컴포넌트를 어떻게 물고 있는지.

## 관련
- 하위 도메인: [[WxCore]](foundation) 및 [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxSave]].
- 콘텐츠 상위: `Plugins/GameFeatures/`의 GameFeature 플러그인(Experience가 이름으로 활성).

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 66파일 — `/readme-writer`로 갱신*
