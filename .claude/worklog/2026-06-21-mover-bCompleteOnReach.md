# mover 태스크에 bCompleteOnReach 추가 — StateTree 직렬 단계 전이 활성화

## 계획

### 목표
ST_Elevator의 Close→Move→Open 직렬(`On State Completed → Next State`)이 안 넘어가는 원인 해결. `ComponentMove`/`ComponentSplineMove`가 도달 후 영원히 `Running`(hold)이라 상태가 완료되지 않는 게 원인. 두 무버에 "도달 시 완료" 노드별 플래그를 더해, 구동 단계만 켜서 직렬 전이가 돌게 한다. 이 무버는 휴식·Door 유지에도 쓰이므로 완료는 blanket이 아니라 opt-in이어야 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `ComponentMove`/`ComponentSplineMove` InstanceData에 `bCompleteOnReach` 추가 + doc, 상단 개요 한 줄 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | 두 태스크 `EnterState`/`Tick`: 도달 시 플래그면 `Succeeded`, `GetDescription` 모드 표기 | 수정 |

### 접근 방식
- **노드별 플래그(기본 off)**: off면 도달 후 `Running`(현 동작, Door·휴식 무영향), on이면 도달/초기 스냅 시 `Succeeded` 반환 → 상태 완료 → `On State Completed` 발동.
- **도달 판정 재사용**: 기존 스냅/도착 분기(`ComponentMove`=`Equals(Target)`, `ComponentSplineMove`=`IsNearlyEqual`)를 그대로 쓰고, 도달 지점에서 한 분기만 추가.
- **안전성**: 기본 off로 회귀 0. on 노드는 그 상태에 `On State Completed` 전이가 있어야 함(없으면 트리 종료) — doc 명시.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `ComponentMove`/`ComponentSplineMove` InstanceData에 `bCompleteOnReach`(기본 false) 추가, 상단 개요에 완료/직렬 규칙 명시 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | 두 태스크 `EnterState`/`Tick`: 도달 시 `bCompleteOnReach`면 `Succeeded` 반환. `GetDescription`: on이면 "completes on reach" 표기 | 수정 |

### 구현·결정과 그 이유
- **노드별 플래그(기본 off)로 결정**: 같은 무버가 "전이 구동(완료)"과 "휴식·Door 포즈 유지(Running)" 두 역할을 겸하므로 blanket 완료는 불가. 대안 둘(완료 전용 태스크 신설=중복, 항상 완료+휴식 재구성=Door 재오서링)보다 blast radius가 작고 무브 로직이 한 곳에 남아(DRY) 버그·유지보수 면에서 우월하다고 판단. 기본 off라 Door·휴식 상태는 무영향(회귀 0).
- **도달 판정 재사용**: 기존 스냅/도착 분기를 그대로 쓰고, 도달 지점에서 플래그면 `Succeeded`만 추가. `ComponentMove`는 `Equals(Target)`, `ComponentSplineMove`는 `IsNearlyEqual` + 초기/스냅 분기에서 즉시 완료(복원 시 단계 캐스케이드).
- **가시화·규칙**: GetDescription에 모드 표기로 노드에서 보이게 하고, "켠 노드는 그 상태에 On State Completed 전이 필수(없으면 트리 종료)"를 헤더 doc에 명시.
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. WxWorld 컴파일·링크 완료, 경고는 변경과 무관한 엔진 C4996.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에셋(사용자)**: ST_Elevator의 Close(문 ComponentMove)·Move(플랫폼 ComponentSplineMove) 구동 노드에 `bCompleteOnReach=✔` + 각 상태에 `On State Completed → Next State` 전이. Open/Sealed 유지 무버는 off.
- **PIE 검증 미완**: Sealed→CallConsoleA 문 열림(self-skip), AtStart↔AtEnd 닫힘→이동→열림 순서, AtEnd 저장/복원, **Door 회귀 없음** 확인.
