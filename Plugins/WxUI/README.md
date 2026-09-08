# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 뼈대로, 게임 UI의 표시·수명·입력 모드를 관장하는 도메인 모듈. 실제 화면은 BP/WBP가, 구조·데이터 배선은 이 C++ 계층이 맡는다.

## 책임
**담당**
- 로컬 플레이어당 레이어 루트(`UWxPrimaryGameLayout`)와 레이어별 위젯 스택 관리, 소프트 클래스 비동기 스트리밍 push
- 화면 수명 오케스트레이션: HUD·메뉴·팝업·사망 화면·대화 창의 push/pop, 게임 정지·입력 모드 재평가
- MVVM 뷰모델 계층 — ASC 어트리뷰트/어빌리티/이펙트, 캐릭터·아이템·상호작용·자막·인디케이터 표시 데이터의 지연 생성과 공유
- 자막·인디케이터 시스템과 그 StateTree 노드 제공(소비 도메인이 UI를 참조하지 않고 에셋에서 골라 쓰게)

**경계 (비담당)**
- 표시 원본 데이터(캐릭터 이름·초상화·ASC)는 소비 측(게임 모듈)이 주입한다 — WxUI는 구체 캐릭터 타입을 모른다
- 어빌리티/어트리뷰트/이펙트의 실제 시뮬레이션은 GameplayAbilities. 상호작용·대화·퀘스트의 로직은 각 도메인 모듈(WxUI는 그 상태를 뷰모델로 비출 뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 레이아웃 수명·레이어 push·정지/대화/사망을 총괄하는 GameInstance 서브시스템 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그 → 위젯 스택 맵을 쥔 화면 루트(z-order = 배열 순서) | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 모든 활성 위젯의 베이스 — 입력 모드·정지 희망을 선언 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 소프트 클래스를 스트리밍해 레이어에 push, before/after 훅과 취소 제공 | `Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxViewModel` | MVVM 베이스 — 공유 VM 조회(Outer=소스)와 이미지 비동기 스트리밍 공통화 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | ASC·이름·초상화를 묶는 캐릭터 Composite VM(하위에 AbilitySystem VM) | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스의 프로젝트 설정 레지스트리 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxUILibrary` | BP 진입 함수 라이브러리(서브시스템·레이아웃 조회, 확인 팝업) | `Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 `UWxHUDLayout`/`UWxGamePopup`)을 상속한 WBP를 만들고, 레이어 push는 `UWxAsyncAction_PushWidgetToLayer`로 한다. push 대상 레이어는 `UI.Layer.*` 게임플레이 태그로 지정.
- 새 뷰모델: `UWxViewModel` 파생. 공유가 필요하면 `FindSharedViewModel`/`GetOrCreate` 패턴(소스 오브젝트를 Outer로 삼아 같은 소스 = 같은 VM)을 따르고, 이미지 필드는 `RequestImageAsync` + `ApplyLoadedImage` 오버라이드로 스트리밍한다.
- 레이아웃·팝업·사망/대화 화면 클래스는 코드가 아니라 `UWxUIDeveloperSettings`(Config=Game)로 주입한다. 컨트롤러별 HUD는 `UWxPlayerLayoutComponent`의 `LayoutClass`로.
- StateTree 노드(`FWxStateTreeTask_PrintSubtitle`·`FWxStateTreeTask_MarkIndicator`)는 순수 구조체 파라미터만 노출해 소비 도메인이 UI 의존 없이 에셋에서 사용한다.
- MVVM 바인딩용 변환 헬퍼는 `UWxMVVMConversionLibrary`(태그/오브젝트 → Visibility 등)에 모은다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면 수명·레이어·정지·대화/사망 흐름의 중심. 모듈 전체 배선이 여기 모인다.
2. `Source/WxUI/Public/MVVM/WxViewModel.h` + `WxViewModel_AbilitySystem.h` — 공유 VM·지연 생성·수명(Outer) 규약. MVVM 계층의 사고방식이 여기 담긴다.
3. `Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` — 소프트 클래스 스트리밍 push의 취소·교체 안전장치(대부분의 화면 코드가 이걸 거친다).

## 관련
- 상위: `WxGame`(HUD/컨트롤러 배선·표시 데이터 주입), 표시 대상 도메인 [[WxCombat]] · [[WxInventory]] · [[WxWorld]] · [[WxDialogue]] · [[WxQuest]]
- 기반: [[WxCore]]

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 56파일 — `/readme-writer`로 갱신*
