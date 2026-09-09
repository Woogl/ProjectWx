# WxUI — UI 시스템

> 화면 레이어 스택(HUD·메뉴·모달·팝업)을 관리하고, 게임 상태를 UMG에 잇는 MVVM 뷰모델과 자막·인디케이터·네임플레이트 같은 표시 요소를 제공하는 런타임 UI 플랫폼.

## 책임
**담당**
- CommonUI 기반 레이어 스택(`UI.Layer.*` 태그로 구분되는 Game/Menu/Modal 등)과 위젯 push/pop, 활성 위젯에 따른 게임 정지·입력 모드 결정
- 로컬 플레이어 수명주기(빙의·컨트롤러 교체) 추적과 그에 맞춘 레이아웃 생성·정리
- 폰 상태 태그(사망·대화) 관찰 후 대응 화면 자동 표시
- MVVM 뷰모델 계층 — 게임 상태(캐릭터/ASC 어트리뷰트·어빌리티·이펙트/아이템/상호작용/자막/인디케이터)를 UMG 바인딩용으로 노출, 이미지 비동기 스트리밍 공통 처리
- 표시 요소: 확인 팝업, 자막(StateTree 태스크), 화면 인디케이터, 월드 공간 네임플레이트

**경계 (비담당)**
- 공용 정의·태그·기반 타입은 [[WxCore]]에 위임
- 구체 캐릭터/전투 데이터는 알지 못한다 — 표시 데이터는 소비 측(게임 모듈·[[WxCombat]] 등)이 뷰모델에 주입한다
- 실제 위젯 계층·바인딩 그래프(WBP)는 BP 에셋 영역이라 이 모듈 범위 밖

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 시스템의 허브 — 플레이어/폰 추적, 레이어 push, 정지 결정을 총괄 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | `UI.Layer` 태그 → 위젯 스택 맵을 들고 z-order를 관리하는 루트 레이아웃 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입점 — 서브시스템·레이아웃 접근, 레이어 위젯 비활성화, 확인 팝업 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스 비동기 스트리밍 후 레이어에 push, push 전 초기화 훅 제공 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxActivatableWidget` | 활성화형 위젯 베이스 — 입력 모드·정지 의사 선언 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 모든 뷰모델의 베이스 — 공유 인스턴스 조회·이미지 비동기 로드 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` / `UWxViewModel_AbilitySystem` | 소비 측이 주입/조회하는 Composite 뷰모델(캐릭터·ASC 진입점) | `Plugins/WxUI/Source/WxUI/Public/MVVM/` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스를 지정하는 프로젝트 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 새 화면/HUD는 `UWxActivatableWidget`(또는 `UWxHUDLayout`)을 상속해 WBP로 만들고, `UWxAsyncAction_PushWidgetToLayer` 또는 매니저의 push 경로로 `UI.Layer` 태그를 지정해 띄운다.
- 새 표시 데이터는 `UWxViewModel` 파생을 만들고 `FindSharedViewModel`/`GetOrCreate`(Source를 Outer로 하는 공유 키) 규약을 따른다 — 발행자와 소비자는 같은 Source Outer 약속으로만 이어진다. 이미지는 소프트 참조를 노출하지 말고 `RequestImageAsync`로 로드한 하드 참조만 노출한다.
- 레이아웃·팝업·사망/대화 화면 클래스는 `UWxUIDeveloperSettings`(Config = Game)에서 데이터 주도로 지정한다.
- 로컬 플레이어 단수 전제 — 레이아웃·추적 상태가 단수라 스플릿스크린 시 나중 플레이어가 앞을 갈아치운다. 로컬 표시 요소(인디케이터 등)는 복제하지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 플레이어/폰 추적부터 레이어 push·정지 결정까지 모듈의 제어 흐름이 모이는 지점
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — 게임 상태를 UMG에 잇는 뷰모델 계층의 공유·이미지 로딩 규약

## 관련
- 상위: 게임 모듈([[WxGame]])과 각 도메인([[WxCombat]]·[[WxInventory]]·[[WxDialogue]] 등)이 표시 데이터를 뷰모델에 주입해 사용한다. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 56파일 — `/readme-writer`로 갱신*
