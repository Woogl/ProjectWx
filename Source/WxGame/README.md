# WxGame — 게임 조립 모듈

> 도메인 플러그인(Combat·Inventory·AI·Dialogue·Quest·World·UI)을 하나의 플레이 가능한 게임으로 조립하는 최상위 게임 모듈. Lyra식 Experience/GameFeature 파이프라인으로 "이 판이 무엇인지"를 데이터로 정의하고, 그에 맞는 Pawn·컴포넌트·시작 지급을 런타임에 붙인다.

## 책임
**담당**
- 프레임워크 조립: GameMode/GameState/WorldSettings/PlayerController/PlayerState의 프로젝트 구체 클래스.
- Experience 파이프라인: Experience 확정(URL→WorldSettings) → 매니저 복제 로드 → GameFeature 활성 → 액션 실행 → Pawn 스폰·시작 인벤토리 지급.
- 플레이어·에너미 공통 캐릭터 골격(ASC를 캐릭터에 직접 소유)과 팀/컨트롤러(플레이어·AI) 구체 클래스.
- 도메인을 잇는 게임 특화 접착제: 입력 구성, 아이템 사용/상호작용 어빌리티·AnimNotify, MVVM 리졸버·뷰모델(도메인 데이터 → WxUI 위젯).

**경계 (비담당)**
- 전투 규칙·어빌리티 실효과는 [[WxCombat]], 아이템·인벤토리 저장은 [[WxInventory]], 위젯 자체는 [[WxUI]], 지각·행동트리 실행은 [[WxAI]], 대화/퀘스트 진행은 [[WxDialogue]]·[[WxQuest]], 상호작용·스폰 대상은 [[WxWorld]]에 위임. 본 모듈은 이들을 조립·중개만 한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정, 로드 완료까지 Pawn 스폰·시작 지급을 미루는 서버 게이트 | `Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | "이 판의 구성"을 정의하는 프라이머리 DA — Pawn·GameFeature·액션의 출처 | `Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | Experience를 복제 로드·적용하는 주체(서버 직접·클라 OnRep) | `Framework/WxExperienceManagerComponent.h` |
| `AWxGameState` | ModularGameplay receiver이자 Experience 매니저의 거주처 | `Framework/WxGameState.h` |
| `UWxGameFeatureAction_AddComponents` | 넷모드 무관·사이드 플래그 없이 receiver 액터에 컴포넌트를 주입하는 액션 | `Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스, ASC 직접 소유·컴포넌트 receiver | `Character/WxCharacterBase.h` |
| `AWxPlayerController` | Experience가 요청한 컨트롤러 컴포넌트(인벤토리·상호작용·대화·HUD) 주입 지점 | `Controller/WxPlayerController.h` |
| `AWxWorldSettings` | 맵이 자기 기본 Experience를 지정하는 자리(확정의 마지막 단계) | `Framework/WxWorldSettings.h` |

## 확장 포인트 / 규약
- Experience 확정 순서: 진입 URL `?Experience=이름` → `AWxWorldSettings.GameplayExperience`. 그 뒤 폴백 없음 — 둘 다 비면 매니저가 에러로 드러낸다.
- Pawn 클래스의 유일한 출처는 `UWxExperienceDefinition.DefaultPawnClass` — GameMode의 상속 `DefaultPawnClass`는 읽지 않는다. 비우면 폰 없는 프론트엔드 Experience로 보고 엔진 스펙테이터 폰으로 빙의.
- 컴포넌트 주입은 데이터 주도: 대상 액터(Character/Controller/PlayerState/GameState)가 ModularGameplay receiver로 opt-in돼 있어야 하며, `UWxGameFeatureAction_AddComponents`가 컴포넌트 베이스 클래스로부터 대상을 도출해 요청만 건다.
- 액션 합성: `UWxExperienceDefinition.Actions` + `UWxExperienceActionSet`(공유 묶음)이 실행 순서대로 평탄화된다. 시작 인벤토리·HUD 클래스는 ActionSet이 실어 나른다.
- Experience/ActionSet 에셋은 네이티브 클래스 인스턴스로만 생성 — BP 서브클래스는 PrimaryAssetType이 달라 스캔에서 빠진다.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — Experience 확정부터 지연 스폰·지급까지 서버 조립 흐름의 뼈대
2. `Framework/WxExperienceManagerComponent.h` — 로드 상태 머신(번들→GameFeature→액션)의 전모
3. `Character/WxCharacterBase.h` — 캐릭터 골격과 ASC·팀·컴포넌트 receiver 규약
4. `MVVM/WxViewModelResolver_PlayerCharacter.h` — 도메인 데이터를 WxUI 위젯으로 잇는 리졸버 패턴의 대표 예

## 관련
- 상위: 실제 콘텐츠 구성은 `Plugins/GameFeatures/`의 GameFeature 플러그인이 Experience 에셋으로 켠다. 조립 대상 도메인은 [[WxCombat]]·[[WxInventory]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]]·[[WxWorld]]·[[WxUI]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 71파일 — `/readme-writer`로 갱신*
