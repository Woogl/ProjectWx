# WxUI — UI 시스템

> 게임 화면 전체를 책임지는 런타임 UI 플러그인. CommonUI 레이어 스택으로 HUD·메뉴·팝업의 표시/입력/정지를 관리하고, 게임플레이 상태를 MVVM ViewModel로 위젯에 공급한다. 화면 인디케이터와 자막도 함께 소유한다.

## 책임
**담당**
- 로컬 플레이어 화면 구성: 레이어(z-order) 스택, HUD/메뉴/모달 push, 활성 위젯에 따른 입력 모드·게임 정지 재평가
- 확인 팝업(`ShowConfirmation`)과 사망·대화 화면의 자동 표시/회수
- 게임플레이 데이터 → 표시용 ViewModel 변환 (`Wx|MVVM`): ASC의 어트리뷰트·어빌리티·이펙트, 캐릭터/아이템/상호작용 등을 위젯에 노출
- 화면 인디케이터 액터(`AWxIndicator`)와 자막 표시, 그리고 이를 거는 StateTree 태스크 노드

**경계 (비담당)**
- 표시할 데이터의 의미/원본: 대상은 `IWxUIData`(→ [[WxCore]])로만 조회하며 구체 타입을 모른다
- 위젯·WBP 애셋의 시각 디자인, 실제 MVVM 바인딩 그래프 (Content 측 BP/WBP 소관)
- 인벤토리·대화·전투 등 도메인 로직: 각 도메인 모듈([[WxInventory]]·[[WxDialogue]]·[[WxCombat]])이 소유하며, WxUI는 그 결과만 표시

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이아웃 생성·push·팝업·화면 자동표시·정지 재평가의 오케스트레이터 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어별 `CommonActivatableWidgetStack`을 담는 화면 루트. 태그→스택 맵 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxViewModel` | 모든 VM의 베이스. 소프트 이미지 비동기 스트리밍과 `FindSharedViewModel`(소스=공유 키) 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC당 하나의 Composite VM. 어트리뷰트/어빌리티/이펙트 자식 VM을 지연 생성·추적 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModelResolver_PlayerCharacter` | 위젯 소유 PC의 빙의 폰·ASC로 캐릭터 VM을 해석하는 MVVM Resolver | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_PlayerCharacter.h` |
| `UWxUIDeveloperSettings` | 레이아웃/팝업/사망·대화 화면 클래스를 프로젝트 설정으로 지정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `AWxIndicator` | 대상을 가리키는 화면 인디케이터 액터(위치만 담당, 비복제) | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` |

## 확장 포인트 / 규약
- **새 ViewModel**: `UWxViewModel`을 상속하고, 소프트 이미지는 `RequestImageAsync`/`ApplyLoadedImage` 훅으로 노출한다(소프트 참조를 UMG에 직접 넘기지 않음). 공유가 필요하면 데이터 소스를 Outer로 `GetOrCreate`/`FindSharedViewModel` 패턴을 따른다 — Outer가 곧 공유 키다.
- **새 위젯을 화면에 붙일 때**: 레이어 태그(`UI.Layer.*`)를 골라 `UWxAsyncAction_PushWidgetToLayer`(BP: `PushWidgetToLayer`)로 push. 활성화 전 초기화는 `BeforePush`/`SetBeforePushCallback`에서 한다.
- **위젯이 데이터를 받는 법**: WBP에서 `UWxViewModelResolver_*`를 MVVM 소스로 지정. 자막처럼 화면당 하나인 것은 MVVM 글로벌 컬렉션에 단일 인스턴스로 둔다(`UWxViewModel_Subtitle::GetOrCreate`).
- **정지/입력**: `UWxActivatableWidget`의 `InputMode`·`bPauseGame`로 선언하면 매니저가 전 레이어를 재평가해 실제 적용(멀티플레이 미적용).
- **소비 도메인용 노드**: 인디케이터·자막은 StateTree 태스크(`FWxStateTreeTask_MarkIndicator`·`FWxStateTreeTask_PrintSubtitle`)로도 제공해, 퀘스트 등이 WxUI를 참조하지 않고 에셋에서 골라 쓰게 한다.
- **레이어/입력 태그는 C++ 네이티브 선언이 아니라** 프로젝트 태그(`UI.Layer.*`, `UI.Action.*`)를 meta 필터로 참조한다.
- **단일 로컬 플레이어 전제(v1)**: 매니저의 레이아웃·추적 상태는 단수다. 인디케이터·자막은 비복제 로컬 표시.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면 수명·레이어·정지의 중앙. 이 모듈이 "언제 무엇을 띄우는지"의 전모가 여기 헤더 주석에 있다.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — VM 계층의 뿌리와 공유 인스턴스 규약. 데이터 흐름을 잡으려면 먼저 읽는다.
3. `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp` — 위젯이 어떻게 살아 있는 게임플레이 데이터에 연결되는지의 짧고 대표적인 예.

## 관련
- 상위: 컨트롤러에 붙는 `UWxPlayerLayoutComponent`가 HUD를 push하고, 각 도메인([[WxCombat]]·[[WxInventory]]·[[WxDialogue]]·[[WxQuest]])이 표시할 데이터와 상태 태그를 공급한다.
- 의존 규약: 표시 데이터는 [[WxCore]]의 `IWxUIData`로만 읽어 도메인 타입에 의존하지 않는다.

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 58파일 — `/readme-writer`로 갱신*
