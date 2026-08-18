# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 얹은 게임 UI 기반 모듈. 레이어드 화면(HUD·메뉴·팝업·대화·자막)의 push/정지 오케스트레이션과, ASC·상호작용·인디케이터 등 도메인 상태를 위젯이 바인딩할 평면 표시 계약(ViewModel)을 제공한다.

## 책임
**담당**
- 레이어 스택 관리와 화면 push: `UWxPrimaryGameLayout`(z-order 레이어 맵)와 `UWxUIManagerSubsystem`(플레이어별 레이아웃 생성, HUD/사망/대화 화면 자동 push, 활성 위젯 기반 게임 정지 재평가).
- CommonUI 위젯 베이스 계층: `UWxActivatableWidget`/`UWxGamePopup`/`UWxButtonBase`/`UWxTabListWidgetBase` 등 C++ 베이스와 입력 모드·팝업 계약.
- MVVM 뷰모델 계약: 표시용 이미지 비동기 스트리밍 베이스(`UWxViewModel`)와 도메인별 파생(Character/AbilitySystem/Attribute/Ability/Effect/Interaction/Selection/Subtitle/Indicator), UMG 바인딩용 컨버전(`UWxMVVMConversionLibrary`).
- 화면 오버레이 시스템: 자막(`WxViewModel_Subtitle` + 글로벌 컬렉션 리졸버)과 인디케이터(`UWxIndicatorManagerComponent`가 매 틱 화면 좌표 투영), 그리고 두 시스템을 소비 도메인이 참조 없이 쓰도록 함께 제공하는 StateTree 태스크 노드.
- 네임플레이트 컴포넌트(`UWxNameplateComponent`)와 GE UI 데이터(`UWxEffectComponent_UIData`) 등 월드-공간 UI 부착점.

**경계 (비담당)**
- 구체 캐릭터/도메인 타입은 알지 않는다. 표시 데이터는 소비 측이 `Initialize`/`SetSelection` 등으로 push한다 — 전투 스탯 원천은 [[WxCombat]], 인벤토리 항목은 [[WxInventory]], 대화/자막 트리거는 [[WxDialogue]]·[[WxQuest]].
- WBP 위젯 계층·MVVM 바인딩 그래프의 실제 저작은 콘텐츠(BP/WBP)와 GameFeature 쪽 몫이다.
- 어떤 화면 클래스를 실제로 쓸지는 코드가 아니라 `UWxUIDeveloperSettings` 설정·Experience 주입으로 정한다.

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 도메인 의존). 엔진 서브시스템은 CommonUI/CommonInput, UMG, ModelViewViewModel(MVVM), GameplayAbilities(ASC 관찰), StateTree(오버레이 노드), ModularGameplay(컨트롤러 컴포넌트 주입), UniversalObjectLocator(인디케이터 대상 지정).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이어 push·화면 자동 표시·게임 정지 재평가의 단일 창구 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어 태그→위젯 스택 맵을 소유하는 화면 루트. 매니저가 이 위로 push | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP/게임 코드가 매니저·레이어를 건드리는 진입 함수 라이브러리(팝업 포함) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·HUD·사망·대화 화면 클래스를 설정으로 주입하는 배선점 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxViewModel` | 모든 파생 VM의 베이스. 표시 이미지 비동기 스트리밍을 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC의 어트리뷰트/어빌리티/이펙트를 지연 생성 자식 VM으로 노출하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxIndicatorManagerComponent` | 로컬 컨트롤러에 붙어 화면 인디케이터의 좌표를 매 틱 투영·발행 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 의사를 얹은 CommonUI 위젯 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 `UWxGamePopup`/`UWxHUDLayout`)을 상속한 WBP를 만들고, 어느 레이어에 뜰지는 `UI.Layer.*` 게임플레이 태그와 `UWxUIDeveloperSettings`/Experience로 배선한다. 코드로 클래스를 고정하지 않는다.
- 새 표시 데이터: `UWxViewModel` 파생 VM을 추가하고 소비 측이 `Initialize`/`Set...`으로 push한다. 이미지 필드는 소프트 참조를 `RequestImageAsync`에 넘기면 베이스가 로드 후 `ApplyLoadedImage`로 반영한다.
- 화면당 하나뿐인 전역 표시(자막·선택): MVVM 글로벌 컬렉션(`UMVVMGameSubsystem`)에 단일 인스턴스로 등록하고 리졸버로 위젯이 찾아가게 한다 — 표시 위젯과 값을 push하는 소스가 같은 인스턴스를 잡는 구조.
- 도메인이 UI를 참조하지 않고 오버레이를 쓰게: 자막/인디케이터는 StateTree 태스크 노드(`FWxStateTreeTask_PrintSubtitle`·`FWxStateTreeTask_MarkIndicator`)를 본 모듈이 함께 제공하므로 퀘스트·기믹 에셋이 노드만 골라 쓴다.
- 권한 모델: 표시는 "보는 사람의 로컬 사건"이라 인디케이터/자막은 로컬 컨트롤러·같은 게임 인스턴스 전제(v1 싱글/리슨 호스트)로 동작하며, 인디케이터 매니저는 원격 사본에서 등록을 거부한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 화면이 언제 어떻게 뜨고 정지가 어떻게 갈리는지, 모듈 전체 제어 흐름의 허브.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — 모든 VM이 공유하는 이미지 스트리밍 계약. 파생 VM을 읽기 전 베이스부터.
3. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 태그↔스택 매핑과 push API의 실제 목적지.

## 관련
- 상위: 게임 모듈 [[WxGame]]과 GameFeature 콘텐츠(Experience로 레이아웃/HUD 배선), 표시 데이터를 push하는 도메인 [[WxCombat]]·[[WxInventory]]·[[WxDialogue]]·[[WxQuest]]. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `36dc0e1` · 생성일 2026-08-18 · 소스 63파일 — `/readme-writer`로 갱신*
