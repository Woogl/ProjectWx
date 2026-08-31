# WxGame — 기본 게임 모듈 (도메인 조립)

> 도메인 플러그인(전투·인벤토리·UI·AI·대화·퀘스트·월드)을 실제 게임으로 엮는 기본 게임 모듈. Lyra 이식 Experience 시스템으로 "이 판에 무엇을 켜고 어떤 컴포넌트를 누구에게 붙일지"를 데이터 주도로 확정하고, 프레임워크·캐릭터·컨트롤러 등 조립 지점의 구체 클래스를 제공한다.

## 책임

**담당**
- Experience 시스템: 게임 모드별 구성 정의(GameFeature 활성 + 액션 실행 + 기본 폰/인벤토리)를 확정·복제·로드하는 파이프라인.
- GameMode/GameState/WorldSettings/PlayerController/PlayerState 등 프레임워크 구체 클래스 — ModularGameplay receiver로서 Experience가 요청한 컴포넌트의 부착 지점.
- 캐릭터 계층(`AWxCharacterBase` → Player/Enemy/Boss, NPC)과 여기에 붙는 도메인 컴포넌트(ASC·장비·투사체·락온 등)의 조립·초기화.
- MVVM 리졸버/뷰모델: 도메인 데이터(인벤토리·퀘스트·대화·보스 등)를 [[WxUI]] 뷰모델로 이어주는 양쪽 의존 브릿지.
- 플레이어 입력 구성(`UWxInputConfig`)과 게임플레이 입력 처리, 상호작용·아이템 사용 어빌리티/애님노티파이.

**경계 (비담당)**
- 전투 규칙·어트리뷰트·장비 로직은 [[WxCombat]], 인벤토리 저장·아이템 정의는 [[WxInventory]], 위젯·뷰모델 베이스는 [[WxUI]], AI 판단은 [[WxAI]], 대화·퀘스트 진행은 [[WxDialogue]]·[[WxQuest]], 상호작용/스폰 인터페이스는 [[WxWorld]]에 위임. 공용 정의는 [[WxCore]].

## 핵심 타입 (진입점)

| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 진입 URL·WorldSettings로 Experience를 확정해 매니저에 넘기고, 로드 완료까지 폰 스폰·기본 인벤토리 지급을 지연 | `Framework/WxGameMode.h` |
| `AWxGameState` | Experience 매니저 컴포넌트의 거주처이자 ModularGameplay receiver | `Framework/WxGameState.h` |
| `UWxExperienceDefinition` | 한 판의 구성(폰 클래스·GameFeature·액션·액션셋)을 담는 프라이머리 데이터 에셋 | `Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | Experience 참조를 복제하고 서버·클라 각자 GameFeature 로드→액션 실행 파이프라인을 주행 | `Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 넷모드 무관하게 대상 액터에 프레임워크 컴포넌트를 주입하는 Experience 액션 | `Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | ASC를 직접 소유하는 공통 베이스 캐릭터, 도메인 컴포넌트 조립 지점 | `Character/WxCharacterBase.h` |
| `AWxPlayerController` | 게임플레이/입력 소유 분리 위에서 Experience가 요청한 컨트롤러 컴포넌트를 받는 receiver | `Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 폰의 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 MVVM 브릿지 | `MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약

- **판 구성은 코드가 아니라 Experience 에셋으로 한다.** `UWxExperienceDefinition`에 켤 GameFeature 이름·실행할 `UGameFeatureAction`·공유 `UWxExperienceActionSet`을 지정한다. GameMode의 상속 `DefaultPawnClass`는 읽지 않으며 폰 클래스도 Experience가 정한다.
- **컴포넌트 주입은 `AddComponents` 액션으로.** 대상 액터(Character/PlayerController/PlayerState/GameState)는 ModularGameplay receiver로 opt-in돼 있어야 하고, 컴포넌트 클래스가 상속한 프레임워크 베이스가 곧 부착 대상 선언이다.
- **캐릭터 확장**: `AWxCharacterBase`를 상속해 ASC·장비·투사체 등 도메인 컴포넌트를 생성자에서 조립한다. 무기·메타휴먼·HUD 위젯 등 구체 에셋은 BP 디폴트에서 지정한다.
- **UI 연결은 MVVM 리졸버로.** 도메인 데이터는 위젯 뷰모델 컨텍스트 리졸버(`WxViewModelResolver_*`)가 게임/도메인 타입과 [[WxUI]] 뷰모델 양쪽에 의존해 주입한다 — 도메인 뷰모델은 게임 모듈을 참조할 수 없기 때문.
- Experience가 지정한 `GameFeaturesToEnable` 이름 문자열이 `Plugins/GameFeatures/` 콘텐츠 플러그인을 켜는 유일한 경로다.

## 여기서부터 읽어라

1. `Framework/WxExperienceManagerComponent.h` — 이 모듈의 심장. 확정→복제→GameFeature 로드→액션 실행의 로드 상태 머신 전체가 여기 있다.
2. `Framework/WxGameMode.h` — Experience 확정 순서와 로드-지연 스폰/지급 흐름. 매니저와 짝으로 읽는다.
3. `Character/WxCharacterBase.h` — 캐릭터가 어떤 도메인 컴포넌트를 어떻게 소유·초기화하는지의 표준.

## 관련

- 조립 대상 도메인: [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxWorld]], 공용 foundation [[WxCore]].
- 상위: `Plugins/GameFeatures/`의 콘텐츠 플러그인은 Experience의 `GameFeaturesToEnable`로만 켜진다.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 68파일 — `/readme-writer`로 갱신*
