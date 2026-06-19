# 기믹 Interaction StateTree 태스크 통합

## 계획

### 목표
Door·Elevator가 각자 가지던 StateTree "인터랙션 토글" 태스크를 모든 Gimmick 공용 단일 태스크(`Wx Gimmick Interaction`)로 통합하고, 토글 동작을 공통 부모 `AWxGimmick`에 둔다. 두 태스크는 구조가 동일하고(인스턴스 데이터 `bool` 하나, EnterState 1회 토글, 틱 없음) 결국 자기 액터의 `UWxInteractionComponent`들을 일괄 토글할 뿐이라 중복이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmick.h` | `void SetInteractionEnabled(bool)` 선언 | 수정 |
| `WxGimmick.cpp` | 모든 `UWxInteractionComponent` 순회 토글 구현 + include | 수정 |
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_GimmickInteraction`(+InstanceData) | 신규 |
| `WxGimmickStateTreeNodes.cpp` | 위 태스크 구현 | 신규 |
| `WxDoorStateTreeNodes.h/.cpp` | `FWxStateTreeTask_DoorInteraction`(+InstanceData) 삭제, 상단 주석 갱신 | 삭제·수정 |
| `WxDoor.h/.cpp` | `SetConsoleInteractionEnabled` 삭제, 주석 갱신 | 삭제·수정 |
| `WxElevatorStateTreeNodes.h/.cpp` | `FWxStateTreeTask_ElevatorInteraction`(+InstanceData) 삭제, 상단 주석 갱신 | 삭제·수정 |
| `WxElevator.h/.cpp` | `SetAllInteractionsEnabled` 삭제, 주석 갱신 | 삭제·수정 |

### 접근 방식
- **토글 소유처를 베이스로 승격**: `AWxGimmick`은 "상호작용 가능한 월드 오브젝트의 공통 부모"이므로, 자기 액터의 모든 `UWxInteractionComponent`를 일괄 토글하는 `SetInteractionEnabled(bool)`을 베이스에 둔다. Door(콘솔 1개)·Elevator(인터랙션 3개) 모두 "전체 토글" 의미라 기존 동작과 일치. 선택적 토글 요구가 생기기 전까지 non-virtual(방어적 virtual 금지).
- **단일 공통 태스크**: `FWxStateTreeTask_GimmickInteraction`이 `Context.GetOwner()`를 `AWxGimmick`으로 캐스트해 `SetInteractionEnabled` 호출. 기존 태스크 둘과 액터 메서드 둘은 제거.
- **에셋 이전은 수동**: CoreRedirects 미사용. 빌드 후 사용자가 ST_Door/ST_Elevator의 인터랙션 노드를 새 공통 태스크로 교체·재저장.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmick.h/.cpp` | `SetInteractionEnabled(bool)` 추가 — 자기 액터의 모든 `UWxInteractionComponent` 순회 토글, `WxInteractionComponent.h` include | 수정 |
| `WxGimmickStateTreeNodes.h/.cpp` | `FWxStateTreeTask_GimmickInteraction`(+InstanceData) — 모든 기믹 공용 인터랙션 토글 태스크 | 신규 |
| `WxDoorStateTreeNodes.h/.cpp` | `FWxStateTreeTask_DoorInteraction`(+InstanceData) 삭제, 상단 주석을 "노드 하나(DoorPose)"로 갱신 | 삭제·수정 |
| `WxDoor.h/.cpp` | `SetConsoleInteractionEnabled` 삭제, `DoorInteraction` 언급 주석 정리 | 삭제·수정 |
| `WxElevatorStateTreeNodes.h/.cpp` | `FWxStateTreeTask_ElevatorInteraction`(+InstanceData) 삭제, 상단 주석을 "노드 둘(Move·DoorPose)"로 갱신 | 삭제·수정 |
| `WxElevator.h/.cpp` | `SetAllInteractionsEnabled` 삭제 | 삭제·수정 |

### 구현·결정과 그 이유
- **토글을 베이스로 끌어올림**: 두 태스크의 유일한 차이는 호출 메서드였고 그마저 "자기 액터의 인터랙션 컴포넌트 일괄 토글"로 동일했다. `AWxGimmick`이 "상호작용 가능한 월드 오브젝트의 공통 부모"이므로, 컴포넌트 순회 토글을 베이스에 두면 종류별 메서드가 사라지고 한 태스크로 합쳐진다. 컴포넌트를 직접 순회하므로 Door(1개)·Elevator(3개) 모두 추가 배선 없이 동작한다.
- **공통 태스크는 베이스 타입에만 의존**: `Cast<AWxGimmick>` 으로 받아 어떤 기믹이든 재사용 가능. 기존 노드들의 "얇은 프리미티브만 호출" 패턴을 유지.
- **non-virtual 유지**: 현재 모든 기믹이 "전체 토글" 동일 의미라 가상 함수가 불필요. 선택적 토글이 필요한 기믹이 생기면 그때 승격.

### 계획 대비 달라진 점
- **Elevator 커스텀 조건 제거가 합쳐짐**: 작업 중 동일 파일들에서 별개 리팩터(커스텀 `StateIs` 조건 → 엔진 기본 Enum Compare)가 외부(IDE)에서 동시 진행 중이었다. Door는 이미 정리됐고 Elevator는 중간 상태(컴파일 불가)였는데, 진행 중 `ElevatorStateIs`가 외부 편집으로 양쪽 파일에서 마저 제거되어 일관 상태가 됐다. 사용자 승인대로 인터랙션 통합을 그 위에 적용. (인터랙션 통합 자체는 계획대로.)

### 후속 과제
- **에셋 수동 재작성(사용자, 에디터)**: ST_Door(2상태)·ST_Elevator(5상태)의 깨진 인터랙션 노드를 `Wx Gimmick Interaction`으로 교체, `bEnableInteraction` 재설정(Door: Close=true/Open=false, Elevator: 정지 true/전이 false) 후 저장.
