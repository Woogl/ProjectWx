# 범용 메시 이동 StateTree 태스크 (컴포넌트 + LocalOffset)

## 계획

### 목표
Door/Elevator 의 문 여닫기 태스크(DoorPose, ElevatorDoorPose)를 컴포넌트와 로컬 오프셋을 인자로 받는 자체 완결형 범용 이동 태스크 `FWxStateTreeTask_ComponentMove` 하나로 통합한다. 이동 로직이 태스크로 들어가 어떤 기믹이든 메시를 옮길 수 있게 된다. 스플라인 플랫폼 이동(ElevatorMove)은 범위 외.

### 수정 범위
| 파일 | 수정 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h/.cpp` | `FWxStateTreeTask_ComponentMove`(순수 mover) 신설 | 신규 |
| `WxDoor.h/.cpp` | 문 이동 멤버 전부 제거(alpha/앵커/너비/pre-snap/DoorPose 의존 getter). State·SetDoorState·인터랙션 유지. WxGimmick 무변경 | 수정 |
| `WxElevator.h/.cpp` | 문 이동 멤버 제거(door alpha/앵커/너비), SnapVisualsToState 의 문 알파 제거. 플랫폼·ElevatorMove·SetElevatorState 유지 | 수정 |
| `WxElevatorStateTreeNodes.h/.cpp` | ElevatorDoorPose 삭제, `FWxStateTreeTask_ElevatorAdvance`(타이머 승급) 신설, ElevatorMove 유지 | 수정 |
| `WxDoorStateTreeNodes.*` | DoorPose 삭제(파일 비면 제거) | 삭제 |

### 접근 방식 (순수 mover + 에셋 전이)
- **ComponentMove(순수)**: 인스턴스 `FName TargetComponent` + `FVector LocalOffset` + `float Duration` + (런타임) `MoveSpeed`. 앵커=컴포넌트 아키타입 상대위치, 목표=앵커+offset. State 완전 무관, 항상 Running(도달 후 hold). WxGimmick/인터페이스 변경 없음.
- **스냅 vs 슬라이드**: 초기 진입(`SourceStateID` 무효)/Duration<=0/이미 목표면 스냅. 슬라이드는 EnterState 에서 `MoveSpeed = |목표-현재|/Duration` 저장(닫힘 offset=0 시 속도 0 되는 문제 회피), Tick 이 `VInterpConstantTo`.
- **전이 감지**: ST 에셋의 Enum Compare 전이 조건(`State != 현재상태 → Root 재선택`)으로 처리. 태스크가 State 를 안 읽는다.
- **엘리베이터 문 승급**: 신규 엘베 전용 `FWxStateTreeTask_ElevatorAdvance`(Duration 경과 후 서버 `SetElevatorState(PromoteToState)`). 플랫폼 `ElevatorMove`·`SetElevatorState` 는 그대로.
- **수동 단계**: ST_Door/ST_Elevator 에셋 재오서링(ComponentMove 배선·offset·Duration, Enum Compare 전이 조건). 사용자.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|

### 구현·결정과 그 이유

### 계획 대비 달라진 점

### 후속 과제
