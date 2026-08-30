# WxGame — 게임 조립 모듈

> 도메인 플러그인들(WxCombat·WxInventory·WxUI·WxAI·WxDialogue·WxQuest·WxSave·WxWorld)을 실제 게임으로 엮는 기본 게임 모듈. Experience 기반 게임플레이 구성, 프레임워크 액터(GameMode/State/Character/Controller), 그리고 도메인↔UI를 잇는 MVVM 브릿지를 소유한다.

## 책임
**담당**
- Experience 로드·적용 파이프라인 (Lyra 이식): 맵/URL이 고른 구성 에셋 → GameFeature 활성 → 액션 실행 → 폰 스폰·기본 지급
- 프레임워크 액터 구현체: GameMode/GameState/PlayerState/PlayerController와 Player·Enemy·Boss·Npc 캐릭터
- ModularGameplay 컴포넌트 receiver 노출 — 도메인 플러그인의 컴포넌트를 Experience 액션이 폰/컨트롤러/스테이트에 주입
- 게임플레이 입력(이동/시선/점프/어빌리티)과 카메라·락온 소유
- MVVM 리졸버/뷰모델로 도메인 데이터를 WxUI 위젯에 주입

**경계 (비담당)**
- 각 시스템 로직 자체는 도메인 플러그인이 소유 — 전투 [[WxCombat]], 인벤토리 [[WxInventory]], UI 위젯/뷰모델 베이스 [[WxUI]], AI [[WxAI]], 대화 [[WxDialogue]], 퀘스트 [[WxQuest]], 세이브 [[WxSave]], 월드 상호작용 [[WxWorld]]
- 공용 정의(팀 타입 등)는 [[WxCore]]
- 메뉴/UI 토글 입력은 CommonUI 액션(WxHUDLayout)이 처리

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 서버 전용. 이 판의 Experience를 URL→WorldSettings 순으로 확정해 GameState 매니저에 넘기고, 로드 완료까지 폰 스폰·기본 지급을 미룬다 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxExperienceDefinition` | 게임플레이 구성 프라이머리 DA — 폰 클래스·활성 GameFeature·액션·ActionSet | `Source/WxGame/Framework/WxExperienceDefinition.h` |
| `UWxExperienceManagerComponent` | 로드·적용의 주체. GameState 컴포넌트로 복제되어 서버/클라가 각자 같은 파이프라인 주행 | `Source/WxGame/Framework/WxExperienceManagerComponent.h` |
| `AWxGameState` | Experience 매니저의 거주처이자 ModularGameplay receiver | `Source/WxGame/Framework/WxGameState.h` |
| `AWxCharacterBase` | 플레이어/에너미 공통 베이스. ASC를 캐릭터에 직접 소유, ModularGameplay receiver | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력·카메라 붐·락온·소환 소유. 입력은 `UWxInputConfig` DA에서 주입 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxPlayerController` | Experience가 요청한 컨트롤러 컴포넌트(인벤토리·상호작용 스캐너·대화·스폰) 주입 지점 | `Source/WxGame/Controller/WxPlayerController.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 Pawn의 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 MVVM 브릿지 | `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- **새 게임플레이 구성**: `UWxExperienceDefinition` 네이티브 인스턴스 에셋을 만든다(BP 서브클래스 금지 — PrimaryAssetType이 달라져 스캔에서 빠짐). 여러 Experience가 공유하는 묶음은 `UWxExperienceActionSet`으로 뽑아 `ActionSets`에 합성.
- **폰/컨트롤러/스테이트에 시스템 부착**: `UWxGameFeatureAction_AddComponents` 등 Experience 액션으로 컴포넌트를 주입 — 프레임워크 액터는 receiver일 뿐 도메인 플러그인을 직접 알지 않는다.
- **맵 기본 구성**: `AWxWorldSettings::GameplayExperience`가 Experience 확정의 마지막 폴백. 진입 URL `?Experience=이름`이 우선.
- **도메인↔UI 연결**: WxUI 뷰모델은 게임 모듈을 참조할 수 없으므로, 양쪽에 의존하는 `MVVM/` 리졸버(`WxViewModelResolver_*`)와 뷰모델(`WxViewModel_*`)이 데이터 주입을 대행. 새 UI 데이터 소스는 여기에 브릿지를 추가.
- **입력**: 게임플레이 입력은 `UWxInputConfig` DA로 데이터 주도. 어빌리티 발동 IA는 어빌리티 CDO가 보유하고 AbilitySet 부여에서 바인딩이 파생 — InputConfig에 두지 않는다.
- **리플리케이션**: Experience 참조가 GameState 서브오브젝트로 복제(서버 직접 호출 / 클라 OnRep)되어 GameMode가 서버 전용이어도 클라 적용이 성립. 캐릭터 ASC는 PlayerState가 아닌 캐릭터 소유(리스폰 시 스탯 재초기화).

## 여기서부터 읽어라
1. `Source/WxGame/Framework/WxGameMode.h` — Experience 확정→스폰 지연이라는 이 모듈의 뼈대 흐름이 여기 서술돼 있다
2. `Source/WxGame/Framework/WxExperienceManagerComponent.h` — 로드 상태 머신(Loading→GameFeatures→Actions→Loaded)과 복제 모델
3. `Source/WxGame/Character/WxCharacterBase.h` — 캐릭터가 ASC·장비·팀을 어떻게 물고 receiver로 확장되는지
4. `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h` — 도메인/UI 참조 방향 제약을 리졸버가 어떻게 우회하는지의 대표 예

## 관련
- 조립 대상: [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxSave]] · [[WxWorld]]
- 공용 정의: [[WxCore]]
- 콘텐츠 계층: `Plugins/GameFeatures/`의 GameFeature 플러그인들이 Experience의 `GameFeaturesToEnable`로 켜진다

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 64파일 — `/readme-writer`로 갱신*
