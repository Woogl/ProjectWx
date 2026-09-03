# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 기반으로 게임의 화면 UI(HUD·메뉴·팝업·네임플레이트·인디케이터·자막)를 띄우고, 폰 상태에 반응해 전환·정지를 조율한다.

## 책임
**담당**
- 레이어 기반 화면 구성: `UI.Layer.*` 태그로 나뉜 CommonUI 위젯 스택과, 로컬 플레이어당 하나 생성되는 루트 레이아웃 관리.
- 위젯 푸시 파이프라인: 위젯 클래스 비동기 스트리밍 → 레이어 스택에 push, 로드 중 화면 교체·컴포넌트 소멸에 대한 취소 처리.
- 게임 상태 연동: 빙의 폰의 ASC 상태 태그(사망·대화)를 관찰해 해당 화면을 띄우고, 활성 위젯의 정지 요구를 모아 게임 정지를 걸고 해제한다.
- MVVM 뷰모델 계층: ASC의 어트리뷰트/어빌리티/이펙트/보유 태그와 캐릭터·상호작용·인디케이터·자막 데이터를 UMG 바인딩용 뷰모델로 노출하고, 표시 이미지의 비동기 로드를 공통 제공.
- 표시 액터·노드: 화면 인디케이터 액터와, 이를 구동하는 StateTree 태스크(인디케이터·자막)를 함께 제공.

**경계 (비담당)**
- 위젯/WBP의 시각 구조(위젯 계층·바인딩 그래프)는 BP 콘텐츠이며 C++ 범위 밖이다.
- 구체 캐릭터·전투·인벤토리 데이터 정의는 각 도메인 모듈이 소유한다 — WxUI는 타입을 알지 못하고, 표시 데이터는 소비 측이 뷰모델에 주입한다.
- 어떤 HUD를 띄울지·컴포넌트를 어디에 붙일지는 Experience/GameFeature가 정한다 — 이 모듈은 발행된 값과 주입된 컴포넌트를 받아 동작할 뿐이다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 레이아웃 생성·위젯 푸시·상태 태그 관찰·게임 정지를 조율하는 허브 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 위젯 스택을 든 루트 레이아웃(z-order = 배열 순서) | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스를 스트리밍해 레이어에 push, 취소 가능한 비동기 액션 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxHUDComponent` | Experience가 컨트롤러에 주입해 Game 레이어에 HUD를 띄우는 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Component/WxHUDComponent.h` |
| `UWxViewModel` | 표시 이미지 비동기 로드·공유 뷰모델 조회를 제공하는 모든 VM의 베이스 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `AWxIndicator` | 대상을 가리키고 화면 밖이면 가장자리로 당기는 화면 인디케이터 액터 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스 지정(Config = Game) | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 레이어 추가: `LayoutClass`의 `LayerTags`(`UI.Layer.*`) 배열에 태그를 넣는다 — 배열 인덱스가 z-order다. 위젯 푸시는 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 또는 서브시스템 API를 쓴다.
- 표시 VM 추가: `UWxViewModel`을 상속하고, 아이콘/초상화 등은 `RequestImageAsync` + `ApplyLoadedImage`로 소프트 참조를 비동기 로드한다. ASC 파생 데이터는 `UWxViewModel_AbilitySystem` 아래에 붙이고, 바인딩용 변환은 `UWxMVVMConversionLibrary`에 둔다.
- StateTree UI 노드는 시스템을 소유한 이 모듈이 함께 제공한다(`FWxStateTreeTask_MarkIndicator`, `FWxStateTreeTask_PrintSubtitle`) — 퀘스트 등 소비 도메인이 WxUI를 참조하지 않고 에셋에서 노드를 골라 쓰게 하려는 것이다.
- 데이터 주도: 팝업/사망/대화/레이아웃 클래스는 `UWxUIDeveloperSettings`가, HUD 클래스는 Experience가 서브시스템에 발행한 값(`SetGameHUDClass`)이, 자막은 `FWxSubtitleTableRow` DataTable이 구동한다.
- 로컬 표시 전제: 인디케이터·자막·정지는 복제하지 않으며 로컬 플레이어 단수를 가정한다(v1 싱글/리슨 호스트). 자막·전역 자막 VM은 MVVM 글로벌 컬렉션에 하나만 둔다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — 레이아웃 생성부터 상태 태그 관찰·정지까지 모듈의 제어 흐름이 모이는 곳.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 태그 ↔ 위젯 스택 모델. 화면 구성을 이해하는 출발점.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — Composite/지연 생성/공유 키 패턴. 나머지 VM이 같은 관례를 따른다.

## 관련
- 상위: Experience/GameFeature 플러그인이 HUD 클래스를 발행하고 [[WxUI]] 컴포넌트를 컨트롤러에 주입한다. 표시 데이터와 StateTree 노드는 [[WxCombat]]·[[WxInventory]]·[[WxDialogue]]·[[WxQuest]] 등 도메인 모듈이 소비한다.
- 공용 정의·게임플레이 태그·Experience 골격은 [[WxCore]]에 의존한다.

---
*문서 기준 커밋 `f0aad4c` · 생성일 2026-09-03 · 소스 58파일 — `/readme-writer`로 갱신*
