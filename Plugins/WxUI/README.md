# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 축으로 한 게임 UI 기반. 위젯을 레이어에 push 하고, 게임플레이 상태(ASC 태그·어트리뷰트·인디케이터·자막)를 뷰모델로 옮겨 UMG에 노출한다. 실제 화면(WBP)은 콘텐츠가 저작하고, 이 모듈은 그 뼈대와 데이터 흐름만 제공한다.

## 책임
**담당**
- 레이어드 UI 호스트: `UWxPrimaryGameLayout`(레이어별 `UCommonActivatableWidgetStack`)와 `UWxUIManagerSubsystem`(push/pop, 입력 모드, 게임 정지 재평가).
- MVVM 뷰모델 계층: `UWxViewModel` 파생들이 ASC·인디케이터·상호작용·자막 등 소스를 표시 필드로 변환. 소프트 이미지 비동기 스트리밍 공통 제공.
- HUD·컴포넌트 주입점: `UWxHUDComponent`, `UWxNameplateComponent`, `UWxIndicatorManagerComponent`(컨트롤러/위젯 컴포넌트, Experience가 주입).
- 공용 팝업·확인창(`UWxGamePopup`/`UWxGamePopupDescriptor`), BP 진입 라이브러리(`UWxUILibrary`).
- 소비 도메인이 UI를 참조하지 않고 쓰도록 StateTree 노드 제공(자막 출력·인디케이터 표시).

**경계 (비담당)**
- 어트리뷰트·GameplayEffect·능력 등 뷰모델이 미러링하는 원천 상태 → [[WxCombat]](ASC는 GameplayAbilities).
- 대화 세션 상태·자막 내용 저작 → [[WxDialogue]](자막 노드는 여기서 제공, 진행은 대화/퀘스트가 소비).
- 상호작용 대상 탐지 → [[WxWorld]].
- 어떤 HUD/위젯 WBP를 언제 띄울지의 결정과 컴포넌트 주입 목록 → Experience(GameFeature) 계층.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이어 push/팝업/HUD, 사망·대화 태그 관찰, 정지 재평가 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그 → 위젯 스택 맵. z-order 관리, push 구현 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 의사를 가진 활성 위젯 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | MVVM 베이스. 공유 VM 조회·소프트 이미지 비동기 스트리밍 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxHUDComponent` | 로컬 플레이어 HUD를 Game 레이어에 띄우는 컨트롤러 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Component/WxHUDComponent.h` |
| `UWxIndicatorManagerComponent` | 화면 좌표 인디케이터 목록 보유·매 틱 투영·발행 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUILibrary` | BP 진입점(레이어 push, 팝업, 활성 위젯 제어) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 소프트 클래스 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 `UWxGamePopup`) 파생 WBP → 로드된 클래스는 `UWxUIManagerSubsystem::PushContentToLayer`, 소프트 클래스는 `UWxAsyncAction_PushWidgetToLayer`로 레이어 태그(`UI.Layer.*`)를 지정해 push. 정지·입력 모드는 위젯의 `bPauseGame`/`InputMode`가 선언하고 매니저가 전 레이어를 재평가해 실제 적용.
- 새 표시 데이터: `UWxViewModel` 파생 VM 추가 → 소스를 Outer로 `FindSharedViewModel`로 공유. 위젯은 VM 존재를 알 뿐 소스 컴포넌트를 직접 참조하지 않음(예: 인디케이터/캐릭터 VM은 컴포넌트 델리게이트를 구독).
- 컴포넌트 주입은 코드가 아니라 Experience 에셋의 주입 목록으로 함(`UWxHUDComponent`/`UWxIndicatorManagerComponent`/`UWxNameplateComponent`는 부착 주체를 모름). 원격 사본 거부는 각 컴포넌트 책임.
- 소비 도메인용 StateTree 노드: `FWxStateTreeTask_PrintSubtitle`(자막), `FWxStateTreeTask_MarkIndicator`(인디케이터) — 도메인이 WxUI를 코드 참조하지 않고 에셋에서 선택.
- 레이어 태그·화면 클래스 등 설정은 `UWxUIDeveloperSettings`(Config=Game)에서 데이터 주도.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — UI의 중심 오케스트레이터. 레이어·팝업·HUD·정지·상태 태그 관찰이 모두 여기로 모임.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 태그가 실제 위젯 스택으로 어떻게 매핑되는지.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — 게임플레이 상태가 UMG로 넘어가는 MVVM 관문(공유·이미지 스트리밍 규약).

## 관련
- 상위: <[[WxGame]] / Experience(GameFeature) 계층이 HUD·컴포넌트 주입을 결정>

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 63파일 — `/readme-writer`로 갱신*
