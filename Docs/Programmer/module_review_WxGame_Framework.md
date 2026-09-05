# WxGame Framework — 코드 리뷰

> GameMode에서 Experience 로드·복제·액션 실행을 GameState 컴포넌트로 분리한 방향은 적절하다. 남은 핵심 과제는 GameMode의 FrontEnd 정책 의존, Experience 실행기의 HUD 정책 의존, 콘텐츠 주입·수명주기 계약을 정리하는 것이다.
> 이번 리뷰는 `Source/WxGame/Framework/`의 C++ 16파일을 대상으로 한다. 관련 FrontEnd·Character와 엔진 코드는 교차 확인했으며, WxGame 모듈 전체 리뷰가 아니다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 0 |

현재 소스만으로 항상 발생한다고 확정할 치명적 런타임 오류는 발견하지 않았다. 아래 1~4는 확인된 구조적 제약이며, 5~6은 특정 액션·실패 입력에서 드러나는 수명주기 및 성공 판정 문제이다. 실행 재현은 하지 않았다.

`Source/`와 `Plugins/`의 h/cpp에서 `AWxGameMode`, `GetAuthGameMode`, `GetGameMode`를 검색한 결과, WxGameMode 외부의 직접 참조는 발견하지 않았다. 따라서 우선 줄일 것은 외부 코드의 GameMode 호출이 아니라 **GameMode가 구체 콘텐츠 정책을 아는 정도**이다. BP 참조까지 없다는 의미는 아니다.

## 결과

### 1. 🟡 GameMode가 FrontEnd의 선택·도착·입력 보류 정책을 직접 호출한다

- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:47`, `Source/WxGame/Framework/WxGameMode.cpp:72`, `Source/WxGame/Framework/WxGameMode.cpp:81`
- **범주**: 설계/구조
- **문제**: 폰 클래스 선택에 `UWxGameFlowSubsystem::GetSelectedPawnClass`를 사용하고, 플레이어 시작 전후에 `ValidateArrival`와 `HoldArrivalPawn`을 호출한다. FrontEnd를 별도 콘텐츠로 옮겨도 기본 GameMode에 이 의존이 남는다. 또한 `GetDefaultPawnClassForController_Implementation`의 `InController`를 사용하지 않고 GameInstance의 단일 선택을 조회하므로, 플레이어별 선택으로 확장할 때 현재 계약을 바꿔야 한다. 현재 단일 로컬 플레이 흐름이 잘못됐다는 판정은 아니다.
- **제안**: GameMode에는 Experience 선택·서버 스폰 게이트·엔진 override 어댑터를 남긴다. FrontEnd는 플레이어별 스폰 정책/요청에 선택 값을 전달하고, GameMode는 그 좁은 계약으로 폰 클래스를 조회한다. 도착 후 입력·스트리밍 대기는 컨트롤러 컴포넌트 또는 기존 Flow가 소유한다. 서버에서 실행되는 도착 검증이 필요하면 로컬 UI 대기와 분리한 시작 허용 계약을 제공한다. 클래스를 통째로 Subsystem에 옮기는 것으로 끝내지 않는다.
- **확신도**: 높음

### 2. 🟡 범용 Experience 실행기에 HUD 종류와 합성 우선순위가 들어 있다

- **위치**: `Source/WxGame/Framework/WxExperienceActionSet.h:41`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:27`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:343`
- **범주**: 설계/구조
- **문제**: ActionSet이 `UWxHUDLayout`을 알고, 매니저가 ActionSets 중 첫 번째 비어 있지 않은 HUD를 골라 `UWxUIManagerSubsystem`에 발행한다. UI 콘텐츠를 제거하거나 여러 콘텐츠의 HUD를 합성하려면 범용 로더와 데이터 스키마를 수정해야 한다. 여러 ActionSet에 HUD가 있으면 배열 순서가 묵시적 우선순위가 된다. WxGame이 WxUI에 의존하는 것 자체는 프로젝트 규칙 위반이 아니다.
- **제안**: 기존 액션 배열을 활용하여 HUD 등록·해제를 담당하는 별도 액션을 둔다. UI 매니저는 제공자별 등록 핸들과 충돌 정책을 소유하고, 액션은 자신의 등록만 회수한다. UI 브릿지는 WxGame 또는 상단 GameFeature에 두어 WxUI가 WxGame을 역참조하지 않게 한다. 범용 Experience 매니저는 액션 실행과 상태만 관리한다.
- **확신도**: 높음

### 3. 🟡 컴포넌트 상속만으로 타깃을 추론해 콘텐츠별 적용 범위를 표현할 수 없다

- **위치**: `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h:22`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:25`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:144`
- **범주**: 설계/구조
- **문제**: `UPawnComponent`이면 모든 opt-in Pawn, `UControllerComponent`이면 모든 opt-in Controller에 요청한다. 플레이어 Pawn만, 특정 차량만, 특정 컨트롤러만 대상으로 삼을 설정이 없다. 컴포넌트마다 소유자 캐스트·종류 판별을 넣어야 하므로 구성 정보가 콘텐츠 구현으로 흩어진다. 또한 네 프레임워크 베이스 이외 컴포넌트는 147행에서 거부하므로 일반 월드 오브젝트 기능에도 이 액션을 그대로 재사용할 수 없다. 복제 컴포넌트의 authority 생성 제한은 UE 5.8 컴포넌트 매니저가 처리하므로 여기서 중복 버그로 지적하지 않는다.
- **제안**: 엔트리에 선택적인 ReceiverClass를 제공하고 현재 추론을 기본값으로 유지한다. ReceiverClass는 엔진이 허용하는 구체 Actor 하위 클래스로 제한하고 opt-in·컴포넌트 호환성을 검증한다. 일반 오브젝트 기능이 실제 필요할 때 컴포넌트 허용 범위를 `UActorComponent`까지 넓힌다. 이때 수신 클래스의 소프트 참조도 번들에 포함한다. 필요에 따라 서버·클라이언트 적용 여부를 데이터로 선언하되 복제 컴포넌트 중복 생성을 유발하지 않는다.
- **확신도**: 높음

### 4. 🟡 C++ GameFeature가 사용할 Framework API가 DLL 경계에 공개되지 않았다

- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.h:34`, `Source/WxGame/Framework/WxExperienceDefinition.h:22`, `Source/WxGame/Framework/WxExperienceActionSet.h:18`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h:34`
- **범주**: 설계/구조
- **문제**: 위 타입은 `WXGAME_API` 또는 `MinimalAPI`가 없으며, 매니저의 `CallOrRegister_OnExperienceLoaded` 등 주요 메서드는 cpp에 정의되어 있다. 별도 C++ GameFeature DLL에서 이 메서드를 직접 호출하거나 관련 타입을 네이티브 확장하는 공개 계약이 준비되어 있지 않다. `AWxGameState`의 getter를 통해 포인터를 얻을 수 있는 것과, 그 타입의 비공개 심벌을 DLL 밖에서 호출할 수 있는 것은 별개다. 현재 GameFeature 소비 코드의 링크 실패를 재현한 결과는 아니며, BP 에셋 참조가 불가능하다는 뜻도 아니다.
- **제안**: 실제 콘텐츠가 소비할 타입·함수만 공개 API로 확정한다. 매니저 상태 조회·준비 완료 구독은 모듈 밖에서 사용 가능하게 export하고, 직접 상속시킬 타입만 export 범위를 넓힌다. 새 도메인 플러그인을 급히 추가하기보다 현재 허용된 `GameFeature → WxGame/도메인` 방향 안에서 작은 공개 면을 만든다.
- **확신도**: 높음

### 5. 🟡 임의 GameFeatureAction을 받지만 전체 수명주기를 제공하지 않는다

- **위치**: `Source/WxGame/Framework/WxExperienceDefinition.h:52`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:64`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:80`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:335`
- **범주**: 버그/정확성
- **문제**: 액션 배열은 일반 `UGameFeatureAction`을 허용하지만 실행은 Registering → Loading → Activating에서 끝나며, UE 5.8의 `OnGameFeatureActivated`를 호출하지 않는다. 종료도 Deactivating 직후 Unregistering을 호출하며 `OnGameFeatureUnloading`을 호출하지 않는다. pauser가 있어도 완료 콜백이 비어 있고 로그만 남긴다. 또한 의존 GameFeature의 비활성화를 먼저 요청한 뒤 Experience 소유 액션을 정리하고, 액션 자체도 활성 순서로 정리한다. 후속 액션이 앞선 액션이나 플러그인 자원을 사용하거나 Activated/Unloading/비동기 정리에 의존하는 경우 기대한 초기화·정리가 성립하지 않는다. 현재 AddComponents만의 항상 재현되는 결함으로 단정하지 않는다.
- **제안**: 지원할 액션 계약을 먼저 확정한다. 일반 GameFeatureAction 호환을 목표로 한다면 실제 활성화한 액션 목록을 보관하고 완료 훅 및 대응하는 종료 훅을 제공한다. 종속 액션은 역순으로 정리하고 그 뒤 플러그인 요청을 해제한다. 비동기 정리를 허용한다면 소유자 EndPlay 이전에 완료를 기다릴 수 있는 해제 단계가 필요하다. 동기 액션만 지원할 경우 호환 가능한 액션을 명시적으로 제한·검증한다.
- **확신도**: 높음(누락 경로), 중간(현재 콘텐츠에서의 영향)

