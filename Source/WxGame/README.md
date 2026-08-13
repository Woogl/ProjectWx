# WxGame — 게임 조립 모듈

> 도메인 플러그인(전투·인벤토리·AI·대화·퀘스트·세이브·UI·월드)을 실제 게임으로 조립하는 기본 게임 소스 모듈. Experience(Lyra 이식) 기반 데이터 주도 구성으로 GameMode·캐릭터·컨트롤러·입력을 엮는다.

## 책임
**담당**
- Experience 로드 파이프라인: GameMode 가 Experience 확정 → GameState 의 매니저 컴포넌트가 복제·로드(에셋 번들 → GameFeature 활성 → 액션 실행) → 폰 스폰 게이트
- 프레임워크 골격: `AWxGameMode` / `AWxGameState` / `AWxPlayerState` / `AWxPlayerController` / `AWxWorldSettings` (전부 ModularGameplay receiver)
- 캐릭터 계층: 플레이어·에너미·보스·NPC 구체 클래스와 공통 베이스(ASC 직접 소유, 외형·장비·워핑 컴포넌트 합성)
- 플레이어 입력 바인딩(Enhanced Input)과 개발용 콘솔 치트
- WxUI ↔ 게임 데이터 브리지: MVVM 뷰모델·리졸버(WxUI 는 게임 모듈을 참조 못 하므로 양쪽에 의존하는 이 계층이 데이터 주입)

**경계 (비담당)**
- 전투/장비/락온 로직 → [[WxCombat]], 인벤토리·보상 → [[WxInventory]], AI 인지·BT 태스크 → [[WxAI]], 대화 세션 → [[WxDialogue]], 상호작용 스캔·세이브 → [[WxWorld]]·[[WxSave]], 위젯 클래스·UI 매니저 → [[WxUI]]
- 컴포넌트/화면의 실제 조립은 각 플러그인 컴포넌트가 자기 스폰·빙의 이벤트를 직접 따라간다 — 컨트롤러는 중개하지 않는다

## 의존성
- **주요 의존**: `WxCore` `WxCombat` `WxInventory` `WxUI` `WxWorld` `WxAI` `WxDialogue` `WxQuest` `WxSave` (모든 도메인 플러그인) + `GameplayAbilities` · `GameFeatures`/`ModularGameplay` · `ModelViewViewModel`(MVVM) · `EnhancedInput` · `MotionWarping` · `MetaHumanSDKRuntime`/`HairStrandsCore`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 이 판의 Experience 확정, 로드 완료까지 폰 스폰·시작 지급 지연 | `Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 게임플레이 구성(폰 클래스·GameFeature·액션) 프라이머리 데이터 에셋 | `Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | 서버·클라 각자 로드 파이프라인 주행, 완료 브로드캐스트 | `Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션(receiver 에 자동 부착) | `Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스, ASC 직접 소유, 컴포넌트 합성 | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력 소유(이동/시선/어빌리티), 카메라·락온 | `Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | BT 구동 적, 처형 상호작용·처치 보상 | `Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | ModularGameplay receiver(주입 컴포넌트 거주처) | `Controller/WxPlayerController.h` |

## 확장 포인트 / 규약
- **새 게임 구성**: `UWxExperienceDefinition` 에셋을 만들어 `DefaultPawnClass`·`GameFeaturesToEnable`·`Actions`/`ActionSets` 지정. 맵은 `AWxWorldSettings.DefaultGameplayExperience`, 진입은 `?Experience=이름` URL 로 선택(URL → WorldSettings → GameMode 폴백 순).
- **새 컴포넌트 주입**: Experience 의 `UWxGameFeatureAction_AddComponents.ComponentList` 에 등록. 대상 액터는 컴포넌트가 상속한 ModularGameplay 베이스(Pawn/Controller/PlayerState/GameState)로 도출되고, 해당 액터는 receiver 로 opt-in 돼 있어야 한다.
- **새 캐릭터**: `AWxCharacterBase`(또는 `AWxPlayerCharacter`/`AWxEnemyCharacter`) 상속 후 BP 서브클래스에서 무기 ChildActor·메타휴먼 에셋·`InputConfig`(플레이어)·`BehaviorTreeAsset`(적) 지정. 어빌리티 입력은 InputConfig 가 아니라 AbilitySet 부여 대상에서 파생.
- **새 치트**: `UWxCheatManager` 에 `UFUNCTION(Exec)` 추가. Standalone·에디터(권위 측)에만 존재하므로 권위 가드 불필요.
- **새 UI 데이터 브리지**: WxUI 뷰모델에 게임 데이터를 주입하려면 `MVVM/` 에 뷰모델·리졸버(`UMVVMViewModelContextResolver`)를 두고 WBP 의 View Bindings 에서 Resolver 로 선택.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 판이 시작될 때 Experience 확정·폰 스폰 지연의 전체 흐름 주석이 있다
2. `Framework/WxExperienceManagerComponent.h` — 로드 파이프라인(번들 → GameFeature → 액션) 상태 머신
3. `Character/WxCharacterBase.h` — 캐릭터가 어떤 컴포넌트로 조립되고 ASC·팀·사망을 어떻게 다루는지

## 관련
- 상위: 게임의 최상단 조립 모듈. 콘텐츠 분류는 `Plugins/GameFeatures/` 의 GameFeature 플러그인이 이 모듈과 도메인 플러그인 위에 얹힌다. 도메인별 세부는 [[WxCombat]] [[WxInventory]] [[WxAI]] [[WxDialogue]] [[WxQuest]] [[WxSave]] [[WxUI]] [[WxWorld]] [[WxCore]] 참조.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 64파일 — `/readme-writer`로 갱신*
