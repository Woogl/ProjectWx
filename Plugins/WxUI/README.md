# WxUI — UI 시스템

> 화면 레이어 스택(HUD·메뉴·모달)을 소유하고, 게임플레이 데이터를 위젯이 바인딩할 수 있는 ViewModel 로 바꿔 내보내는 모듈이다.
> 화면 인디케이터와 자막처럼 "화면에만 존재하는 표시물"도 여기서 소유하며, 다른 도메인이 쓸 StateTree 노드까지 함께 제공한다.

## 책임
**담당**
- 로컬 플레이어 화면의 레이어 구조(`UI.Layer.*` 스택)와 위젯 푸시·비동기 클래스 스트리밍
- 게임 정지·입력 모드(`FUIInputConfig`) 결정 — 활성 위젯들을 재평가해 최종 상태를 정한다
- ASC(어트리뷰트·어빌리티·이펙트)와 캐릭터 표시 데이터를 ViewModel 트리로 노출
- 확인 팝업, 사망 화면·대화 창 같은 전역 화면의 표시 트리거
- 화면 인디케이터 액터와 자막 슬롯, 그리고 이를 구동하는 StateTree 태스크 노드

**경계 (비담당)**
- 게임플레이 태그·`IWxUIData`(표시 이름/아이콘) 정의는 [[WxCore]]. 본 모듈은 `WxGameplayTags::UI_Layer_*`·`UI_Action_*` 를 **사용만** 한다.
- 어빌리티·이펙트·어트리뷰트의 실제 로직은 [[WxCombat]]. 여기서는 ASC 를 읽어 표시로 바꾸기만 한다.
- 상호작용 후보 탐색은 [[WxWorld]]의 스캐너가, 그 목록 VM(`UWxViewModel_InteractionList`)은 `WxGame` 이 소유한다. 본 모듈은 항목 하나짜리 `UWxViewModel_Interaction` 만 제공한다.
- 자막·인디케이터를 *언제* 띄울지는 [[WxQuest]] 등 소비 도메인의 StateTree 에셋이 정한다. 본 모듈은 노드와 표시 수단만 제공한다.
- 위젯 계층·바인딩 그래프는 WBP 에셋이 소유한다. C++ 은 `BindWidget` 계약과 베이스 클래스만 둔다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 모듈의 중심. 레이아웃 생성·레이어 푸시·게임 정지 재평가·상태 태그(사망/대화) 감시가 모두 여기로 모인다 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 태그 → `UCommonActivatableWidgetStack` 맵을 들고 있는 화면 루트. 레이어 개념의 실체 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 모든 위젯 푸시가 지나가는 단일 경로(스트리밍·취소 포함). 푸시 관련 버그는 먼저 여기를 본다 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxActivatableWidget` | 모든 화면 위젯의 베이스. 입력 모드·정지 요구를 서브시스템에 알리는 지점 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | VM 계층의 루트. 공유 VM 조회(`FindSharedViewModel`)와 이미지 비동기 로드를 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC 하나를 물고 자식 VM(어트리뷰트·어빌리티 슬롯·활성 이펙트)을 지연 생성하는 허브 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스를 프로젝트 설정에서 주입하는 자리 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `AWxIndicator` | 대상과 분리된 독립 액터로 화면 인디케이터 위치를 계산(화면 밖이면 가장자리 클램프) | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` |

## 확장 포인트 / 규약
- **레이어**: `UI.Layer.Game / GameMenu / Menu / Modal` (선언은 `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`). 레이아웃 WBP 의 `LayerTags` 배열 순서가 곧 z-order 이며, 스택은 `NativeOnInitialized` 에서 태그별로 생성된다.
- **새 화면 추가**: `UWxActivatableWidget` 파생 WBP 를 만들고 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(LayerTag, Class)` 로 푸시한다. `InputMode`·`bPauseGame` 만 세팅하면 입력·정지는 서브시스템이 알아서 재평가한다.
- **새 뷰모델 추가**: `UWxViewModel` 파생. 소스 오브젝트를 Outer 로 두고 `FindSharedViewModel` 패턴을 따르면 같은 소스를 보는 위젯들이 인스턴스를 공유한다. 이미지 필드는 직접 로드하지 말고 `RequestImageAsync` + `ApplyLoadedImage` 를 쓴다.
- **위젯에 VM 연결**: WBP 의 MVVM 뷰모델 소스로 `UWxViewModelResolver_PlayerCharacter`(빙의 폰의 Character VM) 또는 `UWxViewModelResolver_Subtitle`(글로벌 자막 슬롯)을 지정한다. 전자는 위젯이 **빙의 이후** 생성될 것을 전제한다.
- **데이터 주도**: 자막 1편 = `FWxSubtitleTableRow` DataTable 1개(행이 `NextRow` 로 스스로 이어짐). 표시 이름·아이콘은 소스 오브젝트의 `IWxUIData`(WxCore)에서 읽는다. 화면 클래스 4종은 `UWxUIDeveloperSettings`(DefaultGame.ini)로 주입한다.
- **StateTree 노드**: `FWxStateTreeTask_MarkIndicator`·`FWxStateTreeTask_PrintSubtitle` 은 소비 도메인이 WxUI 를 참조하지 않고도 에셋에서 고를 수 있도록 본 모듈이 제공한다. 두 노드 모두 자기가 띄운 것만 회수한다.
- **권한 모델**: 인디케이터·자막·HUD 는 복제하지 않는 로컬 표시물이며, 게임 정지(`bPauseGame`)는 싱글/리슨 호스트(v1) 전제다. 최대 4인 멀티에서는 각 클라이언트가 자기 화면을 스스로 만든다.
- **한계**: 레이아웃과 추적 상태는 로컬 플레이어 1명을 전제로 단수로 유지된다 — 스플릿스크린이 필요해지면 `ULocalPlayer` 키로 묶어야 한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — 로컬 플레이어 등장 → 레이아웃 생성 → 폰 태그 감시 → 화면 푸시까지의 전체 수명 흐름이 한 파일에 있다.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어라는 개념이 실제로 무엇인지(태그 → 스택) 여기서 확정된다.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_Character.h` → `WxViewModel_AbilitySystem.h` — VM 트리의 공유 키(Outer)와 지연 생성 규칙이 이 순서로 드러난다.
4. `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp` — 취소·레이아웃 교체 등 푸시 경로의 예외 처리가 모여 있다.
5. `Plugins/WxUI/Source/WxUI/Public/Component/WxPlayerLayoutComponent.h` — 컨트롤러 쪽에서 HUD 가 어떻게 붙고 걷히는지의 반대편 끝.

## 관련
- 상위: `WxGame` — `AWxCharacterBase` 가 `IWxUIData` 를 구현해 표시 데이터를 공급하고, `UWxViewModel_InteractionList`·`UWxViewModel_Quest` 가 본 모듈의 VM 을 확장한다. `UWxPlayerLayoutComponent`·`UWxNameplateComponent` 부착은 BP 에서 이뤄진다.
- 함께 보기: [[WxCore]](태그·`IWxUIData` 정의), [[WxCombat]](VM 이 읽는 ASC 데이터의 출처), [[WxWorld]](상호작용 후보 탐색). StateTree 노드를 쓰는 도메인은 C++ 의존 없이 에셋에서만 연결되므로, 소비처는 코드가 아니라 ST 에셋에서 찾는다.

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 58파일 — `/readme-writer`로 갱신*
