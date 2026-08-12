# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel) 뷰모델을 축으로, 게임 전역의 화면·HUD·팝업 배치와 도메인 데이터의 표시 계약을 담당하는 도메인 플러그인. WBP가 본체이며 C++는 레이어 관리·뷰모델·확장 베이스를 제공한다.

## 책임
**담당**
- 레이어 기반 화면 스택 관리: `UWxUIManagerSubsystem`이 PlayerController/Pawn 생명주기를 관찰해 HUD·사망·대화 화면을 정해진 레이어에 push하고, 활성 위젯 상태로 게임 정지를 재평가한다.
- MVVM 뷰모델 계층: ASC(어트리뷰트/어빌리티/이펙트)·캐릭터·선택·상호작용·자막 등 도메인 데이터를 UMG가 바인딩할 평면 표시 필드로 노출한다. 소프트 이미지 참조의 비동기 스트리밍을 베이스에서 공통 제공한다.
- 위젯 베이스 클래스: `UWxActivatableWidget`(입력 모드·정지 옵션), HUD 루트, 탭/버튼/팝업 공통 위젯.
- 월드 대상 표시 부착물: 네임플레이트(WidgetComponent 확장)·화면 인디케이터(컨트롤러 컴포넌트 + 투영).
- BP/BFL 진입점: 레이어 push, 확인 팝업, MVVM 컨버전 함수, 비동기 위젯 push.

**경계 (비담당)**
- 구체 캐릭터 타입·게임플레이 로직은 알지 못한다 — 표시 데이터(`FWxCharacterUIData`)는 소비 측(게임 모듈·도메인 플러그인)이 주입한다.
- 상호작용/인벤토리 등 도메인 소스가 선택 뷰모델에 값을 push하며, 그 도메인 로직은 각 시스템 소관.
- 공용 정의·기반 타입은 [[WxCore]]에 위임.

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 의존). 엔진 서브시스템: CommonUI/CommonInput, UMG, ModelViewViewModel, GameplayAbilities, StateTree, ModularGameplay. (에디터 전용: EnhancedInput·AssetRegistry — 디자인타임 EI 아이콘 프리뷰용)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs·uplugin·include 모두 Wx 의존은 WxCore뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이어 push·PC/Pawn 관찰·화면 전환·게임 정지 재평가의 허브 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 `CommonActivatableWidgetStack`을 보유하는 루트 레이아웃. z-order = 배열 순서 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·HUD·사망·대화 화면 클래스를 Config로 지정하는 프로젝트 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxActivatableWidget` | 모든 화면 위젯의 베이스(입력 모드·`bPauseGame`). HUD/팝업이 이를 상속 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 뷰모델 베이스. 소프트 이미지의 비동기 스트리밍을 `RequestImageAsync`/`ApplyLoadedImage`로 공통화 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출하는 Composite. 바인딩 요청 시 지연 생성 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModel_Selection` | 소스 무관 "현재 선택 하나"의 범용 표시 VM. 도메인 브리지가 push | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Selection.h` |
| `UWxUILibrary` | BFL 진입점(레이어 push·확인 팝업·닫기 유틸) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- **새 화면 위젯**: `UWxActivatableWidget`을 WBP로 상속하고, `UWxUIDeveloperSettings` 또는 `UWxHUDLayout`의 소프트 클래스 슬롯에 지정. 코드가 아니라 설정/레이아웃 슬롯으로 연결한다.
- **레이어 push**: BP는 `UWxUILibrary::PushSoftContentToLayer` 또는 `UWxAsyncAction_PushWidgetToLayer`(로드 지연 대비), C++는 `UWxUIManagerSubsystem`의 push 계열. 레이어 태그는 `UI.Layer.*` 계층.
- **새 뷰모델**: `UWxViewModel`을 상속하고 `FieldNotify` 프로퍼티를 노출. 이미지 필드는 소프트 참조를 넘겨 `ApplyLoadedImage`로 수신 — WBP는 일반 Image의 `SetBrushResourceObject`에 바인딩.
- **MVVM 컨버전**: 어트리뷰트/어빌리티 자식 VM은 `UWxMVVMConversionLibrary`의 Get...ViewModel 함수로 중첩 바인딩 시 지연 생성한다.
- **월드 표시 부착**: 네임플레이트는 오너 액터가 서브오브젝트로 생성 후 `InitializeViewModels(ASC, UIData)` 호출. 인디케이터 매니저는 Experience 에셋의 컴포넌트 주입 목록으로 컨트롤러에 부착(코드가 클래스를 모름).

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면 생명주기·레이어·정지의 중심. 모듈 흐름의 출발점.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조와 push 메커니즘.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_Character.h`/`WxViewModel_AbilitySystem.h` — 뷰모델 베이스와 Composite 데이터 주입 규약.
4. `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` — BP가 실제로 이 모듈을 호출하는 표면.

## 관련
- 상위: [[WxCore]] (유일한 Wx 의존). 표시 데이터를 주입·소비하는 도메인은 [[WxCombat]]·[[WxInventory]]·[[WxDialogue]] 등 게임 모듈 측.

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 64파일 — `/readme-writer`로 갱신*
