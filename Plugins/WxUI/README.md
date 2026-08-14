# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel) 뷰모델을 두 축으로 화면 표시 전반을 책임지는 도메인 플러그인. C++는 위젯 베이스·뷰모델·표시 데이터 브리지의 골격만 제공하고, 실제 위젯 계층과 바인딩 그래프는 BP/WBP가 채운다.

## 책임
**담당**
- 레이어 기반 화면 스택 관리(레이어별 push, z-order, 게임 정지 재평가) — `UWxUIManagerSubsystem` + `UWxPrimaryGameLayout`
- 게임 상태(ASC 어트리뷰트/어빌리티/이펙트, 선택 대상, 인디케이터, 자막)를 표시 데이터로 옮기는 뷰모델 계층(`UWxViewModel` 파생)과 공용 비동기 이미지 스트리밍
- 위젯 베이스 클래스(Activatable·팝업·버튼·탭·액션 위젯)와 BP/서브시스템 진입점
- 월드 표시 컴포넌트: 네임플레이트(`UWxNameplateComponent`), 화면 좌표 인디케이터(`UWxIndicatorManagerComponent`)
- 소비 도메인이 UI 모듈을 참조하지 않고 에셋에서 골라 쓰는 StateTree 노드(자막 출력·인디케이터 마킹)

**경계 (비담당)**
- 구체 캐릭터/아이템/상호작용 타입 — 소비 측이 `FWxCharacterUIData` 주입, `SetSelection`, ST 노드 등으로 표시 데이터를 push 한다.
- 전투/어빌리티 상태 자체의 소유 — [[WxCombat]] 계열이 소유하고 WxUI 는 ASC 를 관찰만 한다.
- 상호작용·대화·퀘스트의 도메인 로직 — [[WxWorld]]·[[WxDialogue]]·[[WxQuest]]. WxUI 는 그 결과를 표시할 계약(VM·ST 노드)만 제공한다.
- WBP 위젯 계층·MVVM 바인딩 그래프·특정 WBP 애셋 — 콘텐츠(BP)에서 저작.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진: CommonUI/CommonInput, ModelViewViewModel(MVVM), GameplayAbilities, StateTree, ModularGameplay, UMG. (에디터 전용으로 AssetRegistry·EnhancedInput — 디자인타임 아이콘 프리뷰에서만.)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (uplugin·Build.cs·소스 include 모두 WxCore 외 Wx 참조 없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 레이어 push·확인 팝업·정지 재평가·상태 태그(사망/대화) 관찰을 모으는 GameInstance 오케스트레이터 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그별 `CommonActivatableWidgetStack` 을 들고 실제 push 를 수행하는 루트 위젯 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·HUD·사망/대화 화면 클래스를 config 로 주입하는 데이터 주도 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxViewModel` | 모든 뷰모델의 추상 베이스. FieldName 단위 비동기 이미지 스트리밍 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC 를 어트리뷰트/어빌리티/이펙트 자식 VM 으로 지연 노출하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩이 자식 VM(Get Attribute/Ability ViewModel 등)을 지연 획득하는 BP 컨버전 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxIndicatorManagerComponent` | 화면 인디케이터 목록을 들고 매 틱 화면 좌표를 계산·발행하는 ControllerComponent | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxActivatableWidget` | 입력 모드·정지 의사를 갖는 위젯 베이스(팝업·HUD·화면의 공통 부모) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |

## 확장 포인트 / 규약
- **새 화면 위젯**: `UWxActivatableWidget` 을 상속하고 `InputMode`/`bPauseGame` 을 세팅한다. 정지 적용은 위젯이 직접 하지 않고 서브시스템이 전 레이어를 재평가해 결정한다. HUD 루트는 `UWxHUDLayout` 을 상속.
- **새 뷰모델**: `UWxViewModel` 을 상속하고 표시 필드는 `FieldNotify` UPROPERTY 로 둔다. 이미지(텍스처/머터리얼) 필드는 `RequestImageAsync`/`ApplyLoadedImage` 오버라이드로 비동기 로드에 태운다. WxUI 는 도메인 타입을 모르므로 값은 항상 소비 측이 `Initialize`/`Set...` 로 주입한다.
- **글로벌 공유 VM**: 선택(`UWxViewModel_Selection`)·자막(`UWxViewModel_Subtitle`)은 화면당 하나라 엔진 `UMVVMGameSubsystem` 글로벌 컬렉션에 등록되고, 도메인 브리지가 값을 push 한다.
- **데이터 주도 설정**: `UWxUIDeveloperSettings`(Config=Game)의 소프트 클래스가 레이아웃/팝업/HUD/사망·대화 화면을 구동한다. 미지정 슬롯은 해당 동작 없음.
- **자막**: `FWxSubtitleTableRow` DataTable 한 줄 = 자막 한 줄, `NextRow` 로 이어지고 `None` 이면 종료. `FWxStateTreeTask_PrintSubtitle` 로 에셋에서 재생.
- **인디케이터**: 매니저 부착은 코드가 아니라 Experience 에셋 주입 목록으로 한다. 레벨에서 대상 지정은 `FWxStateTreeTask_MarkIndicator`(FUniversalObjectLocator).

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이어 push·정지·상태 화면 전환의 제어 흐름이 모이는 허브. 모듈 전체 그림이 여기서 잡힌다.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` + `WxViewModel_AbilitySystem.h` — 뷰모델 축의 베이스와 대표 Composite. 지연 생성·비동기 이미지 규약이 담김.
3. `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` — 어떤 애셋이 UI 를 구동하는지 데이터 주도 진입점.

## 관련
- 상위: [[WxGame]] 및 GameFeature 콘텐츠 플러그인이 Experience 로 매니저·HUD·인디케이터를 조립한다. ASC 상태는 [[WxCombat]], 선택/자막/인디케이터의 값 push 는 [[WxWorld]]·[[WxDialogue]]·[[WxQuest]] 가 담당.

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 64파일 — `/readme-writer`로 갱신*
