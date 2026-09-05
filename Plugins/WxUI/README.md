# WxUI — UI 시스템

> CommonUI 레이어 스택 위에 게임의 화면·HUD·팝업을 띄우고 관리하며, MVVM 뷰모델로 게임 상태(ASC·상호작용·자막 등)를 위젯에 노출하는 도메인 모듈. BP/WBP가 실체지만 뼈대는 여기 C++ 타입이 잡는다.

## 책임
**담당**
- 레이어 기반 화면 스택 관리 — 레이어 태그별 위젯 push/pop, 메뉴 활성 여부·게임 정지 판정
- HUD·확인 팝업·사망/대화 화면 등 게임 라이프사이클 연동 UI 오케스트레이션
- MVVM 뷰모델 계층 — 표시용 데이터를 위젯에 노출, 아이콘/초상화 비동기 스트리밍
- 화면 인디케이터(월드 대상 지시)·자막 재생, 이를 데이터 도메인에서 쓰게 하는 StateTree 노드
- CommonUI 확장 위젯 베이스(버튼·탭·팝업·Activatable)

**경계 (비담당)**
- 표시할 구체 데이터(캐릭터 이름·초상화 등)는 소비 측이 주입 — WxUI는 구체 캐릭터/게임 타입을 모른다
- 어떤 HUD를 띄울지, 어떤 컴포넌트를 컨트롤러에 붙일지는 Experience(게임 모듈) 결정
- 실제 위젯 계층·바인딩 그래프는 BP/WBP 애셋 담당
- 전투/인벤토리 등 도메인 로직은 각 도메인 모듈

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이션 허브 — 레이어 push, 팝업/HUD/사망/대화 화면 연동, 정지 판정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그→위젯 스택 매핑을 쥔 루트 레이아웃 위젯 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스 비동기 스트리밍 후 레이어에 push하는 async action | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxViewModel` | MVVM 뷰모델 베이스 — 공유 인스턴스 조회·이미지 비동기 스트리밍 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스 프로젝트 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxUILibrary` | BP 진입점 — 서브시스템/레이아웃 조회, 확인 팝업 표시 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxActivatableWidget` | 화면 위젯 베이스 — 입력 모드·게임 정지 의사 표명 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDComponent` | 로컬 플레이어 HUD를 Game 레이어에 붙이는 컨트롤러 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Component/WxHUDComponent.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 `UWxGamePopup`) 상속. 레이어 push는 `UWxAsyncAction_PushWidgetToLayer` 또는 매니저 API로. 위젯이 게임 정지를 원하면 `bPauseGame`으로 표명하되 실제 정지는 매니저가 전 레이어를 재평가해 결정.
- 새 뷰모델: `UWxViewModel` 상속. 데이터 소스를 Outer로 하는 공유 인스턴스 규약(`FindSharedViewModel`/`GetOrCreate`)이 발행자↔소비자를 잇는 유일한 연결. 아이콘/초상화는 `RequestImageAsync`/`ApplyLoadedImage`로 소프트 참조를 로드해 노출.
- 데이터 주입: WxUI는 구체 타입을 모르므로 표시 데이터(이름·초상화 등)는 소비 측이 `Initialize`로 주입(`UWxNameplateComponent`, `UWxViewModel_Character`).
- 데이터 주도 자막: `FWxSubtitleTableRow` DataTable로 정의, 행이 `NextRow`로 스스로 이어짐.
- 도메인 연동: 자막·인디케이터는 StateTree 태스크(`WxStateTreeTask_PrintSubtitle`·`WxStateTreeTask_MarkIndicator`)로 노출해, 퀘스트 등 소비 도메인이 WxUI를 코드 참조하지 않고 에셋에서 노드를 골라 쓰게 한다.
- 위젯 클래스 설정은 `UWxUIDeveloperSettings`(프로젝트 설정) 또는 Experience가 매니저에 발행한 값으로.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 모듈 전체 흐름의 허브. 레이어·팝업·HUD·사망/대화가 어떻게 엮이는지 여기서 잡힌다.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — MVVM 계층의 공통 규약(공유 인스턴스·이미지 스트리밍). 파생 VM들이 전제로 삼는다.
3. `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` — 위젯이 화면에 올라오는 실제 메커니즘(스트리밍→push).

## 관련
- 상위: Experience(게임 모듈)가 HUD 클래스 발행·`UWxHUDComponent` 주입으로 이 모듈을 구동. 자막/인디케이터 노드는 [[WxQuest]] 등 데이터 도메인이 소비. 뷰모델은 [[WxCombat]]의 ASC/어트리뷰트를 표시.
- 의존: [[WxCore]] (공용 정의)

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 58파일 — `/readme-writer`로 갱신*
