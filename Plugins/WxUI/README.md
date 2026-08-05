# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel) 뷰모델을 축으로 하는 UI 도메인 플러그인. 화면·팝업·HUD의 push/정지/입력을 관리하고, 게임플레이 상태(ASC 태그·상호작용·대화·인디케이터)를 WBP가 바인딩할 평면 표시 데이터로 옮긴다.

## 책임
**담당**
- 레이어 기반 화면 관리: `UWxPrimaryGameLayout`(태그별 CommonActivatableWidget 스택)과 `UWxUIManagerSubsystem`(push/확인 팝업/게임 정지 재평가/HUD·사망·대화 화면 자동 전환).
- MVVM 뷰모델 계층: 표시 필드만 노출하는 `UWxViewModel` 파생들(캐릭터/어빌리티/어트리뷰트/이펙트/상호작용/선택/자막/인디케이터)과 이미지 비동기 스트리밍 공통 처리.
- 화면 좌표 인디케이터·화면 자막·네임플레이트: `UWxIndicatorManagerComponent`(로컬 PC 투영), 자막 VM, `UWxNameplateComponent`(ASC 태그 기반 표시/거리 스케일). 인디케이터·자막은 StateTree 태스크 노드까지 함께 제공.
- 공용 위젯 베이스: `UWxActivatableWidget`·`UWxButtonBase`·`UWxTabListWidgetBase`·`UWxGamePopup` 등 WBP가 파생할 C++ 베이스와 BP 파사드(`UWxUILibrary`·`UWxMVVMConversionLibrary`).

**경계 (비담당)**
- 표시할 도메인 데이터의 원천 — 전투/ASC는 [[WxCombat]], 상호작용·월드 오브젝트는 [[WxWorld]], 대화 세션은 [[WxDialogue]]. WxUI는 구체 타입을 모른 채 소비 측이 push/주입한 값만 표시한다.
- WBP·위젯 계층·MVVM View 바인딩 자체(에셋 측 저작).
- Experience/GameFeature를 통한 컴포넌트 주입 결정(GameMode·Experience 에셋).

## 의존성
- **주요 의존**: `WxCore`, CommonUI/CommonInput, UMG/Slate, ModelViewViewModel, GameplayAbilities, StateTreeModule, ModularGameplay. (에디터 전용: AssetRegistry·EnhancedInput — EI 아이콘 프리뷰용.)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅ (`WxUI.Build.cs`·`WxUI.uplugin` 의존은 엔진 모듈 + `WxCore`뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 진입점. push·확인 팝업·정지 재평가·HUD/사망/대화 화면 자동 전환 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 태그별 위젯 스택을 z-order로 소유하는 레이아웃 루트 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 파사드(레이어 push·확인 팝업·활성 위젯 비활성화) | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 레이아웃/팝업/HUD·사망·대화 화면 클래스 설정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxViewModel` | 이미지 비동기 스트리밍을 공통 제공하는 뷰모델 베이스 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | 캐릭터 표시(이름/초상화) + 자식 AbilitySystem VM Composite | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_Selection` | 소스 무관 "현재 선택" 상세 표시 글로벌 VM | `Source/WxUI/Public/MVVM/WxViewModel_Selection.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 옵션을 가진 화면 베이스 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxGamePopup` / `UWxGamePopupDescriptor` | 확인 팝업 위젯 베이스와 서술자 | `Source/WxUI/Public/Widget/WxGamePopup.h` |
| `UWxIndicatorManagerComponent` | 로컬 PC의 화면 좌표 인디케이터 투영·발행 | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxNameplateComponent` | ASC 태그 기반 표시/거리 스케일 네임플레이트 | `Source/WxUI/Public/Component/WxNameplateComponent.h` |

## 확장 포인트 / 규약
- 새 화면: `UWxActivatableWidget`(정지가 필요하면 `bPauseGame`)를 WBP로 파생 → `UWxUIManagerSubsystem::PushContentToLayer`/`UWxUILibrary::PushSoftContentToLayer`로 `UI.Layer.*` 태그에 push. 로드 지연이 문제면 `UWxAsyncAction_PushWidgetToLayer`.
- 레이어 태그(`UI.Layer.*`)·액션 태그(`UI.Action.*`)는 네이티브 C++ 선언이 아니라 태그 소스에 정의되며, `UWxPrimaryGameLayout::LayerTags`의 배열 순서가 z-order다(0 = 최하단).
- 새 표시 데이터: `UWxViewModel` 파생 VM을 만들고 도메인 소스가 값을 push/Initialize; 이미지 슬롯은 `RequestImageAsync`/`ApplyLoadedImage`로 소프트 참조를 로드해 하드 참조만 노출(WBP는 일반 Image의 SetBrushResourceObject에 바인딩).
- 확인 팝업: `UWxGamePopupDescriptor::CreateConfirmation*` 또는 `UWxUILibrary::ShowConfirmationPopup`(BP)로 Modal 레이어에 표시, 결과는 델리게이트로 회수.
- StateTree 노드(`WxIndicatorStateTreeNodes`·`WxSubtitleStateTreeNodes`)로 인디케이터/자막을 태스크 단위로 붙인다 — 소비 도메인은 WxUI를 코드로 참조하지 않고 에셋에서 노드만 고른다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — UI 전체의 조율자. push·정지·화면 자동 전환의 계약이 모두 여기 있다.
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조와 z-order 모델.
3. `Source/WxUI/Public/MVVM/WxViewModel.h` — 뷰모델 계층의 공통 베이스(이미지 스트리밍 규약).
4. `Source/WxUI/Public/System/WxUIDeveloperSettings.h` — 어떤 WBP가 어디에 결선되는지 데이터 주도 설정 표면.

## 관련
- 상위: [[WxGame]] (Experience/GameMode가 레이아웃·컴포넌트를 주입), 데이터 원천 [[WxCombat]]·[[WxWorld]]·[[WxDialogue]]
- 기반: [[WxCore]]

---
*문서 기준 커밋 `6e08d6d` · 생성일 2026-08-05 · 소스 64파일 — `/readme-writer`로 갱신*
