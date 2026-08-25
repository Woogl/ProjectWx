# WxUI — UI 시스템

> CommonUI 기반의 레이어 스택 UI를 오케스트레이션하고, ASC·표시 데이터를 MVVM 뷰모델로 화면에 잇는 도메인 플러그인. C++는 위젯·뷰모델·컴포넌트의 베이스와 배관을 제공하고, 실제 외형은 BP/WBP 콘텐츠가 저작한다.

## 책임
**담당**
- CommonUI ActivatableWidget을 태그로 구분된 레이어 스택에 push/pop 하는 화면 전환 배관 (`UWxUIManagerSubsystem`, `UWxPrimaryGameLayout`)
- MVVM 뷰모델 계층 — ASC(어트리뷰트/어빌리티/이펙트)·캐릭터·상호작용·자막·인디케이터를 표시 필드로 노출, 이미지 비동기 스트리밍 공통 처리
- HUD·네임플레이트·화면공간 인디케이터·확인 팝업의 C++ 베이스 위젯과 부착용 컨트롤러/위젯 컴포넌트
- 자막 시스템 소유 및 StateTree 태스크 노드 제공(소비 도메인이 UI를 참조하지 않고 에셋에서 골라 쓰도록)

**경계 (비담당)**
- 위젯 외형·계층·바인딩(WBP)은 콘텐츠 저작물이며 여기서 나열하지 않는다
- HUD/사망/대화 화면으로 "어떤 위젯을 띄울지"와 컴포넌트 부착은 Experience 에셋(GameFeature)이 정한다 — 본 모듈은 정해진 값을 실어 두고 띄우기만 한다
- 어트리뷰트·어빌리티·대화 세션 등 원천 데이터는 소비만 하고 소유하지 않는다(ASC/GAS는 엔진, 도메인 로직은 각 도메인 플러그인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 중앙 오케스트레이터(GameInstanceSubsystem). 레이어 push, 확인 팝업, HUD·사망·대화 화면 관리, 게임 정지 재평가 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 화면 루트 위젯. `UI.Layer` 태그로 키된 ActivatableWidgetStack 들을 z-order 순으로 보유 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 모든 화면 위젯의 베이스. 입력 모드·게임 정지 의사(`ShouldPauseGame`) 표명 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | MVVM 뷰모델 베이스. 공유 VM 조회(`FindSharedViewModel`)·이미지 비동기 스트리밍 제공 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출하는 Composite. ASC당 하나 지연 생성 | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxUILibrary` | Blueprint 진입점(BFL). 서브시스템·레이아웃 조회, 레이어 push, 확인 팝업 | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 프로젝트 전역 레이아웃·팝업·사망/대화 화면 클래스 지정(Config) | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxIndicatorManagerComponent` | 로컬 PC의 화면공간 인디케이터 목록을 들고 매 틱 화면 좌표를 투영·발행 | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 `UWxGamePopup`)을 상속한 WBP를 만들고, `UWxUIManagerSubsystem::PushContentToLayer` 계열이나 `UWxUILibrary`/`UWxAsyncAction_PushWidgetToLayer`로 `UI.Layer` 태그를 지정해 push 한다.
- 새 레이어: `UWxPrimaryGameLayout`의 `LayerTags`(z-order = 배열 순서)에 `UI.Layer.*` 태그를 추가하고 LayerContainer에 대응 Stack을 배치한다.
- 새 뷰모델: `UWxViewModel`을 상속하고, 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage`로 소프트 참조를 로드해 노출한다. ASC 파생 데이터는 `UWxViewModel_AbilitySystem`의 `GetOrCreate...` 지연 생성 경로를 쓴다.
- 데이터 주도 설정: 전역 클래스는 `UWxUIDeveloperSettings`, 콘텐츠별 HUD·화면 지정은 Experience 에셋이 서브시스템/컴포넌트에 주입한다. 팝업 구성은 `UWxGamePopupDescriptor`.
- 자막 노드는 `FWxStateTreeTask_PrintSubtitle`(StateTree 태스크)로 소비 도메인 에셋에서 직접 배치한다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면 흐름의 중심. 레이어 push·팝업·HUD/사망/대화 처리와 정지 로직이 모여 있다.
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조와 태그→스택 매핑의 골격.
3. `Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_AbilitySystem.h` — 뷰모델 배관과 ASC 노출 구조.

## 관련
- 상위: Experience(GameFeature)가 HUD·컴포넌트 주입과 화면 클래스 지정으로 이 모듈을 구동한다. GAS·CommonUI·ModelViewViewModel·StateTree(엔진), 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 63파일 — `/readme-writer`로 갱신*
