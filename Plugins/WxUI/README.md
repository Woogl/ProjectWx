# WxUI — UI 시스템

> CommonUI + UMG View Model(MVVM) 기반 UI 모듈. 레이어 스택으로 화면 전환을 관리하고, GAS 상태(어트리뷰트/쿨다운/이펙트)와 캐릭터 표시 데이터를 ViewModel로 노출해 WBP가 비즈니스 로직 없이 바인딩만으로 HUD·메뉴를 그릴 수 있게 한다.

## 책임
**담당**
- `UWxPrimaryGameLayout`의 4개 레이어 스택(Game / GameMenu / Menu / Modal)으로 위젯 푸시/팝 및 화면 전환 관리
- `UWxUIManagerSubsystem`(GameInstance Subsystem)으로 PlayerController 단위 레이아웃 생성/재생성 및 레이어 푸시 진입점 제공
- GAS 상태를 UI 바인딩용으로 노출하는 ViewModel 계층(캐릭터 Composite / 어빌리티 시스템 Composite / 어트리뷰트 / 어빌리티 쿨다운·충전 / 액티브 이펙트)
- CommonUI 위젯 베이스(`UWxActivatableWidget`: 입력 모드/일시정지 처리), HUD 루트(`UWxHUDLayout`: 메뉴 토글 액션 수신), MVVM 친화 위젯(`UWxLazyImage`)
- UI 표시 데이터의 "모양" 소유(`FWxCharacterUIData`, `UWxAbilityComponent_UIData`, `UWxEffectComponent_UIData`)와 BP UI 유틸리티(레이어 제어, MVVM 컨버전, 비동기 위젯 푸시)
- ASC 기반 네임플레이트 위젯 컴포넌트(`UWxNameplateComponent`: 거리 스케일·표시 조건)

**경계 (비담당)**
- 전투/어빌리티 로직 자체는 [[WxCombat]] — WxUI는 ASC가 발행하는 어트리뷰트/이펙트 변경만 관찰해 표시. 구체 캐릭터 타입을 모르므로 표시 데이터(`FWxCharacterUIData`)는 소비 측이 주입
- 인벤토리 데이터/아이템 정의는 [[WxInventory]] — WxUI(`UWxHUDLayout`)는 인벤토리 메뉴 WBP를 레이어에 푸시하는 진입점만 제공
- 공용 Gameplay Tag(`UI.Layer.*` 등) 선언은 [[WxCore]] (`WxGameplayTags`). 입력 키 매핑은 Project Settings의 CommonUI Input Settings 책임
- 실제 HUD/메뉴 위젯의 레이아웃·비주얼·바인딩 그래프는 게임 콘텐츠 WBP([[WxGame]])

