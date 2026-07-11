# ComponentSplineMove 이동 중 재트리거 시 vertex 스냅 제거

## 계획

### 목표
`Wx Component Spline Move`(엘리베이터 플랫폼 이동)가 라이브 진입 시 "가장 가까운 스플라인 포인트(끝점)"를 시작점으로 잡아, 플랫폼이 끝점 사이를 이동 중일 때 반대편으로 재트리거하면 현재 위치가 아니라 가까운 끝점으로 스냅됐다가 슬라이드하는 버그를 고친다. 시작점을 플랫폼의 실제 현재 스플라인 거리로 잡아 스냅 없이 현재 위치에서 곧바로 반전하게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.cpp` | `ComponentSplineMove::EnterState` 라이브 분기의 시작점을 `FindNearestSplinePointIndex`(vertex) → `GetDistanceAlongSplineAtLocation`(실제 위치)로 교체 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | 유일 사용처였던 `AtSplinePoint` 조건 노드가 이미 삭제돼 미사용이 된 `FindNearestSplinePointIndex` 헬퍼 제거 | 삭제 |
| `WxGimmickStateTreeNodes.h/.cpp` | 스테일 주석 정리(헤더 개요의 삭제된 `AtSplinePoint` 줄, "가장 가까운 포인트/끝점 주차 전제" 설명을 실제 동작에 맞게) | 수정(주석) |

### 접근 방식
- **시작점 = 실제 현재 스플라인 거리**: `Spline->GetDistanceAlongSplineAtLocation(Component->GetComponentLocation(), World)` 로 현재 위치의 스플라인 거리를 직접 구한다. vertex 양자화가 사라져 이동 중 반전도 현재 지점에서 바로 목표로 슬라이드한다. 끝점에 주차된 정상 왕복은 시작=끝점이라 기존과 동일(회귀 없음).
- **속도**: 기존 `MoveSpeed = |목표 - 시작|/Duration` 유지(제네릭·무가정). 이동 중 반전은 남은 거리를 Duration에 걸쳐 주파한다(등속 반전이 필요하면 이후 `GetSplineLength()/Duration` 변형 가능하나 이번엔 미채택).
- **헬퍼 제거**: `FindNearestSplinePointIndex` 는 삭제된 `AtSplinePoint` 조건이 유일 사용처였고 이 교체 후 완전 미사용이 되므로 함께 제거한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.cpp` | `ComponentSplineMove::EnterState` 라이브 시작점을 `GetDistanceAlongSplineAtLocation(현재 월드 위치)`로 교체, 미사용 `FindNearestSplinePointIndex` 헬퍼 제거, 속도 주석 정정 | 수정·삭제 |
| `WxGimmickStateTreeNodes.h` | 개요의 삭제된 `AtSplinePoint` 줄 제거, ComponentSplineMove 개요·struct doc·Duration/CurrentDistance 필드 주석을 "실제 현재 위치 시작"에 맞게 정정 | 수정(주석) |

### 구현·결정과 그 이유
- **시작점을 vertex → 실제 스플라인 거리로**: 기존은 라이브 진입 시 `FindNearestSplinePointIndex`로 가장 가까운 끝점을 시작으로 잡고 그 끝점으로 `SetWorldLocation` 해서, 이동 중(끝점 사이) 반대편 재트리거 시 플랫폼이 가까운 끝점으로 튕겼다. "정지 시 항상 끝점 주차"라는 전제가 이동 중 반전에서 깨진 것. 엔진 `GetDistanceAlongSplineAtLocation` 으로 현재 위치의 스플라인 거리를 직접 잡으니 양자화가 사라져 현재 지점에서 곧바로 반전한다. 끝점 주차 상태의 정상 왕복은 시작=끝점이라 값이 동일 → 회귀 없음.
- **속도는 남은 거리/Duration 유지(제네릭·무가정)**: 이동 중 반전은 남은 거리를 Duration에 주파(왕복보다 느린 복귀). 등속 반전이 필요하면 `GetSplineLength()/Duration`(2끝점 전제) 변형이 있으나, 무가정·최소변경을 택해 미채택.
- **헬퍼 제거**: `FindNearestSplinePointIndex`는 이미 삭제된 `AtSplinePoint` 조건이 유일 사용처였고, 이 교체로 완전 미사용이 되어 함께 제거. 관련 스테일 주석(개요의 AtSplinePoint 줄 등)도 정리.

### 계획 대비 달라진 점
- 계획대로. (속도 옵션은 승인 시 기본안=제네릭 남은거리/Duration 확정)

### 후속 과제
- **PIE 런타임 검증(사용자)**: 플랫폼 이동 도중 반대편 콘솔로 재트리거 시 스냅 없이 현재 지점에서 반전하는지, 끝점 주차 상태의 정상 왕복은 그대로인지 확인. 반전 복귀 속도(남은거리/Duration)가 체감상 느리면 등속 변형 재검토.
