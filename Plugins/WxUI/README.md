# WxUI — UI 시스템

> CommonUI 레이어 스택과 ModelViewViewModel(MVVM) 뷰모델로 게임 UI를 구성하는 도메인 플러그인. 위젯은 구체 게임 타입을 모르고, 표시 데이터는 소비 측(게임 모듈)이 뷰모델에 주입한다.

## 책임
**담당**
- CommonUI 기반 화면 스택 관리: 레이어(태그 z-order) · 활성 위젯 · 게임 정지 재평가 (`UWxUIManagerSubsystem`, `UWxPrimaryGameLayout`)
- MVVM 뷰모델 계층: ASC 어트리뷰트/어빌리티/이펙트, 캐릭터, 자막, 인디케이터, 상호작용을 UMG 바인딩용 표시 필드로 노출
- 공용 위젯 베이스: ActivatableWidget · Button · Tab · Popup · HUDLayout · ActionWidget
- 월드 표시 컴포넌트: 네임플레이트(`UWxNameplateComponent`), 화면 인디케이터(`UWxIndicatorManagerComponent`)
- 자막/인디케이터의 StateTree 노드를 함께 제공해, 소비 도메인이 WxUI를 참조하지 않고도 에셋에서 골라 쓰게 함

**경계 (비담당)**
- 표시할 데이터의 원천(캐릭터 스탯·아이콘·화자 등)은 소비 측이 주입 — WxUI는 구체 타입을 모름
- 어빌리티/이펙트 판정·실행은 GameplayAbilities(엔진)와 [[WxCombat]] 소관, WxUI는 읽기만
- 대화 세션·퀘스트 진행 로직은 [[WxDialogue]] · [[WxQuest]]. WxUI는 그 신호(태그·ST 노드)에 반응해 화면만 띄움

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존), 엔진: `CommonUI`/`CommonInput` · `ModelViewViewModel` · `GameplayAbilities` · `StateTreeModule` · `ModularGameplay` · `UMG` · `UniversalObjectLocator`. 에디터 전용: `EnhancedInput`·`AssetRegistry`(디자인타임 아이콘 프리뷰)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이어 push, 확인 팝업, 빙의/사망/대화 태그 관찰, 게임 정지 재평가의 오케스트레이터 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그마다 `UCommonActivatableWidgetStack`을 두는 화면 루트. 배열 순서가 z-order | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUIDeveloperSettings` | Layout/Popup/HUD/Death/Dialogue 위젯 클래스를 config로 지정 | `Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxUILibrary` | BP 진입점: 레이어 push, 위젯 비활성화, 확인 팝업 | `Source/WxUI/Public/WxUILibrary.h` |
| `UWxViewModel` | MVVM 베이스. 아이콘/초상화 소프트 참조의 비동기 스트리밍을 공통 제공 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | 캐릭터 단위 Composite VM(이름·초상화 + 자식 AbilitySystem VM) | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxViewModel_AbilitySystem` | ASC당 하나. 어트리뷰트/어빌리티/이펙트 VM을 지연 생성·관리 | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 컨버전(태그→Visibility, 어트리뷰트 VM 획득) | `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxActivatableWidget` | CommonUI 위젯 베이스(입력 모드, 게임 정지 요청) | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxGamePopup` / `UWxGamePopupDescriptor` | 확인 팝업 베이스와 그 서술자(버튼 구성·결과 enum) | `Source/WxUI/Public/Widget/WxGamePopup.h` |
| `UWxNameplateComponent` | WidgetComponent 확장. ASC 태그 기반 표시 판정 + 거리 스케일 | `Source/WxUI/Public/Component/WxNameplateComponent.h` |
| `UWxIndicatorManagerComponent` | 컨트롤러 컴포넌트. 화면 인디케이터 목록·투영 계산·발행 | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `FWxStateTreeTask_PrintSubtitle` | 자막 출력 ST 태스크(DataTable 행 체인) | `Source/WxUI/Public/Subtitle/WxStateTreeTask_PrintSubtitle.h` |
| `FWxStateTreeTask_MarkIndicator` | 인디케이터 표시 ST 태스크(로케이터 대상) | `Source/WxUI/Public/Indicator/WxStateTreeTask_MarkIndicator.h` |

## 확장 포인트 / 규약
- **새 위젯**: `UWxActivatableWidget`(전체화면·모달) 또는 `UWxButtonBase`/`UWxTabListWidgetBase` 등 CommonUI 베이스를 상속. `InputMode`·`bPauseGame`으로 입력/정지 성향 지정. 실제 정지 적용은 매니저가 전 레이어를 재평가해 결정.
- **새 뷰모델**: `UWxViewModel`을 상속. 아이콘/초상화는 소프트 참조로 받아 `RequestImageAsync`에 맡기고 `ApplyLoadedImage`로 수신 — 로드된 하드 참조만 UMG에 노출한다. Composite는 `FindSharedViewModel`/`GetOrCreate...`로 데이터 소스를 공유 키 삼아 인스턴스를 공유.
- **View Bindings Resolver**: `UMVVMViewModelContextResolver` 파생(`UWxViewModelResolver_Attribute`/`_Indicator`/`_Subtitle`)을 WBP의 Creation Type=Resolver로 선택. 매니저·ASC가 위젯보다 늦게 도착할 수 있어, 리졸버가 준 인스턴스는 고정한 채 도착 신호로 내부 상태만 교체한다.
- **데이터 주도 설정**: 화면 클래스(레이아웃/HUD/사망/대화/팝업)는 전부 `UWxUIDeveloperSettings`의 소프트 클래스 config. 자막은 `FWxSubtitleTableRow` DataTable로 한 편을 정의.
- **레이어 태그**: `UI.Layer.*` (config·`LayerTags` 배열에서 참조). 태그 원천은 WxCore/프로젝트 태그 — WxUI에는 Native Tag 선언이 없다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면 흐름 전체의 지휘부. 어떤 신호가 어떤 레이어에 무엇을 띄우는지 여기서 시작
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어=스택 구조와 push API의 실체
3. `Source/WxUI/Public/MVVM/WxViewModel.h` + `WxViewModel_AbilitySystem.h` — VM 계층의 뿌리와 ASC 브리지. 데이터 주입/공유 패턴이 여기 응축
4. `Source/WxUI/Private/Component/WxNameplateComponent.cpp` — VM 주입 → MVVM View 바인딩의 구체 사용 예

## 관련
- 상위: [[WxGame]] (실제 위젯 클래스·Experience 주입 설정)
- 데이터 원천: [[WxCombat]](ASC/어트리뷰트) · [[WxWorld]](상호작용) · [[WxDialogue]]·[[WxQuest]](자막·인디케이터 ST 노드 소비)

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 61파일 — `/readme-writer`로 갱신*
