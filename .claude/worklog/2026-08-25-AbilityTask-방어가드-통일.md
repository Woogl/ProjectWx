# AbilityTask 방어 가드 통일

## 계획

### 목표
같은 폴더의 AbilityTask 4개가 방어 수준이 제각각이다. SlowTime만 `GetWorld()`를 무가드 역참조하고, 브로드캐스트 10곳 중 `ShouldBroadcastAbilityTaskDelegates()` 가드는 한 곳에만 걸려 있다. 두 가드를 폴더 전체에 하나의 규칙으로 통일한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `.../Task/WxAbilityTask_SlowTime.cpp` | `Activate`·`TickTask` World 널 가드, `OnFinished` 브로드캐스트 가드 | 수정 |
| `.../Task/WxAbilityTask_LockOnTarget.cpp` | `OnTargetLost` 4곳·`OnRetargetRequested` 1곳 브로드캐스트 가드 | 수정 |
| `.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `Activate`의 `OnCancelled` 2곳 브로드캐스트 가드 | 수정 |
| `.../Task/WxAbilityTask_WaitMoving.cpp` | `OnMovingChanged` 2곳 브로드캐스트 가드 | 수정 |

경로 접두사: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/`

### 접근 방식
- **규칙 1 — 전 브로드캐스트 지점에 엔진 헬퍼**: `ShouldBroadcastAbilityTaskDelegates()`는 엔진이 "델리게이트를 어빌리티 그래프로 쏘기 전에 부르라"고 못박은 계약이고 구현은 `Ability && Ability->IsActive()` 한 줄이다. 엔진은 틱 구동 태스크까지 전 지점에 건다. 가드가 실패해도 뒤따르는 `return`·`EndTask()`는 그대로 돈다.
- **래퍼·베이스 클래스 없음**: 엔진의 모든 태스크가 호출부에서 직접 거는 형태라, 자체 래퍼를 끼우면 통일이 아니라 이 폴더만 엔진과 어긋난다.
- **규칙 2 — SlowTime World 가드**: `Activate`·`TickTask` 모두 `if (!World) { EndTask(); return; }`를 앞세운다. `Activate`에서는 딜레이션을 걸기 **전**에 둔다 — 경과 시간을 못 재면 `TickTask`가 딜레이션을 걷을 수 없으므로 애초에 걸지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `.../Task/WxAbilityTask_SlowTime.cpp` | `Activate`·`TickTask`에 World 널 가드 추가, `OnFinished` 브로드캐스트 가드 | 수정 |
| `.../Task/WxAbilityTask_LockOnTarget.cpp` | 브로드캐스트 5곳에 가드 추가 | 수정 |
| `.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `OnCancelled` 2곳에 가드 추가 | 수정 |
| `.../Task/WxAbilityTask_WaitMoving.cpp` | `OnMovingChanged` 2곳에 가드 추가 | 수정 |

경로 접두사: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/`

### 구현·결정과 그 이유
- **엔진 헬퍼를 호출부에서 직접**: 래퍼 메서드나 공용 베이스 클래스를 만들지 않았다. 엔진의 모든 AbilityTask가 호출부에서 직접 거는 형태라, 자체 추상화를 끼우면 통일이 아니라 이 폴더만 엔진과 어긋난다.
- **가드 실패해도 정리는 실행**: 브로드캐스트만 접고 `return`·`EndTask()`는 그대로 둔다. 어빌리티가 죽었다고 태스크 정리를 건너뛰면 딜레이션·레티클이 남는다.
- **SlowTime 가드를 딜레이션 설정 앞에**: World가 없으면 경과 시간을 못 재고, 그러면 `TickTask`가 딜레이션을 걷을 수 없다. 걸고 나서 못 걷느니 애초에 걸지 않는다.
- **`Activate()` 브로드캐스트도 포함**: 어빌리티 발동 중 동기 호출이라 항상 참이지만, 예외를 두면 다시 들쭉날쭉해진다. 이 항목의 요구가 "전 지점 통일"이다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 브로드캐스트 지점 11곳 전부에 가드가 걸렸음은 grep 대조로 확인(파일별 가드 수 = 브로드캐스트 수). World 널 경로는 발현 창이 좁아 실측하지 않았고 컴파일만 확인했다.
