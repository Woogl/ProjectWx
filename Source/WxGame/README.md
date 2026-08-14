# WxGame — 게임 조립 모듈

> 여러 Wx 플러그인을 하나의 플레이 가능한 게임으로 조립하는 최종 게임 모듈. Character 계층, Experience/GameFeature 프레임워크, GAS 응용 어빌리티, 입력·치트·MVVM 배선이 여기 모인다.

## 책임
**담당**
- 캐릭터 계층(`AWxCharacterBase` → Player/Enemy/Boss)과 대화 NPC, 공용 이동 컴포넌트, 메타휴먼 외형 조립
- Experience/GameFeature 프레임워크: 판별 게임플레이 구성을 정의(Definition/ActionSet)하고, GameMode가 확정해 GameState 매니저가 서버·클라 각자 로드·적용
- 프레임워크 액터 구체 클래스: `AWxGameMode`·`AWxGameState`·`AWxPlayerController`·`AWxEnemyController`·`AWxPlayerState`·`AWxWorldSettings`
- GAS 응용 어빌리티(상호작용·아이템 사용)와 그 AnimNotify, 개발용 치트, 게임플레이 입력 구성(`UWxInputConfig`)
- WxUI 뷰모델과 게임 데이터를 잇는 MVVM 리졸버/관찰 뷰모델

**경계 (비담당)**
- 전투 계산·어트리뷰트·락온: [[WxCombat]] (여기선 ASC/AttributeSet를 캐릭터에 얹기만 함)
- 인벤토리·보상·아이템 정의: [[WxInventory]] / 상호작용 스캐너·월드 오브젝트: [[WxWorld]]
- AI 인지·BT 태스크·정찰: [[WxAI]] / 대화 세션·계약: [[WxDialogue]] / 퀘스트: [[WxQuest]]
- 위젯·HUD·UIManager·뷰모델 본체: [[WxUI]] / 저장·복원: [[WxSave]] / 어빌리티 베이스·공용 정의: [[WxCore]]

## 의존성
- **주요 의존**: `WxCore`·`WxCombat`·`WxInventory`·`WxUI`·`WxWorld`·`WxAI`·`WxDialogue`·`WxQuest`·`WxSave` (모든 도메인 플러그인) + 엔진 `GameFeatures`·`ModularGameplay`·`GameplayAbilities`·`ModelViewViewModel`·`MotionWarping`·`EnhancedInput`·`MetaHumanSDKRuntime`
- 규칙: 기본 게임 모듈로 여러 플러그인을 조립하는 정상 역할(「WxCore 외 참조 금지」 규칙 무관).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 캐릭터 계층 루트. ASC를 폰에 직접 소유(PlayerState 불필요), ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 베이스 + 카메라·락온·입력. 입력은 `UWxInputConfig`에서 주입 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 베이스 + `IWxSpawnable`·`IWxInteractable`(처형). `AWxBossCharacter`가 상속 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxGameMode` | 판의 Experience 확정 → GameState 매니저에 위임, 로드 완료까지 폰 스폰 게이팅 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 게임플레이 구성 정의 프라이머리 데이터 에셋(폰 클래스·GameFeature·액션) | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | GameState 상주. Experience 로드 파이프라인 주체(번들→GF→액션→브로드캐스트) | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션. 대상은 컴포넌트 베이스에서 도출 | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 폰의 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 MVVM 리졸버 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 캐릭터**: `AWxCharacterBase`(Abstract)를 상속하고 BP 디폴트에서 무기 ChildActor·메타휴먼 에셋·`UIData`·`Team`을 지정. 공용 CMC(`UWxCharacterMovementComponent`)와 ASC는 베이스가 배선한다. 적은 `AWxEnemyCharacter`, 보스는 `AWxBossCharacter`를 상속.
- **새 Experience**: `UWxExperienceDefinition` 네이티브 인스턴스 에셋을 만들어 `DefaultPawnClass`·`GameFeaturesToEnable`·`Actions`·`ActionSets`를 채운다. 여러 Experience가 공유할 액션·시작 아이템은 `UWxExperienceActionSet`으로 묶는다. 맵 기본값은 `AWxWorldSettings`, 모드 폴백은 GameMode의 `DefaultExperience`, 진입 URL `?Experience=`가 최우선.
- **새 GameFeatureAction**: 엔진 `UGameFeatureAction`을 상속. 폰·컨트롤러 등에 컴포넌트를 붙일 땐 스톡 대신 `UWxGameFeatureAction_AddComponents`를 쓰고, 대상 액터는 ModularGameplay receiver로 opt-in돼 있어야 한다(`AWxCharacterBase`·PlayerController·PlayerState·GameState 모두 receiver).
- **데이터 주도 구동**: GameMode(서버 전용)가 Experience를 확정 → GameState의 매니저 컴포넌트가 참조를 복제 → 서버는 직접·클라는 `OnRep`으로 같은 로드 파이프라인 주행(에셋 번들 비동기 로드 → GameFeature 플러그인 활성 → 자기 월드 한정 컨텍스트로 액션 실행 → 로드 완료 브로드캐스트). 로드는 비동기라 접속보다 늦을 수 있어 GameMode가 완료까지 폰 스폰·시작 지급을 미룬다.
- **리플리케이션/권한**: 상호작용 어빌리티는 ServerOnly(감지는 클라 스캐너, 검증·실행은 서버), 아이템 사용은 LocalPredicted(차감 단계만 서버 게이팅). 복제 컴포넌트는 GameFeature 매니저가 authority 액터에서만 생성한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정·폰 스폰 게이팅. 게임 부팅 흐름의 시작점
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 파이프라인 상태기계 전체가 여기 담겨 데이터 주도 조립의 핵심
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터 계층·ASC 소유 모델·ModularGameplay 주입의 기반

## 관련
- 상위: 이 모듈을 켜는 것은 `Plugins/GameFeatures/`의 GameFeature 플러그인(콘텐츠 분류) — Experience의 `GameFeaturesToEnable` 이름 문자열로 활성화된다. 함께 볼 도메인: [[WxCombat]]·[[WxUI]]·[[WxWorld]]·[[WxSave]]

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 64파일 — `/readme-writer`로 갱신*
