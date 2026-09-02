# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 토대로 게임 화면을 구성한다. HUD·팝업·대화창·자막·화면 인디케이터·네임플레이트를 띄우고, 게임플레이 데이터(ASC 어트리뷰트/어빌리티/이펙트 등)를 위젯에 바인딩할 뷰모델을 공급한다.

## 책임
**담당**
- 로컬 플레이어의 최상위 레이아웃과 레이어별 위젯 스택 관리(z-order 레이어에 ActivatableWidget push/pop).
- 위젯 활성 상태에 따른 게임 정지 조율, 사망/대화 상태 태그를 관찰한 화면 전환.
- 표시용 뷰모델 계층(캐릭터·ASC·어트리뷰트·어빌리티·이펙트·자막·인디케이터·상호작용)과 소프트 이미지 비동기 스트리밍.
- 자막·인디케이터를 소비 도메인이 UI 모듈 참조 없이 에셋에서 골라 쓸 수 있게 StateTree 노드까지 함께 제공.
- CommonUI 파생 베이스 위젯(버튼·탭·팝업·HUD 레이아웃)과 BP/BP-Async 진입점.

**경계 (비담당)**
- 무엇을 표시할지의 원본 데이터(전투 수치·인벤토리 내용 등)는 각 도메인 모듈 소유. WxUI는 주입받은 데이터와 ASC만 뷰모델로 감싼다.
- 어떤 HUD·화면을 켤지의 결정은 UI 밖(Experience 에셋)이 발행하며, WxUI는 발행된 값을 소비한다.
- 위젯 계층·MVVM 바인딩 그래프 등 실제 화면 구성은 WBP(콘텐츠) 소유.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이아웃 생성, 팝업/대화/사망 화면 push, 정지 조율 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 로컬 플레이어 최상위 위젯. 태그별 레이어 스택 보유(배열 순서 = z-order) | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입점(매니저/레이아웃 조회, 레이어 비활성화, 확인 팝업) | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스 비동기 로드 후 레이어에 push하는 async action(push 전/후 훅) | `Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxActivatableWidget` | 모든 화면 위젯의 베이스(입력 모드·게임 정지 의사 표시) | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 뷰모델 베이스. 공유 VM 조회·이미지 비동기 스트리밍 공통 제공 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출하는 Composite | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxUIDeveloperSettings` | 레이아웃·확인 팝업·사망/대화 화면 클래스 프로젝트 설정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- **화면 위젯**: `UWxActivatableWidget`을 WBP로 파생. `InputMode`/`bPauseGame`으로 입력·정지 의사를 표시하고, 실제 정지는 매니저가 전 레이어를 재평가해 결정한다. 팝업은 `UWxGamePopup`(+ `UWxGamePopupDescriptor`) 파생.
- **레이어 push**: 레이어는 `UI.Layer` 카테고리 태그로 지정. 즉시 클래스는 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`, 인스턴스는 매니저의 `PushWidgetInstanceToLayer`.
- **뷰모델**: `UWxViewModel` 파생. 데이터 소스를 Outer로 공유 VM을 찾는 `FindSharedViewModel`, 소프트 이미지는 `RequestImageAsync`/`ApplyLoadedImage` 오버라이드로 로드. 어트리뷰트/어빌리티 VM은 바인딩 요청 시 지연 생성.
- **소비 도메인 연동**: 자막·인디케이터는 각각 `WxStateTreeTask_PrintSubtitle`/`WxStateTreeTask_MarkIndicator`를 에셋에서 골라 쓰며, 자막 VM은 MVVM 글로벌 컬렉션에 화면당 하나. 표시 데이터(캐릭터명·초상화 등)는 게임 모듈이 `Initialize`로 주입.
- **설정 주도**: 레이아웃·팝업·사망/대화 화면 클래스는 `UWxUIDeveloperSettings`(Config=Game)에서 지정.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이아웃 생성부터 화면 전환·정지까지 전체 흐름의 허브. 헤더 주석이 조율 규약을 담는다.
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어/z-order 모델. 위젯이 어디에 쌓이는지의 근간.
3. `Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_AbilitySystem.h` — 공유 VM·이미지 스트리밍 규약과 게임플레이 데이터 노출 방식.

## 관련
- 상위: Experience 에셋이 HUD/화면 클래스를 발행하고 `UWxHUDComponent`를 컨트롤러에 주입. 게임플레이 데이터는 [[WxCombat]]·[[WxWorld]] 등 도메인 모듈이 제공하며, 자막·인디케이터 StateTree 노드는 [[WxQuest]]·[[WxDialogue]] 등이 소비한다.

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 58파일 — `/readme-writer`로 갱신*
