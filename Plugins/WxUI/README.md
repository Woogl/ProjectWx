# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 기반으로, 게임플레이 화면(HUD·메뉴·팝업·대화 창)과 월드 표시물(네임플레이트·화면 인디케이터·자막)을 띄우고 관리하는 도메인.

## 책임
**담당**
- 레이어 스택(HUD·Menu·Modal 등) 구성과 위젯 push, 게임 일시정지·입력 모드 조율
- 로컬 플레이어 빙의/사망/대화 상태를 관찰해 그에 맞는 화면을 자동으로 띄우고 걷는 오케스트레이션
- 표시 데이터를 UMG에 노출하는 MVVM 뷰모델 계층(캐릭터·어빌리티·속성·아이템·자막·인디케이터 등)과 이미지 비동기 스트리밍
- 월드 표시물: 거리 기반 네임플레이트, 화면 가장자리 클램프 인디케이터, 자막 — 및 이를 데이터에서 부리는 StateTree 태스크 노드

**경계 (비담당)**
- 어빌리티·속성 값의 소유와 계산은 GAS/[[WxCombat]] 쪽. 뷰모델은 대상의 `IWxUIData`·ASC를 통해 읽기만 한다
- 인벤토리/아이템 데이터 자체는 [[WxInventory]], 대화 세션 진행은 [[WxDialogue]], 상호작용 대상 판정은 [[WxWorld]] — 본 모듈은 그 상태를 관찰해 화면에만 반영한다
- WBP/위젯 계층·MVVM 바인딩 그래프 등 실제 표시 구성은 콘텐츠(BP/WBP)가 소유

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 오케스트레이터. 플레이어/폰 상태를 구독해 레이아웃 생성·화면 push·정지 재평가를 총괄 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그 → 위젯 스택 맵을 든 루트 위젯. 실제 push의 종착지 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스를 프로젝트 설정에서 주입하는 데이터 주도 지점 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxActivatableWidget` | CommonUI 파생 화면 베이스. 입력 모드·정지 희망을 선언 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스 비동기 로드 후 레이어에 push. push 계열이 공통으로 경유 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxViewModel` | 뷰모델 베이스. 공유 VM 조회와 아이콘/초상화 비동기 스트리밍 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModelResolver_PlayerCharacter` | 위젯을 빙의 폰의 공유 Character VM에 잇는 MVVM 리졸버 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_PlayerCharacter.h` |
| `AWxIndicator` | 대상을 가리키는 화면 인디케이터 액터. 화면 밖이면 가장자리로 클램프 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` |

## 확장 포인트 / 규약
- **새 화면 추가**: `UWxActivatableWidget`(또는 팝업이면 `UWxGamePopup`, HUD면 `UWxHUDLayout`)를 상속한 WBP를 만들고, 표시 클래스는 `UWxUIDeveloperSettings` 또는 컨트롤러 컴포넌트의 소프트 클래스 슬롯에 지정한다. push는 `UWxAsyncAction_PushWidgetToLayer`가 로드/취소까지 처리한다.
- **레이어**: `UI.Layer.*` 게임플레이 태그로 스택을 식별한다. z-order는 `UWxPrimaryGameLayout::LayerTags` 배열 순서. 입력 토글은 `UI.Action.*` 태그를 CommonUI 액션으로 수신.
- **새 뷰모델**: `UWxViewModel` 파생. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage`로 소프트 참조를 로드해 노출한다. 캐릭터 VM은 ASC를 Outer로 공유(`GetOrCreate`)돼 같은 대상을 보는 위젯끼리 인스턴스를 공유한다.
- **다른 도메인이 UI를 부리는 법**: 인디케이터·자막은 StateTree 태스크(`FWxStateTreeTask_MarkIndicator`·`FWxStateTreeTask_PrintSubtitle`)로 노출되어, 퀘스트 등 소비 도메인이 WxUI를 코드 참조하지 않고 에셋에서 노드만 골라 쓴다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면이 언제·어떻게 뜨고 걷히는지, 정지·입력 조율까지 흐름의 중심
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` + `Widget/WxAsyncAction_PushWidgetToLayer.h` — 레이어 스택 구조와 push 경로
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` + `MVVM/WxViewModelResolver_PlayerCharacter.h` — 데이터가 위젯에 닿는 MVVM 축과 공유 VM 규약

## 관련
- 사용처: [[WxGame]](컨트롤러 레이아웃 컴포넌트), [[WxQuest]]·[[WxDialogue]](인디케이터·자막 노드 소비), [[WxCombat]]/[[WxInventory]](표시 데이터 원천)
- 함께 보는 모듈: [[WxCore]](공용 정의·`IWxUIData` 등 유일한 Wx 의존)

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 58파일 — `/readme-writer`로 갱신*
