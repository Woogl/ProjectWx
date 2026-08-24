# WxUI — UI 시스템

> CommonUI + MVVM(ModelViewViewModel) 위에서 게임의 모든 화면을 조립하는 UI 도메인 플러그인. 레이어 스택에 위젯을 얹고, 게임플레이 데이터(ASC·상호작용·자막·인디케이터)를 뷰모델로 노출해 위젯이 바인딩만 하면 되게 한다.

## 책임
**담당**
- 플레이어별 UI 루트(`UWxPrimaryGameLayout`) 생성과, GameplayTag 레이어(z-order) 단위의 위젯 push/pop.
- UI 흐름 오케스트레이션: 확인 팝업, 사망/대화 태그 관찰에 따른 화면 전환, 활성 위젯 기반 게임 정지.
- 게임플레이 데이터 → 표시 데이터 변환. ASC의 어트리뷰트/어빌리티/이펙트, 캐릭터, 상호작용, 자막, 인디케이터를 Composite ViewModel로 노출하고, 표시용 이미지(아이콘/초상화)를 비동기 스트리밍한다.
- 화면 인디케이터의 월드→스크린 좌표 투영과, 자막/인디케이터를 소비 도메인이 UI 모듈을 참조하지 않고 데이터로 쓰게 해주는 StateTree 태스크 제공.
- CommonUI 파생 공용 위젯 베이스(activatable·버튼·탭·액션 위젯·팝업)와 BP 진입 라이브러리.

**경계 (비담당)**
- 구체 캐릭터/전투/인벤토리 타입을 알지 않는다 — 소비 측(게임 모듈·[[WxCombat]]·[[WxInventory]])이 대상에서 읽어 `Initialize`로 뷰모델에 주입한다.
- 대화·퀘스트의 로직을 갖지 않는다. 세션 상태 태그를 관찰해 창을 띄우고 걷을 뿐, 진행은 [[WxDialogue]]·[[WxQuest]]가 소유한다.
- 어떤 HUD/레이아웃/팝업 위젯을 쓸지는 UI 밖(Experience 에셋·`UWxUIDeveloperSettings`)이 정하며, 컴포넌트 부착도 코드가 아니라 Experience 주입 목록으로 한다.
- 위젯(WBP) 계층·MVVM 바인딩 그래프 자체는 콘텐츠 자산이라 여기서 다루지 않는다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | UI 오케스트레이터. 레이어 push·팝업·사망/대화 전환·게임 정지의 허브 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 태그별 위젯 스택을 담은 플레이어 UI 루트 | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 입력 모드·정지 요청을 얹은 모든 화면 위젯의 베이스 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | VM 베이스. 이미지 비동기 스트리밍·공유 VM 조회 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC 하나를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxHUDComponent` | Experience가 컨트롤러에 주입하는 HUD 부착 컴포넌트 | `Plugins/WxUI/Source/WxUI/Public/Component/WxHUDComponent.h` |
| `UWxIndicatorManagerComponent` | 로컬 인디케이터 목록의 매 틱 스크린 좌표 투영 | `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` |
| `UWxUILibrary` | 레이어 push·팝업의 BP 진입점 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |

## 확장 포인트 / 규약
- **새 화면 추가**: `UWxActivatableWidget`(팝업은 `UWxGamePopup`)을 상속한 WBP를 만들고, `UWxUIManagerSubsystem::PushContentToLayer` / `UWxUILibrary::PushSoftContentToLayer` 로 `UI.Layer.*` 태그 레이어에 얹는다. 레이어 집합과 z-order는 `UWxPrimaryGameLayout::LayerTags` 배열이 정한다.
- **표시 데이터 노출**: 데이터 원천당 `UWxViewModel` 파생 하나를 만들고, 소비 측이 `Initialize`로 주입한다. ASC 계열은 `UWxViewModel_AbilitySystem::GetOrCreate`(ASC를 Outer로 ASC당 1개)·`GetOrCreateAttributeViewModel` 등 지연 생성 규약을 따른다. 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage` 로 베이스가 스트리밍한다.
- **VM ↔ 위젯 바인딩**: 위젯은 서브시스템/컴포넌트를 알지 못한다. `UMVVMViewModelContextResolver` 파생(예: `UWxViewModelResolver_Subtitle`, `UWxViewModelResolver_Indicator`)이 View에 VM을 물려주고, 위젯은 `FieldNotify` 필드에 바인딩한다.
- **다른 도메인에서 UI 태우기**: UI 모듈을 참조하지 않고도 자막/인디케이터를 쓰도록 StateTree 태스크(`FWxStateTreeTask_PrintSubtitle`, `WxStateTreeTask_MarkIndicator`)를 이 모듈이 제공한다 — 에셋에서 노드를 골라 데이터로 구동한다.
- **콘텐츠 지정**: 레이아웃/팝업/사망·대화 화면 클래스는 `UWxUIDeveloperSettings`(Config=Game) 소프트 클래스로, HUD 클래스는 Experience가 `UWxUIManagerSubsystem::SetGameHUDClass`로 발행한다.

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이어 생성부터 사망/대화 전환·정지까지 UI 제어 흐름의 진입점.
2. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — "레이어 = 태그별 위젯 스택" 이라는 화면 구성 모델.
3. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` → `WxViewModel_AbilitySystem.h` — 게임플레이 데이터가 위젯에 닿는 MVVM 경로와 지연 생성/공유 VM 규약.
4. `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorManagerComponent.h` — 컨트롤러 컴포넌트가 로컬 사건을 투영해 VM으로 발행하는 "매니저→VM→위젯" 패턴의 대표 사례.

## 관련
- 상위: Experience(게임 모듈·`Plugins/GameFeatures/`)가 컴포넌트 주입·HUD 클래스 발행으로 이 모듈을 구동한다.
- 함께 보는 모듈: [[WxCore]](공용 정의), 데이터 원천인 [[WxCombat]]·[[WxInventory]]·[[WxDialogue]]·[[WxQuest]]([[WxWorld]] 상호작용 포함).

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 63파일 — `/readme-writer`로 갱신*
