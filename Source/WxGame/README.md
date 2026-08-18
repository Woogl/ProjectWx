# WxGame — 기본 게임 모듈 (조립 · 프레임워크)

> 오픈월드 액션 RPG의 기본 게임 모듈. GameMode/GameState/Controller/Character 등 프레임워크 실체를 정의하고, Experience(Lyra 이식) 파이프라인으로 도메인 플러그인들을 한 판에 조립·주입한다.

## 책임
**담당**
- **프레임워크 실체**: `AWxGameMode`·`AWxGameState`·`AWxPlayerController`·`AWxPlayerState`·`AWxWorldSettings` — 엔진 프레임워크의 프로젝트 구현체이자 ModularGameplay 컴포넌트 receiver.
- **Experience 파이프라인**: 한 판의 게임플레이 구성을 데이터로 정의(`UWxExperienceDefinition`)하고, 서버가 고른 참조를 복제해 서버·클라 각자 로드·적용(`UWxExperienceManagerComponent`). GameFeature 플러그인 활성 + 컴포넌트 주입(`UWxGameFeatureAction_AddComponents`)의 실행 주체.
- **캐릭터 계층**: `AWxCharacterBase`(ASC 직접 소유) → 플레이어/에너미/보스, 그리고 대화 전용 액터 `AWxNpc`. 무기 자식 액터·메타휴먼 부착·이동 컴포넌트 등 폰 합성.
- **플레이어 입력**: 이동/시선/점프 등 직접 바인딩 입력(`AWxPlayerCharacter` + `UWxInputConfig`).
- **MVVM 브리지**: `WxUI` 뷰모델을 게임 모듈 데이터(빙의 폰·ASC·인벤토리 등)와 잇는 리졸버/뷰모델 — 양쪽에 의존하는 이 모듈만이 배선 가능.

**경계 (비담당)**
- 전투 규칙·ASC/AttributeSet 정의 → [[WxCombat]] (여기선 캐릭터에 부착만).
- 인벤토리 상태·아이템 → [[WxInventory]], 상호작용/월드 오브젝트 → [[WxWorld]], 대화 → [[WxDialogue]], 퀘스트 → [[WxQuest]], AI 행동/컨트롤 로직 → [[WxAI]], 세이브/로드 → [[WxSave]].
- 위젯·HUD·화면 전환·뷰모델 정의 → [[WxUI]] (`UWxUIManagerSubsystem`이 컨트롤러 상태를 직접 추적; 이 모듈은 화면을 중개하지 않는다).
- 공용 정의(팀 타입 등) → [[WxCore]].

## 의존성
- **주요 의존**: `WxCore`·`WxCombat`·`WxInventory`·`WxUI`·`WxWorld`·`WxAI`·`WxDialogue`·`WxQuest`·`WxSave` (도메인 플러그인 전부), `GameFeatures`·`ModularGameplay`·`GameplayAbilities`(GAS)·`ModelViewViewModel`(MVVM)·`EnhancedInput`·`MotionWarping`·`MetaHumanSDKRuntime`.
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 진입 URL→WorldSettings→기본값 순으로 Experience 확정, 로드 완료까지 폰 스폰·지급 지연 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceManagerComponent` | GameState에 거주, Experience 로드 파이프라인(번들 로드→GF 활성→액션 실행) 주행·브로드캐스트 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxExperienceDefinition` | 한 판의 게임플레이 구성 데이터 에셋(폰 클래스·GF 목록·액션) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxGameFeatureAction_AddComponents` | 컴포넌트 클래스가 상속한 프레임워크 베이스로 대상 도출, 사이드 플래그 없이 주입 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스, ASC/AttributeSet 직접 소유, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerController` | 컨트롤러 컴포넌트(인벤토리·상호작용·대화·스폰) 주입 허브, 입력/화면은 위임 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 폰의 ASC/표시 데이터를 WxUI 뷰모델에 주입(WBP Resolver로 선택) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 게임플레이 모드/구성**: `UWxExperienceDefinition` 에셋을 네이티브 클래스 인스턴스로 신설(BP 서브클래스 금지 — PrimaryAssetType 어긋남). 맵 기본값은 `AWxWorldSettings.DefaultGameplayExperience`, 진입 URL은 `?Experience=이름`.
- **폰/컨트롤러에 시스템 부착**: GameFeature 플러그인에서 `UWxGameFeatureAction_AddComponents` 엔트리로 컴포넌트를 등록. 대상 액터는 컴포넌트 베이스(Pawn/Controller/PlayerState/GameState 컴포넌트)로 도출되며, 대상은 ModularGameplay receiver여야 한다.
- **새 캐릭터**: `AWxCharacterBase`(또는 `AWxEnemyCharacter`) 상속 후 BP에서 무기 ChildActorClass·BehaviorTree·MetaHuman 에셋 지정.
- **리플리케이션/권한**: Experience는 GameState 서브오브젝트로 복제 — GameMode가 서버에만 있어도 클라 적용 성립. 서버는 직접 호출, 클라는 OnRep으로 동일 파이프라인 주행. 캐릭터 ASC는 PlayerState가 아닌 캐릭터 소유(리스폰 시 스탯 재초기화).

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — Experience 로드 파이프라인의 전체 흐름·상태기계. 모듈 조립 방식의 중심.
2. `Source/WxGame/Framework/WxGameMode.h` — 판 시작 시퀀스(Experience 확정 → 폰 스폰 지연 → 지급). 프레임워크 진입 순서를 doc-comment로 서술.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터에 GAS·장비·팀·모션워핑이 어떻게 붙는지.

## 관련
- 상위: 콘텐츠 조립은 `Plugins/GameFeatures/` 의 GameFeature 플러그인이 이 모듈의 Experience/AddComponents 위에서 켠다.
- 함께 보는 도메인: [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxSave]].

---
*문서 기준 커밋 `36dc0e1` · 생성일 2026-08-18 · 소스 66파일 — `/readme-writer`로 갱신*
