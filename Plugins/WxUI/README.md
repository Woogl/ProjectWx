# WxUI — UI 시스템

> CommonUI 레이어 스택과 ModelViewViewModel(MVVM)을 기반으로 하는 UI 도메인 플러그인. 활성화 위젯 스택·팝업·전역 뷰모델을 관리하고, 게임 상태(ASC 어트리뷰트/어빌리티/이펙트, 캐릭터, 상호작용, 선택)를 View 로 노출하는 C++ 골격을 제공한다. 실제 화면(WBP)은 이 골격에 바인딩한다.

## 책임
**담당**
- CommonUI 레이어 스택 관리: `UWxPrimaryGameLayout`(Game/GameMenu/Menu/Modal 레이어)과 `UWxUIManagerSubsystem`(레이어 push, 로컬플레이어 생명주기, 위젯 활성 상태 기반 게임 정지 재평가).
- 활성화 위젯 베이스 계층: `UWxActivatableWidget`(입력 모드·정지 정책) → `UWxHUDLayout`·`UWxGamePopup`.
- MVVM 뷰모델 계층: `UWxViewModel` 파생 VM 들이 ASC/캐릭터/상호작용/선택 데이터를 순수 표시 계약으로 노출. Blueprint 바인딩용 컨버전(`UWxMVVMConversionLibrary`)·파사드(`UWxUILibrary`) 제공.
- 월드 부착 UI: `UWxNameplateComponent`(WidgetComponent 확장, 거리 스케일·태그 기반 표시).
- UI 클래스 설정: `UWxUIDeveloperSettings`(레이아웃/팝업 클래스 소프트 참조).

**경계 (비담당)**
- ASC·어트리뷰트·어빌리티 정의는 [[WxCombat]]/[[WxCore]] 소유. VM 은 ASC 에서 값을 읽어 표시만 한다.
- 상호작용 대상 레지스트리·선택 소유권은 [[WxWorld]]. WxUI 는 엔진 타입 인자로 값을 받아 표시만 하며(도메인 타입 미참조), 레지스트리 델리게이트 연결은 게임 모듈(WxGame) 리졸버가 수행한다.
- 캐릭터/어빌리티 표시 데이터(이름·초상화·아이콘)의 저작·주입은 소비 측(게임 모듈·BP)이 담당. WxUI 는 데이터 "모양"(`FWxCharacterUIData` 등)만 소유.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존, `WxAbilityComponent` 등), CommonUI/CommonInput(레이어·활성화 위젯·입력 모드), ModelViewViewModel(VM 베이스·전역 컬렉션), GameplayAbilities/GameplayTags(ASC·태그), UMG/Slate.
- 규칙: 위반 없음 ✅ (WxCore 외 Wx 플러그인 참조 없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 중앙 오케스트레이터(GameInstanceSubsystem). 레이어 push·팝업·전역 선택 VM·게임 정지 | `Plugins\WxUI\Source\WxUI\Public\System\WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 스택 루트 위젯(태그→스택 맵). 서브시스템이 생성·소유 | `Plugins\WxUI\Source\WxUI\Public\System\WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 모든 활성화 위젯의 베이스. 입력 모드·게임 정지 정책 | `Plugins\WxUI\Source\WxUI\Public\Widget\WxActivatableWidget.h` |
| `UWxGamePopup` | 팝업 위젯 베이스 + `UWxGamePopupDescriptor`·`EWxPopupResult` | `Plugins\WxUI\Source\WxUI\Public\Widget\WxGamePopup.h` |
| `UWxViewModel` | MVVM 뷰모델 베이스(Abstract). 전 파생 VM 의 뿌리 | `Plugins\WxUI\Source\WxUI\Public\MVVM\WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC Composite VM. 어트리뷰트/어빌리티/이펙트 자식 VM 지연 생성 | `Plugins\WxUI\Source\WxUI\Public\MVVM\WxViewModel_AbilitySystem.h` |
| `UWxUILibrary` / `UWxMVVMConversionLibrary` | BP 파사드(레이어·팝업 제어) / MVVM 바인딩 컨버전 함수 | `Plugins\WxUI\Source\WxUI\Public\WxUILibrary.h` |
| `UWxNameplateComponent` | 월드 부착 네임플레이트(WidgetComponent 확장, ASC VM 바인딩) | `Plugins\WxUI\Source\WxUI\Public\Component\WxNameplateComponent.h` |

## 확장 포인트 / 규약
- **새 화면 위젯**: `UWxActivatableWidget` 을 상속(BP)하고, `UWxUIDeveloperSettings`·`UWxHUDLayout` 의 소프트 클래스 슬롯에 지정하거나 `UWxUIManagerSubsystem::PushContentToLayer`·`UWxUILibrary`·`UWxAsyncAction_PushWidgetToLayer` 로 레이어에 push 한다. 레이어는 `UI.Layer.*` 게임플레이 태그로 식별(네이티브 선언 아님, 문자열 태그).
- **정지 정책**: 위젯이 `bPauseGame` 을 켜면 서브시스템이 전 레이어 활성 위젯을 재평가해 게임 정지를 적용한다. 위젯은 서브시스템을 알지 못한다.
- **새 뷰모델**: `UWxViewModel` 을 상속하고 `Deinitialize()` 를 오버라이드해 구독을 해제한다. 표시 필드는 `FieldNotify` UPROPERTY 로 노출. 전역 공유 VM 은 `UMVVMGameSubsystem` 컬렉션에 등록(예: 선택 VM `"VM_Selection"`).
- **도메인 값 주입 규약**: WxUI 는 도메인 타입을 참조하지 않으므로, VM 은 엔진 타입/평면 표시 필드만 노출하고 값은 외부 소스가 push 한다(`SetSelection`, `Initialize` 등).
- **아이콘/초상화**: `TSoftObjectPtr<UTexture2D>` 를 그대로 노출만 하고 View 측 `UCommonLazyImage` 가 비동기 로드·수명 관리(VM 은 `LoadSynchronous` 미호출).

## 여기서부터 읽어라
1. `Plugins\WxUI\Source\WxUI\Public\System\WxUIManagerSubsystem.h` — UI 진입점. 레이어 push·팝업·정지·전역 VM 이 모두 여기서 시작한다.
2. `Plugins\WxUI\Source\WxUI\Public\System\WxPrimaryGameLayout.h` — 레이어 스택 구조. 위젯이 어디에 쌓이는지 이해의 기준.
3. `Plugins\WxUI\Source\WxUI\Public\MVVM\WxViewModel_AbilitySystem.h` — 게임 상태→UI 노출의 대표 패턴(지연 생성·자식 Composite VM).

## 관련
- 상위: [[WxCore]]
- 데이터 소스: [[WxCombat]], [[WxWorld]]
---
*문서 기준 커밋 `d0c804a` · 생성일 2026-07-12 · 소스 48파일 — `/readme-writer`로 갱신*