### 6. 🟡 필수 컴포넌트 주입 실패도 Experience 성공으로 발행될 수 있다

- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:210`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:218`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:346`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:137`
- **범주**: 버그/정확성
- **문제**: 번들 취소를 완료와 같은 콜백으로 처리하며, AddComponents는 클래스 로드 실패 또는 지원하지 않는 컴포넌트이면 로그 후 건너뛴다. 매니저는 이 결과를 받지 않고 `Loaded`를 발행하여 GameMode의 스폰 게이트를 연다. 필수 컴포넌트가 잘못 지정되거나 쿠킹에서 빠졌을 때 기능이 누락된 상태로 시작할 수 있다. 반면 GameFeature 플러그인 이름·활성 실패는 이미 Failed로 처리하므로 그 부분은 문제에서 제외한다. 기본 폰이 이후 생성되므로 Loaded가 미래의 모든 Pawn 초기화 완료를 뜻할 수도 없다.
- **제안**: 우선 에디터 검증에서 주입 가능 타입·누락 참조를 검증하고, 필수 액션 의존성의 실패가 성공으로 합쳐지지 않게 결과 전달 경로를 둔다. `Experience 설정 적용 완료`와 `각 Pawn의 필수 컴포넌트 준비 완료`를 분리한다. 기존 ModularGameplay 초기화 상태/이벤트를 활용하여 폰별 기능은 자신의 의존성이 충족된 뒤 시작하도록 한다. 선택 액션을 허용한다면 필수/선택 구분을 명시한다.
- **확신도**: 높음(실패 입력에서의 코드 경로), 중간(현재 에셋에서의 발생 여부)

## 권장 책임 경계와 적용 순서

| 소유자 | 남길 책임 |
| --- | --- |
| `AWxGameMode` | URL/월드의 Experience 선택, 서버 스폰 게이트, 엔진 스폰 override 연결 |
| `UWxExperienceManagerComponent` | 선택된 구성 복제, 로드·액션 수명주기·성공/실패 상태 |
| 플레이어 스폰 정책 | Controller별 선택/기본 Pawn 해석 및 서버 시작 허용 판단 |
| FrontEnd/컨트롤러 기능 | 메뉴 선택 전달, 도착 화면·입력 보류·스트리밍 대기 |
| HUD 액션/UI 매니저 | UI 등록 핸들, 표시 우선순위, 자신의 등록 해제 |
| 콘텐츠 GameFeature | 기능별 액션·컴포넌트·에셋과 활성/비활성 정리 |

