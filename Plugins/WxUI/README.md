# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 기반으로 화면 UI를 조립하는 도메인 플러그인. C++는 레이어/팝업/HUD 오케스트레이션, ASC 기반 뷰모델, 화면 인디케이터·네임플레이트 같은 재사용 인프라를 제공하고, 실제 위젯 외형은 WBP가 맡는다.

## 책임
**담당**
- CommonUI 레이어 스택(`UWxPrimaryGameLayout`)과 그 위 push/팝업/게임 정지 재평가를 관장하는 `UWxUIManagerSubsystem`.
- MVVM 뷰모델 계층: 비동기 이미지 스트리밍 베이스(`UWxViewModel`)와 ASC(어트리뷰트/어빌리티/이펙트/OwnedTags)·캐릭터·선택·상호작용 표시용 파생 VM, UMG 바인딩용 컨버전 라이브러리.
- 화면 좌표 인디케이터 인프라(등록증 발급 매니저 + Slate 캔버스 + StateTree 노드)와 월드 공간 네임플레이트 컴포넌트.
- BP 파사드(`UWxUILibrary`), 확인 팝업, HUD 레이아웃, Tab/Button/Action 등 CommonUI 위젯 베이스.

**경계 (비담당)**
- 표시할 도메인 데이터의 진실은 각 도메인 소유 — VM은 소비 측이 push/inject하는 평면 표시 계약만 노출한다. 어트리뷰트/어빌리티/이펙트 원본은 [[WxCombat]] 계열 ASC, 선택 소스는 [[WxWorld]]·[[WxInventory]] 등.
- 구체 위젯 외형·레이아웃·바인딩 배선은 WBP/BP 애셋(코드 밖).
- 실제 화면 문자열/레이어 태그의 게임 정의는 소비 측 Experience·config가 주입.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존, 예: `FWxActorTarget`). 엔진 서브시스템으로 CommonUI/CommonInput, ModelViewViewModel, GameplayAbilities, StateTree, ModularGameplay(`UControllerComponent`), UMG. 에디터 전용으로 AssetRegistry·EnhancedInput(디자인타임 아이콘 프리뷰).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이어 push·확인 팝업·HUD/사망화면 생성·게임 정지 재평가의 중앙 오케스트레이터 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 `UCommonActivatableWidgetStack`을 담는 루트 위젯. z-order = 태그 배열 순서 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxViewModel` | MVVM 베이스. 텍스처/머터리얼 소프트 참조를 필드별로 비동기 스트리밍해 하드 참조로 노출 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트/OwnedTags 자식 VM으로 노출하는 Composite(요청 시 지연 생성) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 BP 컨버전(태그→Visibility, 어트리뷰트/어빌리티/이펙트 VM 조회) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxUILibrary` | UI 매니저·레이어 제어·확인 팝업의 BP 파사드 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxIndicatorManagerComponent` | 로컬 PC에 매달린 화면 인디케이터 등록증 목록. 캔버스가 구독해 그림 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxNameplateComponent` | ASC 태그 조건으로 표시되고 카메라 거리로 스케일되는 월드 네임플레이트 위젯 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h` |
| `UWxActivatableWidget` | CommonUI 액티버터블 위젯 베이스(입력 모드·게임 정지 의사) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |

## 확장 포인트 / 규약
- **레이어 push**: 동기 로드는 `UWxUILibrary::PushSoftContentToLayer`, 로드 지연이 문제면 `UWxAsyncAction_PushWidgetToLayer`(BP async). 레이어 지정은 `UI.Layer.*` 게임플레이 태그(코드에서 native 선언은 없음 — config/애셋이 정의).
- **새 뷰모델**: `UWxViewModel`을 상속하고 이미지 슬롯이 있으면 `RequestImageAsync`/`ApplyLoadedImage(FieldName,...)`를 쓴다. 도메인 타입 참조 없이 평면 표시 필드만 노출하고, 값은 소비 측이 push/inject한다. UMG는 소프트 참조가 아닌 로드된 하드 참조를 일반 `Image`의 `SetBrushResourceObject`에 바인딩.
- **캐릭터/네임플레이트**: 소비 측이 `FWxCharacterUIData`+ASC를 `InitializeViewModels`/`Initialize`로 주입 — WxUI는 구체 캐릭터 타입을 모른다.
- **게임 정지**: `UWxActivatableWidget::bPauseGame`을 켜면 매니저가 전 레이어를 재평가해 정지 결정(위젯은 서브시스템을 모른다).
- **화면 인디케이터**: `UWxIndicatorManagerComponent::AddIndicator`로 등록증을 받고 해제 시 반납. StateTree로 쓰려면 `FWxStateTreeTask_MarkIndicator` 노드를 애셋에서 선택(소비 도메인이 UI 모듈을 참조하지 않아도 됨).
- **화면 클래스 주입**: 레이아웃/확인 팝업/HUD/사망·대화 화면은 `UWxUIDeveloperSettings`의 소프트 클래스 슬롯(config)으로 연결 — 코드가 아니라 설정으로 배선.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이어/팝업/HUD/사망화면/정지의 진입점이자 수명 흐름의 중심.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` + `WxViewModel_AbilitySystem.h` — VM 계층의 이미지 스트리밍 규약과 ASC 지연 생성 패턴.
3. `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorStateTreeNodes.h` — 인디케이터 등록증·캔버스·StateTree 노드가 맞물리는 방식(소유·해제 규약 포함).

## 관련
- 상위: [[WxGame]] / GameFeature 콘텐츠 플러그인(Experience 애셋이 위젯·컴포넌트를 주입)
- 데이터 소스: [[WxCombat]](ASC) · [[WxWorld]] · [[WxInventory]] · [[WxDialogue]](표시할 도메인 진실)

---
*문서 기준 커밋 `59acb24` · 생성일 2026-07-30 · 소스 60파일 — `/readme-writer`로 갱신*
