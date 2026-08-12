# WxUI — UI 시스템

> CommonUI 레이어 스택 위에 게임 UI를 세우고, MVVM(ModelViewViewModel) 뷰모델로 게임플레이 상태를 표시 데이터로 옮기는 도메인 플러그인. 화면 인디케이터·네임플레이트·팝업·자막 등 표시 계열 공통 인프라를 함께 제공한다.

## 책임
**담당**
- 레이어 기반 위젯 스택 관리: `UWxPrimaryGameLayout`(레이어별 `UCommonActivatableWidgetStack`)과 이를 push/pop 하는 `UWxUIManagerSubsystem`.
- 로컬 플레이어 생명주기 연동: LocalPlayer/PlayerController/빙의 폰 교체를 추적해 HUD를 세우고, ASC 상태 태그(사망·대화)로 화면을 띄우고 걷는다.
- MVVM 뷰모델 계층: 이미지 비동기 스트리밍 베이스(`UWxViewModel`)와 캐릭터/어트리뷰트/이펙트/어빌리티/선택/상호작용/자막/인디케이터용 파생 VM.
- 표시용 컴포넌트: 화면 인디케이터(`UWxIndicatorManagerComponent`), 월드 네임플레이트(`UWxNameplateComponent`).
- 공통 위젯 베이스: ActivatableWidget/버튼/탭/HUD 레이아웃/확인 팝업, 그리고 BP 진입점(`UWxUILibrary`)과 비동기 push 액션.
- 소비 도메인이 UI 모듈을 참조하지 않고 쓸 수 있도록 인디케이터·자막을 켜고 끄는 StateTree 태스크 노드 제공.

**경계 (비담당)**
- 구체 캐릭터/아이템/상호작용 타입을 알지 않는다 — 표시 데이터(`FWxCharacterUIData`, 선택 필드 등)는 소비 측이 주입/push 한다.
- WBP 위젯 계층·바인딩 그래프 등 에셋 내부 구조(콘텐츠).

## 의존성
- **주요 의존**: `WxCore`. 엔진 서브시스템으로 `CommonUI`/`CommonInput`, `ModelViewViewModel`(UMG MVVM), `GameplayAbilities`(ASC 상태·어트리뷰트 소스), `StateTree`, `ModularGameplay`, `UMG`, `UniversalObjectLocator`.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이어 push/팝업/게임 정지 재평가, 플레이어·폰·상태 태그 추적 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어별 위젯 스택 루트. z-order는 `LayerTags` 배열 순서 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입점(레이어 push, 팝업 표시, ActivatableWidget 조작) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxViewModel` | 이미지(텍스처/머터리얼) 비동기 스트리밍을 제공하는 VM 베이스 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | ASC + 주입 표시 데이터를 묶는 Composite VM(AbilitySystem 자식 VM 중첩) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_Selection` | 소스 무관 "현재 선택" 글로벌 VM. 글로벌 컬렉션에 등록 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Selection.h` |
| `UWxIndicatorManagerComponent` | 로컬 인디케이터 목록 보유·매 틱 화면 좌표 투영(ControllerComponent) | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUIDeveloperSettings` | 레이아웃/팝업/HUD/사망·대화 화면 클래스의 Config 지정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- **새 화면/패널**: `UWxActivatableWidget`(또는 `UWxGamePopup`)을 상속한 WBP를 만들고, `UWxUILibrary`/`UWxUIManagerSubsystem`으로 `UI.Layer.*` 태그가 가리키는 레이어에 push. `InputMode`/`bPauseGame`으로 입력·정지 의사를 선언하면 서브시스템이 전 레이어를 재평가해 실제 정지를 결정한다.
- **새 표시 데이터**: `UWxViewModel`(또는 알맞은 파생)을 상속해 `FieldNotify` 프로퍼티를 노출하고, 소프트 이미지는 `RequestImageAsync`/`ApplyLoadedImage` 규약을 따른다(VM은 로드된 하드 참조만 UMG에 노출 → 일반 Image의 `SetBrushResourceObject`에 바인딩).
- **캐릭터 표시**: `UWxNameplateComponent`를 오너 생성자에서 서브오브젝트로 만들고 BeginPlay 이후 `InitializeViewModels(ASC, FWxCharacterUIData)` 호출. 표시 조건은 `VisibilityRequirements`(ASC 태그)로 저작한다.
- **소비 도메인에서 인디케이터/자막 켜기**: UI 모듈을 참조하지 않고 StateTree 에셋에서 `MarkIndicators` 등 `WxIndicator/WxSubtitle` StateTree 태스크 노드를 골라 쓴다(대상은 `FUniversalObjectLocator`로 지정).
- **팝업**: `UWxGamePopupDescriptor::CreateConfirmation*` 팩토리로 서술자를 만들고 `ShowConfirmation`/`ShowConfirmationPopup`으로 Modal 레이어에 표시, 결과는 `FWxPopupResultDelegate`로 수신.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 이 모듈의 제어 흐름 중심. 레이어 push, 플레이어·폰·상태 태그 추적, 정지 재평가가 여기 모인다.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어/스택 구조와 z-order 규약.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — MVVM 베이스와 이미지 스트리밍 규약(모든 파생 VM의 공통 계약).
4. `Plugins/WxUI/Source/WxUI/Public/Indicator/WxStateTreeTask_MarkIndicators.h` — 소비 도메인이 역참조 없이 UI를 켜는 패턴의 대표 예.

## 관련
- 상위: [[WxCore]] (공용 정의)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 64파일 — `/readme-writer`로 갱신*
