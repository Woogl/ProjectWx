# WxBTTask_Wander 의 TotalTime 중복 상태 제거

## 계획

### 목표
`UWxBTTask_Wander::TotalTime` 이 `Duration` 의 복사본일 뿐이라 종료 판정에 상태를 하나 더 들고 있을 근거가 없다. 동작은 그대로 두고 중복 상태만 걷어낸다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h` | `TotalTime` 선언 삭제 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` | `TotalTime = Duration;` 삭제, 종료 판정을 `ElapsedTime >= Duration` 으로 | 수정 |

### 접근 방식
- **`Duration` 직접 비교**: 실행 중 `TotalTime` 을 바꾸는 곳이 없어 항상 두 값이 같다. 노드가 실행마다 인스턴스화돼도 `Duration` 은 아키타입 값이 그대로 복사돼 들어오므로 판정식이 동일하다.
- **`ElapsedTime` 은 유지**: 실제로 누적되는 상태이고, 실행 시작 시 0 초기화도 이전 실행의 누적을 지우는 역할이 있다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h` | `TotalTime` 멤버 삭제 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` | 복사 대입 삭제, 종료 판정을 지속 시간 프로퍼티와 직접 비교 | 수정 |

### 구현·결정과 그 이유
- **중복 상태를 지운 이유**: 두 값이 나뉘어 있으면 읽는 사람이 "실행 중에 달라질 수 있나" 를 확인하러 가야 한다. 실제로는 대입 한 번뿐이라 확인할 게 없는데도 그 비용만 남아 있었다.
- **판정식이 동일한 근거**: 노드가 실행마다 인스턴스화되지만 지속 시간은 에디터 노출 프로퍼티라 아키타입 값이 그대로 복사돼 들어온다. 인스턴스마다 유효하므로 비교 대상만 바뀌고 결과는 같다.
- **누적 시간은 유지**: 이쪽은 매 틱 실제로 자라는 상태고, 실행 시작 시 0 초기화도 인스턴스가 재사용될 때 이전 실행의 누적을 지우는 역할이 있다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. WxEditor(Development) 빌드 성공.
