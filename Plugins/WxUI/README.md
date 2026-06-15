# WxUI — UI 시스템

> CommonUI 레이어 스택 위에 게임의 모든 위젯을 배치·전환하고, GAS(ASC) 상태를 MVVM ViewModel로 변환해 UMG에 흘려보내는 UI 프레임워크 모듈.

## 책임
**담당**
- 레이어 기반 위젯 스택 관리: `UWxPrimaryGameLayout`(Game/GameMenu/Menu/Modal 레이어)과 `UWxUIManagerSubsystem`(레이어에 위젯 푸시, 플레이어별 레이아웃 생성)
- ASC → UI 데이터 변환: `UWxViewModel_*` 계열이 어트리뷰트/어빌리티/이펙트/OwnedTags 변경을 관찰해 FieldNotify로 노출 (지연 생성 캐시)
- CommonUI 입력/일시정지 규약을 캡슐화한 위젯 베이스(`UWxActivatableWidget`, `UWxHUDLayout`)
- 월드 액터에 부착하는 네임플레이트 위젯 컴포넌트(`UWxNameplateComponent`)와 UMG 바인딩용 컨버전 라이브러리

**경계 (비담당)**
- 전투/어빌리티 로직 자체는 [[WxCombat]] — WxUI는 ASC가 발행하는 어트리뷰트/이펙트 변경만 관찰해 표시. 구체 캐릭터 타입을 모르므로 표시 데이터(`FWxCharacterUIData`)는 소비 측이 주입
- 인벤토리 데이터/아이템 정의는 [[WxInventory]] — WxUI(`UWxHUDLayout`)는 인벤토리 메뉴 WBP를 레이어에 푸시하는 진입점만 제공
- 상호작용 대상 선택 로직은 [[WxWorld]] — `UWxViewModel_Interaction`은 [[WxGame]] 리졸버가 연결한 델리게이트로 목록/선택을 받기만 함
- WBP 위젯 계층·MVVM 바인딩 그래프·레이아웃 콘텐츠 저작은 [[WxGame]] (BP 측)

## 의존성
- **주요 의존**: [[WxCore]], 엔진 서브시스템 `CommonUI`(+`CommonInput`), `ModelViewViewModel`, `GameplayAbilities`, `UMG`
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (WxWorld/WxGame 등은 주석상 경계 설명일 뿐 코드 의존 없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 플레이어 레이아웃 생성 + 레이어에 위젯 푸시하는 런타임 진입점 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 4개 레이어 스택(`UI.Layer.*` 태그→`UCommonActivatableWidgetStack`)을 보유하는 루트 레이아웃 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | CommonUI 입력 모드/게임 일시정지 규약을 캡슐화한 위젯 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDLayout` | 상시 활성 HUD 루트. 메뉴 토글 액션으로 인벤토리/메인메뉴 WBP를 푸시 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h` |
| `UWxViewModel` | 모든 ViewModel의 베이스. `IsInitialized` FieldNotify로 shell/실데이터 상태 구분 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | 캐릭터 단위 Composite VM. AbilitySystem 자식 VM + 이름/초상화 노출 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_AbilitySystem` | ASC의 어트리뷰트/어빌리티/이펙트/태그를 자식 VM으로 지연 노출하는 Composite VM | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxNameplateComponent` | 월드 액터용 네임플레이트 WidgetComponent. ASC 기반 VM 초기화 + 거리 스케일/태그 가시성 | `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩에서 어트리뷰트/어빌리티/이펙트 VM을 끌어오는 컨버전 함수 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |

## 확장 포인트 / 규약
- **새 위젯**: `UWxActivatableWidget`를 상속하면 입력 모드(`InputMode`)·일시정지(`bPauseGame`) 규약을 자동으로 받는다. 레이어에 올릴 때는 매니저/`UWxAsyncAction_PushWidgetToLayer`(BP async, `UI.Layer` 태그 필터)로 푸시.
- **새 ViewModel**: `UWxViewModel`를 상속하고 `Initialize` 말미에 `SetInitialized(true)`, `Deinitialize` 진입 시 `SetInitialized(false)`를 호출하는 규약을 지킨다. Composite VM은 자식 VM을 생성/소유하고 UMG가 프로퍼티 경로로 중첩 바인딩한다.
- **UI 데이터 주입**: WxUI는 구체 캐릭터 타입을 모른다. 게임 측이 `FWxCharacterUIData`(이름/초상화)를 저작해 `Initialize`로 주입한다. 어빌리티 아이콘 등은 `UWxAbilityComponent_UIData`/`UWxEffectComponent_UIData` 데이터 컴포넌트로 BP에 부착.
- **레이아웃 설정**: 사용할 루트 레이아웃 클래스는 `UWxUIDeveloperSettings.LayoutClass`(Config=Game, Soft 참조)로 데이터 주도 지정. 매니저가 플레이어 컨트롤러 설정 시 비동기 로드·생성한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — UI 진입점. 레이아웃이 언제 생성되고 위젯이 어떻게 레이어에 들어가는지의 시작점
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — GAS→UI 데이터 흐름의 핵심. 지연 생성 캐시와 이펙트/태그 이벤트 처리 패턴
3. `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` — 모든 위젯이 따르는 입력/일시정지 베이스 규약

## 관련
- 상위: [[WxGame]](레이아웃·HUD·메뉴 WBP 콘텐츠 보유), GAS 상태 원천은 [[WxCombat]], 인벤토리 표현은 [[WxInventory]]

---
*문서 기준 커밋 `6402bb0` · 생성일 2026-06-15 · 소스 42파일 — `/readme-writer`로 갱신*
