# WxGame — 게임 조립 모듈

> 도메인 플러그인(전투·인벤토리·AI·대화 등)을 하나의 플레이 가능한 게임으로 조립하는 기본 게임 모듈. GameMode/Experience로 이 판의 구성을 확정하고, 캐릭터·컨트롤러 등 프레임워크 액터의 구체 클래스를 제공한다.

## 책임
**담당**
- 게임 프레임워크 구체 클래스: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`, 캐릭터 계층(`AWxCharacterBase` 및 파생)
- Experience 기반 게임플레이 조립: 데이터 에셋 정의 → 에셋 번들 로드 → GameFeature 활성 → 액션 실행의 로드 파이프라인
- ModularGameplay receiver 배선 + 컴포넌트 주입 액션(`UWxGameFeatureAction_AddComponents`)
- 플레이어 입력(이동/시선/어빌리티) 소유와 캐릭터 이동 튜닝(`UWxCharacterMovementComponent`)
- 도메인 플러그인 데이터를 위젯에 잇는 MVVM 뷰모델/리졸버(게임 모듈만 양쪽을 참조 가능)

**경계 (비담당)**
- 전투·어트리뷰트 계산은 [[WxCombat]], 인벤토리·보상은 [[WxInventory]], AI 지각/BT는 [[WxAI]], 대화는 [[WxDialogue]]에 위임
- HUD·사망 화면·대화 창 표시는 [[WxUI]]의 UIManager가 컨트롤러 빙의를 직접 따라가며 처리 — 컨트롤러가 화면을 중개하지 않음
- savable 액터 상태 복원은 [[WxSave]]의 월드 서브시스템이 자동 처리

## 의존성
- **주요 의존**: `WxCore` `WxCombat` `WxInventory` `WxUI` `WxWorld` `WxAI` `WxDialogue` `WxQuest` `WxSave` (모든 도메인 플러그인) + GameFeatures / ModularGameplay / GameplayAbilities / ModelViewViewModel(MVVM) / MotionWarping / EnhancedInput / CommonUI / MetaHumanSDKRuntime
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 이 판의 Experience 확정 + 로드 완료까지 폰 스폰·시작 지급을 게이트 | `Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 폰 클래스·GameFeature·액션을 담는 프라이머리 데이터 에셋(조립의 단일 출처) | `Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState에 얹혀 서버·클라 각자 로드 파이프라인을 주행하는 주체 | `Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션(대상은 컴포넌트 베이스에서 도출) | `Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스, ASC 직접 소유, ModularGameplay receiver | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 플레이어 폰, 입력 소유(`UWxInputConfig` 주입)·카메라·락온 | `Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 폰, BT 지정·처형 상호작용·처치 보상 | `Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | ModularGameplay receiver, 주입 컴포넌트(인벤토리·상호작용·대화·스폰) 거주처 | `Controller/WxPlayerController.h` |

## 확장 포인트 / 규약
- **조립은 Experience 중심**: GameMode의 `DefaultPawnClass`는 읽히지 않는다 — 폰 클래스의 유일한 출처는 Experience. 진입 URL(`?Experience=`) → `AWxWorldSettings` → GameMode 폴백 순으로 확정
- **GameFeature 활성**은 Experience의 `GameFeaturesToEnable` 이름 문자열로만 참조(역방향 코드 참조 금지). PIE 다중 세션은 `UWxExperienceManager` 엔진 서브시스템이 URL별 카운팅으로 조기 비활성 방지
- **컴포넌트 주입 모델**: 액터는 receiver로 opt-in(`AWxPlayerController`/`AWxPlayerState`/`AWxGameState`/`AWxCharacterBase`), 주입 컴포넌트의 사이드 제한은 컴포넌트 자신이 가드(HasAuthority / IsLocalController). 액션은 넷모드 무관하게 요청만 한다
- **리플리케이션 권위 모델**: GameMode는 서버 전용, Experience 참조는 GameState 서브오브젝트로 복제 → 클라는 OnRep으로 동일 파이프라인 주행. 로드는 비동기라 접속보다 늦을 수 있어 GameMode가 폰 스폰을 로드 완료까지 미룸
- **ASC는 캐릭터 직접 소유**(PlayerState 아님) — 리스폰 시 스탯을 새로 초기화. 클라는 `OnRep_PlayerState`에서 `InitAbilitySystem`
- **MVVM 브리지**: 도메인 뷰모델(WxUI 소속)이 게임 모듈을 참조할 수 없으므로, 양쪽에 의존하는 리졸버(`UWxViewModelResolver_PlayerCharacter`)·뷰모델이 여기서 데이터를 주입

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 조립의 진입점. Experience 확정·폰 스폰 게이트의 전모가 클래스 주석에 있다
2. `Framework/WxExperienceManagerComponent.h` — 로드 파이프라인(번들→GameFeature→액션→브로드캐스트)의 상태 기계
3. `Character/WxCharacterBase.h` — 캐릭터 계층의 뿌리. ASC 소유·팀·사망/래그돌·장비·메타휴먼 조립의 배선
4. `Framework/WxGameFeatureAction_AddComponents.h` — 컴포넌트 주입이 어떻게 대상·사이드를 도출하는지

## 관련
- 상위: 이 모듈이 조립·사용하는 도메인 플러그인 — [[WxCore]] [[WxCombat]] [[WxInventory]] [[WxUI]] [[WxWorld]] [[WxAI]] [[WxDialogue]] [[WxQuest]] [[WxSave]]
- GameFeature(콘텐츠) 플러그인은 Experience의 이름 문자열로만 켜진다

---
*문서 기준 커밋 `de46ee7` · 생성일 2026-08-11 · 소스 64파일 — `/readme-writer`로 갱신*
