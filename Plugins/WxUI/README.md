# WxUI — UI 시스템

> CommonUI 레이어 스택과 ModelViewViewModel(MVVM) 뷰모델로 게임의 런타임 UI를 책임진다. C++는 레이어 관리·위젯 베이스·표시 데이터 뷰모델(진실 소스는 밖)을 제공하고, 실제 위젯 외형은 WBP가 담당한다.

## 책임
**담당**
- CommonUI 레이어 스택 관리: `UWxPrimaryGameLayout`(Game/GameMenu/Menu/Modal 레이어)와 `UWxUIManagerSubsystem`을 통한 레이어별 위젯 push
- 위젯 베이스 클래스: 입력 모드/게임 일시정지를 캡슐화한 `UWxActivatableWidget`, HUD 루트 `UWxHUDLayout`
- MVVM 뷰모델 계층: ASC(어트리뷰트/어빌리티/이펙트) 및 캐릭터/선택/상호작용 표시 데이터를 View에 노출하는 순수 표시 계약
- 월드 부착 UI 컴포넌트: 네임플레이트(`UWxNameplateComponent`)
- UI 바인딩용 Blueprint Function Library 및 비동기 push 액션

**경계 (비담당)**
- 어트리뷰트/어빌리티/이펙트의 실제 값·수명 — GAS([[WxCombat]] 등이 세팅하는 ASC)가 진실 소스이며 VM은 표시만
- 상호작용 대상 목록/선택 상태의 소유 — [[WxWorld]] 레지스트리가 소유하고 WxGame 리졸버가 델리게이트로 VM에 흘려줌
- 구체 캐릭터 타입·표시 데이터의 출처 — 소비 측(WxGame)이 `FWxCharacterUIData`를 주입
- 위젯 외형·계층·바인딩 그래프 — WBP(범위 밖)

## 의존성
- **주요 의존**: `WxCore` · 엔진: `CommonUI` / `CommonInput` / `ModelViewViewModel` / `UMG` / `GameplayAbilities` / `GameplayTags` / `DeveloperSettings`
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (uplugin/Build.cs 상 Wx 의존은 `WxCore`뿐. [[WxWorld]] 등 타 도메인은 참조하지 않고 WxGame 리졸버 경유 델리게이트로만 연결)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxUIManagerSubsystem` | GameInstance 서브시스템. 레이아웃 생성·레이어 push 진입점, 글로벌 Selection VM 소유 | `Source/WxUI/Public/System/WxUIManagerSubsystem.h` |
| `UWxPrimaryGameLayout` | 4개 레이어 스택(Game/GameMenu/Menu/Modal)을 태그로 관리하는 루트 위젯 | `Source/WxUI/Public/System/WxPrimaryGameLayout.h` |
| `UWxActivatableWidget` | 입력 모드·게임 일시정지를 다루는 모든 활성화 위젯의 베이스 | `Source/WxUI/Public/Widget/WxActivatableWidget.h` |
| `UWxViewModel` | 모든 뷰모델의 추상 베이스(`Deinitialize` 규약) | `Source/WxUI/Public/MVVM/WxViewModel.h` |
| `UWxViewModel_AbilitySystem` | ASC를 어트리뷰트/어빌리티/이펙트 자식 VM으로 노출(지연 생성) | `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` |
| `UWxViewModel_Character` | 캐릭터 표시(이름/초상화) + AbilitySystem 자식 VM 묶는 Composite | `Source/WxUI/Public/MVVM/WxViewModel_Character.h` |
| `UWxMVVMConversionLibrary` | UMG 바인딩용 컨버전(어트리뷰트/어빌리티/이펙트 VM 조회, 태그→Visibility) | `Source/WxUI/Public/MVVM/WxMVVMConversionLibrary.h` |
| `UWxUILibrary` | UI 매니저/레이아웃 접근 및 레이어 제어 BP 라이브러리 | `Source/WxUI/Public/WxUILibrary.h` |

## Gameplay Tags
C++ Native Tag 선언 없음. 레이어/액션 태그(`UI.Layer.*`, `UI.Action.*`)는 문자열·에셋 태그로 참조되며 코드에서 정적 선언하지 않는다.

## 확장 포인트 / 규약
- **새 위젯**: `UWxActivatableWidget`(또는 `UWxHUDLayout`)을 베이스로 WBP 작성 → `UWxUIManagerSubsystem::PushContentToLayer` 혹은 `UWxAsyncAction_PushWidgetToLayer`로 레이어에 push. 레이어는 `FGameplayTag`(`UI.Layer.*`)로 지정.
- **새 뷰모델**: `UWxViewModel` 상속, 표시 필드는 `UPROPERTY(BlueprintReadOnly, FieldNotify)`. VM은 도메인 타입을 참조하지 않는 **순수 표시 계약**을 유지하고, 진실 소스(ASC/레지스트리/게임 모듈)가 값을 push하도록 둔다.
- **ASC 바인딩**: Composite VM(`_AbilitySystem`)에 중첩 바인딩하고, `UWxMVVMConversionLibrary`의 Get Attribute/Ability ViewModel 컨버전으로 자식 VM을 지연 조회한다.
- **레이아웃 지정**: `UWxUIDeveloperSettings::LayoutClass`(Project Settings, Config=Game)로 플레이어별 생성할 `UWxPrimaryGameLayout` 지정.
- **타 도메인 연동**: WxUI는 타 도메인을 직접 참조하지 않으므로, 목록/선택 등 외부 상태는 WxGame 리졸버가 `Handle*` 델리게이트 콜백에 연결해 흘려준다.

## 여기서부터 읽어라
1. `Source/WxUI/Public/System/WxUIManagerSubsystem.h` — UI 진입점. 레이아웃 생성/레이어 push/글로벌 VM 소유 흐름의 중심
2. `Source/WxUI/Public/System/WxPrimaryGameLayout.h` — 레이어 스택 구조(무엇이 어디로 push되는지)
3. `Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h` — MVVM 계층의 핵심. 지연 생성·자식 VM·컨버전 라이브러리 연계 이해의 기준점

## 관련
- 상위: WxGame(리졸버가 도메인 상태를 VM에 연결, 표시 데이터 주입), [[WxCombat]](ASC 진실 소스), [[WxWorld]](상호작용 레지스트리 — 델리게이트 경유)

---
*문서 기준 커밋 `7a536dd` · 생성일 2026-07-06 · 소스 44파일 — `/readme-writer`로 갱신*
