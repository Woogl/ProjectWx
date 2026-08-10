# WxGame — 게임 모듈

> 기본 게임 모듈(플러그인 아님). GameMode·GameState·PlayerController·Character 등 프레임워크 골격을 세우고, Experience(데이터 주도 게임플레이 구성)로 도메인 플러그인들을 한 판에 조립·활성화한다.

## 책임
**담당**
- 프레임워크 조립: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`와 플레이어·에너미·NPC 캐릭터 골격 정의
- Experience 파이프라인: `UWxExperienceDefinition` 확정 → 에셋 번들 로드 → GameFeature 플러그인 활성 → 액션 실행(컴포넌트 주입)까지의 로드 주행(서버·클라 각자)
- ModularGameplay receiver 지정: Pawn/Controller/PlayerState/GameState가 Experience 액션이 요청한 프레임워크 컴포넌트를 자동 부착받는 대상이 됨
- 플레이어 입력 소유: `AWxPlayerCharacter`의 이동/시선/어빌리티 입력, `UWxInputConfig` 주입
- 도메인 경계에 걸친 조립 타입 소유: `AWxNpc`(대화+외형), MVVM 리졸버/뷰모델(게임 상태 → WxUI 뷰모델 데이터 주입)

**경계 (비담당)**
- 전투·GAS 로직 → [[WxCombat]], 인벤토리 → [[WxInventory]], UI 위젯/매니저 → [[WxUI]], 대화 → [[WxDialogue]], 퀘스트 → [[WxQuest]], 세이브/복원 → [[WxSave]], AI → [[WxAI]], 월드 상호작용 → [[WxWorld]]
- 화면 표시는 중개하지 않음 — `UWxUIManagerSubsystem`(WxUI)이 컨트롤러 빙의·폰 상태 태그를 직접 추적
- 주입 컴포넌트 중개도 하지 않음 — 사용하는 쪽이 직접 조회하고 늦은 도착을 스스로 감당

## 의존성
- **주요 의존**: 조립 대상 Wx 플러그인 `WxCore`, `WxCombat`, `WxInventory`, `WxUI`, `WxWorld`, `WxAI`, `WxDialogue`, `WxQuest`, `WxSave` 전부. 엔진 서브시스템은 `GameplayAbilities`, `CommonUI`, `GameFeatures`, `ModularGameplay`, `ModelViewViewModel`(MVVM), `EnhancedInput`, `MotionWarping`, `GameplayTags`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정(URL→WorldSettings→기본)·비동기 로드 후 폰 스폰/시작 지급 게이트 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성 프라이머리 데이터 에셋(폰 클래스·GameFeature·액션) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState 거주. Experience 참조 복제 후 서버·클라 각자 로드 파이프라인 주행 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션(대상은 컴포넌트 베이스에서 도출) | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스. ASC 직접 소유, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 플레이어 캐릭터. 카메라·락온·입력(`UWxInputConfig`) 소유 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | 컨트롤러 컴포넌트 주입 receiver(인벤토리·상호작용·대화·PlayerSpawn) | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 Pawn→위젯 뷰모델 생성/주입(WxUI가 게임 모듈을 못 봐서 양쪽 아는 리졸버가 수행) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- Experience가 단일 진실원: 플레이어 폰 클래스는 `UWxExperienceDefinition::DefaultPawnClass`에서만 오고 GameMode의 `DefaultPawnClass`는 읽지 않음. 활성 GameFeature 플러그인·컴포넌트 주입도 Experience/ActionSet이 결정
- 로드 게이트: 로드가 비동기라 접속(PostLogin)보다 늦을 수 있어 폰 스폰·시작 지급을 로드 완료까지 미룸(`CallOrRegister_OnExperienceLoaded`)
- ModularGameplay opt-in: 컴포넌트를 받으려는 액터(Pawn/Controller/PlayerState/GameState)는 receiver로 등록돼 있어야 하고, 복제/사이드 제한은 컴포넌트 자신이 가드
- PIE 다중 세션: `UWxExperienceManager`(엔진 서브시스템)가 GameFeature 활성 요청을 URL별 카운팅해 조기 비활성 방지

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정·로드 게이트·폰 스폰·시작 지급의 전체 진입 흐름이 doc-comment에 요약됨
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 서버/클라 공통 로드 파이프라인의 상태 머신과 각 단계 콜백
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 ASC를 직접 소유하는 GAS 조립과 receiver 계약

## 관련
- 조립 대상 도메인 플러그인: [[WxCore]] · [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxSave]]
- 상위: `Plugins/GameFeatures/`의 GameFeature 플러그인이 Experience의 `GameFeaturesToEnable` 이름 문자열로 이 모듈의 로드 파이프라인에 얹힘

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 64파일 — `/readme-writer`로 갱신*
