# UI 구조

UI 레이어와 화면 띄우기, 입력 모드, 게임 일시정지, 팝업, 월드에 붙는 위젯을 모은 문서다.
뷰모델 구성과 바인딩 규칙은 [UI 뷰모델](UI-뷰모델.md), 적·보스 HP 표시는 [네임플레이트](네임플레이트.md)에 있다.

## 구현

### 레이어와 화면 띄우기
- Lyra의 CommonUI 구조를 옮겼다. 코드는 `Source/WxGame/UI/` 루트 클래스와 `Foundation`(팝업·버튼·레이어 push 비동기 액션), `Frontend`, `IndicatorSystem`, `MVVM`, `Subsystem`, `Subtitle` 폴더로 나뉜다.
- `UWxUIManagerSubsystem`(게임 인스턴스 서브시스템)이 로컬 플레이어의 PC가 정해질 때마다 `UWxPrimaryGameLayout`을 새로 만들어 화면에 붙인다. 레이아웃·확인 팝업 클래스는 `UWxUIDeveloperSettings`(`Config/DefaultGame.ini`)의 `WBP_PrimaryGameLayout`·`WBP_ConfirmationPopup`이다.
- 레이아웃은 아래부터 `UI.Layer.Game`·`GameMenu`·`Menu`·`Modal` 순으로 `UCommonActivatableWidgetStack`을 하나씩 두고, 전환 시간은 0이다.
- 모든 화면은 `UWxUIManagerSubsystem::PushWidgetInstanceToLayer`를 거쳐 스택에 들어간다. 코드와 BP는 클래스를 비동기 로드한 뒤 push하는 `UWxAsyncAction_PushWidgetToLayer`를 쓰고, 로드 중 레이아웃이 바뀌면 옛 요청은 버려진다.
- 레이아웃을 채우는 화면은 로컬 PC의 컴포넌트 `UWxPlayerLayoutComponent`가 띄운다.
  - HUD(`UWxHUDLayout`)는 빙의가 바뀔 때마다 걷고 Game 레이어에 다시 push한다. 그래서 HUD의 뷰모델 리졸버가 새 폰을 기준으로 다시 돈다.
  - 폰 ASC에 `Ability.Death`가 붙으면 사망 화면을 Menu 레이어에, `State.Dialogue`가 붙으면 대화 화면을 Game 레이어 맨 위에 띄운다. 대화 화면은 태그가 걷히면 띄운 컴포넌트가 닫는다.
- HUD 에셋 `WBP_GameHUD`는 09-09에 `WBP_GameLayout`으로 이름이 바뀌었다(e0e3ecc51).
- HUD는 CommonUI 액션 `UI.Action.Inventory`·`UI.Action.MainMenu`로 인벤토리·메인 메뉴를 Menu 레이어에 push한다. 로드가 끝나기 전 같은 입력이 와도 메뉴가 겹쳐 쌓이지 않게 진행 중 요청을 기억한다.
- `UI.Action.FreeCursor`를 누르는 동안 입력 설정을 All·커서 표시·시점 입력 무시로 바꾸고, 떼면 HUD 자신의 희망 설정으로 되돌린다.
- `UWxUILibrary`는 서브시스템·레이아웃 조회, 소유 activatable 닫기(`DeactivateOwningActivatable`), 확인 팝업(`ShowConfirmationPopup`)을 BP에 준다. 프론트엔드의 캐릭터·레벨 선택 버튼과 새 게임 요청은 `UWxFrontEndLibrary`가 준다.

### 입력 모드
- 화면 베이스 `UWxActivatableWidget`은 `InputMode`(기본 Game)로 희망 입력 설정을 낸다. Game은 `CapturePermanently`, Menu는 `NoCapture`다.
- HUD의 메뉴 토글 바인딩은 `InputMode=Game`이다. 메뉴가 열려 Menu 모드가 되면 매칭되지 않아 메뉴가 겹쳐 열리지 않고, 닫기는 메뉴 위젯의 뒤로 가기 처리가 맡는다.