## 의존성
- **주요 의존**: [[WxCore]] · CommonUI · CommonInput · ModelViewViewModel(UMG ViewModel) · GameplayAbilities · GameplayTags · DeveloperSettings · UMG
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅ (Build.cs·uplugin 모두 확인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance Subsystem. PC당 레이아웃 생성/재생성, 레이어 푸시 진입점 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 4개 레이어 스택 보유, `FGameplayTag → Stack` 매핑으로 위젯 푸시(Abstract, WBP가 구현) | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | CommonUI 화면 베이스. 입력 모드/싱글플레이 일시정지 처리(Abstract) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDLayout` | 상시 활성 HUD 루트(`UI.Layer.Game`). `UI.Action.*` 입력으로 메뉴 토글 푸시 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h` |
| `UWxViewModel` | 모든 ViewModel의 베이스. `IsInitialized` FieldNotify로 shell/실데이터 구분(Abstract) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | 캐릭터 Composite VM. 자식 AbilitySystem VM 소유 + 이름/초상화 표시 데이터 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_AbilitySystem` | Composite VM. ASC의 어트리뷰트/어빌리티/이펙트/OwnedTags를 자식 VM으로 노출(지연 생성) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 MVVM 컨버전(태그→Visibility, VM에서 어트리뷰트/어빌리티/이펙트 자식 VM 조회) | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxNameplateComponent` | ASC 기반 네임플레이트 WidgetComponent. 거리 스케일·표시 조건(ShowIfAny/HideIfAny) | `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h` |
| `UWxEffectComponent_UIData` | GameplayEffect에 붙이는 UI 데이터(이름/아이콘). VM이 읽어감 | `Plugins/WxUI/Source/WxUI/Public/Component/WxEffectComponent_UIData.h` |
| `UWxUILibrary` | BP용 UI 유틸(매니저/레이아웃 조회, 레이어·Activatable 제어) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |

## Gameplay Tags
- 선언: `UI.Action.*`/`UI.Layer.*` 모두 `WxGameplayTags`(WxCore)에 네이티브 태그로 선언되어 있고 WxUI가 소비한다(GameplayTagFilter `UI.Layer` 사용).
- 주요 네임스페이스: `UI.Action.*` — CommonUI 액션 태그(인벤토리/메인 메뉴 토글). HUD가 입력 액션을 수신해 해당 메뉴를 Menu 레이어에 푸시하며, 키 매핑은 CommonUI Input Settings에서 지정.

## 확장 포인트 / 규약
- **새 화면 추가**: `UWxActivatableWidget`를 부모로 WBP를 만들고, `UWxUILibrary::GetUIManagerSubsystem` → `PushContentToLayer(LayerTag, …)` 또는 `UWxAsyncAction_PushWidgetToLayer`(비동기 클래스 로드)로 푸시. `LayerTag`는 `UI.Layer.*`(WxCore 선언) 중 하나.
- **레이아웃 교체**: Project Settings의 `UWxUIDeveloperSettings::LayoutClass`(Config=Game)에 사용할 PrimaryGameLayout WBP를 지정. 매니저가 PC 세팅 시 이 클래스를 로드해 플레이어 화면에 부착한다.
- **새 ViewModel 추가**: `UWxViewModel` 파생. `Initialize` 말미에 `SetInitialized(true)`, `Deinitialize` 진입 시 `SetInitialized(false)` 호출이 규약(`IsInitialized` FieldNotify가 shell/실데이터를 구분). 프로퍼티는 `FieldNotify`로 노출하고 외부 신호(어트리뷰트 변경·GE 추가/제거·태그 변경)에 바인딩해 Setter로 갱신.
- **MVVM 컨버전 함수**: `UWxMVVMConversionLibrary`에 `BlueprintPure` static으로 추가. 리터럴 입력용 구조체/컨테이너 인자는 값 전달로(예: `FGameplayTagContainer AbilityTags`) — const 참조면 MVVM 바인딩 패널에서 리터럴 편집이 불가하다.
- **지연 생성 모델**: 어트리뷰트 VM은 (Current, Max) 어트리뷰트 쌍, 어빌리티 VM은 Asset Tags HasAll 질의를 키로, 바인딩이 컨버전 함수(`GetAttributeViewModel`/`GetAbilityViewModel`)로 실제 요청할 때만 `GetOrCreate...`로 생성·캐시된다. 이펙트 VM은 활성 GE 추가/제거 이벤트로 관리.
- **MVVM 친화 위젯**: UE 5.7 바인딩 픽커는 1-arg 함수만 노출하므로 다인자 setter는 `UWxLazyImage::SetLazyTexture`처럼 1-arg 래퍼를 만들어 바인딩 타겟으로 쓴다.
- **데이터 주도 설정**: GameplayEffect에 `UWxEffectComponent_UIData`, 어빌리티에 `UWxAbilityComponent_UIData`(아이콘)를 붙여 표시 데이터를 지정하면 VM이 읽어 표시. 캐릭터 표시 이름/초상화는 `FWxCharacterUIData`를 소비 측에서 저작해 `UWxViewModel_Character::Initialize`/`UWxNameplateComponent::InitializeViewModels`로 주입.
- **멀티(최대 4인)**: `UWxActivatableWidget::bPauseGame`는 게임 일시정지를 직접 호출하는 싱글플레이 한정(헤더 주석 명시). 레이아웃은 PlayerController 단위로 생성된다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — UI가 어떻게 띄워지는지(PC당 레이아웃 생성/재생성, 레이어 푸시)의 시작점
2. `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp` — `UI.Layer.*` 태그 → 위젯 스택 매핑과 푸시 동작
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — GAS 상태가 ViewModel로 흘러드는 구조(자식 VM Composite, 지연 생성)
4. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` — WBP가 자식 ViewModel을 끌어오는 바인딩 컨버전 규약

## 관련
- 상위: [[WxGame]](레이아웃·HUD·메뉴 WBP 콘텐츠 보유), GAS 상태 원천은 [[WxCombat]], 인벤토리 표현은 [[WxInventory]]
- 공용 정의: [[WxCore]] (`UI.Layer.*`·`UI.Action.*` Native Tag)

---
*문서 기준 커밋 `157ccd5` · 생성일 2026-06-13 · 소스 38파일 — `/readme-writer`로 갱신*
