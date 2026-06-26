# 타겟–자신 거리 갱신 BTService

## 계획

### 목표
TargetActor와 AI 폰 사이 거리를 Blackboard float 키 `TargetDistance` 에 주기적으로 기록한다. 디자이너가 엔진 기본 `Blackboard` 데코레이터의 `Less/Greater` 비교만으로 근접·원거리 패턴을 데이터로 분기할 수 있게 해, 커스텀 거리 데코레이터를 불필요하게 만든다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxBlackboardKeys.h` | `TargetDistance`(float) 키 선언 + `GetSelfActor`/`SetTargetDistance`/`ClearTargetDistance` accessor 선언 | 수정 |
| `WxBlackboardKeys.cpp` | 위 키 정의 + accessor 구현(`UBlackboardKeyType_Float` 검증) | 수정 |
| `WxBTService_TargetDistance.h` | `UWxBTService_TargetDistance : public UBTService` 선언 | 신규 |
| `WxBTService_TargetDistance.cpp` | TickNode 에서 SelfActor·TargetActor 거리 계산 후 키 기록 | 신규 |

### 접근 방식
- **Service 가 거리를 Blackboard 에 운반**: 매 노드가 거리를 재계산하지 않고, 서비스가 `TargetDistance` float 키 한 곳에 써둔다. 분기는 엔진 기본 `Blackboard` 데코레이터(arithmetic 비교)가 담당 → 커스텀 데코레이터 0개.
- **Self 출처는 `SelfActor` 키**: 제어 폰 직접 참조 대신 Blackboard `SelfActor` 키를 읽는다(getter 신규 추가). Target 은 기존 `GetTargetActor`.
- **타겟 없음 처리**: Target 이 null 이면 `ClearTargetDistance` → stale 거리 제거. 기본 데코레이터의 `IsSet` 게이팅 가능.
- **진입 즉시 갱신**: `bCallTickOnSearchStart=true` 로 서브트리 진입 첫 평가에서 신선한 값을 읽게 한다. `Interval=0.1f`.
- **수평거리 옵션**: `bUse2DDistance`(기본 true) — Z 무시. 슬로프·캡슐 높이차로 인한 근접 판정 오류 방지.

### 디자이너 작업 (코드 외)
- `BB_Enemy` 에 Float 키 `TargetDistance` 추가(이름·타입 일치 필수).
- 전투 서브트리 컴포지트에 `WxBTService_TargetDistance` 부착.
- 분기는 기본 `Blackboard` 데코레이터로 `TargetDistance < N`(근접) / `>= N`(원거리).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxBlackboardKeys.h` | `TargetDistance`(float) 키 + `GetSelfActor`/`SetTargetDistance`/`ClearTargetDistance` accessor 선언 | 수정 |
| `WxBlackboardKeys.cpp` | 위 키 정의·accessor 구현, `BlackboardKeyType_Float` 검증 | 수정 |
| `WxBTService_TargetDistance.h` | `UWxBTService_TargetDistance : public UBTService` 선언 | 신규 |
| `WxBTService_TargetDistance.cpp` | TickNode 에서 Self·Target 거리 계산 후 키 기록 | 신규 |

### 구현·결정과 그 이유
- **거리 운반은 Service, 비교는 엔진 기본 데코레이터**: float 키에 거리를 써두면 디자이너가 기본 `Blackboard` 데코레이터의 arithmetic 비교(Less/Greater)로 임계값만 지정해 근접·원거리 분기를 구성할 수 있다. 커스텀 거리 데코레이터를 만들지 않는 이유.
- **Self 는 `SelfActor` 키에서**: 제어 폰 직접 참조 대신 Blackboard 규약을 따라 일관성 유지(승인 시 요청 반영). 기존엔 setter 만 있어 `GetSelfActor` 를 새로 추가.
- **타겟 없으면 Clear**: stale 거리가 남으면 원거리 패턴이 오발동할 수 있어, Target 부재 시 키를 비워 `IsSet` 게이팅과 호환되게 함.
- **진입 즉시 갱신(`bCallTickOnSearchStart`)**: 서브트리 진입 첫 평가에서 데코레이터가 신선한 값을 읽도록. 갱신 주기 `Interval=0.1f`.
- **수평거리 기본(`bUse2DDistance=true`)**: 슬로프·캡슐 높이차로 인한 근접 판정 오류를 막기 위함. 필요 시 3D 로 전환 가능.

### 계획 대비 달라진 점
- Self 출처를 "제어 폰 직접 참조" → "`SelfActor` 키 읽기" 로 변경(사용자 요청). 이에 따라 `GetSelfActor` accessor 추가가 계획 범위에 포함됨.

### 후속 과제
- 디자이너 측 에디터 작업 미완: `BB_Enemy` 에 Float 키 `TargetDistance` 추가, 전투 서브트리 컴포지트에 서비스 부착, 분기 데코레이터 구성.
- origin-to-origin 거리만 계산 — 큰 적의 근접 체감이 필요하면 캡슐 반지름 차감(edge-to-edge) 옵션을 후속으로 검토.