### 게임 일시정지
- `UWxUIManagerSubsystem::RefreshGamePause`가 정지를 정한다. 네 레이어의 활성 위젯 중 `ShouldPauseGame()`이 참인 것이 하나라도 있으면 추적 중인 로컬 PC에 `SetPause(true, FCanUnpause)`, 없으면 `SetPause(false)`를 건다. 스탠드얼론에서만 건다.
- 재평가 시점은 서브시스템이 push한 위젯의 `OnActivated`·`OnDeactivated`를 구독해 잡는다.
- UI는 `HandleCanUnpause`(정지를 원하는 활성 위젯이 없음)를 대리자로 걸어 엔진의 해제 규약에 참여한다. 그래서 남의 해제는 UI의 정지를 지우지 못하고, 대리자를 건 다른 정지도 UI의 해제에 지워지지 않는다.
- `UWxActivatableWidget::bPauseGame`은 정지를 원한다는 데이터일 뿐이고, 위젯은 정지를 직접 걸지 않으며 서브시스템도 모른다. `UWxGamePopup`은 생성자에서 켜고, 정지하는 메뉴는 WBP에서 켠다.
- 알고 남긴 한계: `PC->SetPause`는 이미 정지 중이면 정지 주체를 더하지 않아, 남이 먼저 건 정지 위에서 UI가 열리면 그쪽이 풀 때 함께 풀린다.

### 팝업
- 모달은 `UWxGamePopup`(추상 베이스, Menu 입력·정지), `UWxConfirmationPopup`(확인·거절·취소 고정 버튼 중 서술자가 고른 것만 표시), 서술자 `UWxGamePopupDescriptor`, 결과 `EWxPopupResult`, 버튼 구성 `EWxPopupButtonLayout`으로 이뤄진다.
- `UWxUIManagerSubsystem::ShowConfirmation`이 Modal 레이어에 띄운다. 클래스는 서브시스템 초기화에서 동기 로드해 둔다.
- 결과 콜백은 사용자가 고를 때 한 번만 온다. 띄우지 못하면 `Killed`로 끝나고, 표시된 팝업을 밖에서 닫으면 결과가 오지 않는다.

### 월드에 붙는 위젯
- 스크린 공간 `UWidgetComponent`로 그리는 표시는 적 네임플레이트·락온 레티클([네임플레이트](네임플레이트.md)), [플레이어 캐릭터](플레이어-캐릭터.md)의 스태미나 바(`AWxPlayerCharacter::StaminaWidget`, 로컬 조종일 때만 보임), 인디케이터(`AWxIndicator`), 데미지 플로터(`UWxCueNotify_DamageFloater`가 띄우는 `AWxDamageFloaterActor`)다.
- 인디케이터는 StateTree 태스크 `FWxStateTreeTask_MarkIndicator`가 띄우는 독립 액터이고 복제하지 않는다.
  - 대상이 로드돼 있는 동안만 부착해 따라가고, 언로드·파괴된 동안은 기록 좌표를 가리킨다.
  - 화면 밖이나 카메라 뒤 대상은 여백을 둔 화면 가장자리로 당기고, 당긴 화면 좌표를 역투영한 월드 지점으로 액터를 옮겨 그린다.
  - 표시 내용은 태스크가 넘긴 위젯(`IWxIndicatorWidget` 구현)이 가진다.
  - 스크린 공간 위젯은 레이어 스택이 아니라 뷰포트에 붙어 메뉴가 덮지 못하므로, Menu·Modal 레이어가 떠 있으면(`IsMenuLayerActive`) 숨는다.

### 엔진 동작에 기댄 곳
- CommonUI 액션 바인딩의 `InputMode`는 포함이 아니라 정확 매칭이다. 현재 입력 모드가 All이거나 바인딩 모드와 같을 때만 발동한다(`FActionRouterBindingCollection::ProcessNormalInput`).
  - 그래서 `InputMode=All` 바인딩은 All 모드에서만 돈다.
  - 아무 위젯도 입력 설정을 걸지 않은 시작 직후는 사실상 All이라 모든 바인딩이 매칭되어, '첫 입력만 되고 메뉴를 한 번 거치면 안 되는' 현상이 난다.
  - FreeCursor를 누르는 동안은 All 모드라 Released 바인딩이 매칭된다.
