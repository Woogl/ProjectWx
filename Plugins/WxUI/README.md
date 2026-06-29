# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 기반으로 게임의 UI 표시·입력·데이터 바인딩을 담당하는 도메인 플러그인. 위젯은 BP/WBP가 본체이고, 본 모듈은 그 베이스 클래스·뷰모델·매니저 등 C++ 골격을 제공한다.

## 책임
**담당**
- 레이어 기반 위젯 스택 관리: `UWxPrimaryGameLayout`이 Game/GameMenu/Menu/Modal 레이어(`UCommonActivatableWidgetStack`)를 들고, `UWxUIManagerSubsystem`이 LocalPlayer 단위로 레이아웃을 생성·소유한다.
- CommonUI 활성화 위젯 베이스(`UWxActivatableWidget`)와 HUD 루트(`UWxHUDLayout`) 제공: 입력 모드/게임 일시정지/메뉴 토글 액션 처리.
- ASC(AbilitySystem) 상태를 UMG에 노출하는 MVVM 뷰모델 계층: 어트리뷰트/어빌리티/이펙트/소유 태그를 지연 생성 VM으로 바인딩.
- 캐릭터/어빌리티/이펙트/상호작용 표시 데이터 모양 정의(`FWxCharacterUIData` 등)와 네임플레이트 위젯 컴포넌트.
- BP 바인딩용 Function Library(레이어 제어, MVVM 컨버전)와 비동기 위젯 푸시 액션.

**경계 (비담당)**
- 어빌리티·어트리뷰트·이펙트의 실제 게임플레이 로직 → GameplayAbilities / `[[WxCombat]]`. 본 모듈은 읽기 전용 표시만.
- 구체 캐릭터 타입 — 표시 데이터(`FWxCharacterUIData`)는 소비 측(`[[WxGame]]`/캐릭터 BP)이 `Initialize`로 주입한다.
- 상호작용 대상 레지스트리·선택 소유 → `[[WxWorld]]`. `UWxViewModel_InteractionList`는 외부 리졸버가 연결한 엔진 타입 델리게이트로 목록/선택을 수신해 표시만 한다.
- 실제 위젯 비주얼/레이아웃/MVVM 바인딩 그래프는 WBP 자산이 소유.

## 의존성
- **주요 의존**: `WxCore`(`UWxAbilityComponent` 확장, `WxGameplayTags.h`), CommonUI/CommonInput, ModelViewViewModel(MVVM), GameplayAbilities, GameplayTags, UMG.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`.uplugin`·`Build.cs`의 Wx 의존은 `WxCore` 단독. cross-plugin include도 `WxGameplayTags.h`(WxCore)뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. LocalPlayer별 레이아웃 생성 + 레이어 푸시의 단일 진입점 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 4개 레이어 스택(Game/GameMenu/Menu/Modal)을 들고 태그로 위젯 푸시 (Abstract, BP 파생) | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | CommonUI 활성화 위젯 베이스. 입력 모드/게임 일시정지 캡슐화 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDLayout` | 항상 활성화되는 HUD 루트. 메뉴 토글 액션을 Menu 레이어 푸시로 연결 | `Source/WxUI/Public/Widget/WxHUDLayout.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트/태그 자식 VM으로 노출(지연 생성) | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 컨버전(Get Attribute/Ability ViewModel, 태그→Visibility) | `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxAsyncAction_PushWidgetToLayer` | 소프트클래스 비동기 로드 후 레이어에 푸시(BP 비동기 노드) | `Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxNameplateComponent` | ASC 기반 MVVM 초기화 + 거리별 스케일을 캡슐화한 위젯 컴포넌트 | `Source/WxUI/Public/Component/WxNameplateComponent.h` |

## 확장 포인트 / 규약
- **새 뷰모델**: `UWxViewModel`을 상속하고, 정리 로직은 `Deinitialize` 오버라이드에 모은다(베이스 `BeginDestroy`가 자동 호출). 재초기화 가능한 VM은 `Initialize` 진입부에서 `Deinitialize`를 먼저 호출해 이전 상태를 비운다.
- **Composite VM 패턴**: `UWxViewModel_Character` → `UWxViewModel_AbilitySystem` → 어트리뷰트/어빌리티/이펙트 자식 VM. 어트리뷰트·어빌리티 VM은 UI 바인딩이 요청할 때만 `GetOrCreate...`로 지연 생성된다.
- **새 활성화 위젯/HUD**: `UWxActivatableWidget`(또는 `UWxHUDLayout`)을 상속한 WBP를 만들고, 입력 모드/일시정지/메뉴 위젯 클래스를 EditDefaults로 저작한다.
- **표시 데이터 주입**: 캐릭터는 `FWxCharacterUIData`, 어빌리티는 `UWxAbilityComponent_UIData`, 이펙트는 `UWxEffectComponent_UIData`로 소비 측 BP에서 저작 → VM/컴포넌트가 소비.
- **레이어 푸시**: 동기는 `UWxUIManagerSubsystem::PushContentToLayer`(또는 `UWxUILibrary`), BP 비동기(Soft 클래스 로드)는 `UWxAsyncAction_PushWidgetToLayer`. LayerTag는 `UI.Layer.*` 계층.
- **레이아웃 데이터 주도 설정**: `UWxUIDeveloperSettings::LayoutClass`(Config=Game)로 사용할 `UWxPrimaryGameLayout` 서브클래스를 지정한다(서브시스템이 PlayerController 세팅 시 소프트클래스 동기 로드/생성).
- **MVVM 바인딩 헬퍼**: `UWxLazyImage`는 UE 5.7 MVVM 픽커가 1-arg 함수만 노출하는 제약을 위해 `SetLazyTexture` 래퍼를 추가한 `UCommonLazyImage` 확장.
- C++ 네이티브 GameplayTag 선언은 본 모듈에 없음 — `UI.Layer.*`/`UI.Action.*` 등은 외부(`WxCore`) 정의 태그를 사용.

## 여기서부터 읽어라
1. `Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — UI 부트스트랩의 시작점. LocalPlayer/PlayerController 콜백 → 레이아웃 생성 → 레이어 푸시 흐름.
2. `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — MVVM 데이터 흐름의 핵심. ASC 이벤트 → 자식 VM 지연 생성/갱신 구조 파악.
3. `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` — WBP 바인딩이 VM을 가져오는 경로.
4. `Source/WxUI/Public/Widget/WxActivatableWidget.h` — 모든 화면 위젯의 입력/일시정지 베이스 동작.

## 관련
- 상위: 게임 모듈 `[[WxGame]]`(`LayoutClass` 설정·상호작용 리졸버·캐릭터 데이터 주입), foundation `[[WxCore]]`(`UWxAbilityComponent`·GameplayTags). 데이터 소스로 `[[WxCombat]]`·`[[WxWorld]]`.

---
*문서 기준 커밋 `97577fb` · 생성일 2026-06-29 · 소스 38파일 — `/readme-writer`로 갱신*
