# WxQuest — 코드 리뷰

> StateTree 러너와 저널 책임의 경계는 작고 명확하며, `WxCore`만 참조하는 모듈 규칙도 지킨다. README, 플러그인 설정, Build.cs와 Public/Private C++ 소스 14개를 검토했으며, StateTree 실행 중 재진입과 잘못된 에셋 조립에서 퀘스트 진행이 멈추거나 유실되는 경로를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 Blueprint 수주 진입점이 StateTree 재진입 안전 경로를 우회한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`
- **범주**: 버그/정확성
- **문제**: `UWxQuestLibrary::StartQuest`는 어느 Blueprint에서도 호출할 수 있지만 `ActivateQuest`를 직접 호출한다. 이 함수는 진행 중인 러너를 정지한 직후 같은 호출 스택에서 `SetStateTreeReference`와 `StartLogic`을 수행하며, 헤더도 StateTree 실행 콜스택 밖에서만 호출해야 한다고 명시한다. 따라서 StateTree 태스크가 호출한 Blueprint 등 러너 실행 중의 경로에서 이 노드를 쓰면 기존 퀘스트는 정지하고 새 에셋 교체·시작은 엔진 재진입 가드에 거부되어, 활성 퀘스트와 저널이 함께 사라질 수 있다.
- **제안**: 공개 Blueprint 진입점은 `RequestActivateQuest`로 위임해 항상 다음 틱에 활성화하고, 즉시 실행이 필요한 검증된 C++ 호출부만 `ActivateQuest`를 사용한다.
- **확신도**: 높음

### 2. 🟡 빈 로케이터 또는 영구 미해석 대상이 퀘스트를 무기한 정지시킨다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23`
- **범주**: 버그/정확성
- **문제**: 빈 `Target`을 경고만 남기고 `Running`으로 진입하며, 이후 `SyncFind`가 null이면 매 틱 계속 `Running`을 반환한다. 이 태스크는 상태 완료를 내는 대기 노드이므로 빈 로케이터, 삭제된 액터, 잘못된 경로가 들어가면 해당 상태와 단일 활성 퀘스트 체인이 끝나지 않는다. 액터 삭제·경로 변경의 경우에는 진단 로그도 남지 않는다.
- **제안**: 빈 로케이터는 진입 시 `Failed`를 반환해 조립 오류를 상태 전이로 드러내고, 미해석 상태는 허용 시간이 지난 뒤 한 번 경고하거나 실패 정책을 명시한다.
- **확신도**: 높음

### 3. 🟡 다음 틱 활성화 요청이 합류되지 않아 같은 프레임의 이전 요청이 뒤늦게 퀘스트를 덮어쓴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:48`
- **범주**: 버그/정확성
- **문제**: `RequestActivateQuest`는 타이머 핸들이나 대기 에셋을 보관하지 않고 호출마다 `SetTimerForNextTick`을 추가한다. 같은 프레임에 체인 태스크와 다른 수주 요청이 겹치면 예약 순서대로 여러 `ActivateQuest`가 실행되어 중간 퀘스트가 잠시 시작된 뒤 다음 요청에 즉시 정지되거나, 플레이어의 새 수주가 이전 체인 예약으로 덮일 수 있다. 중간 퀘스트의 진입 부수효과는 저널 상태만으로 되돌릴 수 없다.
- **제안**: 단일 대기 요청과 `FTimerHandle`을 멤버로 관리하고 새 요청마다 기존 예약을 취소하거나 마지막 요청으로 교체한다. 컴포넌트 종료 시에도 예약을 해제한다.
- **확신도**: 중간

### 4. 🟡 체인 태스크의 조립 오류가 런타임에서 식별되지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23`
- **범주**: 버그/정확성
- **문제**: `FWxStateTreeTask_StartNextQuest`는 오너에서 `UWxQuestComponent`를 찾지 못하면 `Failed`만 반환하고 로그를 남기지 않는다. 이 태스크는 완료 판정에서 제외되도록 설정되어 있어 반환값만으로는 상태 완료나 실패를 신뢰성 있게 알리지 못한다. 같은 조건을 처리하는 제목·목표 태스크는 경고 로그를 남기므로, 체인만 조용히 끊기면 빈 종점과 잘못된 러너 조립을 구분하기 어렵다.
- **제안**: 형제 태스크와 같은 수준의 `LogWxQuest` 경고를 남기고, 헤더 주석에서 완료 판정 제외 태스크의 반환 상태가 흐름 제어 신호가 아님을 분명히 한다.
- **확신도**: 높음

### 5. 🟢 런타임 생성 러너가 퀘스트 컴포넌트보다 긴 수명으로 남을 수 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:117`
- **범주**: 설계/구조
- **문제**: 러너는 `UWxQuestComponent`가 아니라 GameState를 Outer로 하여 등록하지만, 컴포넌트에 `EndPlay`나 `OnUnregister` 정리가 없다. Experience 또는 GameFeature 변경으로 퀘스트 컴포넌트만 제거되는 경로가 생기면 등록된 러너가 StateTree 실행과 델리게이트 바인딩을 유지할 수 있다.
- **제안**: 종료 훅에서 러너를 정지하고 델리게이트·다음 틱 예약을 해제한 뒤 `DestroyComponent`로 등록을 정리한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`
- **훑은 파일**: `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/README.md`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, 모듈 밖 소비자 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`
- **규칙 대조 결과(위반 없음)**: `WxQuest.Build.cs`와 `.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않는다. 소스·Build.cs 첫 줄의 저작권 표기, 델리게이트 콜백 `Handle` 접두, `BeginPlay`의 `Super::` 호출, Blueprint Function Library 안의 `BlueprintCallable`, StateTree 예외 사유가 적힌 `GetInstanceDataType()` 인라인 정의를 확인했다.
- **미검토 / 한계**: StateTree 에셋과 Blueprint 이벤트 그래프는 범위 밖이므로 재진입 호출과 같은 프레임 중복 예약이 현재 데이터에서 실제로 발생하는지는 확인하지 못했다. 멀티플레이 저널 복제는 README가 싱글/리슨 호스트 범위를 명시하므로 현 시점의 결함으로 분류하지 않았다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 14파일 — `/module-review`로 갱신*