- CommonUI는 `bSupportsActivationFocus`가 꺼진 위젯을 leafmost 활성 노드로 보지 않아, 그 위젯의 `GetDesiredInputConfig`를 로그 없이 적용하지 않는다. HUD의 Game 입력 설정은 이 값이 켜져 있어야 걸린다.
- 레이어 스택에 루트 콘텐츠가 없어, 위젯이 하나뿐인 스택에서 그것이 비활성화돼도 스택 이벤트(`OnDisplayedWidgetChanged`)가 나지 않는다. 일시정지가 위젯 자신의 `OnActivated`·`OnDeactivated`를 구독하는 이유다. `GetActiveWidget`도 비활성화된 위젯을 돌려줄 수 있어 `IsActivated()`로 거른다.
- 게임모드의 해제는 등록된 정지 주체의 `FCanUnpause`에 되물어 동의한 것과 대리자가 없는 것만 걷는다. 새 정지 주체는 자기 대리자를 걸어야 남의 해제에 풀리지 않는다.
- WBP 그래프의 입력 노드는 엔진이 소유 PC의 입력 컴포넌트 클래스로 위젯 전용 입력 컴포넌트를 만들어 Construct~Destruct 동안만 PC 입력 스택에 올린다. 위젯에서 Enhanced Input을 쓰려고 따로 만들 객체가 없다.
- 스크린 공간 위젯 컴포넌트는 보일 때만 화면에 붙고, 붙어야 위젯이 Construct된다. 그래서 원격 폰의 위젯 컴포넌트를 숨겨 두면 그 위젯의 입력도 스택에 오르지 않는다.
- 스크린 공간 위젯 컴포넌트를 화면에서 떼는 일은 컴포넌트 틱(`UpdateWidgetOnScreen`)에서만 일어난다.
  - `SetVisibility(false)`와 함께 틱을 끄거나 `bStartWithTickEnabled=false`로 두면 화면에 영영 남는다. 액터가 살아남는 경우(시체·비활성)에만 드러난다.
  - 틱 비용을 줄이려면 틱을 끄지 말고 틱 본문 앞에 `IsVisible()` 게이트를 둔다.
  - 인디케이터가 정지 중에도 위젯 컴포넌트 틱을 켜 두는 이유다.

## 결정
- 2026-06-11 메뉴 입력을 CommonUI 액션으로 옮겼다. 게임 중 상주하는 HUD의 토글 바인딩은 Game 모드에서 매칭되도록 `InputMode=Game`으로 둔다. (커밋 3f597c031)
- 2026-07-11 모달 UI 이름을 Lyra의 Dialog·Screen·Messaging 혼용 대신 `Popup`으로 통일했다. Lyra의 Screen과 Dialog가 겹치고 대사(Dialogue)와 헷갈려서다. (사용자 결정, 커밋 f2ba53ae0)
  - `Popup`은 모달 전용이다. `Screen`은 전체화면 비모달 뷰(설정·로딩 등)에 남기고, 토스트·알림은 다른 이름(Toast·Notification)을 준다.
  - 새 모달 위젯은 `Popup` 접미사를 쓰고, 결과·프로토콜 용어도 `Popup`으로 한다(Messaging을 쓰지 않는다). Lyra 코드를 옮길 때도 이름은 바꾼다.
  - Dialog와 Popup 사이 개명이 세 번 왕복한 끝의 결론이라 되돌리지 않는다.
  - 에러 팝업(`ShowErrorPopup`·`ErrorPopupClass`)은 07-29에 지웠다. (커밋 f50526315)
- 2026-07-12 위젯이 정지를 직접 걸던 방식을 서브시스템이 매번 활성 위젯을 재평가하는 방식으로 바꿨다. 메뉴와 팝업이 서로 다른 레이어에서 함께 정지를 걸 때 하나를 닫으면 풀리던 조기 해제를 막는다. 누적 상태가 없어 조기 해제와 영구 정지가 원천적으로 생기지 않는다. (사용자 요구, 커밋 6d5b32a15)
  - 위젯은 `bPauseGame` 데이터만 갖고 정지 로직을 건드리지 않는다.
  - 새 화면은 반드시 `UWxUIManagerSubsystem` 경로로 push한다. 거치지 않은 위젯은 정지 구독에서 빠진다.
