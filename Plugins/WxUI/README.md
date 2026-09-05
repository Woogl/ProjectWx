# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 기반으로 게임의 화면 UI를 관리한다. HUD·팝업·메뉴부터 월드 공간 네임플레이트·화면 인디케이터·자막까지, 무엇을 언제 띄울지를 데이터 주도로 오케스트레이션하는 도메인 모듈이다.

## 책임
**담당**
- 레이어(HUD/Game/Menu/Modal 등) 스택 관리와 위젯 push/pop, 입력 모드·게임 정지 조정
- ViewModel 계층(캐릭터·ASC·어트리뷰트·아이템 등)과 표시용 이미지 비동기 스트리밍
- 확인 팝업, 사망/대화 화면 등 상태 태그에 반응하는 화면 전환
- 월드 공간 네임플레이트·화면 인디케이터·자막 표시 및 StateTree 노드 제공

**경계 (비담당)**
- 구체 캐릭터/아이템 타입 정의와 표시 데이터의 원본 — 소비 측(게임 모듈)이 `Initialize`로 주입한다
- 무엇을 띄울지의 정책(HUD 클래스 지정 등) — Experience 에셋이 UI 매니저에 발행
- 전투·인벤토리 등 도메인 로직 — [[WxCombat]], [[WxInventory]] 등이 소유하고 WxUI는 그 상태만 관찰

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 전체 UI 오케스트레이터. 레이아웃 생성·상태 태그 관찰·화면 전환의 중심 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 위젯 스택을 보유하는 루트 위젯 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스의 데이터 주도 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 소프트 클래스 로드 후 지정 레이어에 push하는 비동기 액션 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 지정을 갖는 화면 위젯 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 모든 ViewModel의 베이스. 이미지 비동기 스트리밍·공유 VM 조회 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxUILibrary` | BP 진입점. 서브시스템/레이아웃 접근, 확인 팝업 표시 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `AWxIndicator` | 대상을 가리키는 화면 인디케이터 액터(화면 밖이면 가장자리 클램프) | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` |

## 확장 포인트 / 규약
- 새 화면 위젯은 `UWxActivatableWidget`를 상속하고 `InputMode`·`bPauseGame`로 입력·정지를 선언한다. 실제 정지 적용은 UI 매니저가 전 레이어를 재평가해 결정한다.
- 새 ViewModel은 `UWxViewModel`를 상속한다. 소스를 Outer로 공유 인스턴스를 만드는 규약(`FindSharedViewModel`/`GetOrCreate`)이 발행자와 소비자를 잇는 유일한 연결고리다.
- WxUI는 구체 도메인 타입을 알지 못하므로 표시 데이터는 소비 측이 `Initialize`/`InitializeViewModels`로 주입한다.
- 어떤 위젯을 어느 레이어에 띄울지는 `UWxUIDeveloperSettings`와 Experience 발행값으로 정한다 — 코드가 아니라 데이터.
- 자막·인디케이터는 StateTree 노드(`FWxStateTreeTask_PrintSubtitle`, `FWxStateTreeTask_MarkIndicator`)를 함께 제공해, 퀘스트 등 소비 도메인이 WxUI를 참조하지 않고도 에셋에서 골라 쓴다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이아웃 생성부터 상태 태그 관찰·화면 전환·게임 정지까지 전체 제어 흐름의 허브
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 태그(`UI.Layer.*`)와 z-order, 위젯 스택 구조
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — MVVM 베이스와 공유 VM/이미지 스트리밍 규약(파생 VM 이해의 출발점)

## 관련
- 상위: Experience 에셋이 HUD/레이아웃 정책을 발행하고 GameFeature가 콘텐츠를 켠다. 도메인 상태를 [[WxCombat]]·[[WxInventory]] 등에서 관찰하며, 공용 정의는 [[WxCore]]에 의존한다.

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 57파일 — `/readme-writer`로 갱신*
