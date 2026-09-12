# WxGame — 게임 메인 모듈

> 각 Wx 플러그인이 제공하는 기능을 실제 게임의 캐릭터·프레임워크·컨트롤러·HUD 뷰모델로 조립하는 최종 결합 지점이다. 플러그인은 부품을, 이 모듈은 완성품을 만든다.

## 책임
**담당**
- 플레이/적/NPC 캐릭터 계층(`AWxCharacterBase` 파생)에 ASC·전투·이동·외형 컴포넌트를 조립하고 사망·팀·리스폰 흐름을 배선
- 프레임워크(`AWxGameMode`/`AWxGameState`/`AWxPlayerController`/`AWxAIController`/`AWxPlayerState`)에 플레이어 단위·판 단위 컴포넌트를 소유시키는 배치
- 프론트엔드 흐름(캐릭터·목적지 선택 → 맵 오픈 → 도착 폰 지정): `UWxGameFlowSubsystem`
- 도메인 데이터를 WxUI 위젯에 잇는 MVVM 뷰모델·리졸버(`Resolver`가 위젯별로 VM 생성)
- 게임 고유 어빌리티(`UWxAbility_Interact`·`UWxAbility_UseItem`), 입력 구성(`UWxInputConfig`), 치트(`UWxCheatManager`)
- 메타휴먼 어셈블 산출물을 오너 메시에 조립하는 `UWxMetaHumanComponent`

**경계 (비담당)**
- ASC·어트리뷰트·락온·히트스톱·피니셔·어빌리티 베이스 → [[WxCombat]]
- 인벤토리 컴포넌트·아이템 정의·아이템 사용 컴포넌트·보상 테이블 → [[WxInventory]]
- AI 행동·인지 컴포넌트, BT/블랙보드 → [[WxAI]]
- 상호작용 스캐너·스포너·`IWxSpawnable`/`IWxInteractable` 계약 → [[WxWorld]]
- 대화 세션·`AWxDialogueActor` → [[WxDialogue]]
- 퀘스트 컴포넌트 → [[WxQuest]]
- HUD 레이아웃·네임플레이트·CommonUI 액션·위젯 기반 클래스 → [[WxUI]]
- 공용 정의·`IWxUIData`·뷰모델 베이스 → [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxCharacterBase` | 플레이어·적 공통 베이스. ASC를 캐릭터가 직접 소유, 여러 도메인 컴포넌트의 조립 원점 | `Source/WxGame/Character/WxCharacterBase.h` |
| `AWxPlayerCharacter` | 입력·카메라·아이템사용·인풋버퍼를 얹은 조종 캐릭터 | `Source/WxGame/Character/WxPlayerCharacter.h` |
| `AWxEnemyCharacter` | AI·상호작용·보상·보스 표시를 결합한 적 캐릭터 | `Source/WxGame/Character/WxEnemyCharacter.h` |
| `AWxPlayerController` | 폰 리스폰에도 살아남는 플레이어 단위 컴포넌트(인벤토리·스캐너·대화·레이아웃) 보유 | `Source/WxGame/Controller/WxPlayerController.h` |
| `AWxGameMode` | 맵별 BP·프론트엔드 선택으로 스폰 폰 클래스를 결정 | `Source/WxGame/Framework/WxGameMode.h` |
| `UWxGameFlowSubsystem` | 프론트엔드 선택을 들고 맵을 열어 도착 GameMode에 폰을 넘김 | `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` |
| `UWxMetaHumanComponent` | 에셋 지정만으로 바디·페이스·그룸·복장을 오너 메시에 부착 | `Source/WxGame/Character/Component/WxMetaHumanComponent.h` |

## 확장 포인트 / 규약
- 새 캐릭터: `AWxCharacterBase`/`AWxPlayerCharacter`/`AWxEnemyCharacter`를 상속한 BP로 구성. 무기·외형·팀·보상은 BP 디폴트 프로퍼티로 데이터 주도 지정(생성자 서브오브젝트는 항상 존재)
- 새 UI 데이터 바인딩: `UMVVMViewModelContextResolver` 파생 리졸버를 `Source/WxGame/MVVM/`에 추가하고 위젯이 위젯별 VM을 받게 함. 리졸버는 뷰 상태를 보관하지 않으며 구독은 반환한 VM이 소유
- 새 게임 어빌리티: [[WxCombat]]의 `UWxAbilityBase`를 상속. 상호작용·아이템사용은 서버 권위 실행만 담당하고 감지·프롬프트는 각각 스캐너·인벤토리가 처리
- 리플리케이션/권한: ASC를 캐릭터가 소유하고 리스폰마다 새로 초기화하므로 `AWxPlayerState`는 상태를 갖지 않음. 락온 타겟은 서버 권위 복제
- 프론트엔드 컨트롤러에도 같은 플레이어 컴포넌트가 붙지만 각자 자기 가드로 무동작

## 여기서부터 읽어라
1. `Source/WxGame/Character/WxCharacterBase.h` — 어떤 플러그인의 무엇이 캐릭터에 조립되는지 한눈에 보이는 결합의 원점
2. `Source/WxGame/Controller/WxPlayerController.h` — 폰이 아닌 플레이어 단위로 사는 인벤토리·스캐너·대화·레이아웃의 소유처
3. `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h` — 선택 → 맵 오픈 → 폰 결정으로 이어지는 게임 진입 흐름
4. `Source/WxGame/MVVM/` — 도메인 상태를 WxUI로 잇는 리졸버·뷰모델 다발(Ability·Boss·Dialogue·Inventory·Quest·InteractionList)

## 관련
- 상위: 게임의 최상위 조립 모듈 — 모든 Wx 도메인 플러그인([[WxCombat]]·[[WxInventory]]·[[WxAI]]·[[WxWorld]]·[[WxDialogue]]·[[WxQuest]]·[[WxUI]]·[[WxCore]])을 참조해 실제 플레이 대상으로 결합

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 59파일 — `/readme-writer`로 갱신*