1. **먼저 Flow·HUD 결합을 분리한다.** GameMode의 엔진 연결점은 유지하고, 기존 액션 체계와 작은 스폰 정책 계약을 활용한다. GameMode를 없애는 작업이 목표는 아니다.
2. **콘텐츠 확장 계약을 정한다.** 필요한 API export, ReceiverClass 선택, 필수 의존성·준비 상태, 액션 수명주기를 정리한다. 실패·준비 판단을 새로운 전역 Subsystem 하나에 모두 모으지 않는다.
3. **작은 콘텐츠 하나로 경계를 검증한다.** `Plugins/GameFeatures/Wx<콘텐츠명>/`에 기능 하나를 옮기고 `ExplicitlyLoaded=true`, `BuiltInInitialFeatureState=Registered`로 구성한다. 기본 Experience는 허용된 이름 문자열로 활성화한다. 기본 게임/도메인에서 해당 플러그인 클래스·에셋을 역참조하지 않는다. 기본 Experience의 Actions/ActionSets가 GameFeature 소유 클래스를 직접 잡는 구성도 피한다.
4. **그 뒤 Character의 고정 조립을 단계적으로 줄인다.** `WxCharacterBase.cpp:34`부터 ASC·전투·장비·MetaHuman이, `WxPlayerCharacter.cpp:49`부터 Finisher·ItemUse·InputBuffer 등이 기본 서브오브젝트로 부착된다. Framework만 분리해도 이 기능들은 여전히 상시 포함된다. 기반 능력과 선택 콘텐츠를 구분하고, 선택 기능부터 초기화·해제·저장 상태 소유권을 함께 옮긴다. 모든 컴포넌트를 한 번에 동적 주입으로 바꾸지 않는다.

현재 `Config/DefaultGame.ini:48`과 `Config/DefaultGame.ini:49`의 Experience/ActionSet 스캔은 `/Game/Framework`에 한정되고, 저장소에는 `Plugins/GameFeatures/`가 없다. 따라서 GameFeature 구조는 준비된 확장점이며, 콘텐츠 독립 설치·제거까지 검증된 상태는 아니다. 기능 내부에 둘 에셋과 기본 게임에 남길 구성 에셋을 결정한 뒤 스캔·쿠킹 정책을 검증해야 한다.

## 후속 구현의 완료 기준

- 콘텐츠 플러그인을 비활성화·제외한 기본 Experience에서도 게임이 시작되고 기본 게임에서 콘텐츠 에셋의 역참조가 없다.
- 필수 컴포넌트 로드 실패는 성공으로 발행되지 않으며, 실패 상태가 호출자에게 전달된다.
- 컨트롤러/폰이 먼저 존재하는 경우와 액션이 먼저 등록된 경우 모두 필요한 기능이 한 번만 초기화된다.
- 활성화 → 월드 종료 → 재진입 및 다중 PIE 종료에서 컴포넌트·HUD·델리게이트가 남지 않는다.
- 네트워크를 지원 범위로 삼을 경우 서버/클라이언트 로드 순서 차이와 late join에서 폰별 준비 조건이 유지된다. 플레이어별 선택은 별도 서버 권위 전달 경로로 검증한다.
- 실제 C++ GameFeature 모듈이 공개 Framework API를 사용하는 Editor Development 빌드와 콘텐츠 포함 패키징을 통과한다.

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`.
- **훑은 파일**: `Source/WxGame/Framework/WxExperienceManager.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`.
- **교차 확인**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Config/DefaultGame.ini`. 설치된 UE 5.8의 `GameFeatureAction.h`, `GameFeaturesSubsystem.h`, `GameFrameworkComponentManager.cpp`에서 수명주기 훅과 주입 대상·복제 생성 제한을 확인했다.
- **미검토 / 한계**: BP 이벤트 그래프·에셋 참조 그래프·쿠킹 결과, 실행 중 로드 실패/네트워크/다중 PIE 재현은 검토하지 않았다. 소스를 변경하지 않았으므로 빌드는 실행하지 않았다. 기준 커밋 위 현재 작업 트리를 읽었으며 기존 `Docs/Programmer/module_review_WxGame.md`는 수정하지 않았다. 아래 소스 수는 Framework h/cpp만 센 값이다.

---
*문서 기준 커밋 `3025580b` · 리뷰일 2026-09-05 · 소스 16파일 — `/module-review`로 갱신*
