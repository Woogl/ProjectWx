# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델 위에 게임의 화면·HUD·팝업·자막·인디케이터를 얹는 UI 도메인. 표시할 데이터는 알지만 그 데이터의 출처(캐릭터·전투·대화)는 알지 못한다.

## 책임
**담당**
- 레이어 기반 화면 스택 관리와 화면 push/pop, 게임 정지·입력 모드 조정 (`WxUIManagerSubsystem` + `WxPrimaryGameLayout`)
- ASC/캐릭터 표시 정보를 UMG에 노출하는 MVVM 뷰모델 계층 (`WxViewModel` 파생), 바인딩용 변환 함수
- 확인 팝업, HUD, 액티버터블 위젯 등 공용 위젯 베이스와 비동기 위젯 로드/push 프리미티브
- 자막·화면 인디케이터의 런타임 표시 및 이를 거는 StateTree 노드 제공

**경계 (비담당)**
- 구체 캐릭터 타입과 표시 데이터의 원본(이름·초상화·어트리뷰트 정의)은 게임 모듈이 뷰모델에 주입한다 — WxUI는 소비자를 모른다
- 어빌리티/어트리뷰트/이펙트 상태의 소유는 GAS이며, 전투 규칙은 [[WxCombat]]에 있다
- 대화 세션의 개시·종료는 [[WxDialogue]]가 소유하고, WxUI는 상태 태그를 관찰해 대화 창만 띄운다
- 자막·인디케이터를 "언제 무엇에" 거는지는 소비 도메인([[WxQuest]]·[[WxWorld]] 등)이 StateTree 에셋에서 결정한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 전체 오케스트레이터. 플레이어별 레이아웃 생성, 폰 ASC 태그(사망·대화) 관찰, 화면·팝업 push, 정지 재평가 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | `UI.Layer` 태그별 위젯 스택을 z-order로 보유하는 최상위 레이아웃 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 모든 화면의 베이스. 입력 모드·게임 정지 의사 표명 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 뷰모델 베이스. Outer로 공유 인스턴스 조회, 표시 이미지 비동기 스트리밍 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC 하나당 하나. 어트리뷰트·어빌리티·이펙트 자식 VM을 지연/이벤트 관리하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 위젯 클래스 비동기 로드 후 레이어에 push. push 전 초기화 훅·취소 지원 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxUILibrary` | Blueprint 진입점. 레이아웃 접근·확인 팝업 표시·레이어 비활성화 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 레이아웃/팝업/사망·대화 화면 클래스를 config로 데이터 주입 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 새 화면은 `UWxActivatableWidget`(또는 `UWxGamePopup`/`UWxHUDLayout`)을 상속한 WBP로 만들고, 레이어에 `UWxAsyncAction_PushWidgetToLayer` 또는 매니저 서브시스템 경로로 push한다.
- 새 표시 뷰모델은 `UWxViewModel`을 상속한다. 공유가 필요하면 데이터 소스를 Outer로 하는 `FindSharedViewModel`/`GetOrCreate` 관례를 따른다 — "같은 Outer를 집는다"가 발행자·소비자를 잇는 유일한 연결이다. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage`로 다룬다.
- 화면 클래스·레이아웃·팝업은 하드 참조 대신 `UWxUIDeveloperSettings`의 `TSoftClassPtr` config로 지정한다(미지정이면 해당 동작 없음).
- 자막·인디케이터를 다른 도메인이 참조 없이 쓰도록, 표시를 거는 StateTree 태스크(`WxStateTreeTask_PrintSubtitle`, `WxStateTreeTask_MarkIndicator`)를 본 모듈이 함께 제공한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — UI 생명주기와 화면 push/정지/태그 관찰이 모두 여기서 엮인다. 모듈 전체의 제어 흐름 지도.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — VM 베이스의 공유·비동기 이미지 규약. `WxViewModel_AbilitySystem`/`_Character`로 내려가면 HUD 데이터 흐름이 보인다.
3. `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` — 위젯이 실제로 레이어에 올라가는 경로와 취소·초기화 훅.

## 관련
- 상위: [[WxGame]]
- 참조: [[WxCore]]

---
*문서 기준 커밋 `81c04f5` · 생성일 2026-09-11 · 소스 56파일 — `/readme-writer`로 갱신*
