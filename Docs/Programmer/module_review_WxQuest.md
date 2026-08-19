# WxQuest — 코드 리뷰

> 15파일 규모의 작고 응집도 높은 모듈이다. 권위 경계·저널 수명·재진입 회피 같은 까다로운 지점이 헤더 주석과 README 에 이미 정확히 문서화돼 있어 전반적으로 건강하며, 남은 지적은 "엔진이 조용히 거절하는 실패 경로를 코드가 성공으로 취급한다"는 계열에 몰려 있다. 커버리지: 모듈의 15개 소스 전부를 읽었고, 판정 근거를 위해 엔진 `UStateTreeComponent`·`FStateTreeReference`·`UGameFrameworkComponentManager` 구현과 `Source/WxGame` 의 소비처(뷰모델·AddComponents 액션)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 에셋 교체 거부를 성공으로 취급해 이전 퀘스트를 다시 시작한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:35`
- **범주**: 버그/정확성
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 세 호출의 반환값을 아무도 보지 않는다. 엔진 `UStateTreeComponent::SetStateTreeReference` 는 현재 트리가 Running 이면 경고 로그만 남기고 **교체 없이 반환**하며, 같은 파일의 `StopLogic` 은 `bIsRunning` 이 이미 false 면 `Context::Stop()` 까지 건너뛰고 조기 반환한다. 즉 정지가 실제로 관철되지 못한 상황에서 `StartLogic()` 이 그대로 이어지면, 요청한 퀘스트가 아니라 **직전 퀘스트가 처음부터 재시작**되고 저널도 그 퀘스트로 다시 채워진다. 러너 실행 콜스택 안에서의 호출이 대표적인 경로인데(엔진 `StopLogic` 은 재진입 시 정지를 프레임 끝으로 미룬다), 이는 `RequestActivateQuest` 로 피하도록 문서화만 돼 있고 코드로 강제되지 않는다. 증상은 로그 한 줄(`LogStateTree` Warning)뿐이라 "왜 다른 퀘스트가 켜졌는지" 추적이 어렵다.
- **제안**: `SetStateTreeReference` 직후 러너의 실제 참조가 요청한 에셋인지 확인하고, 어긋나면 `StartLogic` 을 생략한 뒤 `LogWxQuest` Error 로 남긴다(잘못된 호출 지점을 즉시 지목할 수 있다).
- **확신도**: 중간

### 2. 🟡 도달 대상이 비면 퀘스트가 경고 한 줄만 남기고 영구 정지한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23`
- **범주**: 버그/정확성
- **문제**: 로케이터가 비어 있으면 `EnterState` 가 경고를 남기고도 `Running` 을 반환하고, `Tick` 은 `SyncFind` 가 계속 null 을 주므로 영원히 `Running` 을 반환한다(`WxStateTreeTask_WaitMoveToTarget.cpp:48`). 결과는 상태 완료 불가 → 그 퀘스트가 되돌릴 방법 없이 멈추는 소프트락이며, 다음 퀘스트 체인까지 함께 죽는다. 같은 범주의 "잘못된 조립"을 나머지 세 태스크는 전부 `Failed` 로 즉시 드러내므로(`WxStateTreeTask_SetQuestObjective.cpp:23`, `WxStateTreeTask_SetQuestTitle.cpp:23`, `WxStateTreeTask_StartNextQuest.cpp:21`) 실패 정책이 이 태스크에서만 갈라진다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 를 반환해 다른 태스크와 정책을 맞춘다(대상이 스트리밍 아웃돼 일시적으로 해석되지 않는 경우는 지금처럼 대기 유지). 의도적으로 대기를 고수한다면 최소한 태스크의 `IsDataValid`/`GetDescription` 이 아니라 에셋 검증 단계에서 걸리도록 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 헤더 주석이 "경고를 남긴다"를 명시적 선택으로 적어 두었다)

### 3. 🟡 런타임 생성한 러너 컴포넌트에 대칭 정리가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:117`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 오너에 `UStateTreeComponent` 를 만들어 `RegisterComponent` 까지 하고 델리게이트를 걸지만, `EndPlay`·`OnUnregister` 오버라이드가 없어(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:79` 의 유일한 오버라이드가 `BeginPlay`) 소유권이 한 방향으로만 성립한다. 이 컴포넌트는 코드가 아니라 주입으로 붙고, 프로젝트 자신의 주입 액션이 GameFeature 비활성 시 요청 핸들을 놓아 주입 컴포넌트를 파괴한다(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:78` → 엔진 `UGameFrameworkComponentManager::DestroyInstancedComponent`). 반면 러너는 매니저가 추적하지 않으므로 GameState 에 그대로 남아 계속 틱하며, 그 안의 퀘스트 태스크는 이제 컴포넌트를 못 찾아 매 진입 경고 후 `Failed` 를 낸다. 현재는 비활성이 `UWxExperienceManagerComponent::EndPlay`(월드 티어다운) 에서만 일어나 실피해가 없는 잠복 결함이지만, 인게임 Experience 교체가 생기는 순간 고아 러너로 드러난다.
- **제안**: `EndPlay`(또는 `OnUnregister`)에서 `StopLogic` 후 러너를 `DestroyComponent` 하고 `OnStateTreeRunStatusChanged` 바인딩을 해제한다.
- **확신도**: 중간

### 4. 🟢 사용하지 않는 빌드 의존 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 전체에 `FGameplayTag`·태그 헤더 사용이 한 건도 없다. (같은 목록의 `WxCore` 도 미사용이지만 전 플러그인이 일괄로 싣는 프로젝트 관례이므로 별건으로 보지 않는다.)
- **제안**: 항목을 지운다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, 나머지 `Public/` 헤더 전부, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **참고로 읽은 모듈 밖 코드**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(저널 소비처), `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`(부착·회수 경로)
- **미검토 / 한계**:
  - 규칙 점검 결과 위반 없음 — 저작권 첫 줄·`Wx` prefix·`Handle` 콜백 prefix·`Super::BeginPlay` 호출·`BlueprintCallable` 사용처(BP Function Library 한 곳)·람다 부재 모두 충족. 태스크 헤더의 `GetInstanceDataType()` 인라인 정의는 각 헤더가 코딩 규칙 6 의 예외임을 명시해 두어 지적하지 않았다. `WxCore` 외 Wx 플러그인 참조도 없다.
  - 저널 미복제(클라이언트에 퀘스트 UI 가 뜨지 않음)는 헤더 주석이 v1 싱글/리슨 호스트 전제로 명시한 기지의 보류 사항이라 발견으로 올리지 않았다. 멀티 정책이 정해지면 별도 검토 대상이다.
  - 퀘스트 StateTree 에셋(`/Game/Quest/**`)의 상태 구성·링크 값은 BP/에셋 영역이라 범위 밖이다. 특히 "상태마다 판정 태스크 최소 1개" 규약의 실제 준수 여부는 코드로 확인할 수 없다.
  - `FUniversalObjectLocator::SyncFind` 를 캐시 없이 매 틱 도는 비용은 경로 조회 수준으로 판단해 성능 지적에서 제외했다(실측하지 않음).

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 15파일 — `/module-review`로 갱신*
