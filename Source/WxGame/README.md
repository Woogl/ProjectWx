# WxGame — 게임 조립 모듈

> 여러 Wx 도메인 플러그인을 실제 플레이 가능한 게임으로 엮는 최상위 런타임 모듈. 캐릭터·컨트롤러·게임 프레임워크·입력·프론트엔드 흐름과, 도메인 데이터를 UI에 잇는 MVVM 브리지를 소유한다.

## 책임
**담당**
- 구체 캐릭터/폰 계층: 공용 베이스(`AWxCharacterBase`, ASC를 캐릭터가 직접 소유)와 플레이어·적·NPC 파생.
- 게임 프레임워크: GameMode/GameState/PlayerState/PlayerController/AIController — 어느 도메인 컴포넌트를 어디에 붙일지 결정하는 조립 지점.
- 로컬 입력 바인딩: 이동·시선·점프·크라우치·어빌리티 입력을 폰에서 받아 `UWxInputConfig` DA로 주입 ([[WxUI]] CommonUI 메뉴 입력·어빌리티 발동 IA는 제외).
- 프론트엔드 → 게임 맵 전환 흐름(`UWxGameFlowSubsystem`)과 리스폰.
- 여러 플러그인에 걸친 합성물: 도메인 뷰모델을 위젯별로 만들어 [[WxUI]]와 도메인을 잇는 MVVM 리졸버/뷰모델, 소비 아이템 사용 파이프라인.

**경계 (비담당)**
- 전투 규칙·ASC/어트리뷰트·락온·히트스톱은 [[WxCombat]]; 여기선 컴포넌트를 캐릭터에 장착만 한다.
- 인벤토리 저장·아이템 정의는 [[WxInventory]]; 위젯·아이콘·CommonUI 레이아웃은 [[WxUI]].
- 지각·행동 트리·행동 컴포넌트는 [[WxAI]]; 상호작용 스캔·스포너·미니언·인터랙터블 계약은 [[WxWorld]].
- 대화 진행은 [[WxDialogue]], 퀘스트 상태는 [[WxQuest]]. 공용 정의는 [[WxCore]].

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 플레이어·적 공통 베이스. ASC/어트리뷰트/락온/히트스톱/무기 등 [[WxCombat]] 컴포넌트를 장착하는 지점 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 카메라·입력·소비 아이템 사용을 얹은 플레이어 폰 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | 적 폰. [[WxWorld]] Spawnable/Interactable/Minion 계약과 보스 표시 상태를 조립 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 인벤토리·상호작용 스캐너·대화 세션·레이아웃 등 플레이어 단위 컴포넌트의 거주처 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxAIController` | AI가 모는 모든 폰의 컨트롤러. BT가 고른 타겟을 락온으로 연결 | `Source/WxGame/Controller/WxAIController.h` |
| `AWxGameMode` | 맵/프론트엔드 선택에 따라 폰 클래스를 고르는 조립 진입 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택(폰·목적지)을 들고 맵을 열어 도착 GameMode가 소비 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `UWxViewModelResolver_*` | 빙의 폰/도메인 데이터를 [[WxUI]] 뷰모델에 위젯별로 주입하는 브리지 | `Source/WxGame/MVVM/` |

## 확장 포인트 / 규약
- 새 캐릭터는 `AWxCharacterBase`(또는 `AWxPlayerCharacter`/`AWxEnemyCharacter`)를 BP로 파생해 만든다. 클래스는 `Abstract`이며 무기/메타휴먼 등 서브오브젝트 에셋은 BP 디폴트에서 지정한다.
- 입력 추가는 `UWxInputConfig` DA에 IA를 넣고 폰에서 바인딩. 어빌리티 발동 IA는 어빌리티 CDO가, 메뉴 입력은 CommonUI가 소유하므로 여기 넣지 않는다.
- 새 UI 데이터 연결은 `UMVVMViewModelContextResolver`를 파생한 리졸버로 만든다 — 도메인 모듈은 게임 모듈을 참조할 수 없으므로 양쪽에 의존하는 리졸버가 데이터를 주입한다.
- 권한 모델: ASC는 캐릭터가 소유하며 리스폰마다 새로 초기화(PlayerState에 스탯 없음). 서버 권위 값(락온 타겟 등)은 복제로 전 머신이 같은 대상을 읽는다.

## 여기서부터 읽어라
1. `Source/WxGame/WxGame.Build.cs` — 이 모듈이 소비하는 도메인 플러그인 목록으로 경계를 먼저 잡는다.
2. `Source/WxGame/Character/WxCharacterBase.h` — 도메인 컴포넌트가 실제로 캐릭터에 조립되는 방식과 ASC 소유 모델.
3. `Source/WxGame/Controller/WxPlayerController.h` — 폰 리스폰을 넘어 살아남는 플레이어 단위 컴포넌트가 어디에 붙는지.

## 관련
- 상위: 게임 실행 진입. 각 시스템 세부는 [[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxAI]] · [[WxWorld]] · [[WxDialogue]] · [[WxQuest]] · [[WxCore]] 참조.

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 65파일 — `/readme-writer`로 갱신*
