# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 뼈대로 한 게임 UI 프레임워크. 위젯 베이스 클래스, 레이어/팝업 관리 서브시스템, ASC·캐릭터·상호작용을 UMG에 노출하는 뷰모델 계층을 제공한다.

## 책임
**담당**
- 레이어드 UI 스택 관리: `UWxPrimaryGameLayout`(레이어 태그별 위젯 스택)와 이를 플레이어별로 생성·구동하는 `UWxUIManagerSubsystem`.
- 위젯 베이스 클래스군: Activatable/HUD/팝업/버튼/탭/액션/LazyImage 등 CommonUI 파생 공통 위젯.
- MVVM 뷰모델 계층: ASC(어트리뷰트/어빌리티/이펙트/OwnedTags), 캐릭터 표시 데이터, 선택 대상, 상호작용을 UMG 바인딩용 도메인-무관 표시 계약으로 노출.
- 확인/에러 팝업 파이프라인과 BP 파사드(`UWxUILibrary`), MVVM 바인딩 컨버전 함수(`UWxMVVMConversionLibrary`).
- 월드 네임플레이트 컴포넌트, GameplayEffect UI 데이터 컴포넌트.

**경계 (비담당)**
- 전투/어트리뷰트 정의·계산은 [[WxCombat]] (WxUI는 ASC를 표시만 함).
- 인벤토리 데이터·아이템 뷰모델은 [[WxInventory]].
- 상호작용 판정·대상 탐지는 [[WxWorld]] (VM은 push된 표시값만 보유).
- 구체 캐릭터 타입·표시 데이터 저작은 소비 측(게임 모듈)이 `FWxCharacterUIData`로 주입.

## 의존성
- **주요 의존**: `WxCore`, `CommonUI` / `CommonInput`, `ModelViewViewModel`(MVVM), `GameplayAbilities`(ASC 관찰), `UMG`, `DeveloperSettings`. 에디터 전용으로 `EnhancedInput`·`AssetRegistry`(`WxActionWidget` 디자인타임 아이콘 프리뷰).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore`만 참조).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 플레이어별 레이아웃 생성, 레이어 push, 팝업, 게임 정지 재평가, 공유 Selection VM 소유 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 `UCommonActivatableWidgetStack` 컨테이너. 배열 순서가 z-order | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 모든 화면 위젯 베이스. 입력 모드/게임 정지 의향(`ShouldPauseGame`) 노출 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDLayout` | 상시 활성 HUD 루트. 메뉴 토글 입력을 Menu 레이어 push로 연결 | `Source/WxUI/Public/Widget/WxHUDLayout.h` |
| `UWxGamePopup` / `UWxGamePopupDescriptor` | 팝업 위젯 베이스 + 헤더/본문/버튼 서술자, 결과 델리게이트 | `Source/WxUI/Public/Widget/WxGamePopup.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트/OwnedTags 자식 VM으로 노출(지연 생성) | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModel_Character` | 캐릭터 Composite VM. AbilitySystem 자식 VM + 이름/초상화 | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxUILibrary` | BP 파사드. 서브시스템/레이아웃 접근, 레이어 제어, 팝업 표시 | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 컨버전(태그→Visibility, 어트리뷰트/어빌리티 VM 조회) | `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |

## 확장 포인트 / 규약
- **새 화면 위젯**: `UWxActivatableWidget`(Abstract) 상속 → WBP로 저작. HUD는 `UWxHUDLayout`, 팝업은 `UWxGamePopup` 파생. 게임 정지가 필요하면 `bPauseGame`을 켜면 서브시스템이 전 레이어를 재평가해 적용한다(위젯은 서브시스템을 모른다).
- **새 뷰모델**: `UWxViewModel`(Abstract, `UMVVMViewModelBase` 파생) 상속. 값은 외부 소스가 push하고 VM은 도메인 타입을 참조하지 않는 평면 표시 필드만 노출(`UWxViewModel_Selection` 참조). `FieldNotify` UPROPERTY로 바인딩을 통지한다.
- **레이어 추가**: `UWxPrimaryGameLayout::LayerTags`(BP 디폴트, `UI.Layer.*` 태그)에 추가. 레이어/액션 태그는 이 모듈이 선언하지 않고 프로젝트 태그 소스에서 온다.
- **레이아웃/팝업 클래스 지정**: `UWxUIDeveloperSettings`(Project Settings)에서 `LayoutClass`·`ConfirmationPopupClass`·`ErrorPopupClass`를 소프트 클래스로 설정.
- **네임플레이트**: 오너 액터 생성자에서 `UWxNameplateComponent` 서브오브젝트 생성 → BeginPlay 이후 `InitializeViewModels(ASC, UIData)`.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 모듈의 런타임 오케스트레이터. 레이어·팝업·정지·공유 VM의 진입점.
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조와 push API. UI 배치의 뼈대.
3. `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — ASC를 UMG로 잇는 MVVM 계층의 중심. 지연 생성·자식 VM 패턴이 여기에 있다.
4. `Source/WxUI/Public/Widget/WxActivatableWidget.h` — 모든 화면 위젯의 공통 계약(입력 모드/정지 의향).

## 관련
- 상위: [[WxGame]] (레이아웃/팝업 클래스 저작·주입), [[WxCore]] (공용 정의)
- 데이터 소스: [[WxCombat]], [[WxInventory]], [[WxWorld]]

---
*문서 기준 커밋 `c275320` · 생성일 2026-07-24 · 소스 53파일 — `/readme-writer`로 갱신*
