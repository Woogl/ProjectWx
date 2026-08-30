# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM 뷰모델을 기반으로, 게임의 모든 화면·HUD·팝업·자막·인디케이터·네임플레이트 표시를 책임진다. 무엇을 언제 띄울지는 Experience/도메인이 정하고, 이 모듈은 어떻게 띄우고 무엇에 바인딩할지를 제공한다.

## 책임
**담당**
- 레이어(GameplayTag)별 위젯 스택 관리와 위젯 push/pop, 비동기 로드 push 액션
- 화면 오케스트레이션: HUD·사망 화면·대화 창을 폰 상태 태그·빙의 변화에 반응해 띄우고 걷음, 게임 정지(pause) 재평가
- MVVM 뷰모델 계층: ASC 어트리뷰트/어빌리티/이펙트, 캐릭터, 상호작용, 자막, 인디케이터를 UMG에 노출하고 표시용 이미지 비동기 스트리밍
- 확인 팝업, HUD/탭/버튼/액션 등 CommonUI 위젯 베이스 클래스
- 월드 표시물: 네임플레이트(WidgetComponent), 화면 인디케이터(투영 좌표 계산)
- 소비 도메인이 UI 모듈을 참조하지 않고 쓸 수 있도록 자막·인디케이터용 StateTree Task 제공

**경계 (비담당)**
- 무엇을 언제 띄울지의 결정 — Experience 에셋과 도메인 로직에 위임(이 모듈은 클래스와 트리거만 받는다)
- 어트리뷰트·어빌리티·GE 원본 데이터 — [[WxCombat]]/GameplayAbilities ASC가 소유, 뷰모델은 관찰만
- 구체 캐릭터 타입 지식 — 표시 데이터는 소비 측(게임 모듈)이 읽어 `Initialize`로 주입
- WBP 위젯 계층·MVVM 바인딩 그래프 — 콘텐츠(Blueprint) 영역

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | 화면 오케스트레이터. 빙의·상태 태그를 구독해 HUD/사망/대화 화면과 게임 정지를 제어 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 레이어(GameplayTag)→위젯 스택 맵. 모든 push의 착지점 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxAsyncAction_PushWidgetToLayer` | soft 클래스 비동기 로드 후 레이어에 push, 취소 지원 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h` |
| `UWxViewModel` | 모든 뷰모델의 베이스. 공유 VM 조회와 이미지 비동기 스트리밍 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC의 어트리뷰트/어빌리티/이펙트를 자식 VM으로 노출하는 Composite | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxActivatableWidget` | CommonActivatableWidget 베이스. 입력 모드·게임 정지 의사 표명 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxHUDComponent` | Experience가 주입하는 컨트롤러 컴포넌트. HUD를 Game 레이어에 push | `Plugins/WxUI/Source/WxUI/Public/Component/WxHUDComponent.h` |
| `UWxIndicatorManagerComponent` | 로컬 인디케이터 목록을 들고 매 틱 화면 좌표를 투영·발행 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUIDeveloperSettings` | 레이아웃·팝업·사망/대화 화면 클래스 프로젝트 설정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |
| `UWxUILibrary` | BP 진입점(매니저/레이아웃 조회, 팝업, 레이어 비활성화) | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- **새 화면 띄우기**: `UWxAsyncAction_PushWidgetToLayer` 또는 `UWxUIManagerSubsystem::PushContentToLayer`로 레이어 태그(`UI.Layer.*`)에 push. 위젯은 `UWxActivatableWidget`을 상속하고 입력 모드·`bPauseGame`을 지정.
- **새 뷰모델**: `UWxViewModel` 상속. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage`로 soft 참조를 로드해 하드 참조만 UMG에 노출. 공유가 필요하면 데이터 소스를 Outer로 만들어 `FindSharedViewModel`로 재사용.
- **데이터 주입 규약**: WxUI는 구체 캐릭터/도메인 타입을 모른다. 소비 측이 대상에서 표시 데이터를 읽어 `Initialize(...)`로 주입한다(예: `UWxViewModel_Character`, `UWxNameplateComponent`).
- **클래스 지정은 데이터 주도**: 레이아웃/팝업/사망/대화 위젯 클래스는 `UWxUIDeveloperSettings`(config)로, HUD 클래스는 Experience가 `SetGameHUDClass`로 매니저에 발행. 컴포넌트 부착도 Experience 에셋의 주입 목록으로 하며 컨트롤러는 UI 클래스를 모른다.
- **도메인용 StateTree Task**: 자막(`FWxStateTreeTask_PrintSubtitle`)·인디케이터(`FWxStateTreeTask_MarkIndicator`) 노드를 본 모듈이 제공해, 퀘스트 등 소비 도메인이 WxUI를 코드 참조하지 않고 에셋에서 골라 쓴다.
- **표시는 로컬 사건**: 인디케이터·게임 정지·화면은 보는 사람마다 다르므로 컨트롤러/로컬에 매달린다. 인디케이터 매니저는 원격 사본(데디 서버가 든 PC)에서 등록을 거부. 최대 4인 멀티에서 `bPauseGame`은 적용되지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 모듈의 오케스트레이션 중심. 화면들이 어떤 신호에 반응해 뜨고 걷히는지가 여기 다 있다.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 모델. 모든 push가 최종적으로 도달하는 곳.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — 뷰모델 계층의 뿌리와 공유·이미지 스트리밍 규약.

## 관련
- 상위: Experience 에셋과 GameMode가 이 모듈의 컴포넌트/HUD 클래스를 주입·발행한다. 뷰모델은 [[WxCombat]]의 ASC를 관찰하고, 자막·인디케이터 StateTree Task는 [[WxQuest]]/[[WxDialogue]] 같은 도메인이 소비한다.

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 63파일 — `/readme-writer`로 갱신*
