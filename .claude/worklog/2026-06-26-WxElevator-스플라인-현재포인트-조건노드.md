# WxElevator 스플라인 현재 포인트 조회 — StateTree 조건 노드 추가

## 계획

### 목표
ST_Elevator 에서 "플랫폼이 현재 몇 번째 스플라인 포인트에 있는가"를 컨디션으로 분기할 수 있게 한다. 판정은 플랫폼의 실제 월드 위치에서 가장 가까운 스플라인 포인트(기하)로 하며, 기믹 무관 제네릭 StateTree Condition 노드로 제공한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | 상단 개요 주석에 새 조건 노드 1줄 추가, `StateTreeConditionBase.h` include, `ComponentSplineMove` 뒤에 조건 노드(인스턴스 데이터 + 조건 구조체) 선언 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 익명 네임스페이스에 `FindNearestSplinePointIndex` 헬퍼 추출, `ComponentSplineMove::EnterState` 인라인 nearest 루프를 헬퍼 호출로 교체, 조건 노드 정의(`TestCondition` + `GetDescription`) 추가 | 수정 |

### 접근 방식
- **기하 판정 제네릭 조건 노드**: `FWxStateTreeCondition_AtSplinePoint`(DisplayName "Wx At Spline Point"). 인스턴스 데이터는 (TargetComponent, Spline, PointIndex, bInvert). `TestCondition` 은 대상 컴포넌트의 현재 위치에서 가장 가까운 스플라인 포인트 인덱스를 구해 PointIndex(클램프)와 비교하고 bInvert 로 토글. 멤버 저장 없는 순수 파생.
- **nearest 로직 공유**: `ComponentSplineMove::EnterState` 가 라이브 전이 시작점을 잡는 "가장 가까운 포인트" 루프와 동일하므로, 파일 로컬 헬퍼 `FindNearestSplinePointIndex` 로 추출해 태스크·조건이 공유(드리프트 방지). 기존 `GetMoveAnchor` 와 같은 익명 네임스페이스.
- **함수 아닌 조건 노드인 이유**: StateTree 컨디션은 액터 함수 게터에 안정적으로 바인딩하기 어렵고, 이 파일은 기믹 무관 동작을 제네릭 노드로 두는 패턴이라 일관.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | 개요 주석에 AtSplinePoint 조건 1줄, `StateTreeConditionBase.h` include, `ComponentSplineMove` 뒤에 `FWxStateTreeCondition_AtSplinePoint`(+인스턴스 데이터) 선언 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | 익명 네임스페이스에 `FindNearestSplinePointIndex` 헬퍼 추출, `ComponentSplineMove::EnterState` nearest 루프를 헬퍼 호출로 교체, 조건 노드 `TestCondition`·`GetDescription` 정의 추가 | 수정 |

### 구현·결정과 그 이유
- **기하 판정 + 멤버 저장 없음**: 현재 포인트는 플랫폼의 실제 위치에서 가장 가까운 스플라인 포인트로 그때그때 파생한다. 별도 상태 멤버를 두지 않아 권위/복원과 충돌할 여지가 없고, 정지 시 컴포넌트가 항상 포인트에 주차되는 기존 전제와 맞물려 안정적으로 끝점을 가리킨다.
- **nearest 로직 공유**: 동일 탐색이 `ComponentSplineMove` 의 라이브 시작점 산정과 새 조건 양쪽에 필요해, 파일 로컬 헬퍼로 한 곳에 두고 둘이 호출하게 했다(중복·드리프트 제거, 기존 `GetMoveAnchor` 와 같은 결).
- **함수 아닌 제네릭 조건 노드**: StateTree 컨디션은 액터 함수 게터 바인딩이 까다롭고, 이 파일은 기믹 무관 동작을 제네릭 노드로 두는 패턴이라 조건 노드가 일관·재사용에 유리. `bInvert` 로 부정 검사도 한 노드로 처리(엔진 Compare 관용구).
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. WxWorld 컴파일·링크 완료, 경고는 변경과 무관한 엔진 C4996 뿐.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에디터 재오서링(사용자)**: `Content/WorldObject/Gimmick/ST_Elevator.uasset` 의 분기할 상태/전이에 `Wx At Spline Point` 조건 추가 → `TargetComponent`=`PlatformRoot`, `Spline`=`SplineComponent`, `PointIndex`=대상 끝점 인덱스.
- **PIE 검증(사용자)**: 정지 시 해당 포인트에서 true, 반대 끝점에서 false 확인(이동 중에는 중점에서 nearest 전환 — 설계상 수용).