- 2026-09-03 UI 정지 해제를 엔진 `FCanUnpause` 대리자 규약에 참여시켰다. 게임모드에 정지 주체를 직접 거는 우회는 재평가마다 주체가 쌓여 기각했다. (커밋 1778b4542)
- 2026-08-03 HUD 요소는 WBP에 배치하고 뷰모델 필드에 View Binding으로 잇는다. 위젯 C++ 클래스는 새로 만들지 않는다. (사용자 결정 "위젯 C++ 클래스를 추가하고 싶진 않습니다, 제가 뷰바인딩 할 수 있게만 해주세요", 커밋 4e53cce0b)
  - 코드가 위젯을 만들어 붙이면 풀링·비동기 클래스 로드·수명 관리가 따라붙고, 배치형이면 저작이 에디터에서 끝난다.
  - 새 위젯 클래스가 필요해 보이면 먼저 'VM 필드 + 스톡 위젯 바인딩'으로 되는지 따진다.
  - `UWxHUDLayout::RebuildWidget`으로 HUD 트리를 감싸 요소를 까는 방식은 다시 넣지 않는다.
  - 월드 대상을 따라가는 표시는 그 뒤 스크린 공간 위젯 컴포넌트로 옮겼다. 스태미나 바는 08-12에 플레이어 캐릭터로(4bdddcd58), 인디케이터는 09-02에 HUD 슬롯·매니저·등록증을 걷고 독립 액터로(cc116b621) 갔다. 인디케이터를 대상 액터의 컴포넌트로 두지 않은 것은 대상이 언로드된 동안에도 기록 좌표를 가리켜야 해서다.
- 2026-09-30 로컬 플레이어는 하나로 전제하고 화면 분할은 대비하지 않는다. 서브시스템의 레이아웃·추적 PC가 단수다. (사용자 결정 "화면 분할은 대비 안해도 되요")

## 미결
- 07-30에 Z키 아이템 사용이 안 되고 NPC 대화 뒤 포커스를 잃는 문제의 원인을 HUD 에셋(당시 `WBP_GameHUD`)의 Supports Activation Focus가 꺼진 것으로 진단했다. HUD가 leafmost 활성 노드가 되지 못해 Game 입력 설정이 한 번도 적용되지 않고, 대화 창이 건 Menu 설정이 남는다. 그 뒤 에셋을 고쳤는지는 확인하지 못했다.

## 관련
- [UI 뷰모델](UI-뷰모델.md)
- [게임 프레임워크 구조](게임-프레임워크-구조.md) — 컨트롤러 컴포넌트 구성
- [네임플레이트](네임플레이트.md)
- [초반 구간과 퀘스트](초반-구간과-퀘스트.md)

## 출처
- 사용자 대화로 정한 지난 결정: Claude 메모리 기록에서 옮기고 HEAD 2a3baca6a 코드로 확인 (2026-10-06 조회)
- `.claude/worklog/2026-09-02-인디케이터-위젯컴포넌트-전환.md` (cc116b621, 09-20 삭제)
- `Source/WxGame/UI/Subsystem/WxUIManagerSubsystem.cpp` (7960789cf)
- `Source/WxGame/UI/WxActivatableWidget.h` (d37e1dd32)
- `Source/WxGame/UI/WxPrimaryGameLayout.cpp` (d37e1dd32)
- `Source/WxGame/UI/WxHUDLayout.cpp` (d37e1dd32)
- `Source/WxGame/UI/WxPlayerLayoutComponent.cpp` (2e937bd57)
- `Source/WxGame/UI/WxUILibrary.cpp` (d37e1dd32)
- `Source/WxGame/UI/Foundation/` (7f8e2731d)
- `Source/WxGame/UI/Frontend/` (e1b01ba2c)
- `Source/WxGame/UI/IndicatorSystem/` (e28fef4b2)
- `Source/WxGame/Character/WxPlayerCharacter.cpp` (e1b01ba2c)
- `Source/WxGame/AbilitySystem/Cues/WxCueNotify_DamageFloater.cpp` (d37e1dd32)
- `Source/WxGame/WxGameplayTags.cpp` (8b331baec)
- `Config/DefaultGame.ini` (d37e1dd32)
