# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 기반으로 게임의 화면 UI(HUD·팝업·인디케이터·자막·네임플레이트)를 구성하는 런타임 플러그인. C++ 는 뼈대(레이어·뷰모델 베이스·매니저)만 제공하고, 실제 위젯 계층은 BP/WBP 로 저작한다.

## 책임
**담당**
- 화면 레이어 스택 관리와 위젯 push/deactivate (`UWxPrimaryGameLayout`, `UWxUIManagerSubsystem`)
- 표시용 데이터의 MVVM 뷰모델화 — ASC 어트리뷰트/어빌리티/이펙트, 캐릭터, 인터랙션, 인디케이터, 자막 (`UWxViewModel` 파생)
- 화면 인디케이터(마커)의 좌표 투영과 슬롯 관리 (`UWxIndicatorManagerComponent`)
- HUD·네임플레이트·확인 팝업·자막의 표시 진입점
- 소비 도메인이 UI 모듈을 참조하지 않고 에셋에서 고를 수 있는 StateTree 노드(인디케이터 표시·자막 출력)

**경계 (비담당)**
- 무엇을 표시할지의 실제 데이터 — 구체 캐릭터 타입을 알지 못하므로 소비 측(`WxGame`·도메인 플러그인)이 `Initialize`/주입으로 넣는다.
- 어떤 HUD·화면을 띄울지, 컴포넌트 부착 여부 — Experience 에셋(GameFeature)이 정해 UI 매니저에 발행하거나 컨트롤러에 주입한다.
- 어트리뷰트·태그·이펙트의 원천 — GameplayAbilities ASC.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이어 생성, 위젯 push, 확인 팝업, 사망·대화 태그 관찰, 게임 정지 재평가 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | GameplayTag 별 `CommonActivatableWidgetStack` 을 z-order 로 들고, 레이어에 위젯을 push | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxViewModel` | 모든 뷰모델의 Abstract 베이스. 표시용 이미지(텍스처/머터리얼) 비동기 스트리밍 공통 제공 | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_Character` | ASC·이름·초상화를 묶는 Composite 뷰모델 (네임플레이트·HUD 진입점) | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxIndicatorManagerComponent` | 로컬 PC 에 매달려 인디케이터 목록의 화면 좌표를 매 틱 투영·발행하는 ControllerComponent | `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxHUDComponent` | Experience 가 발행한 HUD 를 Game 레이어에 띄우는 ControllerComponent | `Source/WxUI/Public/Component/WxHUDComponent.h` |
| `UWxActivatableWidget` | 입력 모드·게임 정지 지정을 얹은 `CommonActivatableWidget` 베이스 (WBP 상속 루트) | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxUILibrary` | BP 진입 함수 라이브러리 — 서브시스템·레이아웃 접근, 확인 팝업, 레이어 비활성 | `Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- **위젯 추가**: WBP 를 `UWxActivatableWidget`(또는 `UWxHUDLayout`·`UWxGamePopup` 등 파생)에서 상속하고, `EditDefaultsOnly` 로 입력 모드·정지 여부를 지정한다. 레이어에는 `UWxAsyncAction_PushWidgetToLayer`(BP) 또는 서브시스템 API 로 push 한다.
- **뷰모델 추가**: `UWxViewModel` 을 상속하고 표시 필드에 `FieldNotify` 를 단다. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage` 로 소프트 참조를 비동기 로드한다. 정리는 `Deinitialize` 를 오버라이드하고 `Super` 호출로 이미지 요청을 일괄 취소한다.
- **뷰모델 공유**: 데이터 소스를 Outer 로 `FindSharedViewModel` — 같은 소스를 보는 위젯끼리 같은 인스턴스를 가리킨다(클래스 정확 일치).
- **매니저 늦은 도착 규약**: 인디케이터·자막·인터랙션 뷰모델은 인스턴스를 고정한 채 도착 신호(`OnAnyManagerReady` 등)로 내부 상태만 갈아끼운다 — 리졸버가 돌려준 인스턴스를 뷰가 교체할 수 없기 때문.
- **데이터 주도 설정**: 레이아웃·팝업·사망/대화 화면 클래스는 `UWxUIDeveloperSettings`(Config=Game) 에서 소프트 클래스로 지정. HUD 클래스는 Experience 가 런타임에 UI 매니저로 발행.
- **도메인 연동 노드**: 인디케이터/자막은 본 모듈이 소유하되 StateTree 태스크로도 노출해, 퀘스트 등 소비 도메인이 UI 모듈 참조 없이 에셋에서 골라 쓴다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 전체 UI 수명주기와 오케스트레이션의 중심. 레이어·팝업·상태 관찰이 여기서 엮인다.
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조와 push 경로. UI 배치의 기본 모델.
3. `Source/WxUI/Public/MVVM/WxViewModel.h` — 모든 표시 데이터가 통과하는 MVVM 베이스와 비동기 이미지 규약.
4. `Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` — 로컬-사건 표시(인디케이터)를 컨트롤러에 매다는 패턴의 대표 예.

## 관련
- 상위: `WxGame` 및 GameFeature/도메인 플러그인이 표시 데이터를 주입하고, Experience 에셋이 HUD·컴포넌트·화면 클래스를 정해 이 모듈을 구동한다.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 63파일 — `/readme-writer`로 갱신*
