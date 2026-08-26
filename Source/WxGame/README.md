# WxGame — 기본 게임 모듈 (조립 지점)

> GameMode·GameState·Controller·Character·PlayerState 등 프레임워크 뼈대를 정의하고, Experience/GameFeature 데이터로 각 도메인 플러그인(Combat·Inventory·AI·Dialogue·Quest·UI·World·Save)을 한 판에 엮어 부팅하는 게임 모듈이다.

## 책임
**담당**
- 프레임워크 조립: `AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxPlayerState`/`AWxCharacterBase` 계층과 ModularGameplay receiver 지정.
- Experience 부팅: 이 판의 Experience 확정 → 복제 → 서버·클라 각자 로드(GameFeature 활성 + 액션 실행) → 폰 스폰·기본 인벤토리 지급 게이팅.
- 플레이어 본체: 카메라·입력(EnhancedInput)·이동(비대칭 중력)·MetaHuman 외형 조립, 소비 아이템/상호작용 어빌리티.
- MVVM 브리지: 게임 데이터를 WxUI 뷰모델로 주입하는 Resolver/게임 모듈 소속 ViewModel.

**경계 (비담당)**
- 전투 계산·AttributeSet·AbilitySet은 [[WxCombat]], 인벤토리 로직은 [[WxInventory]], 뷰모델 베이스·위젯은 [[WxUI]].
- AI 인지·정찰·BT 태스크는 [[WxAI]], 대화 런타임은 [[WxDialogue]], 퀘스트는 [[WxQuest]], 세이브/월드 상태 복원은 [[WxSave]], 상호작용 대상·스폰은 [[WxWorld]].
- 콘텐츠 분류·GameFeature 플러그인은 `Plugins/GameFeatures/` 아래에 둔다(이 모듈은 이름 문자열로만 켠다).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | Experience 확정·로드 완료까지 폰 스폰 지연·기본 인벤토리 지급 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceManagerComponent` | Experience 복제·로드 파이프라인(GameFeature 활성 → 액션 실행) 주체 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `UWxExperienceDefinition` | 한 판의 폰 클래스·GameFeature·액션을 정의하는 프라이머리 데이터 에셋 | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxGameFeatureAction_AddComponents` | 사이드 플래그 없는 컴포넌트 주입 액션(Controller·Pawn·PlayerState에 부착) | `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h` |
| `AWxCharacterBase` | ASC 직접 소유·팀·태그 공용 베이스(Player/Enemy 파생) | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 카메라·입력·록온·스태미나 위젯을 얹은 플레이어 폰 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | 컨트롤러 컴포넌트(인벤토리·스캐너·대화·PlayerSpawn) receiver | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxWorldSettings` | 맵별 기본 Experience 지정(확정의 마지막 폴백) | `Source/WxGame/Framework/WxWorldSettings.h` |

## 확장 포인트 / 규약
- 새 판 구성은 `UWxExperienceDefinition` 에셋을 네이티브 인스턴스로 만들어 `DefaultPawnClass`·`GameFeaturesToEnable`·`Actions`/`ActionSets`를 채운다. `?Experience=이름` URL 옵션 → `AWxWorldSettings::GetDefaultGameplayExperience` 순으로 확정되며 폴백은 없다.
- 컴포넌트를 폰/컨트롤러/PlayerState에 붙이려면 `UWxGameFeatureAction_AddComponents` 엔트리에 컴포넌트 클래스만 지정한다(대상은 컴포넌트가 상속한 프레임워크 베이스에서 도출, 사이드 제한은 컴포넌트가 스스로).
- 공유 액션·기본 지급 아이템·게임 HUD는 `UWxExperienceActionSet`으로 묶어 여러 Experience가 합성한다.
- 플레이어 입력은 `UWxInputConfig` DA로 주입, MetaHuman 외형은 `UWxMetaHumanComponent`에 에셋만 지정.
- WxUI 뷰모델에 게임 데이터를 주입할 땐 `UWxViewModelResolver_*`(양쪽 의존)에서 처리한다 — 뷰모델 자체는 게임 모듈을 참조하지 못한다.

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — 부팅 순서(Experience 확정 → 로드 대기 → 스폰·지급)의 전모가 클래스 주석에 정리돼 있다.
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 상태 머신과 서버/클라 각자 주행 흐름. Experience가 어떻게 GameFeature·액션으로 풀리는지의 심장.
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터 계층·ASC 소유·팀/태그 계약. Player/Enemy/Boss/Npc 파생의 뿌리.

## 관련
- 상위: 이 모듈이 엮는 도메인 플러그인 전반 [[WxCombat]] [[WxInventory]] [[WxAI]] [[WxDialogue]] [[WxQuest]] [[WxUI]] [[WxWorld]] [[WxSave]], 공용 정의 [[WxCore]]

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 66파일 — `/readme-writer`로 갱신*
