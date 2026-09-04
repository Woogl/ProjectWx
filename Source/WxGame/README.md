# WxGame — 게임 조립 모듈

> 도메인 플러그인(WxCombat·WxInventory·WxUI·WxAI·WxDialogue·WxQuest·WxWorld)을 하나의 플레이 가능한 게임으로 조립하는 최상위 게임 모듈. Lyra 식 Experience 파이프라인으로 무엇을 켤지 데이터 주도로 결정하고, 구체 캐릭터·컨트롤러·프레임워크 클래스를 여기서 확정한다.

## 책임
**담당**
- Experience 파이프라인: GameMode 가 이 판의 Experience 를 확정 → GameState 의 매니저 컴포넌트가 참조를 복제해 서버·클라 각자 로드(GameFeature 활성 + 액션 실행).
- 프레임워크 구체 클래스: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`/`AWxWorldSettings`.
- 구체 캐릭터 계층: 도메인 컴포넌트(ASC·전투·장비·메타휴먼)를 실제로 부착·초기화하는 `AWxCharacterBase`와 파생(`AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxNpc`).
- 도메인 ↔ WxUI 브릿지: 양쪽에 의존하는 MVVM ViewModel·Resolver (WxUI 는 게임 모듈을 참조할 수 없으므로 데이터 주입을 여기서 수행).
- 입력 바인딩(`UWxInputConfig` 주도), 치트, 게임 전용 어빌리티/애님노티파이.

**경계 (비담당)**
- 전투 규칙·GAS 어트리뷰트/이펙트는 [[WxCombat]], 아이템·인벤토리 모델은 [[WxInventory]], 위젯·뷰모델 정의는 [[WxUI]], AI 로직은 [[WxAI]], 대화는 [[WxDialogue]], 퀘스트는 [[WxQuest]], 월드 상호작용은 [[WxWorld]]에 위임. WxGame 은 이들을 부착·연결할 뿐 규칙을 재정의하지 않는다.
- 콘텐츠 단위 기능(예: WxFishing)은 `Plugins/GameFeatures/`의 GameFeature 플러그인이 담당하며, Experience 가 이름으로 켠다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 서버 전용. Experience 확정 + 폰 스폰/시작 지급을 로드 완료까지 게이트 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 이 판의 구성(폰 클래스·GameFeature·액션·액션셋)을 담는 프라이머리 데이터 에셋 | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | Experience 로드·적용의 주체. GameState 에 상주, 참조 복제 + OnRep 로드 파이프라인 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | Experience/GameFeature 가 대상 액터에 컴포넌트를 주입하는 액션 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스. ASC 직접 소유, 도메인 컴포넌트 부착 지점 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 입력(이동/시선/어빌리티)·카메라 소유 플레이어 폰 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | ModularGameplay receiver — Experience 가 요청한 컨트롤러 컴포넌트 주입처 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 Pawn 의 데이터를 WxUI 뷰모델에 주입하는 MVVM 리졸버(브릿지 대표) | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- 새 플레이 구성: `UWxExperienceDefinition` 에셋을 만들고 `GameFeaturesToEnable`(이름 문자열)·`Actions`·`ActionSets`·`DefaultPawnClass` 를 채운다. 진입 URL `?Experience=이름` → `AWxWorldSettings.GameplayExperience` 순으로 확정된다(둘 다 비면 미확정 = 에러).
- 새 컴포넌트 주입: 대상 액터를 ModularGameplay receiver 로 만들고, `UWxGameFeatureAction_AddComponents` 의 `ComponentList` 에 컴포넌트 클래스를 추가한다. 대상 액터는 컴포넌트가 상속한 프레임워크 베이스에서 도출되므로 별도 지정 불필요.
- 새 캐릭터: `AWxCharacterBase` 를 상속(또는 파생 BP). ASC·전투 컴포넌트는 베이스가 붙이며, 서버는 `PossessedBy` → `InitAbilitySystem`, 클라는 파생의 OnRep 경로로 초기화한다.
- 새 UI 데이터 연결: WxUI 는 게임 모듈을 참조하지 못하므로, 양쪽에 의존하는 `MVVM/` 리졸버가 게임 측 액터를 읽어 뷰모델을 채운다.
- 권한 모델: Experience 는 GameMode(서버)가 고르고 매니저가 복제, 서버는 직접 호출·클라는 OnRep 로 같은 파이프라인을 주행. 폰 스폰은 로드 완료까지 지연된다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.cpp` — 한 판이 시작되는 지점. Experience 확정 → 로드 대기 → 폰 스폰/시작 지급의 전체 흐름.
2. `Source/WxGame/Framework/WxExperienceManagerComponent.cpp` — Experience 가 실제로 어떻게 로드·적용되는지(번들 로드 → GameFeature 활성 → 액션 실행).
3. `Source/WxGame/Character/WxCharacterBase.cpp` — 도메인 플러그인 컴포넌트들이 캐릭터에 어떻게 조립·초기화되는지.

## 관련
- 상위: 게임 실행 루트(`Wx.uproject`)와 `Plugins/GameFeatures/`의 GameFeature 플러그인이 이 모듈의 Experience/프레임워크 위에 얹힌다.
- 함께 보기: 조립 대상인 [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxWorld]] 와 공용 정의 [[WxCore]].

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 69파일 — `/readme-writer`로 갱신*
