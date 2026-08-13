# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel) 뷰모델을 축으로 화면 표시를 담당하는 도메인 플러그인. C++는 위젯 베이스·뷰모델·표시 데이터 브리지의 골격을 제공하고, 실제 위젯 계층/바인딩은 BP/WBP가 채운다.

## 책임
**담당**
- 레이어 기반 화면 스택 관리(push/pop, z-order, 정지 재평가) — `UWxUIManagerSubsystem` + `UWxPrimaryGameLayout`
- 게임 상태 → 표시 데이터 변환용 뷰모델 계층(`UWxViewModel` 파생) 및 비동기 이미지 스트리밍
- 위젯 베이스 클래스(ActivatableWidget, 팝업, 버튼, 탭, 액션 위젯)와 BP/서브시스템 진입점
- 월드 공간 표시 컴포넌트: 네임플레이트(`UWxNameplateComponent`), 화면 좌표 인디케이터(`UWxIndicatorManagerComponent`)
- 자막 출력 및 인디케이터 마킹용 StateTree 태스크

**경계 (비담당)**
- 구체 캐릭터/아이템/상호작용 타입 — 소비 측(게임 모듈·각 도메인)이 `FWxCharacterUIData`나 `SetSelection` 등으로 표시 데이터를 주입한다.
- 전투/어빌리티 상태 자체의 소유 — [[WxCombat]] 계열이 소유, WxUI는 ASC를 관찰만 한다.
- WBP 위젯 계층·MVVM 바인딩 그래프 — 콘텐츠(BP)에서 저작.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: CommonUI/CommonInput, ModelViewViewModel, GameplayAbilities, StateTree, ModularGameplay, UMG.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Build.cs·소스 include 모두 WxCore만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 오케스트레이터. 레이아웃 생성, 레이어 push, 팝업·HUD·사망·대화 화면 조율, 정지 재평가 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그 → 위젯 스택 맵. 실제 push 대상 컨테이너 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | 데이터 주도 설정. 레이아웃/팝업/HUD/사망/대화 위젯 클래스를 소프트 참조로 지정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxViewModel` | 전 뷰모델의 베이스. 소프트 이미지 비동기 스트리밍(`RequestImageAsync`)을 공통 제공 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | 캐릭터 표시용 Composite VM(이름/초상화 + AbilitySystem 자식 VM). 주입 진입점 | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_Selection` | 소스 무관 "현재 선택 대상" 글로벌 VM. 도메인이 `SetSelection`으로 push | `Source/WxUI/Public/MVVM/WxViewModel_Selection.h` |
| `UWxUILibrary` | BP 진입점(레이어 push, 팝업, ActivatableWidget 제어) | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxActivatableWidget` / `UWxGamePopup` | 화면 위젯 베이스. 입력 모드·게임 정지 의사, 팝업 결과 계약 | `Source/WxUI/Public/Widget/WxActivatableWidget.h`, `Widget/WxGamePopup.h` |

## 확장 포인트 / 규약
- **새 위젯**: `UWxActivatableWidget`(전체 화면류)나 `UWxButtonBase`/`UWxTabButtonBase` 등을 상속한 WBP를 만들고, 소프트 클래스를 `UWxUIDeveloperSettings` 또는 push 호출 지점에 지정한다. 위젯은 서브시스템을 알 필요가 없다 — 활성/비활성 델리게이트를 서브시스템이 관찰해 정지를 재평가한다.
- **새 뷰모델**: `UWxViewModel`(또는 그 파생)을 상속. 표시 필드는 `UPROPERTY(FieldNotify)`로 노출하고, 이미지는 소프트 참조를 `RequestImageAsync` → `ApplyLoadedImage` 오버라이드로 받는다(WBP는 일반 Image의 SetBrushResourceObject에 바인딩). 소비 측이 도메인 데이터를 `Initialize`/`SetXxx`로 주입하는 규약 — WxUI는 구체 도메인 타입을 참조하지 않는다.
- **데이터 주도**: 어떤 위젯이 어느 상황에 뜨는지는 코드가 아니라 `UWxUIDeveloperSettings`의 소프트 클래스 슬롯이 구동한다. 레이어 태그(`UI.Layer.*`)와 z-order는 `UWxPrimaryGameLayout`의 `LayerTags` 배열(에디터 저작)이 결정한다 — C++ Native Tag 선언은 없다.
- **인디케이터/네임플레이트**: 부착은 코드가 아니라 Experience 에셋의 컴포넌트 주입으로 한다. MVVM 컨버전(`UWxMVVMConversionLibrary`)이 ASC/태그 → 위젯 바인딩을 지연 생성 방식으로 잇는다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 전체 조율 흐름(레이아웃·HUD·팝업·대화·정지)의 허브. 여기서 나머지 타입의 쓰임이 드러난다.
2. `Source/WxUI/Public/MVVM/WxViewModel.h` — 뷰모델 계층의 공통 계약(이미지 스트리밍). 파생 VM들을 읽기 전 전제.
3. `Source/WxUI/Public/System/WxUIDeveloperSettings.h` — 무엇이 어떤 위젯을 띄우는지의 데이터 주도 설정.

## 관련
- 상위: 게임 모듈([[WxGame]])과 각 도메인이 표시 데이터를 주입/소비한다. 상호작용·대화·전투 상태를 관찰하므로 [[WxDialogue]]·[[WxCombat]]와 런타임에 맞물린다(코드 의존은 아님, ASC/태그 경유).

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 64파일 — `/readme-writer`로 갱신*
