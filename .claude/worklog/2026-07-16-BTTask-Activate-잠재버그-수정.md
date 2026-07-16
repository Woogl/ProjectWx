# WxBTTask_ActivatePattern / ActivateAbility 잠재 버그 수정

## 계획

### 목표
`WxBTTask_ActivatePattern` 점검에서 찾은 세 결함(#1 댕글링 포인터, #2 이동 동기완료 시 InProgress 영구정지, #3 Abort 시 과광범위 취소)을 제거한다. #1·#3은 복사원인 형제 노드 `WxBTTask_ActivateAbility`에도 동일 존재하므로 함께 고친다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivatePattern.cpp` | #1 핸들 재조회, #2 Idle 동기완료 감지, #3 CancelAbilityHandle | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp` | #1 핸들 재조회, #3 CancelAbilityHandle | 수정 |

### 접근 방식
- **#1 (댕글링 제거)**: `GetActivatableAbilities()`에서 `FGameplayAbilitySpec*`를 들고 있지 않고 매칭 스펙의 **핸들만** 캡처한다. `TryActivateAbility(Handle)` 성공 후 `ASC->FindAbilitySpecFromHandle(Handle)`로 재조회해 `IsActive()`를 판정한다. 활성화 도중 어빌리티 부여/제거로 내부 `TArray`가 재할당되어도 안전.
- **#2 (동기완료 감지, ActivatePattern 전용)**: `RequestSuccessful` 케이스에서 `MoveTo` 반환 직후 `AIController->GetPathFollowingComponent()->GetStatus()`가 이미 `Idle`이면 이동이 동기 완료된 것이므로(콜백 유실) 델리게이트를 떼고 즉시 `BeginActivateAbility`. 정상 비동기 이동은 `Moving`/`Waiting`이라 무영향.
- **#3 (정확한 취소)**: `AbortTask`의 발동 페이즈에서 `CancelAbilities(&Tags)` 대신 `CancelAbilityHandle(ActivatedHandle)`. `CleanUp()`이 `ActivatedHandle`을 리셋하므로 CleanUp 전에 핸들을 캡처. 델리게이트 선(先)해제 순서(재진입 방지)는 유지.

- 헤더/시그니처 변경 없음. 필요한 심볼(`FindAbilitySpecFromHandle`, `GetPathFollowingComponent`, `EPathFollowingStatus`, `CancelAbilityHandle`)은 기존 include로 이미 가시 범위.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivatePattern.cpp` | #1 핸들 재조회, #2 `RequestSuccessful` 시 Idle 동기완료 감지, #3 `CancelAbilityHandle` | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp` | #1 핸들 재조회, #3 `CancelAbilityHandle` | 수정 |

### 구현·결정과 그 이유
- **#1 핸들 캡처 후 재조회**: `TryActivateAbility`가 활성화 도중 어빌리티 부여/제거로 `ActivatableAbilities` 배열을 재할당하면 활성화 전 `Spec*`가 댕글링된다. 스펙 포인터 대신 핸들만 들고, 활성화 후 `FindAbilitySpecFromHandle`로 재조회해 `IsActive()` 판정 → use-after-free 제거. 동기종료 판정 로직은 재조회한 `ActiveSpec` 기준으로 동일 유지.
- **#2 Idle 감지로 발동**: `MoveId`는 `MoveTo` 반환 후에만 나와 플래그 세팅이 콜백보다 늦을 수밖에 없다. 순서 교체 대신, 반환 시점 PathFollowing 상태가 이미 `Idle`(=동기 완료로 콜백 유실)이면 그 자리에서 발동하도록 방어. 정상 비동기 이동은 `Moving`/`Waiting`이라 무영향.
- **#3 핸들 기반 취소 + 선(先)캡처**: `CancelAbilities(&Tags)`는 동일 태그의 다른 인스턴스까지 취소하므로 `CancelAbilityHandle(ActivatedHandle)`로 정확히 이 Task가 발동한 것만 취소. `CleanUp()`이 `ActivatedHandle`을 리셋하므로 CleanUp 전에 핸들을 캡처. 델리게이트 선해제 순서(재진입 방지)는 유지.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 정적 추적으로 경로별 동작(성공/실패/Abort/동기완료)을 재확인했고 WxEditor(Development) 빌드는 성공. 런타임에서 동기 완료 경로(#2)는 재현이 드물어 실측 검증은 미수행.

---

## 후속 수정 — 동일 태그 다중 어빌리티 대비 선택 로직

### 계획
- **목표**: 현재는 어빌리티 태그가 유니크하지만 향후 중복 가능성을 대비. 두 노드의 어빌리티 선택을 "첫 매치 하나만 시도"에서 "매칭 후보를 순회하며 첫 발동 성공을 채택"으로 강화.
- **수정 범위**: `WxBTTask_ActivatePattern.cpp`(`BeginActivateAbility`), `WxBTTask_ActivateAbility.cpp`(`ExecuteTask`) — 선택 블록 교체. 헤더/시그니처 변경 없음, temp 컨테이너 없음.
- **접근**: 단일 루프에서 후보마다 바로 `TryActivateAbility` 시도, 첫 성공에서 `break`. 채택 핸들은 호출 전에 캡처(성공 후 dangling `IterSpec` 접근 방지). 실패(`CanActivate` 게이트)는 `ActivatableAbilities`를 바꾸지 않으므로 계속 순회 안전, 성공은 즉시 break라 재할당돼도 이터레이터 재사용 없음.

### 완료
- **수정 파일**: `WxBTTask_ActivatePattern.cpp`(`BeginActivateAbility`), `WxBTTask_ActivateAbility.cpp`(`ExecuteTask`) — 선택 블록을 단일 루프 "후보 순회 → 첫 성공 채택"으로 교체.
- **결정·이유**: 사용자 지시에 따라 후보 사전 수집(temp `TArray`) 대신 **단일 루프 인라인 시도**를 채택. 첫 성공에서 break하고 채택 핸들을 호출 전에 캡처하므로, 활성화로 인한 배열 재할당 시에도 dangling `IterSpec` 접근/이터레이터 무효화가 없다. 실패 시 배열 불변이라 다음 후보로 안전하게 진행. `feedback_no_unnecessary_abstraction`와도 부합.
- **검증**: WxEditor(Development) 빌드 성공(EXIT CODE 0). 정적 추적으로 후보 1개(기존 동일)/다수 중 첫 후보 실패→다음 채택/전부 실패→Failed 경로 확인.
- **계획 대비**: 계획대로.
