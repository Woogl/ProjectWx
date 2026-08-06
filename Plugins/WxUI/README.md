# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델 계층을 얹어, 도메인 상태를 화면 표시로 바꾸는 UI 인프라 모듈. C++은 골격(레이어·뷰모델·표시 컴포넌트·위젯 베이스)만 제공하고 실제 화면은 BP/WBP가 채운다.

## 책임
**담당**
- **레이어/화면 인프라**: `UWxPrimaryGameLayout`(태그별 위젯 스택)과 `UWxUIManagerSubsystem`(LocalPlayer 부착·레이어 push·게임 정지 재평가·사망/대화 화면 반응). 확인 팝업(`UWxGamePopup*`)과 BP 파사드(`UWxUILibrary`).
- **MVVM 뷰모델 계층**: 표시용 데이터 계약을 노출하는 `UWxViewModel` 파생 일습(ASC/어트리뷰트/어빌리티/이펙트/캐릭터, 그리고 자막·인디케이터·선택·상호작용). 이미지 비동기 스트리밍은 베이스가 공통 제공.
- **월드 부착 표시**: 화면 인디케이터(`UWxIndicatorManagerComponent`+`UWxIndicatorDescriptor`), 네임플레이트(`UWxNameplateComponent`), 화면 자막(`UWxViewModel_Subtitle`). 인디케이터·자막은 StateTree 노드까지 함께 제공해 소비 도메인이 에셋에서 골라 쓴다.
- **공용 위젯 베이스**: `UWxActivatableWidget`, `UWxButtonBase`/`UWxTabButtonBase`, `UWxActionWidget`, `UWxHUDLayout`.

**경계 (비담당)**
- 도메인 상태(전투 태그·인벤토리·대화 세션 등)는 소유하지 않는다 — WxUI는 표시 계약만 정의하고, 무엇을·왜 표시할지는 알지 못한다. 구체 캐릭터/선택 대상 타입은 소비 측(게임 모듈 [[WxGame]], 도메인 모듈)이 `FWxCharacterUIData` 등으로 **주입**한다. StateTree 표시 노드의 발주자는 [[WxQuest]] 등 소비 도메인이다.
- ASC·GameplayTag는 엔진 타입으로만 다루며 도메인 플러그인을 참조하지 않는다.

## 의존성
- **주요 의존**: WxCore, CommonUI/CommonInput, ModelViewViewModel(MVVM), GameplayAbilities, StateTree, ModularGameplay, UMG. (에디터 전용으로 AssetRegistry/EnhancedInput — `WxActionWidget`의 디자인타임 아이콘 프리뷰에서만.)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 진입점. 레이어 push·확인 팝업·게임 정지 재평가, 빙의/사망/대화 태그 관찰 후 화면 반응 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | `UI.Layer.*` 태그별 `CommonActivatableWidgetStack` 컨테이너. push의 실제 착지점 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 파사드(레이어 push·팝업). `BlueprintCallable` 진입 지점 | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·HUD·사망/대화 화면 소프트 클래스 설정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxViewModel` | 뷰모델 베이스. 소프트 이미지 비동기 스트리밍 공통 제공(Abstract) | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC 어트리뷰트/어빌리티/이펙트/OwnedTags를 자식 VM으로 노출(Composite) | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModel_Character` | 캐릭터 표시 데이터 + AbilitySystem 자식 VM 묶음 | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 컨버전(태그→Visibility, 어트리뷰트/어빌리티/이펙트 VM Get) | `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxIndicatorManagerComponent` | 화면 인디케이터 목록 보유·매 틱 화면좌표 투영(ControllerComponent) | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxNameplateComponent` | ASC 기반 네임플레이트 VM 초기화·거리 스케일·태그 표시조건(WidgetComponent) | `Source/WxUI/Public/Component/WxNameplateComponent.h` |
| `UWxViewModel_Subtitle` | 글로벌 컬렉션에 하나뿐인 자막 VM + Resolver. 표시와 ST 노드가 같은 인스턴스 공유 | `Source/WxUI/Public/MVVM/WxViewModel_Subtitle.h` |
| `UWxGamePopup` / `UWxGamePopupDescriptor` | 확인 팝업 위젯 베이스와 데이터 서술자 | `Source/WxUI/Public/Widget/WxGamePopup.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 소프트 위젯 클래스 비동기 로드 후 레이어 push(BP Async) | `Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |

## 확장 포인트 / 규약
- **새 화면 위젯**: `UWxActivatableWidget`를 상속(`InputMode`·`bPauseGame` 설정). 팝업은 `UWxGamePopup`/`UWxConfirmationPopup`, HUD 루트는 `UWxHUDLayout` 상속. 버튼은 `UWxButtonBase`/`UWxTabButtonBase`.
- **새 뷰모델**: `UWxViewModel` 상속. 표시 필드는 `FieldNotify` UPROPERTY로, 소프트 이미지는 `RequestImageAsync`+`ApplyLoadedImage` 오버라이드로 노출한다(WBP는 일반 Image의 `SetBrushResourceObject`에 바인딩).
- **레이어 push**: 코드는 `UWxUIManagerSubsystem::Push*ToLayer`, BP는 `UWxUILibrary`/`UWxAsyncAction_PushWidgetToLayer`. 레이어 태그는 `UI.Layer.*`(Layout의 `LayerTags` 배열 순서가 z-order).
- **표시 데이터 주입 규약**: WxUI는 도메인 타입을 모른다 — 소비 측이 `FWxCharacterUIData`(캐릭터), `UWxEffectComponent_UIData`(GE 아이콘) 등으로 표시 데이터를 저작·주입한다.
- **StateTree 표시 노드**: 자막(`FWxStateTreeTask_PrintSubtitle`, `WxSubtitleTableRow` 데이터테이블)·인디케이터 노드를 제공. 소비 도메인은 WxUI를 참조하지 않고 에셋에서 노드를 선택해 쓴다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 모듈의 진입점. 레이어·팝업·게임 정지·화면 반응이 모두 여기서 시작된다.
2. `Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_AbilitySystem.h` — 뷰모델 계층의 베이스와 대표 Composite. 표시 계약이 어떻게 구성되는지 파악.
3. `Source/WxUI/Public/System/WxUIDeveloperSettings.h` — 레이아웃/HUD/팝업/사망·대화 화면이 어떤 소프트 클래스로 연결되는지.

## 관련
- 상위: [[WxGame]](표시 데이터 주입·화면 배치), [[WxCore]](공용 정의)
- 소비 도메인: [[WxQuest]]·[[WxDialogue]] 등(StateTree 표시 노드·대화/자막 트리거) — 단, 역방향 참조는 없다.

---
*문서 기준 커밋 `2fdf0ab` · 생성일 2026-08-06 · 소스 64파일 — `/readme-writer`로 갱신*
