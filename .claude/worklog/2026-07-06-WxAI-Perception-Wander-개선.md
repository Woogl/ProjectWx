# WxAI Perception/Wander 개선 (#2, #3, #5)

## 계획

### 목표
WxAI 분석에서 나온 3가지를 고친다 — Perception의 CharacterMovement null 미체크(#2), Wander 감속을 Patrol과 통일하고 주석을 실제 동작과 일치(#3), Wander가 navmesh를 벗어나지 않도록 안전화(#5).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp` | `SetTargetActor` 회전 모드 발행부에서 CharacterMovement 플래그 쓰기만 `if (Movement)`로 가드, 포커스 설정/해제는 항상 적용 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h` | `MoveSpeedMultiplier` 주석 정정, `LookAheadDistance` UPROPERTY·`CachedMaxWalkSpeed` 멤버·`OnTaskFinished` 선언 추가 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` | 감속을 MaxWalkSpeed 클램프/복원(Patrol 방식)으로 전환, TickTask에서 진행 방향 navmesh 투영 후 안전할 때만 이동 입력 | 수정 |

### 접근 방식
- **#2 방어 코드 일관화**: `WxBTTask_Patrol.cpp`의 `if (UCharacterMovementComponent* Movement = ...)` 패턴과 맞춘다. 포커스는 Movement 불필요하므로 분리.
- **#3 감속 통일**: 입력 스케일 대신 `MaxWalkSpeed *= MoveSpeedMultiplier`로 클램프하고 `OnTaskFinished`(모든 종료 경로)에서 복원. `bNotifyTaskFinished=true`. AddMovementInput은 scale 1.0.
- **#5 navmesh 안전**: TickTask에서 `Pawn + MoveDirection * LookAheadDistance`를 `UNavigationSystemV1::ProjectPointToNavigation`(낮은 수직 extent)으로 투영, navmesh 위일 때만 입력. 앞이 비었으면 그 틱 이동 생략하고 Duration까지 제자리 대기.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp` | `SetTargetActor`에서 포커스 발행은 그대로 두고 CharacterMovement 플래그 쓰기만 `if (Movement)`로 가드 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h` | `MoveSpeedMultiplier` 주석 정정, `LookAheadDistance` UPROPERTY·`CachedMaxWalkSpeed` 멤버·`OnTaskFinished` 선언 추가 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` | 감속을 MaxWalkSpeed 클램프/복원으로 전환, TickTask에 navmesh 투영 게이팅 추가, include 3종 추가 | 수정 |

### 구현·결정과 그 이유
- **#2 포커스와 Movement 분리**: `SetFocus`/`ClearFocus`는 MovementComponent가 없어도 유효하므로 항상 적용하고, 회전 모드 플래그 쓰기만 가드했다. 조기 return으로 통째 가드하면 Movement 없는 폰에서 포커스까지 누락돼 동작이 달라지기 때문.
- **#3 감속을 MaxWalkSpeed 클램프로 통일**: 입력 스케일 방식은 CMC 아날로그 입력에만 통하고 주석과도 어긋났다. Patrol과 동일하게 `MaxWalkSpeed *= 배율` 후 `OnTaskFinished`에서 복원(도착·중단·실패 모든 경로 커버)해 두 태스크의 "감속" 의미를 일치시켰다. AddMovementInput은 scale 1.0.
- **#5 navmesh 게이팅은 조기 종료가 아닌 틱 스킵**: 앞이 navmesh 밖이면 그 틱 이동만 생략하고 Duration은 그대로 소진한다. 조기 Succeeded로 끝내면 Wander 슬롯 재진입이 반복돼 방향 재추첨 스래싱이 생기므로, 제자리 대기로 시간 박스 의미를 보존했다. 투영 수직 extent를 150으로 낮춰 아래로 꺼지는 지형을 안전으로 오판하지 않게 함.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 분석에서 남긴 #1(타겟 재선택 패스), #4(센스 기본 구성), #6(RandomChoice+조건 데코 한계 문서화)는 미착수.
- Wander가 벽/낭떠러지를 마주치면 그 방향으로는 이번 사이클 동안 정지만 한다. 필요 시 향후 방향 선택 단계에서 navmesh 가능 방향만 고르도록 개선 여지.
