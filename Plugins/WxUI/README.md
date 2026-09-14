# WxUI — UI 시스템

> CommonUI 레이어 스택과 MVVM(ModelViewViewModel)을 뼈대로, 게임 화면·HUD·팝업·인디케이터를 띄우고 데이터에 연결하는 UI 도메인 모듈. 화면 구성(위젯 계층·바인딩)은 BP/WBP가 맡고, C++은 진입점·수명·데이터 브리지를 제공한다.

## 책임
**담당**
- 레이어 스택 기반 화면 관리: 레이어별 위젯 push/pop, 메뉴 활성 여부, 게임 정지(Pause) 재평가.
- MVVM 뷰모델 계층: 캐릭터·어빌리티·아이템·상호작용·자막·인디케이터 등 표시 데이터를 UMG에 노출하는 ViewModel과 공유·해석(Resolver) 규약.
- 데이터 주도 UI 설정(레이아웃/팝업/사망·대화 화면 클래스), 확인 팝업, 화면 인디케이터 액터, 네임플레이트 컴포넌트.

**경계 (비담당)**
- 표시할 원본 데이터의 정의(`IWxUIData` 등)와 태그 정의는 [[WxCore]]에 위임.
- 어빌리티·이펙트·어트리뷰트의 실제 값은 GAS(GameplayAbilities)에서 읽고, 전투 로직은 [[WxCombat]]·인벤토리는 [[WxInventory]] 등 각 도메인이 소유.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이아웃 생성, 레이어 push, 사망·대화 상태 태그 감시, 게임 정지 조율의 중심 오케스트레이터 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 화면 최상위 위젯. `UI.Layer` 태그별 `UCommonActivatableWidgetStack`을 z-order 순으로 보유(Abstract, BP 파생) | `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxUILibrary` | BP 진입 함수 라이브러리. 서브시스템/레이아웃 조회, 확인 팝업 표시 | `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` |
| `UWxViewModel` | 모든 뷰모델의 베이스. 소스 Outer 기반 공유본 조회와 이미지 비동기 스트리밍 공통 제공 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModelResolver_PlayerCharacter` | MVVM Context Resolver. 위젯 소유 PC의 빙의 Pawn ASC에서 공유 Character VM을 해석 | `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_PlayerCharacter.h` |
| `UWxActivatableWidget` | CommonUI 활성 위젯 베이스. 입력 모드·정지 요청 정의(실제 정지는 서브시스템이 재평가) | `Plugins/WxUI/Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxGamePopupDescriptor` / `UWxGamePopup` | 확인 팝업 서술자와 위젯. 버튼 레이아웃·결과(`EWxPopupResult`) 규약 | `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h` |
| `UWxUIDeveloperSettings` | 데이터 주도 설정. 레이아웃·팝업·사망·대화 화면 클래스(Soft) 지정 | `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` |

## 확장 포인트 / 규약
- 새 화면 위젯: `UWxActivatableWidget`(또는 팝업이면 `UWxGamePopup`)을 BP로 파생하고, 레이어 태그(`UI.Layer.*`)를 지정해 서브시스템/라이브러리로 push한다. 정지가 필요하면 `bPauseGame`으로 요청하되 최종 판단은 `UWxUIManagerSubsystem`이 전 레이어를 재평가해 내린다.
- 새 ViewModel: `UWxViewModel` 파생. 소스 오브젝트를 Outer로 공유본을 만들고(`FindSharedViewModel`), 이미지 필드는 `RequestImageAsync`/`ApplyLoadedImage` 규약으로 비동기 로드한다. WBP는 소프트 참조가 아닌 로드된 하드 참조만 바인딩한다.
- MVVM 연결: 위젯의 뷰모델 소스는 `UMVVMViewModelContextResolver` 파생 Resolver로 해석한다(예: PlayerCharacter). 위젯은 빙의 완료 후 생성되어야 Pawn/ASC 전제가 지켜진다.
- 화면 구성 클래스·팝업·사망/대화 화면은 코드 상수가 아니라 `UWxUIDeveloperSettings`(Config = Game)로 주입한다.
- 상태 연동: 사망·대화 등은 폰 ASC의 상태 태그를 서브시스템이 감시해 자동으로 화면을 띄우고 걷는다(위젯은 서브시스템을 알지 못함).

## 여기서부터 읽어라
1. `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` — 레이어·정지·상태 태그 감시가 모이는 중심. 모듈 제어 흐름의 출발점.
2. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h` — 공유·이미지 스트리밍 규약을 담은 VM 베이스. 데이터 흐름의 기준.
3. `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 태그와 위젯 스택의 매핑 구조.

## 관련
- 상위: [[WxCore]] (공용 정의·`IWxUIData`·태그)

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 58파일 — `/readme-writer`로 갱신*
