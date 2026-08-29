# WxGame — 코드 리뷰

> Experience의 서버 선택·복제·클라이언트별 로드 경계와 GameFeature/액션 수명 관리는 대체로 명확하다. 이번에는 `Source/WxGame`의 README, Build.cs, Experience 정의·매니저·GameMode·GameFeature 액션과 그 내부 소비 지점에 한정해 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 존재하지 않는 GameFeature 이름을 필수 구성 누락으로 처리하지 않는다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:326`
- **범주**: 버그/정확성
- **문제**: `CollectGameFeaturePluginURLs`는 이름을 URL로 해석하지 못하면 오류만 기록하고 해당 항목을 목록에서 제외한다(331~338행). 이후 로드 수는 해석에 성공한 URL 수만으로 결정되므로(231행), 모두 찾지 못한 경우 즉시 `FinishExperienceLoad()`를 호출하고(231~235행), 일부만 찾지 못한 경우에도 남은 플러그인의 성공 뒤 Experience를 완료한다(263~274행). `GameFeaturesToEnable`에 적은 필수 기능이 빠져도 폰 스폰과 기본 지급 게이트가 열려 불완전한 세션이 시작된다.
- **제안**: URL 해석 실패를 활성 실패와 같은 실패 상태로 누적하고, 요청한 이름이 하나라도 해석되지 않으면 액션 실행·HUD 발행·완료 브로드캐스트를 막는다. 선택 기능을 허용할 필요가 있으면 데이터에 필수/선택 여부를 명시한다.
- **확신도**: 높음

### 2. 🔴 월드 종료 뒤에도 진행 중인 GameFeature 콜백이 Experience 액션을 실행할 수 있다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:59`
- **범주**: 객체 수명주기
- **문제**: `EndPlay`는 진행 중인 활성 요청을 해제하고 URL 배열을 비우지만(61행, 278~291행), `LoadState`와 `NumGameFeaturePluginsLoading`은 `LoadingGameFeatures` 상태로 둔다. 활성화 콜백은 `CreateUObject`로 등록돼 있어 컴포넌트가 아직 살아 있는 EndPlay 구간에 도착할 수 있으며(239~244행), 이때 콜백은 그대로 카운터를 줄인다(247~275행). 마지막 콜백이 성공이면 `FinishExperienceLoad()`가 종료 중인 월드에 액션을 활성화하고 HUD를 발행한다(293~323행). 또한 해제는 활성 완료 전 수행되므로, 늦게 성공한 플러그인이 비활성화되지 않고 남을 수 있다.
- **제안**: 종료 진입 플래그를 두고 늦은 콜백에서는 Experience 완료 경로를 절대 타지 않게 한다. 진행 중인 요청은 모든 콜백 회수 뒤에 해제하거나, 늦은 성공 콜백에서 해당 활성화를 즉시 정리해 요청 카운트와 실제 플러그인 상태를 일치시킨다.
- **확신도**: 높음

### 3. 🟡 실패 상태가 외부 소비자에게 노출되지 않아 이후 등록 콜백이 영구 대기한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:126`
- **범주**: 설계/구조
- **문제**: 실패 시 매니저는 `Failed`로만 전이하고 기존 대기 델리게이트를 비운다(265~271행). 그러나 `CallOrRegister_OnExperienceLoaded`는 실패 여부를 구분하지 않고 `IsExperienceLoaded()`가 거짓이면 새 델리게이트를 계속 저장한다(126~135행). `LoadState`는 private이고 실패 델리게이트도 없어, 실패 후 생성된 소비자는 로드 중·미확정·영구 실패를 구분하지 못한 채 콜백을 영구 대기한다.
- **제안**: 읽기 전용 로드 상태 또는 `OnExperienceLoadFailed`를 공개하고, `Failed` 상태의 등록은 즉시 실패 결과를 반환하거나 거절한다. GameMode나 UI가 복구 안내·재시도 정책을 구현할 수 있게 실패 원인도 전달한다.
- **확신도**: 높음

### 4. 🟡 동일 GameFeatureAction 인스턴스가 중복 등록되면 수명주기 훅을 중복 실행한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:342`
- **범주**: 버그/정확성
- **문제**: `CollectActions`는 Experience 본체와 모든 ActionSet의 액션 포인터를 그대로 누적한다(344~365행). 같은 액션 인스턴스가 둘 이상의 배열에 들어가도 중복을 제거하지 않으며, 두 데이터 에셋의 검증도 널 항목만 검사한다(`Source/WxGame/Framework/WxExperienceDefinition.cpp:21`, `Source/WxGame/Framework/WxExperienceActionSet.cpp:16`). 그 결과 `OnGameFeatureRegistering`·`OnGameFeatureLoading`·`OnGameFeatureActivating`과 종료 훅이 같은 인스턴스에 두 번 호출돼, 비멱등 액션은 중복 컴포넌트 요청·델리게이트 등록 또는 이중 해제를 일으킬 수 있다.
- **제안**: 액션 수집 시 포인터 기준으로 순서를 보존한 중복 제거를 적용하고, 같은 Experience 안의 중복 ActionSet/액션 참조는 데이터 검증 오류로 표시한다.
- **확신도**: 높음

### 5. 🟢 델리게이트 콜백 이름이 프로젝트 접두사 규칙과 다르다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:22`
- **범주**: 규칙 위반
- **문제**: `WxHandleDeactivationPauserCompleted`는 `FGameFeatureDeactivatingContext` 생성자에 콜백으로 전달되지만(70행), 프로젝트 규칙의 Delegate 콜백 `Handle` 접두사 대신 `WxHandle`로 시작한다.
- **제안**: `HandleDeactivationPauserCompleted`처럼 `Handle`로 시작하도록 이름을 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManager.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxExperienceDefinition.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`
- **미검토 / 한계**: Experience·ActionSet·GameFeature 데이터 에셋의 실제 조합, PrimaryAsset 스캔 설정, BP/WBP 내부 위젯·이벤트 그래프는 범위 밖이다. `Source/WxGame` 밖 도메인 플러그인과 UI 서브시스템의 내부 구현도 검토하지 않았다.

---
*문서 기준 커밋 `8cb9e492` · 리뷰일 2026-08-29 · 소스 66파일 — `/module-review`로 갱신*
