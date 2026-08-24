# WxUI — UI 시스템

> CommonUI + MVVM(ModelViewViewModel) 기반의 UI 골격을 제공하는 도메인 플러그인. 레이어 스택 위에 위젯을 push 하는 흐름, ASC/캐릭터/자막 등을 UMG에 노출하는 뷰모델, 화면 인디케이터·이름표·자막 표시를 담당한다.

## 책임
**담당**
- 레이어드 UI 프레임(`UWxPrimaryGameLayout`)과 태그별 스택 push/pop, 그리고 이를 여닫는 게임 인스턴스 오케스트레이터(`UWxUIManagerSubsystem`)
- CommonUI 파생 베이스 위젯군(Activatable/Button/Tab/ActionWidget/Popup)과 BP/Async 진입점
- 표시 데이터를 UMG에 잇는 MVVM 뷰모델 계층(`UWxViewModel` 및 파생), 이미지 비동기 스트리밍 공통화
- 화면 인디케이터 투영(`UWxIndicatorManagerComponent`), 이름표(`UWxNameplateComponent`), 자막(뷰모델 + StateTree 노드)

**경계 (비담당)**
- 전투/어트리뷰트의 실제 값과 ASC — [[WxCombat]]가 소유, 여기선 뷰모델로 관측만 함
- 인벤토리·대화·퀘스트의 도메인 로직 — [[WxInventory]] · [[WxDialogue]] · [[WxQuest]]. UI는 상태 태그·자막 노드로 연결
- 어떤 HUD/화면 클래스를 실제로 띄울지의 결정 — Experience 에셋(GameFeature/게임 모듈)이 주입, UI는 담아 두고 실행만 함

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이어 생성, push, 확인 팝업, 폰 상태 태그(사망·대화) 관찰, 게임 정지 재평가 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 태그별 `CommonActivatableWidgetStack`을 z-order로 들고 위젯을 push 하는 레이아웃 루트 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입점. 서브시스템·레이아웃 획득, 소프트 위젯 push, 확인 팝업 | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxActivatableWidget` | CommonUI 화면 베이스. 입력 모드·정지 의사(`ShouldPauseGame`) 선언. 팝업/HUD/화면의 공통 부모 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | MVVM 베이스. 아이콘/초상화 등 이미지의 비동기 스트리밍과 공유 VM 조회(`FindSharedViewModel`) 공통화 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC의 어트리뷰트/어빌리티/이펙트/보유 태그를 자식 VM으로 노출하는 Composite. ASC당 하나(`GetOrCreate`) | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxIndicatorManagerComponent` | 로컬 PC의 화면 인디케이터 목록을 매 틱 투영하는 컨트롤러 컴포넌트. 등록증(`UWxIndicatorDescriptor`) 발급 | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUIDeveloperSettings` | 레이아웃·확인 팝업·사망/대화 화면의 소프트 클래스를 지정하는 프로젝트 설정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 새 화면/팝업은 `UWxActivatableWidget`(또는 `UWxGamePopup`)을 상속한 WBP로 만들고, `UWxUILibrary::PushSoftContentToLayer` / `UWxAsyncAction_PushWidgetToLayer` / 서브시스템 push로 태그 레이어에 올린다. 레이어 태그는 `UI.Layer.*`(예: Game / Menu / Modal), 입력 액션은 `UI.Action.*`.
- 정지(pause)는 위젯이 `bPauseGame`으로 의사만 표하고, 실제 적용은 서브시스템이 전 레이어 활성 위젯을 재평가해 결정한다(멀티플레이 미적용).
- 새 뷰모델은 `UWxViewModel`을 상속해 `Deinitialize`에서 자기 정리 후 Super 호출. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage` 규약을 따른다. 소스 오브젝트를 Outer로 공유 조회하려면 `FindSharedViewModel`.
- 어떤 HUD/사망·대화 화면을 띄울지는 UI가 정하지 않는다 — Experience가 서브시스템(`SetGameHUDClass`)이나 `UWxUIDeveloperSettings`에 소프트 클래스를 발행하고, `UWxHUDComponent`가 Experience 주입 목록으로 컨트롤러에 부착된다(컨트롤러는 컴포넌트 클래스를 모름).
- 자막 노드(`FWxStateTreeTask_PrintSubtitle`)와 인디케이터 마킹 노드는 본 모듈이 StateTree 태스크로 제공한다 — 소비 도메인(퀘스트 등)이 WxUI를 코드 참조하지 않고 에셋에서 골라 쓰게 하려는 배치.
- 탭 UI는 `UWxTabListWidgetBase` + `FWxTabDescriptor`(프리레지스터/동적 등록)와 `IWxTabButtonInterface`로 구성한다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이어 생성부터 push·팝업·상태 태그 관찰·정지까지 전체 흐름의 중심
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 = 태그별 스택이라는 뼈대 모델
3. `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — 도메인 값을 UMG로 잇는 MVVM 패턴의 대표 사례(지연 생성·증분 갱신)
4. `Source/WxUI/Public/WxUILibrary.h` — BP·콘텐츠 측에서 이 시스템에 들어오는 관문

## 관련
- 상위: Experience/게임 모듈이 HUD·화면 클래스를 주입([[WxGame]]), 도메인 값 관측 대상 [[WxCombat]], 상태 연결 [[WxDialogue]] · [[WxInventory]] · [[WxQuest]]

---
*문서 기준 커밋 `e1999dc` · 생성일 2026-08-24 · 소스 63파일 — `/readme-writer`로 갱신*
