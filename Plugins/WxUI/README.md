# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 얹은 게임 UI 도메인. 위젯을 어느 레이어에 어떻게 띄울지 관리하고, 게임플레이 상태(ASC·상호작용·자막·인디케이터)를 위젯이 바인딩할 표시 데이터로 변환한다. 본체 위젯은 WBP이며, 이 모듈은 그 베이스 C++ 타입과 오케스트레이션을 제공한다.

## 책임
**담당**
- 레이어드 UI 오케스트레이션: `UWxUIManagerSubsystem`이 로컬 플레이어·빙의 폰을 추적해 HUD/대화/사망 화면을 레이어에 push, 활성 위젯 기반으로 게임 정지 재평가
- ViewModel 계층(MVVM): ASC의 어트리뷰트/어빌리티/이펙트, 캐릭터·선택·상호작용·자막·인디케이터를 위젯이 바인딩할 평면 표시 필드로 노출
- 위젯 베이스 클래스: Activatable/버튼/탭/팝업 등 CommonUI 파생 베이스와 확인 팝업 흐름
- 화면 표시 서비스: 월드 대상 위 화면 인디케이터, 화면 자막, 그리고 이를 켜는 StateTree 노드
- 컴포넌트: 캐릭터 네임플레이트(WidgetComponent), 이펙트 UI 데이터

**경계 (비담당)**
- 표시할 도메인 데이터의 원천(전투 수치·인벤토리·상호작용 대상 등)은 소비 측이 주입/push한다 — WxUI는 구체 게임 타입을 알지 않는다
- 구체 위젯 레이아웃·MVVM 바인딩 그래프는 WBP 에셋에 있다(코드는 계약만 제공)

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 모듈 의존). 엔진: `CommonUI`/`CommonInput`, `UMG`, `ModelViewViewModel`, `GameplayAbilities`, `StateTreeModule`, `ModularGameplay`
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 플레이어/폰 추적, 레이어 push, 정지 재평가, 공유 선택 VM 소유 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 태그별 위젯 스택(z-order) 보유. 실제 push 대상 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입점(레이어 push, 팝업, Activatable 비활성화) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxViewModel` | 모든 VM의 베이스. Soft 이미지 비동기 스트리밍 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxActivatableWidget` | 레이어에 올리는 화면 베이스(입력 모드·정지 의사) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxIndicatorManagerComponent` | 로컬 플레이어의 화면 인디케이터 목록 투영(ControllerComponent) | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxNameplateComponent` | ASC 기반 네임플레이트 VM 초기화 + 거리 스케일(WidgetComponent) | `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h` |

## 확장 포인트 / 규약
- **위젯 추가**: `UWxActivatableWidget`을 상속한 WBP를 만들고, 레이어 태그(`UI.Layer.*`)로 `UWxUIManagerSubsystem::PushContentToLayer`(또는 `UWxUILibrary`/`UWxAsyncAction_PushWidgetToLayer`)로 띄운다. 화면 클래스는 `UWxUIDeveloperSettings`(HUD/사망/대화/레이아웃/팝업)에 지정한다.
- **ViewModel 추가**: `UWxViewModel`을 상속하면 이미지 소프트 참조는 `RequestImageAsync`/`ApplyLoadedImage`로 비동기 로드된다. 값은 항상 외부 소스가 push하고, VM은 도메인 타입을 참조하지 않는 평면 표시 필드만 노출한다.
- **화면당 하나뿐인 VM**(선택/자막)은 엔진 `UMVVMGameSubsystem` 글로벌 컬렉션에 보관해 표시 위젯과 push 소스가 같은 인스턴스를 찾게 한다.
- **MVVM 바인딩 컨버전**: `UWxMVVMConversionLibrary`가 어트리뷰트/어빌리티/이펙트 VM을 지연 생성·조회하는 BlueprintPure 함수를 제공한다(WBP 바인딩에서 호출).
- **소비 도메인용 StateTree 노드**: 인디케이터/자막 시스템을 소유한 본 모듈이 `FWxStateTreeTask_MarkIndicators`·`FWxStateTreeTask_PrintSubtitle`을 함께 제공한다 — 퀘스트 등 소비 도메인이 WxUI를 코드 참조하지 않고 에셋에서 노드를 골라 쓴다.
- **인디케이터/네임플레이트 부착**은 코드가 아니라 Experience 에셋의 컴포넌트 주입 목록으로 한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` — 플레이어/폰 추적, 레이어 push, 정지 재평가의 전체 흐름
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — VM 베이스와 이미지 스트리밍 규약(모든 VM의 공통 토대)
3. `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` — 어떤 위젯 클래스가 어느 상황에 붙는지 배선표

## 관련
- 상위: 표시 데이터 원천 도메인(전투·상호작용·대화·퀘스트)이 push 소스이자 소비자다. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 64파일 — `/readme-writer`로 갱신*
