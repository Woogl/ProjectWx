# ComponentMove 닫힘 슬라이드 수정 + ST_Elevator 현실적 문 구성

## 계획

### 목표
엘리베이터를 "문 닫고 → 이동 → 문 열기"의 현실적 문 동작으로 만들기 위해 `ST_Elevator`를 평면 3-leaf + 내부 choreography로 구성한다. 그 전제로, 공용 `Wx Component Move`가 라이브에서 목표=아키타입(문 닫힘, offset 0)일 때 속도 0이 되어 슬라이드를 못 끝내는 버그를 고친다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_ComponentMoveInstanceData`에 런타임 `float MoveSpeed` 추가, ComponentMove 속도 모델 관련 주석 정정 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `ComponentMove::EnterState`를 비-const 인스턴스로, 라이브 분기에서 `MoveSpeed = (Target-현재상대위치).Size()/Duration` 산출. `Tick`의 속도를 `Instance.MoveSpeed`로 교체 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxElevator.h` | 클래스 doc의 ST 구조 설명(끝점 부모/문 자식 중첩)을 평면 leaf + 내부 choreography로 정정 | 수정(주석) |

### 접근 방식
- **ComponentMove 속도 = 시작→목표 실제 거리/Duration(EnterState 1회 산출)**: `Wx Component Spline Move`의 검증된 패턴(`:259-260`, 인스턴스 `MoveSpeed` 저장)을 그대로 따른다. 현행은 `Tick`이 매 틱 `LocalOffset.Size()/Duration`을 재계산해, 목표가 아키타입(offset 0)인 '닫기'에서 속도 0 → 데드락. 시작 위치 기준 실제 거리로 산출하면 닫기·중도 재진입까지 올바르게 슬라이드한다.
- **회귀 없음**: 기존 라이브 용법은 전부 아키타입에서 출발(열기/펼치기)이라 `|Target-시작| == |LocalOffset|` → 속도·타이밍 불변. `bReachNow`(초기진입/Duration≤0/이미 목표 시 즉시 스냅) 가드 유지로 거리 0 시 0-나눗셈 없음.
- **ST_Elevator(사용자 에디터 오서링)**: 최상위 leaf 3개(Enum Compare `State==자기값` enter), leaf 안에 Closing→Traveling→(Opening→)Idle 자식 시퀀스. 떠나기/정지는 기존 Door 2-state 패턴(Enum Compare 폴링 전이 + 정지 태스크 완료 판정 제외) 그대로. self-anchoring mover라 같은 층 전이는 즉시 완료로 collapse, 복원은 스냅 캐스케이드.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | ComponentMove 인스턴스 데이터에 런타임 `MoveSpeed` 추가, 속도 모델·개요·struct doc 주석 정정 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | `ComponentMove::EnterState` 비-const화 + 라이브 분기에서 `MoveSpeed=(Target-현재).Size()/Duration` 산출, `Tick`이 저장 속도로 보간(LocalOffset 기반 재계산 제거) | 수정 |
| `WxElevator.h` | 클래스 doc의 ST 구조(끝점 부모/문 자식 중첩) → 평면 leaf + 내부 choreography로 정정 | 수정(주석) |

### 구현·결정과 그 이유
- **ComponentMove 속도를 SplineMove와 동일 모델로 통일**: 닫기 목표는 아키타입(offset 0)이라 구 `LocalOffset.Size()/Duration` 속도가 0이 되어 슬라이드가 영영 안 끝났다. 시작(현재 위치)→목표 실제 거리를 Duration으로 나눠 EnterState에서 1회 산출하니, 닫기든 열기든 중도 재진입이든 모두 일정 속도로 도달한다. 같은 파일의 SplineMove가 쓰는 검증된 패턴이라 두 mover의 속도 의미가 일관된다.
- **회귀 없음**: 기존 라이브 용법은 전부 아키타입에서 출발(열기/펼치기)이라 `|Target-시작| == |LocalOffset|` → 속도·타이밍 불변. `bReachNow` 가드(초기진입/Duration≤0/이미 목표 즉시 스냅)를 유지해 거리 0일 때 0-나눗셈도 없다.
- **포기한 속성 1개**: 구현은 "재진입해도 줄어든 거리로 재계산 안 해 감속 없음"이라는 옛 속성을 명시적으로 버린다. ComponentMove를 슬라이드 도중 재진입하는 사용자는 없고(중도 재호출이 잦은 플랫폼은 SplineMove를 쓴다), 도어/뚜껑류는 단계마다 별도 자식 상태로 1회씩만 진입하므로 무해하다.
- **WxElevator.h 주석 정정**: 채택 구조가 "끝점 부모가 Spline Move, 문 자식"에서 "권위 State별 평면 leaf + 각 leaf 내부의 닫기→이동→열기 choreography"로 바뀌어, doc을 실제 설계에 맞췄다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **ST_Elevator 에디터 오서링(사용자)**: 평면 3-leaf(Closed/AtStart/AtEnd, enter=Enum Compare `State==자기값`) + 각 leaf 내부 `Closing → Traveling → (Opening →) Idle` 자식 시퀀스. 떠나기·정지는 기존 Door 2-state 패턴(Enum Compare 폴링 전이 + 정지 태스크 완료 판정 제외) 그대로. 바인딩: PlatformRoot/SplineComponent/DoorLeft·DoorRight/각 Interaction. 스플라인 포인트 0=Start,1=End. 문 열림 오프셋은 디자인값.
- **런타임 PIE 검증(사용자)**: 다른 층=닫기→이동→열기 순서, 같은 층(Closed→AtStart)=문 개폐만 보임(닫기·이동 collapse), 세이브/스트리밍 복원 시 저장 State의 끝점·문 포즈로 즉시 스냅, 멀티 PIE에서 클라가 복제 State 추종.
