# WxBTTask_ActivateAbility 후보 순회에 어빌리티 리스트 스코프 락 적용

## 계획

### 목표
`UWxBTTask_ActivateAbility::ExecuteTask` 가 `ActivatableAbilities` 를 락 없이 순회하며 루프 안에서 `TryActivateAbility` 를 호출한다. 활성화가 실패해도 순회는 계속되는데, 실패 통지를 받은 게임 코드가 어빌리티를 부여·제거하면 순회 중인 참조가 무효화된다. 엔진 표준 스코프 락으로 막는다. `Docs/Programmer/module_review_WxAI.md` 발견 12(🟢) 대응.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp` | 후보 순회 루프를 `FScopedAbilityListLock` 스코프로 감싸고, 가정에 기대던 주석 3개를 락 근거 주석으로 대체 | 수정 |

헤더·시그니처 변경 없음. `FScopedAbilityListLock` 은 이미 포함된 `AbilitySystemComponent.h` 경유로 가시 범위라 추가 include 불필요.

### 접근 방식
- **위험의 실체**: 엔진 UE 5.8 기준 `TryActivateAbility` 자체는 리스트 락을 걸지 않는다(`AbilitySystemComponent_Abilities.cpp:1604`). 락은 `InternalTryActivateAbility` 안에서 시작해(`:1724`) 그 함수가 리턴할 때 풀리고, 그 순간 대기 중이던 부여/제거가 일괄 반영된다. 실패 경로에서도 `NotifyAbilityFailed` → `AbilityFailedCallbacks.Broadcast` 로 게임 코드가 돌며(`:1673` 은 아무 락도 없는 상태), 그 안에서 Give/Clear 가 일어나면 `Items` 가 Add·`RemoveAtSwap` 되어 이터레이터가 무효화된다. 후보가 하나뿐이어도 그 하나가 실패하면 나머지 배열을 계속 훑으므로 태그 유니크 여부와 무관하다.
- **엔진·자사 선례 그대로**: 엔진 입력 경로(`AbilityLocalInputPressed`)와 2026-07-31 에 고친 `UWxAbilitySystemComponent` 입력 라우팅이 같은 순회에 같은 락을 건다. 순회 중 활성화를 락 없이 하는 곳은 이 BT 태스크 하나만 남았다.
- **매크로 대신 직접 선언**: `ABILITYLIST_SCOPE_LOCK()` 은 `*this` 로 전개되어 ASC 멤버 함수 전용이다. BT 태스크는 외부 호출자라 `FScopedAbilityListLock ActiveScopeLock(*ASC);` 로 쓰되 변수명은 엔진과 맞춘다.
- **락을 루프에만 거는 이유**: 루프 뒤의 핸들 재조회·`IsActive()` 판정은 대기 부여/제거가 반영된 뒤 이뤄져야 한다. 명시적 블록으로 루프만 감싸면 flush 시점이 기존(=`TryActivateAbility` 리턴 시 내부 락 해제)과 같아 동작이 동일하다.
- **`CandidateHandle` 임시 변수 제거**: 락이 있으면 호출 후에도 `IterSpec` 이 유효하므로 "호출 전에 핸들을 캡처한다" 는 우회와 그 근거 주석들이 필요 없어진다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp` | 후보 순회 루프를 `FScopedAbilityListLock` 블록으로 감싸고, 임시 핸들 변수와 가정 기반 주석 3개 제거 | 수정 |

### 구현·결정과 그 이유
- **가정 대신 계약으로**: 기존 코드는 "성공 시 즉시 break", "실패는 목록을 안 바꾼다" 는 두 가정 위에 안전을 세우고 그 근거를 주석 세 곳에 나눠 적어 두었다. 두 번째 가정은 GAS 가 보장하지 않아 언제든 무너질 수 있고, 주석은 엔진 내부 동작을 단정하는 만큼 낡기 쉬웠다. 락 한 줄로 보장을 코드가 지게 하고 주석은 한 곳으로 모았다.
- **행동 변화 없음을 우선**: 락 범위를 루프로 한정해, 대기 부여/제거가 반영되는 시점이 기존과 같도록 했다. 함수 끝까지 끌면 곧 제거될 스펙에도 종료 델리게이트를 걸게 되어 의미가 달라진다.
- **재조회 블록은 유지**: 활성화 도중 배열이 재할당될 수 있다는 설명과 동기 종료 감지 로직은 락 도입 뒤에도 그대로 유효하다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- PIE 실동작 확인은 미실시 — 컴파일 검증(WxEditor Development, 성공)까지만 했다. 락 추가 외에 제어 흐름 변경이 없어 회귀 여지가 낮다고 판단했다.
- 이번 변경으로 방어되는 시나리오(어빌리티 활성화 실패 콜백에서 Give/Clear)는 현재 프로젝트에 구독자가 없어 재현 자체가 불가능하다. 향후 실패 콜백을 쓰게 되면 이 락이 전제가 된다.
