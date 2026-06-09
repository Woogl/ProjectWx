# WxUI — UI 시스템

> CommonUI + UMG View Model(MVVM) 기반 UI 모듈. 레이어 스택으로 화면 전환을 관리하고, GAS 상태(어트리뷰트/쿨다운/이펙트)를 ViewModel로 노출해 WBP가 비즈니스 로직 없이 바인딩만으로 HUD·메뉴를 그릴 수 있게 한다.

## 책임
**담당**
- `UWxPrimaryGameLayout`의 4개 레이어 스택(Game / GameMenu / Menu / Modal)으로 위젯 푸시/팝 및 화면 전환 관리
- `UWxUIManagerSubsystem`(GameInstance Subsystem)으로 PlayerController 단위 레이아웃 생성/재생성 및 레이어 진입점 제공
- GAS 상태를 UI 바인딩용으로 노출하는 ViewModel 계층(어트리뷰트/어빌리티 쿨다운/액티브 이펙트/액터 표시명)
- CommonUI 위젯 베이스(`UWxActivatableWidget`: 입력 모드/일시정지 처리), MVVM 친화 위젯(`UWxLazyImage`)
- Blueprint에서 쓰는 UI 유틸리티 라이브러리(레이어 제어, MVVM 컨버전, 비동기 위젯 푸시) 및 GameplayEffect용 UI 데이터 컴포넌트(`UWxEffectComponent_UIData`)

**경계 (비담당)**
- 전투/어빌리티 로직 자체는 [[WxCombat]] — WxUI는 ASC가 발행하는 어트리뷰트/이펙트 변경만 관찰해 표시
- 인벤토리 데이터/아이템 정의는 [[WxInventory]] — WxUI는 슬롯 WBP 표현만 담당
- 공용 Gameplay Tag(`UI.Layer.*` 등) 선언은 [[WxCore]] (`WxGameplayTags`)
- 실제 HUD/메뉴 위젯의 레이아웃·비주얼·바인딩 그래프는 게임 콘텐츠 WBP([[WxGame]])

## 의존성
- **주요 의존**: [[WxCore]] · CommonUI · CommonInput · ModelViewViewModel(UMG ViewModel) · GameplayAbilities · GameplayTags · DeveloperSettings · UMG/Slate
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance Subsystem. PC당 레이아웃 생성/재생성, 레이어 푸시 진입점 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 4개 레이어 스택 보유, `FGameplayTag → Stack` 매핑으로 위젯 푸시(Abstract, WBP가 구현) | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | `LayoutClass`(사용할 PrimaryGameLayout WBP) 프로젝트 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxActivatableWidget` | CommonUI 화면 베이스. 입력 모드/싱글플레이 일시정지 처리(Abstract) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 모든 ViewModel의 베이스. `IsInitialized` FieldNotify로 shell/실데이터 구분(Abstract) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | Composite VM. ASC의 어트리뷰트/어빌리티/이펙트/OwnedTags를 자식 VM 배열로 관리 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModel_Attribute` | 임의 어트리뷰트(현재/최대/퍼센트/empty/full) 변경 추적 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Attribute.h` |
| `UWxViewModel_Ability` | 어빌리티 쿨다운/충전(Charges) 상태를 CooldownGE 기준으로 노출 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` |
| `UWxViewModel_Effect` | 액티브 GameplayEffect의 남은 시간/스택/아이콘 노출 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Effect.h` |
| `UWxEffectComponent_UIData` | GameplayEffect에 붙이는 UI 데이터(이름/아이콘/설명). VM이 읽어감 | `Plugins/WxUI/Source/WxUI/Public/Component/WxEffectComponent_UIData.h` |
| `UWxUILibrary` | BP용 UI 유틸(레이어/Activatable 제어) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxAsyncAction_PushWidgetToLayer` | BP Async. 위젯 클래스 비동기 로드 후 레이어 푸시 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |

## 폴더 구성
- `Public/System` — 레이아웃·매니저 서브시스템·개발자 설정(UI 골격)
- `Public/Widget` — CommonUI 위젯 베이스(`WxActivatableWidget`), MVVM 친화 위젯(`WxLazyImage`), 비동기 푸시 액션
- `Public/MVVM` — ViewModel 계층 + MVVM 컨버전 BP 라이브러리(`WxMVVMConversionLibrary`)
- `Public/Component` — GameplayEffect용 UI 데이터 컴포넌트
- 루트 — `WxUILibrary`(BP 유틸), `WxUIModule`

## 확장 포인트 / 규약
- **새 화면 추가**: `UWxActivatableWidget`를 부모로 WBP를 만들고, `UWxUILibrary::GetUIManagerSubsystem` → `PushContentToLayer(LayerTag, …)` 또는 `UWxAsyncAction_PushWidgetToLayer`로 푸시. `LayerTag`는 `UI.Layer.*`(WxCore 선언) 중 하나.
- **레이아웃 교체**: Project Settings의 `UWxUIDeveloperSettings::LayoutClass`에 사용할 PrimaryGameLayout WBP를 지정. 매니저가 PC 세팅 시 이 클래스를 동기 로드해 `AddToPlayerScreen`.
- **새 ViewModel 추가**: `UWxViewModel` 파생. `Initialize` 말미에 `SetInitialized(true)`, `Deinitialize` 진입 시 `SetInitialized(false)` 호출이 규약(`IsInitialized` FieldNotify가 shell/실데이터를 구분).
- **데이터 주도 설정**: GameplayEffect에 `UWxEffectComponent_UIData`를 붙여 이름/아이콘/설명을 에디터에서 지정하면 `UWxViewModel_Effect`가 읽어 표시. 어빌리티 쿨다운/충전 수는 어빌리티 CDO의 CooldownGE에서 자동 추출.
- **WBP 바인딩 패턴**: ViewModel은 모두 `FieldNotify` 프로퍼티로 노출되어 MVVM View Binding 대상. `UWxMVVMConversionLibrary`로 태그→bool/Visibility, VM 배열에서 태그/어트리뷰트로 자식 VM 찾기 등의 컨버전 제공.
- **멀티(최대 4인)**: `UWxActivatableWidget::bPauseGame`는 싱글플레이 한정(코드 주석 명시). 레이아웃은 PlayerController 단위로 생성된다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — UI가 어떻게 띄워지는지(PC당 레이아웃 생성/재생성, 레이어 푸시)의 시작점
2. `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp` — `UI.Layer.*` 태그 → 위젯 스택 매핑과 푸시 동작
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — GAS 상태가 ViewModel로 흘러드는 구조(자식 VM Composite)
4. `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` — 모든 화면 위젯의 베이스 규약(입력 모드/일시정지)

## 관련
- 상위: [[WxGame]](레이아웃·HUD·메뉴 WBP 콘텐츠 보유), GAS 상태 원천은 [[WxCombat]], 인벤토리 표현은 [[WxInventory]]
- 공용 정의: [[WxCore]] (`UI.Layer.*` Native Tag)

---
*문서 기준 커밋 `d60410d8` · 생성일 2026-06-09 · 소스 70파일 — `/readme-writer`로 갱신*
