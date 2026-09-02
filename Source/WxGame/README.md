# WxGame — 기본 게임 모듈 (플러그인 조립)

> 도메인 플러그인(WxCombat·WxInventory·WxUI 등)을 하나의 게임으로 조립하는 최상위 Runtime 모듈. Lyra 식 Experience/GameFeature 파이프라인으로 이 판의 게임플레이 구성을 데이터 주도로 확정하고, GameMode·GameState·컨트롤러·캐릭터 같은 프레임워크 진입점을 소유한다.

## 책임
**담당**
- Experience 확정·로드·적용 파이프라인 (GameMode가 고르고 GameState 매니저가 복제·주행)
- 프레임워크 최상위 조립: GameMode / GameState / PlayerController / PlayerState / 캐릭터 베이스
- GameFeature 플러그인 활성화 및 ModularGameplay 컴포넌트 주입 액션
- 플레이어 폰·입력·카메라, MVVM ViewModel 결선, 치트

**경계 (비담당)**
- 전투 로직 — [[WxCombat]]
- 인벤토리·아이템 정의 — [[WxInventory]]
- UI 위젯·HUD 레이아웃 — [[WxUI]]
- AI 행동·컨트롤러 두뇌 — [[WxAI]]
- 대화·퀘스트 도메인 — [[WxDialogue]] · [[WxQuest]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 이 판의 Experience 확정, 로드 완료까지 폰 스폰·시작 지급 지연 (서버 전용) | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 게임플레이 구성 정의 데이터 에셋 (폰 클래스·GameFeature·액션) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | Experience 로드·적용의 주체, 상태기계 (GameState 컴포넌트, 참조 복제) | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `AWxGameState` | ModularGameplay receiver이자 Experience 매니저의 거주처 | `Source/WxGame/Framework/WxGameState.h` |
| `AWxWorldSettings` | 맵이 자기 기본 Experience를 지정하는 자리 | `Source/WxGame/Framework/WxWorldSettings.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션 (스톡 대체) | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxPlayerController` | Experience가 요청 등록한 컨트롤러 컴포넌트의 주입 대상 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxCharacterBase` | ASC 소유 공통 베이스 캐릭터 (폰 대상 주입 receiver) | `Source/WxGame/Character/WxCharacterBase.h` |
| `EWxTeam` | 캐릭터 피아 구분 팀 열거형 (엔진 `FGenericTeamId`로 환산) | `Source/WxGame/Character/WxTeamTypes.h` |

## 확장 포인트 / 규약
- **Experience 확정 순서**: 진입 URL 옵션 `?Experience=이름` → `AWxWorldSettings.GameplayExperience` 순. 둘 다 비면 폴백 없이 미확정(무효 ID)으로 두고 매니저가 에러로 드러낸다. 상속받은 `DefaultPawnClass`는 읽지 않는다 — 폰 클래스의 유일한 출처는 Experience다(비우면 스펙테이터 폰으로 빙의하는 프론트엔드 Experience).
- **복제 모델**: GameMode는 서버 전용, Experience 참조만 GameState 서브오브젝트로 복제된다. 서버는 직접 호출, 클라는 `OnRep`으로 각자 같은 로드 파이프라인(에셋 번들 → GameFeature 활성 → 액션 실행)을 주행한다.
- **컴포넌트 주입**: 도메인 플러그인의 컴포넌트는 `UWxGameFeatureAction_AddComponents`를 통해 ModularGameplay receiver(GameState/PlayerController/PlayerState/캐릭터)에 붙는다. 대상 액터는 컴포넌트 클래스가 상속한 프레임워크 베이스에서 도출된다.
- **데이터 주도 규약**: `UWxExperienceDefinition`·`UWxExperienceActionSet` 에셋은 반드시 네이티브 클래스 인스턴스로 만든다 — BP 서브클래스 인스턴스는 `PrimaryAssetType`이 달라져 스캔·URL 해석에서 빠진다.
- `ActionSet`으로 여러 Experience가 액션·GameFeature·시작 지급·HUD 클래스를 공유 합성한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정과 로드-대기 스폰의 전체 진입 흐름이 여기서 시작한다.
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 파이프라인 상태기계(`EWxExperienceLoadState`)의 실체. 서버·클라 공통 주행 경로.
3. `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` — 조립이 실제 컴포넌트로 액터에 꽂히는 지점.

## 관련
- 상위: 조립하는 도메인 플러그인들 — [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxCore]]
- 콘텐츠 GameFeature 플러그인은 Experience의 `GameFeaturesToEnable` 이름 문자열로만 켜진다.

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 70파일 — `/readme-writer`로 갱신*
