# WxGame — 게임 조립 모듈

> 프로젝트의 기본 게임 모듈. GameMode·Controller·Character 등 프레임워크 골격을 정의하고, 각 Wx 플러그인(전투·인벤토리·UI·AI·대화·퀘스트·월드)을 실제 폰과 컨트롤러 위에 조립·배선한다.

## 책임
**담당**
- 게임 프레임워크 골격 정의: GameMode / GameState / PlayerController / AIController / Character 계층
- 캐릭터 조립: ASC를 캐릭터가 직접 소유, 무기·모션워프·락온·히트스톱·메타휴먼 서브오브젝트 배선
- 플레이어 단위 조립: 인벤토리·상호작용 스캐너·대화 세션·레이아웃 컴포넌트를 컨트롤러에 상주
- 프론트엔드 흐름: 선택한 폰·레벨로 맵을 열고 도착 GameMode가 그 폰을 쓰게 함
- 도메인 교차 결선: MVVM 리졸버/뷰모델 브리지, 대화×외형이 겹치는 NPC, 상호작용 실행 어빌리티

**경계 (비담당)**
- 전투 규칙·대미지·어트리뷰트·락온 컴포넌트는 [[WxCombat]]
- 인벤토리 데이터·아이템 정의·보상 테이블은 [[WxInventory]]
- 뷰모델 베이스·HUD 레이아웃·CommonUI 위젯은 [[WxUI]]
- AI 인지·블랙보드 동기화는 [[WxAI]], 대화 데이터·액터 계약은 [[WxDialogue]], 퀘스트 진행은 [[WxQuest]]
- 상호작용 감지·스포너·미니언 계약 등 월드 상호작용은 [[WxWorld]]
- 공용 정의·GAS 베이스는 [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxGameMode` | 맵별 조립 지점. GameState/PlayerState 지정, 프론트엔드 선택 폰 우선 결정 | `Framework/WxGameMode.h` |
| `AWxCharacterBase` | 플레이어·에너미 공통 베이스. ASC를 직접 소유, 팀/사망/래그돌 처리 | `Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 게임플레이 입력·카메라·아이템 사용·입력 버퍼 소유 | `Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | AI 조립·상호작용·보상·보스 표시·미니언 계약을 얹은 적 캐릭터 | `Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 폰 리스폰에도 살아남는 플레이어 단위 컴포넌트(인벤토리·스캐너·대화·레이아웃) 소유 | `Controller/WxPlayerController.h` |
| `AWxAIController` | AI 폰 전용 컨트롤러. 인지→블랙보드 결선은 WxAI에 위임 | `Controller/WxAIController.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택 폰·레벨을 들고 맵 전환을 구동 | `FrontEnd/WxGameFlowSubsystem.h` |
| `UWxViewModelResolver_PlayerCharacter` | 빙의 폰의 ASC/표시 데이터를 WxUI 뷰모델에 주입하는 도메인 브리지 | `MVVM/WxViewModelResolver_PlayerCharacter.h` |

## 확장 포인트 / 규약
- GameMode BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings의 GameModeOverride로 맵별 구성을 고른다. 폰은 프론트엔드 선택이 우선, 없으면 `DefaultPawnClass`.
- 캐릭터 외형·입력·행동은 데이터 주도: `UWxInputConfig` DA(이동/시선/점프/웅크리기), `UWxMetaHumanComponent` 에셋 슬롯, `UWxAIBehaviorComponent`의 BehaviorTree, `AWxEnemyCharacter`의 `RewardRow` DataTable.
- 리플리케이션/권한: ASC를 캐릭터가 직접 소유해 리스폰마다 스탯을 새로 초기화(PlayerState 미사용). `Team`·`LockOnComponent`는 서버 권위로 복제, 상호작용 실행(`UWxAbility_Interact`)은 ServerOnly.
- `UWxCharacterMovementComponent`가 전 캐릭터 공용 이동으로 교체되어 비대칭 낙하(상승/하강 중력 차)를 제공한다.

## 여기서부터 읽어라
1. `Framework/WxGameMode.h` — 무엇이 어떻게 조립되는지의 최상위 진입점(클래스 지정·폰 선택)
2. `Character/WxCharacterBase.h` — 캐릭터가 어떤 서브오브젝트·인터페이스를 물고 ASC를 어떻게 소유하는지
3. `Controller/WxPlayerController.h` — 플레이어 단위 상태가 어느 컴포넌트로 갈리는지
4. `MVVM/WxViewModelResolver_PlayerCharacter.h` — 게임 모듈이 WxUI를 참조하지 못하는 뷰모델에 어떻게 데이터를 주입하는지

## 관련
- 상위: 이 모듈이 최종 조립점이다. `WxCore` 위에서 `WxCombat`·`WxInventory`·`WxUI`·`WxAI`·`WxDialogue`·`WxQuest`·`WxWorld`를 폰·컨트롤러·위젯에 결선한다.

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 67파일 — `/readme-writer`로 갱신*
