# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 기반으로 게임 UI의 런타임 골격을 제공하는 도메인 플러그인. 화면 레이아웃·팝업·HUD·자막·화면 인디케이터의 C++ 토대를 깔고, 실제 위젯 외형은 BP/WBP가 채운다.

## 책임
**담당**
- 로컬 플레이어당 레이어 스택 레이아웃 소유와 레이어별 위젯 push (`UWxPrimaryGameLayout`, `UWxAsyncAction_PushWidgetToLayer`).
- UI 수명주기 오케스트레이션: HUD·확인 팝업·사망 화면·대화 창의 생성·표시·정리, 활성 위젯 기반 게임 정지 재평가 (`UWxUIManagerSubsystem`).
- CommonUI 파생 위젯 베이스: 활성화 위젯·버튼·탭 리스트·팝업의 공용 규약 (`Widget/`).
- MVVM 뷰모델 베이스와 표시 이미지 비동기 스트리밍, ASC/어트리뷰트/어빌리티/이펙트·캐릭터·자막·인디케이터·상호작용 뷰모델 (`MVVM/`).
- 화면 인디케이터 투영: 매 틱 월드 대상의 화면 좌표 계산·발행 (`Indicator/`).
- 자막 슬롯 소유와 StateTree 자막 출력 노드 (`Subtitle/`).

**경계 (비담당)**
- 어트리뷰트·어빌리티·상태 태그의 원천인 ASC는 [[WxCombat]]가 소유하며, 뷰모델은 이를 관찰만 한다.
- 구체 캐릭터 타입·표시 데이터(이름·초상화)는 소비 측(게임 모듈)이 `Initialize`로 주입한다 — WxUI는 캐릭터 타입을 모른다.
- 어떤 HUD를 띄울지, 어떤 컴포넌트를 컨트롤러에 붙일지는 Experience 에셋(GameFeature/게임 모듈)이 정해 UI 매니저에 발행한다.
- 자막·대화 세션을 여는 트리거는 [[WxDialogue]]·[[WxQuest]] 등 소비 도메인의 StateTree 노드가 push한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이아웃·팝업·HUD·사망/대화 화면·게임 정지를 총괄하는 UI 오케스트레이터 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 `UCommonActivatableWidgetStack`을 z-order로 들고 위젯을 push하는 레이아웃 루트 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 요청을 얹은 CommonUI 활성화 위젯 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스를 비동기 로드해 레이어에 push하는 async action (레이아웃 교체 시 취소) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxViewModel` | 표시 이미지 비동기 스트리밍·공유 뷰모델 조회를 제공하는 MVVM 뷰모델 베이스 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC의 어트리뷰트/어빌리티/이펙트를 자식 VM으로 지연 노출하는 Composite 뷰모델 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxIndicatorManagerComponent` | 화면 인디케이터 목록을 들고 매 틱 화면 좌표를 투영·발행하는 컨트롤러 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUILibrary` | 서브시스템·레이아웃 접근과 확인 팝업 표시를 여는 Blueprint Function Library | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- **위젯 추가**: 활성화 위젯은 `UWxActivatableWidget`, 팝업은 `UWxGamePopup`, 버튼은 `UWxButtonBase`, 탭 리스트는 `UWxTabListWidgetBase`를 상속한 WBP로 만든다. HUD 루트는 `UWxHUDLayout`.
- **레이어 규약**: 레이어는 `UI.Layer.*` 게임플레이 태그로 식별하며 배열 순서가 z-order다(0=최하단). 위젯 표시는 대상 레이어 태그로 push하고, 위젯 클래스는 `TSoftClassPtr`로 지정해 비동기 로드한다.
- **뷰모델 추가**: `UWxViewModel`을 상속하고, 표시 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage` 훅으로 스트리밍한다. ASC당·소스당 하나를 공유해야 하는 VM은 `FindSharedViewModel`/`GetOrCreate` 패턴을 따른다(소스를 Outer로 캐시).
- **주입 규약**: HUD 클래스·부착 컴포넌트는 코드가 아니라 Experience 에셋이 정한다. 캐릭터 표시 데이터·ASC는 소비 측이 `Initialize`로 넘긴다.
- **StateTree 노드**: 자막 출력은 `FWxStateTreeTask_PrintSubtitle`. 자막 뷰모델은 MVVM 글로벌 컬렉션에 하나만 존재하며 소비 도메인이 값을 push한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이아웃 생성부터 HUD·팝업·화면 전환·정지까지 UI 흐름 전체가 한곳에서 보인다.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` + `Widget/WxActivatableWidget.h` — 레이어 스택과 위젯 베이스가 CommonUI 위에서 어떻게 얹히는지.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` + `MVVM/WxViewModel_AbilitySystem.h` — 뷰모델 베이스와 ASC 컴포지트로 MVVM 데이터 흐름을 파악.
4. `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` — 컨트롤러 컴포넌트 → 등록증 → 뷰모델 → HUD 위젯의 로컬 표시 파이프라인.

## 관련
- 상위: <[[WxGame]]> (Experience가 HUD·주입 목록을 발행)
- 데이터 소스: <[[WxCombat]]> (ASC·어트리뷰트·상태 태그)

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 60파일 — `/readme-writer`로 갱신*
